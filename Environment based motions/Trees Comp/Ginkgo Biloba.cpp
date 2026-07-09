#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include <random>
#include <queue>

// --- CORE TRANSLATION & MATH TYPES ---
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

struct GinkgoVertex {
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    float uvX, uvY;
    float spurShootId;    // Identifies specialized leaf grouping clusters
    float structuralAge;  // For procedural cork-bark erosion layers
};

struct MeshStreamBuffer {
    std::vector<GinkgoVertex> vertices;
    std::vector<uint32_t> indices;
};

struct SpurShootCluster {
    Vector3 pivotPoint;
    Vector3 outwardNormal;
    float densityRadius;
};

// --- AAA GINKGO STRUCTURAL SEGMENT ---
class GinkgoBranchSegment {
public:
    Vector3 startPos;
    Vector3 endPos;
    Vector3 growthVec;
    float radiusStart;
    float radiusEnd;
    int depthLayer;
    uint32_t uniqueID;

    std::vector<SpurShootCluster> localizedSpurs;
    std::vector<Matrix4x4> instancedFanLeafTransforms;

    GinkgoBranchSegment(Vector3 s, Vector3 e, Vector3 g, float rs, float re, int depth, uint32_t id)
        : startPos(s), endPos(e), growthVec(g), radiusStart(rs), radiusEnd(re), depthLayer(depth), uniqueID(id) {}

    // Generates the deeply furrowed, cork-like slate grey bark profile topology
    void BuildSegmentTopology(MeshStreamBuffer& outBuffer, int radialFidelity) {
        uint32_t rootIndex = static_cast<uint32_t>(outBuffer.vertices.size());
        Vector3 forward = growthVec;
        forward.Normalize();

        Vector3 up = (std::abs(forward.y) < 0.9f) ? Vector3{0.0f, 1.0f, 0.0f} : Vector3{1.0f, 0.0f, 0.0f};
        Vector3 right = forward.Cross(up);
        right.Normalize();
        up = right.Cross(forward);
        up.Normalize();

        for (int i = 0; i <= radialFidelity; ++i) {
            float alpha = (static_cast<float>(i) / radialFidelity) * 2.0f * 3.14159265f;
            float cosA = std::cos(alpha);
            float sinA = std::sin(alpha);

            Vector3 radialDir = (right * cosA) + (up * sinA);
            radialDir.Normalize();

            // Apply procedural furrowing distortion (Ginkgo's signature rough fissured bark)
            float furrowNoise = 1.0f + 0.12f * std::sin(alpha * 5.0f) * std::cos(startPos.y * 0.5f);

            GinkgoVertex vStart, vEnd;
            vStart.position = startPos + (radialDir * (radiusStart * furrowNoise));
            vStart.normal = radialDir;
            vStart.tangent = right * (-sinA) + up * cosA;
            vStart.uvX = static_cast<float>(i) / radialFidelity;
            vStart.uvY = 0.0f;
            vStart.spurShootId = static_cast<float>(uniqueID);
            vStart.structuralAge = static_cast<float>(depthLayer) / 10.0f;

            vEnd.position = endPos + (radialDir * (radiusEnd * furrowNoise));
            vEnd.normal = radialDir;
            vEnd.tangent = vStart.tangent;
            vEnd.uvX = vStart.uvX;
            vEnd.uvY = 1.0f;
            vEnd.spurShootId = vStart.spurShootId;
            vEnd.structuralAge = vStart.structuralAge;

            outBuffer.vertices.push_back(vStart);
            outBuffer.vertices.push_back(vEnd);
        }

        // Triangulate indices
        for (int i = 0; i < radialFidelity; ++i) {
            uint32_t idx0 = rootIndex + (i * 2);
            uint32_t idx1 = idx0 + 1;
            uint32_t idx2 = idx0 + 2;
            uint32_t idx3 = idx0 + 3;

            outBuffer.indices.push_back(idx0);
            outBuffer.indices.push_back(idx1);
            outBuffer.indices.push_back(idx2);

            outBuffer.indices.push_back(idx1);
            outBuffer.indices.push_back(idx3);
            outBuffer.indices.push_back(idx2);
        }
    }
};

// --- GINKGO BILOBA ECOSYSTEM PIPELINE ---
class GinkgoEvolutionaryPipeline {
private:
    uint32_t idAllocationCounter = 0;
    std::mt19937 randomEngine;

public:
    GinkgoEvolutionaryPipeline(unsigned int seed) : randomEngine(seed) {}

