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

struct BaobabVertex {
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    float uvX, uvY;
    float expansionChannel; // Vertex color channel: Maps real-time succulent trunk water inflation
    float branchMassWeight; // Controls heavy dampening (Massive trunks don't sway easily)
};

struct BaobabMeshBuffer {
    std::vector<BaobabVertex> vertices;
    std::vector<uint32_t> indices;
};

// --- AAA BAOBAB STRUCTURAL SEGMENT ---
class BaobabBranchSegment {
public:
    Vector3 startPoint;
    Vector3 endPoint;
    Vector3 growthDirection;
    float radiusStart;
    float radiusEnd;
    int depthLayer;
    uint32_t segmentUID;

    std::vector<Matrix4x4> instancedFingerLeafTransforms;

    BaobabBranchSegment(Vector3 s, Vector3 e, Vector3 g, float rs, float re, int depth, uint32_t id)
        : startPoint(s), endPoint(e), growthDirection(g), radiusStart(rs), radiusEnd(re), depthLayer(depth), segmentUID(id) {}

    // Generates massive, smooth yet elephants-skin wrinkled trunk profiles
    void GenerateSucculentTopology(BaobabMeshBuffer& meshOut, int radialSegments) {
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

            // PROCEDURAL CAUDEX PROFILE: Baobab trunks bulge heavily near the lower-middle section
            float heightNormalized = startPoint.y / 25.0f; // Baseline tree scale reference
            float bottleBulge = 1.0f;
            if (depthLayer == 0) {
                // Mathematical bell curve to form the massive succulent water drum look
                bottleBulge = 1.0f + 0.65f * std::exp(-std::pow((heightNormalized - 0.35f) / 0.25f, 2.0f));
            }

            // Smooth elephant-skin horizontal wrinkles typical of Adansonia bark
            float wrinkleNoise = 1.0f + 0.015f * std::sin(startPoint.y * 3.5f) * std::cos(angle * 2.0f);
            float finalRadiusScale = bottleBulge * wrinkleNoise;

            BaobabVertex vBase, vTip;
            vBase.position = startPoint + (radialDir * (radiusStart * finalRadiusScale));
            vBase.normal = radialDir;
            vBase.tangent = right * (-sinA) + up * cosA;
            vBase.uvX = static_cast<float>(i) / radialSegments;
            vBase.uvY = 0.0f;
            vBase.expansionChannel = (depthLayer == 0) ? 1.0f : 0.0f; // Only trunk responds to hydration scaling
            vBase.branchMassWeight = (depthLayer < 2) ? 1.0f : 0.1f;    // Base trunk has massive inertia weight

            // Recalculate bulge parameters for the top endpoint of segment
            float heightNormalizedTip = endPoint.y / 25.0f;
            float bottleBulgeTip = 1.0f;
            if (depthLayer == 0) {
                bottleBulgeTip = 1.0f + 0.65f * std::exp(-std::pow((heightNormalizedTip - 0.35f) / 0.25f, 2.0f));
            }
            float finalRadiusScaleTip = bottleBulgeTip * wrinkleNoise;

            vTip.position = endPoint + (radialDir * (radiusEnd * finalRadiusScaleTip));
            vTip.normal = radialDir;
            vTip.tangent = vBase.tangent;
            vTip.uvX = vBase.uvX;
            vTip.uvY = 1.0f;
            vTip.expansionChannel = vBase.expansionChannel;
            vTip.branchMassWeight = vBase.branchMassWeight;

            meshOut.vertices.push_back(vBase);
            meshOut.vertices.push_back(vTip);
        }

        // Triangulate Indices
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

// --- CORE BAOBAB BOTTLE-TRUNK PIPELINE MANAGER ---
class BaobabPipelineManager {
private:
    uint32_t uidCounter = 0;
    std::mt19937 savannaRandomEngine;

public:
    BaobabPipelineManager(unsigned int seed) : savannaRandomEngine(seed) {}

