#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include <algorithm>

// --- CORE INDUSTRIAL MATH ENGINE ---
struct Vector3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vector3 operator+(const Vector3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vector3 operator-(const Vector3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vector3 operator*(float s) const { return {x*s, y*s, z*s}; }
    Vector3 Cross(const Vector3& o) const { return {y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x}; }
    float Dot(const Vector3& o) const { return x*o.x + y*o.y + z*o.z; }
    float Length() const { return std::sqrt(x*x + y*y + z*z); }
    Vector3 Normalize() const { float l = Length(); return l > 0.001f ? *this * (1.0f / l) : Vector3{}; }
};

// Gimbal lock isolation layer
struct Quaternion {
    float w = 1.0f, x = 0.0f, y = 0.0f, z = 0.0f;
    static Quaternion FromAxisAngle(const Vector3& axis, float rad) {
        float h = rad * 0.5f; float s = std::sin(h);
        return { std::cos(h), axis.x * s, axis.y * s, axis.z * s };
    }
    Vector3 Rotate(const Vector3& v) const {
        Vector3 qv{x, y, z}; Vector3 t = qv.Cross(v) * 2.0f;
        return v + t * w + qv.Cross(t);
    }
};

// --- ENVIRONMENT & STRESS COEFFICIENTS ---
struct WeatherCondition {
    Vector3 fluidVelocity = {0.0f, 0.0f, 0.0f}; // Wind/Storm speeds
    float barometricPressureDiff = 0.0f;       // Inside vs Outside pressure differential (Pascals)
    float structuralVibrationHz = 0.0f;        // Resonance from explosions or earthquakes
};

// --- GLASS BREAKAGE GRID SUB-SYSTEM ---
struct GlassFragment {
    Vector3 offsetPos;
    float mechanicalStress = 0.0f;
    float maxStressLimit = 180.0f;            // Glass yields instantly at peak threshold
    bool isShattered = false;
};

class BrittleGlassPane {
public:
    std::vector<GlassFragment> grid;
    float overallGlassThickness = 0.005f;     // 5mm residential grade standard glass
    bool entirePaneDropped = false;

    BrittleGlassPane() {
        // Generates a 4x4 high-fidelity grid representing localized breakable glass sections
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                GlassFragment frag;
                frag.offsetPos = {(x - 1.5f) * 0.25f, (y - 1.5f) * 0.35f, 0.0f};
                grid.push_back(frag);
            }
        }
    }

    void HandleBallisticPenetration(const Vector3& localImpact, float kineticEnergy) {
        if (entirePaneDropped) return;

        std::cout << ">> [Ballistic Impact] Bullet energy: " << kineticEnergy << " J tearing through glass panel.\n";
        
        for (auto& frag : grid) {
            if (frag.isShattered) continue;

            float dist = (frag.offsetPos - localImpact).Length();
            // Localized kinetic energy dispersion matrix
            float stressIncurred = (kineticEnergy / (dist + 0.1f)) * 0.25f;
            frag.mechanicalStress += stressIncurred;

            if (frag.mechanicalStress >= frag.maxStressLimit) {
                frag.isShattered = true;
                std::cout << "   -> [Glass Shattered] Shard displaced at coordinate: (" 
                          << frag.offsetPos.x << ", " << frag.offsetPos.y << ")\n";
            }
        }
        EvaluateStructuralLattice();
    }

    void EvaluatePressureLoad(float pressurePascals, float frameAngle) {
        if (entirePaneDropped) return;

        // Angle scale factor determines exposure surface profile (90 deg open windows take zero load)
        float exposureFactor = std::cos(frameAngle); 
        float effectiveLoad = std::abs(pressurePascals) * exposureFactor;

        if (effectiveLoad > 850.0f) { // 850 Pa is peak structural rating limit for thin glass
            std::cout << ">> [CRITICAL PRESSURE SHATTER] Wind/Blast load reached " << effectiveLoad << " Pa! Glass pane blown out!\n";
            for (auto& frag : grid) {
                frag.isShattered = true;
            }
            entirePaneDropped = true;
        }
    }

    float GetGlassWeightMultiplier() const {
        float remainingMass = 0.0f;
        for (const auto& frag : grid) {
            if (!frag.isShattered) remainingMass += 1.0f;
        }
        return remainingMass / 16.0f; // Structural mass ratio remaining inside the frame
    }

