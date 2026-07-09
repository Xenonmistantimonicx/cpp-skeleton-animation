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

struct BristleconeVertex {
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    float uvX, uvY;
    float liveStripWeight;  // Vertex attribute: 1.0 = Zinda Strip Bark, 0.0 = Dead Polished Wood
    float windFlexibility;  // Outer gnarled branches flex less due to high wood density
};

struct BristleconeMeshBuffer {
    std::vector<BristleconeVertex> vertices;
    std::vector<uint32_t> indices;
};

// --- AAA BRISTLECONE MODEL STRUCTURAL NODE ---
class BristleconeBranchSegment {
public:
    Vector3 startPoint;
    Vector3 endPoint;
    Vector3 growthDirection;
    float radiusStart;
    float radiusEnd;
    int currentDepth;
    uint32_t segmentID;

    std::vector<Matrix4x4> bottleBrushNeedleTransforms;

    BristleconeBranchSegment(Vector3 s, Vector3 e, Vector3 g, float rs, float re, int depth, uint32_t id)
        : startPoint(s), endPoint(e), growthDirection(g), radiusStart(rs), radiusEnd(re), currentDepth(depth), segmentID(id) {}

    // Generates highly twisted, spiral gnarled trunk topology with structural strip-bark channel offsets
    void GenerateGnarledTopology(BristleconeMeshBuffer& meshOut, int radialSegments) {
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
            
            // THE GNARLED TWIST: Apply a progressive helix rotation angle along the segment height
            float spiralOffset = (startPoint.y * 0.15f);
            float cosA = std::cos(angle + spiralOffset);
            float sinA = std::sin(angle + spiralOffset);

            Vector3 radialDir = (right * cosA) + (up * sinA);
            radialDir.Normalize();

            // High erosion ridges mimicking centuries of sandblasting alpine winds
            float windErosionSculpt = 1.0f + 0.18f * std::sin(angle * 3.0f + spiralOffset);

            // STRIP BARK MAPPING: Only a localized angular quadrant remains alive (1.0 weight)
            float liveStrip = (angle > 1.0f && angle < 2.5f) ? 1.0f : 0.0f;

            BristleconeVertex vBase, vTip;
            vBase.position = startPoint + (radialDir * (radiusStart * windErosionSculpt));
            vBase.normal = radialDir;
            vBase.tangent = right * (-sinA) + up * cosA;
            vBase.uvX = static_cast<float>(i) / radialSegments;
            vBase.uvY = 0.0f;
            vBase.liveStripWeight = liveStrip;
            vBase.windFlexibility = static_cast<float>(currentDepth) * 0.12f;

            vTip.position = endPoint + (radialDir * (radiusEnd * windErosionSculpt));
            vTip.normal = radialDir;
            vTip.tangent = vBase.tangent;
            vTip.uvX = vBase.uvX;
            vTip.uvY = 1.0f;
            vTip.liveStripWeight = liveStrip;
            vTip.windFlexibility = vBase.windFlexibility;

            meshOut.vertices.push_back(vBase);
            meshOut.vertices.push_back(vTip);
        }

        // Connect Index Streams
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

// --- CORE CHRONOLOGICAL ANCIENT PIPELINE MANAGER ---
class BristleconePinePipeline {
private:
    uint32_t internalIDGenerator = 0;
    std::mt19937 weatherRandomEngine;

public:
    BristleconePinePipeline(unsigned int seed) : weatherRandomEngine(seed) {}

