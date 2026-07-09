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

struct DarkOakVertex {
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    float uvX, uvY;
    float fusedCoreChannel; // Vertex allocation mapping: Controls multi-trunk ribbed blending
    float roofFlexWeights;   // Dictates strict low-frequency drag response for flat canopies
};

struct DarkOakMeshBuffer {
    std::vector<DarkOakVertex> vertices;
    std::vector<uint32_t> indices;
};

// --- AAA DARK OAK STRUCTURAL SEGMENT ---
class DarkOakBranchSegment {
public:
    Vector3 startPoint;
    Vector3 endPoint;
    Vector3 growthDirection;
    float radiusStart;
    float radiusEnd;
    int depthLayer;
    uint32_t segmentUID;

    std::vector<Matrix4x4> flatRoofLeafTransforms;

    DarkOakBranchSegment(Vector3 s, Vector3 e, Vector3 g, float rs, float re, int depth, uint32_t id)
        : startPoint(s), endPoint(e), growthDirection(g), radiusStart(rs), radiusEnd(re), depthLayer(depth), segmentUID(id) {}

    // Generates a heavy, ribbed fused quad-core trunk and flat branch offsets
    void GenerateFusedTopology(DarkOakMeshBuffer& meshOut, int radialSegments) {
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

            // PROCEDURAL QUAD-CORE WELDING: Generate 4 heavy external pillar ribs to simulate fused trunks
            float quadRibNoise = std::cos(angle * 4.0f); // 4 clear corner protrusions
            float weldWeight = (depthLayer == 0) ? 1.0f : 0.0f;
            
            float finalRadiusScale = 1.0f + (quadRibNoise * 0.18f * weldWeight);

            DarkOakVertex vBase, vTip;
            vBase.position = startPoint + (radialDir * (radiusStart * finalRadiusScale));
            vBase.normal = radialDir;
            vBase.tangent = right * (-sinA) + up * cosA;
            vBase.uvX = static_cast<float>(i) / radialSegments;
            vBase.uvY = startPoint.y * 0.25f;
            vBase.fusedCoreChannel = weldWeight;
            vBase.roofFlexWeights = (depthLayer > 1) ? 1.0f : 0.0f;

            // Compute endpoint values
            float finalRadiusScaleTip = 1.0f + (quadRibNoise * 0.18f * weldWeight);
            vTip.position = endPoint + (radialDir * (radiusEnd * finalRadiusScaleTip));
            vTip.normal = radialDir;
            vTip.tangent = vBase.tangent;
            vTip.uvX = vBase.uvX;
            vTip.uvY = endPoint.y * 0.25f;
            vTip.fusedCoreChannel = weldWeight;
            vTip.roofFlexWeights = vBase.roofFlexWeights;

            meshOut.vertices.push_back(vBase);
            meshOut.vertices.push_back(vTip);
        }

        // Connect Primitive Indices Map
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

// --- CORE ULTRA-WIDE CANOPY PIPELINE MANAGER ---
class DarkOakPipelineManager {
private:
    uint32_t uidPool = 0;
    std::mt19937 fantasyRandomEngine;

public:
    DarkOakPipelineManager(unsigned int seed) : fantasyRandomEngine(seed) {}

