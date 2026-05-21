#include "raylib.h"
#include <cmath>
#include <vector>
#include <string>
#include <memory>

// =====================================================================================
// DATA STRUCTURES FOR COSMIC SIMULATION
// =====================================================================================
struct QuantumStar {
    Vector2 coordinates;
    float   nuclearRadius;
    float   luminosityAlpha;
    float   scintillationPhase;  // Independent sine entry point
    float   scintillationSpeed;  // Frequency of twinkle variance
    int     parallaxLayerId;     // 0 = Deep Space, 1 = Mid Sky, 2 = Near Canopy
    Color   spectralTint;        // Temperature color (Blue-white, Pure White, Light Yellow)
};

struct ShootingStar {
    Vector2 structuralPosition;
    Vector2 trajectoryVelocity;
    float   rayLength;
    float   currentLifespan;
    float   maxDuration;
    float   velocityScale;
    bool    isActive;
};

// =====================================================================================
// PROCEDURAL STARFIELD ADVANCED PIPELINE SUBSYSTEM
// =====================================================================================
class StarfieldAdvancedEngine {
private:
    int paneWidth;
    int paneHeight;
    std::vector<QuantumStar> cosmicMatrix;
    ShootingStar dynamicMeteor;

    // Helper Trigonometry Constants
    float ComputeSineTransform(float value) const { return std::sin(value); }

    Color SelectSpectralTint() {
        int choice = GetRandomValue(0, 100);
        if (choice < 15) return Color{ 175, 220, 255, 255 }; // Type-O Blue Giant star emulation
        if (choice > 85) return Color{ 255, 230, 180, 255 }; // Type-M Red Dwarf profile
        return RAYWHITE;                                     // Standard white balance
    }

public:
    StarfieldAdvancedEngine(int width, int height, int targetDensity) 
        : paneWidth(width), paneHeight(height) {
        
        // Initialize active tracking state for dynamic physics objects
        dynamicMeteor.isActive = false;

        // 1. PROCEDURAL CANOPY CONTEXT GENERATION
        for (int i = 0; i < targetDensity; ++i) {
            int layer = GetRandomValue(0, 2); // Split structure across 3 layers
            float size = 0.5f;
            if (layer == 1) size = 1.1f;
            if (layer == 2) size = 1.8f; // Closer stars are optically wider

            cosmicMatrix.push_back({
                { (float)GetRandomValue(0, paneWidth), (float)GetRandomValue(0, (int)(paneHeight * 0.72f)) },
                size,
                (float)GetRandomValue(30, 100) / 100.0f,
                (float)GetRandomValue(0, 360) * (3.14159f / 180.0f),
                (float)GetRandomValue(3, 9) * 0.5f,
                layer,
                SelectSpectralTint()
            });
        }
    }

    void ProcessSystemPhysics(float timeDeltaStep, float cameraPanOffset) {
        float absoluteRuntime = GetTime();

        // 2. PARALLAX OFFSET MATRICES PROCESSING
        for (auto& star : cosmicMatrix) {
            // Apply speed reduction factors based on inverted geometric depth layers
            float parallaxSpeedFactor = (float)(star.parallaxLayerId + 1) * 0.15f;
            star.coordinates.x -= (cameraPanOffset * parallaxSpeedFactor);

            // Frame wrapping boundaries logic check
            if (star.coordinates.x < 0) star.coordinates.x += paneWidth;
            if (star.coordinates.x > paneWidth) star.coordinates.x -= paneWidth;
        }

        // 3. SHOOTING STAR METEOR SIMULATION ITERATOR LOOP
        if (!dynamicMeteor.isActive) {
            // Percentile structural activation check (Approx once every few seconds randomly)
            if (GetRandomValue(0, 1200) < 4) {
                dynamicMeteor.isActive = true;
                dynamicMeteor.structuralPosition = { (float)GetRandomValue(100, paneWidth - 300), (float)GetRandomValue(20, 250) };
                dynamicMeteor.trajectoryVelocity = { (float)GetRandomValue(600, 900), (float)GetRandomValue(300, 500) }; // High speed vectors
                dynamicMeteor.rayLength = (float)GetRandomValue(40, 90);
                dynamicMeteor.currentLifespan = 0.0f;
                dynamicMeteor.maxDuration = (float)GetRandomValue(4, 9) * 0.1f; // Quick burst streak lifespan
            }
        } else {
            // Updating displacement kinetics coordinates
            dynamicMeteor.structuralPosition.x += dynamicMeteor.trajectoryVelocity.x * timeDeltaStep;
            dynamicMeteor.structuralPosition.y += dynamicMeteor.trajectoryVelocity.y * timeDeltaStep;
            dynamicMeteor.currentLifespan += timeDeltaStep;

            if (dynamicMeteor.currentLifespan >= dynamicMeteor.maxDuration) {
                dynamicMeteor.isActive = false; // Lifecycle termination frame reached
            }
        }
    }

