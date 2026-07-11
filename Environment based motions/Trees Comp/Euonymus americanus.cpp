#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>

struct Vector3 {
    float x, y, z;
    Vector3 operator+(const Vector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vector3 operator-(const Vector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vector3 Cross(const Vector3& v) const { return { y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x }; }
    void Normalize() { float len = std::sqrt(x*x + y*y + z*z); if(len > 0.0f) { x /= len; y /= len; z /= len; } }
};

struct Vertex {
    Vector3 pos;
    Vector3 normal;
    float u, v;
    float wartyNoiseFactor; // Controls real-time displacement pass inside graphics memory
};

class EuonymusAssetEngine {
private:
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;
    const float PI = 3.14159265359f;

public:
    // 1. GENERATE DISTINCT SQUARE CROSS-SECTION STEM
    void BuildSquareStem(Vector3 start, Vector3 end, float width) {
        uint32_t startIdx = static_cast<uint32_t>(m_Vertices.size());
        int segments = 20;

        Vector3 direction = end - start;
        Vector3 up = {0.0f, 1.0f, 0.0f);
        if (std::abs(direction.Dot({0.0f, 1.0f, 0.0f})) > 0.9f) up = {1.0f, 0.0f, 0.0f};
        Vector3 right = direction.Cross(up); right.Normalize();
        up = right.Cross(direction); up.Normalize();

        // Construct 4 sharp corners along the extrusion path vector
        // Squaring function interpolation instead of standard polar circular layouts
        for (int i = 0; i <= segments; ++i) {
            float t = (float)i / segments;
            Vector3 currentCenter = start + direction * t;

            // 4 exact corner shifts mapping directly to square geometry nodes
            Vector3 offsets[4] = {
                (right * 1.0f + up * 1.0f) * width,
                (right * -1.0f + up * 1.0f) * width,
                (right * -1.0f + up * -1.0f) * width,
                (right * 1.0f + up * -1.0f) * width
            };

            for (int c = 0; c < 4; ++c) {
                Vertex v;
                v.pos = currentCenter + offsets[c];
                
                // Sharp Normal vectors facing perfectly outward from core square planes
                Vector3 n = offsets[c]; n.Normalize();
                v.normal = n;
                v.u = (float)c / 4.0f;
                v.v = t;
                v.wartyNoiseFactor = 0.0f; // Stems are completely sleek and square
                m_Vertices.push_back(v);
            }
        }

        // Stitch index matrix for sharp angular edges
        for (int i = 0; i < segments; ++i) {
            for (int c = 0; c < 4; ++c) {
                int nextC = (c + 1) % 4;
                uint32_t currRow = startIdx + i * 4;
                uint32_t nextRow = startIdx + (i + 1) * 4;

                m_Indices.push_back(currRow + c);
                m_Indices.push_back(nextRow + c);
                m_Indices.push_back(currRow + nextC);

                m_Indices.push_back(currRow + nextC);
                m_Indices.push_back(nextRow + c);
                m_Indices.push_back(nextRow + nextC);
            }
        }
    }

    // 2. GENERATE THE WARTY BURSTING SEED CAPSULE
    void BuildBurstingCapsule(Vector3 centerPosition, float radius) {
        uint32_t baseIdx = static_cast<uint32_t>(m_Vertices.size());
        int rings = 24;
        int sectors = 24;

        for (int r = 0; r <= rings; ++r) {
            float phi = PI * (float)r / rings;
            for (int s = 0; s <= sectors; ++s) {
                float theta = 2.0f * PI * (float)s / sectors;

                // Core spherical coordinates base mapping
                float x = std::sin(phi) * std::cos(theta);
                float y = std::cos(phi);
                float z = std::sin(phi) * std::sin(theta);

                Vector3 surfacePos = {x, y, z};
                
                // Procedural mathematical high-frequency noise simulation for warty surface spikes
                float wartySpikePattern = std::sin(x * 35.0f) * std::cos(z * 35.0f) * std::sin(y * 20.0f);
                float activeRadius = radius;
                
                if (wartySpikePattern > 0.2f) {
                    activeRadius += wartySpikePattern * 0.12f * radius; // Extrude tiny warts
                }

                // Simulate bursting fissure gap (Separating the capsule into 4 distinct quadrants)
                if (std::abs(x * z) < 0.08f && y > -0.2f) {
                    activeRadius -= 0.15f * radius; // Create deep split grooves
                }

                Vertex v;
                v.pos = centerPosition + surfacePos * activeRadius;
                v.normal = surfacePos; // Spherical normal map base assignment
                v.u = (float)s / sectors;
                v.v = (float)r / rings;
                v.wartyNoiseFactor = std::max(0.0f, wartySpikePattern);

                m_Vertices.push_back(v);
            }
        }

        // Face map index compilation loops
        for (int r = 0; r < rings; ++r) {
            for (int s = 0; s < sectors; ++s) {
                uint32_t r0 = baseIdx + r * (sectors + 1);
                uint32_t r1 = baseIdx + (r + 1) * (sectors + 1);

                m_Indices.push_back(r0 + s);
                m_Indices.push_back(r1 + s);
                m_Indices.push_back(r0 + (s + 1));

                m_Indices.push_back(r0 + (s + 1));
                m_Indices.push_back(r1 + s);
                m_Indices.push_back(r1 + (s + 1));
            }
        }
    }

    void ExportOBJ(const std::string& fileName) {
        std::ofstream out(fileName);
        if (!out.is_open()) return;

        out << "# Production Assembly Object Mapping Data: Euonymus americanus \n";
        for (const auto& v : m_Vertices) out << "v " << v.pos.x << " " << v.pos.y << " " << v.pos.z << "\n";
        for (const auto& v : m_Vertices) out << "vt " << v.u << " " << v.v << "\n";
        for (const auto& v : m_Vertices) out << "vn " << v.normal.x << " " << v.normal.y << " " << v.normal.z << "\n";

        out << "\ng Euonymus_Hearts_A_Bustin_Mesh\n";
        out << "usemtl M_Euonymus_Photometric_Core\n";
        
        for (size_t i = 0; i < m_Indices.size(); i += 3) {
            out << "f " << m_Indices[i]+1 << " " << m_Indices[i+1]+1 << " " << m_Indices[i+2]+1 << "\n";
        }
        out.close();
        std::cout << "[SUCCESS] Compiliation finished completely. Asset ready inside disk system.\n";
    }
};

int main() {
    EuonymusAssetEngine compiler;
    // Generate complex branch layout: Green square stems linking directly into bursting red capsule heads
    compiler.BuildSquareStem({0.0f, 0.0f, 0.0f}, {0.0f, 3.5f, 0.0f}, 0.12f);
    compiler.BuildSquareStem({0.0f, 3.5f, 0.0f}, {-1.2f, 4.8f, 0.5f}, 0.08f);
    compiler.BuildBurstingCapsule({-1.2f, 4.8f, 0.5f}, 0.45f); // Terminal fruit capsule deployment
    
    compiler.ExportOBJ("Euonymus_Americanus_Asset.obj");
    return 0;
}
