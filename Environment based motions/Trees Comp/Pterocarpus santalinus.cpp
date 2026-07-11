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
    void Normalize() { float len = std::sqrt(x*x + y*y + z*z); if(len > 0.0f) { x /= len; y /= len; z /= len; } }
};

struct SandersVertex {
    Vector3 pos;
    Vector3 normal;
    float u, v;
    float heartwoodRatio;      // 1.0 = Pure Red Heartwood Core, 0.0 = Light Outer Sapwood
    float crocodileCleftMask;  // Deep fracture line multiplier for procedural displacement
};

class PterocarpusSantalinusCompiler {
private:
    std::vector<SandersVertex> m_Vertices;
    std::vector<uint32_t>      m_Indices;

    // Mathematical Square Wave to create deep, sharp plateaus (Crocodile Bark Clefts)
    float ComputeCrocodileCleft(float uvCoord, float frequency) {
        float wave = std::sin(uvCoord * frequency * 2.0f * PI);
        // Sharpen the edges to make deep rectangular valleys instead of smooth waves
        return (wave > 0.7f) ? 1.0f : (wave < -0.7f) ? -1.0f : wave * 1.42f;
    }

public:
    void CompileSandalwoodAsset() {
        std::cout << "[AAA SANDALWOOD_ENGINE]: Fracture-Baking Crocodile Bark Plates & Core Heartwood Density...\n";

        int verticalSegments = 40;
        float segmentHeight = 0.35f;
        float baseRadius = 0.65f;

        for (int y = 0; y <= verticalSegments; ++y) {
            float vProgress = (float)y / verticalSegments;
            Vector3 ringCenter = {0.0f, y * segmentHeight, 0.0f};

            uint32_t ringStartIdx = static_cast<uint32_t>(m_Vertices.size());
            int radialResolution = 64; // High resolution required to hold sharp displacement cuts

            for (int r = 0; r <= radialResolution; ++r) {
                float uProgress = (float)r / radialResolution;
                float angle = uProgress * 2.0f * PI;

                // CROCODILE BARK MATHEMATICAL DISPLACEMENT
                // Intersecting horizontal and vertical high-frequency square cuts
                float horizontalClefts = ComputeCrocodileCleft(uProgress, 10.0f); // 10 vertical cracks around circumference
                float verticalClefts   = ComputeCrocodileCleft(vProgress, 14.0f); // 14 horizontal blocks along height
                
                // Combine maps to form rectangular plate patterns
                float combinedFracture = (horizontalClefts * verticalClefts);
                float displacementAmt = (combinedFracture < -0.2f) ? -0.06f : 0.03f; // Deep inset valleys, extruded plates

                // Mature tree tapers slightly, base has slight buttressing flares
                float adaptiveRadius = baseRadius * (1.0f - vProgress * 0.3f) + (vProgress < 0.15f ? (1.15f - vProgress) * 0.08f : 0.0f);
                float finalRadius = adaptiveRadius + displacementAmt;

                Vector3 direction = {std::cos(angle), 0.0f, std::sin(angle)};
                Vector3 displacedPos = ringCenter + direction * finalRadius;

                // HEARTWOOD DENSITY PROFILING
                // Inner 70% of the radius contains the high-value red santalin extract
                float normalizedRadiusLocation = finalRadius / baseRadius;
                float heartwoodMask = (normalizedRadiusLocation < 0.72f) ? 1.0f : saturate((0.85f - normalizedRadiusLocation) / 0.13f);

                SandersVertex v;
                v.pos = displacedPos;
                v.normal = direction; // Simplified base normal; fragment shader handles micro-cleft lighting
                v.u = uProgress;
                v.v = vProgress * 5.0f;
                v.heartwoodRatio = heartwoodMask;
                v.crocodileCleftMask = combinedFracture;

                m_Vertices.push_back(v);
            }

            // Stitch the loops together into a solid watertight geometric tube
            if (y < verticalSegments) {
                for (int r = 0; r < radialResolution; ++r) {
                    uint32_t currentRingIdx = ringStartIdx;
                    uint32_t nextRingIdx    = ringStartIdx + (radialResolution + 1);

                    m_Indices.push_back(currentRingIdx + r);
                    m_Indices.push_back(nextRingIdx + r);
                    m_Indices.push_back(currentRingIdx + r + 1);

                    m_Indices.push_back(currentRingIdx + r + 1);
                    m_Indices.push_back(nextRingIdx + r);
                    m_Indices.push_back(nextRingIdx + r + 1);
                }
            }
        }
    }

private:
    float saturate(float val) { return std::max(0.0f, std::min(1.0f, val)); }

public:
    void ExportAssetOBJ(const std::string& filename) {
        std::ofstream stream(filename);
        if (!stream.is_open()) return;

        stream << "# AAA Asset Pipeline Output: Pterocarpus santalinus (Red Sanders)\n";
        for (const auto& v : m_Vertices) stream << "v " << v.pos.x << " " << v.pos.y << " " << v.pos.z << "\n";
        for (const auto& v : m_Vertices) stream << "vt " << v.u << " " << v.v << "\n";
        
        // Pass the procedural metadata down inside normal tracking vectors for the shader pipeline
        for (const auto& v : m_Vertices) stream << "vn " << v.normal.x << " " << v.normal.y << " " << v.heartwoodRatio << "\n";

        stream << "\ng Pterocarpus_Santalinus_Trunk\nusemtl M_Red_Sanders_PBR\n";
        for (size_t i = 0; i < m_Indices.size(); i += 3) {
            stream << "f " << m_Indices[i]+1 << "/" << m_Indices[i]+1 << "/" << m_Indices[i]+1 << " "
                   << m_Indices[i+1]+1 << "/" << m_Indices[i+1]+1 << "/" << m_Indices[i+1]+1 << " "
                   << m_Indices[i+2]+1 << "/" << m_Indices[i+2]+1 << "/" << m_Indices[i+2]+1 << "\n";
        }
        stream.close();
        std::cout << "[SUCCESS]: Red Sanders mesh architecture compiled dynamically to: " << filename << "\n";
    }
};

int main() {
    PterocarpusSantalinusCompiler engine;
    engine.CompileSandalwoodAsset();
    engine.ExportAssetOBJ("Pterocarpus_Santalinus_Core.obj");
    return 0;
}
