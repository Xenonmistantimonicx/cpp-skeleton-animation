#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>
#include <algorithm>

const float ENGINE_PI = 3.14159265359f;

struct TransformVector3D {
    float x, y, z;
    TransformVector3D operator+(const TransformVector3D& v) const { return {x + v.x, y + v.y, z + v.z}; }
    TransformVector3D operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
};

struct FicusCallosaVertex {
    TransformVector3D position;
    TransformVector3D normal;
    float texU, texV;
    float scabrousRoughnessIntensity; // Texture mapping data mask for micro-roughness concentration
    float twigSwellingSwayMask;       // 0.0 = Rigid column trunk, 1.0 = Swollen twig nodule tips
    float verticalHeightIndex;        // Height factor tracking structural mechanical dampening
};

struct ColumnarSkeletalNode {
    int jointID;
    int parentJointID;
    TransformVector3D spatialDirection;
    float rigidStiffnessFactor;       // Callosa twigs are extremely thick/stiff compared to droopy figs
    float angularDeflectionLimit;
    float runtimeElasticSwayResponse;
};

class FicusCallosaSimulationCore {
private:
    std::vector<ColumnarSkeletalNode> m_TreeBones;
    std::vector<FicusCallosaVertex>    m_VertexBuffer;
    std::vector<uint32_t>             m_IndexBuffer;
    float m_TimeSystemState;

public:
    FicusCallosaSimulationCore() : m_TimeSystemState(0.0f) {}

    void ConstructSlenderColumnarSkeleton() {
        std::cout << "[AAA-CALLOSA]: Generating Tall Slender Columnar Trunk & Rigid Twig Matrix...\n";
        
        // Base Anchor Location (Root point)
        m_TreeBones.push_back({0, -1, {0.0f, 0.0f, 0.0f}, 1.00f, 0.00f, 0.0f});
        // Slender Tall Trunk Profile Segments (High structural integrity)
        m_TreeBones.push_back({1, 0, {0.0f, 6.0f, 0.0f}, 0.95f, 0.02f, 0.0f});
        m_TreeBones.push_back({2, 1, {0.0f, 5.5f, 0.0f}, 0.88f, 0.05f, 0.0f});
        
        // Terminal Swollen Twigs (Thick ends, high rigid resistance profile)
        m_TreeBones.push_back({3, 2, {-1.8f, 1.2f, 0.4f}, 0.92f, 0.12f, 0.0f}); // Swollen twig terminal chain A
        m_TreeBones.push_back({4, 2, {1.9f, 1.0f, -0.3f}, 0.94f, 0.10f, 0.0f});  // Swollen twig terminal chain B
    }

    void EvaluateRigidWindStiffness(TransformVector3D externalWindForce, float frameDelta) {
        m_TimeSystemState += frameDelta;

        for (auto& bone : m_TreeBones) {
            if (bone.parentJointID == -1) continue;

            // Ficus callosa displays highly limited elastic resonance due to thick rigid branches
            float highFrequencyDampeningNoise = std::sin(m_TimeSystemState * 3.8f + (bone.jointID * 1.5f));
            
            // Stiffness parameter heavily resists structural bending vectors compared to normal banyans
            float effectiveBendingAllowance = (1.0f - bone.rigidStiffnessFactor) * bone.angularDeflectionLimit;
            
            float calculatedSwayForce = (externalWindForce.x * 0.4f + externalWindForce.z * 0.6f) * effectiveBendingAllowance;
            bone.runtimeElasticSwayResponse = calculatedSwayForce * highFrequencyDampeningNoise;
        }
    }

    void GenerateScabrousMeshBuffers() {
        std::cout << "[AAA-MESH]: Processing Sandpaper Surface Vertex Arrays...\n";
        
        int circularSegments = 12;
        int verticalSteps = 20;

        for (int v = 0; v <= verticalSteps; ++v) {
            float heightRatio = (float)v / verticalSteps;
            float verticalY = heightRatio * 11.5f;

            // Callosa features a straight columnar profile with minimal base trunk flaring
            float columnRadius = 0.8f * (1.0f - (heightRatio * 0.45f));

            for (int c = 0; c < circularSegments; ++c) {
                float currentAngle = ((float)c / circularSegments) * 2.0f * ENGINE_PI;

                FicusCallosaVertex vert;
                vert.position = {std::cos(currentAngle) * columnRadius, verticalY, std::sin(currentAngle) * columnRadius};
                vert.normal = {std::cos(currentAngle), 0.0f, std::sin(currentAngle)};
                vert.texU = (float)c / circularSegments;
                vert.texV = heightRatio;
                
                // Pack high-frequency data properties inside the engine vertex buffers directly
                vert.scabrousRoughnessIntensity = 0.85f; // High base sandpaper texture scaling
                vert.twigSwellingSwayMask = (heightRatio > 0.8f) ? 1.0f : 0.0f;
                vert.verticalHeightIndex = heightRatio;

                m_VertexBuffer.push_back(vert);
            }
        }
        std::cout << "[AAA-MESH SUCCESS]: Vertex structural maps generated successfully.\n";
    }
};

int main() {
    FicusCallosaSimulationCore callosaAssetPipeline;
    callosaAssetPipeline.ConstructSlenderColumnarSkeleton();
    callosaAssetPipeline.GenerateScabrousMeshBuffers();

    TransformVector3D highCanopyCrossWind = {2.8f, 0.0f, 1.9f};
    float executionDeltaTime = 0.016f;

    std::cout << "[SIMULATION RUNTIME]: Evaluating high-frequency leaf-twig dampening vectors...\n";
    for(int frame = 0; frame < 30; ++frame) {
        callosaAssetPipeline.EvaluateRigidWindStiffness(highCanopyCrossWind, executionDeltaTime);
    }

    return 0;
}