    std::vector<std::unique_ptr<BaobabBranchSegment>> CompileVesselTreeGraph(
        Vector3 groundOrigin, int maxDepth, float trunkLength, float massiveRadius) 
    {
        std::vector<std::unique_ptr<BaobabBranchSegment>> treeGraph;

        struct LSystemFrame {
            Vector3 start;
            Vector3 heading;
            float length;
            float radius;
            int depth;
        };

        std::queue<LSystemFrame> processingQueue;
        // The core cylindrical base trunk starts ultra-thick and stocky
        processingQueue.push({groundOrigin, Vector3{0.0f, 1.0f, 0.0f}, trunkLength, massiveRadius, 0});

        while (!processingQueue.empty()) {
            LSystemFrame activeFrame = processingQueue.front();
            processingQueue.pop();

            Vector3 endpoint = activeFrame.start + (activeFrame.heading * activeFrame.length);
            
            // Dramatic taper shift: Branches suddenly become very thin compared to the giant trunk
            float taperFactor = (activeFrame.depth == 0) ? 0.32f : 0.74f;
            float coreTaperRadius = activeFrame.radius * taperFactor;

            auto segment = std::make_unique<BaobabBranchSegment>(
                activeFrame.start, endpoint, activeFrame.heading,
                activeFrame.radius, coreTaperRadius, activeFrame.depth, ++uidCounter
            );

            // Populate hand-shaped (digitale) leaf cluster instances only at the terminal tips
            if (activeFrame.depth == maxDepth) {
                PopulateDigitaleLeafTransforms(segment->instancedFingerLeafTransforms, endpoint, 16);
            }

            treeGraph.push_back(std::move(segment));

            if (activeFrame.depth < maxDepth) {
                // Upside-Down Crown Rule: Core trunk suddenly erupts into root-like gnarled horizontal branches
                int forks = (activeFrame.depth == 0) ? 4 : 2; 
                float spreadAngle = (activeFrame.depth == 0) ? 0.85f : 0.45f; // Sharp sudden outward angles

                for (int i = 0; i < forks; ++i) {
                    float orientationFactor = static_cast<float>(i) - static_cast<float>(forks - 1) / 2.0f;
                    
                    Vector3 branchHeading = activeFrame.heading;
                    std::uniform_real_distribution<float> jitter(-0.2f, 0.2f);
                    
                    branchHeading.x += std::sin(orientationFactor * spreadAngle) * 0.6f + jitter(savannaRandomEngine);
                    branchHeading.z += std::cos(orientationFactor * spreadAngle) * 0.6f + jitter(savannaRandomEngine);
                    // Force flattening on higher levels to look like stunted root webs
                    branchHeading.y += (activeFrame.depth == 0) ? 0.2f : -0.1f; 
                    branchHeading.Normalize();

                    processingQueue.push({
                        endpoint, branchHeading,
                        activeFrame.length * 0.68f, coreTaperRadius,
                        activeFrame.depth + 1
                    });
                }
            }
        }
        return treeGraph;
    }

private:
    void PopulateDigitaleLeafTransforms(std::vector<Matrix4x4>& container, const Vector3& tipPivot, int leafDensity) {
        std::uniform_real_distribution<float> posSpread(-0.9f, 0.9f);
        for (int i = 0; i < leafDensity; ++i) {
            Matrix4x4 mat = Matrix4x4::Identity();
            // Group leaves into compact global rosettes around terminal stubs
            mat.m[3][0] = tipPivot.x + posSpread(savannaRandomEngine);
            mat.m[3][1] = tipPivot.y + posSpread(savannaRandomEngine) * 0.4f;
            mat.m[3][2] = tipPivot.z + posSpread(savannaRandomEngine);

            float twistY = posSpread(savannaRandomEngine) * 3.14159265f;
            mat.m[0][0] = std::cos(twistY); mat.m[0][2] = std::sin(twistY);

            container.push_back(mat);
        }
    }
};

int main() {
    BaobabPipelineManager engine{888};
    BaobabMeshBuffer compiledMeshAsset;

    std::cout << "[PRODUCTION PIPELINE ACTIVATED]: Drawing Succulent Caudex Geometry...\n";
    auto networkGraph = engine.CompileVesselTreeGraph(Vector3{0.0f, 0.0f, 0.0f}, 5, 14.0f, 4.5f); // Base Radius set to 4.5 meters!

    for (const auto& chunk : networkGraph) {
        // Core trunk needs huge radial precision to look perfectly smooth and thick
        int resolution = (chunk->depthLayer == 0) ? 32 : 12;
        chunk->GenerateSucculentTopology(compiledMeshAsset, resolution);
    }

    std::cout << "-> Successfully Rendered Heavy Core Asset Stream Mappings!\n";
    std::cout << "-> Compiled Mesh Vertices: " << compiledMeshAsset.vertices.size() << " Hardware Inputs.\n";
    std::cout << "-> Compiled Mesh Indices : " << compiledMeshAsset.indices.size() << " Topology Indices.\n";
    return 0;
}
