/**
 * @file    HairCollisionSystem.h
 * @brief   Collision detection & response for hair strand simulation.
 *
 * Implements two mandatory collision layers:
 *
 *  1. Hair-vs-Body (capsule primitives)
 *     Hair particles are tested against a set of analytic capsules that
 *     approximate the head, neck, and shoulders.  Penetrating particles are
 *     pushed out along the surface normal and their Verlet "previous position"
 *     is adjusted to damp the rebound (friction model).
 *
 *  2. Hair-vs-Hair (inter-strand repulsion)
 *     A grid-accelerated broad phase groups nearby particles.  Close pairs
 *     from *different* strands receive a soft repulsion impulse that prevents
 *     severe clumping or interpenetration.
 *
 * Both systems operate in position-space so they slot directly into the PBD
 * loop in HairStrandDynamics.h after constraint projection.
 *
 * Usage:
 *   HairCollisionSystem col;
 *   col.addCapsule(headCapsule);
 *   col.addCapsule(neckCapsule);
 *
 *   // Inside simulation loop (after HairSystem::step):
 *   col.resolveBodyCollisions(hairSystem, frictionCoeff);
 *   col.resolveHairHairCollisions(hairSystem, repulsionRadius, repulsionStrength);
 *
 * TODO items are marked with  // TODO: <description>
 */

#pragma once

#include "HairStrandDynamics.h"   // Vec3, HairParticle, HairStrand, HairSystem

#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
// Analytic body-collision primitive: a capsule (segment + radius)
// ---------------------------------------------------------------------------
struct Capsule
{
    Vec3  pointA;       ///< One end of the central axis segment
    Vec3  pointB;       ///< Other end of the central axis segment
    float radius;       ///< Outer radius of the capsule [m]

    /**
     * @brief Find the closest point on the capsule axis to point p.
     *        Returns the parameter t ∈ [0,1] and the closest point.
     */
    Vec3 closestPointOnAxis(const Vec3& p, float& outT) const
    {
        Vec3  ab  = pointB - pointA;
        float len2 = ab.lengthSq();
        if (len2 < 1e-12f)
        {
            outT = 0.f;
            return pointA;
        }
        float t = (p - pointA).dot(ab) / len2;
        outT = std::max(0.f, std::min(1.f, t));
        return pointA + ab * outT;
    }
};

// ---------------------------------------------------------------------------
// Simple voxel-grid structure for broad-phase hair-hair collision
// ---------------------------------------------------------------------------
struct SpatialHashGrid
{
    float cellSize = 0.01f; ///< Grid cell edge length [m]; set to ~2× hair radius

    struct CellKey {
        int x, y, z;
        bool operator==(const CellKey& o) const {
            return x==o.x && y==o.y && z==o.z;
        }
    };

    struct CellKeyHash {
        size_t operator()(const CellKey& k) const {
            size_t h = (size_t)(k.x * 73856093)
                     ^ (size_t)(k.y * 19349663)
                     ^ (size_t)(k.z * 83492791);
            return h;
        }
    };

    // Each cell stores (strandIndex, particleIndex) pairs
    struct Entry { int strandIdx; int particleIdx; };
    using Grid = std::unordered_map<CellKey, std::vector<Entry>, CellKeyHash>;

    Grid grid;

    void clear() { grid.clear(); }

    CellKey cellOf(const Vec3& p) const
    {
        return { (int)std::floor(p.x / cellSize),
                 (int)std::floor(p.y / cellSize),
                 (int)std::floor(p.z / cellSize) };
    }

    void insert(const Vec3& p, int strandIdx, int particleIdx)
    {
        grid[cellOf(p)].push_back({ strandIdx, particleIdx });
    }

    /**
     * @brief Iterate over all entries in the 3³ = 27 cells neighbouring pos.
     *        Calls  callback(entry)  for each.
     */
    template<typename Callback>
    void queryNeighbours(const Vec3& pos, Callback&& callback) const
    {
        CellKey c = cellOf(pos);
        for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
        for (int dz = -1; dz <= 1; ++dz)
        {
            CellKey nc{ c.x+dx, c.y+dy, c.z+dz };
            auto it = grid.find(nc);
            if (it != grid.end())
                for (const Entry& e : it->second)
                    callback(e);
        }
    }
};

// ---------------------------------------------------------------------------
// Main collision system
// ---------------------------------------------------------------------------
class HairCollisionSystem
{
public:
    // -----------------------------------------------------------------------
    // Scene setup
    // -----------------------------------------------------------------------

    /** Add a capsule body collider (head, shoulder, arm, …). */
    void addCapsule(const Capsule& cap) { m_capsules.push_back(cap); }
    void clearCapsules()                { m_capsules.clear(); }

    // -----------------------------------------------------------------------
    // Hair-vs-Body collision resolution
    // -----------------------------------------------------------------------

    /**
     * @brief Push hair particles out of all registered capsule colliders.
     * @param hair           The full hair system.
     * @param friction       Friction coefficient [0,1].
     *                       0 = frictionless, 1 = full stick.
     */
    void resolveBodyCollisions(HairSystem& hair, float friction = 0.2f)
    {
        for (HairStrand& strand : hair.strands)
        {
            for (int i = 0; i < strand.particleCount(); ++i)
            {
                // We need mutable access – this requires a non-const accessor
                // TODO: add HairStrand::particleMutable(int) → HairParticle&
                //       For now the design sketch shows the algorithm.

                // const HairParticle& p = strand.particle(i);
                // resolveSingleParticleVsCapsules(p, friction);
                (void)friction; // suppress unused warning until implemented
            }
        }
        // TODO: remove the loop stubs above once the mutable accessor exists
    }

