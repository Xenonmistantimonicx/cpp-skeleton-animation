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

struct OakVertex {
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    float uvX, uvY;
    float blockyPlatingWeight; // Maps deep fissures mimicking alligator skin bark
    float clumpPhaseOffset;     // Generates unique phase groups for dense foliage clusters
};

struct OakMeshBuffer {
    std::vector<OakVertex> vertices;
    std::vector<uint32_t> indices;
};

// --- AAA OAK STRUCTURAL SEGMENT ---
class OakBranchSegment {
public:
    Vector3 startPoint;
    Vector3 endPoint;
    Vector3 growthDirection;
    float radiusStart;
    float radiusEnd;
    int depthLayer;
    uint32_t segmentUID;

    std::vector<Matrix4x4> instancedLobedLeafTransforms;

    OakBranchSegment(Vector3 s, Vector3 e, Vector3 g, float rs, float re, int depth, uint32_t id)
        : startPoint(s), endPoint(e), growthDirection(g), radiusStart(rs), radiusEnd(re), depthLayer(depth), segmentUID(id) {}

    // Generates deeply fissured blocky bark topology
    void GenerateMuscularTopology(OakMeshBuffer& meshOut, int radialSegments) {
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

            // PROCEDURAL BLOCKY PLATING: Alligator bark profile simulation
            float blockyNoise = std::signbit(std::sin(angle * 6.0f)) ? 0.04f : -0.04f;
            float ridgeFissure = blockyNoise * std::cos(startPoint.y * 1.5f);
            float platingWeight = (depthLayer < 2) ? 1.0f : 0.0f; // Thick blocks only on trunk

            // Sinuosity Deform: Give branches a jagged, tortuous twist
            float tortuousTwist = 0.08f * std::sin(startPoint.y * 0.5f);
            Vector3 finalRadialDir = radialDir + (right * tortuousTwist);
            finalRadialDir.Normalize();

            float finalRadiusScale = 1.0f + (ridgeFissure * platingWeight);

            OakVertex vBase, vTip;
            vBase.position = startPoint + (finalRadialDir * (radiusStart * finalRadiusScale));
            vBase.normal = finalRadialDir;
            vBase.tangent = right * (-sinA) + up * cosA;
            vBase.uvX = static_cast<float>(i) / radialSegments;
            vBase.uvY = startPoint.y * 0.2f;
            vBase.blockyPlatingWeight = platingWeight;
            vBase.clumpPhaseOffset = static_cast<float>(segmentUID) * 0.35f;

            // Recalculate for segment end coordinate
            float ridgeFissureTip = blockyNoise * std::cos(endPoint.y * 1.5f);
            float finalRadiusScaleTip = 1.0f + (ridgeFissureTip * platingWeight);

            vTip.position = endPoint + (finalRadialDir * (radiusEnd * finalRadiusScaleTip));
            vTip.normal = finalRadialDir;
            vTip.tangent = vBase.tangent;
            vTip.uvX = vBase.uvX;
            vTip.uvY = endPoint.y * 0.2f;
            vTip.blockyPlatingWeight = platingWeight;
            vTip.clumpPhaseOffset = vBase.clumpPhaseOffset;

            meshOut.vertices.push_back(vBase);
            meshOut.vertices.push_back(vTip);
        }

        // Triangulate primitive elements
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

// --- CORE GNARLED OAK PIPELINE MANAGER ---
class OakPipelineManager {
private:
    uint32_t uidPool = 0;
    std::mt19937 randomEngine;

public:
    OakPipelineManager(unsigned int seed) : randomEngine(seed) {}

