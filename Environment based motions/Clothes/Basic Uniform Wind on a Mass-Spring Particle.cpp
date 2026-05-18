#include <iostream>

struct Vector3 {
    float x, y, z;
    Vector3 operator+(const Vector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
};

struct Particle {
    Vector3 position;
    Vector3 velocity;
    Vector3 force;
    float mass;
    bool isStatic; // e.g., pinned corners of a cape or flag
};

class ClothSimulation {
public:
    Particle particle;
    Vector3 windVelocity = {5.0f, 0.0f, 2.0f; // Uniform constant wind

    void init() {
        particle = {{0.0f, 5.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1.0f, false};
    }

    void applyForces() {
        particle.force = {0.0f, -9.81f * particle.mass, 0.0f}; // Gravity

        if (!particle.isStatic) {
            // Simple aerodynamic drag approximation: F = c * (V_wind - V_particle)
            float dragCoeff = 0.5f;
            Vector3 relativeVelocity = windVelocity + (particle.velocity * -1.0f);
            Vector3 windForce = relativeVelocity * dragCoeff; 
            
            particle.force += windForce;
        }
    }

    void update(float dt) {
        if (particle.isStatic) return;

        // Explicit Euler Integration
        Vector3 acceleration = particle.force * (1.0f / particle.mass);
        particle.velocity += acceleration * dt;
        particle.position += particle.velocity * dt;
    }
};

int main() {
    ClothSimulation sim;
    sim.init();
    for (int i = 0; i < 5; ++i) {
        sim.applyForces();
        sim.update(0.016f); // ~60 FPS
        std::cout << "Step " << i << " Position: (" << sim.particle.position.x << ", " << sim.particle.position.y << ")\n";
    }
    return 0;
}
