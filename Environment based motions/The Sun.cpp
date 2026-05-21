#include "raylib.h"
#include <cmath>
#include <vector>
#include <string>
#include <memory>

// =====================================================================================
// ARCHITECTURAL CONSTANTS & MATHEMATICAL STRUCTS
// =====================================================================================
const float SYNODIC_MONTH_DAYS = 29.53059f; // Real-world lunar phase cycle length
const int   MAX_ASTEROIDS_METEORS   = 3;

struct Vector2D {
    double x;
    double y;
    
    Vector2 ToRaylib() const { return Vector2{(float)x, (float)y}; }
};

struct SolarFlareRay {
    float angleOffset;
    float lengthScale;
    float pulseSpeed;
};

struct CelestialBody {
    Vector2D position;
    float    apparentRadius;
    Color    coreColor;
    Color    spectralGlow;
};

// =====================================================================================
// QUANTUM ATMOSPHERE ENVIRONMENT MANAGEMENT ENGINE
// =====================================================================================
class AdvancedAtmosphereEngine {
private:
    // Core Engine Metrics
    double internalTimeHours;
    double cycleSpeedMultiplier;
    double lunarAgeDays;             // Tracks moon phase progression independently
    
    int renderWidth;
    int renderHeight;
    Vector2D orbitalPivot;
    float orbitalRadius;

    // Atmospheric Shader Elements
    std::vector<SolarFlareRay> solarFlares;
    Texture2D atmosphericNoise;      // For cosmic background micro-textures if needed

    // Internal Mathematical Interpolations
    float ComputeMathematicalSine(float angleRadians) const {
        return std::sin(angleRadians);
    }
    
    float ComputeMathematicalCosine(float angleRadians) const {
        return std::cos(angleRadians);
    }

    Color InterpolateRGBA(Color baseline, Color target, float factor) {
        factor = floatMax(0.0f, floatMin(1.0f, factor));
        return Color{
            (unsigned char)(baseline.r + (target.r - baseline.r) * factor),
            (unsigned char)(baseline.g + (target.g - baseline.g) * factor),
            (unsigned char)(baseline.b + (target.b - baseline.b) * factor),
            (unsigned char)(baseline.a + (target.a - baseline.a) * factor)
        };
    }

    float floatMin(float a, float b) { return (a < b) ? a : b; }
    float floatMax(float a, float b) { return (a > b) ? a : b; }

public:
    AdvancedAtmosphereEngine(int width, int height, double startingHour)
        : renderWidth(width), renderHeight(height), internalTimeHours(startingHour) {
        
        cycleSpeedMultiplier = 0.02; // Configured base speed
        lunarAgeDays = 14.0;         // Starts near Full Moon phase state
        
        orbitalPivot = { (double)width / 2.0, (double)height + 250.0 };
        orbitalRadius = (float)height * 1.15f;

        // Construct 24 structural unique Solar Flare rays for algorithmic variance
        for (int i = 0; i < 24; ++i) {
            solarFlares.push_back({
                (float)i * (360.0f / 24.0f) * (PI / 180.0f),
                (float)GetRandomValue(70, 130) / 100.0f,
                (float)GetRandomValue(3, 8) * 1.5f
            });
        }
    }

    ~AdvancedAtmosphereEngine() {}

    void UpdateSystemPipeline(float frameDeltaTime) {
        // Precise time advancements calculation
        internalTimeHours += (frameDeltaTime * cycleSpeedMultiplier);
        if (internalTimeHours >= 24.0) {
            internalTimeHours -= 24.0;
            lunarAgeDays += 1.0; // Advance moon phase cycle every technical day
            if (lunarAgeDays >= SYNODIC_MONTH_DAYS) lunarAgeDays -= SYNODIC_MONTH_DAYS;
        }
    }

