#include <iostream>
#include <cmath>
#include <vector>

struct Particle { float x, y, z; float fx, fy, fz; };

class StormSimulation {
private:
    // Simple pseudo-random hash to simulate turbulent spatial noise
    float noise3D(float x, float y, float z) {
        float s = std::sin(x * 12.9898f + y * 78.233f + z * 37.719f) * 43758.5453f;
        return s - std::floor(s);
    }

public:
    void applyStormForces(std::vector<Particle>& clothGrid, float time) {
        float stormIntensity = 25.0f; // High velocity storm

        for (auto& p : clothGrid) {
            // Generate chaotic, evolving spatial wind vectors
            float nx = noise3D(p.x * 0.1f, time, 0.0f) - 0.5f;
            float ny = noise3D(0.0f, p.y * 0.1f, time) - 0.5f;
            float nz = noise3D(time, 0.0f, p.z * 0.1f) - 0.5f;

            // Base storm direction (e.g., heavy gale heading in positive X) + turbulence
            float windX = 15.0f + (nx * stormIntensity);
            float windY = -2.0f + (ny * stormIntensity * 0.5f); // Downdrafts
            float windZ = 5.0f  + (nz * stormIntensity);

            // Apply direct force burst
            p.fx += windX * 0.8f;
            p.fy += windY * 0.8f;
            p.fz += windZ * 0.8f;
        }
    }
};
