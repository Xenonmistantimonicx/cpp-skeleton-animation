#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>
#include <algorithm>

const float PI = 3.14159265359f;

struct Vector3 {
    float x, y, z;
    Vector3 operator+(const Vector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vector3 operator-(const Vector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vector3 Cross(const Vector3& v) const { return { y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x }; }
    void Normalize() { float len = std::sqrt(x*x + y*y + z*z); if(len > 0.0f) { x /= len; y /= len; z /= len; } }
};

struct CrassifoliaVertex {
    Vector3 pos;
    Vector3 normal;
    float u, v;
    float waxyCrustDensity; // 1.0 = High salinity white crust overlay, 0.0 = Raw grey undertone
    float elementClassification; // 0.0 = Swollen Caudex Timber, 1.0 = Thick Obovate Leaf
};

class FicusCrassifoliaCompiler {
private:
    std::vector<CrassifoliaVertex> m_Vertices;
    std::vector<uint32_t>          m_Indices;

    float AnalyticalNoise(float x, float z) {
        return std::fract(std::sin(x * 124.98f + z * 543.21f) * 43758.5453f);
    }

public:
    void CompileMightyCrassifolia() {
        std::cout << "[AAA ECO-ENGINE]: Compiling Ficus microcarpa var. crassifolia Swollen Architecture...\n";

        int heightSteps = 35;
        float heightInterval = 0.25f; // Short, stout, thick trunk growth increments
        float standardRadius = 0.5f;

        // Step 1: Compute Swollen Bulbous Caudex & Distorted Pillars
        for (int h = 0; h <= heightSteps; ++h) {
            float hFactor = (float)h / heightSteps;
            Vector3 tierCenter = {0.0f, h * heightInterval, 0.0f};

            uint32_t ringStartIdx = static_cast<uint32_t>(m_Vertices.size());
            int ringResolution = 50;

            for (int r = 0; r <= ringResolution; ++r) {
                float angle = 2.0f * PI * (float)r / ringResolution;

                // CAUDEX SWISS-KNIFE MATHEMATICAL DISTORTION
                // Radially inflates the trunk near ground level (hFactor -> 0) to simulate swollen root bases
                float caudexSwell = 1.0f;
                if (hFactor < 0.35f) {
                    float swellGradient = std::pow(1.0f - (hFactor / 0.35f), 3.0f);
                    // Add asymmetric lumpy growths around the base circle
                    float lumpFormation = 1.5f + std::sin(angle * 4.0f) * 0.4f + std::cos(angle * 7.0f) * 0.15f;
                    caudexSwell += lumpFormation * 3.2f * swellGradient;
                }

                // Append minor high-frequency ridge textures representing salt-crust weathering
                float microScabs = AnalyticalNoise(std::cos(angle), hFactor * 12.0f);
                float crustPresence = (microScabs > 0.4f) ? saturate(1.2f - hFactor) : 0.0f;

                float activeRadius = standardRadius * (1.0f - hFactor * 0.4f) * caudexSwell;
                Vector3 directionalVector = {std::cos(angle), 0.0f, std::sin(angle)};
                Vector3 vertexPosition = tierCenter + directionalVector * activeRadius;

                CrassifoliaVertex v;
                v.pos = vertexPosition;
                v.normal = directionalVector; if (hFactor < 0.2f) v.normal.y -= 0.3f; v.normal.Normalize();
                v.u = (float)r / ringResolution;
                v.v = hFactor * 6.0f;
                v.waxyCrustDensity = crustPresence;
                v.elementClassification = 0.0f; // Structural Caudex Timber

                m_Vertices.push_back(v);
            }

            // Topology Grid Stitching
            if (h < heightSteps) {
                for (int r = 0; r < ringResolution; ++r) {
                    uint32_t rowCurr = ringStartIdx;
                    uint32_t rowNext = ringStartIdx + (ringResolution + 1);

                    m_Indices.push_back(rowCurr + r);
                    m_Indices.push_back(rowNext + r);
                    m_Indices.push_back(rowCurr + r + 1);

                    m_Indices.push_back(rowCurr + r + 1);
                    m_Indices.push_back(rowNext + r);
                    m_Indices.push_back(rowNext + r + 1);
                }
            }

            // Canopy Spawning Pass - Dense clusters at top tiers
            if (h > (heightSteps - 4)) {
                SpawnThickObovateCanopy(tierCenter, standardRadius * 2.5f);
            }
        }
    }

private:
    void SpawnThickObovateCanopy(Vector3 position, float radius) {
        int foliageClustersCount = 8;
        uint32_t leafStartOffset = static_cast<uint32_t>(m_Vertices.size());

        for (int c = 0; c < foliageClustersCount; ++c) {
            float localAngle = (float)c * (2.0f * PI / foliageClustersCount);
            Vector3 centerOffset = {std::cos(localAngle) * radius, (float)c * 0.1f, std::sin(localAngle) * radius};
            Vector3 absoluteClusterCenter = position + centerOffset;

            // Generate dense opposite leaf card geometries mirroring the compact structure of var. crassifolia
            CrassifoliaVertex l0, l1, l2, l3;
            float lSize = 0.35f;

            l0.pos = absoluteClusterCenter + Vector3{-lSize, 0.0f, -lSize}; l0.normal = {0.0f, 1.0f, 0.0f}; l0.u = 0.0f; l0.v = 0.0f; l0.waxyCrustDensity = 0.0f; l0.elementClassification = 1.0f;
            l1.pos = absoluteClusterCenter + Vector3{ lSize, 0.0f, -lSize}; l1.normal = {0.0f, 1.0f, 0.0f}; l1.u = 1.0f; l1.v = 0.0f; l1.waxyCrustDensity = 0.0f; l1.elementClassification = 1.0f;
            l2.pos = absoluteClusterCenter + Vector3{-lSize, 0.0f,  lSize}; l2.normal = {0.0f, 1.0f, 0.0f}; l2.u = 0.0f; l2.v = 1.0f; l2.waxyCrustDensity = 0.0f; l2.elementClassification = 1.0f;
            l3.pos = absoluteClusterCenter + Vector3{ lSize, 0.0f,  lSize}; l3.normal = {0.0f, 1.0f, 0.0f}; l3.u = 1.0f; l3.v = 1.0f; l3.waxyCrustDensity = 0.0f; l3.elementClassification = 1.0f;

            m_Vertices.push_back(l0); m_Vertices.push_back(l1); m_Vertices.push_back(l2); m_Vertices.push_back(l3);

            uint32_t base = leafStartOffset + (c * 4);
            m_Indices.push_back(base); m_Indices.push_back(base + 1); m_Indices.push_back(base + 2);
            m_Indices.push_back(base + 2); m_Indices.push_back(base + 1); m_Indices.push_back(base + 3);
        }
    }

    float saturate(float val) { return std::max(0.0f, std::min(1.0f, val)); }

public:
    void ExportAssetOBJ(const std::string& targetPath) {
        std::ofstream file(targetPath);
        if (!file.is_open()) return;

        file << "# AAA Production Asset Registry Database: Ficus microcarpa var. crassifolia (Wax Fig)\n";
        for (const auto& v : m_Vertices) file << "v " << v.pos.x << " " << v.pos.y << " " << v.pos.z << "\n";
        for (const auto& v : m_Vertices) file << "vt " << v.u << " " << v.v << "\n";
        for (const auto& v : m_Vertices) file << "vn " << v.normal.x << " " << v.normal.y << " " << v.elementClassification << "\n";

        file << "\ng Ficus_Crassifolia_Master_Mesh\nusemtl M_Ficus_Crassifolia_PBR\n";
        for (size_t i = 0; i < m_Indices.size(); i += 3) {
            file << "f " << m_Indices[i]+1 << "/" << m_Indices[i]+1 << "/" << m_Indices[i]+1 << " "
                 << m_Indices[i+1]+1 << "/" << m_Indices[i+1]+1 << "/" << m_Indices[i+1]+1 << " "
                 << m_Indices[i+2]+1 << "/" << m_Indices[i+2]+1 << "/" << m_Indices[i+2]+1 << "\n";
        }
        file.close();
        std::cout << "[SUCCESSFUL BUILD]: Packed mesh stream pipeline generated successfully at path: " << targetPath << "\n";
    }
};

int main() {
    FicusCrassifoliaCompiler compiler;
    compiler.CompileMightyCrassifolia();
    compiler.ExportAssetOBJ("Ficus_Microcarpa_Crassifolia.obj");
    return 0;
}
