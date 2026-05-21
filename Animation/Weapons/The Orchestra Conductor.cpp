#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>

// Humne jo Window aur Door ke physics calculations likhe the, unhe ab abstract "PhysicsComponent" bana diya
class PhysicsComponent {
public:
    virtual void ApplyFluidForces(const struct Vector3D& fluidVel, float density) = 0;
    virtual void TakeBallisticDamage(const struct Vector3D& impactPoint, float energy) = 0;
    virtual void TickPhysics(float dt) = 0;
    virtual ~PhysicsComponent() = default;
};

// --- DATA STRUCTURES ---
struct Vector3D { float x, y, z; };

struct BlastWave {
    Vector3D epicentre;
    float energyJoules;
    float radius;
};

// --- 1. GLOBAL WIND/FLUID VECTOR FIELD ---
// Ye system puri duniya me storm ya pani ke bahaav ki real-time map database rakhta hai
class GlobalFluidField {
public:
    Vector3D GetFluidVelocityAtPoint(const Vector3D& worldPos) {
        // Real game me yahan ek 3D Noise map (Perlin/Simplex Noise) chalta hai
        // Abhi hum test storm velocity return kar rahe hain
        return { 0.0f, 0.0f, 35.0f }; // 35 m/s storm striking the entire area
    }
};

// --- 2. THE WORLD ENTITY & SPATIAL MANAGER ---
// Ye manager sab kuch cover karta hai: Saare objects, weather aur damage updates isi ke through routing hote hain
class WorldSimulationManager {
private:
    std::vector<std::shared_ptr<PhysicsComponent>> m_activeWorldObjects;
    GlobalFluidField m_fluidField;

public:
    void RegisterObjectInWorld(std::shared_ptr<PhysicsComponent> obj) {
        m_activeWorldObjects.push_back(obj);
    }

    // PURE ENVIRONMENT CONTROL: Ek hi jhatke me puri duniya ke physical structures par asar padta hai
    void UpdateGlobalWeatherAndPhysics(float deltaTime) {
        for (auto& obj : m_activeWorldObjects) {
            // Fake position passing, real system uses component transform matrix
            Vector3D dummyPos = { 10.0f, 0.0f, 5.0f }; 
            
            Vector3D localWind = m_fluidField.GetFluidVelocityAtPoint(dummyPos);
            
            // Har object apni internal geometry ke hisab se hawa ko calculate karega
            obj->ApplyFluidForces(localWind, 1.225f); 
            obj->TickPhysics(deltaTime);
        }
    }

    // GLOBAL DESTRUCTION PROPAGATION: Bomb fata to uske radius me aane wali har khidki/door tootegi
    void TriggerExplosionShockwave(const BlastWave& blast) {
        std::cout << "\n[GLOBAL WORLD MANAGER] Explosion Triggered! Radiant energy: " 
                  << blast.energyJoules << " Joules.\n";
        
        for (auto& obj : m_activeWorldObjects) {
            // Yahan Spatial Partitioning (Octree) check hota hai ki object blast radius me hai ya nahi
            // Agar radius ke andar hai, to impact pass hoga
            Vector3D dummyImpactPoint = { 0.0f, 0.0f, 0.0f };
            obj->TakeBallisticDamage(dummyImpactPoint, blast.energyJoules);
        }
    }
};

// --- SYSTEM MODULE COUPLING TEST ---
class GameWindowComponent : public PhysicsComponent {
public:
    void ApplyFluidForces(const Vector3D& vel, float den) override {
        std::cout << " -> Window processing aerodynamics at " << vel.z << " m/s\n";
    }
    void TakeBallisticDamage(const Vector3D& pt, float energy) override {
        std::cout << " -> Glass element evaluating fracturing threshold from blast wave energy!\n";
    }
    void TickPhysics(float dt) override {}
};

int main() {
    WorldSimulationManager globalWorldEngine;

    // Duniya me multiple objects register karo
    auto frontWindow = std::make_shared<GameWindowComponent>();
    globalWorldEngine.RegisterObjectInWorld(frontWindow);

    std::cout << "=== GLOBAL SIMULATION TICK STARTING ===\n";
    // 1. Weather Update Cycle
    globalWorldEngine.UpdateGlobalWeatherAndPhysics(0.016f);

    // 2. Weapon / Hazard Impact Cycle
    BlastWave rpgImpact = { {10.0f, 2.0f, 5.0f}, 4500.0f, 5.0f };
    globalWorldEngine.TriggerExplosionShockwave(rpgImpact);

    return 0;
}
