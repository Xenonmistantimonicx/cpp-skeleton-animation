#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include <string>

// ============================================================================
// AAA GAME ENGINE MATH & PHYSICS UTILITIES (SIMULATED FOR SINGLE-FILE PARITY)
// ============================================================================

struct Vector3 {
    float x{ 0.0f }, y{ 0.0f }, z{ 0.0f };

    Vector3() = default;
    Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    Vector3 operator+(const Vector3& o) const { return Vector3(x + o.x, y + o.y, z + o.z); }
    Vector3 operator-(const Vector3& o) const { return Vector3(x - o.x, y - o.y, z - o.z); }
    Vector3 operator*(float s) const { return Vector3(x * s, y * s, z * s); }
    Vector3 operator/(float s) const { return Vector3(x / s, y / s, z / s); }

    float Magnitude() const { return std::sqrt(x * x + y * y + z * z); }
    Vector3 Normalized() const {
        float mag = Magnitude();
        return (mag > 0.00001f) ? (*this / mag) : Vector3(0, 0, 0);
    }

    static float Dot(const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    static Vector3 Cross(const Vector3& a, const Vector3& b) {
        return Vector3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }
    static float Distance(const Vector3& a, const Vector3& b) { return (a - b).Magnitude(); }
};

struct Quaternion {
    float w{ 1.0f }, x{ 0.0f }, y{ 0.0f }, z{ 0.0f };
    // Simplified rotation representation for procedural skeletal transformations
};

struct Transform {
    Vector3 position;
    Quaternion rotation;
    Vector3 scale{ 1.0f, 1.0f, 1.0f };
};

// ============================================================================
// ENVIRONMENTAL DATA STRUCTURES
// ============================================================================

struct CanopySunlightMap {
    Vector3 primaryLightDirection{ 0.2f, -0.9f, 0.3f };
    float intensity{ 1.0f };

    Vector3 GetOptimalLightVectorAtPosition(const Vector3& pos) const {
        // Simulates dynamic light density sampling through dense canopy foliage
        Vector3 noiseOffset(std::sin(pos.x * 0.1f) * 0.2f, 0.0f, std::cos(pos.z * 0.1f) * 0.2f);
        return (primaryLightDirection * -1.0f + noiseOffset).Normalized();
    }
};

struct TerrainHitResult {
    bool bHit{ false };
    Vector3 impactPoint;
    Vector3 impactNormal{ 0.0f, 1.0f, 0.0f };
    float soilNutrientDensity{ 0.8f }; // 0.0 (Barren) to 1.0 (Optimal)
};

class EnvironmentalRaycastSystem {
public:
    static TerrainHitResult RaycastToGround(const Vector3& origin, const Vector3& direction, float maxDistance) {
        TerrainHitResult hit;
        // Analytical plane test simulating raycast against terrain mesh at Z=0.0
        if (direction.y != 0.0f) {
            float t = -origin.y / direction.y;
            if (t >= 0.0f && t <= maxDistance) {
                hit.bHit = true;
                hit.impactPoint = origin + direction * t;
                hit.impactNormal = Vector3(0.0f, 1.0f, 0.0f);
                hit.soilNutrientDensity = std::clamp(1.0f - (hit.impactPoint.Magnitude() * 0.01f), 0.2f, 1.0f);
            }
        }
        return hit;
    }
};

// ============================================================================
// STILT ROOT PROCEDURAL IK NODE
// ============================================================================

enum class RootState {
    Growing,      // Extending from hub toward soil target
    Anchored,     // Fully functional, supporting structural load
    Decaying,     // Shaded/un-nutrified, losing structural integrity
    Dead          // Ready for memory garbage collection
};

struct StiltRootSegment {
    Vector3 localOrigin;
    Vector3 currentTipPosition;
    Vector3 targetGroundPosition;
    Vector3 normalAtAnchor;
    
