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

struct BirchVertex {
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    float uvX, uvY;
    float lenticelScaringWeight; // Generates procedural dark slashes on chalky white bark
    float stemElasticityAlpha;  // High values on top parts for intense wind flexibility
};

struct BirchMeshBuffer {
    std::vector<BirchVertex> vertices;
    std::vector<uint32_t> indices;
};

// --- AAA BIRCH STRUCTURAL SEGMENT ---
class BirchBranchSegment {
public:
    Vector3 startPoint;
    Vector3 endPoint;
    Vector3 growthDirection;
    float radiusStart;
    float radiusEnd;
    int depthLayer;
    uint32_t segmentUID;

    std::vector<Matrix4x4> instancedSerratedLeafTransforms;

    BirchBranchSegment(Vector3 s, Vector3 e, Vector3 g, float rs, float re, int depth, uint32_t id)
        : startPoint(s), endPoint(e), growthDirection(g), radiusStart(rs), radiusEnd(re), depthLayer(depth), segmentUID(id) {}

    // Generates ultra-slender, smooth topology with rough charcoal base deforms
    void GenerateSlenderTopology(BirchMeshBuffer& meshOut, int radialSegments) {
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

            // PROCEDURAL LENTICELS & BASE SCARRING: Rough dark blocks at the base, smooth white trunk on top
            float baseAgeScaring = 0.0f;
            if (depthLayer == 0 && startPoint.y < 3.0f) {
                // Heavy rough blocky distortion near the root crown zone
                baseAgeScaring = 0.08f * std::signbit(std::sin(angle * 5.0f)) * (1.0f - (startPoint.y / 3.0f));
            }
            
            float finalRadiusScale = 1.0f + baseAgeScaring;

            BirchVertex vBase, vTip;
            vBase.position = startPoint + (radialDir * (radiusStart * finalRadiusScale));
            vBase.normal = radialDir;
            vBase.tangent = right * (-sinA) + up * cosA;
            vBase.uvX = static_cast<float>(i) / radialSegments;
            vBase.uvY = startPoint.y * 0.5f; // High vertical tiling density for lenticels
            vBase.lenticelScaringWeight = (startPoint.y < 3.0f && depthLayer == 0) ? 1.0f : 0.0f;
            vBase.stemElasticityAlpha = saturate(startPoint.y / 22.0f); // Elasticity grows linearly with height

            // Compute endpoint metrics
            float baseAgeScaringTip = (endPoint.y < 3.0f && depthLayer == 0) ? 0.08f * std::signbit(std::sin(angle * 5.0f)) * (1.0f - (endPoint.y / 3.0f)) : 0.0f;
            float finalRadiusScaleTip = 1.0f + baseAgeScaringTip;

            vTip.position = endPoint + (radialDir * (radiusEnd * finalRadiusScaleTip));
            vTip.normal = radialDir;
            vTip.tangent = vBase.tangent;
            vTip.uvX = vBase.uvX;
            vTip.uvY = endPoint.y * 0.5f;
            vTip.lenticelScaringWeight = vBase.lenticelScaringWeight;
            vTip.stemElasticityAlpha = saturate(endPoint.y / 22.0f);

            meshOut.vertices.push_back(vBase);
            meshOut.vertices.push_back(vTip);
        }

        // Map Primitives Connect
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

private:
    float saturate(float val) { return std::max(0.0f, std::min(1.0f, val)); }
};

// --- CORE GRACIFUL BIRCH COMPILER PIPELINE ---
class BirchPipelineManager {
private:
    uint32_t uidPool = 0;
    std::mt19937 randomEngine;

public:
    BirchPipelineManager(unsigned int seed) : randomEngine(seed) {}

