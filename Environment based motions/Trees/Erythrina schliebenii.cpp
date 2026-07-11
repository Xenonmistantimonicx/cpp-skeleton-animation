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
    Vector3 TransformPoint(const Vector3& p) const {
        return {
            p.x * m[0][0] + p.y * m[1][0] + p.z * m[2][0] + m[3][0],
            p.x * m[0][1] + p.y * m[1][1] + p.z * m[2][1] + m[3][1],
            p.x * m[0][2] + p.y * m[1][2] + p.z * m[2][2] + m[3][2]
        };
    }
    Vector3 TransformDirection(const Vector3& d) const {
        return {
            d.x * m[0][0] + d.y * m[1][0] + d.z * m[2][0],
            d.x * m[0][1] + d.y * m[1][1] + d.z * m[2][1],
            d.x * m[0][2] + d.y * m[1][2] + d.z * m[2][2]
        };
    }
};

struct Vertex {
    Vector3 pos;
    Vector3 normal;
    float u, v;
};

class ErythrinaMeshPipeline {
private:
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;
    const float PI = 3.14159265359f;

    Matrix4x4 LookAt(Vector3 origin, Vector3 targetDir) {
        targetDir.Normalize();
        Vector3 up = (std::abs(targetDir.y) < 0.9f) ? Vector3{0.0f, 1.0f, 0.0f} : Vector3{1.0f, 0.0f, 0.0f};
        Vector3 right = targetDir.Cross(up); right.Normalize();
        up = right.Cross(targetDir); up.Normalize();

        Matrix4x4 mat = Matrix4x4::Identity();
        mat.m[0][0] = right.x;     mat.m[0][1] = right.y;     mat.m[0][2] = right.z;
        mat.m[1][0] = targetDir.x; mat.m[1][1] = targetDir.y; mat.m[1][2] = targetDir.z;
        mat.m[2][0] = up.x;        mat.m[2][1] = up.y;        mat.m[2][2] = up.z;
        mat.m[3][0] = origin.x;    mat.m[3][1] = origin.y;    mat.m[3][2] = origin.z;
        return mat;
    }

public:
    void GenerateArmoredSegment(Vector3 base, Vector3 tip, float rStart, float rEnd, int radialSubdiv, float vOffset) {
        uint32_t indexBase = static_cast<uint32_t>(m_Vertices.size());
        Vector3 branchVector = tip - base;
        float h = std::sqrt(branchVector.Dot(branchVector));
        Matrix4x4 transformStack = LookAt(base, branchVector);

        int verticalRings = 12; // Production fidelity resolution
        for (int r = 0; r <= verticalRings; ++r) {
            float t = (float)r / verticalRings;
            float currentR = rStart * (1.0f - t) + rEnd * t;
            float segmentY = h * t;

            for (int i = 0; i <= radialSubdiv; ++i) {
                float theta = ((float)i / radialSubdiv) * 2.0f * PI;
                Vector3 radialNormal = { std::cos(theta), 0.0f, std::sin(theta) };

                // ERYTHRINA ALGORITHM: High frequency procedural spine injection matrix
                float thornFreq = 8.0f; 
                float waveFactor = std::sin(theta * thornFreq) * std::cos((segmentY + vOffset) * 3.5f);
                float sharpThornExtrusion = 0.0f;
                
                if (waveFactor > 0.4f) {
                    // Exponential power sharpens the topological peak of the spine vertex
                    sharpThornExtrusion = std::pow((waveFactor - 0.4f) * 1.666f, 4.0f) * 0.5f * currentR;
                }

                float activeRadius = currentR + sharpThornExtrusion;
                Vector3 localPos = { radialNormal.x * activeRadius, segmentY, radialNormal.z * activeRadius };

                Vertex vertex;
                vertex.pos = transformStack.TransformPoint(localPos);
                vertex.normal = transformStack.TransformDirection(radialNormal);
                vertex.u = (float)i / radialSubdiv;
                vertex.v = t + vOffset; // Global seamless continuous wrap
                m_Vertices.push_back(vertex);
            }
        }

        // Stitch index buffer triangles
        for (int r = 0; r < verticalRings; ++r) {
            for (int i = 0; i < radialSubdiv; ++i) {
                uint32_t r0 = indexBase + r * (radialSubdiv + 1);
                uint32_t r1 = indexBase + (r + 1) * (radialSubdiv + 1);

                m_Indices.push_back(r0 + i);
                m_Indices.push_back(r1 + i);
                m_Indices.push_back(r0 + (i + 1));

                m_Indices.push_back(r0 + (i + 1));
                m_Indices.push_back(r1 + i);
                m_Indices.push_back(r1 + (i + 1));
            }
        }
    }

    void ExecuteRecursiveLSystem(Vector3 node, Vector3 direction, float len, float rad, int currentDepth, float globalYOffset) {
        if (currentDepth > 5) return; // Production level density depth

        Vector3 endpoint = node + direction * len;
        GenerateArmoredSegment(node, endpoint, rad, rad * 0.72f, 24, globalYOffset);

        // Natural organic phyllotaxis angles
        float splitAngle = 0.42f; 
        Vector3 branchA = { direction.x + std::sin(splitAngle), direction.y * 0.85f + std::cos(splitAngle), direction.z + 0.1f };
        Vector3 branchB = { direction.x - std::sin(splitAngle), direction.y * 0.85f + std::cos(splitAngle), direction.z - 0.2f };
        branchA.Normalize();
        branchB.Normalize();

        ExecuteRecursiveLSystem(endpoint, branchA, len * 0.76f, rad * 0.62f, currentDepth + 1, globalYOffset + len);
        ExecuteRecursiveLSystem(endpoint, branchB, len * 0.72f, rad * 0.62f, currentDepth + 1, globalYOffset + len * 1.4f);
    }

    void WriteMeshToDisk(const std::string& path) {
        std::ofstream stream(path);
        if (!stream.is_open()) return;

        stream << "# Ready-to-Use Procedural Erythrina schliebenii Geometry\n";
        for (const auto& v : m_Vertices) stream << "v " << v.pos.x << " " << v.pos.y << " " << v.pos.z << "\n";
        for (const auto& v : m_Vertices) stream << "vt " << v.u << " " << v.v << "\n";
        for (const auto& v : m_Vertices) stream << "vn " << v.normal.x << " " << v.normal.y << " " << v.normal.z << "\n";
        
        stream << "\nusemtl Erythrina_Bark_Material\n";
        for (size_t i = 0; i < m_Indices.size(); i += 3) {
            uint32_t i0 = m_Indices[i] + 1; uint32_t i1 = m_Indices[i+1] + 1; uint32_t i2 = m_Indices[i+2] + 1;
            stream << "f " << i0 << "/" << i0 << "/" << i0 << " " << i1 << "/" << i1 << "/" << i1 << " " << i2 << "/" << i2 << "/" << i2 << "\n";
        }
        stream.close();
        std::cout << "[SUCCESS] File exported as " << path << " -> Open directly in Blender!\n";
    }
};

int main() {
    ErythrinaMeshPipeline generator;
    generator.ExecuteRecursiveLSystem({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 7.0f, 0.9f, 0, 0.0f);
    generator.WriteMeshToDisk("ErythrinaCoreAsset.obj");
    return 0;
}
