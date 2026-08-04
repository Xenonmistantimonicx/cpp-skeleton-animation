#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include <string>

// ============================================================================
// AAA GAME ENGINE MATH & PHYSICS UTILITIES
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
    
    static Quaternion Identity() { return Quaternion{ 1.0f, 0.0f, 0.0f, 0.0f }; }
};

struct Transform {
    Vector3 position;
    Quaternion rotation;
    Vector3 scale{ 1.0f, 1.0f, 1.0f };
};

// ============================================================================
// ENVIRONMENT & PHENOLOGY STRUCTURES
// ============================================================================

enum class PhenologySeason {
    EarlySpring,  // Bright lime-green unfurling leaf buds
    Summer,       // Deep glossy green curtain canopy
    Autumn,       // Fiery copper/bronze foliage transition
    Winter        // Deciduous bare weeping silhouette with branch drooping
};

struct WorldWindField {
    Vector3 windDirection{ 1.0f, 0.0f, 0.3f };
    float baseSpeed{ 4.5f };        // Meters per second
    float gustIntensity{ 0.4f };    // Gust modifier
    float turbulenceFrequency{ 1.2f };

    Vector3 SampleWindVelocityAtPosition(const Vector3& pos, float time) const {
        float gustNoise = std::sin(pos.x * 0.1f + time * turbulenceFrequency) 
                        * std::cos(pos.z * 0.1f + time * turbulenceFrequency * 0.7f);
        float currentSpeed = baseSpeed + (gustNoise * gustIntensity * baseSpeed);
        return windDirection.Normalized() * currentSpeed;
    }
};

struct GroundTerrainHit {
    bool bHit{ false };
    Vector3 impactPoint;
    Vector3 normal{ 0.0f, 1.0f, 0.0f };
};

class TerrainCollisionSystem {
public:
    static GroundTerrainHit RaycastToGround(const Vector3& origin, float maxDistance = 20.0f) {
        GroundTerrainHit hit;
        // Analytical terrain test against ground plane at Z=0.0
        if (origin.y <= 0.0f) {
            hit.bHit = true;
            hit.impactPoint = Vector3(origin.x, 0.0f, origin.z);
            hit.normal = Vector3(0.0f, 1.0f, 0.0f);
        }
        return hit;
    }
};

// ============================================================================
// PENDENT BRANCH BONE NODE (WEEPING IK SYSTEM)
// ============================================================================

struct WeepingBranchJoint {
    Vector3 localPosition;
    Vector3 worldPosition;
    Vector3 previousWorldPosition; // Verlet integration velocity buffer
    Vector3 restingDirection;      // Natural anatomical direction
    
    float segmentLength{ 0.8f };
    float thicknessRadius{ 0.08f };
    float flexuralRigidity{ 0.8f }; // High = Stiff main limb, Low = Pendulous tip
    float damping{ 0.92f };
    
    bool bIsDrapingOnGround{ false };
};

class WeepingBranchChain {
public:
    std::vector<WeepingBranchJoint> m_joints;
    Vector3 m_rootAttachmentWorldPos;
    bool m_bIsMajorLimb{ false };

public:
    WeepingBranchChain() = default;

    void BuildProceduralBranchChain(const Vector3& startPos, const Vector3& initialDir, int numSegments, float baseRadius, bool isLimb) {
        m_rootAttachmentWorldPos = startPos;
        m_bIsMajorLimb = isLimb;

        Vector3 currentPos = startPos;
        Vector3 dir = initialDir.Normalized();

        for (int i = 0; i < numSegments; ++i) {
            WeepingBranchJoint joint;
            
            // Progressive weeping decay: outer segments droop downward (-Y) under gravity
            float t = static_cast<float>(i) / static_cast<float>(numSegments);
            if (!m_bIsMajorLimb) {
                // Secondary/tertiary foliage tip branch bends heavily down toward ground
                dir = (dir * (1.0f - t) + Vector3(0.0f, -1.0f, 0.0f) * t * 1.5f).Normalized();
            }

            joint.segmentLength = 0.6f * (1.0f - t * 0.3f);
            joint.thicknessRadius = baseRadius * (1.0f - t * 0.85f);
            
            // Rigidity falls off drastically at weeping branch ends
            joint.flexuralRigidity = m_bIsMajorLimb ? (0.95f - t * 0.3f) : (0.4f - t * 0.38f);
            joint.restingDirection = dir;
            
            currentPos = currentPos + (dir * joint.segmentLength);
            joint.localPosition = currentPos - startPos;
            joint.worldPosition = currentPos;
            joint.previousWorldPosition = currentPos;

            m_joints.push_back(joint);
        }
    }