private:
    void EvaluateStructuralLattice() {
        int brokenCount = 0;
        for (const auto& frag : grid) { if (frag.isShattered) brokenCount++; }
        if (brokenCount >= 12) { // 75% of the glass grid is gone
            entirePaneDropped = true;
            std::cout << "   -> [Lattice Collapse] Remaining glass shards lose structural holding. Entire pane drops out.\n";
        }
    }
};

// --- MAIN PROCEDURAL WINDOW CLASS ---
class ArticulatedProceduralWindow {
private:
    // Physical Kinematics (Hinged Rotational States)
    float m_angle = 0.0f;
    float m_angularVelocity = 0.0f;
    float m_angularAcceleration = 0.0f;
    
    const float m_frameInertiaBase = 4.2f;
    const float m_hingeDamping = 1.8f;
    const float m_maxLatchLimit = 1.3962f; // 80 Degrees max out swinging limit

    // Window latch configurations
    bool m_isLocked = true;
    float m_targetAngleGoal = 0.0f;
    float m_actuatorForce = 0.0f;

    std::unique_ptr<BrittleGlassPane> m_glassComponent;

public:
    Vector3 windowNormal = {0.0f, 0.0f, 1.0f};

    ArticulatedProceduralWindow() {
        m_glassComponent = std::make_unique<BrittleGlassPane>();
    }

    void SetLatchLock(bool lock) {
        m_isLocked = lock;
        std::cout << "[Window Mechanical Status] Latch lock state set to: " << (lock ? "LOCKED" : "UNLOCKED") << "\n";
    }

    void SetActuatorTarget(bool open) {
        if (m_isLocked) {
            std::cout << "[Warning] Cannot actuate motorized window, mechanical latch lock is engaged!\n";
            return;
        }
        m_targetAngleGoal = open ? m_maxLatchLimit : 0.0f;
        m_actuatorForce = 85.0f; // Automated mechanical arm spring tension
    }

    void ProcessImpactTrace(const Vector3& localPos, float kineticEnergy, const Vector3& velocityVec) {
        // 1. Pass impact to breakable glass layers
        m_glassComponent->HandleBallisticPenetration(localPos, kineticEnergy);

        // 2. Angular impulse transfer to frame rotation loop
        if (!m_isLocked) {
            float leverArm = localPos.x;
            float projectedImpactForce = velocityVec.Dot({0.0f, 0.0f, 1.0f}) * kineticEnergy;
            float rawTorque = leverArm * projectedImpactForce;
            m_angularVelocity += (rawTorque / m_frameInertiaBase);
        } else if (kineticEnergy > 500.0f) {
            // High magnitude blast snaps the structural lock pin completely
            m_isLocked = false;
            std::cout << "[STRUCTURAL DAMAGE] Mechanical lock pin snapped due to high-energy ballistic load!\n";
        }
    }

