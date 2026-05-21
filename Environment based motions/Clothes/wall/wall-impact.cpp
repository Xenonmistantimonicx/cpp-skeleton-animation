#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

// --- CLEAN 3D VECTOR FOR WALL PHYSICS ---
struct Vector3 {
    float x, y, z;
    Vector3 operator+(const Vector3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vector3 operator-(const Vector3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    float DistSq(const Vector3& o) const { return (x-o.x)*(x-o.x) + (y-o.y)*(y-o.y) + (z-o.z)*(z-o.z); }
};

// --- DEDICATED BRICK STRUCT (Memory Aligned) ---
struct Brick {
    Vector3 localPos;          // Deewar ke andar eeth ki coordinate position
    float brickHealth = 100.0f; // Individual brick density integrity
    float jointStiffness = 1.0f;// Cement bond strength with surrounding bricks (0.0 = Broken bond)
    bool isStaticAnchor = false;// Kya yeh eeth zameen ya pillar se judi hai?
    bool isAir = false;         // True ho gaya matlab yahan ab gaddha (hole) ho chuka hai
};

// --- THE PURE WALL ENGINE ---
class SolidGameWall {
private:
    std::vector<Brick> m_bricks;
    int m_rows;
    int m_cols;
    float m_brickLength = 0.4f; // 40cm standard brick length
    float m_brickHeight = 0.2f; // 20cm standard brick height
    Vector3 m_wallWorldOrigin;

    // Fast Flat-Array Lookup Helper
    inline int GetBrickIndex(int row, int col) const {
        return row * m_cols + col;
    }

public:
    SolidGameWall(int rows, int cols, Vector3 origin) 
        : m_rows(rows), m_cols(cols), m_wallWorldOrigin(origin) {
        
        m_bricks.resize(m_rows * m_cols);

        for (int r = 0; r < m_rows; ++r) {
            for (int c = 0; c < m_cols; ++c) {
                int idx = GetBrickIndex(r, c);
                auto& brick = m_bricks[idx];
                
                // Actual grid laying calculation (offsetting alternate rows for realistic brick pattern)
                float xOffset = (r % 2 == 0) ? 0.0f : (m_brickLength * 0.5f);
                brick.localPos = { (c * m_brickLength) + xOffset, r * m_brickHeight, 0.0f };

                // Base rows and side pillars are marked as rigid anchors
                if (r == 0 || c == 0 || c == m_cols - 1) {
                    brick.isStaticAnchor = true;
                    brick.brickHealth = 250.0f; // Reinforced rebar/concrete pillars
                }
            }
        }
        std::cout << "[WALL ENGINE] Solid brick wall generated with " << m_rows * m_cols << " synchronized bricks.\n";
    }

    // --- PURE LOCALIZED WALL HIT ENGINE ---
    void ApplyPhysicalImpact(Vector3 worldHitPoint, float impactEnergy, float blastRadius) {
        // Convert world collision directly into relative wall face coordinates
        Vector3 localHit = worldHitPoint - m_wallWorldOrigin;
        float radiusSq = blastRadius * blastRadius;

        int destroyedBricksCount = 0;
        int weakenedJointsCount = 0;

        std::cout << "\n[IMPACT] Shockwave hitting wall coordinate at relative X: " << localHit.x << " Y: " << localHit.y << "\n";

        // Step 1: Scan bricks instantly using cache lines
        for (auto& brick : m_bricks) {
            if (brick.isAir) continue;

            float distSq = brick.localPos.DistSq(localHit);
            if (distSq <= radiusSq) {
                float actualDist = std::sqrt(distSq);
                float falloff = 1.0f - (actualDist / blastRadius); // 1 at epicenter, 0 at border

                // Calculate damage based on distance from bullet/grenade impact point
                float directDamage = impactEnergy * falloff;
                
                if (!brick.isStaticAnchor) {
                    brick.brickHealth -= directDamage;
                    brick.jointStiffness -= (directDamage * 0.02f); // Cement crumbles
                } else {
                    brick.brickHealth -= (directDamage * 0.4f); // Anchors absorb shockwaves better
                }

                // Check if brick is completely turned into dust
                if (brick.brickHealth <= 0.0f) {
                    brick.isAir = true;
                    destroyedBricksCount++;
                    // Dispatch directly to GPU for rendering particles without blocking the CPU
                    TriggerDebrisRenderSpawn(brick.localPos + m_wallWorldOrigin);
                } else if (brick.jointStiffness <= 0.3f) {
                    weakenedJointsCount++;
                }
            }
        }

        std::cout << " >> Result: " << destroyedBricksCount << " bricks shattered to dust, " 
                  << weakenedJointsCount << " mortar joints cracked.\n";

        // Step 2: Structural Integrity Check (Deiwari Structural Fall)
        // Agar beech ki eetein gayab ho gayi hain toh upar wali eetein bina support ke hawa me nahi reh sakti!
        EnforceWallGravityCascade();
    }

private:
    void EnforceWallGravityCascade() {
        int gravityCollapses = 0;

        // Bottom-up pass to check if any brick is hanging in the air
        for (int r = 1; r < m_rows; ++r) {
            for (int c = 1; c < m_cols - 1; ++c) {
                int currentIdx = GetBrickIndex(r, c);
                
                if (m_bricks[currentIdx].isAir || m_bricks[currentIdx].isStaticAnchor) 
                    continue;

                // Check the brick directly underneath
                int underIdx = GetBrickIndex(r - 1, c);

                // Pure physics rule: If the cement joint is broken AND the brick below is missing, gravity wins!
                if (m_bricks[underIdx].isAir && m_bricks[currentIdx].jointStiffness < 0.5f) {
                    m_bricks[currentIdx].isAir = true;
                    gravityCollapses++;
                    TriggerDebrisRenderSpawn(m_bricks[currentIdx].localPos + m_wallWorldOrigin);
                }
            }
        }

        if (gravityCollapses > 0) {
            std::cout << " >> Structural Fall: " << gravityCollapses << " hanging bricks collapsed due to loss of lower foundation!\n";
        }
    }

    void TriggerDebrisRenderSpawn(Vector3 spawnPosition) {
        // GPU instant draw call link goes here
    }
};

// --- SIMULATION RUN ---
int main() {
    // Spawn a 15-row high, 30-column wide brick wall at world position (0,0,0)
    SolidGameWall levelWall(15, 30, {0.0f, 0.0f, 0.0f});

    // Test 1: Standard Bullet Impact (Point Damage)
    std::cout << "\n--- TEST 1: AK-47 7.62mm BULLET STRIKE ---";
    levelWall.ApplyPhysicalImpact({5.0f, 1.2f, 0.0f}, 60.0f, 0.5f); // 50cm micro blast radius

    // Test 2: Heavy Frag Grenade Blast (Massive Area Damage at the base)
    std::cout << "\n--- TEST 2: HE GRENADE EXPLOSION AT THE FOUNDATION ---";
    levelWall.ApplyPhysicalImpact({5.2f, 0.2f, 0.0f}, 350.0f, 1.8f); // 1.8m destructive shockwave radius

    return 0;
}
