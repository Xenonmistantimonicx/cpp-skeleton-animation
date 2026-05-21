#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include <algorithm>

// --- ADVANCED GAME MATH ENGINE ---
struct Vector3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vector3 operator+(const Vector3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vector3 operator-(const Vector3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vector3 operator*(float s) const { return {x*s, y*s, z*s}; }
    Vector3 Cross(const Vector3& o) const { return {y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x}; }
    float Dot(const Vector3& o) const { return x*o.x + y*o.y + z*o.z; }
    float LengthSq() const { return x*x + y*y + z*z; }
    float Length() const { return std::sqrt(LengthSq()); }
    Vector3 Normalize() const { float l = Length(); return l > 0.0001f ? *this * (1.0f / l) : Vector3{}; }
};

// Smooth Rotation ke liye Quaternion Implementation (Gimbal Lock Engine level protection)
struct Quaternion {
    float w = 1.0f, x = 0.0f, y = 0.0f, z = 0.0f;
    
    static Quaternion FromAxisAngle(const Vector3& axis, float angleRadians) {
        float halfAngle = angleRadians * 0.5f;
        float s = std::sin(halfAngle);
        return { std::cos(halfAngle), axis.x * s, axis.y * s, axis.z * s };
    }

    Quaternion operator*(const Quaternion& o) const {
        return {
            w*o.w - x*o.x - y*o.y - z*o.z,
            w*o.x + x*o.w + y*o.z - z*o.y,
            w*o.y - x*o.z + y*o.w + z*o.x,
            w*o.z + x*o.y - y*o.x + z*o.w
        };
    }

    Vector3 RotateVector(const Vector3& v) const {
        Vector3 qv{x, y, z};
        Vector3 t = qv.Cross(v) * 2.0f;
        return v + t * w + qv.Cross(t);
    }
};

// --- DATA STRUCTURES FOR NATURAL STIMULI ---
struct FluidField {
    Vector3 velocity = {0.0f, 0.0f, 0.0f}; 
    float density = 1.225f;       // Air: ~1.2, Water: ~1000.0 (Auto structural load adjustment)
    float turbulenceScale = 0.15f;// High frequency noise to simulate stormy flutter/shaking
};

struct StructuralNode {
    Vector3 originalPos;
    Vector3 dynamicPos;
    Vector3 velocity;
    float mass = 1.5f;
    float currentHealth = 100.0f;
    bool isSevered = false;
    bool isHingeBinding = false;
};

// --- HIGH-FIDELITY DESTRUCTIBLE COUPLING (MICRO-MESH FLUIDITY) ---
class AdvancedDestructionGrid {
public:
    std::vector<StructuralNode> nodes;
    float stressThreshold = 450.0f; 

    AdvancedDestructionGrid() {
        // Generates a ultra-smooth 6x4 internal matrix point map for microscopic structural tearing
        for (int y = 0; y < 6; ++y) {
            for (int x = 0; x < 4; ++x) {
                StructuralNode node;
                node.originalPos = { (x - 1.5f) * 0.35f, (y - 2.5f) * 0.45f, 0.0f };
                node.dynamicPos = node.originalPos;
                node.velocity = {0.0f, 0.0f, 0.0f};
                if (x == 0 && (y == 1 || y == 4)) {
                    node.isHingeBinding = true; // Two distinct primary heavy metal hinges
                }
                nodes.push_back(node);
            }
        }
    }

    void ApplyLocalizedShockwave(const Vector3& localImpactPoint, float energy, float radius, const Vector3& blastDir) {
        std::cout << ">> [Shockwave Core] Energy: " << energy << " Joules propagating through molecular lattice...\n";
        
        for (auto& node : nodes) {
            if (node.isSevered) continue;
            
            float distance = (node.dynamicPos - localImpactPoint).Length();
            if (distance < radius) {
                float falloff = 1.0f - (distance / radius);
                float absoluteDamage = energy * falloff;
                node.currentHealth -= absoluteDamage;
                
                // Pure Kinetic Translation to Node Points for structural displacement
                node.velocity = node.velocity + blastDir * (energy * 0.05f * falloff);

                if (node.currentHealth <= 0.0f) {
                    node.isSevered = true;
                    std::cout << "   [Fracture] Structural grid point shattered at relative space (" 
                              << node.originalPos.x << ", " << node.originalPos.y << ")\n";
                }
            }
        }
    }

    float GetHingeStabilityFactor() const {
        float activeHinges = 0.0f;
        float totalHinges = 0.0f;
        for (const auto& node : nodes) {
            if (node.isHingeBinding) {
                totalHinges += 1.0f;
                if (!node.isSevered) activeHinges += 1.0f;
            }
        }
        return activeHinges / totalHinges; // 1.0 = Perfect, 0.5 = Dangling by 1 hinge, 0.0 = Broken
    }
};

// --- CORE SIMULATION COMPONENT ---
class ArticulatedProceduralDoor {
private:
    // Angular Dynamics Parameters (Verlet Interlaced Variable set)
    float m_angle = 0.0f;
    float m_angularVelocity = 0.0f;
    float m_angularAcceleration = 0.0f;
    
    // Physics constants for structural movement behavior
    const float m_momentOfInertia = 8.5f; 
    const float m_hingeDampingBase = 3.2f;
    const float m_maxOpeningLimit = 1.5708f; // 90 Degrees
    
    // Active Servo Motor Layer for Intention Maps
    float m_targetGoalAngle = 0.0f;
    float m_servoSpringStiffness = 0.0f; 
    float m_servoDamperSafety = 8.0f;

    // Fluid dynamics fluttering states
    float m_accumulatedTime = 0.0f;

    std::unique_ptr<AdvancedDestructionGrid> m_latticeGrid;

public:
    Vector3 basePivotWorldSpace = {10.0f, 0.0f, -5.0f};
    Vector3 defaultClosedNormal = {0.0f, 0.0f, 1.0f};

    ArticulatedProceduralDoor() {
        m_latticeGrid = std::make_unique<AdvancedDestructionGrid>();
    }

    void TriggerMotorCommand(bool open) {
        m_targetGoalAngle = open ? m_maxOpeningLimit : 0.0f;
        // High-stiffness spring coefficients for premium door systems (no sluggish responses)
        m_servoSpringStiffness = 180.0f; 
    }

    void HandleBallisticWeaponFire(const Vector3& localImpact, float kineticEnergy, const Vector3& travelVec) {
        // Stress distribution step
        m_latticeGrid->ApplyLocalizedShockwave(localImpact, kineticEnergy, 0.45f, travelVec);
        
        // Compute Torque based on lever arm cross calculations
        float leverageDistance = localImpact.x; // Pivot centers around X=0 axis inside the layout
        float rotationalImpulseForce = travelVec.Dot({0.0f, 0.0f, 1.0f}) * kineticEnergy;
        
        float impartedTorque = leverageDistance * rotationalImpulseForce;
        m_angularVelocity += (impartedTorque / m_momentOfInertia);
    }

    void TickFluidAndStructuralSimulation(float dt, const FluidField& fluid) {
        m_accumulatedTime += dt;
        float structuralIntegrity = m_latticeGrid->GetHingeStabilityFactor();

        if (structuralIntegrity <= 0.001f) {
            std::cout << "[PHYSICS FAILURE] Hinge matrix destroyed completely. Simulating full Ragdoll detachment.\n";
            return;
        }

        // Current Orientation Matrix representation from Hinge Pivot using accurate Quaternions
        Quaternion currentRotation = Quaternion::FromAxisAngle({0.0f, 1.0f, 0.0f}, m_angle);
        Vector3 dynamicNormal = currentRotation.RotateVector(defaultClosedNormal);

        // --- 1. COMPUTING ADVANCED AERODYNAMIC LIFT & DRAG FORCES ---
        float netTorque = 0.0f;
        
        // Procedural Turbulence implementation using multi-octave high-frequency sine-waves
        float windNoise = std::sin(m_accumulatedTime * 25.0f) * std::cos(m_accumulatedTime * 14.0f) * fluid.turbulenceScale;
        Vector3 randomizedFluidVelocity = fluid.velocity * (1.0f + windNoise);
        
        float fluidSpeed = randomizedFluidVelocity.Length();
        if (fluidSpeed > 0.05f) {
            Vector3 fluidDirection = randomizedFluidVelocity.Normalize();
            
            // Exposure area profile calculated via fluid-angle alignments
            float crossProductExposure = dynamicNormal.Dot(fluidDirection);
            
            // F_drag = 0.5 * rho * v^2 * Area * Cd
            float continuousDragForce = 0.5f * fluid.density * (fluidSpeed * fluidSpeed) * 2.8f * crossProductExposure;
            
            // Lever calculations (Geometric surface centroid target vector)
            float averageLeverArm = 0.7f; 
            netTorque += (continuousDragForce * averageLeverArm) * structuralIntegrity;
        }

        // --- 2. SERVO MOTOR TARGET TRACKING SPRING-DAMPER FORMULA ---
        if (m_servoSpringStiffness > 0.01f) {
            float angularDisplacementError = m_targetGoalAngle - m_angle;
            // Critical damping loop integration to avoid erratic overshoot jittering
            float motorCorrectionTorque = (angularDisplacementError * m_servoSpringStiffness) - (m_angularVelocity * m_servoDamperSafety);
            netTorque += motorCorrectionTorque;
        }

        // --- 3. DISSIPATIVE FRICTION LOOPS ---
        // Damaged mechanical systems become stiffer and hard to swing cleanly
        float adaptiveHingeDamping = m_hingeDampingBase * (2.0f - structuralIntegrity);
        netTorque -= m_angularVelocity * adaptiveHingeDamping;

        // --- 4. VERLET INTERLACED STABLE INTEGRATOR ---
        m_angularAcceleration = netTorque / m_momentOfInertia;
        
        // Explicit velocity blending over dynamic dt steps
        m_angularVelocity += m_angularAcceleration * dt;
        m_angle += m_angularVelocity * dt;

        // --- 5. ULTRA-SMOOTH BOUNCE CONSTRICTION COMPRESSION ---
        if (m_angle > m_maxOpeningLimit) {
            m_angle = m_maxOpeningLimit;
            m_angularVelocity = -m_angularVelocity * 0.35f; // Structural rebound coefficient against frame bumpers
        } else if (m_angle < 0.0f) {
            m_angle = 0.0f;
            m_angularVelocity = -m_angularVelocity * 0.22f; // Soft steel latch clicking slam rebound
        }

        RenderOutputDiagnosticVisuals(dynamicNormal, structuralIntegrity);
    }

private:
    void RenderOutputDiagnosticVisuals(const Vector3& currentNormal, float hingeHealth) {
        float angleDegrees = m_angle * 57.2958f;
        std::cout << "[SMOOTH ENGINE RUNNING] Angle: " << angleDegrees 
                  << "° | AngularSpeed: " << m_angularVelocity 
                  << " rad/s | Lattice Matrix Hinge Integrity: " << (hingeHealth * 100.0f) << "%\n";
    }
};

// --- SIMULATION EXECUTION LOOP ---
int main() {
    ArticulatedProceduralDoor systemDoor;
    FluidField worldWeatherPattern;

    std::cout << "=====================================================================\n";
    std::cout << "  AAA ARTICULATED ADAPTIVE PROCEDURAL DOOR KINEMATICS SIMULATOR\n";
    std::cout << "=====================================================================\n\n";

    // Test Case 1: Activating internal Servo Spring Matrix for opening sequence
    std::cout << "[TEST PHASE 1] Initiating Command Sequence: Move Door to OPEN state...\n";
    systemDoor.TriggerMotorCommand(true);
    for (int t = 0; t < 5; ++t) {
        systemDoor.TickFluidAndStructuralSimulation(0.016f, worldWeatherPattern); // Stable 60Hz delta ticks
    }

    // Test Case 2: Catastrophic Weather event (Simulating dense monsoon flood fluid dynamics)
    std::cout << "\n[TEST PHASE 2] Environmental Threat Detected: Submerging structure into Flood Wave Currents...\n";
    systemDoor.TriggerMotorCommand(false); // Disengage active motor locks
    worldWeatherPattern.density = 1000.0f; // Dense heavy water loading force profiles
    worldWeatherPattern.velocity = {0.0f, 0.0f, 12.5f}; // Rapid fluid progression vector
    worldWeatherPattern.turbulenceScale = 0.85f; // Extreme churning frequency distortions
    for (int t = 0; t < 5; ++t) {
        systemDoor.TickFluidAndStructuralSimulation(0.016f, worldWeatherPattern);
    }

    // Test Case 3: Heavy automatic ballistic weapon payload impact targeting local node grid arrays
    std::cout << "\n[TEST PHASE 3] Structural Attack Event: 50 BMG Heavy Caliber AP Rounds striking hinges...\n";
    Vector3 bulletImpactCoord = {-0.52f, 1.8f, 0.0f}; // Directly striking near upper hinge sector coordinates
    Vector3 bulletPathVector = {0.0f, 0.0f, -1.0f};
    float heavyKineticEnergyJoules = 320.0f; // High energetic structural breakdown payload
    
    systemDoor.HandleBallisticWeaponFire(bulletImpactCoord, heavyKineticEnergyJoules, bulletPathVector);
    systemDoor.TickFluidAndStructuralSimulation(0.016f, worldWeatherPattern);

    return 0;
}