    void UpdatePhysicsIK(const Vector3& rootWorldPos, const WorldWindField& wind, float dt, float engineTime) {
        m_rootAttachmentWorldPos = rootWorldPos;
        if (m_joints.empty()) return;

        // 1. Verlet Integration & Forces Pass
        for (size_t i = 0; i < m_joints.size(); ++i) {
            auto& joint = m_joints[i];

            Vector3 velocity = (joint.worldPosition - joint.previousWorldPosition) * joint.damping;
            joint.previousWorldPosition = joint.worldPosition;

            // Environmental Gravity Force
            Vector3 gravityForce(0.0f, -9.81f * (1.0f - joint.flexuralRigidity), 0.0f);

            // Aerodynamic Drag Force (Foliage wind resistance)
            Vector3 windVel = wind.SampleWindVelocityAtPosition(joint.worldPosition, engineTime);
            float dragCoefficient = m_bIsMajorLimb ? 0.05f : 0.25f; // Leaves increase drag area
            Vector3 windForce = windVel * dragCoefficient;

            // Elastic Stiffness Force returning joint toward resting anatomical position
            Vector3 parentPos = (i == 0) ? m_rootAttachmentWorldPos : m_joints[i - 1].worldPosition;
            Vector3 targetAnatomicalPos = parentPos + (joint.restingDirection * joint.segmentLength);
            Vector3 springForce = (targetAnatomicalPos - joint.worldPosition) * (joint.flexuralRigidity * 40.0f);

            Vector3 totalAcceleration = gravityForce + windForce + springForce;
            joint.worldPosition = joint.worldPosition + velocity + (totalAcceleration * dt * dt);
        }

        // 2. Backward/Forward Constraint Satisfaction Pass (FABRIK / Distance Constraints)
        // Root attachment point constraint
        m_joints[0].worldPosition = m_rootAttachmentWorldPos + (m_joints[0].worldPosition - m_rootAttachmentWorldPos).Normalized() * m_joints[0].segmentLength;

        for (size_t i = 1; i < m_joints.size(); ++i) {
            Vector3 parentPos = m_joints[i - 1].worldPosition;
            Vector3 delta = m_joints[i].worldPosition - parentPos;
            float currentDist = delta.Magnitude();
            float targetDist = m_joints[i].segmentLength;

            if (currentDist > 0.0001f) {
                m_joints[i].worldPosition = parentPos + (delta / currentDist) * targetDist;
            }
        }

        // 3. Ground Draping Collision Pass (Cascading foliage trailing on terrain)
        for (auto& joint : m_joints) {
            GroundTerrainHit hit = TerrainCollisionSystem::RaycastToGround(joint.worldPosition);
            if (hit.bHit) {
                joint.worldPosition.y = hit.impactPoint.y + joint.thicknessRadius;
                joint.bIsDrapingOnGround = true;
            } else {
                joint.bIsDrapingOnGround = false;
            }
        }
    }
};

// ============================================================================
// AAA WEEPING BEECH SYSTEM SUBSYSTEM
// ============================================================================

class WeepingBeechEntity {
private:
    Transform m_worldTransform;
    PhenologySeason m_currentSeason{ PhenologySeason::Summer };

    // Anatomical Attributes
    float m_treeHeight{ 12.0f };             // Mature weeping beech height
    float m_canopySpreadRadius{ 9.5f };      // Wide spreading weeping curtain
    float m_trunkBarkRoughness{ 0.85f };     // Smooth gray elephant-skin beech bark
    
