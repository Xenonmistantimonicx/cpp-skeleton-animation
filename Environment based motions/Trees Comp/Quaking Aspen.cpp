#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include <random>
#include <queue>

// --- CORE MATHEMATICAL STRUCTURES ---
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

struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    float uvX, uvY;
    float quakingWeight; // Vertex color alpha channel masking
    float branchID;      // Dynamic wind phase multiplier offset
};

struct ProceduralMeshBuffer {
    std::vector<Vertex> vertexBuffer;
    std::vector<uint32_t> indexBuffer;
};

struct BarkScarInstance {
    Vector3 projectionCenter;
    float radius;
    float depthIntensity;
};

// --- AAA PROCEDURAL APEN SEGMENT SPECIFICATION ---
class AspenGrowthSegment {
public:
    Vector3 startPosition;
    Vector3 endPosition;
    Vector3 growthDirection;
    float baseRadius;
    float tipRadius;
    int currentLODLevel;
    uint32_t segmentUID;

    std::vector<BarkScarInstance> projectedScars;
    std::vector<Matrix4x4> instanceLeafTransforms;

    AspenGrowthSegment(Vector3 start, Vector3 end, Vector3 dir, float baseR, float tipR, int lod, uint32_t id)
        : startPosition(start), endPosition(end), growthDirection(dir), baseRadius(baseR), tipRadius(tipR), currentLODLevel(lod), segmentUID(id) {}

    // Generates a fully smooth procedural cylinder profile with custom UV alignment for white bark tiling
    void GenerateSkeletalMesh(ProceduralMeshBuffer& outMesh, int radialSegments) {
        uint32_t baseVertexIndex = static_cast<uint32_t>(outMesh.vertexBuffer.size());
        Vector3 forward = growthDirection;
        forward.Normalize();

        // Establish an orthonormal basis matrix for smooth ring generation
        Vector3 up = (std::abs(forward.y) < 0.9f) ? Vector3{0.0f, 1.0f, 0.0f} : Vector3{1.0f, 0.0f, 0.0f};
        Vector3 right = forward.Cross(up);
        right.Normalize();
        up = right.Cross(forward);
        up.Normalize();

        // 1. Generate Continuous Smooth Vertices Stream
        for (int i = 0; i <= radialSegments; ++i) {
            float angle = (static_cast<float>(i) / radialSegments) * 2.0f * 3.14159265f;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);

            Vector3 radialDir = (right * cosA) + (up * sinA);
            radialDir.Normalize();

            // Compute precise continuous profile topology interpolation
            Vertex baseVert, tipVert;
            baseVert.position = startPosition + (radialDir * baseRadius);
            baseVert.normal = radialDir;
            baseVert.tangent = right * (-sinA) + up * cosA;
            baseVert.uvX = static_cast<float>(i) / radialSegments;
            baseVert.uvY = 0.0f;
            baseVert.quakingWeight = 0.0f; // Structural wood remains stiff
            baseVert.branchID = static_cast<float>(segmentUID);

            tipVert.position = endPosition + (radialDir * tipRadius);
            tipVert.normal = radialDir;
            tipVert.tangent = baseVert.tangent;
            tipVert.uvX = baseVert.uvX;
            tipVert.uvY = 1.0f;
            tipVert.quakingWeight = 0.0f;
            tipVert.branchID = static_cast<float>(segmentUID);

            // Procedural Scar Mesh Deformation Projection
            ApplyScarDeformation(baseVert);
            ApplyScarDeformation(tipVert);

            outMesh.vertexBuffer.push_back(baseVert);
            outMesh.vertexBuffer.push_back(tipVert);
        }

        // 2. Compute Index Buffer topology layout for smooth hardware rendering
        for (int i = 0; i < radialSegments; ++i) {
            uint32_t v0 = baseVertexIndex + (i * 2);
            uint32_t v1 = v0 + 1;
            uint32_t v2 = v0 + 2;
            uint32_t v3 = v0 + 3;

            // Triangle 1
            outMesh.indexBuffer.push_back(v0);
            outMesh.indexBuffer.push_back(v1);
            outMesh.indexBuffer.push_back(v2);

            // Triangle 2
            outMesh.indexBuffer.push_back(v1);
            outMesh.indexBuffer.push_back(v3);
            outMesh.indexBuffer.push_back(v2);
        }
    }

private:
    void ApplyScarDeformation(Vertex& vertex) {
        for (const auto& scar : projectedScars) {
            Vector3 delta = vertex.position - scar.projectionCenter;
            float distance = std::sqrt(delta.Dot(delta));
            if (distance < scar.radius) {
                // Smooth hermite interpolation calculation for indenting eye-shaped scars
                float t = distance / scar.radius;
                float profileFactor = (1.0f - (3.0f * t * t - 2.0f * t * t * t));
                
                // Indent vertex position inward along inverse normal axis
                vertex.position = vertex.position - (vertex.normal * (scar.depthIntensity * profileFactor));
                // Darken uv assignment locally to map specialized texture coordinates
                vertex.uvX += 0.5f * profileFactor; 
            }
        }
    }
};

