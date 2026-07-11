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

struct PlantVertex {
    Vector3 pos;
    Vector3 normal;
    float u, v;
    float exfoliationMask; // 0.0 = Old rough bark, 1.0 = Shedded new inner wood layer
    float nodeType;        // 0.0 = Timber/Branch, 1.0 = Foliage, 2.0 = Velvet Bract Flag
};

struct GrowthNode {
    Vector3 basePos;
    Vector3 direction;
    float branchLength;
    float radiusStart;
    float radiusEnd;
    int generationLevel;
    float ageScale;        // Controls the peeling bark threshold
};

class EmmenopterysEcosystemEngine {
private:
    std::vector<PlantVertex> m_GlobalVertices;
    std::vector<uint32_t>    m_GlobalIndices;
    
    // Deterministic pseudo-random generation for botanical structural authenticity
    float StructuralHash(float x, float y, float z) {
        float dotProduct = x * 12.9898f + y * 78.233f + z * 45.164f;
        float sineWave = std::sin(dotProduct) * 43758.5453123f;
        return sineWave - std::floor(sineWave);
    }

public:
    void SimulateGrowthMatrix() {
        std::stack<GrowthNode> simulationStack;
        
        std::cout << "[AAA ECO-ENGINE]: Simulating Emmenopterys henryi Growth Grid...\n";

        // Seed 3 initial massive architectural trunks to simulate forest canopy competition
        simulationStack.push({ {0.0f, 0.0f, 0.0f}, {0.05f, 1.0f, 0.02f}, 4.2f, 0.45f, 0.35f, 0, 1.0f });
        simulationStack.push({ {0.0f, 0.2f, 0.0f}, {-0.15f, 0.95f, -0.1f}, 3.8f, 0.38f, 0.28f, 0, 0.85f });

        while (!simulationStack.empty()) {
            GrowthNode current = simulationStack.top();
            simulationStack.pop();

            BuildTimberSegment(current);

            if (current.generationLevel < 5) { // Deep recursive distribution splits
                Vector3 tipPosition = current.basePos + current.direction * current.branchLength;
                
                // Opposite branching behavior—typical characteristic layout of Emmenopterys
                Vector3 orthogonalVector1 = current.direction + Vector3{0.45f, 0.2f, -0.15f};
                Vector3 orthogonalVector2 = current.direction + Vector3{-0.45f, 0.2f, 0.15f};
                
                float structuralDecay = 0.72f;

                simulationStack.push({ tipPosition, orthogonalVector1, current.branchLength * structuralDecay, current.radiusEnd, current.radiusEnd * 0.55f, current.generationLevel + 1, current.ageScale * 0.7f });
                simulationStack.push({ tipPosition, orthogonalVector2, current.branchLength * structuralDecay, current.radiusEnd, current.radiusEnd * 0.55f, current.generationLevel + 1, current.ageScale * 0.7f });
            }
        }
    }

private:
    void BuildTimberSegment(const GrowthNode& node) {
        uint32_t startIndexOffset = static_cast<uint32_t>(m_GlobalVertices.size());
        const int ringResolution = 32; 
        const int heightResolution = 20;

        Vector3 axis = node.direction; axis.Normalize();
        Vector3 binormal = {0.0f, 1.0f, 0.0f};
        if (std::abs(axis.Dot(binormal)) > 0.96f) binormal = {1.0f, 0.0f, 0.0f};
        Vector3 tangent = axis.Cross(binormal); tangent.Normalize();
        binormal = tangent.Cross(axis); binormal.Normalize();

        for (int y = 0; y <= heightResolution; ++y) {
            float vFactor = (float)y / heightResolution;
            Vector3 centerPoint = node.basePos + axis * (node.branchLength * vFactor);
            float activeRadius = node.radiusStart + (node.radiusEnd - node.radiusStart) * vFactor;

            for (int x = 0; x <= ringResolution; ++x) {
                float radialAngle = 2.0f * PI * (float)x / ringResolution;
                
                // Real-world Bark Peeling (Exfoliation) Simulation Math
                // Creates longitudinal fracture ridges where bark detaches from the trunk tissue
                float structuralNoise = StructuralHash(std::cos(radialAngle), vFactor * 4.5f, node.ageScale);
                float exfoliationEdge = std::sin(radialAngle * 6.0f) * std::cos(vFactor * 8.0f) + structuralNoise * 0.3f;
                
                float surfaceDisplacement = 0.0f;
                float exfoliationMaskValue = 0.0f;

                // Older branches peel heavily based on age scale threshold parameters
                if (node.ageScale > 0.4f && exfoliationEdge > 0.1f) {
                    surfaceDisplacement = 0.025f * activeRadius; // Outer curling dead bark flake layer
                    exfoliationMaskValue = 0.0f;                 // Dead outer skin signifier
                } else {
                    surfaceDisplacement = -0.01f * activeRadius; // New smooth inner sapwood layer exposed
                    exfoliationMaskValue = 1.0f;                  // Fresh under-layer signifier
                }

                float computationalRadius = activeRadius + surfaceDisplacement;
                Vector3 vertexOffset = tangent * std::cos(radialAngle) * computationalRadius + binormal * std::sin(radialAngle) * computationalRadius;

                PlantVertex vertex;
                vertex.pos = centerPoint + vertexOffset;
                Vector3 normalVector = vertexOffset; normalVector.Normalize();
                vertex.normal = normalVector;
                vertex.u = (float)x / ringResolution;
                vertex.v = vFactor * (node.branchLength * 0.33f);
                vertex.exfoliationMask = exfoliationMaskValue;
                vertex.nodeType = 0.0f; // Timber declaration identifier

                m_GlobalVertices.push_back(vertex);
            }
        }

        // Stitch dynamic vertex buffer index arrays
        for (int y = 0; y < heightResolution; ++y) {
            for (int x = 0; x < ringResolution; ++x) {
                uint32_t currentTier = startIndexOffset + y * (ringResolution + 1);
                uint32_t upperTier   = startIndexOffset + (y + 1) * (ringResolution + 1);

                m_GlobalIndices.push_back(currentTier + x);
                m_GlobalIndices.push_back(upperTier + x);
                m_GlobalIndices.push_back(currentTier + x + 1);

                m_GlobalIndices.push_back(currentTier + x + 1);
                m_GlobalIndices.push_back(upperTier + x);
                m_GlobalIndices.push_back(upperTier + x + 1);
            }
        }

        // If node reaches terminal canopy tiers, sprout the broad leaves and legendary velvet bract structures
        if (node.generationLevel >= 4) {
            InjectCanopyStructures(node.basePos + axis * node.branchLength, axis, tangent);
        }
    }

