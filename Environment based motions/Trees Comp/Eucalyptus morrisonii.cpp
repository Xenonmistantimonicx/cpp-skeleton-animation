#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>
#include <stack>
#include <random>

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

struct Vertex {
    Vector3 pos;
    Vector3 normal;
    float u, v;
    float barkType; // 0.0 = Base rough bark, 1.0 = Smooth canopy bark
};

struct BranchNode {
    Vector3 startPos;
    Vector3 direction;
    float length;
    float startRadius;
    float endRadius;
    int depth;
};

class EucalyptusFullEngine {
private:
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;
    std::vector<Vertex> m_LeafVertices;
    std::vector<uint32_t> m_LeafIndices;
    
    // Perlin-like simple hash noise for rough fibrous bark simulation
    float Noise(float x, float y) {
        float n = std::sin(x * 12.9898f + y * 78.233f) * 43758.5453f;
        return n - std::floor(n);
    }

public:
    // Generates a complete branch segment with realistic twist and bark deformation
    void GenerateSegment(const BranchNode& node) {
        uint32_t startVertIdx = static_cast<uint32_t>(m_Vertices.size());
        const int radialSegments = 24;  // High fidelity mesh density
        const int heightSegments = 15;

        Vector3 dirNorm = node.direction;
        dirNorm.Normalize();

        Vector3 up = {0.0f, 1.0f, 0.0f};
        if (std::abs(dirNorm.Dot(up)) > 0.95f) up = {1.0f, 0.0f, 0.0f};
        Vector3 right = dirNorm.Cross(up); right.Normalize();
        up = right.Cross(dirNorm); up.Normalize();

        float barkFactor = (float)node.depth / 4.0f; // Transition parameter

        for (int i = 0; i <= heightSegments; ++i) {
            float hFactor = (float)i / heightSegments;
            
            // Eucalyptus typical growth twist & spiral deformation
            float twistAngle = hFactor * 0.4f * (1.0f - barkFactor);
            Vector3 rotatedRight = right * std::cos(twistAngle) - up * std::sin(twistAngle);
            Vector3 rotatedUp = right * std::sin(twistAngle) + up * std::cos(twistAngle);

            // Add organic curvature to the trunk direction
            Vector3 curveOffset = rotatedRight * std::sin(hFactor * PI) * 0.05f * node.length;
            Vector3 center = node.startPos + dirNorm * (node.length * hFactor) + curveOffset;
            
            float currentRadius = node.startRadius + (node.endRadius - node.startRadius) * hFactor;

            for (int j = 0; j <= radialSegments; ++j) {
                float angle = 2.0f * PI * (float)j / radialSegments;
                
                // Fibrous rough bark calculation for lower sections
                float barkDeform = 0.0f;
                if (node.depth <= 1) { 
                    float noiseVal = Noise(angle * 3.0f, hFactor * 12.0f);
                    barkDeform = (std::sin(angle * 8.0f) * 0.08f + noiseVal * 0.04f) * currentRadius;
                }

                float finalRadius = currentRadius + barkDeform;
                Vector3 surfaceOffset = rotatedRight * std::cos(angle) * finalRadius + rotatedUp * std::sin(angle) * finalRadius;

                Vertex v;
                v.pos = center + surfaceOffset;
                Vector3 norm = surfaceOffset; norm.Normalize();
                v.normal = norm;
                v.u = (float)j / radialSegments;
                v.v = hFactor * (node.length * 0.5f);
                v.barkType = saturate(barkFactor);

                m_Vertices.push_back(v);
            }
        }

        // Triangle generation logic for branch cylinders
        for (int i = 0; i < heightSegments; ++i) {
            for (int j = 0; j < radialSegments; ++j) {
                uint32_t currRow = startVertIdx + i * (radialSegments + 1);
                uint32_t nextRow = startVertIdx + (i + 1) * (radialSegments + 1);

                m_Indices.push_back(currRow + j);
                m_Indices.push_back(nextRow + j);
                m_Indices.push_back(currRow + j + 1);

                m_Indices.push_back(currRow + j + 1);
                m_Indices.push_back(nextRow + j);
                m_Indices.push_back(nextRow + j + 1);
            }
        }

        // Generate leaves if we are at terminal twig levels (Canopy build)
        if (node.depth >= 3) {
            GenerateFoliageCard(node.startPos + dirNorm * node.length, dirNorm);
        }
    }

