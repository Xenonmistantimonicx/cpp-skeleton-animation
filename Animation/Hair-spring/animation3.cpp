/**
 * @file    HairStrandDynamics.h
 * @brief   Position-Based Dynamics (PBD) solver for hair strand simulation.
 *
 * Physics model:
 *   - Each hair is a chain of N particles connected by distance constraints.
 *   - Verlet integration advances particle positions under gravity, drag,
 *     and external wind forces.
 *   - An angular (bending) spring resists deviation from the rest angle
 *     between consecutive segments (Cosserat-style rod approximation).
 *   - A stretch constraint keeps consecutive particle distances near the
 *     rest length (inextensibility).
 *
 * Coordinate system: right-handed, Y-up.
 *
 * Usage:
 *   HairStrand strand;
 *   strand.init(rootPosition, numParticles, segmentLength);
 *   for (every simulation step)
 *   {
 *       strand.applyForces(gravity, wind, dt);
 *       strand.integrate(dt);
 *       strand.solveConstraints(iterations);
 *   }
 *
 * TODO items are marked with  // TODO: <description>
 */

#pragma once

#include <vector>
#include <array>
#include <cmath>

// ---------------------------------------------------------------------------
// Minimal 3-D vector (replace with your engine's math library as needed)
// ---------------------------------------------------------------------------
struct Vec3
{
    float x = 0.f, y = 0.f, z = 0.f;

    Vec3 operator+(const Vec3& b) const { return {x+b.x, y+b.y, z+b.z}; }
    Vec3 operator-(const Vec3& b) const { return {x-b.x, y-b.y, z-b.z}; }
    Vec3 operator*(float s)       const { return {x*s,   y*s,   z*s};   }
    Vec3& operator+=(const Vec3& b){ x+=b.x; y+=b.y; z+=b.z; return *this; }

    float dot  (const Vec3& b) const { return x*b.x + y*b.y + z*b.z; }
    Vec3  cross(const Vec3& b) const {
        return { y*b.z - z*b.y,
                 z*b.x - x*b.z,
                 x*b.y - y*b.x };
    }
    float lengthSq() const { return dot(*this); }
    float length()   const { return std::sqrt(lengthSq()); }
    Vec3  normalized() const {
        float l = length();
        return (l > 1e-8f) ? (*this * (1.f / l)) : Vec3{};
    }
};

// ---------------------------------------------------------------------------
// Single particle in the hair strand
// ---------------------------------------------------------------------------
struct HairParticle
{
    Vec3  position;          ///< Current world-space position
    Vec3  prevPosition;      ///< Previous position (for Verlet integration)
    Vec3  force;             ///< Accumulated external forces (reset each step)
    float mass        = 1.f; ///< Particle mass [kg]
    float inverseMass = 1.f; ///< Pre-computed 1/mass  (0 = kinematic/pinned)
    bool  pinned      = false;///< Root particle is usually pinned
};

// ---------------------------------------------------------------------------
// Distance constraint  |p_i - p_j| == restLength
// ---------------------------------------------------------------------------
struct DistanceConstraint
{
    int   indexA;
    int   indexB;
    float restLength;
    float stiffness = 1.f; ///< [0,1] – fraction corrected per iteration
};

// ---------------------------------------------------------------------------
// Bending constraint (angular spring) between three consecutive particles
// ---------------------------------------------------------------------------
struct BendingConstraint
{
    int   indexA;           ///< i-1
    int   indexB;           ///< i   (hinge)
    int   indexC;           ///< i+1
    float restAngle;        ///< Angle at rest [radians]
    float stiffness = 0.3f; ///< Bending resistance (lower = softer hair)
};

// ---------------------------------------------------------------------------
// One hair strand
// ---------------------------------------------------------------------------
class HairStrand
{
public:
    // -- Construction / initialisation ---------------------------------------