    std::vector<std::unique_ptr<BirchBranchSegment>> CompileSlenderCanopyGraph(
        Vector3 basePos, int maxDepth, float tallLength, float narrowRadius) 
    {
        std::vector<std::unique_ptr<BirchBranchSegment>> treeGraph;

        struct FrameNode {
            Vector3 start;
            Vector3 direction;
            float length;
            float radius;
            int depth;
        };

        std::queue<FrameNode> processQueue;
        // Slender, whip-like elegant vertical main trunk initialization
        processQueue.push({basePos, Vector3{0.02f, 0.98f, 0.01f}, tallLength, narrowRadius, 0});

        while (!processQueue.empty()) {
            FrameNode active = processQueue.front();
            processQueue.pop();

            Vector3 endCoord = active.start + (active.direction * active.length);
            float taperRadius = active.radius * 0.78f;

            auto segment = std::make_unique<BirchBranchSegment>(
                active.start, endCoord, active.direction,
                active.radius, taperRadius, active.depth, ++uidPool
            );

            // Populate high-density instanced teardrop leaves at thin terminal ends
            if (active.depth >= maxDepth - 2) {
                PopulateAirySerratedTransforms(segment->instancedSerratedLeafTransforms, endCoord, 18);
            }

            treeGraph.push_back(std::move(segment));

            if (active.depth < maxDepth) {
                // Narrow upright crown rule: Small branching angles to keep profile thin and tall
                int forks = (active.depth == 0) ? 1 : 2; 
                float tightAngle = 0.28f; // Minimal outward spreading

                for (int i = 0; i < forks; ++i) {
                    float bias = (i == 0) ? -1.0f : 1.0f;
                    Vector3 nextDir = active.direction;
                    
                    std::uniform_real_distribution<float> jitter(-0.08f, 0.08f);
                    nextDir.x += (bias * tightAngle) + jitter(randomEngine);
                    nextDir.z += jitter(randomEngine);
                    nextDir.y += 0.3f; // High vertical priority
                    nextDir.Normalize();

                    processQueue.push({
                        endCoord, nextDir,
                        active.length * 0.72f, taperRadius,
                        active.depth + 1
                    });
                }
            }
        }
        return treeGraph;
    }

private:
    void PopulateAirySerratedTransforms(std::vector<Matrix4x4>& container, const Vector3& tip, int count) {
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (int i = 0; i < count; ++i) {
            Matrix4x4 mat = Matrix4x4::Identity();
            
            // Loose, airy hanging clouds layout to let god-rays pierce through cleanly
            mat.m[3][0] = tip.x + dist(randomEngine) * 0.8f;
            mat.m[3][1] = tip.y + dist(randomEngine) * 0.9f - 0.4f; // Light weeping tilt
            mat.m[3][2] = tip.z + dist(randomEngine) * 0.8f;

            float rotY = dist(randomEngine) * 3.14159265f;
            mat.m[0][0] = std::cos(rotY); mat.m[0][2] = std::sin(rotY);

            container.push_back(mat);
        }
    }
};

int main() {
    BirchPipelineManager compiler{777};
    BirchMeshBuffer compiledAssetMesh;

    std::cout << "[AAA PRODUCTION ENGINE]: Building Slender White Birch Architecture Graph...\n";
    // Generates an elegant, tall 22-meter slender birch asset
    auto layerSegments = compiler.CompileSlenderCanopyGraph(Vector3{0.0f, 0.0f, 0.0f}, 6, 9.0f, 0.65f); // Base trunk radius is only 65cm!

    for (const auto& seg : layerSegments) {
        int steps = (seg->depthLayer < 2) ? 16 : 8;
        seg->GenerateSlenderTopology(compiledAssetMesh, steps);
    }

    std::cout << "-> Slender Kinetic Vertex Cache Buffer Successfully Allocated!\n";
    std::cout << "-> Target Vertices Stream: " << compiledAssetMesh.vertices.size() << " Hardware Shader Nodes.\n";
    std::cout << "-> Target Indices Stream : " << compiledAssetMesh.indices.size() << " Faces Indice Data Array.\n";
    return 0;
}