    void InjectCanopyStructures(Vector3 position, Vector3 growthDirection, Vector3 planeTangent) {
        // Standard structural orientation setup for hanging canvas cards
        Vector3 binormalDir = growthDirection.Cross(planeTangent); binormalDir.Normalize();
        uint32_t leafBaseIdx = static_cast<uint32_t>(m_GlobalVertices.size());

        // 1. BROAD ELONGATED OPPOSITE LEAF CARDS (Deciduous canopy components)
        PlantVertex l0, l1, l2;
        l0.pos = position; l0.normal = binormalDir; l0.u = 0.5f; l0.v = 0.0f; l0.exfoliationMask = 1.0f; l0.nodeType = 1.0f;
        l1.pos = position + (planeTangent * 0.5f) + (growthDirection * 0.6f); l1.normal = binormalDir; l1.u = 1.0f; l1.v = 1.0f; l1.exfoliationMask = 1.0f; l1.nodeType = 1.0f;
        l2.pos = position - (planeTangent * 0.5f) + (growthDirection * 0.6f); l2.normal = binormalDir; l2.u = 0.0f; l2.v = 1.0f; l2.exfoliationMask = 1.0f; l2.nodeType = 1.0f;

        m_GlobalVertices.push_back(l0); m_GlobalVertices.push_back(l1); m_GlobalVertices.push_back(l2);
        m_GlobalIndices.push_back(leafBaseIdx); m_GlobalIndices.push_back(leafBaseIdx + 1); m_GlobalIndices.push_back(leafBaseIdx + 2);

        // 2. THE LEGENDARY GIANT WHITE BRACT FLAGS (The iconic hallmark of E. Henryi)
        // Highly flexible, delicate structures drooping dramatically to flutter under low wind velocities
        uint32_t bractBaseIdx = static_cast<uint32_t>(m_GlobalVertices.size());
        Vector3 droopVector = (growthDirection * 0.3f - binormalDir * 0.7f); droopVector.Normalize();

        PlantVertex b0, b1, b2, b3;
        float bWidth = 0.35f; float bLength = 0.9f;
        
        b0.pos = position; // Fixed stem intersection anchor point
        b0.normal = planeTangent; b0.u = 0.5f; b0.v = 0.0f; b0.exfoliationMask = 1.0f; b0.nodeType = 2.0f; // Type 2 = White Bract

        b1.pos = position + droopVector * (bLength * 0.5f) + planeTangent * bWidth;
        b1.normal = planeTangent; b1.u = 1.0f; b1.v = 0.5f; b1.exfoliationMask = 1.0f; b1.nodeType = 2.0f;

        b2.pos = position + droopVector * (bLength * 0.5f) - planeTangent * bWidth;
        b2.normal = planeTangent; b2.u = 0.0f; b2.v = 0.5f; b2.exfoliationMask = 1.0f; b2.nodeType = 2.0f;

        b3.pos = position + droopVector * bLength; // Tip of the white flag component
        b3.normal = planeTangent; b3.u = 0.5f; b3.v = 1.0f; b3.exfoliationMask = 1.0f; b3.nodeType = 2.0f;

        m_GlobalVertices.push_back(b0); m_GlobalVertices.push_back(b1); m_GlobalVertices.push_back(b2); m_GlobalVertices.push_back(b3);

        m_GlobalIndices.push_back(bractBaseIdx); m_GlobalIndices.push_back(bractBaseIdx + 1); m_GlobalIndices.push_back(bractBaseIdx + 2);
        m_GlobalIndices.push_back(bractBaseIdx + 2); m_GlobalIndices.push_back(bractBaseIdx + 1); m_GlobalIndices.push_back(bractBaseIdx + 3);
    }

public:
    void ExportSerializedAsset(const std::string& fileName) {
        std::ofstream diskStream(fileName);
        if (!diskStream.is_open()) return;

        diskStream << "# Emmenopterys Henryi Master Mesh Database Configuration Output System\n";
        
        // Serialize absolute geometric float coordinates arrays
        for (const auto& v : m_GlobalVertices) {
            diskStream << "v " << v.pos.x << " " << v.pos.y << " " << v.pos.z << "\n";
        }
        // Pack custom texture tracking offsets inside standard UV spaces channels
        for (const auto& v : m_GlobalVertices) {
            diskStream << "vt " << v.u << " " << v.v << "\n";
        }
        // Custom pipeline normal mapping projections vector array
        for (const auto& v : m_GlobalVertices) {
            // Inject structural classification masks inside normal vector data tracks to avoid channel losses
            diskStream << "vn " << v.normal.x << " " << v.normal.y << " " << v.nodeType << "\n";
        }

        diskStream << "\ng Emmenopterys_Complete_StructuralMesh\n";
        diskStream << "usemtl M_Emmenopterys_MasterMaterial\n";
        
        for (size_t i = 0; i < m_GlobalIndices.size(); i += 3) {
            uint32_t i0 = m_GlobalIndices[i] + 1; uint32_t i1 = m_GlobalIndices[i+1] + 1; uint32_t i2 = m_GlobalIndices[i+2] + 1;
            diskStream << "f " << i0 << "/" << i0 << "/" << i0 << " " << i1 << "/" << i1 << "/" << i1 << " " << i2 << "/" << i2 << "/" << i2 << "\n";
        }
        diskStream.close();
        std::cout << "[COMPLETE SUCCESS]: Production Asset fully built and stored at target destination: " << fileName << "\n";
    }
};

int main() {
    EmmenopterysEcosystemEngine plantEngine;
    plantEngine.SimulateGrowthMatrix();
    plantEngine.ExportSerializedAsset("Emmenopterys_Henryi_AAA_Asset.obj");
    return 0;
}
