#include "raylib.h"
#include <cmath>
#include <vector>
#include <string>
#include <memory>

// =====================================================================================
// MATHEMATICAL & LUNAR GEOMETRY STRUCTURES
// =====================================================================================
struct Crater {
    Vector2 relativePosition; // Position relative to moon center (-1.0 to 1.0)
    float radiusFactor;       // Size percentage relative to moon radius
    float depthAlpha;         // Shadow opacity
};

struct LunarTelemetry {
    float orbitalAngle;
    float synodicPhase;      // 0.0 to 1.0 (New Moon to Full Moon Loop)
    float librationWobble;   // Real-world lunar wobble deviation
    float apparentScale;     // Perigee/Apogee distance scaling factor
};

// =====================================================================================
// QUANTUM LUNAR ADVANCED SUBSYSTEM ENGINE
// =====================================================================================
class LunarAdvancedSubsystem {
private:
    int renderWidth;
    int renderHeight;
    float baseMoonRadius;
    std::vector<Crater> proceduralCraterMap;

    // Helper Math Matrix Calculations
    float ComputeSine(float rad) const { return std::sin(rad); }
    float ComputeCosine(float rad) const { return std::cos(rad); }

    Color InterpolateColors(Color c1, Color c2, float t) {
        t = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);
        return Color{
            (unsigned char)(c1.r + (c2.r - c1.r) * t),
            (unsigned char)(c1.g + (c2.g - c1.g) * t),
            (unsigned char)(c1.b + (c2.b - c1.b) * t),
            (unsigned char)(c1.a + (c2.a - c1.a) * t)
        };
    }

    void GenerateSurfaceTopography() {
        // Hardcoding distinct structural craters mimicking lunar highlands and basins
        // Central Basins (Maria)
        proceduralCraterMap.push_back({{ -0.3f, -0.2f }, 0.28f, 0.22f }); // Mare Imbrium
        proceduralCraterMap.push_back({{ 0.2f, -0.1f }, 0.35f, 0.25f });  // Mare Serenitatis
        proceduralCraterMap.push_back({{ 0.4f, 0.2f }, 0.22f, 0.20f });   // Mare Tranquillitatis
        
        // Explicit Impact Craters (With high structural integrity values)
        proceduralCraterMap.push_back({{ -0.1f, 0.5f }, 0.09f, 0.45f });  // Tycho Crater
        proceduralCraterMap.push_back({{ -0.4f, 0.1f }, 0.07f, 0.38f });  // Copernicus Crater
        proceduralCraterMap.push_back({{ 0.1f, -0.4f }, 0.06f, 0.35f });  // Plato Crater
        
        // Micro-crater distribution noise fields
        for(int i = 0; i < 15; i++) {
            float angle = (float)GetRandomValue(0, 360) * (PI / 180.0f);
            float dist = (float)GetRandomValue(20, 90) / 100.0f;
            proceduralCraterMap.push_back({
                { ComputeCosine(angle) * dist, ComputeSine(angle) * dist },
                (float)GetRandomValue(2, 5) / 100.0f,
                (float)GetRandomValue(15, 40) / 100.0f
            });
        }
    }

