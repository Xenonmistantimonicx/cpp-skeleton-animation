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

struct BanyanVertex {
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    float uvX, uvY;
    float propRootWeight; // Vertex color data: 1.0 = Supporting Pillar, 0.0 = Standard Branch
    float branchUID;      // Dynamic wind phase multiplier offset
};

struct MeshBufferStream {
    std::vector<BanyanVertex> vertices;
    std::vector<uint32_t> indices;
};

// --- AAA BANYAN ARCHITECTURAL SEGMENT ---
class BanyanBranchSegment {
public:
    Vector3 startPoint;
    Vector3 endPoint;
    Vector3 growthDirection;
    float radiusStart;
    float radiusEnd;
    int currentDepth;
    uint32_t segmentUID;

    bool isSupportingPillar = false;
    std::vector<Matrix4x4> instancedLeatheryLeafTransforms;

    BanyanBranchSegment(Vector3 s, Vector3 e, Vector3 g, float rs, float re, int depth, uint32_t id)
        : startPoint(s), endPoint(e), growthDirection(g), radiusStart(rs), radiusEnd(re), depthLayer(depth), segmentUID(id) {}

    // Generates deeply furrowed, gnarled Banyan wood topology
    void GeneratePillarTopology(MeshBufferStream& outMesh, int radialFidelity) {
        uint32_t baseVertexIndex = static_cast<uint32_t>(outMesh.vertices.size());
        Vector3 forward = growthDirection;
        forward.Normalize();

        Vector3 up = (std::abs(forward.y) < 0.9f) ? Vector3{0.0f, 1.0f, 0.0f} : Vector3{1.0f, 0.0f, 0.0f};
        Vector3 right = forward.Cross(up);
        right.Normalize();
        up = right.Cross(forward);
        up.Normalize();

        for (int i = 0; i <= radialFidelity; ++i) {
            float theta = (static_cast<float>(i) / radialFidelity) * 2.0f * 3.14159265f;
            float cosT = std::cos(theta);
            float sinT = std::sin(theta);

            Vector3 radialDir = (right * cosT) + (up * sinT);
            radialDir.Normalize();

            // Procedural Gnarled Furrowing (Characteristic Banyan rough texture)
            float twistFactor = (startPoint.y * 0.1f) + (isSupportingPillar ? 3.0f : 0.0f);
            float roughNoise = 1.0f + 0.15f * std::sin(theta * 6.0f + twistFactor);

            BanyanVertex vBase, vTip;
            vBase.position = startPoint + (radialDir * (radiusStart * roughNoise));
            vBase.normal = radialDir;
            vBase.tangent = right * (-sinT) + up * cosT;
            vBase.uvX = static_cast<float>(i) / radialFidelity;
            vBase.uvY = 0.0f;
            vBase.propRootWeight = isSupportingPillar ? 1.0f : 0.0f;
            vBase.branchUID = static_cast<float>(segmentUID);

            vTip.position = endPoint + (radialDir * (radiusEnd * roughNoise));
            vTip.normal = radialDir;
            vTip.tangent = vBase.tangent;
            vTip.uvX = vBase.uvX;
            vTip.uvY = 1.0f;
            vTip.propRootWeight = vBase.propRootWeight;
            vTip.branchUID = vBase.branchUID;

            outMesh.vertices.push_back(vBase);
            outMesh.vertices.push_back(vTip);
        }

        // Triangulate index buffers
        for (int i = 0; i < radialFidelity; ++i) {
            uint32_t v0 = baseVertexIndex + (i * 2);
            uint32_t v1 = v0 + 1;
            uint32_t v2 = v0 + 2;
            uint32_t v3 = v0 + 3;

            outMesh.indices.push_back(v0);
            outMesh.indices.push_back(v1);
            outMesh.indices.push_back(v2);

            outMesh.indices.push_back(v1);
            outMesh.indices.push_back(v3);
            outMesh.indices.push_back(v2);
        }
    }
};

// --- CORE VATAVRIKSHA CLONAL COMPILER PIPELINE ---
class BanyanPipelineManager {
private:
    uint32_t globalIDAllocator = 0;
    std::mt19937 dynamicRandomEngine;

public:
    BanyanPipelineManager(unsigned int seed) : dynamicRandomEngine(seed) {}