    void RenderStarfieldCanopy(float globalAtmosphereVisibility) {
        if (globalAtmosphereVisibility <= 0.02f) return; // Completely mask out if solar glare dominates completely

        float engineTime = GetTime();

        // 4. SCINTILLATION DISPERSION PIPELINE RENDER
        for (const auto& star : cosmicMatrix) {
            // Rayleigh scattering masking based on vertical coordinates profile
            // Low horizon height causes early optical fading curves
            float horizonMaskFactor = 1.0f - (star.coordinates.y / (paneHeight * 0.72f));
            horizonMaskFactor = std::pow(horizonMaskFactor, 1.5f); // Smooth log transition

            // Calculate complex mathematical frequency twinkle pulse
            float waveValue = ComputeSineTransform(engineTime * star.scintillationSpeed + star.scintillationPhase);
            // Deep space layers scale down fluctuations slightly to mimic realistic visual limits
            float layerStabilizer = (star.parallaxLayerId == 0) ? 0.25f : 0.55f;
            
            float calculatedAlpha = star.luminosityAlpha * (1.0f - layerStabilizer + (waveValue * layerStabilizer));
            float finalCombinedAlpha = calculatedAlpha * globalAtmosphereVisibility * horizonMaskFactor;

            if (finalCombinedAlpha > 0.02f) {
                DrawCircleV(star.coordinates, star.nuclearRadius, ColorAlpha(star.spectralTint, finalCombinedAlpha));
                
                // Optically heavy closer layer stars receive glow crossbar matrices anchors
                if (star.parallaxLayerId == 2 && finalCombinedAlpha > 0.6f) {
                    DrawLineV({star.coordinates.x - 3, star.coordinates.y}, {star.coordinates.x + 3, star.coordinates.y}, ColorAlpha(star.spectralTint, finalCombinedAlpha * 0.4f));
                    DrawLineV({star.coordinates.x, star.coordinates.y - 3}, {star.coordinates.x, star.coordinates.y + 3}, ColorAlpha(star.spectralTint, finalCombinedAlpha * 0.4f));
                }
            }
        }

        // 5. METEOR RENDER EXECUTION STRUCTURE
        if (dynamicMeteor.isActive) {
            float trajectoryAlpha = 1.0f - (dynamicMeteor.currentLifespan / dynamicMeteor.maxDuration);
            Vector2 pathOrigin = dynamicMeteor.structuralPosition;
            
            // Calculate tail trailing vectors parameters back tracking
            Vector2 velocityDirectionNorm = { dynamicMeteor.trajectoryVelocity.x, dynamicMeteor.trajectoryVelocity.y };
            float dynamicMag = std::sqrt(velocityDirectionNorm.x * velocityDirectionNorm.x + velocityDirectionNorm.y * velocityDirectionNorm.y);
            velocityDirectionNorm.x /= dynamicMag;
            velocityDirectionNorm.y /= dynamicMag;

            Vector2 tailTermination = {
                pathOrigin.x - (velocityDirectionNorm.x * dynamicMeteor.rayLength),
                pathOrigin.y - (velocityDirectionNorm.y * dynamicMeteor.rayLength)
            };

            // High brightness tail extrusion block
            DrawLineEx(pathOrigin, tailTermination, 2.2f, ColorAlpha(RAYWHITE, trajectoryAlpha * globalAtmosphereVisibility));
            DrawCircleV(pathOrigin, 2.0f, ColorAlpha(SKYBLUE, trajectoryAlpha * globalAtmosphereVisibility));
        }
    }
};

