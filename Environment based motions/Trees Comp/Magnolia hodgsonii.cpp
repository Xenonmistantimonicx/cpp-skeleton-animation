#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>
#include <random>
#include <algorithm>

const float PI = 3.14159265359f;

struct Vector3 {
    float x, y, z;
    Vector3 operator+(const Vector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vector3 operator-(const Vector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vector3 Cross(const Vector3& v) const { return { y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x }; }
    float Dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
    void Normalize() { float len = std::sqrt(x*x + y*y + z*z); if(len > 0.0f) { x /= len; y /= len; z /= len; } }
};

struct MagnoliaBoneNode {
    int nodeID;
    int parentID;
    Vector3 localOffset;
    Vector3 animatedRotation; // Pitch, Yaw, Roll tracking
    float structuralMass;     // Higher for terminal branches holding giant flowers
    float flexInertiaDamping; // Resistance coefficient to rapid oscillations
    int biologicalClassification; // 0 = Trunk Core, 1 = Secondary Branch, 2 = Heavy Blossom Tip
};

struct MagnoliaVertexPBR {
    Vector3 position;
    Vector3 normal;
    float u, v;
    float boneWeight;
    float stipuleScarIntensity; // Structural roughness factor mapping ring scars
    float componentType;        // 0.0 = Fluted Wood, 1.0 = Massive Blade Leaf, 2.0 = Heavy Terminal Flower
};

class MagnoliaAnimationPipelineEngine {
private:
    std::vector<MagnoliaBoneNode> m_Skeleton;
    std::vector<MagnoliaVertexPBR> m_MeshVertices;
    std::vector<uint32_t>          m_MeshIndices;
    float m_GlobalRuntimeTimer;

public:
    MagnoliaAnimationPipelineEngine() : m_GlobalRuntimeTimer(0.0f) {}

    void InitializeSkeletalRig() {
        std::cout << "[AAA ANIMATION-ENGINE]: Constructing Custom Rigid Hierarchy for Magnolia hodgsonii...\n";
        
        // Root Trunk base
        m_Skeleton.push_back({0, -1, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 800.0f, 0.95f, 0});
        // Mid Trunk Section
        m_Skeleton.push_back({1, 0, {0.0f, 4.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 500.0f, 0.90f, 0});
        // Primary Crown Branching Fork
        m_Skeleton.push_back({2, 1, {-1.5f, 3.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, 120.0f, 0.65f, 1});
        m_Skeleton.push_back({3, 1, {1.8f, 2.8f, -0.8f}, {0.0f, 0.0f, 0.0f}, 135.0f, 0.62f, 1});
        
        // Terminal endpoints loaded with heavy monolithic ivory blossoms
        m_Skeleton.push_back({4, 2, {-1.2f, 1.8f, 0.5f}, {0.0f, 0.0f, 0.0f}, 45.0f, 0.32f, 2}); // Heavy Flower weight load
        m_Skeleton.push_back({5, 3, {1.4f, 2.0f, -0.6f}, {0.0f, 0.0f, 0.0f}, 50.0f, 0.28f, 2});
    }

    // SIMULATES HIGH-INERTIA STOCHASTIC WIND RESPONSE
    // Calculates low-frequency pendular swings for heavy leaves and flower nodes
    void SolveProceduralWindSimulation(Vector3 baselineWindVector, float gustIntensity, float deltaTime) {
        m_GlobalRuntimeTimer += deltaTime;

        for (auto& bone : m_Skeleton) {
            if (bone.parentID == -1) continue; // Base root anchored inside soil terrain matrix

            // Generate structural frequencies using multiple sine harmonics
            float phaseShift = bone.nodeID * 1.67f;
            float primaryWave   = std::sin(m_GlobalRuntimeTimer * 1.2f + phaseShift);
            float turbulentGust = std::cos(m_GlobalRuntimeTimer * 3.8f - phaseShift) * gustIntensity;

            // Compute projection matrix of wind force against bone vector directions
            float forceProjection = baselineWindVector.x * 0.4f + baselineWindVector.z * 0.6f;
            float totalMechanicalForce = (primaryWave * 0.7f + turbulentGust * 0.9f) * forceProjection;

            // SYSTEM BEHAVIOR INERTIA OVERRIDES VIA BOTANICAL CLASSES
            if (bone.biologicalClassification == 2) {
                // Heavy Terminal Blossom Node: Low frequency, high deflection amplitude, long pendulum decay
                float heavyDroopFactor = 0.25f * (bone.structuralMass / 50.0f); // Gravity sag simulation
                bone.animatedRotation.x = (totalMechanicalForce * 0.45f) + heavyDroopFactor; // Sag along Pitch
                bone.animatedRotation.z = (totalMechanicalForce * 0.35f);                     // Roll sway
            } 
            else if (bone.biologicalClassification == 1) {
                // Secondary structural wood: Medium resistance
                bone.animatedRotation.x = totalMechanicalForce * 0.15f * (1.0f - bone.flexInertiaDamping);
                bone.animatedRotation.y = totalMechanicalForce * 0.08f;
            }
            else {
                // Massive core trunk: Minimal deflection under heavy gusts
                bone.animatedRotation.z = totalMechanicalForce * 0.02f;
            }
        }
    }

    void GenerateMeshProfile() {
        std::cout << "[AAA MESH-BUILDER]: Extruding Fluted Trunk and Injecting Massive Blade Leaf Clusters...\n";
        
        int segments = 20;
        float segmentRadius = 0.8f;

        for (int i = 0; i <= segments; ++i) {
            float vTrack = (float)i / segments;
            Vector3 center = {0.0f, i * 0.5f, 0.0f};
            uint32_t ringStartIdx = static_cast<uint32_t>(m_MeshVertices.size());

            int radialPoints = 32;
            for (int r = 0; r <= radialPoints; ++r) {
                float angle = 2.0f * PI * (float)r / radialPoints;

                // FLUTED TRUNK GEOMETRY MATH
                // Generates deep structural grooves running vertically along the bark profile
                float flutingRidges = 1.0f + std::sin(angle * 5.0f) * 0.12f; 
                
                // Add explicit horizontal rings representing stipule scar zones
                float annularRingCheck = std::abs(std::sin(vTrack * 25.0f));
                float scarFactor = (annularRingCheck > 0.94f) ? 1.0f : 0.0f;

                float computedRadius = segmentRadius * (1.0f - vTrack * 0.5f) * flutingRidges;
                Vector3 offsetVector = {std::cos(angle) * computedRadius, 0.0f, std::sin(angle) * computedRadius};

                MagnoliaVertexPBR v;
                v.position = center + offsetVector;
                v.normal = offsetVector; v.normal.Normalize();
                v.u = (float)r / radialPoints;
                v.v = vTrack * 4.0f;
                v.boneWeight = (vTrack < 0.5f) ? 0.0f : 1.0f; // Assign skin bindings to mid skeleton
                v.stipuleScarIntensity = scarFactor;
                v.componentType = 0.0f; // Wood class tag

                m_MeshVertices.push_back(v);
            }

            // Topology Grid Stitching
            if (i < segments) {
                for (int r = 0; r < radialPoints; ++r) {
                    uint32_t cRow = ringStartIdx;
                    uint32_t nRow = ringStartIdx + (radialPoints + 1);

                    m_MeshIndices.push_back(cRow + r);
                    m_MeshIndices.push_back(nRow + r);
                    m_MeshIndices.push_back(cRow + r + 1);
                    m_MeshIndices.push_back(cRow + r + 1);
                    m_MeshIndices.push_back(nRow + r);
                    m_MeshIndices.push_back(nRow + r + 1);
                }
            }
        }
        
        // Spawn leaf node assets at terminal indices
        SpawnMassiveLeafBlades({0.0f, 10.0f, 0.0f});
    }

private:
    void SpawnMassiveLeafBlades(Vector3 apexPoint) {
        uint32_t customStartOffset = static_cast<uint32_t>(m_MeshVertices.size());
        
        // Inject 2-foot long leathery leaf cards with micro-stipule traces
        for(int l = 0; l < 6; ++l) {
            float localAngle = (float)l * (2.0f * PI / 6.0f);
            Vector3 leafOffset = {std::cos(localAngle) * 1.5f, 0.0f, std::sin(localAngle) * 1.5f};
            Vector3 absPos = apexPoint + leafOffset;

            MagnoliaVertexPBR leafVert;
            leafVert.position = absPos;
            leafVert.normal = {0.0f, 1.0f, 0.0f};
            leafVert.u = 0.5f; leafVert.v = 0.5f;
            leafVert.boneWeight = 1.0f;
            leafVert.stipuleScarIntensity = 0.0f;
            leafVert.componentType = 1.0f; // Leaf classification profile

            m_MeshVertices.push_back(leafVert);
        }
        
        // Generate pseudo topology indexing markers
        for(size_t i = 0; i < 4; ++i) {
            m_MeshIndices.push_back(customStartOffset);
            m_MeshIndices.push_back(customStartOffset + static_cast<uint32_t>(i) + 1);
            m_MeshIndices.push_back(customStartOffset + static_cast<uint32_t>(i) + 2);
        }
    }

public:
    void ExportStateSnapshotOBJ(const std::string& path) {
        std::ofstream stream(path);
        if (!stream.is_open()) return;

        stream << "# Production High-Inertia Animation Simulation Capture: Magnolia hodgsonii\n";
        for (const auto& v : m_MeshVertices) {
            // Apply simple runtime bone scaling vectors directly inside compile pipeline to simulate deformation
            float rotationOffsetAmt = std::sin(m_Skeleton[1].animatedRotation.x) * v.boneWeight * 0.4f;
            stream << "v " << v.position.x + rotationOffsetAmt << " " << v.position.y << " " << v.position.z << "\n";
        }
        for (const auto& v : m_MeshVertices) stream << "vt " << v.u << " " << v.v << "\n";
        for (const auto& v : m_MeshVertices) stream << "vn " << v.normal.x << " " << v.normal.y << " " << v.componentType << "\n";

        stream << "\ng Magnolia_Hodgsonii_System\nusemtl M_Magnolia_Master_PBR\n";
        for (size_t i = 0; i < m_MeshIndices.size(); i += 3) {
            stream << "f " << m_MeshIndices[i]+1 << "/" << m_MeshIndices[i]+1 << "/" << m_MeshIndices[i]+1 << " "
                   << m_MeshIndices[i+1]+1 << "/" << m_MeshIndices[i+1]+1 << "/" << m_MeshIndices[i+1]+1 << " "
                   << m_MeshIndices[i+2]+1 << "/" << m_MeshIndices[i+2]+1 << "/" << m_MeshIndices[i+2]+1 << "\n";
        }
        stream.close();
        std::cout << "[SIMULATION PIPELINE SUCCESS]: State Snapshot compiled to: " << path << "\n";
    }
};

int main() {
    MagnoliaAnimationPipelineEngine runtimePipeline;
    runtimePipeline.InitializeSkeletalRig();
    runtimePipeline.GenerateMeshProfile();
    
    // Simulate real-time ticks under severe weather conditions
    Vector3 baselineStormWind = {2.5f, -0.2f, 1.8f};
    float dynamicGustFactor = 1.45f;
    float simulatedDeltaTime = 0.016f; // Standard 60FPS lock frame runtime step

    std::cout << "[SIMULATION RUNTIME]: Ticking physics loop for 120 frames...\n";
    for(int frame = 0; frame < 120; ++frame) {
        runtimePipeline.SolveProceduralWindSimulation(baselineStormWind, dynamicGustFactor, simulatedDeltaTime);
    }

    runtimePipeline.ExportStateSnapshotOBJ("Magnolia_Hodgsonii_Animated.obj");
    return 0;
}
