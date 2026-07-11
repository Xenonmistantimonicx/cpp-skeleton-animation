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

struct FicusVertex {
    Vector3 pos;
    Vector3 normal;
    float u, v;
    float latexSeepageMask; // 1.0 = Area bleeding white latex sap, 0.0 = Dry bark
    float structuralType;  // 0.0 = Wood/Buttress, 1.0 = Oval Leaf Card
};

struct TrunkSection {
    Vector3 start;
    Vector3 end;
    float baseRadius;
    int tierIndex;
};

class FicusEcosystemEngine {
private:
    std::vector<FicusVertex> m_Vertices;
    std::vector<uint32_t>    m_Indices;

    float RadialHash(float angle) {
        return std::sin(angle * 14.233f) * std::cos(angle * 7.191f);
    }

public:
    void ConstructGiantFicus() {
        std::cout << "[AAA FICUS-ENGINE]: Extruding Columnar Trunk and Buttress Flanges...\n";
        
        // Build height profile layers for the column
        int heightTiers = 25;
        float segmentHeight = 0.8f;
        float baseRadius = 1.4f;

        for (int t = 0; t <= heightTiers; ++t) {
            float heightFactor = (float)t / heightTiers;
            Vector3 center = {0.0f, t * segmentHeight, 0.0f};
            
            // Core radius attenuation as we go up
            float currentRadius = baseRadius * std::max(0.3f, 1.0f - (heightFactor * 0.6f));
            uint32_t ringStartIdx = static_cast<uint32_t>(m_Vertices.size());

            int radialSegments = 40; // High fidelity circle layout
            for (int r = 0; r <= radialSegments; ++r) {
                float angle = 2.0f * PI * (float)r / radialSegments;

                // GIANT BUTTRESS ROOT MODELING MATH
                // At the base (heightFactor near 0), stretch the radius out along 4 specific cardinal angles
                float buttressMultiplier = 1.0f;
                if (heightFactor < 0.25f) {
                    float flangeStrength = std::pow(1.0f - (heightFactor / 0.25f), 2.5f);
                    // 4 massive support wings/flanges matching sharp sine peaks
                    float wingPattern = std::pow(std::abs(std::sin(angle * 2.0f)), 16.0f); 
                    buttressMultiplier += wingPattern * 4.5f * flangeStrength;
                }

                float finalRadius = currentRadius * buttressMultiplier;
                
                // Inject random bark scars that bleed latex
                float structuralScars = RadialHash(angle) * (1.0f - heightFactor);
                float latexMask = (structuralScars > 0.45f) ? 1.0f : 0.0f;

                Vector3 surfacePos = center + Vector3{std::cos(angle) * finalRadius, 0.0f, std::sin(angle) * finalRadius};

                FicusVertex v;
                v.pos = surfacePos;
                Vector3 n = {std::cos(angle), 0.1f, std::sin(angle)}; n.Normalize();
                v.normal = n;
                v.u = (float)r / radialSegments;
                v.v = heightFactor * 10.0f;
                v.latexSeepageMask = latexMask;
                v.structuralType = 0.0f;

                m_Vertices.push_back(v);
            }

            // Stitch indices for the trunk cylinder
            if (t < heightTiers) {
                for (int r = 0; r < radialSegments; ++r) {
                    uint32_t currRow = ringStartIdx;
                    uint32_t nextRow = ringStartIdx + (radialSegments + 1);

                    m_Indices.push_back(currRow + r);
                    m_Indices.push_back(nextRow + r);
                    m_Indices.push_back(currRow + r + 1);

                    m_Indices.push_back(currRow + r + 1);
                    m_Indices.push_back(nextRow + r);
                    m_Indices.push_back(nextRow + r + 1);
                }
            }

            // High Canopy Sprouting (Sprout leaf masses at the very top tier)
            if (t == heightTiers) {
                GenerateCanopyUmbrella(center, currentRadius);
            }
        }
    }

private:
    void GenerateCanopyUmbrella(Vector3 apexPoint, float radius) {
        int leafCount = 120;
        uint32_t leafBaseIdx = static_cast<uint32_t>(m_Vertices.size());

        for (int i = 0; i < leafCount; ++i) {
            float phi = (float)i * 0.15f;
            float spreadDist = radius + (float)i * 0.06f;
            
            Vector3 leafPos = apexPoint + Vector3{std::cos(phi) * spreadDist, std::sin((float)i) * 0.5f, std::sin(phi) * spreadDist};
            
            // Ficus albipila oval canopy leaf cards
            FicusVertex l0, l1, l2;
            l0.pos = leafPos; l0.normal = {0.0f, 1.0f, 0.0f}; l0.u = 0.5f; l0.v = 0.0f; l0.latexSeepageMask = 0.0f; l0.structuralType = 1.0f;
            l1.pos = leafPos + Vector3{0.4f, -0.2f, 0.4f}; l1.normal = {0.0f, 1.0f, 0.0f}; l1.u = 1.0f; l1.v = 1.0f; l1.latexSeepageMask = 0.0f; l1.structuralType = 1.0f;
            l2.pos = leafPos + Vector3{-0.4f, -0.2f, 0.4f}; l2.normal = {0.0f, 1.0f, 0.0f}; l2.u = 0.0f; l2.v = 1.0f; l2.latexSeepageMask = 0.0f; l2.structuralType = 1.0f;

            m_Vertices.push_back(l0); m_Vertices.push_back(l1); m_Vertices.push_back(l2);

            uint32_t idx = leafBaseIdx + (i * 3);
            m_Indices.push_back(idx); m_Indices.push_back(idx + 1); m_Indices.push_back(idx + 2);
        }
    }

public:
    void ExportAssetOBJ(const std::string& path) {
        std::ofstream file(path);
        if (!file.is_open()) return;

        file << "# Production Mesh Engine Asset File: Ficus albipila (Abbey Tree)\n";
        for (const auto& v : m_Vertices) file << "v " << v.pos.x << " " << v.pos.y << " " << v.pos.z << "\n";
        for (const auto& v : m_Vertices) file << "vt " << v.u << " " << v.v << "\n";
        for (const auto& v : m_Vertices) file << "vn " << v.normal.x << " " << v.normal.y << " " << v.latexSeepageMask << "\n";

        file << "\ng Ficus_Giant_Buttress_Mesh\nusemtl M_Ficus_Master_PBR\n";
        for (size_t i = 0; i < m_Indices.size(); i += 3) {
            file << "f " << m_Indices[i]+1 << "/" << m_Indices[i]+1 << "/" << m_Indices[i]+1 << " "
                 << m_Indices[i+1]+1 << "/" << m_Indices[i+1]+1 << "/" << m_Indices[i+1]+1 << " "
                 << m_Indices[i+2]+1 << "/" << m_Indices[i+2]+1 << "/" << m_Indices[i+2]+1 << "\n";
        }
        file.close();
        std::cout << "[EXPORT COMPLETE]: Ficus columnar geometry written to " << path << "\n";
    }
};

int main() {
    FicusEcosystemEngine engine;
    engine.ConstructGiantFicus();
    engine.ExportAssetOBJ("Ficus_Albipila_AAA.obj");
    return 0;
}
