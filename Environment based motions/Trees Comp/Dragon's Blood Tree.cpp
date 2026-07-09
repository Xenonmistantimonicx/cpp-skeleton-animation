#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include <random>
#include <queue>

// --- CORE TRANSLATION & MATRIX MATH TYPE UTILITIES ---
struct Vector3 {
    float x, y, z;
    Vector3 operator+(const Vector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vector3 operator-(const Vector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vector3 Cross(const Vector3& v) const { return { y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x }; }
    float Dot(const Vector3& v) const { return x*v.x + y*v.y + z*v.z; }
    void Normalize() { float len = std::sqrt(x*x + y*y + z*z); if(len > 0.0f) { x /= len; y /= len; z /= len; } }
};

struct Matrix4x4 {
    float m[4][4];
    static Matrix4x4 Identity() {
        Matrix4x4 mat{};
        mat.m[0][0] = 1.0f; mat.m[1][1] = 1.0f; mat.m[2][2] = 1.0f; mat.m[3][3] = 1.0f;
        return mat;
    }
};

struct DragonsVertex {
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    float uvX, uvY;
    float bleedingWeight; // Vertex color data assigning dynamic resin fluid origin channels
    float structuralTier; // Identifies horizontal canopy layers for local wind multipliers
};

struct GeometryMeshBuffer {
    std::vector<DragonsVertex> vertices;
    std::vector<uint32_t> indices;
};

struct CinnabarBleedNode {
    Vector3 coordinateOrigin;
    float structuralStressFactor;
    float viscosityFlowRate;
};

// --- AAA DRAGONS BLOOD MODEL STRUCTURAL NODE ---
class DragonsBranchSegment {
public:
    Vector3 nodeStart;
    Vector3 nodeEnd;
    Vector3 orientationVec;
    float radiusBottom;
    float radiusTop;
    int generationDepth;
    uint32_t branchUID;

    std::vector<CinnabarBleedNode> activeFluidEmitters;
    std::vector<Matrix4x4> instancedSwordLeafTransforms;

    DragonsBranchSegment(Vector3 s, Vector3 e, Vector3 o, float rb, float rt, int gen, uint32_t id)
        : nodeStart(s), nodeEnd(e), orientationVec(o), radiusBottom(rb), radiusTop(rt), generationDepth(gen), branchUID(id) {}

    // Generates scale-like, highly compartmentalized grey-brown armor bark topology
    void GenerateArmoredTopology(GeometryMeshBuffer& meshOut, int radialSteps) {
        uint32_t initialIndexOffset = static_cast<uint32_t>(meshOut.vertices.size());
        Vector3 forward = orientationVec;
        forward.Normalize();

        Vector3 up = (std::abs(forward.y) < 0.9f) ? Vector3{0.0f, 1.0f, 0.0f} : Vector3{1.0f, 0.0f, 0.0f};
        Vector3 right = forward.Cross(up);
        right.Normalize();
        up = right.Cross(forward);
        up.Normalize();

        for (int i = 0; i <= radialSteps; ++i) {
            float theta = (static_cast<float>(i) / radialSteps) * 2.0f * 3.14159265f;
            float cosT = std::cos(theta);
            float sinT = std::sin(theta);

            Vector3 radialDir = (right * cosT) + (up * sinT);
            radialDir.Normalize();

            // Dracaena bark profile has dynamic blocky corrugation (segmented structural layout)
            float plateCorrugation = 1.0f + 0.05f * std::sin(theta * 12.0f) * std::sin(nodeStart.y * 2.0f);

            DragonsVertex vBase, vTip;
            vBase.position = nodeStart + (radialDir * (radiusBottom * plateCorrugation));
            vBase.normal = radialDir;
            vBase.tangent = right * (-sinT) + up * cosT;
            vBase.uvX = static_cast<float>(i) / radialSteps;
            vBase.uvY = 0.0f;
            vBase.bleedingWeight = (generationDepth >= 3 && generationDepth <= 5) ? 1.0f : 0.0f;
            vBase.structuralTier = static_cast<float>(generationDepth);

            vTip.position = nodeEnd + (radialDir * (radiusTop * plateCorrugation));
            vTip.normal = radialDir;
            vTip.tangent = vBase.tangent;
            vTip.uvX = vBase.uvX;
            vTip.uvY = 1.0f;
            vTip.bleedingWeight = vBase.bleedingWeight;
            vTip.structuralTier = vBase.structuralTier;

            meshOut.vertices.push_back(vBase);
            meshOut.vertices.push_back(vTip);
        }

        // Stitch geometric buffer index maps
        for (int i = 0; i < radialSteps; ++i) {
            uint32_t v0 = initialIndexOffset + (i * 2);
            uint32_t v1 = v0 + 1;
            uint32_t v2 = v0 + 2;
            uint32_t v3 = v0 + 3;

            meshOut.indices.push_back(v0);
            meshOut.indices.push_back(v1);
            meshOut.indices.push_back(v2);

            meshOut.indices.push_back(v1);
            meshOut.indices.push_back(v3);
            meshOut.indices.push_back(v2);
        }
    }
};

// --- CORE MATHEMATICAL DICHOTOMOUS COMPILER PIPELINE ---
class DragonsBloodPipelineManager {
private:
    uint32_t uniqueIDTracker = 0;
    std::mt19937 proceduralRandomEngine;

public:
    DragonsBloodPipelineManager(unsigned int seed) : proceduralRandomEngine(seed) {}

    std::vector<std::unique_ptr<DragonsBranchSegment>> CompileTreeGraph(
        Vector3 groundOrigin, int depthLimit, float startLen, float startRad) 
    {
        std::vector<std::unique_ptr<DragonsBranchSegment>> structuralGraph;

        struct LSystemFrame {
            Vector3 start;
            Vector3 heading;
            float segmentLength;
            float segmentRadius;
            int currentDepth;
        };

        std::queue<LSystemFrame> executionQueue;
        // Stout, solid lower trunk structure baseline setup
        executionQueue.push({groundOrigin, Vector3{0.0f, 1.0f, 0.0f}, startLen, startRad, 0});

        while (!executionQueue.empty()) {
            LSystemFrame activeFrame = executionQueue.front();
            executionQueue.pop();

            Vector3 endpoint = activeFrame.start + (activeFrame.heading * activeFrame.segmentLength);
            float coreTaperRadius = activeFrame.segmentRadius * 0.76f;

            auto segment = std::make_unique<DragonsBranchSegment>(
                activeFrame.start, endpoint, activeFrame.heading,
                activeFrame.segmentRadius, coreTaperRadius, activeFrame.currentDepth, ++uniqueIDTracker
            );

            // Add dynamic fluids bleeding emitters inside major structural node splits
            if (activeFrame.currentDepth >= 2 && activeFrame.currentDepth <= 4) {
                segment->activeFluidEmitters.push_back({endpoint, 0.85f, 0.05f});
            }

            // Generate Terminal Sword-Leaf Rigid Rosette Matrix Arrays for Instanced Draw Calls
            if (activeFrame.currentDepth == depthLimit) {
                GenerateRigidRosetteTransforms(segment->instancedSwordLeafTransforms, endpoint, 40);
            }

            structuralGraph.push_back(std::move(segment));

            if (activeFrame.currentDepth < depthLimit) {
                // Strict Binary Dichotomous Branch Split Rule: Always 2 symmetric paths
                int splitMultiplier = 2;
                
                // Force an increasing outer angle spread calculation to lock the iconic flat ceiling plate profile
                float baselineSpread = 0.35f + (static_cast<float>(activeFrame.currentDepth) * 0.08f);

                for (int i = 0; i < splitMultiplier; ++i) {
                    float orientationSign = (i == 0) ? -1.0f : 1.0f;

                    Vector3 divergentHeading = activeFrame.heading;
                    divergentHeading.x += std::sin(orientationSign * baselineSpread) * 0.45f;
                    divergentHeading.z += std::cos(orientationSign * baselineSpread) * 0.45f;
                    // Gradually decrease vertical Y lift on higher bounds to flatten umbrella perimeter bounds
                    divergentHeading.y *= (1.0f - (static_cast<float>(activeFrame.currentDepth) * 0.05f)); 
                    divergentHeading.Normalize();

                    executionQueue.push({
                        endpoint, divergentHeading,
                        activeFrame.segmentLength * 0.82f, coreTaperRadius,
                        activeFrame.currentDepth + 1
                    });
                }
            }
        }
        return structuralGraph;
    }

private:
    void GenerateRigidRosetteTransforms(std::vector<Matrix4x4>& instanceContainer, const Vector3& tipPivot, int leafDensity) {
        std::uniform_real_distribution<float> angleDist(-1.0f, 1.0f);
        for (int i = 0; i < leafDensity; ++i) {
            Matrix4x4 transformMatrix = Matrix4x4::Identity();
            // Bundle positions right above the terminal twig point
            transformMatrix.m[3][0] = tipPivot.x + angleDist(proceduralRandomEngine) * 0.2f;
            transformMatrix.m[3][1] = tipPivot.y + std::uniform_real_distribution<float>(0.0f, 0.8f)(proceduralRandomEngine);
            transformMatrix.m[3][2] = tipPivot.z + angleDist(proceduralRandomEngine) * 0.2f;

            // Stiff outward rotation vector alignment calculation
            float pitch = angleDist(proceduralRandomEngine) * 0.4f - 0.5f; // Pointing upwards and out
            float yaw = angleDist(proceduralRandomEngine) * 3.14159265f;
            
            transformMatrix.m[0][0] = std::cos(yaw); transformMatrix.m[0][2] = std::sin(yaw);
            transformMatrix.m[1][1] = std::cos(pitch);
            
            instanceContainer.push_back(transformMatrix);
        }
    }
};

int main() {
    DragonsBloodPipelineManager compiler{777};
    GeometryMeshBuffer finalMeshAsset;

    std::cout << "[PRODUCTION PIPELINE ACTIVATED]: Drawing Dichotomous Architecture Graph...\n";
    auto totalSegments = compiler.CompileTreeGraph(Vector3{0.0f, 0.0f, 0.0f}, 6, 16.0f, 2.5f);

    for (const auto& chunk : totalSegments) {
        // High polygon count on lower thick stems, low fidelity profile mapping on high tiers
        int fidelitySteps = (chunk->generationDepth < 2) ? 24 : 12;
        chunk->GenerateArmoredTopology(finalMeshAsset, fidelitySteps);
    }

    std::cout << "-> Successfully Batch Processed Mesh Streams!\n";
    std::cout << "-> Total Vertices Generated: " << finalMeshAsset.vertices.size() << " Hardware Vertex Inputs.\n";
    std::cout << "-> Total Indices Generated : " << finalMeshAsset.indices.size() << " Indices Buffers Array.\n";
    return 0;
}