    std::vector<std::unique_ptr<BanyanBranchSegment>> CompileClonalCanopy(
        Vector3 basePosition, int depthLimit, float initialLength, float initialRadius) 
    {
        std::vector<std::unique_ptr<BanyanBranchSegment>> outSegments;

        struct NodeStackItem {
            Vector3 start;
            Vector3 direction;
            float length;
            float radius;
            int depth;
            bool isPillarNode;
        };

        std::queue<NodeStackItem> buildingQueue;
        // Stout massive initial trunk starting from sacred substrate loam
        buildingQueue.push({basePosition, Vector3{0.0f, 1.0f, 0.0f}, initialLength, initialRadius, 0, false});

        while (!buildingQueue.empty()) {
            NodeStackItem active = buildingQueue.front();
            buildingQueue.pop();

            Vector3 computedEndPoint = active.start + (active.direction * active.length);
            float taperRadius = active.radius * 0.72f;

            auto segment = std::make_unique<BanyanBranchSegment>(
                active.start, computedEndPoint, active.direction,
                active.radius, taperRadius, active.depth, ++globalIDAllocator
            );
            segment->isSupportingPillar = active.isPillarNode;

            // Generate localized hanging prop roots from horizontal heavy branches
            if (active.depth >= 2 && active.depth <= 4 && std::abs(active.direction.y) < 0.4f) {
                if (std::uniform_real_distribution<float>(0, 1)(dynamicRandomEngine) < 0.15f) {
                    buildingQueue.push({computedEndPoint, Vector3{0.0f, -1.0f, 0.0f}, 45.0f, active.radius * 0.4f, 10, true});
                }
            }

            // Generate terminal nodes instanced leaves matrix buffer
            if (active.depth >= depthLimit - 2 && !active.isPillarNode) {
                PopulateInstanceLeatheryTransforms(segment->instancedLeatheryLeafTransforms, computedEndPoint, 24);
            }

            outSegments.push_back(std::move(segment));

            if (active.depth < depthLimit && !active.isPillarNode) {
                // Symmetrical binary splits pushing out massively horizontally
                int forksCount = (active.depth < 2) ? 2 : (active.depth > 6 ? 1 : 3);
                
                for (int i = 0; i < forksCount; ++i) {
                    float factor = static_cast<float>(i) - static_cast<float>(forksCount - 1) / 2.0f;
                    
                    Vector3 nextDir = active.direction;
                    std::uniform_real_distribution<float> angleJitter(-0.2f, 0.2f);
                    nextDir.x += std::sin(factor * 0.55f) * 0.35f + angleJitter(dynamicRandomEngine);
                    nextDir.z += std::cos(factor * 0.55f) * 0.35f + angleJitter(dynamicRandomEngine);
                    // Adjust downward gravity pull on higher tiers to push horizontal expanse
                    nextDir.y += 0.25f * (1.0f - active.depth / static_cast<float>(depthLimit));
                    nextDir.Normalize();

                    buildingQueue.push({
                        computedEndPoint, nextDir,
                        active.length * 0.76f, taperRadius,
                        active.depth + 1, false
                    });
                }
            }
        }
        return outSegments;
    }

private:
    void PopulateInstanceLeatheryTransforms(std::vector<Matrix4x4>& container, const Vector3& tip, int count) {
        std::uniform_real_distribution<float> distribution(-1.1f, 1.1f);
        for (int i = 0; i < count; ++i) {
            Matrix4x4 mat = Matrix4x4::Identity();
            // Assign matrices clustered tightly around the twig end coordinate
            mat.m[3][0] = tip.x + distribution(dynamicRandomEngine);
            mat.m[3][1] = tip.y + distribution(dynamicRandomEngine) * 0.6f;
            mat.m[3][2] = tip.z + distribution(dynamicRandomEngine);

            // Random rotation around Y for organic orientation layout
            float yaw = distribution(dynamicRandomEngine) * 3.14159265f;
            mat.m[0][0] = std::cos(yaw); mat.m[0][2] = std::sin(yaw);
            mat.m[2][0] = -std::sin(yaw); mat.m[2][2] = std::cos(yaw);

            container.push_back(mat);
        }
    }
};

int main() {
    BanyanPipelineManager engine{108};
    MeshBufferStream finalGeometryMesh;

    std::cout << "[GINKGO SEED INITIALIZED]: Processing Grand Assembly Topology Layers...\n";
    auto totalSegments = engine.CompileClonalCanopy(Vector3{0.0f, 0.0f, 0.0f}, 7, 15.0f, 2.5f);

    for (const auto& seg : totalSegments) {
        int steps = (seg->depthLayer < 3 || seg->isSupportingPillar) ? 18 : 10;
        seg->GeneratePillarTopology(finalGeometryMesh, steps);
    }

    std::cout << "[FINAL ASSEMBLY COMPLETE]\n";
    std::cout << "-> Buffer Stream Vertices: " << finalGeometryMesh.vertices.size() << " Unique Node Points.\n";
    std::cout << "-> Buffer Stream Indices : " << finalGeometryMesh.indices.size() << " Triangle Facet Mappings.\n";
    return 0;
}
