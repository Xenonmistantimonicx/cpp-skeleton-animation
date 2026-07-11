#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>

// --- HIGH PERFORMANCE VECTOR MATHEMATICS PACK ---
struct Vector3 {
    float x, y, z;
    Vector3 operator+(const Vector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vector3 operator-(const Vector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vector3 Cross(const Vector3& v) const { return { y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x }; }
    float Dot(const Vector3& v) const { return x*v.x + y*v.y + z*v.z; }
    void Normalize() { float len = std::sqrt(x*x + y*y + z*z); if(len > 0.0f) { x /= len; y /= len; z /= len; } }
};

struct Vertex {
    Vector3 pos;
    Vector3 normal;
    float u, v;
    float veinMask; // Injected vector node mapping for advanced shader operations
};

class EchinodorusPipelineEngine {
private:
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;
    const float PI = 3.14159265359f;

public:
    // Generates a fully dense, high-fidelity Cellophane Leaf with procedural micro-veins
    void GenerateHighFidelityLeaf(Vector3 rootOrigin, float leafRotationAngle, float leafLength, float leafWidth, float stemArchPower) {
        uint32_t baseVertexOffset = static_cast<uint32_t>(m_Vertices.size());
        
        // High-fidelity tessellation density bounds
        int lengthSteps = 60; 
        int widthSteps = 32;

        float cosRot = std::cos(leafRotationAngle);
        float sinRot = std::sin(leafRotationAngle);

        for (int y = 0; y <= lengthSteps; ++y) {
            float v = (float)y / lengthSteps;
            float currentLength = v * leafLength;

            // Parametric silhouette curve for the broad, fragile oval shape of E. berteroi
            float profileWidth = std::sin(v * PI) * leafWidth;
            if (v > 0.75f) { // Gradual elegant taper towards the translucent apex
                profileWidth = std::sin(v * PI) * leafWidth * (1.0f - (v - 0.75f) * 4.0f);
            }

            // Exponential macro-curvature (Stem drop arch due to fluid weight inside water currents)
            float stemArchY = std::pow(v, 2.3f) * stemArchPower;

            for (int x = 0; x <= widthSteps; ++x) {
                float u = (float)x / widthSteps;
                float normalizedX = (u * 2.0f) - 1.0f; // Normalized coordinate bounds [-1, 1]

                float currentX = normalizedX * profileWidth;

                // PROCEDURAL VEIN & CELLOPHANE WRINKLE MATHEMATICS
                // Generate a high frequency secondary sinusoidal wrinkle that mimics dried cellophane texture
                float microRipple = std::sin(v * 24.0f + normalizedX * 8.0f) * 0.025f * (1.0f - std::abs(normalizedX));
                
                // Procedural Midrib definition (thicker spine projection at the leaf center alignment axis)
                float midribDeform = 0.0f;
                float absX = std::abs(normalizedX);
                if (absX < 0.05f) {
                    midribDeform = (0.05f - absX) * 0.3f * leafWidth;
                }

                float localZ = stemArchY + microRipple - midribDeform;

                // Transform points dynamically inside the Rosette Coordinate Matrix Plane
                Vector3 transformedPos;
                transformedPos.x = rootOrigin.x + (currentX * cosRot - localZ * sinRot);
                transformedPos.y = rootOrigin.y + currentLength;
                transformedPos.z = rootOrigin.z + (currentX * sinRot + localZ * cosRot);

                // Precise Analytical Normal Estimation Matrix
                Vector3 tangentX = { cosRot, 0.0f, sinRot };
                Vector3 tangentY = { 0.0f, 1.0f, 0.0f };
                if (absX > 0.01f) {
                    tangentY.z = (std::pow(v, 1.3f) * stemArchPower) / leafLength;
                }
                Vector3 surfaceNormal = tangentX.Cross(tangentY);
                surfaceNormal.Normalize();

                Vertex vert;
                vert.pos = transformedPos;
                vert.normal = surfaceNormal;
                vert.u = u;
                vert.v = v;
                // Structural Vein Mask allocation vector (1.0 = thick vein center, 0.0 = tissue wing edge)
                vert.veinMask = std::max(0.0f, 1.0f - (absX * 15.0f)); 

                m_Vertices.push_back(vert);
            }
        }

        // Mathematical Topology Index Matrix Stitching
        for (int y = 0; y < lengthSteps; ++y) {
            for (int x = 0; x < widthSteps; ++x) {
                uint32_t r0 = baseVertexOffset + y * (widthSteps + 1);
                uint32_t r1 = baseVertexOffset + (y + 1) * (widthSteps + 1);

                // Face Triangle Segment A
                m_Indices.push_back(r0 + x);
                m_Indices.push_back(r1 + x);
                m_Indices.push_back(r0 + (x + 1));

                // Face Triangle Segment B
                m_Indices.push_back(r0 + (x + 1));
                m_Indices.push_back(r1 + x);
                m_Indices.push_back(r1 + (x + 1));
            }
        }
    }

    void CompileFullRosetteCluster() {
        std::cout << "[AAA ENGINE CORE]: Compiling Echinodorus berteroi Cluster Graph...\n";
        
        // Generate a dense, multi-layered spiral rosette array (360-degree ecosystem)
        int leafCount = 12;
        float goldenAngle = 2.39996f; // Radians distribution index mapping (approx 137.5 degrees)

        for (int i = 0; i < leafCount; ++i) {
            float currentRotation = i * goldenAngle;
            float layerScale = 1.0f - ((float)i / leafCount) * 0.4f; // Inner leaves are smaller
            
            Vector3 centerOffset = { std::cos(currentRotation) * 0.05f, 0.0f, std::sin(currentRotation) * 0.05f };
            
            GenerateHighFidelityLeaf(
                centerOffset, 
                currentRotation, 
                5.0f * layerScale,   // Leaf length scalar
                1.1f * layerScale,   // Ultra-broad width factor
                -0.75f * layerScale  // Arc droop curvature
            );
        }
    }

    void ExportAssetToOBJ(const std::string& pathName) {
        std::ofstream file(pathName);
        if (!file.is_open()) {
            std::cerr << "[IO ERROR]: Vertex serialization stream initialization failed!\n";
            return;
        }

        file << "# Production-Grade Echinodorus berteroi Vertex Cache Master\n";
        file << "# Total Evaluated Triangles: " << m_Indices.size() / 3 << "\n";

        for (const auto& v : m_Vertices) file << "v " << v.pos.x << " " << v.pos.y << " " << v.pos.z << "\n";
        for (const auto& v : m_Vertices) file << "vt " << v.u << " " << v.v << "\n";
        // Export custom custom channel values hidden inside specific normal vectors if required
        for (const auto& v : m_Vertices) file << "vn " << v.normal.x << " " << v.normal.y << " " << v.normal.z << "\n";

        file << "\ng Echinodorus_Cellophane_Leaves\n";
        file << "usemtl M_Cellophane_Submerged_Master\n";
        
        for (size_t i = 0; i < m_Indices.size(); i += 3) {
            uint32_t i0 = m_Indices[i] + 1; uint32_t i1 = m_Indices[i+1] + 1; uint32_t i2 = m_Indices[i+2] + 1;
            file << "f " << i0 << "/" << i0 << "/" << i0 << " " << i1 << "/" << i1 << "/" << i1 << " " << i2 << "/" << i2 << "/" << i2 << "\n";
        }
        file.close();
        std::cout << "[SUCCESS]: High-Grade Asset Buffer compiled into out disk matrix: " << pathName << "\n";
    }
};

int main() {
    EchinodorusPipelineEngine productionCompiler;
    productionCompiler.CompileFullRosetteCluster();
    productionCompiler.ExportAssetToOBJ("Echinodorus_Berteroi_AAA_Asset.obj");
    return 0;
}