    void ExecuteRenderPipeline() {
        float globalTimeSec = GetTime();

        // 1. HORIZON RAYLEIGH DISPERSION GRADIENT MAPPING
        // Processing scattering parameters dynamically based on solar elevation
        float solarAngleRad = ((float)(internalTimeHours - 6.0) / 24.0f) * 2.0f * PI;
        float sunElevation = -ComputeMathematicalSine(solarAngleRad); // High value = High in sky

        Color dynamicZenith, dynamicHorizon;
        
        if (sunElevation > 0.0f) { // Daytime / Transition states
            float noonFactor = sunElevation; // Maxes at 1.0 at noon
            Color noonZenith   = Color{ 12, 85, 185, 255 };
            Color noonHorizon  = Color{ 134, 206, 243, 255 };
            Color goldenZenith = Color{ 35, 45, 90, 255 };
            Color goldenHorizon= Color{ 240, 115, 45, 255 };

            dynamicZenith  = InterpolateRGBA(goldenZenith, noonZenith, noonFactor);
            dynamicHorizon = InterpolateRGBA(goldenHorizon, noonHorizon, noonFactor);
        } else { // Nighttime states
            float nightFactor = -sunElevation; 
            Color midnightZenith  = Color{ 2, 4, 12, 255 };
            Color midnightHorizon = Color{ 8, 12, 32, 255 };
            Color twilightZenith  = Color{ 20, 25, 55, 255 };
            Color twilightHorizon = Color{ 45, 30, 65, 255 };

            dynamicZenith  = InterpolateRGBA(twilightZenith, midnightZenith, nightFactor);
            dynamicHorizon = InterpolateRGBA(twilightHorizon, midnightHorizon, nightFactor);
        }
        DrawRectangleGradientV(0, 0, renderWidth, renderHeight, dynamicZenith, dynamicHorizon);

        // 2. SUN MATHEMATICAL CORE POSITIONING & POSITION ENGINE
        Vector2D sunCoordinates = {
            orbitalPivot.x + ComputeMathematicalCosine(solarAngleRad) * orbitalRadius,
            orbitalPivot.y + ComputeMathematicalSine(solarAngleRad) * orbitalRadius
        };

        // 3. SOLAR GLOW & RADIATIVE ATMOSPHERIC FLARE RENDER ENGINE
        if (sunCoordinates.y < renderHeight + 100) {
            Vector2 sunCenter = sunCoordinates.ToRaylib();
            
            // Atmospheric Scattering Vector (Mie Scattering Color shift emulation)
            Color scatterColor = Color{255, 255, 210, 255};
            if (sunElevation < 0.2f) {
                // Shift to Red-Orange when clipping horizon boundary line
                float horizonProximity = (0.2f - sunElevation) / 0.2f;
                scatterColor = InterpolateRGBA(scatterColor, Color{255, 80, 20, 255}, horizonProximity);
            }

            // A. Draw Outer Volumetric Corona Glow (Dynamic Pulse Wave based on absolute time)
            float baseCoronaPulse = 180.0f + 25.0f * ComputeMathematicalSine(globalTimeSec * 2.1f);
            for (int r = 4; r > 0; --r) {
                float layerRadius = baseCoronaPulse * ((float)r / 4.0f);
                float layerAlpha = 0.03f * (5 - r) * floatMax(0.1f, sunElevation);
                DrawCircleGradient(sunCenter.x, sunCenter.y, layerRadius, ColorAlpha(scatterColor, layerAlpha), ColorAlpha(scatterColor, 0.0f));
            }

            // B. Precise Dynamic Algorithmic Solar Ray Flares (Complex geometry projection)
            float rayMasterRotation = globalTimeSec * 0.15f; // Slow rotation of rays
            for (const auto& flare : solarFlares) {
                float dynamicAngle = flare.angleOffset + rayMasterRotation;
                float cyclePulse = flare.lengthScale * (1.0f + 0.15f * ComputeMathematicalSine(globalTimeSec * flare.pulseSpeed));
                
                // Construct triangle coordinates extending outwards from sun nucleus core
                float rayThickness = 0.06f; // Radians wide
                Vector2 vertexLeft = {
                    (float)(sunCoordinates.x + ComputeMathematicalCosine(dynamicAngle - rayThickness) * 35.0),
                    (float)(sunCoordinates.y + ComputeMathematicalSine(dynamicAngle - rayThickness) * 35.0)
                };
                Vector2 vertexRight = {
                    (float)(sunCoordinates.x + ComputeMathematicalCosine(dynamicAngle + rayThickness) * 35.0),
                    (float)(sunCoordinates.y + ComputeMathematicalSine(dynamicAngle + rayThickness) * 35.0)
                };
                Vector2 apexTip = {
                    (float)(sunCoordinates.x + ComputeMathematicalCosine(dynamicAngle) * (60.0f * cyclePulse)),
                    (float)(sunCoordinates.y + ComputeMathematicalSine(dynamicAngle) * (60.0f * cyclePulse))
                };

                DrawTriangle(vertexLeft, apexTip, vertexRight, ColorAlpha(scatterColor, 0.22f * floatMax(0.0f, sunElevation)));
            }

            // C. Ultra Core Fusion Nucleus Render
            DrawCircleV(sunCenter, 38.0f, ColorAlpha(scatterColor, 0.7f));
            DrawCircleV(sunCenter, 32.0f, RAYWHITE);
        }

        // 4. PRECISE ADVANCED LUNAR ORBIT & SYNODIC PHASE RENDER ENGINE
        float lunarAngleRad = solarAngleRad + PI; // Locked anti-meridian trajectory mapping
        Vector2D moonCoordinates = {
            orbitalPivot.x + ComputeMathematicalCosine(lunarAngleRad) * orbitalRadius,
            orbitalPivot.y + ComputeMathematicalSine(lunarAngleRad) * orbitalRadius
        };

        if (moonCoordinates.y < renderHeight + 100) {
            Vector2 moonCenter = moonCoordinates.ToRaylib();
            float radiusMoon = 30.0f;
            
            // Dynamic Lunar Aura Glow
            DrawCircleGradient(moonCenter.x, moonCenter.y, radiusMoon * 2.5f, ColorAlpha(LIGHTGRAY, 0.15f), ColorAlpha(LIGHTGRAY, 0.0f));

            // Synodic Phase Ratio Calculation [0.0 = New Moon, 0.5 = Full Moon, 1.0 = Loop New Moon]
            float phaseNormalized = lunarAgeDays / SYNODIC_MONTH_DAYS;
            
            // Physical Back-plate Drawing Configuration (Lit portion of moon structural shape)
            DrawCircleV(moonCenter, radiusMoon, Color{235, 240, 250, 255});

            // Advanced Shadow Projection Overlap Masking
            // Emulates geometric translation of dark shadow limb crossing over moon surface
            float phaseOffsetShift = (phaseNormalized * 4.0f) - 2.0f; // Range mapped between -2.0 and +2.0
            Vector2 shadowMaskCenter = {
                moonCenter.x + (radiusMoon * phaseOffsetShift),
                moonCenter.y
            };
            
            // Blend shadow color with current deep zenith context to mask correctly
            Color localizedSkyContext = LerpColor(dynamicZenith, dynamicHorizon, (float)moonCoordinates.y / renderHeight);
            DrawCircleV(shadowMaskCenter, radiusMoon * 1.05f, localizedSkyContext);
        }

        // 5. AMBIENT GRAPHICS HUD FOREGROUND LOGIC
        DrawRectangle(0, renderHeight - 60, renderWidth, 60, ColorAlpha(BLACK, 0.8f));
        
        int militaryHours = (int)internalTimeHours;
        int militaryMinutes = (int)((internalTimeHours - (double)militaryHours) * 60.0);
        
        std::string phaseLabel = "Waxing Crescent";
        float currentPhasePercent = (lunarAgeDays / SYNODIC_MONTH_DAYS) * 100.0f;
        if (currentPhasePercent > 45.0f && currentPhasePercent < 55.0f) phaseLabel = "Full Moon Peak";
        else if (currentPhasePercent >= 90.0f || currentPhasePercent <= 5.0f) phaseLabel = "New Moon Stage";

        DrawText(TextFormat("ENVIRONMENT TIME ENGINE: %02d:%02d", militaryHours, militaryMinutes), 25, renderHeight - 42, 18, LIGHTGRAY);
        DrawText(TextFormat("LUNAR SYNODIC PHASE: %s (Age: %.1f Days)", phaseLabel.c_str(), lunarAgeDays), renderWidth - 450, renderHeight - 40, 15, GOLD);
        
        // System telemetry metrics visual overlay box
        DrawRectangle(20, 20, 310, 110, ColorAlpha(DARKBLUE, 0.4f));
        DrawRectangleLines(20, 20, 310, 110, ColorAlpha(CYAN, 0.5f));
        DrawText("ATMOSPHERE SCATTERING ENGINE v3.5", 35, 35, 13, CYAN);
        DrawText(TextFormat("Sun Vector Elevation: %.4f", sunElevation), 35, 60, 12, RAYWHITE);
        DrawText(TextFormat("Solar Radiation Flux: %.2f W/m²", floatMax(0.0f, sunElevation) * 1361.0f), 35, 80, 12, YELLOW);
        DrawText(TextFormat("Ray Density: %d Rendered Primitives", solarFlares.size()), 35, 100, 12, GRAY);
    }
    
