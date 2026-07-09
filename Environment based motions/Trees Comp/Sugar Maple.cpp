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

struct MapleVertex {
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    float uvX, uvY;
    float shaggyPlateWeight; // Generates rough curled plate distortions for aging bark
    float leafVeinDistance;  // Normalized channel mapping for procedural autumn color bleeding
};

struct MapleMeshBuffer {
    std::vector<MapleVertex> vertices;
    std::vector<uint32_t> indices;
};

// --- AAA SUGAR MAPLE STRUCTURAL SEGMENT ---
class MapleBranchSegment {
public:
    Vector3 startPoint;
    Vector3 endPoint;
    Vector3 growthDirection;
    float radiusStart;
    float radiusEnd;
    int depthLayer;
    uint32_t segmentUID;

    std::vector<Matrix4x4> instancedPalmateLeafTransforms;

    MapleBranchSegment(Vector3 s, Vector3 e, Vector3 g, float rs, float re, int depth, uint32_t id)
        : startPoint(s), endPoint(e), growthDirection(g), radiusStart(rs), radiusEnd(re), depthLayer(depth), segmentUID(id) {}

    // Generates trunk topology with jagged shaggy plate bark deforms
    void GenerateShaggyTopology(MapleMeshBuffer& meshOut, int radialSegments) {
        uint32_t initialIndexOffset = static_cast<uint32_t>(meshOut.vertices.size());
        Vector3 forward = growthDirection;
        forward.Normalize();

        Vector3 up = (std::abs(forward.y) < 0.9f) ? Vector3{0.0f, 1.0f, 0.0f} : Vector3{1.0f, 0.0f, 0.0f};
        Vector3 right = forward.Cross(up);
        right.Normalize();
        up = right.Cross(forward);
        up.Normalize();

        for (int i = 0; i <= radialSegments; ++i) {
            float angle = (static_cast<float>(i) / radialSegments) * 2.0f * 3.14159265f;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);

            Vector3 radialDir = (right * cosA) + (up * sinA);
            radialDir.Normalize();

            // PROCEDURAL SHAGGY BARK: Vertical lifting plates typical of mature Acer saccharum
            float plateNoise = std::sin(angle * 4.0f) * std::cos(startPoint.y * 0.8f);
            float shaggyWeight = (plateNoise > 0.3f && depthLayer < 3) ? 1.0f : 0.0f;
            
            // Displace plates outward, with higher intensity on the edges to simulate curling away from the trunk
            float plateDeform = shaggyWeight * 0.06f * (plateNoise - 0.3f);
            float finalRadius = 1.0f + plateDeform;

            MapleVertex vBase, vTip;
            vBase.position = startPoint + (radialDir * (radiusStart * finalRadius));
            vBase.normal = radialDir;
            vBase.tangent = right * (-sinA) + up * cosA;
            vBase.uvX = static_cast<float>(i) / radialSegments;
            vBase.uvY = 0.0f;
            vBase.shaggyPlateWeight = shaggyWeight;
            vBase.leafVeinDistance = static_cast<float>(depthLayer) / 8.0f;

            vTip.position = endPoint + (radialDir * (radiusEnd * finalRadius));
            vTip.normal = radialDir;
            vTip.tangent = vBase.tangent;
            vTip.uvX = vBase.uvX;
            vTip.uvY = 1.0f;
            vTip.shaggyPlateWeight = shaggyWeight;
            vTip.leafVeinDistance = vBase.leafVeinDistance;

            meshOut.vertices.push_back(vBase);
            meshOut.vertices.push_back(vTip);
        }

