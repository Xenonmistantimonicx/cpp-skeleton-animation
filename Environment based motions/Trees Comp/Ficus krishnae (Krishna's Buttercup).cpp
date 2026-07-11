#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>
#include <memory>

const float PI = 3.14159265359f;

struct MathVector3 {
    float x, y, z;
    MathVector3 operator+(const MathVector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    MathVector3 operator-(const MathVector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    MathVector3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    void Normalize() { float len = std::sqrt(x*x + y*y + z*z); if(len > 0.0f) { x /= len; y /= len; z /= len; } }
};

// SIMD-aligned highly packed structure for vertex arrays (AAA Engine standard layout)
struct FicusVertexAAA {
    MathVector3 position;
    MathVector3 normal;
    float texU, texV;
    float cupPocketDepth;   // Mask tracking the interior depth of the folded cup leaf
    float dynamicRainLoad;  // Weight multiplier calculated at runtime when raining
    float componentTypeID;  // 0.0 = Corrugated Trunk, 1.0 = Pocket Leaf, 2.0 = Aerial Prop Root
};

struct FicusSkeletalNode {
    int jointIndex;
    int parentJointIndex;
    MathVector3 localRestPosition;
    MathVector3 activeRotation;
    float windDragCoefficient; // Leaf-cups act as parachutes, catching heavy aerodynamic drag
    float currentHydroWeight;   // Weight accumulator from trapped water inside the leaf cup pockets
    float structuralFlexDamping;
};

class FicusKrishnaeSimulationEngine {
private:
    std::vector<FicusSkeletalNode> m_SkeletonNodes;
    std::vector<FicusVertexAAA>    m_RenderVertices;
    std::vector<uint32_t>          m_RenderIndices;
    float m_SimulationTimeClock;

public:
    FicusKrishnaeSimulationEngine() : m_SimulationTimeClock(0.0f) {}

    void BuildSkeletalHierarchy() {
        std::cout << "[AAA-FICUS]: Spawning High-Performance Skeleton with Prop-Root Chains...\n";
        
        // Root Anchor Node
        m_SkeletonNodes.push_back({0, -1, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.05f, 0.0f, 0.98f});
        // Main Composite Trunk Core
        m_SkeletonNodes.push_back({1, 0, {0.0f, 3.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.15f, 0.0f, 0.92f});
        // Crown Primary Spline Forks
        m_SkeletonNodes.push_back({2, 1, {-2.0f, 2.5f, 0.5f}, {0.0f, 0.0f, 0.0f}, 0.65f, 0.0f, 0.55f});
        m_SkeletonNodes.push_back({3, 1, {2.2f, 2.3f, -0.5f}, {0.0f, 0.0f, 0.0f}, 0.68f, 0.0f, 0.52f});
        
        // Aerial Prop Roots: Low mechanical drag but highly subject to pendular sway closer to earth
        m_SkeletonNodes.push_back({4, 2, {0.0f, -3.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.25f, 0.0f, 0.35f});
        m_SkeletonNodes.push_back({5, 3, {0.0f, -2.8f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.22f, 0.0f, 0.38f});
    }

    // HYDRO-DYNAMICS AND AERODYNAMIC DRAG SOLVER
    // Simulates water pooling inside the unique leaf pockets and updating physics parameters
    void ProcessEnvironmentalSimulation(MathVector3 globalWindVec, float precipitationIntensity, float dt) {
        m_SimulationTimeClock += dt;

        for (auto& joint : m_SkeletonNodes) {
            if (joint.parentJointIndex == -1) continue;

            // Step 1: Hydro-Loading simulation loop
            if (joint.windDragCoefficient > 0.5f) { // Identifies leaf-heavy structural clusters
                // Leaf cups pool water efficiently up to a threshold, increasing mass and droop gravity
                joint.currentHydroWeight = std::min(4.5f, joint.currentHydroWeight + (precipitationIntensity * 0.4f * dt));
            }

            // Step 2: Stochastic multi-harmonic wind turbulence with high drag multipliers for cup shapes
            float frequencyScaler = 1.45f + (joint.jointIndex * 0.2f);
            float baseOscillation = std::sin(m_SimulationTimeClock * frequencyScaler) * 0.6f;
            
            // Aerodynamic drag force projection modified by accumulated water weight
            float operationalDrag = joint.windDragCoefficient * (1.0f + (joint.currentHydroWeight * 0.15f));
            float windForceMagnitude = (globalWindVec.x * 0.5f + globalWindVec.z * 0.8f) * operationalDrag;

            // Step 3: Branch bending and structural deformation offsets
            float dynamicBendingMoment = (baseOscillation * windForceMagnitude) * (1.0f - joint.structuralFlexDamping);
            
            // Hydro-droop sag injected into Pitch Axis (X)
            float verticalGravitySag = joint.currentHydroWeight * 0.08f;
            joint.activeRotation.x = dynamicBendingMoment + verticalGravitySag;
            joint.activeRotation.z = dynamicBendingMoment * 0.7f; // Roll sway axis response
        }
    }

    void GenerateOptimizedGeometry() {
        std::cout << "[AAA-GEOMETRY]: Generating Conical Cup Leaves and Rope-Like Aerial Root Mesh Data...\n";
        
        // Procedurally populate cup leaf profile matrices
        int leavesToSpawn = 48;
        for (int i = 0; i < leavesToSpawn; ++i) {
            float leafNormalizedPhase = (float)i / leavesToSpawn;
            float radialAngle = leafNormalizedPhase * 2.0f * PI;

            // Generate explicit mathematical cones representing the Ficus Krishnae cup morphology
            MathVector3 apex = {std::cos(radialAngle) * 2.5f, 6.0f + std::sin(leafNormalizedPhase * 4.0f) * 0.5f, std::sin(radialAngle) * 2.5f};
            
            // Build 3 vertex profiles for the pocket leaf to track cup interior vs exterior rim profiles
            for (int vStep = 0; vStep < 3; ++vStep) {
                FicusVertexAAA vert;
                float depthFactor = (float)vStep / 2.0f; // 1.0 implies deepest zone of the cup pocket
                
                vert.position = apex + MathVector3{std::cos(radialAngle) * 0.3f * depthFactor, -0.4f * depthFactor, std::sin(radialAngle) * 0.3f * depthFactor};
                vert.normal = {0.0f, 1.0f, 0.0f}; // Upfacing profile normals for smooth lighting vectors
                vert.texU = leafNormalizedPhase;
                vert.texV = depthFactor;
                vert.cupPocketDepth = depthFactor; // Pack cup structural topology directly inside vertex buffers
                vert.dynamicRainLoad = 0.0f;
                vert.componentTypeID = 1.0f; // Leaf element identification

                m_RenderVertices.push_back(vert);
            }
            
            uint32_t currentBaseIndex = static_cast<uint32_t>(m_RenderVertices.size()) - 3;
            m_RenderIndices.push_back(currentBaseIndex);
            m_RenderIndices.push_back(currentBaseIndex + 1);
            m_RenderIndices.push_back(currentBaseIndex + 2);
        }
    }

    void ExportSimulationStateOBJ(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) return;

        file << "# AAA High-Fidelity Ficus Krishnae Technical Simulation Outflow\n";
        for (const auto& v : m_RenderVertices) {
            // Apply skeletal runtime deformation matrix mapping to simulate hydro-droop directly
            float simulatedBendingDeformation = std::sin(m_SkeletonNodes[2].activeRotation.x) * v.cupPocketDepth * 0.5f;
            file << "v " << v.position.x << " " << v.position.y - simulatedBendingDeformation << " " << v.position.z << "\n";
        }
        for (const auto& v : m_RenderVertices) file << "vt " << v.texU << " " << v.texV << "\n";
        for (const auto& v : m_RenderVertices) file << "vn " << v.normal.x << " " << v.normal.y << " " << v.componentTypeID << "\n";

        file << "\ng Ficus_Krishnae_Mesh\nusemtl M_Ficus_Master_PBR\n";
        for (size_t i = 0; i < m_RenderIndices.size(); i += 3) {
            file << "f " << m_RenderIndices[i]+1 << "/" << m_RenderIndices[i]+1 << "/" << m_RenderIndices[i]+1 << " "
                 << m_RenderIndices[i+1]+1 << "/" << m_RenderIndices[i+1]+1 << "/" << m_RenderIndices[i+1]+1 << " "
                 << m_RenderIndices[i+2]+1 << "/" << m_RenderIndices[i+2]+1 << "/" << m_RenderIndices[i+2]+1 << "\n";
        }
        file.close();
        std::cout << "[PIPELINE SUCCESS]: Game ready snapshot generated: " << filename << "\n";
    }
};

int main() {
    FicusKrishnaeSimulationEngine engineInstance;
    engineInstance.BuildSkeletalHierarchy();
    engineInstance.GenerateOptimizedGeometry();

    // Simulating extreme monsoonal weather downpour conditions (High rain load, intense gusting forces)
    MathVector3 monsoonStormWind = {3.2f, -0.4f, 2.5f};
    float densePrecipitationVolume = 1.85f; // Heavy rain volume per frame accumulation
    float gameLoopDeltaTime = 0.016f;       // Fixed 60FPS lock duration

    std::cout << "[SIMULATION RUNTIME]: Processing hydro-loading skeletal physics updates...\n";
    for(int frame = 0; frame < 60; ++frame) {
        engineInstance.ProcessEnvironmentalSimulation(monsoonStormWind, densePrecipitationVolume, gameLoopDeltaTime);
    }

    engineInstance.ExportSimulationStateOBJ("Ficus_Krishnae_Production.obj");
    return 0;
}
