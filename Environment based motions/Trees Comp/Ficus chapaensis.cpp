#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>
#include <stack>
#include <algorithm>

const float PI = 3.14159265359f;

struct Vector3 {
    float x, y, z;
    Vector3 operator+(const Vector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vector3 operator-(const Vector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vector3 Cross(const Vector3& v) const { return { y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x }; }
    float Dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
    void Normalize() { float len = std::sqrt(x*x + y*y + z*z); if(len > 0.0f) { x /= len; y /= len; z /= len; } }
};

struct ChapaVertex {
    Vector3 pos;
    Vector3 normal;
    float u, v;
    float caulifloryNodeMask; // 1.0 = Trigger cluster position for trunk-born figs, 0.0 = Standard bark
    float segmentClassification; // 0.0 = Structural Wood, 1.0 = Foliage, 2.0 = Cauliflory Fruit Sphere
};

class FicusChapaensisCompiler {
private:
    std::vector<ChapaVertex> m_Vertices;
    std::vector<uint32_t>    m_Indices;

    float AnalyticalNoise(float scaleX, float scaleY) {
        return std::fract(std::sin(scaleX * 12.9898f) * std::cos(scaleY * 78.233f) * 43758.5453f);
    }

public:
    void BuildFicusEcosystem() {
        std::cout << "[AAA CHAPAE_ENGINE]: Braiding Strangling Aerial Root Lattices and Injecting Cauliflory Nodes...\n";
        
        int heightSteps = 30;
        float heightInterval = 0.5f;
        float primaryTrunkRadius = 0.9f;

        // Step 1: Generate Fused Root Columnar Trunk
        for (int h = 0; h <= heightSteps; ++h) {
            float hProgress = (float)h / heightSteps;
            Vector3 centerPos = {0.0f, h * heightInterval, 0.0f};
            
            uint32_t currentRingStartIdx = static_cast<uint32_t>(m_Vertices.size());
            int angularResolution = 48; // High density perimeter for smooth roots fusing

            for (int r = 0; r <= angularResolution; ++r) {
                float angle = 2.0f * PI * (float)r / angularResolution;

                // ANASTOMOSIS MATHEMATICAL SIMULATION
                // Braids multiple minor aerial root columns that twist and fuse together as they hit the ground
                float rootBraidPattern = std::sin(angle * 6.0f + hProgress * 8.0f) * 0.15f;
                float secondaryBraid  = std::cos(angle * 12.0f - hProgress * 4.0f) * 0.05f;
                
                float dynamicRadius = primaryTrunkRadius * (1.0f - hProgress * 0.4f) + (rootBraidPattern + secondaryBraid) * (1.0f - hProgress * 0.5f);

                // CAULIFLORY INJECTION POINT SEARCH
                // Lower mature trunk sections selectively produce high-intensity fruit clusters
                float patchRandomness = AnalyticalNoise(std::cos(angle), hProgress * 15.0f);
                float fruitTrigger = (hProgress > 0.05f && hProgress < 0.35f && patchRandomness > 0.82f) ? 1.0f : 0.0f;

                Vector3 displacementVector = {std::cos(angle) * dynamicRadius, 0.0f, std::sin(angle) * dynamicRadius};

                ChapaVertex v;
                v.pos = centerPos + displacementVector;
                Vector3 normal = displacementVector; normal.Normalize();
                v.normal = normal;
                v.u = (float)r / angularResolution;
                v.v = hProgress * 8.0f;
                v.caulifloryNodeMask = fruitTrigger;
                v.segmentClassification = 0.0f; // Core structure timber

                m_Vertices.push_back(v);

                // Procedural Fruit generation if mask is active
                if (fruitTrigger > 0.5f && r % 4 == 0) {
                    GenerateWoodBornFigs(v.pos, normal);
                }
            }

            // Index stitching logic for procedural core columns
            if (h < heightSteps) {
                for (int r = 0; r < angularResolution; ++r) {
                    uint32_t rowA = currentRingStartIdx;
                    uint32_t rowB = currentRingStartIdx + (angularResolution + 1);

                    m_Indices.push_back(rowA + r);
                    m_Indices.push_back(rowB + r);
                    m_Indices.push_back(rowA + r + 1);

                    m_Indices.push_back(rowA + r + 1);
                    m_Indices.push_back(rowB + r);
                    m_Indices.push_back(rowB + r + 1);
                }
            }
        }
    }

private:
    // Generates localized cluster spheres mimicking real trunk-born cauliflory fig fruits
    void GenerateWoodBornFigs(Vector3 basePosition, Vector3 surfaceNormal) {
        uint32_t fruitStartOffset = static_cast<uint32_t>(m_Vertices.size());
        float fruitRadius = 0.08f;
        Vector3 clusterCenter = basePosition + surfaceNormal * (fruitRadius * 0.5f);

        // Low poly highly scalable fast mathematical proxy spheres for cluster performance
        int latSegments = 6;
        int lonSegments = 6;

        for (int i = 0; i <= latSegments; ++i) {
            float latAngle = PI * (float)i / latSegments;
            for (int j = 0; j <= lonSegments; ++j) {
                float lonAngle = 2.0f * PI * (float)j / lonSegments;

                Vector3 localPos = {
                    std::sin(latAngle) * std::cos(lonAngle) * fruitRadius,
                    std::cos(latAngle) * fruitRadius,
                    std::sin(latAngle) * std::sin(lonAngle) * fruitRadius
                };

                ChapaVertex fv;
                fv.pos = clusterCenter + localPos;
                fv.normal = localPos; fv.normal.Normalize();
                fv.u = (float)j / lonSegments;
                fv.v = (float)i / latSegments;
                fv.caulifloryNodeMask = 0.0f;
                fv.segmentClassification = 2.0f; // Identifier flag 2 = Cauliflory Fruit

                m_Vertices.push_back(fv);
            }
        }

        // Stitch low-overhead indices data matrices for clustered spheres
        for (int i = 0; i < latSegments; ++i) {
            for (int j = 0; j < lonSegments; ++j) {
                uint32_t r1 = fruitStartOffset + i * (lonSegments + 1);
                uint32_t r2 = fruitStartOffset + (i + 1) * (lonSegments + 1);

                m_Indices.push_back(r1 + j);
                m_Indices.push_back(r2 + j);
                m_Indices.push_back(r1 + j + 1);

                m_Indices.push_back(r1 + j + 1);
                m_Indices.push_back(r2 + j);
                m_Indices.push_back(r2 + j + 1);
            }
        }
    }

public:
    void ExportAssetOBJ(const std::string& path) {
        std::ofstream stream(path);
        if (!stream.is_open()) return;

        stream << "# Production High-Fidelity Asset Pipeline Output: Ficus chapaensis (Fused Strangling Fig)\n";
        for (const auto& v : m_Vertices) stream << "v " << v.pos.x << " " << v.pos.y << " " << v.pos.z << "\n";
        for (const auto& v : m_Vertices) stream << "vt " << v.u << " " << v.v << "\n";
        for (const auto& v : m_Vertices) stream << "vn " << v.normal.x << " " << v.normal.y << " " << v.segmentClassification << "\n";

        stream << "\ng Ficus_Chapaensis_MasterMesh\nusemtl M_Ficus_Chapa_PBR\n";
        for (size_t i = 0; i < m_Indices.size(); i += 3) {
            stream << "f " << m_Indices[i]+1 << "/" << m_Indices[i]+1 << "/" << m_Indices[i]+1 << " "
                   << m_Indices[i+1]+1 << "/" << m_Indices[i+1]+1 << "/" << m_Indices[i+1]+1 << " "
                   << m_Indices[i+2]+1 << "/" << m_Indices[i+2]+1 << "/" << m_Indices[i+2]+1 << "\n";
        }
        stream.close();
        std::cout << "[SUCCESS]: Exported unified Ficus mesh layout data containing embedded cauliflory tracks to: " << path << "\n";
    }
};

int main() {
    FicusChapaensisCompiler engine;
    engine.BuildFicusEcosystem();
    engine.ExportAssetOBJ("Ficus_Chapaensis_Production.obj");
    return 0;
}