// --- PROCEDURAL GENERATION & LOD SYSTEMS MANAGER ---
class QuakingAspenPipelineManager {
private:
    uint32_t globalUIDCounter = 0;
    std::mt19937 generatorEngine;

public:
    QuakingAspenPipelineManager(unsigned int seed) : generatorEngine(seed) {}

    std::vector<std::unique_ptr<AspenGrowthSegment>> ExecuteGenerationPipeline(
        Vector3 rootPosition, int maxRecursionDepth, float initialLength, float initialRadius) 
    {
        std::vector<std::unique_ptr<AspenGrowthSegment>> outSegments;
        
        // Use an explicit memory allocation tracking queue instead of stack-overflow prone recursion
        struct ConstructionNode {
            Vector3 startPos;
            Vector3 direction;
            float currentLength;
            float currentRadius;
            int currentDepth;
        };

        std::queue<ConstructionNode> buildingQueue;
        buildingQueue.push({rootPosition, Vector3{0.0f, 1.0f, 0.0f}, initialLength, initialRadius, 0});

        while (!buildingQueue.empty()) {
            ConstructionNode activeNode = buildingQueue.front();
            buildingQueue.pop();

            Vector3 calculatedEndPoint = activeNode.startPos + (activeNode.direction * activeNode.currentLength);
            float taperRadius = activeNode.currentRadius * 0.72f;

            auto segment = std::make_unique<AspenGrowthSegment>(
                activeNode.startPos, calculatedEndPoint, activeNode.direction, 
                activeNode.currentRadius, taperRadius, activeNode.currentDepth, ++globalUIDCounter
            );

            // Inject procedural dark eye scars on main base trunks
            if (activeNode.currentDepth < 3) {
                std::uniform_real_distribution<float> scarDist(0.1f, 0.9f);
                Vector3 scarCenter = activeNode.startPos + (activeNode.direction * (activeNode.currentLength * scarDist(generatorEngine)));
                segment->projectedScars.push_back({scarCenter, activeNode.currentRadius * 1.1f, activeNode.currentRadius * 0.25f});
            }

            // Generate structural leaves instance transforms at terminal nodes
            if (activeNode.currentDepth >= maxRecursionDepth - 2) {
                PopulateInstancedLeafBuffer(segment->instanceLeafTransforms, calculatedEndPoint, 24);
            }

            outSegments.push_back(std::move(segment));

            if (activeNode.currentDepth < maxRecursionDepth) {
                // Symmetrical binary/tertiary split math execution rules
                int splits = (activeNode.currentDepth < 2) ? 2 : 3;
                for (int i = 0; i < splits; ++i) {
                    float factor = static_cast<float>(i) - static_cast<float>(splits - 1) / 2.0f;
                    
                    Vector3 nextDirection = activeNode.direction;
                    nextDirection.x += std::sin(factor * 0.45f) * 0.25f;
                    nextDirection.z += std::cos(factor * 0.45f) * 0.25f;
                    nextDirection.y += 0.4f; // Maintain high slender vertical pull axis
                    nextDirection.Normalize();

                    buildingQueue.push({
                        calculatedEndPoint, nextDirection, 
                        activeNode.currentLength * 0.78f, taperRadius, 
                        activeNode.currentDepth + 1
                    });
                }
            }
        }
        return outSegments;
    }

private:
    void PopulateInstancedLeafBuffer(std::vector<Matrix4x4>& buffer, const Vector3& anchor, int count) {
        std::uniform_real_distribution<float> spread(-1.2f, 1.2f);
        for (int i = 0; i < count; ++i) {
            Matrix4x4 mat = Matrix4x4::Identity();
            // Inject hardware instance translations
            mat.m[3][0] = anchor.x + spread(generatorEngine);
            mat.m[3][1] = anchor.y + spread(generatorEngine);
            mat.m[3][2] = anchor.z + spread(generatorEngine);
            buffer.push_back(mat);
        }
    }
};

int main() {
    QuakingAspenPipelineManager engine{1337};
    ProceduralMeshBuffer finalTreeGeometry;

    std::cout << "[PIPELINE INITIALIZED]: Processing Grand Assembly Topology Layers...\n";
    auto totalSegments = engine.ExecuteGenerationPipeline(Vector3{0.0f, 0.0f, 0.0f}, 7, 12.0f, 1.8f);

    // Compute hardware index buffers and multi-layered vertex loops
    for (const auto& seg : totalSegments) {
        // Adjust smooth radial fidelity automatically depending on L-System depth layer (Dynamic CPU LOD)
        int smoothnessLOD = (seg->currentLODLevel < 3) ? 16 : 8;
        seg->GenerateSkeletalMesh(finalTreeGeometry, smoothnessLOD);
    }

    std::cout << "[COMPILED STRUCTURAL TOPOLOGY SUCCESSFULLY]\n";
    std::cout << "-> Compiled Vertices Allocation: " << finalTreeGeometry.vertexBuffer.size() << " Unique Vertex Streams.\n";
    std::cout << "-> Compiled Indices Allocation : " << finalTreeGeometry.indexBuffer.size() << " Index Indices Array.\n";
    return 0;
}