public:
    LunarAdvancedSubsystem(int width, int height) 
        : renderWidth(width), renderHeight(height) {
        baseMoonRadius = 45.0f;
        GenerateSurfaceTopography();
    }

    void RenderLunarCoreSystem(Vector2 center, LunarTelemetry telemetry, Color structuralSkyContext) {
        float runTime = GetTime();
        
        // 1. PHYSICAL APPARENT RADIUS WITH PERIGEE/APOGEE PULSE
        // Simulating the elliptical orbit gravitational distance contraction/expansion
        float dynamicRadius = baseMoonRadius * telemetry.apparentScale;

        // 2. LUNAR AURA CORONA (Atmospheric Mie Scattering around Moon disc)
        Color lunarGlowColor = Color{ 225, 235, 255, 255 };
        for (int i = 5; i > 0; --i) {
            float auraRadius = dynamicRadius * (1.0f + (i * 0.35f));
            float auraAlpha = 0.025f * (6 - i);
            DrawCircleGradient(center.x, center.y, auraRadius, ColorAlpha(lunarGlowColor, auraAlpha), ColorAlpha(lunarGlowColor, 0.0f));
        }

        // 3. BASE DISK VECTOR (The illuminated surface background)
        Color lunarRegolithBase = Color{ 240, 242, 248, 255 };
        DrawCircleV(center, dynamicRadius, lunarRegolithBase);

        // 4. DRAW CRATER TOPOGRAPHY (Mapped with Libration Matrix shifts)
        // Libration offsets mimic perspective shifting due to synchronous rotation variances
        Vector2 librationOffset = {
            ComputeCosine(runTime * 0.5f) * telemetry.librationWobble * 8.0f,
            ComputeSine(runTime * 0.3f) * telemetry.librationWobble * 8.0f
        };

        for (const auto& crater : proceduralCraterMap) {
            // Calculating actual projection coordinates onto screen space
            Vector2 craterWorldPos = {
                center.x + (crater.relativePosition.x * dynamicRadius) + librationOffset.x,
                center.y + (crater.relativePosition.y * dynamicRadius) + librationOffset.y
            };

            // Clipping bounds verification to prevent texture bleeding outside sphere boundary line
            float distanceFromCenter = std::sqrt(std::pow(craterWorldPos.x - center.x, 2) + std::pow(craterWorldPos.y - center.y, 2));
            if (distanceFromCenter < dynamicRadius - 2.0f) {
                float finalCraterRadius = dynamicRadius * crater.radiusFactor;
                // Double layer drawing for depth perception (Shadow and Highlight ridge)
                DrawCircleV(Vector2{craterWorldPos.x + 1.0f, craterWorldPos.y + 1.0f}, finalCraterRadius, ColorAlpha(WHITE, crater.depthAlpha * 0.5f));
                DrawCircleV(craterWorldPos, finalCraterRadius, ColorAlpha(Color{40, 45, 65, 255}, crater.depthAlpha));
            }
        }

        // 5. TRIGONOMETRICAL ELLIPSOID SHADOW PHASING OVERLAY (True Synodic Core Matcher)
        float phase = telemetry.synodicPhase; // Range 0.0 to 1.0
        Color shadowColor = ColorAlpha(structuralSkyContext, 0.96f); // Shadows match dynamic sky context opacity

        if (phase < 0.5f) { 
            // Waning Phase Section (Shadow tracking left to right)
            float segmentFactor = phase / 0.5f; // Normalizing to 0.0 - 1.0
            float curveWidth = dynamicRadius * (1.0f - (segmentFactor * 2.0f));
            
            if (curveWidth >= 0.0f) { // Crescent Stage Shadow
                // Render flat blocking semi-circle mask
                DrawCircleSector(center, dynamicRadius, 90, 270, 36, shadowColor);
                // Render elliptical masking boundary overlay
                for (int y = -(int)dynamicRadius; y <= (int)dynamicRadius; y++) {
                    float xElliptical = curveWidth * std::sqrt(1.0f - std::pow((float)y / dynamicRadius, 2));
                    DrawLineV({center.x - xElliptical, center.y + y}, {center.x, center.y + y}, shadowColor);
                }
            } else { // Gibbous Light Stage (Shadow shrinking on left side)
                float absCurveWidth = std::abs(curveWidth);
                for (int y = -(int)dynamicRadius; y <= (int)dynamicRadius; y++) {
                    float xElliptical = absCurveWidth * std::sqrt(1.0f - std::pow((float)y / dynamicRadius, 2));
                    DrawLineV({center.x - dynamicRadius, center.y + y}, {center.x - xElliptical, center.y + y}, shadowColor);
                }
            }
        } 
        else {
            // Waxing Phase Section (Light expanding from right to left)
            float segmentFactor = (phase - 0.5f) / 0.5f;
            float curveWidth = dynamicRadius * (1.0f - (segmentFactor * 2.0f));

            if (curveWidth >= 0.0f) { // Gibbous Darkening Phase transition
                for (int y = -(int)dynamicRadius; y <= (int)dynamicRadius; y++) {
                    float xElliptical = curveWidth * std::sqrt(1.0f - std::pow((float)y / dynamicRadius, 2));
                    DrawLineV({center.x + xElliptical, center.y + y}, {center.x + dynamicRadius, center.y + y}, shadowColor);
                }
            } else { // New Moon / Sharp Crescent Phase Approaching
                float absCurveWidth = std::abs(curveWidth);
                DrawCircleSector(center, dynamicRadius, 270, 450, 36, shadowColor);
                for (int y = -(int)dynamicRadius; y <= (int)dynamicRadius; y++) {
                    float xElliptical = absCurveWidth * std::sqrt(1.0f - std::pow((float)y / dynamicRadius, 2));
                    DrawLineV({center.x, center.y + y}, {center.x + xElliptical, center.y + y}, shadowColor);
                }
            }
        }

        // 6. DRAW LIMB GLOW EFFECT (Crisp outer perimeter atmospheric rim)
        DrawCircleLines(center.x, center.y, dynamicRadius, ColorAlpha(WHITE, 0.25f));
    }
};