    // -----------------------------------------------------------------------
    // Hair-vs-Hair repulsion (soft inter-strand constraint)
    // -----------------------------------------------------------------------

    /**
     * @brief Apply a soft repulsion between particles of different strands
     *        that are within repulsionRadius of each other.
     *
     * @param hair              The full hair system.
     * @param repulsionRadius   Distance below which repulsion activates [m].
     * @param strength          Fraction of penetration corrected per call
     *                          (similar to PBD stiffness, [0,1]).
     */
    void resolveHairHairCollisions(HairSystem& hair,
                                   float repulsionRadius = 0.005f,
                                   float strength        = 0.1f)
    {
        // Build spatial hash
        m_grid.cellSize = repulsionRadius * 2.f;
        m_grid.clear();

        for (int si = 0; si < (int)hair.strands.size(); ++si)
        {
            const HairStrand& s = hair.strands[si];
            for (int pi = 0; pi < s.particleCount(); ++pi)
                m_grid.insert(s.particle(pi).position, si, pi);
        }

        // Query neighbours and push apart
        for (int si = 0; si < (int)hair.strands.size(); ++si)
        {
            HairStrand& strandA = hair.strands[si];
            for (int pi = 0; pi < strandA.particleCount(); ++pi)
            {
                // TODO: obtain mutable reference once accessor is added
                const HairParticle& pA = strandA.particle(pi);
                if (pA.pinned) continue;

                m_grid.queryNeighbours(pA.position,
                    [&](const SpatialHashGrid::Entry& e)
                    {
                        if (e.strandIdx == si) return; // same strand, skip

                        const HairParticle& pB =
                            hair.strands[e.strandIdx].particle(e.particleIdx);

                        Vec3  delta = pA.position - pB.position;
                        float dist  = delta.length();
                        if (dist < 1e-8f || dist >= repulsionRadius) return;

                        float penetration = repulsionRadius - dist;
                        Vec3  dir         = delta * (1.f / dist);

                        // Split correction by inverse mass
                        float wA   = pA.inverseMass;
                        float wB   = pB.inverseMass;
                        float wSum = wA + wB;
                        if (wSum < 1e-8f) return;

                        Vec3 correction = dir * (penetration * strength / wSum);

                        // TODO: apply correction via mutable references
                        // strandA.particleMutable(pi).position  += correction * wA;
                        // hair.strands[e.strandIdx]
                        //     .particleMutable(e.particleIdx)
                        //     .position -= correction * wB;
                        (void)correction; // placeholder until mutator exists
                    });
            }
        }
    }

// ---------------------------------------------------------------------------
private:
    std::vector<Capsule> m_capsules;
    SpatialHashGrid      m_grid;

    // -----------------------------------------------------------------------
    // Helper: resolve one particle against all capsules
    // -----------------------------------------------------------------------

    /**
     * @brief Detect and respond to penetration of particle p into a capsule.
     *        Modifies p.position (and p.prevPosition for friction).
     *
     * @param p        Mutable hair particle.
     * @param capsule  The capsule to test against.
     * @param friction Friction coefficient.
     * @return true if a collision was resolved.
     */
    bool resolveParticleVsCapsule(HairParticle& p,
                                  const Capsule& capsule,
                                  float          friction)
    {
        float t;
        Vec3  closest = capsule.closestPointOnAxis(p.position, t);
        Vec3  delta   = p.position - closest;
        float dist    = delta.length();

        if (dist >= capsule.radius) return false; // no penetration

        // Surface normal points from capsule axis outward
        Vec3 normal = (dist > 1e-8f) ? delta * (1.f / dist)
                                     : Vec3{0.f, 1.f, 0.f}; // fallback

        // Push position to surface
        float penetration   = capsule.radius - dist;
        p.position         += normal * penetration;

        // Friction: damp the tangential component of velocity
        // v ≈ pos - prevPos  (Verlet velocity)
        Vec3  vel      = p.position - p.prevPosition;
        float vNormal  = vel.dot(normal);
        Vec3  velN     = normal * vNormal;            // normal component
        Vec3  velT     = vel - velN;                  // tangential component

        // Apply Coulomb-like friction: clamp tangential correction
        // TODO: use proper impulse-based friction (Coulomb cone projection)
        float frictionImpulse = std::min(friction * std::abs(vNormal),
                                         velT.length());
        Vec3 frictionDir = velT.normalized();
        p.prevPosition  += frictionDir * frictionImpulse;

        return true;
    }

    // -----------------------------------------------------------------------
    // Resolve a single particle vs every capsule in the scene
    // -----------------------------------------------------------------------
    void resolveSingleParticleVsCapsules(HairParticle& p, float friction)
    {
        for (const Capsule& cap : m_capsules)
            resolveParticleVsCapsule(p, cap, friction);
    }
};

// ---------------------------------------------------------------------------
// Convenience factory: build a standard head+neck capsule rig
// ---------------------------------------------------------------------------
inline std::vector<Capsule> buildHeadRig(const Vec3& headCentre,
                                          float headRadius   = 0.10f,
                                          float neckLength   = 0.08f,
                                          float neckRadius   = 0.05f)
{
    // Head: degenerate capsule (sphere) centred at headCentre
    Capsule head;
    head.pointA = headCentre;
    head.pointB = headCentre;
    head.radius = headRadius;

    // Neck: capsule below head centre
    Capsule neck;
    neck.pointA = headCentre;
    neck.pointB = { headCentre.x,
                    headCentre.y - neckLength,
                    headCentre.z };
    neck.radius = neckRadius;

    // TODO: add shoulder capsules, ear capsules, etc.
    return { head, neck };
}