    // Procedural Pendent Skeleton
    std::vector<WeepingBranchChain> m_majorLimbChains;
    std::vector<WeepingBranchChain> m_weepingCurtainChains;

    // Biological Telemetry Registers
    float m_leafDensity{ 1.0f };             // 0.0 (Bare Winter) to 1.0 (Full Summer)
    Vector3 m_foliageColorRGB{ 0.08f, 0.22f, 0.04f };

public:
    WeepingBeechEntity(const Vector3& spawnPosition) {
        m_worldTransform.position = spawnPosition;
        GenerateProceduralWeepingSkeletalStructure();
        UpdatePhenologyState(PhenologySeason::Summer);
    }

    ~WeepingBeechEntity() = default;

    // ------------------------------------------------------------------------
    // CORE ENGINE TICK
    // ------------------------------------------------------------------------
    void Tick(float deltaTime, float engineTime, const WorldWindField& environmentWind) {
        UpdatePhysicsBranchSimulation(deltaTime, engineTime, environmentWind);
    }

    void UpdatePhenologyState(PhenologySeason newSeason) {
        m_currentSeason = newSeason;

        switch (m_currentSeason) {
            case PhenologySeason::EarlySpring:
                m_leafDensity = 0.4f;
                m_foliageColorRGB = Vector3(0.42f, 0.75f, 0.12f); // Bright tender lime
                break;
            case PhenologySeason::Summer:
                m_leafDensity = 1.0f;
                m_foliageColorRGB = Vector3(0.06f, 0.20f, 0.05f); // Deep glossy forest green
                break;
            case PhenologySeason::Autumn:
                m_leafDensity = 0.85f;
                m_foliageColorRGB = Vector3(0.72f, 0.31f, 0.08f); // Fiery copper/bronze
                break;
            case PhenologySeason::Winter:
                m_leafDensity = 0.0f;                             // Bare weeping skeleton
                m_foliageColorRGB = Vector3(0.1f, 0.1f, 0.1f);
                break;
        }
    }

private:
    // ------------------------------------------------------------------------
    // PROCEDURAL SKELETAL GENERATION
    // ------------------------------------------------------------------------
    void GenerateProceduralWeepingSkeletalStructure() {
        const int primaryLimbs = 5;
        const float archRadius = 4.0f;

        // 1. Build Arching Primary Limbs extending from main trunk crotch
        for (int i = 0; i < primaryLimbs; ++i) {
            float angle = (360.0f / primaryLimbs) * i * (3.14159f / 180.0f);
            
            // Major limbs arch UPWARDS and OUTWARDS first before weeping
            Vector3 limbDir(std::cos(angle) * 0.8f, 0.6f, std::sin(angle) * 0.8f);
            Vector3 limbOrigin = m_worldTransform.position + Vector3(0.0f, m_treeHeight * 0.55f, 0.0f);

            WeepingBranchChain limbChain;
            limbChain.BuildProceduralBranchChain(limbOrigin, limbDir, 8, 0.35f, true);
            m_majorLimbChains.push_back(limbChain);

            // 2. Attach Weeping Foliage Curtains to each major limb segment
            for (size_t j = 2; j < limbChain.m_joints.size(); ++j) {
                Vector3 attachPoint = limbChain.m_joints[j].worldPosition;
                
                // Weeping sub-branches cascade almost vertically downwards (-Y)
                Vector3 weepingDir(std::cos(angle + 0.5f) * 0.2f, -0.9f, std::sin(angle + 0.5f) * 0.2f);
                
                WeepingBranchChain curtainChain;
                curtainChain.BuildProceduralBranchChain(attachPoint, weepingDir, 10, 0.12f, false);
                m_weepingCurtainChains.push_back(curtainChain);
            }
        }
    }

