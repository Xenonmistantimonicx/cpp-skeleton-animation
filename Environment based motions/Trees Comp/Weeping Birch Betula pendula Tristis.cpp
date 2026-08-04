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
};

struct Transform {
    Vector3 position;
    Quaternion rotation;
    Vector3 scale{ 1.0f, 1.0f, 1.0f };
};

// ============================================================================
// PHENOLOGY & ENVIRONMENTAL DATA STRUCTURES
// ============================================================================

enum class BirchPhenologySeason {
    EarlySpring,   // Catkins bloom, tender light-green leaf unfurling
    Summer,        // Glossy dark-green triangular leaves, dense canopy
    Autumn,        // Brilliant golden-yellow foliage transition
    Winter         // Bare white skeletal frame with dormant hanging catkins
};

struct DynamicWindField {
    Vector3 windDirection{ 0.8f, 0.0f, 0.6f };
    float baseVelocity{ 3.8f };        // Meters per second
    float gustFactor{ 0.5f };
    float flutterFrequency{ 14.0f };   // High frequency for thin birch petioles

    Vector3 SampleWindAtPosition(const Vector3& pos, float time) const {
        float noise = std::sin(pos.x * 0.2f + time * 2.1f) * std::cos(pos.z * 0.2f + time * 1.7f);
        float speed = baseVelocity + (noise * gustFactor * baseVelocity);
        return windDirection.Normalized() * speed;
    }
};

// ============================================================================
// WEEPING BIRCH BRANCH NODE & PENDULUM IK
// ============================================================================

struct BirchBranchJoint {
    Vector3 localPos;
    Vector3 worldPos;
    Vector3 prevWorldPos;      // Verlet position buffer
    Vector3 anatomicalRestDir; // Natural botanical orientation
    
    float length{ 0.45f };
    float radius{ 0.03f };     // Extremely thin branchlet tips
    float flexuralRigidity{ 0.2f }; // Low rigidity = high whip-like flexibility
    float damping{ 0.88f };
    
    // Phenology attributes
    bool bHasCatkin{ false };
    float catkinLength{ 0.05f }; // 5cm pendulous catkins
};

class WeepingBirchBranchChain {
public:
    std::vector<BirchBranchJoint> m_joints;
    Vector3 m_rootAttachmentWorldPos;
    bool m_bIsPrimaryLimb{ false };

public:
    WeepingBirchBranchChain() = default;

    void BuildProceduralBirchChain(const Vector3& startPos, const Vector3& initialDir, int segmentCount, float startRadius, bool isPrimary) {
        m_rootAttachmentWorldPos = startPos;
        m_bIsPrimaryLimb = isPrimary;

        Vector3 currentPos = startPos;
        Vector3 dir = initialDir.Normalized();

        for (int i = 0; i < segmentCount; ++i) {
            BirchBranchJoint joint;
            float t = static_cast<float>(i) / static_cast<float>(segmentCount);

            if (!m_bIsPrimaryLimb) {
                // Secondary weeping branchlets drop rapidly straight down (-Y)
                dir = (dir * (1.0f - t) + Vector3(0.0f, -1.0f, 0.0f) * t * 2.0f).Normalized();
                joint.bHasCatkin = (i >= segmentCount - 2); // Catkins cluster at tips
            }

            joint.length = 0.4f * (1.0f - t * 0.2f);
            joint.radius = startRadius * (1.0f - t * 0.8f);
            joint.flexuralRigidity = m_bIsPrimaryLimb ? (0.92f - t * 0.25f) : (0.15f - t * 0.12f);
            joint.anatomicalRestDir = dir;

            currentPos = currentPos + (dir * joint.length);
            joint.localPos = currentPos - startPos;
            joint.worldPos = currentPos;
            joint.prevWorldPos = currentPos;

            m_joints.push_back(joint);
        }
    }

