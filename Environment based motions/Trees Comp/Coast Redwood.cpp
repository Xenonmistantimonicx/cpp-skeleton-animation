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

struct RedwoodVertex {
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    float uvX, uvY;
    float fibrousChannel;   // Maps deep vertical ridges for red bark mapping
    float heightAttenuation;// Critical scalar for high-altitude linear wind sway scaling
};

struct RedwoodMeshBuffer {
    std::vector<RedwoodVertex> vertices;
    std::vector<uint32_t> indices;
};

// --- AAA REDWOOD STRUCTURAL SEGMENT ---
class RedwoodBranchSegment {
public:
    Vector3 startPoint;
    Vector3 endPoint;
    Vector3 growthDirection;
    float radiusStart;
    float radiusEnd;
    int depthLayer;
    uint32_t segmentUID;

    std::vector<Matrix4x4> instancedTieredFoliageTransforms;

    RedwoodBranchSegment(Vector3 s, Vector3 e, Vector3 g, float rs, float re, int depth, uint32_t id)
        : startPoint(s), endPoint(e), growthDirection(g), radiusStart(rs), radiusEnd(re), depthLayer(depth), segmentUID(id) {}

    // Generates deeply channeled vertical fibrous bark geometry
    void GenerateToweringTopology(RedwoodMeshBuffer& meshOut, int radialSegments) {
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

            // FIBROUS RED BARK CHANNELS: Deep vertical furrow peaks and troughs
            float verticalFurrow = std::sin(angle * 8.0f) * std::cos(startPoint.y * 0.2f);
            
            // Buttressing logic: Base trunk flares out dramatically near ground level (startPoint.y close to 0)
            float baseFlare = 1.0f;
            if (depthLayer == 0 && startPoint.y < 8.0f) {
                baseFlare = 1.0f + 0.45f * std::exp(-startPoint.y * 0.25f) * (1.0f + 0.3f * std::cos(angle * 5.0f));
            }
            
            float finalRadiusScale = baseFlare + (verticalFurrow * 0.04f);

            RedwoodVertex vBase, vTip;
            vBase.position = startPoint + (radialDir * (radiusStart * finalRadiusScale));
            vBase.normal = radialDir;
            vBase.tangent = right * (-sinA) + up * cosA;
            vBase.uvX = static_cast<float>(i) / radialSegments;
            vBase.uvY = startPoint.y * 0.1f; // Tiled vertically along trunk length
            vBase.fibrousChannel = verticalFurrow;
            vBase.heightAttenuation = startPoint.y / 110.0f; // Baseline target height parameter

            // Calculate tip vectors
            float verticalFurrowTip = std::sin(angle * 8.0f) * std::cos(endPoint.y * 0.2f);
            float baseFlareTip = 1.0f;
            if (depthLayer == 0 && endPoint.y < 8.0f) {
                baseFlareTip = 1.0f + 0.45f * std::exp(-endPoint.y * 0.25f) * (1.0f + 0.3f * std::cos(angle * 5.0f));
            }
            float finalRadiusScaleTip = baseFlareTip + (verticalFurrowTip * 0.04f);

            vTip.position = endPoint + (radialDir * (radiusEnd * finalRadiusScaleTip));
            vTip.normal = radialDir;
            vTip.tangent = vBase.tangent;
            vTip.uvX = vBase.uvX;
            vTip.uvY = endPoint.y * 0.1f;
            vTip.fibrousChannel = verticalFurrowTip;
            vTip.heightAttenuation = endPoint.y / 110.0f;

            meshOut.vertices.push_back(vBase);
            meshOut.vertices.push_back(vTip);
        }

        // Generate Indice topologies
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

// --- CORE REDWOOD HYPER-SCALE MANAGER PIPELINE ---
class CoastRedwoodPipelineManager {
private:
    uint32_t uidPool = 0;
    std::mt19937 redwoodRandomEngine;

public:
    CoastRedwoodPipelineManager(unsigned int seed) : redwoodRandomEngine(seed) {}