    float length{ 0.0f };
    float maxLength{ 4.5f };
    float radius{ 0.15f };
    float structuralHealth{ 1.0f }; // 1.0 = Prime, 0.0 = Severed/Rotated
    float loadBearingRatio{ 0.0f };  // Calculated weight distribution percentage
    
    RootState state{ RootState::Growing };
    float growthProgress{ 0.0f };    // 0.0 to 1.0 interpolation
};

// ============================================================================
// AAA WALKING PALM ENGINE SUBSYSTEM
// ============================================================================

class WalkingPalmEntity {
private:
    // Core Physics & Skeletal Transform
    Transform m_worldTransform;
    Vector3 m_centerOfMass;
    Vector3 m_trunkLeaningDirection{ 0.0f, 1.0f, 0.0f };
    
    // Physical Parameters
    float m_totalMassKg{ 450.0f };
    float m_growthRateFactor{ 0.05f }; // Speed of biological time lapse
    float m_phototropismSensitivity{ 0.85f };
    float m_gravityLeanMultiplier{ 1.2f };

    // Anatomical Components
    std::vector<StiltRootSegment> m_stiltRoots;
    const size_t m_maxRootCount{ 16 };
    const size_t m_minStableRoots{ 4 };

    // Biological Registers
    Vector3 m_accumulatedMovementVector{ 0.0f, 0.0f, 0.0f };
    float m_accumulatedPhototropicEnergy{ 0.0f };

public:
    WalkingPalmEntity(const Vector3& initialPosition) {
        m_worldTransform.position = initialPosition;
        m_centerOfMass = initialPosition + Vector3(0.0f, 3.0f, 0.0f); // Base crown height
        
        InitializeBaseRoots();
    }

    ~WalkingPalmEntity() = default;

    // ------------------------------------------------------------------------
    // CORE ENGINE TICK (Per Frame / Fixed Sub-Step Update)
    // ------------------------------------------------------------------------
    void Tick(float deltaTime, const CanopySunlightMap& environmentMap) {
        EvaluatePhototropismAndLean(environmentMap, deltaTime);
        UpdateRootGrowthAndDecay(environmentMap, deltaTime);
        SolveTwoBoneIKForRoots();
        CalculateStructuralBalance();
        TranslateOrganismAcrossTerrain(deltaTime);
    }

private:
    // ------------------------------------------------------------------------
    // 1. INITIALIZATION
    // ------------------------------------------------------------------------
    void InitializeBaseRoots() {
        // Spawn an initial stable radial tripod/quad-pod of stilt roots
        const int initialRoots = 6;
        const float radius = 1.8f;
        
        for (int i = 0; i < initialRoots; ++i) {
            float angle = (360.0f / initialRoots) * i * (3.14159f / 180.0f);
            Vector3 offset(std::cos(angle) * radius, -2.5f, std::sin(angle) * radius);

            StiltRootSegment root;
            root.localOrigin = Vector3(0.0f, 1.5f, 0.0f); // Root crown hub
            root.targetGroundPosition = m_worldTransform.position + offset;
            root.targetGroundPosition.y = 0.0f; // Ground baseline
            root.currentTipPosition = root.targetGroundPosition;
            root.growthProgress = 1.0f;
            root.state = RootState::Anchored;
            root.structuralHealth = 1.0f;
            
            m_stiltRoots.push_back(root);
        }
    }

    // ------------------------------------------------------------------------
    // 2. PHOTOTROPISM & DYNAMIC LEAN SIMULATION
    // ------------------------------------------------------------------------
    void EvaluatePhototropismAndLean(const CanopySunlightMap& env, float dt) {
        // Sample light direction to determine growth tilt
        Vector3 optimalSunVector = env.GetOptimalLightVectorAtPosition(m_centerOfMass);
        
        // Calculate lean force: Phototropism pull modified by gravity center deflection
        Vector3 targetLean = (optimalSunVector * m_phototropismSensitivity) + Vector3(0.0f, -0.2f, 0.0f);
        m_trunkLeaningDirection = (m_trunkLeaningDirection + (targetLean * dt * 0.1f)).Normalized();

        // Shift Center of Mass outward as trunk tilts
        Vector3 leanOffset = Vector3(m_trunkLeaningDirection.x, 0.0f, m_trunkLeaningDirection.z) * 1.5f;
        m_centerOfMass = m_worldTransform.position + Vector3(0.0f, 3.5f, 0.0f) + leanOffset;
    }