    // ------------------------------------------------------------------------
    // PHYSICS SIMULATION TICK
    // ------------------------------------------------------------------------
    void UpdatePhysicsBranchSimulation(float dt, float engineTime, const WorldWindField& wind) {
        // Step 1: Update Structural Major Limbs
        for (auto& limb : m_majorLimbChains) {
            limb.UpdatePhysicsIK(limb.m_rootAttachmentWorldPos, wind, dt, engineTime);
        }

        // Step 2: Drive Weeping Curtains attached to moving Major Limb nodes
        size_t curtainIndex = 0;
        for (auto& limb : m_majorLimbChains) {
            for (size_t j = 2; j < limb.m_joints.size(); ++j) {
                if (curtainIndex < m_weepingCurtainChains.size()) {
                    Vector3 dynamicParentPos = limb.m_joints[j].worldPosition;
                    m_weepingCurtainChains[curtainIndex].UpdatePhysicsIK(dynamicParentPos, wind, dt, engineTime);
                    curtainIndex++;
                }
            }
        }
    }

public:
    // ------------------------------------------------------------------------
    // ENGINE TELEMETRY & DIAGNOSTICS DISPLAY
    // ------------------------------------------------------------------------
    void RenderDebugTelemetry() const {
        std::cout << "\n=======================================================\n";
        std::cout << " AAA FAGUS SYLVATICA 'PENDULA' (WEEPING BEECH) SYSTEM  \n";
        std::cout << "=======================================================\n";
        std::cout << "World Pos         : [" << m_worldTransform.position.x << ", " 
                                           << m_worldTransform.position.y << ", " 
                                           << m_worldTransform.position.z << "]\n";
        
        std::string seasonStr;
        switch (m_currentSeason) {
            case PhenologySeason::EarlySpring: seasonStr = "EARLY SPRING (Lime Buds)"; break;
            case PhenologySeason::Summer:      seasonStr = "SUMMER (Dense Green Curtain)"; break;
            case PhenologySeason::Autumn:      seasonStr = "AUTUMN (Copper / Bronze)"; break;
            case PhenologySeason::Winter:      seasonStr = "WINTER (Bare Skeleton)"; break;
        }

        std::cout << "Phenology State   : " << seasonStr << "\n";
        std::cout << "Leaf Density      : " << (m_leafDensity * 100.0f) << "%\n";
        std::cout << "Leaf Tint RGB     : [" << m_foliageColorRGB.x << ", " << m_foliageColorRGB.y << ", " << m_foliageColorRGB.z << "]\n";
        std::cout << "Major Bough Chains: " << m_majorLimbChains.size() << "\n";
        std::cout << "Weeping Curtains  : " << m_weepingCurtainChains.size() << "\n";
        std::cout << "-------------------------------------------------------\n";

        int totalDrapingNodes = 0;
        for (const auto& curtain : m_weepingCurtainChains) {
            for (const auto& joint : curtain.m_joints) {
                if (joint.bIsDrapingOnGround) totalDrapingNodes++;
            }
        }
        std::cout << "Terrain Draping Active Nodes: " << totalDrapingNodes << "\n";
        std::cout << "=======================================================\n";
    }
};

// ============================================================================
// SIMULATED AAA ENGINE RUNTIME LOOP
// ============================================================================

int main() {
    std::cout << "Initializing AAA Weeping Beech Tree Engine Subsystem...\n";

    WeepingBeechEntity weepingBeech(Vector3(0.0f, 0.0f, 0.0f));
    WorldWindField environmentalWind;
    environmentalWind.baseSpeed = 6.0f; // Moderate gusting breeze

    float deltaSimulatedTime = 0.016f; // 60 FPS tick
    int totalSimulationTicks = 15;

    for (int frame = 0; frame < totalSimulationTicks; ++frame) {
        float engineTime = frame * deltaSimulatedTime;

        // Demonstrate seasonal transition at tick 10
        if (frame == 10) {
            weepingBeech.UpdatePhenologyState(PhenologySeason::Autumn);
        }

        weepingBeech.Tick(deltaSimulatedTime, engineTime, environmentalWind);

        if (frame % 5 == 0 || frame == totalSimulationTicks - 1) {
            std::cout << "\n--> SIMULATION FRAME TICK: " << frame;
            weepingBeech.RenderDebugTelemetry();
        }
    }

    return 0;
}