    std::vector<std::unique_ptr<BristleconeBranchSegment>> CompileAncientTreeGraph(
        Vector3 substrateOrigin, int recursionLimit, float initialLen, float initialRad) 
    {
        std::vector<std::unique_ptr<BristleconeBranchSegment>> pineSystemGraph;

        struct GrowthFrame {
            Vector3 start;
            Vector3 direction;
            float length;
            float radius;
            int depth;
        };

        std::queue<GrowthFrame> processQueue;
        // Asymmetric, stunted, wind-battered base trajectory setup
        processQueue.push({substrateOrigin, Vector3{0.1f, 0.9f, 0.0f}, initialLen, initialRad, 0});

        while (!processQueue.empty()) {
            GrowthFrame active = processQueue.front();
            processQueue.pop();

            Vector3 endPoint = active.start + (active.direction * active.length);
            float coreTaperRadius = active.radius * 0.78f;

            auto segment = std::make_unique<BristleconeBranchSegment>(
                active.start, endPoint, active.direction,
                active.radius, coreTaperRadius, active.depth, ++internalIDGenerator
            );

            // BOTTLE-BRUSH INSTANCING: Only generate high density needle streams if attached to living segments
            if (active.depth >= recursionLimit - 3 && (internalIDGenerator % 3 == 0)) {
                PopulateBottleBrushTransforms(segment->bottleBrushNeedleTransforms, endPoint, 60);
            }

            pineSystemGraph.push_back(std::move(segment));

            if (active.depth < recursionLimit) {
                // Stunted Asymmetric Splitting Rule: Weather forced jagged branch directions
                int offshootCount = std::uniform_real_distribution<float>(0, 1)(weatherRandomEngine) > 0.5f ? 2 : 1;

                for (int i = 0; i < offshootCount; ++i) {
                    Vector3 jaggedDir = active.direction;
                    
                    std::uniform_real_distribution<float> windDeform(-0.5f, 0.5f);
                    jaggedDir.x += windDeform(weatherRandomEngine);
                    jaggedDir.z += windDeform(weatherRandomEngine);
                    jaggedDir.y += std::uniform_real_distribution<float>(-0.1f, 0.3f)(weatherRandomEngine); // Stunted twist
                    jaggedDir.Normalize();

                    processQueue.push({
                        endPoint, jaggedDir,
                        active.length * (0.72f + windDeform(weatherRandomEngine) * 0.1f), coreTaperRadius,
                        active.depth + 1
                    });
                }
            }
        }
        return pineSystemGraph;
    }

private:
    void PopulateBottleBrushTransforms(std::vector<Matrix4x4>& container, const Vector3& branchTip, int density) {
        std::uniform_real_distribution<float> radiusDist(0.0f, 3.14159265f * 2.0f);
        for (int i = 0; i < density; ++i) {
            Matrix4x4 mat = Matrix4x4::Identity();
            
            // Bottle-Brush configuration packing needles radially tightly around the branch axis core
            float theta = radiusDist(weatherRandomEngine);
            float spiralSpread = (static_cast<float>(i) / density) * 2.5f;

            mat.m[3][0] = branchTip.x + std::cos(theta) * 0.4f;
            mat.m[3][1] = branchTip.y + spiralSpread - 1.2f;
            mat.m[3][2] = branchTip.z + std::sin(theta) * 0.4f;

            container.push_back(mat);
        }
    }
};

int main() {
    BristleconePinePipeline pipeline{555};
    BristleconeMeshBuffer outAssetMesh;

    std::cout << "[AAA ENGINE PIPELINE]: Allocating Ancient Bristlecone Topology Matrix...\n";
    auto networkGraph = pipeline.CompileAncientTreeGraph(Vector3{0.0f, 0.0f, 0.0f}, 6, 10.0f, 2.8f);

    for (const auto& segment : networkGraph) {
        segment->GenerateGnarledTopology(outAssetMesh, 16);
    }

    std::cout << "-> Successfully Rendered Dynamic Multi-Century Geometric Streams!\n";
    std::cout << "-> Buffer Vertices Stream: " << outAssetMesh.vertices.size() << " Unique Node Points.\n";
    std::cout << "-> Buffer Indices Stream : " << outAssetMesh.indices.size() << " Mapping Face Indices.\n";
    return 0;
}
