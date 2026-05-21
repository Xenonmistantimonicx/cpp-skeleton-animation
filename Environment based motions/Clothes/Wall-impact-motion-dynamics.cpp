#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <iomanip>

// --- HIGH ACCURACY MULTI-PURPOSE SIMD VECTOR ---
struct Vector3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    
    Vector3 operator+(const Vector3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vector3 operator-(const Vector3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    
    float Length() const { return std::sqrt(x*x + y*y + z*z); }
    Vector3 Normalize() const {
        float len = Length();
        return (len > 0.00001f) ? Vector3{x/len, y/len, z/len} : Vector3{0,0,0};
    }
    float DistSq(const Vector3& o) const { 
        return (x-o.x)*(x-o.x) + (y-o.y)*(y-o.y) + (z-o.z)*(z-o.z); 
    }
};

// --- REAL WORLD INCOMING INTERACTABLE OBJECT STRUCT ---
struct ImpactingObject {
    std::string objectName;
    float massKg;              // Object ka asli weight/mass in Kilograms
    Vector3 velocityVector;    // Speed along with directional angle (m/s)
    float contactSurfaceRadius;// Impact point ka area (e.g., Bullet point size vs Car bumper width)
    
    // Calculates Energy dynamically: KE = 0.5 * m * v^2
    float CalculateKineticEnergy() const {
        float speed = velocityVector.Length();
        return 0.5f * massKg * (speed * speed);
    }

    // Calculates Momentum: P = m * v
    float CalculateMomentum() const {
        return massKg * velocityVector.Length();
    }
};

// --- DEDICATED REINFORCED BRICK DATA STRUCTURE ---
struct WallBrick {
    Vector3 localPos;
    float currentHealth = 150.0f;    // 150 Joules of energy threshold resistance
    float mortarStiffness = 1.0f;    // Bond connection with adjacent bricks (0.0 = Detached completely)
    bool isReinforcedPillar = false; // Corner pillars or lintel bars
    bool isPulverized = false;       // True means brick turned into dust/air hole
};

// --- PHYSICAL DYNAMIC DESTRUCTIBLE WALL SYSTEM ---
class PhysicalImpactWall {
private:
    std::vector<WallBrick> m_bricks;
    int m_rows;
    int m_cols;
    float m_brickWidth = 0.30f;  // 30 cm bricks
    float m_brickHeight = 0.15f; // 15 cm bricks
    Vector3 m_wallWorldPosition;

    inline int GetIndex(int r, int c) const { return r * m_cols + c; }

public:
    PhysicalImpactWall(int rows, int cols, Vector3 origin) 
        : m_rows(rows), m_cols(cols), m_wallWorldPosition(origin) {
        
        m_bricks.resize(m_rows * m_cols);

        for (int r = 0; r < m_rows; ++r) {
            for (int c = 0; c < m_cols; ++c) {
                int idx = GetIndex(r, c);
                auto& brick = m_bricks[idx];
                
                // Classic realistic interlaced architectural bond layout
                float staggeredOffset = (r % 2 == 0) ? 0.0f : (m_brickWidth * 0.5f);
                brick.localPos = { (c * m_brickWidth) + staggeredOffset, r * m_brickHeight, 0.0f };

                // Define load-bearing foundation and side structural pillars
                if (r == 0 || c == 0 || c == m_cols - 1) {
                    brick.isReinforcedPillar = true;
                    brick.currentHealth = 600.0f; // High-density concrete rebar structure
                }
            }
        }
    }

    // --- MAIN PHYSICS IMPULSE PROCESSING PIPELINE ---
    void ProcessMassivePhysicalImpact(const ImpactingObject& incomingObj, Vector3 worldHitPoint) {
        Vector3 localHitPos = worldHitPoint - m_wallWorldPosition;
        
        float kineticEnergy = incomingObj.CalculateKineticEnergy();
        float momentum = incomingObj.CalculateMomentum();
        
        std::cout << "\n========================================================================\n";
        std::cout << "  INCOMING IMPACT: " << incomingObj.objectName << "\n";
        std::cout << "========================================================================\n";
        std::cout << " -> Object Mass      : " << incomingObj.massKg << " kg\n";
        std::cout << " -> Velocity Velocity: " << incomingObj.velocityVector.Length() << " m/s (~" << (incomingObj.velocityVector.Length() * 3.6f) << " km/h)\n";
        std::cout << " -> Total Energy     : " << kineticEnergy / 1000.0f << " kJ (KiloJoules)\n";
        std::cout << " -> Linear Momentum  : " << momentum << " kg*m/s\n";
        std::cout << " -> Contact Radius   : " << incomingObj.contactSurfaceRadius << " meters\n";

        // Material stress response variables
        int directShatteredBricks = 0;
        int structurallyWeakenedBricks = 0;

        // Base stress penetration threshold: Mass directly dictates shockwave travel radius inside solid bodies!
        // Massive objects spread shockwaves much further than micro objects even with identical energy signatures.
        float effectiveStressRadius = incomingObj.contactSurfaceRadius + (momentum * 0.0015f);
        float effectiveStressRadiusSq = effectiveStressRadius * effectiveStressRadius;

        // Loop optimized for linear cache memory lines
        for (auto& brick : m_bricks) {
            if (brick.isPulverized) continue;

            float distanceSq = brick.localPos.DistSq(localHitPos);
            if (distanceSq <= effectiveStressRadiusSq) {
                float actualDistance = std::sqrt(distanceSq);
                
                // Mechanical Dissipation Falloff Profile
                float falloff = 1.0f - (actualDistance / effectiveStressRadius);

                // --- THE CORE WEIGHT/MASS EQUATION ---
                // 1. High Mass + High Area = Massive Shear Stress across structural blocks
                // 2. Low Mass + Tiny Area = High Puncture Force concentrated at a singular lock node
                float dynamicForceInflicted = 0.0f;

                if (incomingObj.contactSurfaceRadius < 0.05f) {
                    // Puncture Mechanics (Bullets/Shrapnel): Penetration Depth is key
                    dynamicForceInflicted = (kineticEnergy * falloff) / (incomingObj.contactSurfaceRadius * 10.0f);
                } else {
                    // Blunt Kinetic Impact Mechanics (Humans, Vehicles, Crashing Debris): Momentum transfer wins
                    dynamicForceInflicted = (momentum * falloff * incomingObj.velocityVector.Length() * 0.1f);
                }

                // Apply damage vectors to physical brick assets
                if (brick.isReinforcedPillar) {
                    brick.currentHealth -= (dynamicForceInflicted * 0.35f); // Pillars dissipate kinetic shocks safely
                    brick.mortarStiffness -= (dynamicForceInflicted * 0.001f);
                } else {
                    brick.currentHealth -= dynamicForceInflicted;
                    brick.mortarStiffness -= (dynamicForceInflicted * 0.01f); // Mortar bonding crumbles
                }

                // Evaluate structural integrity thresholds
                if (brick.currentHealth <= 0.0f) {
                    brick.isPulverized = true;
                    directShatteredBricks++;
                } else if (brick.mortarStiffness < 0.6f) {
                    structurallyWeakenedBricks++;
                }
            }
        }

        std::cout << "\n [ENGINE REPORT]\n";
        std::cout << "   [*] Immediate Bricks Obliterated to Shards: " << directShatteredBricks << "\n";
        std::cout << "   [*] Bricks with Cracked Structural Mortar : " << structurallyWeakenedBricks << "\n";

        // Step 2: Recalculate global stability maps based on foundational weight shifts
        EvaluateWallStructuralIntegrity();
    }

private:
    // --- REALTIME STRUCTURAL STABILITY PASSTHROUGH ENGINE ---
    void EvaluateWallStructuralIntegrity() {
        int cascadeCollapses = 0;

        // Bottom-up evaluation pass mimicking real physical gravity load strains
        for (int r = 1; r < m_rows; ++r) {
            for (int c = 1; c < m_cols - 1; ++c) {
                int targetIndex = GetIndex(r, c);
                auto& currentBrick = m_bricks[targetIndex];

                if (currentBrick.isPulverized || currentBrick.isReinforcedPillar) continue;

                // Query support matrices directly beneath the element footprint
                int bottomIndex = GetIndex(r - 1, c);
                int bottomLeftIndex = GetIndex(r - 1, c - 1);
                int bottomRightIndex = GetIndex(r - 1, c + 1);

                // Critical Collapse Logic: If the foundation below is completely gone,
                // check if mortar joints on sides can withstand the suspended gravity load stress.
                if (m_bricks[bottomIndex].isPulverized) {
                    float structuralLoadStress = 1.0f; // Baseline gravity factor

                    if (m_bricks[bottomLeftIndex].isPulverized)  structuralLoadStress += 1.5f;
                    if (m_bricks[bottomRightIndex].isPulverized) structuralLoadStress += 1.5f;

                    // If current brick joints are too loose to hang as a cantilever cantilever beam -> Collapse
                    if (currentBrick.mortarStiffness < (structuralLoadStress * 0.35f)) {
                        currentBrick.isPulverized = true;
                        currentBrick.currentHealth = 0.0f;
                        cascadeCollapses++;
                    }
                }
            }
        }

        if (cascadeCollapses > 0) {
            std::cout << "   [GRAVITY CASCADE] " << cascadeCollapses << " additional bricks collapsed naturally due to heavy load failure!\n";
        }
        std::cout << "========================================================================\n";
    }
};

// --- SIMULATION WORKFLOW ---
int main() {
    // Construct a solid brick wall (20 rows high, 40 columns wide) at world origin layout
    PhysicalImpactWall cityWall(20, 40, {0.0f, 0.0f, 0.0f});

    // --- OBJECT 1: HIGH VELOCITY, ULTRA LOW WEIGHT (AK-47 Round) ---
    ImpactingObject ak47Bullet;
    ak47Bullet.objectName = "7.62mm Ballistic Assault Rifle Bullet";
    ak47Bullet.massKg = 0.008f;               // 8 grams
    ak47Bullet.velocityVector = {0.0f, 0.0f, 715.0f}; // 715 meters/sec
    ak47Bullet.contactSurfaceRadius = 0.00762f; // Tiny localized projectile point

    // --- OBJECT 2: LOW VELOCITY, AVERAGE WEIGHT (Running Human Body) ---
    ImpactingObject runningHuman;
    runningHuman.objectName = "Full-Speed Sprinting Human Linebacker";
    runningHuman.massKg = 95.0f;               // 95 kilograms
    runningHuman.velocityVector = {8.5f, 0.0f, 0.0f};  // 8.5 meters/sec (~30 km/h)
    runningHuman.contactSurfaceRadius = 0.45f;  // Human torso contact shoulder width

    // --- OBJECT 3: HIGH VELOCITY, EXTREMELY HEAVY WEIGHT (Armored Tactical Truck) ---
    ImpactingObject tacticalTruck;
    tacticalTruck.objectName = "Swat Armored Vehicle Ramming Attack";
    tacticalTruck.massKg = 3800.0f;             // 3.8 Metric Tons (3800 kg)
    tacticalTruck.velocityVector = {0.0f, 0.0f, 22.2f}; // 22.2 meters/sec (~80 km/h)
    tacticalTruck.contactSurfaceRadius = 1.8f;  // Massive broad armored front bumper area

    // Execute sequential physical simulation trace events at different wall parts
    cityWall.ProcessMassivePhysicalImpact(ak47Bullet, {6.0f, 2.0f, 0.0f});
    cityWall.ProcessMassivePhysicalImpact(runningHuman, {3.0f, 1.5f, 0.0f});
    cityWall.ProcessMassivePhysicalImpact(tacticalTruck, {6.5f, 0.8f, 0.0f});

    return 0;
}