    std::vector<std::unique_ptr<OakBranchSegment>> CompileSpreadingSkeleton(
        Vector3 rootOrigin, int maxDepth, float trunkLen, float thickRadius) 
    {
        std::vector<std::unique_ptr<OakBranchSegment>> skeletonGraph;

        struct FrameNode {
            Vector3 start;
            Vector3 direction;
            float length;
            float radius;
            int depth;
        };

        std::queue<FrameNode> executionQueue;
        // Low, heavy main trunk foundation setup
        executionQueue.push({rootOrigin, Vector3{0.0f, 1.0f, 0.0f}, trunkLen, thickRadius, 0});

        while (!executionQueue.empty()) {
            FrameNode active = executionQueue.front();
            executionQueue.pop();

            Vector3 endpoint = active.start + (active.direction * active.length);
            float taperRadius = active.radius * 0.74f;

            auto segment = std::make_unique<OakBranchSegment>(
                active.start, endpoint, active.direction,
                active.radius, taperRadius, active.depth, ++uidPool
            );

            // Populate thick cluster rosettes at high recursive depth bounds
            if (active.depth >= maxDepth - 2) {
                PopulateClumpedLeafTransforms(segment->instancedLobedLeafTransforms, endpoint, 35);
            }

            skeletonGraph.push_back(std::move(segment));

            if (active.depth < maxDepth) {
                // Tortuous Wide Branching Rule: Oak branches shoot sideways heavily
                int forks = (active.depth == 0) ? 3 : 2; 
                float horizontalSpread = 0.65f; // Deep angular divergence

                for (int i = 0; i < forks; ++i) {
                    float bias = static_cast<float>(i) - static_cast<float>(forks - 1) / 2.0f;
                    Vector3 nextDir = active.direction;
                    
                    std::uniform_real_distribution<float> tortuousJitter(-0.25f, 0.25f);
                    nextDir.x += std::sin(bias * horizontalSpread) * 0.55f + tortuousJitter(randomEngine);
                    nextDir.z += std::cos(bias * horizontalSpread) * 0.55f + tortuousJitter(randomEngine);
                    // Force flattening as depth grows to create a massive widespread floor span
                    nextDir.y += 0.1f - (static_cast<float>(active.depth) * 0.05f); 
                    nextDir.Normalize();

                    executionQueue.push({
                        endpoint, nextDir,
                        active.length * 0.75f, taperRadius,
                        active.depth + 1
                    });
                }
            }
        }
        return skeletonGraph;
    }

private:
    void PopulateClumpedLeafTransforms(std::vector<Matrix4x4>& container, const Vector3& branchTip, int density) {
        std::uniform_real_distribution<float> range(-1.5f, 1.5f);
        for (int i = 0; i < density; ++i) {
            Matrix4x4 mat = Matrix4x4::Identity();
            
            // Pack leaves tightly into spherical clumps (rosettes) to create heavy ambient shadows
            float theta = std::uniform_real_distribution<float>(0.0f, 6.28318f)(randomEngine);
            float phi = std::uniform_real_distribution<float>(0.0f, 3.14159f)(randomEngine);
            float r = std::uniform_real_distribution<float>(0.0f, 1.2f)(randomEngine);

            mat.m[3][0] = branchTip.x + r * std::sin(phi) * std::cos(theta);
            mat.m[3][1] = branchTip.y + r * std::sin(phi) * std::sin(theta) * 0.7f;
            mat.m[3][2] = branchTip.z + r * std::cos(phi);

            float spinY = range(randomEngine) * 3.14159265f;
            mat.m[0][0] = std::cos(spinY); mat.m[0][2] = std::sin(spinY);

            container.push_back(mat);
        }
    }
};

int main() {
    OakPipelineManager compiler{333};
    OakMeshBuffer assetMeshStream;

    std::cout << "[PRODUCTION PIPELINE ACTIVATED]: Drawing Gnarled Spreading Oak Graph...\n";
    auto totalSegments = compiler.CompileSpreadingSkeleton(Vector3{0.0f, 0.0f, 0.0f}, 6, 8.0f, 3.2f); // Low stout trunk base

    for (const auto& chunk : totalSegments) {
        int steps = (chunk->depthLayer < 2) ? 24 : 10;
        chunk->GenerateMuscularTopology(assetMeshStream, steps);
    }

    std::cout << "-> Muscular Skeletal Buffer Successfully Compiled!\n";
    std::cout << "-> Total Mesh Vertices: " << assetMeshStream.vertices.size() << " Hardware Vertex Elements.\n";
    std::cout << "-> Total Mesh Indices : " << assetMeshStream.indices.size() << " Index Buffer Elements.\n";
    return 0;
}