    void PhysicsTick(const Vector3& rootPos, const DynamicWindField& wind, float dt, float time) {
        m_rootAttachmentWorldPos = rootPos;
        if (m_joints.empty()) return;

        // 1. Verlet Integration & Aerodynamic Force Application
        for (size_t i = 0; i < m_joints.size(); ++i) {
            auto& joint = m_joints[i];

            Vector3 velocity = (joint.worldPos - joint.prevWorldPos) * joint.damping;
            joint.prevWorldPos = joint.worldPos;

            // Gravity Force (Whip-like hanging mass)
            Vector3 gravity(0.0f, -9.81f * (1.0f - joint.flexuralRigidity * 0.8f), 0.0f);

            // Aerodynamic Drag & High-Frequency Petiole Flutter
            Vector3 windVel = wind.SampleWindAtPosition(joint.worldPos, time);
            float flutterNoise = std::sin(time * wind.flutterFrequency + joint.worldPos.y * 4.0f);
            Vector3 flutterForce = Vector3(0.0f, flutterNoise * 0.3f, flutterNoise * 0.3f) * (m_bIsPrimaryLimb ? 0.1f : 1.0f);
            
            Vector3 windForce = (windVel * (m_bIsPrimaryLimb ? 0.08f : 0.35f)) + flutterForce;

            // Anatomical Spring Stiffness
            Vector3 parentPos = (i == 0) ? m_rootAttachmentWorldPos : m_joints[i - 1].worldPos;
            Vector3 targetAnatomicalPos = parentPos + (joint.anatomicalRestDir * joint.length);
            Vector3 springForce = (targetAnatomicalPos - joint.worldPos) * (joint.flexuralRigidity * 50.0f);

            Vector3 totalAccel = gravity + windForce + springForce;
            joint.worldPos = joint.worldPos + velocity + (totalAccel * dt * dt);
        }

        // 2. FABRIK Distance Constraints
        m_joints[0].worldPos = m_rootAttachmentWorldPos + (m_joints[0].worldPos - m_rootAttachmentWorldPos).Normalized() * m_joints[0].length;

        for (size_t i = 1; i < m_joints.size(); ++i) {
            Vector3 parentPos = m_joints[i - 1].worldPos;
            Vector3 delta = m_joints[i].worldPos - parentPos;
            float dist = delta.Magnitude();

            if (dist > 0.0001f) {
                m_joints[i].worldPos = parentPos + (delta / dist) * m_joints[i].length;
            }
        }
    }
};

// ============================================================================
// AAA WEEPING BIRCH SYSTEM SUBSYSTEM
// ============================================================================

class WeepingBirchEntity {
private:
    Transform m_worldTransform;
    BirchPhenologySeason m_season{ BirchPhenologySeason::Summer };

    // Anatomical Attributes
    float m_treeHeight{ 18.0f };             // Mature Betula pendula 'Tristis' height
    float m_barkBetulinWhiteness{ 0.95f };   // 0.0 (Base Fissured Bark) to 1.0 (Pure White Upper Trunk)
    
    // Procedural Skeleton
    std::vector<WeepingBirchBranchChain> m_primaryBoughs;
    std::vector<WeepingBirchBranchChain> m_droopingBranchlets;

    // Phenological Telemetry
    float m_leafDensity{ 1.0f };             // 0.0 (Winter) to 1.0 (Summer)
    float m_catkinScale{ 1.0f };
    Vector3 m_foliageColorRGB{ 0.12f, 0.38f, 0.08f };

public:
    WeepingBirchEntity(const Vector3& spawnPos) {
        m_worldTransform.position = spawnPos;
        GenerateSkeletalStructure();
        SetSeason(BirchPhenologySeason::Summer);
    }

    ~WeepingBirchEntity() = default;

    void Tick(float deltaTime, float engineTime, const DynamicWindField& wind) {
        // Step 1: Update Primary Boughs
        for (auto& bough : m_primaryBoughs) {
            bough.PhysicsTick(bough.m_rootAttachmentWorldPos, wind, deltaTime, engineTime);
        }

        // Step 2: Update Drooping Branchlets attached to moving bough nodes
        size_t branchletIdx = 0;
        for (auto& bough : m_primaryBoughs) {
            for (size_t j = 2; j < bough.m_joints.size(); ++j) {
                if (branchletIdx < m_droopingBranchlets.size()) {
                    Vector3 attachPos = bough.m_joints[j].worldPos;
                    m_droopingBranchlets[branchletIdx].PhysicsTick(attachPos, wind, deltaTime, engineTime);
                    branchletIdx++;
                }
            }
        }
    }