    std::vector<std::unique_ptr<RedwoodBranchSegment>> CompileSequoiaGraph(
        Vector3 landOrigin, int totalLevels, float trunkMaxHeight, float baselineRadius) 
    {
        std::vector<std::unique_ptr<RedwoodBranchSegment>> sequiaNetwork;

        // Stage 1: Build the massive vertical columnar spine (Trunk segments sequential stack)
        Vector3 activeCursor = landOrigin;
        float currentRadius = baselineRadius;
        float heightPerSegment = trunkMaxHeight / static_cast<float>(totalLevels);

        for (int i = 0; i < totalLevels; ++i) {
            Vector3 nextCursor = activeCursor + Vector3{0.0f, heightPerSegment, 0.0f};
            float nextRadius = currentRadius * 0.93f; // Gradual smooth monolithic column taper

            auto trunkSegment = std::make_unique<RedwoodBranchSegment>(
                activeCursor, nextCursor, Vector3{0.0f, 1.0f, 0.0f},
                currentRadius, nextRadius, 0, ++uidPool
            );

            // Perpendicular Tiered Branch Offshoots (Only generate above lower 30% self-pruning clear trunk area)
            if (i > totalLevels * 0.3f) {
                int branchCount = 4;
                float branchRadialStep = 2.0f * 3.14159265f / branchCount;
                
                for (int b = 0; b < branchCount; ++b) {
                    float theta = (b * branchRadialStep) + (i * 0.4f); // Spiral offset tracking
                    Vector3 horizontalHeading = Vector3{std::cos(theta), 0.1f, std::sin(theta)}; // Flat trays bias
                    horizontalHeading.Normalize();

                    Vector3 branchEnd = nextCursor + (horizontalHeading * (15.0f * (1.0f - (i / static_cast<float>(totalLevels)))) );
                    
                    auto sideBranch = std::make_unique<RedwoodBranchSegment>(
                        nextCursor, branchEnd, horizontalHeading,
                        nextRadius * 0.25f, nextRadius * 0.05f, 1, ++uidPool
                    );

                    // Populate terminal spray flat needles list matrix
                    PopulateFlatTierTransforms(sideBranch->instancedTieredFoliageTransforms, branchEnd, 20);
                    sequiaNetwork.push_back(std::move(sideBranch));
                }
            }

            sequiaNetwork.push_back(std::move(trunkSegment));
            activeCursor = nextCursor;
            currentRadius = nextRadius;
        }

        return sequiaNetwork;
    }

private:
    void PopulateFlatTierTransforms(std::vector<Matrix4x4>& container, const Vector3& targetTip, int count) {
        std::uniform_real_distribution<float> range(-1.2f, 1.2f);
        for (int i = 0; i < count; ++i) {
            Matrix4x4 transformMatrix = Matrix4x4::Identity();
            // Organize flat instanced sprays along the horizontal coordinate space planes
            transformMatrix.m[3][0] = targetTip.x + range(redwoodRandomEngine) * 2.0f;
            transformMatrix.m[3][1] = targetTip.y + range(redwoodRandomEngine) * 0.2f; // Low vertical profile variance
            transformMatrix.m[3][2] = targetTip.z + range(redwoodRandomEngine) * 2.0f;

            float angleSpin = range(redwoodRandomEngine) * 0.5f;
            transformMatrix.m[0][0] = std::cos(angleSpin); transformMatrix.m[0][2] = std::sin(angleSpin);

            container.push_back(transformMatrix);
        }
    }
};

int main() {
    CoastRedwoodPipelineManager compiler{444};
    RedwoodMeshBuffer finalMeshBuffer;

    std::cout << "[PRODUCTION PIPELINE ACTIVATED]: Drawing Hyper-Scale Sequoia Monolith Graph...\n";
    // Generates a massive 90-meter tall Coast Redwood asset structure!
    auto executionGraph = compiler.CompileSequoiaGraph(Vector3{0.0f, 0.0f, 0.0f}, 15, 90.0f, 4.2f);

    for (const auto& chunk : executionGraph) {
        int fidelity = (chunk->depthLayer == 0) ? 24 : 8; // Dense main trunk trunk loops, low branch memory usage
        chunk->GenerateToweringTopology(finalMeshBuffer, fidelity);
    }

    std::cout << "-> Monolithic Buffer Streams Successfully Formatted!\n";
    std::cout << "-> Vertex Stream Allocations: " << finalMeshBuffer.vertices.size() << " Active Points.\n";
    std::cout << "-> Index Stream Allocations : " << finalMeshBuffer.indices.size() << " Primitives Element Indexes Mapping.\n";
    return 0;
}