    /**
     * @brief Initialise a straight strand hanging in -Y from rootPos.
     * @param rootPos       World-space root attachment point.
     * @param numParticles  Number of simulated particles (>= 2).
     * @param segLen        Rest length of each segment [m].
     * @param totalMass     Total mass of the strand [kg].
     */
    void init(const Vec3& rootPos,
              int         numParticles,
              float       segLen,
              float       totalMass = 0.01f)
    {
        m_particles.resize(numParticles);
        float particleMass = totalMass / static_cast<float>(numParticles);

        for (int i = 0; i < numParticles; ++i)
        {
            HairParticle& p = m_particles[i];
            p.position     = { rootPos.x, rootPos.y - segLen * i, rootPos.z };
            p.prevPosition = p.position;
            p.force        = {};
            p.mass         = particleMass;
            p.inverseMass  = 1.f / particleMass;
            p.pinned       = (i == 0); // pin root
            if (p.pinned) p.inverseMass = 0.f;
        }

        // Build distance constraints (stretch)
        m_distConstraints.clear();
        for (int i = 0; i + 1 < numParticles; ++i)
            m_distConstraints.push_back({ i, i+1, segLen, 1.f });

        // Build bending constraints
        m_bendConstraints.clear();
        for (int i = 1; i + 1 < numParticles; ++i)
        {
            BendingConstraint bc;
            bc.indexA    = i - 1;
            bc.indexB    = i;
            bc.indexC    = i + 1;
            bc.restAngle = computeAngle(i - 1, i, i + 1);
            bc.stiffness = 0.3f;
            m_bendConstraints.push_back(bc);
        }
    }

    // -- Per-step pipeline ---------------------------------------------------

    /**
     * @brief Accumulate body forces on every free particle.
     * @param gravity   Gravitational acceleration vector (e.g. {0,-9.81,0}).
     * @param wind      Wind force [N] applied uniformly to all particles.
     * @param dragCoeff Linear velocity-drag coefficient.
     */
    void applyForces(const Vec3& gravity,
                     const Vec3& wind,
                     float       dragCoeff = 0.02f)
    {
        for (HairParticle& p : m_particles)
        {
            if (p.pinned) continue;

            // Gravity
            p.force += gravity * p.mass;

            // Wind
            p.force += wind;

            // Velocity-dependent drag:  F_drag = -k * v
            // Approximate velocity from Verlet: v ≈ (pos - prevPos)
            // TODO: expose dragCoeff as a material property
            Vec3 vel = p.position - p.prevPosition;
            p.force += vel * (-dragCoeff / /* dt from last step */ 1.f);
            // NOTE: pass actual dt here once integrated into your engine's loop
        }
    }

    /**
     * @brief Verlet integration step.
     * @param dt  Time-step [s].  Keep below 1/60 s for stability.
     */
    void integrate(float dt)
    {
        float dt2 = dt * dt;

        for (HairParticle& p : m_particles)
        {
            if (p.pinned)
            {
                p.prevPosition = p.position; // keep pinned root up to date
                p.force = {};
                continue;
            }

            Vec3 acceleration = p.force * p.inverseMass;

            // Verlet:  x(t+dt) = 2*x(t) - x(t-dt) + a*dt^2
            Vec3 newPos = p.position * 2.f
                        - p.prevPosition
                        + acceleration * dt2;

            p.prevPosition = p.position;
            p.position     = newPos;
            p.force        = {}; // clear for next step
        }
    }

    /**
     * @brief Iterative constraint projection (PBD solve).
     * @param iterations  Number of Gauss-Seidel iterations (4–10 typical).
     */
    void solveConstraints(int iterations = 6)
    {
        for (int iter = 0; iter < iterations; ++iter)
        {
            solveDistanceConstraints();
            solveBendingConstraints();
        }
    }

    // -- Accessors -----------------------------------------------------------

    int               particleCount() const { return (int)m_particles.size(); }
    const HairParticle& particle(int i) const { return m_particles[i]; }

    /** Move the pinned root to follow the scalp/skeleton. */
    void setRootPosition(const Vec3& pos)
    {
        if (!m_particles.empty())
        {
            m_particles[0].position     = pos;
            m_particles[0].prevPosition = pos;
        }
    }

// ---------------------------------------------------------------------------
private:
    std::vector<HairParticle>       m_particles;
    std::vector<DistanceConstraint> m_distConstraints;
    std::vector<BendingConstraint>  m_bendConstraints;

