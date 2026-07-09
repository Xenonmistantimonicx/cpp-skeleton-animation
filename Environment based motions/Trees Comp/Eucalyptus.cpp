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

struct EucalyptusVertex {
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    float uvX, uvY;
    float peelLayerWeight;  // Controls the vertex offset for peeling ribbon bark strips
    float pendulumPhase;    // Unique phase offset for hanging leaf clusters
};

struct EucalyptusMeshBuffer {
    std::vector<EucalyptusVertex> vertices;
    std::vector<uint32_t> indices;
};

// --- AAA EUCALYPTUS STRUCTURAL SEGMENT ---
class EucalyptusBranchSegment {
public:
    Vector3 startPoint;
    Vector3 endPoint;
    Vector3 growthDirection;
    float radiusStart;
    float radiusEnd;
    int depthLayer;
    uint32_t segmentUID;

    std::vector<Matrix4x4> pendulousLeafTransforms;

    EucalyptusBranchSegment(Vector3 s, Vector3 e, Vector3 g, float rs, float re, int depth, uint32_t id)
        : startPoint(s), endPoint(e), growthDirection(g), radiusStart(rs), radiusEnd(re), depthLayer(depth), segmentUID(id) {}

    // Generates trunk topology with structural ribbon peeling mesh deformation
    void GeneratePeelingTopology(EucalyptusMeshBuffer& meshOut, int radialSegments) {
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

            // PROCEDURAL BARK PEELING: Certain vertical bands (strips) expand outward to look like peeling ribbons
            float longitudinalWave = std::sin(startPoint.y * 0.3f + angle * 2.0f);
            float peelWeight = (longitudinalWave > 0.6f) ? 1.0f : 0.0f;
            
            // Displace peeling vertices slightly outward from the trunk core mesh
            float peelOffset = (peelWeight * 0.04f * std::sin(startPoint.y * 2.0f));
            float finalRadiusScale = 1.0f + peelOffset;

            EucalyptusVertex vBase, vTip;
            vBase.position = startPoint + (radialDir * (radiusStart * finalRadiusScale));
            vBase.normal = radialDir;
            vBase.tangent = right * (-sinA) + up * cosA;
            vBase.uvX = static_cast<float>(i) / radialSegments;
            vBase.uvY = 0.0f;
            vBase.peelLayerWeight = peelWeight;
            vBase.pendulumPhase = static_cast<float>(segmentUID) * 0.5f;

            vTip.position = endPoint + (radialDir * (radiusEnd * finalRadiusScale));
            vTip.normal = radialDir;
            vTip.tangent = vBase.tangent;
            vTip.uvX = vBase.uvX;
            vTip.uvY = 1.0f;
            vTip.peelLayerWeight = peelWeight;
            vTip.pendulumPhase = vBase.pendulumPhase;

            meshOut.vertices.push_back(vBase);
            meshOut.vertices.push_back(vTip);
        }

        // Connect Indices Mappings
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

// --- CORE EUCALYPTUS HIGH-REACHING PIPELINE ---
class EucalyptusPipelineManager {
private:
    uint32_t uidPool = 0;
    std::mt19937 dynamicRandomEngine;

public:
    EucalyptusPipelineManager(unsigned int seed) : dynamicRandomEngine(seed) {}

    std::vector<std::unique_ptr<EucalyptusBranchSegment>> CompileTallCanopyGraph(
        Vector3 groundPos, int maxDepth, float startLen, float startRad) 
    {
        std::vector<std::unique_ptr<EucalyptusBranchSegment>> treeGraph;

        struct FrameNode {
            Vector3 start;
            Vector3 direction;
            float length;
            float radius;
            int depth;
        };

        std::queue<FrameNode> processQueue;
        // Tall, soaring, sweeping main trunk initialization
        processQueue.push({groundPos, Vector3{0.05f, 0.95f, 0.0f}, startLen, startRad, 0});

        while (!processQueue.empty()) {
            FrameNode active = processQueue.front();
            processQueue.pop();

            Vector3 endCoord = active.start + (active.direction * active.length);
            float taperRadius = active.radius * 0.72f;

            auto segment = std::make_unique<EucalyptusBranchSegment>(
                active.start, endCoord, active.direction,
                active.radius, taperRadius, active.depth, ++uidPool
            );

            // Generate Drooping Pendulous Leaf Instances at terminal ends
            if (active.depth >= maxDepth - 2) {
                PopulateHangingLeafTransforms(segment->pendulousLeafTransforms, endCoord, 28);
            }

            treeGraph.push_back(std::move(segment));

            if (active.depth < maxDepth) {
                // Main branches shoot upward sharply before looping out gracefully
                int forks = (active.depth == 0) ? 1 : 2; // Smooth continuous trunk early on
                
                for (int i = 0; i < forks; ++i) {
                    float bias = (i == 0) ? -1.0f : 1.0f;
                    Vector3 nextDir = active.direction;
                    
                    std::uniform_real_distribution<float> jitter(-0.2f, 0.2f);
                    nextDir.x += (bias * 0.35f) + jitter(dynamicRandomEngine);
                    nextDir.z += jitter(dynamicRandomEngine);
                    nextDir.y += 0.2f; // High vertical reach
                    nextDir.Normalize();

                    processQueue.push({
                        endCoord, nextDir,
                        active.length * 0.78f, taperRadius,
                        active.depth + 1
                    });
                }
            }
        }
        return treeGraph;
    }

private:
    void PopulateHangingLeafTransforms(std::vector<Matrix4x4>& buffer, const Vector3& tip, int density) {
        std::uniform_real_distribution<float> radiusDist(-1.5f, 1.5f);
        for (int i = 0; i < density; ++i) {
            Matrix4x4 mat = Matrix4x4::Identity();
            
            // Positions slightly lower than attachment node to model hanging/pendulous foliage
            mat.m[3][0] = tip.x + radiusDist(dynamicRandomEngine) * 0.6f;
            mat.m[3][1] = tip.y + radiusDist(dynamicRandomEngine) * 0.5f - 1.2f; // Forced downward hang
            mat.m[3][2] = tip.z + radiusDist(dynamicRandomEngine) * 0.6f;

            // Invert scale vectors slightly on instances to random-orient long thin sickle leaves
            float spin = radiusDist(dynamicRandomEngine) * 3.14159265f;
            mat.m[0][0] = std::cos(spin); mat.m[0][2] = std::sin(spin);

            buffer.push_back(mat);
        }
    }
};

int main() {
    EucalyptusPipelineManager engine{999};
    EucalyptusMeshBuffer compiledAsset;

    std::cout << "[AAA EUCALYPTUS ENGINE]: Computing Peeling Layer Streams...\n";
    auto segments = engine.CompileTallCanopyGraph(Vector3{0.0f, 0.0f, 0.0f}, 7, 18.0f, 2.4f);

    for (const auto& seg : segments) {
        int steps = (seg->depthLayer < 2) ? 24 : 12;
        seg->GeneratePeelingTopology(compiledAsset, steps);
    }

    std::cout << "-> Successfully Bundled Multi-Layer Ribbon Bark Vertex Cache!\n";
    std::cout << "-> Stream Target Vertices: " << compiledAsset.vertices.size() << " Points Array.\n";
    std::cout << "-> Stream Target Indices : " << compiledAsset.indices.size() << " Faces Indices.\n";
    return 0;
}