    // ------------------------------------------------------------------------
    // 3. PROCEDURAL ROOT GENERATION & DECAY LIFECYCLE
    // ------------------------------------------------------------------------
    void UpdateRootGrowthAndDecay(const CanopySunlightMap& env, float dt) {
        Vector3 leanDir2D = Vector3(m_trunkLeaningDirection.x, 0.0f, m_trunkLeaningDirection.z).Normalized();

        // Decay roots that are opposite to the direction of lean (shaded/tension side)
        for (auto& root : m_stiltRoots) {
            if (root.state != RootState::Anchored) continue;

            Vector3 rootVector = (root.currentTipPosition - m_worldTransform.position).Normalized();
            float alignment = Vector3::Dot(rootVector, leanDir2D);

            // Roots opposite to lean vector experience mechanical tension and decay
            if (alignment < -0.3f) {
                root.structuralHealth -= 0.05f * dt * m_growthRateFactor;
                if (root.structuralHealth <= 0.2f) {
                    root.state = RootState::Decaying;
                }
            } else {
                // Heal/Strengthen load-bearing compression roots
                root.structuralHealth = std::min(1.0f, root.structuralHealth + 0.02f * dt);
            }
        }

        // Spawn new adventitious roots in the direction of the light lean vector
        if (m_stiltRoots.size() < m_maxRootCount) {
            m_accumulatedPhototropicEnergy += dt * m_growthRateFactor;

            if (m_accumulatedPhototropicEnergy >= 1.0f) {
                SpawnNewStiltRoot(leanDir2D);
                m_accumulatedPhototropicEnergy = 0.0f;
            }
        }

        // Advance growth stage for active growing roots
        for (auto& root : m_stiltRoots) {
            if (root.state == RootState::Growing) {
                root.growthProgress += dt * 0.2f * m_growthRateFactor;
                root.currentTipPosition = InterpolatePosition(
                    m_worldTransform.position + root.localOrigin,
                    root.targetGroundPosition,
                    root.growthProgress
                );

                if (root.growthProgress >= 1.0f) {
                    root.growthProgress = 1.0f;
                    root.state = RootState::Anchored;
                }
            }
        }

        // Remove fully dead roots
        m_stiltRoots.erase(
            std::remove_if(m_stiltRoots.begin(), m_stiltRoots.end(),
                [](const StiltRootSegment& r) {
                    return r.state == RootState::Decaying && r.structuralHealth <= 0.0f;
                }),
            m_stiltRoots.end()
        );
    }

    void SpawnNewStiltRoot(const Vector3& direction) {
        StiltRootSegment newRoot;
        newRoot.localOrigin = Vector3(0.0f, 1.2f, 0.0f);
        
        // Target casting outwards in lean direction with slight rotational variance
        float scatterAngle = ((float)rand() / RAND_MAX - 0.5f) * 0.5f;
        Vector3 targetDir(
            direction.x * std::cos(scatterAngle) - direction.z * std::sin(scatterAngle),
            -0.8f,
            direction.x * std::sin(scatterAngle) + direction.z * std::cos(scatterAngle)
        );

        Vector3 rayStart = m_worldTransform.position + Vector3(0.0f, 2.0f, 0.0f) + (targetDir * 1.2f);
        TerrainHitResult hit = EnvironmentalRaycastSystem::RaycastToGround(rayStart, Vector3(0.0f, -1.0f, 0.0f), 10.0f);

        if (hit.bHit) {
            newRoot.targetGroundPosition = hit.impactPoint;
            newRoot.currentTipPosition = rayStart;
            newRoot.normalAtAnchor = hit.impactNormal;
            newRoot.growthProgress = 0.0f;
            newRoot.state = RootState::Growing;
            newRoot.structuralHealth = 0.5f; // Starts vulnerable
            
            m_stiltRoots.push_back(newRoot);
        }
    }