    // Generates iconic lanceolate (long, sword-like) leaf cards
    void GenerateFoliageCard(Vector3 position, Vector3 direction) {
        uint32_t leafStartIdx = static_cast<uint32_t>(m_LeafVertices.size());
        
        Vector3 right = direction.Cross({0.0f, 1.0f, 0.0f});
        if (std::abs(right.Dot(right)) < 0.01f) right = {1.0f, 0.0f, 0.0f};
        right.Normalize();
        Vector3 up = right.Cross(direction); up.Normalize();

        float leafLength = 0.6f;
        float leafWidth = 0.15f;

        // Long thin hanging leaf model typical for Grey Mallee
        Vertex l1, l2, l3, l4;
        l1.pos = position - right * (leafWidth * 0.5f);
        l1.normal = up; l1.u = 0.0f; l1.v = 0.0f; l1.barkType = 1.0f;

        l2.pos = position + right * (leafWidth * 0.5f);
        l2.normal = up; l2.u = 1.0f; l2.v = 0.0f; l2.barkType = 1.0f;

        l3.pos = position + direction * leafLength;
        l3.normal = up; l3.u = 0.5f; l3.v = 1.0f; l3.barkType = 1.0f;

        m_LeafVertices.push_back(l1);
        m_LeafVertices.push_back(l2);
        m_LeafVertices.push_back(l3);

        m_LeafIndices.push_back(leafStartIdx);
        m_LeafIndices.push_back(leafStartIdx + 1);
        m_LeafIndices.push_back(leafStartIdx + 2);
    }

    // L-System simulator stack to grow the entire tree pipeline automatically
    void GrowEucalyptusTree() {
        std::stack<BranchNode> growthStack;

        // Base multi-stem trunk configuration (Typical Mallee clustering)
        growthStack.push({ {0.0f, 0.0f, 0.0f}, {-0.2f, 1.0f, 0.1f}, 2.5f, 0.30f, 0.20f, 0 });
        growthStack.push({ {0.0f, 0.0f, 0.0f}, {0.3f, 0.9f, -0.2f}, 2.2f, 0.28f, 0.18f, 0 });
        growthStack.push({ {0.1f, 0.0f, 0.0f}, {-0.0f, 1.2f, -0.3f}, 2.8f, 0.25f, 0.15f, 0 });

        while (!growthStack.empty()) {
            BranchNode current = growthStack.top();
            growthStack.pop();

            GenerateSegment(current);

            if (current.depth < 4) { // Max recursion depth limit
                Vector3 endPoint = current.startPos + current.direction * current.length;
                
                // Split logic into sub-branches
                Vector3 d1 = current.direction + Vector3{0.3f, 0.4f, -0.1f};
                Vector3 d2 = current.direction + Vector3{-0.4f, 0.3f, 0.3f};

                growthStack.push({ endPoint, d1, current.length * 0.75f, current.endRadius, current.endRadius * 0.6f, current.depth + 1 });
                growthStack.push({ endPoint, d2, current.length * 0.70f, current.endRadius, current.endRadius * 0.55f, current.depth + 1 });
            }
        }
    }

    void SaveToOBJ(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) return;

        file << "# Production Mesh Asset - Eucalyptus morrisonii (Fully Expanded)\n";
        
        // Write Trunk Vertices
        for (const auto& v : m_Vertices) file << "v " << v.pos.x << " " << v.pos.y << " " << v.pos.z << "\n";
        for (const auto& v : m_Vertices) file << "vt " << v.u << " " << v.v << "\n";
        for (const auto& v : m_Vertices) file << "vn " << v.normal.x << " " << v.normal.y << " " << v.normal.z << "\n";

        // Write Leaf Vertices
        size_t leafOffset = m_Vertices.size();
        for (const auto& l : m_LeafVertices) file << "v " << l.pos.x << " " << l.pos.y << " " << l.pos.z << "\n";
        for (const auto& l : m_LeafVertices) file << "vt " << l.u << " " << l.v << "\n";
        for (const auto& l : m_LeafVertices) file << "vn " << l.normal.x << " " << l.normal.y << " " << l.normal.z << "\n";

        // Group 1: Trunk Wood
        file << "\ng Eucalyptus_Trunk_Mesh\nusemtl M_Bark_PBR\n";
        for (size_t i = 0; i < m_Indices.size(); i += 3) {
            file << "f " << m_Indices[i]+1 << " " << m_Indices[i+1]+1 << " " << m_Indices[i+2]+1 << "\n";
        }

        // Group 2: Canopy Leaves
        file << "\ng Eucalyptus_Leaves_Mesh\nusemtl M_Waxy_Leaves_PBR\n";
        for (size_t i = 0; i < m_LeafIndices.size(); i += 3) {
            file << "f " << (m_LeafIndices[i] + leafOffset + 1) << " " 
                 << (m_LeafIndices[i+1] + leafOffset + 1) << " " 
                 << (m_LeafIndices[i+2] + leafOffset + 1) << "\n";
        }

        file.close();
        std::cout << "[SUCCESS] Saved high-fidelity asset data (" << m_Vertices.size() + m_LeafVertices.size() << " total vertices) to " << filename << "\n";
    }

private:
    float saturate(float val) { return std::max(0.0f, std::min(1.0f, val)); }
};

int main() {
    EucalyptusFullEngine engine;
    engine.GrowEucalyptusTree();
    engine.SaveToOBJ("Eucalyptus_Morrisonii_HighPoly.obj");
    return 0;
}