    // -----------------------------------------------------------------------
    // Stretch (distance) constraint solver
    // -----------------------------------------------------------------------
    void solveDistanceConstraints()
    {
        for (const DistanceConstraint& c : m_distConstraints)
        {
            HairParticle& pa = m_particles[c.indexA];
            HairParticle& pb = m_particles[c.indexB];

            Vec3  delta  = pb.position - pa.position;
            float dist   = delta.length();
            if (dist < 1e-8f) continue;

            float error  = (dist - c.restLength) / dist;
            float wSum   = pa.inverseMass + pb.inverseMass;
            if (wSum < 1e-8f) continue;

            Vec3 correction = delta * (error * c.stiffness / wSum);

            if (!pa.pinned) pa.position += correction *  pa.inverseMass;
            if (!pb.pinned) pb.position += correction * -pb.inverseMass;
        }
    }

    // -----------------------------------------------------------------------
    // Bending (angular) constraint solver  –  simple cosine-based PBD
    // -----------------------------------------------------------------------
    void solveBendingConstraints()
    {
        for (const BendingConstraint& c : m_bendConstraints)
        {
            HairParticle& pA = m_particles[c.indexA];
            HairParticle& pB = m_particles[c.indexB]; // hinge
            HairParticle& pC = m_particles[c.indexC];

            Vec3  e0   = pA.position - pB.position;
            Vec3  e1   = pC.position - pB.position;
            float l0   = e0.length();
            float l1   = e1.length();
            if (l0 < 1e-8f || l1 < 1e-8f) continue;

            float cosA = e0.dot(e1) / (l0 * l1);
            cosA = std::max(-1.f, std::min(1.f, cosA)); // clamp for acos
            float angle = std::acos(cosA);

            float error = angle - c.restAngle;
            if (std::abs(error) < 1e-5f) continue;

            // TODO: implement proper gradient-based angular correction
            //       (Müller et al. 2007 bending model or discrete elastic rod)
            //       The placeholder below applies a proportional nudge.
            Vec3 axis = e0.cross(e1);
            float axLen = axis.length();
            if (axLen < 1e-8f) continue;
            axis = axis * (1.f / axLen);

            float correction = error * c.stiffness * 0.5f;
            Vec3  corrA = axis.cross(e0) * ( correction / l0);
            Vec3  corrC = axis.cross(e1) * (-correction / l1);

            float wSum = pA.inverseMass + pB.inverseMass + pC.inverseMass;
            if (wSum < 1e-8f) continue;

            if (!pA.pinned) pA.position += corrA * (pA.inverseMass / wSum);
            if (!pC.pinned) pC.position += corrC * (pC.inverseMass / wSum);
        }
    }

    // -----------------------------------------------------------------------
    // Helper: angle at particle[mid] formed by [prev]-[mid]-[next]
    // -----------------------------------------------------------------------
    float computeAngle(int prev, int mid, int next) const
    {
        Vec3  e0   = m_particles[prev].position - m_particles[mid].position;
        Vec3  e1   = m_particles[next].position - m_particles[mid].position;
        float l0   = e0.length(), l1 = e1.length();
        if (l0 < 1e-8f || l1 < 1e-8f) return 0.f;
        float cosA = e0.dot(e1) / (l0 * l1);
        cosA = std::max(-1.f, std::min(1.f, cosA));
        return std::acos(cosA);
    }
};

// ---------------------------------------------------------------------------
// Optional: collection of strands representing a full head of hair
// ---------------------------------------------------------------------------
class HairSystem
{
public:
    std::vector<HairStrand> strands;

    /**
     * @brief Step the entire hair system forward in time.
     * @param gravity     Gravity vector.
     * @param wind        Wind force.
     * @param dt          Time-step [s].
     * @param iterations  Constraint solver iterations per step.
     */
    void step(const Vec3& gravity,
              const Vec3& wind,
              float       dt,
              int         iterations = 6)
    {
        for (HairStrand& s : strands)
        {
            s.applyForces(gravity, wind);
            s.integrate(dt);
            s.solveConstraints(iterations);
        }
        // TODO: call HairCollisionSystem::resolveCollisions() here
    }
};