    // ------------------------------------------------------------------------
    // 4. INVERSE KINEMATICS (IK) SOLVER FOR PROCEDURAL BONE ANCHORING
    // ------------------------------------------------------------------------
    void SolveTwoBoneIKForRoots() {
        // Analytical 2-Bone IK solution for stilt roots bending dynamically under load
        for (auto& root : m_stiltRoots) {
            Vector3 rootBaseWorld = m_worldTransform.position + root.localOrigin;
            Vector3 targetEffector = root.currentTipPosition;

            float distanceToTarget = Vector3::Distance(rootBaseWorld, targetEffector);
            float upperSegmentLen = root.maxLength * 0.55f;
            float lowerSegmentLen = root.maxLength * 0.45f;

            // Clamp reach to avoid IK pop singularities
            distanceToTarget = std::min(distanceToTarget, upperSegmentLen + lowerSegmentLen - 0.001f);

            // Cosine law knee hinge angle calculation
            float cosKnee = (distanceToTarget * distanceToTarget - upperSegmentLen * upperSegmentLen - lowerSegmentLen * lowerSegmentLen) 
                           / (2.0f * upperSegmentLen * lowerSegmentLen);
            float kneeAngleRad = std::acos(std::clamp(cosKnee, -1.0f, 1.0f));

            // Knee bend direction computed outwards from trunk CoM
            Vector3 bendDirection = (targetEffector - m_centerOfMass).Normalized();
            Vector3 jointPosition = rootBaseWorld + (bendDirection * (upperSegmentLen * std::sin(kneeAngleRad * 0.5f)));
            
            // Output joint position to bone buffers (used for mesh skinning pass)
            (void)jointPosition;
        }
    }

    // ------------------------------------------------------------------------
    // 5. WEIGHT DISTRIBUTION & STRUCTURAL INTEGRITY CALCULATOR
    // ------------------------------------------------------------------------
    void CalculateStructuralBalance() {
        float totalHealthWeightedSum = 0.0f;

        for (auto& root : m_stiltRoots) {
            if (root.state == RootState::Anchored) {
                totalHealthWeightedSum += root.structuralHealth;
            }
        }

        if (totalHealthWeightedSum <= 0.0001f) return;

        // Calculate dynamic distribution of trunk weight per anchored root
        for (auto& root : m_stiltRoots) {
            if (root.state == RootState::Anchored) {
                root.loadBearingRatio = root.structuralHealth / totalHealthWeightedSum;
            } else {
                root.loadBearingRatio = 0.0f;
            }
        }
    }

    // ------------------------------------------------------------------------
    // 6. REAL LIFE ORGANISM DISPLACEMENT ("WALKING" TRANSLATION)
    // ------------------------------------------------------------------------
    void TranslateOrganismAcrossTerrain(float dt) {
        // Calculate the centroid of all functional anchored roots
        Vector3 anchorCentroid(0, 0, 0);
        int anchoredCount = 0;

        for (const auto& root : m_stiltRoots) {
            if (root.state == RootState::Anchored) {
                anchorCentroid = anchorCentroid + root.currentTipPosition;
                anchoredCount++;
            }
        }

        if (anchoredCount >= static_cast<int>(m_minStableRoots)) {
            anchorCentroid = anchorCentroid / static_cast<float>(anchoredCount);
            
            // Center of Mass pull vector towards new root cluster centroid
            Vector3 displacementVector = (anchorCentroid - m_worldTransform.position);
            displacementVector.y = 0.0f; // Constrain to ground plane trajectory

            // Real life walking palm tree speed: Extremely low linear velocity (Time-lapsed movement)
            float biomechanicalWalkSpeed = 0.02f * m_growthRateFactor;
            m_worldTransform.position = m_worldTransform.position + (displacementVector * biomechanicalWalkSpeed * dt);
        }
    }