// =====================================================================================
// SIMULATION GRAPHICS RUNTIME ENVIRONMENT CONTEXT
// =====================================================================================
int main() {
    const int winWidth = 1280;
    const int winHeight = 800;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(winWidth, winHeight, "Super-Precision Algorithmic Lunar Pipeline Engine v4.0");

    std::unique_ptr<LunarAdvancedSubsystem> lunarEngine = std::make_unique<LunarAdvancedSubsystem>(winWidth, winHeight);

    // Engine States trackers
    float masterTimelineTracker = 18.0f; // Fast-forward directly into the night matrix layer (06:00 PM)
    float simulationVelocity = 0.015f;   // Time increment step per processing frame execution loop

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dT = GetFrameTime();
        
        // Process dynamic inputs to modulate pipeline parameters on-the-fly
        if (IsKeyDown(KEY_UP))   simulationVelocity += 0.005f;
        if (IsKeyDown(KEY_DOWN)) simulationVelocity -= 0.005f;
        if (simulationVelocity < 0.0f) simulationVelocity = 0.0f;

        masterTimelineTracker += dT * simulationVelocity;
        if (masterTimelineTracker >= 24.0f) masterTimelineTracker -= 24.0f;

        // Calculate dynamic night vector values to create a live back-plate blending color array
        float skyInterpolationFactor = std::abs(std::sin((masterTimelineTracker / 24.0f) * PI));
        Color activeNightSkyColor = Color{ (unsigned char)(8 + (12 * skyInterpolationFactor)), 
                                           (unsigned char)(10 + (20 * skyInterpolationFactor)), 
                                           (unsigned char)(28 + (35 * skyInterpolationFactor)), 255 };

        // Construct Telemetry Struct Packaging block dynamically
        float synodicPhaseCycle = std::fmod(GetTime() * 0.04f, 1.0f); // Fast cycle looping phase states for demonstration
        float apogeePerigeeScale = 1.0f + 0.12f * std::sin(GetTime() * 0.8f); // High-frequency scaling variation tracking
        
        LunarTelemetry currentTelemetry = {
            (masterTimelineTracker / 24.0f) * 2.0f * (float)PI,
            synodicPhaseCycle,
            0.05f, // Baseline Libration variance limit boundary
            apogeePerigeeScale
        };

        // Static Moon Screen coordinate tracker
        Vector2 screenOrbitPos = { winWidth / 2.0f, winHeight / 2.0f - 50.0f };

        BeginDrawing();
            ClearBackground(activeNightSkyColor);

            // Render Core Modular Subsystem Component Execution
            lunarEngine->RenderLunarCoreSystem(screenOrbitPos, currentTelemetry, activeNightSkyColor);

            // Interface overlay rendering
            DrawRectangle(15, 15, 360, 140, ColorAlpha(BLACK, 0.7f));
            DrawRectangleLines(15, 15, 360, 140, ColorAlpha(WHITE, 0.2f));
            DrawText("ADVANCED LUNAR TELEMETRY NODE", 25, 25, 14, LIGHTGRAY);
            DrawText(TextFormat("Synodic Phase Step: %.4f %%", synodicPhaseCycle * 100.0f), 25, 55, 13, GOLD);
            DrawText(TextFormat("Libration Delta Wobble: %.5f rad", currentTelemetry.librationWobble), 25, 75, 13, CYAN);
            DrawText(TextFormat("Orbit Vector Scale: %.3fx (%s)", apogeePerigeeScale, (apogeePerigeeScale > 1.0f ? "PERIGEE/SUPERMOON" : "APOGEE")), 25, 95, 13, MAGENTA);
            DrawText("System Mechanics: ARROW UP/DOWN controls velocity", 25, 122, 11, GRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