    void ModifySimulationRate(double modificationFactor) {
        cycleSpeedMultiplier += modificationFactor;
        if (cycleSpeedMultiplier < 0.0) cycleSpeedMultiplier = 0.0;
    }
};

// =====================================================================================
// SYSTEM INITIALIZATION CONTEXT ENTRY
// =====================================================================================
int main() {
    const int resolutionWidth  = 1440;
    const int resolutionHeight = 900;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(resolutionWidth, resolutionHeight, "Quantum Solar Glow & Synodic Celestial Pipeline Vector-Engine");

    // Instantiating memory footprint on heap using smart architecture structures
    std::unique_ptr<AdvancedAtmosphereEngine> coreSimulationEngine = 
        std::make_unique<AdvancedAtmosphereEngine>(resolutionWidth, resolutionHeight, 5.50); // Engine starts at Dawn dawn hour (05:30 AM)

    SetTargetFPS(60);

    // Main Execution Cycle Iteration Loop
    while (!WindowShouldClose()) {
        
        // Intercept inputs to tweak the processing execution speeds manually
        if (IsKeyDown(KEY_RIGHT)) coreSimulationEngine->ModifySimulationRate(0.002);
        if (IsKeyDown(KEY_LEFT))  coreSimulationEngine->ModifySimulationRate(-0.002);

        // Frame updates metrics processing block
        coreSimulationEngine->UpdateSystemPipeline(GetFrameTime());

        // Context drawing block structure execution
        BeginDrawing();
            ClearBackground(BLANK); // Handled explicitly inside pipeline architecture engine
            coreSimulationEngine->ExecuteRenderPipeline();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