    void SimulationTick(float dt, const WeatherCondition& weather) {
        // Process glass status changes via weather limits
        m_glassComponent->EvaluatePressureLoad(weather.barometricPressureDiff, m_angle);
        float remainingMassScale = m_glassComponent->GetGlassWeightMultiplier();

        float calculatedInertia = m_frameInertiaBase * (0.4f + 0.6f * remainingMassScale);
        float combinedTorque = 0.0f;

        if (!m_isLocked) {
            // --- 1. AERODYNAMIC LIFT AND DRAG ON OPEN FRAME VALVE ---
            float fluidSpeed = weather.fluidVelocity.Length();
            if (fluidSpeed > 0.1f) {
                Vector3 fluidDir = weather.fluidVelocity.Normalize();
                float exposureScale = windowNormal.Dot(fluidDir);
                
                // Aerodynamic drag formulation tracking residual surface scale metrics
                float aerodynamicForce = 0.5f * 1.225f * (fluidSpeed * fluidSpeed) * (1.2f * remainingMassScale) * exposureScale;
                combinedTorque += aerodynamicForce * 0.45f; // Lever center calculation
            }

            // --- 2. ACTUATOR AUTOMATION INTERPOLATION TRACK ---
            if (m_actuatorForce > 0.01f) {
                float positionError = m_targetAngleGoal - m_angle;
                combinedTorque += (positionError * m_actuatorForce) - (m_angularVelocity * 4.5f);
            }

            // --- 3. SEISMIC RESONANCE SHAKING LOOPS ---
            if (weather.structuralVibrationHz > 0.0f) {
                float resonanceFlutter = std::sin(weather.structuralVibrationHz * 6.28f * dt) * 15.0f;
                combinedTorque += resonanceFlutter;
            }

            // Hinge damping dissipation
            combinedTorque -= m_angularVelocity * m_hingeDamping;

            // --- 4. NUMERICAL INTEGRATION BLOCK ---
            m_angularAcceleration = combinedTorque / calculatedInertia;
            m_angularVelocity += m_angularAcceleration * dt;
            m_angle += m_angularVelocity * dt;

            // --- 5. CORNER CONTACT CONSTRAINTS (BOUNDS CHECKING) ---
            if (m_angle > m_maxLatchLimit) {
                m_angle = m_maxLatchLimit;
                m_angularVelocity = -m_angularVelocity * 0.4f; // Frame bounce profile against brick limits
            } else if (m_angle < 0.0f) {
                m_angle = 0.0f;
                m_angularVelocity = -m_angularVelocity * 0.25f; // Slam shut shock feedback loop
            }
        }

        // Dynamically rotate normal vectors using active Quaternion components
        Quaternion currentRotation = Quaternion::FromAxisAngle({0.0f, 1.0f, 0.0f}, m_angle);
        windowNormal = currentRotation.Rotate({0.0f, 0.0f, 1.0f});

        PrintDiagnosticTelemetry();
    }

private:
    void PrintDiagnosticTelemetry() {
        std::cout << "[WINDOW ENGINE] Angle: " << (m_angle * 57.2958f) << "° | "
                  << "Angular Velocity: " << m_angularVelocity << " rad/s | "
                  << "Normal Vector: (" << windowNormal.x << ", " << windowNormal.z << ")\n";
    }
};

// --- ENGINE EXECUTION PATH ---
int main() {
    ArticulatedProceduralWindow gameWindow;
    WeatherCondition environmentMetrics;

    std::cout << "=====================================================================\n";
    std::cout << "  SYSTEM SIMULATION: DYNAMIC WINDOW & SHATTERABLE GLASS SYSTEM\n";
    std::cout << "=====================================================================\n\n";

    // Scenario 1: Standard open command execution loop without unlocking
    std::cout << "--- PHASE 1: ATTEMPTING MECHANICAL OPERATION WHILE LOCKED ---\n";
    gameWindow.SetActuatorTarget(true);
    gameWindow.SimulationTick(0.016f, environmentMetrics);

    // Scenario 2: Unlocking latch assembly and engaging motor drivers
    std::cout << "\n--- PHASE 2: UNLOCKING LATCH AND ACTUATING ---\n";
    gameWindow.SetLatchLock(false);
    gameWindow.SetActuatorTarget(true);
    for (int i = 0; i < 3; ++i) {
        gameWindow.SimulationTick(0.016f, environmentMetrics);
    }

    // Scenario 3: High energetic ballistic bullet impacts hitting local glass pane points
    std::cout << "\n--- PHASE 3: WEAPON ATTACK MATRIX ENGAGED ---\n";
    Vector3 bulletImpactCoord = {0.25f, -0.35f, 0.0f}; // Local coordinate hits lower right section
    Vector3 bulletPathVec = {0.0f, 0.0f, -1.0f};
    float muzzleKineticEnergy = 450.0f; // High tier standard kinetic weapon round
    
    gameWindow.ProcessImpactTrace(bulletImpactCoord, muzzleKineticEnergy, bulletPathVec);
    gameWindow.SimulationTick(0.016f, environmentMetrics);

    // Scenario 4: Supercritical atmospheric blast shockwave triggers (Pressure breakage)
    std::cout << "\n--- PHASE 4: HIGH ATMOSPHERIC STORM / BLAST PRESSURE WAVE ---\n";
    environmentMetrics.barometricPressureDiff = 1200.0f; // Massive pressure spike exceeding 850 Pa threshold
    gameWindow.SimulationTick(0.016f, environmentMetrics);

    return 0;
}