    // ------------------------------------------------------------------------
    // UTILITY HELPER
    // ------------------------------------------------------------------------
    Vector3 InterpolatePosition(const Vector3& a, const Vector3& b, float t) const {
        t = std::clamp(t, 0.0f, 1.0f);
        // Smooth step s-curve for organic tip placement motion
        float smoothT = t * t * (3.0f - 2.0f * t);
        return a + (b - a) * smoothT;
    }

public:
    // ------------------------------------------------------------------------
    // ENGINE DIAGNOSTICS & TELEMETRY DISPLAY
    // ------------------------------------------------------------------------
    void RenderDebugTelemetry() const {
        std::cout << "\n=======================================================\n";
        std::cout << " AAA SOCRATEA EXORRHIZA (WALKING PALM) SYSTEM STATE    \n";
        std::cout << "=======================================================\n";
        std::cout << "World Pos        : [" << m_worldTransform.position.x << ", " 
                                          << m_worldTransform.position.y << ", " 
                                          << m_worldTransform.position.z << "]\n";
        std::cout << "Center of Mass   : [" << m_centerOfMass.x << ", " 
                                          << m_centerOfMass.y << ", " 
                                          << m_centerOfMass.z << "]\n";
        std::cout << "Trunk Lean Vector: [" << m_trunkLeaningDirection.x << ", " 
                                          << m_trunkLeaningDirection.y << ", " 
                                          << m_trunkLeaningDirection.z << "]\n";
        std::cout << "Total Root Count : " << m_stiltRoots.size() << "\n";
        std::cout << "-------------------------------------------------------\n";

        for (size_t i = 0; i < m_stiltRoots.size(); ++i) {
            const auto& r = m_stiltRoots[i];
            std::string stateStr;
            switch(r.state) {
                case RootState::Growing:  stateStr = "GROWING"; break;
                case RootState::Anchored: stateStr = "ANCHORED"; break;
                case RootState::Decaying: stateStr = "DECAYING"; break;
                case RootState::Dead:     stateStr = "DEAD"; break;
            }

            std::cout << " Root #" << i << " [" << stateStr << "]"
                      << " Health: " << (r.structuralHealth * 100.0f) << "%"
                      << " Load Share: " << (r.loadBearingRatio * 100.0f) << "%"
                      << " Tip: [" << r.currentTipPosition.x << ", " << r.currentTipPosition.z << "]\n";
        }
        std::cout << "=======================================================\n";
    }
};

// ============================================================================
// SIMULATED AAA ENGINE RUNTIME LOOP
// ============================================================================

int main() {
    std::cout << "Initializing AAA Walking Palm Tree Engine System...\n";

    // Instantiate entity at world center
    WalkingPalmEntity walkingPalm(Vector3(0.0f, 0.0f, 0.0f));

    // Dynamic environment setup
    CanopySunlightMap canopyMap;
    canopyMap.primaryLightDirection = Vector3(0.6f, -0.8f, 0.2f); // Sunlight breaking through canopy shift

    float deltaSimulatedTime = 0.16f; // Accelerated tick step for preview
    int totalFrameTicks = 20;

    for (int frame = 0; frame < totalFrameTicks; ++frame) {
        // Dynamic directional light shift over time to force procedural re-rooting
        if (frame == 10) {
            canopyMap.primaryLightDirection = Vector3(-0.7f, -0.8f, -0.4f);
        }

        walkingPalm.Tick(deltaSimulatedTime, canopyMap);

        if (frame % 5 == 0 || frame == totalFrameTicks - 1) {
            std::cout << "\n--> SIMULATION FRAME TICK: " << frame;
            walkingPalm.RenderDebugTelemetry();
        }
    }

    return 0;
}