    std::vector<std::unique_ptr<DarkOakBranchSegment>> CompileFlatOverlordGraph(
        Vector3 substratePos, int maxLevels, float shortHeight, float megaRadius) 
    {
        std::vector<std::unique_ptr<DarkOakBranchSegment>> treeSystemGraph;

        struct LSystemNode {
            Vector3 start;
            Vector3 direction;
            float length;
            float radius;
            int depth;
        };

        std::queue<LSystemNode> compilationQueue;
        // Low, heavy stocky foundational core trunk initialization
        compilationQueue.push({substratePos, Vector3{0.0f, 1.0f, 0.0f}, shortHeight, megaRadius, 0});

        while (!compilationQueue.empty()) {
            LSystemNode active = compilationQueue.front();
            compilationQueue.pop();

            Vector3 endPoint = active.start + (active.direction * active.length);
            
            // Minimal taper: Keep major limbs incredibly thick and blocky right up to the foliage boundary
            float taperFactor = (active.depth == 0) ? 0.85f : 0.72f;
            float coreTaperRadius = active.radius * taperFactor;

            auto segment = std::make_unique<DarkOakBranchSegment>(
                active.start, endPoint, active.direction,
                active.radius, coreTaperRadius, active.depth, ++uidPool
            );

            // Populate dense horizontal shelf-layered leaf grids at the top layer boundaries
            if (active.depth >= maxLevels - 1) {
                PopulateFlatRoofTransforms(segment->flatRoofLeafTransforms, endPoint, 42);
            }

            treeSystemGraph.push_back(std::move(segment));

            if (active.depth < maxLevels) {
                // Table-Top Spreading Rule: Erupt aggressively horizontally to form the roof
                int forks = (active.depth == 0) ? 4 : 2; 
                float flatRadialAngle = 2.0f * 3.14159265f / forks;

                for (int i = 0; i < forks; ++i) {
                    Vector3 nextDir = active.direction;
                    std::uniform_real_distribution<float> jitter(-0.1f, 0.1f);
                    
                    if (active.depth == 0) {
                        // Level 1 explosion shoots almost 75 degrees away from central vertical axis
                        float theta = (i * flatRadialAngle);
                        nextDir.x = std::cos(theta) * 0.85f + jitter(fantasyRandomEngine);
                        nextDir.z = std::sin(theta) * 0.85f + jitter(fantasyRandomEngine);
                        nextDir.y = 0.25f; // Force flat outward progression
                    } else {
                        // Secondary nodes flatten out completely (y close to 0 or negative) to lock the table surface
                        nextDir.x += jitter(fantasyRandomEngine) * 2.0f;
                        nextDir.z += jitter(fantasyRandomEngine) * 2.0f;
                        nextDir.y = -0.05f; // Slight sag typical of giant ancient heavy boughs
                    }
                    nextDir.Normalize();

                    compilationQueue.push({
                        endPoint, nextDir,
                        active.length * (active.depth == 0 ? 1.2f : 0.75f), // Horizontal limbs scale long early
                        coreTaperRadius, active.depth + 1
                    });
                }
            }
        }
        return treeSystemGraph;
    }

private:
    void PopulateFlatRoofTransforms(std::vector<Matrix4x4>& buffer, const Vector3& tipCoord, int matrixCount) {
        std::uniform_real_distribution<float> lateralSpread(-2.2f, 2.2f);
        for (int i = 0; i < matrixCount; ++i) {
            Matrix4x4 mat = Matrix4x4::Identity();
            
            // Build rigid, thin interlocking grid plates along the XZ plane to construct the flat ceiling appearance
            mat.m[3][0] = tipCoord.x + lateralSpread(fantasyRandomEngine);
            mat.m[3][1] = tipCoord.y + lateralSpread(fantasyRandomEngine) * 0.18f; // Clamp height differences to near-zero
            mat.m[3][2] = tipCoord.z + lateralSpread(fantasyRandomEngine);

            float compassAngle = lateralSpread(fantasyRandomEngine) * 3.14159265f;
            mat.m[0][0] = std::cos(compassAngle); mat.m[0][2] = std::sin(compassAngle);

            buffer.push_back(mat);
        }
    }
};

int main() {
    DarkOakPipelineManager engine{666};
    DarkOakMeshBuffer finalAssetMesh;

    std::cout << "[AAA PRODUCTION ENGINE]: Invoking Fused Quad-Core Dark Oak Compiler...\n";
    auto designGraph = engine.CompileFlatOverlordGraph(Vector3{0.0f, 0.0f, 0.0f}, 5, 7.0f, 3.8f); // High radius, short base height

    for (const auto& segment : designGraph) {
        int steps = (segment->depthLayer == 0) ? 32 : 12; // Massive precision on fused core trunk sections
        segment->GenerateFusedTopology(finalAssetMesh, steps);
    }

    std::cout << "-> Fantasy Table-Top Roof Primitive Buffer Successfully Allocated!\n";
    std::cout << "-> Allocated Vertex Node Elements: " << finalAssetMesh.vertices.size() << " Elements Matrix.\n";
    std::cout << "-> Allocated Index Face Elements   : " << finalAssetMesh.indices.size() << " Indices Stream Vector.\n";
    return 0;
}
