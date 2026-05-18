#include <iostream>
#include <cmath>
#include <vector>

struct Particle {
    float x, y, z;
    float vx, vy, vz;
    float fx, fy, fz;
};

class GustyCloth {
private:
    std::vector<Particle> mesh;
    float elapsedTime = 0.0f;

public:
    void initMesh() {
        mesh.push_back({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f});
    }

    void applyGustyWind(float dt) {
        elapsedTime += dt;

        // Base wind speed + sinusoidal variation to create a "gust" effect
        float baseWindX = 8.0f;
        float gustX = baseWindX + 4.0f * std::sin(2.0f * elapsedTime) * std::cos(0.5f * elapsedTime);
        float gustZ = 2.0f * std::sin(3.0f * elapsedTime); 

        for (auto& p : mesh) {
            // Apply simple drag relative to the gusting wind
            float drag = 0.3f;
            p.fx += drag * (gustX - p.vx);
            p.fz += drag * (gustZ - p.vz);
        }
    }
};