    void SetSeason(BirchPhenologySeason newSeason) {
        m_season = newSeason;

        switch (m_season) {
            case BirchPhenologySeason::EarlySpring:
                m_leafDensity = 0.3f;
                m_catkinScale = 1.2f; // Long golden blooming catkins
                m_foliageColorRGB = Vector3(0.55f, 0.82f, 0.20f); // Bright tender lime
                break;
            case BirchPhenologySeason::Summer:
                m_leafDensity = 1.0f;
                m_catkinScale = 0.4f;
                m_foliageColorRGB = Vector3(0.10f, 0.36f, 0.08f); // Glossy birch green
                break;
            case BirchPhenologySeason::Autumn:
                m_leafDensity = 0.8f;
                m_catkinScale = 0.8f; // Male catkins forming for winter
                m_foliageColorRGB = Vector3(0.92f, 0.72f, 0.05f); // Brilliant golden yellow
                break;
            case BirchPhenologySeason::Winter:
                m_leafDensity = 0.0f;
                m_catkinScale = 0.7f; // Dormant catkins hanging on bare branches
                m_foliageColorRGB = Vector3(0.0f, 0.0f, 0.0f);
                break;
        }
    }

private:
    void GenerateSkeletalStructure() {
        const int primaryBoughCount = 6;

        for (int i = 0; i < primaryBoughCount; ++i) {
            float angle = (360.0f / primaryBoughCount) * i * (3.14159f / 180.0f);
            
            // Primary limbs grow UPWARDS at a steep angle (45-60 deg) before sweeping out
            Vector3 boughDir(std::cos(angle) * 0.6f, 0.8f, std::sin(angle) * 0.6f);
            Vector3 boughOrigin = m_worldTransform.position + Vector3(0.0f, m_treeHeight * 0.6f, 0.0f);

            WeepingBirchBranchChain boughChain;
            boughChain.BuildProceduralBirchChain(boughOrigin, boughDir, 7, 0.22f, true);
            m_primaryBoughs.push_back(boughChain);

            // Attach whip-like pendulous branchlets along the bough length
            for (size_t j = 2; j < boughChain.m_joints.size(); ++j) {
                Vector3 attachPoint = boughChain.m_joints[j].worldPos;
                Vector3 weepingDir(std::cos(angle + 0.3f) * 0.15f, -0.95f, std::sin(angle + 0.3f) * 0.15f);

                WeepingBirchBranchChain branchletChain;
                branchletChain.BuildProceduralBirchChain(attachPoint, weepingDir, 12, 0.04f, false);
                m_droopingBranchlets.push_back(branchletChain);
            }
        }
    }

public:
    void RenderDebugTelemetry() const {
        std::cout << "\n=======================================================\n";
        std::cout << " AAA BETULA PENDULA 'TRISTIS' (WEEPING BIRCH) SUBSYSTEM \n";
        std::cout << "=======================================================\n";
        std::cout << "World Pos         : [" << m_worldTransform.position.x << ", " 
                                           << m_worldTransform.position.y << ", " 
                                           << m_worldTransform.position.z << "]\n";

        std::string seasonStr;
        switch (m_season) {
            case BirchPhenologySeason::EarlySpring: seasonStr = "EARLY SPRING (Lime Leaves & Catkin Bloom)"; break;
            case BirchPhenologySeason::Summer:      seasonStr = "SUMMER (Dense Glossy Canopy)"; break;
            case BirchPhenologySeason::Autumn:      seasonStr = "AUTUMN (Golden Yellow Transition)"; break;
            case BirchPhenologySeason::Winter:      seasonStr = "WINTER (Bare White Frame & Dormant Catkins)"; break;
        }

        std::cout << "Phenology Season  : " << seasonStr << "\n";
        std::cout << "Leaf Density      : " << (m_leafDensity * 100.0f) << "%\n";
        std::cout << "Catkin Scale      : " << m_catkinScale << "x\n";
        std::cout << "Primary Boughs    : " << m_primaryBoughs.size() << "\n";
        std::cout << "Drooping Branchlets: " << m_droopingBranchlets.size() << "\n";
        std::cout << "=======================================================\n";
    }
};

// ============================================================================
// SIMULATED AAA ENGINE RUNTIME LOOP
// ============================================================================

int main() {
    std::cout << "Initializing AAA Weeping Birch Tree Subsystem...\n";

    WeepingBirchEntity weepingBirch(Vector3(0.0f, 0.0f, 0.0f));
    DynamicWindField environmentalWind;

    float dt = 0.016f; // 60 FPS tick
    int totalTicks = 15;

    for (int frame = 0; frame < totalTicks; ++frame) {
        float engineTime = frame * dt;

        if (frame == 10) {
            weepingBirch.SetSeason(BirchPhenologySeason::Autumn);
        }

        weepingBirch.Tick(dt, engineTime, environmentalWind);

        if (frame % 5 == 0 || frame == totalTicks - 1) {
            std::cout << "\n--> SIMULATION FRAME TICK: " << frame;
            weepingBirch.RenderDebugTelemetry();
        }
    }

    return 0;
}