// =====================================================================================
// APPLICATION CONTEXT ARCHITECTURE EXECUTION ENTRY
// =====================================================================================
int main() {
    const int winWidth  = 1280;
    const int winHeight = 720;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(winWidth, winHeight, "Quantum Starfield Scintillation & Parallax Space-Engine Sandbox");

    // Constructing star field with 400 procedural particle objects
    std::unique_ptr<StarfieldAdvancedEngine> starfieldEngine = 
        std::make_unique<StarfieldAdvancedEngine>(winWidth, winHeight, 400);

    float globalVisibilityMatrix = 1.0f; // Mock night state setting visibility to full (1.0)
    float cameraVelocityVector = 0.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // 1. Interactive testing interface handles: simulate environment velocity shifting camera positions
        cameraVelocityVector = 0.0f;
        if (IsKeyDown(KEY_RIGHT)) cameraVelocityVector = 150.0f * deltaTime; // Move world right
        if (IsKeyDown(KEY_LEFT))  cameraVelocityVector = -150.0f * deltaTime;

        // Toggle visibility cycle using key inputs to simulate dawn/dusk transitions manually
        if (IsKeyDown(KEY_UP))    globalVisibilityMatrix += 0.4f * deltaTime;
        if (IsKeyDown(KEY_DOWN))  globalVisibilityMatrix -= 0.4f * deltaTime;
        if (globalVisibilityMatrix < 0.0f) globalVisibilityMatrix = 0.0f;
        if (globalVisibilityMatrix > 1.0f) globalVisibilityMatrix = 1.0f;

        // 2. Core Updates Processing block execution 
        starfieldEngine->ProcessSystemPhysics(deltaTime, cameraVelocityVector);

        // Calculate dynamic clear backgrounds variables matching context transitions 
        Color skyBackgroundContext = Color{ (unsigned char)(5 * globalVisibilityMatrix), 
                                            (unsigned char)(6 * globalVisibilityMatrix), 
                                            (unsigned char)(18 * globalVisibilityMatrix), 255 };

        BeginDrawing();
            ClearBackground(skyBackgroundContext);

            // Execute Procedural Engine
            starfieldEngine->RenderStarfieldCanopy(globalVisibilityMatrix);

            // Environment Ground deck reference point
            DrawRectangle(0, winHeight - 100, winWidth, 100, Color{ 12, 14, 16, 255 });

            // Visual overlay metadata frame
            DrawRectangle(20, 20, 420, 135, ColorAlpha(BLACK, 0.75f));
            DrawRectangleLines(20, 20, 420, 135, ColorAlpha(WHITE, 0.15f));
            DrawText("SCINTILLATION CONSTELLATION NODE v5.0", 35, 32, 13, CYAN);
            DrawText(TextFormat("Star Canopy Alpha Gain: %.2f %%", globalVisibilityMatrix * 100.0f), 35, 60, 12, LIGHTGRAY);
            DrawText(TextFormat("Parallax Vector Scroll Offset: %.3f px", cameraVelocityVector), 35, 80, 12, GRAY);
            DrawText("Controls: LEFT/RIGHT Arrows scroll sky background field", 35, 105, 11, DARKGRAY);
            DrawText("          UP/DOWN Arrows fade system into daytime visibility mask", 35, 120, 11, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