        // Connect Indices Map
        for (int i = 0; i < radialSegments; ++i) {
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

// --- CORE MAPLE ROUNDED CROWN GENERATOR ---
class SugarMaplePipelineManager {
private:
    uint32_t uidPool = 0;
    std::mt19937 randomEngine;

public:
    SugarMaplePipelineManager(unsigned int seed) : randomEngine(seed) {}

    std::vector<std::unique_ptr<MapleBranchSegment>> CompileMassiveCrownGraph(
        Vector3 groundPos, int maxDepth, float startLen, float startRad) 
    {
        std::vector<std::unique_ptr<MapleBranchSegment>> treeGraph;

        struct FrameNode {
            Vector3 start;
            Vector3 direction;
            float length;
            float radius;
            int depth;
        };

        std::queue<FrameNode> processQueue;
        // Stout, massive central trunk that splits into a wide, massive rounded dome/crown
        processQueue.push({groundPos, Vector3{0.0f, 1.0f, 0.0f}, startLen, startRad, 0});

        while (!processQueue.empty()) {
            FrameNode active = processQueue.front();
            processQueue.pop();

            Vector3 endCoord = active.start + (active.direction * active.length);
            float taperRadius = active.radius * 0.75f;

            auto segment = std::make_unique<MapleBranchSegment>(
                active.start, endCoord, active.direction,
                active.radius, taperRadius, active.depth, ++uidPool
            );

            // Populate high-density instanced palmate leaf buffers at terminal structures
            if (active.depth >= maxDepth - 2) {
                PopulatePalmateTransforms(segment->instancedPalmateLeafTransforms, endCoord, 32);
            }

            treeGraph.push_back(std::move(segment));

            if (active.depth < maxDepth) {
                // Wide spreading canopy rule: Heavy outward branching angles to form a giant dome
                int forks = (active.depth < 2) ? 2 : 3;
                float angleSpread = 0.55f;

                for (int i = 0; i < forks; ++i) {
                    float factor = static_cast<float>(i) - static_cast<float>(forks - 1) / 2.0f;
                    Vector3 nextDir = active.direction;
                    
                    std::uniform_real_distribution<float> jitter(-0.15f, 0.15f);
                    nextDir.x += std::sin(factor * angleSpread) * 0.45f + jitter(randomEngine);
                    nextDir.z += std::cos(factor * angleSpread) * 0.45f + jitter(randomEngine);
                    nextDir.y += 0.1f; // Balanced upward and outward trajectory
                    nextDir.Normalize();

                    processQueue.push({
                        endCoord, nextDir,
                        active.length * 0.76f, taperRadius,
                        active.depth + 1
                    });
                }
            }
        }
        return treeGraph;
    }

private:
    void PopulatePalmateTransforms(std::vector<Matrix4x4>& container, const Vector3& tip, int density) {
        std::uniform_real_distribution<float> dist(-1.8f, 1.8f);
        for (int i = 0; i < density; ++i) {
            Matrix4x4 mat = Matrix4x4::Identity();
            
            // Form a dense cloud shell around the twig endpoints
            mat.m[3][0] = tip.x + dist(randomEngine);
            mat.m[3][1] = tip.y + dist(randomEngine) * 0.8f;
            mat.m[3][2] = tip.z + dist(randomEngine);

            // Random orient the flat face planes of the palmate geometry
            float rotY = dist(randomEngine) * 3.14159265f;
            mat.m[0][0] = std::cos(rotY); mat.m[0][2] = std::sin(rotY);

            container.push_back(mat);
        }
    }
};

int main() {
    SugarMaplePipelineManager engine{101};
    MapleMeshBuffer finalMesh;

    std::cout << "[AAA SUGAR MAPLE PIPELINE]: Computing Rounded Crown Layers...\n";
    auto segments = engine.CompileMassiveCrownGraph(Vector3{0.0f, 0.0f, 0.0f}, 6, 15.0f, 2.6f);

    for (const auto& seg : segments) {
        int radialFidelity = (seg->depthLayer < 2) ? 24 : 12;
        seg->GenerateShaggyTopology(finalMesh, radialFidelity);
    }

    std::cout << "-> Successfully Compiled Shaggy Plate Vertex Matrices!\n";
    std::cout << "-> Tree Vertex Allocations: " << finalMesh.vertices.size() << " Active Shader Inputs.\n";
    std::cout << "-> Tree Index Allocations : " << finalMesh.indices.size() << " Render Primitive Elements.\n";
    return 0;
}
