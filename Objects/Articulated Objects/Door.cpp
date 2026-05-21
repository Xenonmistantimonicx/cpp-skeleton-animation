#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include <algorithm>

// --- MATH & UTILITY STRUCTURES ---
struct Vector3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vector3 operator+(const Vector3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vector3 operator-(const Vector3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vector3 operator*(float scalar) const { return {x*scalar, y*scalar, z*scalar}; }
    float Length() const { return std::sqrt(x*x + y*y + z*z); }
    Vector3 Normalize() const { float l = Length(); return l > 0.001f ? *this * (1.0f / l) : Vector3{}; }
    static float Dot(const Vector3& a, const Vector3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
};

struct DamageImpact {
    Vector3 location;
    Vector3 direction;
    float force;
    float radius;
};

enum class DoorState { Closed, Opening, Open, Closing, Broken };

// --- STRUCTURAL DESTRUCTION SUB-SYSTEM ---
struct DoorMaterialNode {
    Vector3 relativePosition;
    float health = 100.0f;
    bool isIntact = true;
    bool isHingeAnchor = false;
};

class DestructibleDoorPanel {
public:
    int gridRows = 5;
    int gridCols = 3;
    std::vector<DoorMaterialNode> structuralGrid;

    DestructibleDoorPanel() {
        // Initialize structural integrity grid (e.g., a 5x3 grid of breakable nodes)
        for (int r = 0; r < gridRows; ++r) {
            for (int c = 0; c < gridCols; ++c) {
                DoorMaterialNode node;
                node.relativePosition = { (c - 1.0f) * 0.4f, (r - 2.5f) * 0.5f, 0.0f };
                // Leftmost column anchors to the frame hinges
                if (c == 0) {
                    node.isHingeAnchor = true;
                }
                structuralGrid.push_back(node);
            }
        }
    }

    void ProcessImpact(const DamageImpact& impact) {
        std::cout << "[Destruction] Impact processed at (" << impact.location.x 
                  << ", " << impact.location.y << ") with force: " << impact.force << " N\n";

        int brokenNodesThisFrame = 0;
        for (auto& node : structuralGrid) {
            if (!node.isIntact) continue;

            // Simple distance check from impact vector to node position
            float distance = (node.relativePosition - impact.location).Length();
            if (distance <= impact.radius) {
                // Falloff damage based on distance
                float damageDealt = impact.force * (1.0f - (distance / impact.radius));
                node.health -= damageDealt;

                if (node.health <= 0.0f) {
                    node.isIntact = false;
                    brokenNodesThisFrame++;
                    // Spawn physical debris chunk
                    SpawnDebrisChunk(node.relativePosition, impact.direction * (impact.force * 0.1f));
                }
            }
        }
        if (brokenNodesThisFrame > 0) {
            EvaluateStructuralIntegrity();
        }
    }

    bool CheckHingeConnection() const {
        // If all hinge anchor nodes are destroyed, the entire door collapses off its hinges
        for (const auto& node : structuralGrid) {
            if (node.isHingeAnchor && node.isIntact) {
                return true; 
            }
        }
        return false;
    }

private:
    void EvaluateStructuralIntegrity() {
        if (!CheckHingeConnection()) {
            std::cout << "[ALERT] Structural Integrity Failure: All hinges severed! Door is falling.\n";
        }
    }

    void SpawnDebrisChunk(const Vector3& pos, const Vector3& velocity) {
        std::cout << "  -> Spawning physical debris fragment at relative pos (" 
                  << pos.x << ", " << pos.y << ") Velocity: " << velocity.length << "\n";
    }
};

// --- ENVIRONMENTAL FLUID SIMULATOR ---
struct EnvironmentalState {
    Vector3 windVelocity = { 0.0f, 0.0f, 0.0f }; // Atmospheric storm forces
    Vector3 waterCurrent = { 0.0f, 0.0f, 0.0f };  // Hydrodynamic flooding forces
    float fluidDensity = 1.225f;                  // Changes dynamically if submerged
};

// --- CORE ARTICULATION ENGINE ---
class ArticulatedPhysicalDoor {
private:
    // Physical state
    float m_currentAngle = 0.0f;       // Radians
    float m_angularVelocity = 0.0f;   // Rad/s
    float m_angularAcceleration = 0.0f; // Rad/s^2
    
    // Properties
    const float m_inertia = 12.0f;       // Moment of inertia ($I = \frac{1}{3} m w^2$)
    const float m_damping = 4.5f;        // Mechanical friction in hinge
    const float m_maxAngle = 1.5708f;    // 90 degrees in radians
    const float m_minAngle = 0.0f;       // Fully closed
    
    // Motor controllers for intentional opening/closing
    DoorState m_targetState = DoorState.Closed;
    float m_motorTargetAngle = 0.0f;
    float m_motorStrength = 0.0f;        // Servo torque variable

    std::unique_ptr<DestructibleDoorPanel> m_destructionSystem;

public:
    Vector3 doorNormal = { 0.0f, 0.0f, 1.0f }; // Normal vector facing outward when closed

    ArticulatedPhysicalDoor() {
        m_destructionSystem = std::make_unique<DestructibleDoorPanel>();
    }

    void SetIntentionalState(DoorState state) {
        m_targetState = state;
        if (state == DoorState::Opening) {
            m_motorTargetAngle = m_maxAngle;
            m_motorStrength = 45.0f; // Newton-meters of servo force
        } else if (state == DoorState::Closing) {
            m_motorTargetAngle = m_minAngle;
            m_motorStrength = 45.0f;
        } else {
            m_motorStrength = 0.0f; // Motor disengaged
        }
    }

    void InjectWeaponDamage(const DamageImpact& impact) {
        m_destructionSystem->ProcessImpact(impact);
        
        // Transfer physical kinetic impulse from bullet directly to the hinge's angular momentum
        Vector3 leverageArm = impact.location; // Lever arm vector from hinge center (0,0,0)
        Vector3 forceVector = impact.direction * impact.force;
        
        // Torque = r x F (simplified projection here around the hinge Y-axis pivot)
        float appliedTorque = leverageArm.x * forceVector.z - leverageArm.z * forceVector.x;
        m_angularVelocity += (appliedTorque / m_inertia);
    }

    void Update(float deltaTime, const EnvironmentalState& environment) {
        if (!m_destructionSystem->CheckHingeConnection()) {
            m_targetState = DoorState::Broken;
            std::cout << "[Simulation] Door completely unhinged. Executing freefall rigid body simulation.\n";
            return;
        }

        // 1. Calculate Motor Torque (T_motor = Kp * error)
        float totalTorque = 0.0f;
        if (m_motorStrength > 0.0f) {
            float angleError = m_motorTargetAngle - m_currentAngle;
            totalTorque += angleError * m_motorStrength;
        }

        // 2. Calculate Environmental Drag and Aerodynamic Torques
        // Wind/Water force relies on the cross section exposure relative to the door angle
        Vector3 combinedFluidVelocity = environment.windVelocity + environment.waterCurrent;
        float fluidSpeed = combinedFluidVelocity.Length();
        
        if (fluidSpeed > 0.01f) {
            Vector3 fluidDir = combinedFluidVelocity.Normalize();
            // Calculate effective surface area exposed to fluid direction vector
            float exposureScale = Vector3::Dot(doorNormal, fluidDir); 
            
            // Aerodynamic drag force formula: F = 0.5 * rho * v^2 * A * Cd
            float dragForce = 0.5f * environment.fluidDensity * (fluidSpeed * fluidSpeed) * 2.0f * exposureScale;
            
            // Assume force applies to geometric center of the panel (lever arm = 0.6m)
            float envTorque = dragForce * 0.6f;
            totalTorque += envTorque;
        }

        // 3. Apply Internal Hinge Friction/Damping Torque
        totalTorque -= m_angularVelocity * m_damping;

        // 4. Rigid Body Numerical Integration (Euler-Cromer)
        m_angularAcceleration = totalTorque / m_inertia;
        m_angularVelocity += m_angularAcceleration * deltaTime;
        m_currentAngle += m_angularVelocity * deltaTime;

        // 5. Hard Point Contact Constraints (Hitting door frame anchors)
        if (m_currentAngle >= m_maxAngle) {
            m_currentAngle = m_maxAngle;
            m_angularVelocity = -m_angularVelocity * 0.2f; // Bounce coefficient on stop
        } else if (m_currentAngle <= m_minAngle) {
            m_currentAngle = m_minAngle;
            m_angularVelocity = -m_angularVelocity * 0.1f; // Clanging shut bounce
        }

        // Update dynamic normal vector based on rotation matrix configuration around Pivot
        doorNormal.x = std::sin(m_currentAngle);
        doorNormal.z = std::cos(m_currentAngle);

        PrintTelemetry();
    }

private:
    void PrintTelemetry() {
        std::cout << "[Telemetry] Angle: " << (m_currentAngle * 57.2958f) << " deg | "
                  << "Vel: " << m_angularVelocity << " rad/s | "
                  << "Normal: (" << doorNormal.x << ", " << doorNormal.z << ")\n";
    }
};

// --- SIMULATION EXECUTION LOOP ---
int main() {
    ArticulatedPhysicalDoor processingDoor;
    EnvironmentalState worldEnvironment;
    
    // Test Scenario 1: Open the door under motor control
    std::cout << "--- SCENARIO 1: OPENING INTERACTIVE DOOR ---\n";
    processingDoor.SetIntentionalState(DoorState::Opening);
    for (int step = 0; step < 3; ++step) {
        processingDoor.Update(0.1f, worldEnvironment);
    }

    // Test Scenario 2: High velocity cross-wind storm strikes the door
    std::cout << "\n--- SCENARIO 2: SUDDEN TORNADIC WIND BLOWS ---\n";
    processingDoor.SetIntentionalState(DoorState::Closed); // Disengage active holding patterns
    worldEnvironment.windVelocity = { 0.0f, 0.0f, 45.0f }; // 45 m/s gale winds striking door faces
    for (int step = 0; step < 3; ++step) {
        processingDoor.Update(0.1f, worldEnvironment);
    }

    // Test Scenario 3: Heavy Weapon damage punctures and structurally targets hinges
    std::cout << "\n--- SCENARIO 3: HIGH CALIBER WEAPON IMPACT AT HINGE PIVOT ---\n";
    DamageImpact bulletImpact;
    bulletImpact.location = { -1.0f, 1.2f, 0.0f }; // Directly on left frame hinge bounds
    bulletImpact.direction = { 0.0f, 0.0f, -1.0f };
    bulletImpact.force = 150.0f;                   // Massive kinetic punch
    bulletImpact.radius = 0.3f;                    // Localized destruction radius
    
    processingDoor.InjectWeaponDamage(bulletImpact);
    processingDoor.Update(0.1f, worldEnvironment);

    return 0;
}