    std::vector<std::unique_ptr<GinkgoBranchSegment>> GenerateGinkgoStructure(
        Vector3 baseOrigin, int depthLimit, float initialLen, float initialRad) 
    {
        std::vector<std::unique_ptr<GinkgoBranchSegment>> compiledSegments;

        struct NodeStackItem {
            Vector3 start;
            Vector3 direction;
            float len;
            float rad;
            int depth;
        };

        std::queue<NodeStackItem> processQueue;
        // Ginkgo reaches rigidly upwards towards the sub-canopy layer
        processQueue.push({baseOrigin, Vector3{0.0f, 1.0f, 0.0f}, initialLen, initialRad, 0});

        while (!processQueue.empty()) {
            NodeStackItem active = processQueue.front();
            processQueue.pop();

            Vector3 computedEnd = active.start + (active.direction * active.len);
            float taperRad = active.rad * 0.75f;

            auto segment = std::make_unique<GinkgoBranchSegment>(
                active.start, computedEnd, active.direction,
                active.rad, taperRad, active.depth, ++idAllocationCounter
            );

            // Populate localized leaf transform nodes at micro spur shoot clusters
            if (active.depth >= depthLimit - 3) {
                GenerateSpurClusterTransforms(segment->instancedFanLeafTransforms, computedEnd, 30);
            }

            compiledSegments.push_back(std::move(segment));

            if (active.depth < depthLimit) {
                // Ginkgo uses an erratic, bold, asymmetric branching pattern
                int splitBranches = (active.depth < 2) ? 2 : (std::uniform_real_distribution<float>(0, 1)(randomEngine) > 0.4f ? 3 : 2);
                
                for (int i = 0; i < splitBranches; ++i) {
                    float theta = static_cast<float>(i) - static_cast<float>(splitBranches - 1) / 2.0f;

                    Vector3 splitDir = active.direction;
                    splitDir.x += std::sin(theta * 0.38f) * 0.3f + std::uniform_real_distribution<float>(-0.05f, 0.05f)(randomEngine);
                    splitDir.z += std::cos(theta * 0.38f) * 0.3f + std::uniform_real_distribution<float>(-0.05f, 0.05f)(randomEngine);
                    splitDir.y += 0.5f; // Rigid high-angle vertical trajectory bias
                    splitDir.Normalize();

                    processQueue.push({
                        computedEnd, splitDir,
                        active.len * 0.76f, taperRad,
                        active.depth + 1
                    });
                }
            }
        }
        return compiledSegments;
    }

private:
    void GenerateSpurClusterTransforms(std::vector<Matrix4x4>& buffer, const Vector3& pivot, int leafCount) {
        std::uniform_real_distribution<float> distribution(-0.9f, 0.9f);
        for (int i = 0; i < leafCount; ++i) {
            Matrix4x4 m = Matrix4x4::Identity();
            // Assign spatial matrices surrounding the localized micro shoot
            m.m[3][0] = pivot.x + distribution(randomEngine);
            m.m[3][1] = pivot.y + distribution(randomEngine);
            m.m[3][2] = pivot.z + distribution(randomEngine);
            
            // Inject random rotation around Y-axis for organic orientation
            float angle = distribution(randomEngine) * 3.14159265f;
            m.m[0][0] = std::cos(angle);  m.m[0][2] = std::sin(angle);
            m.m[2][0] = -std::sin(angle); m.m[2][2] = std::cos(angle);

            buffer.push_back(m);
        }
    }
};

int main() {
    GinkgoEvolutionaryPipeline pipeline{42};
    MeshStreamBuffer outGeometry;

    std::cout << "[GINKGO SEED INITIALIZED]: Compiling Living Fossil Mesh Layers...\n";
    auto segments = pipeline.GenerateGinkgoStructure(Vector3{0.0f, 0.0f, 0.0f}, 7, 14.0f, 2.2f);

    for (const auto& seg : segments) {
        int radialSteps = (seg->depthLayer < 3) ? 20 : 10;
        seg->BuildSegmentTopology(outGeometry, radialSteps);
    }

    std::cout << "-> Compiled Vertices: " << outGeometry.vertices.size() << " Elements inside stream buffer.\n";
    std::cout << "-> Compiled Indices : " << outGeometry.indices.size() << " Array indices mappings.\n";
    return 0;
}
