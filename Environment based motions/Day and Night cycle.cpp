#include "raylib.h"
#include <cmath>
#include <vector>
#include <string>

// =====================================================================================
// DEFINITIONS & STRUCTURES
// =====================================================================================
enum DayPhase { DAWN, NOON, DUSK, MIDNIGHT, DEEP_NIGHT };

struct Star {
    Vector2 position;
    float baseRadius;
    float brightness;
    float twinklePhase;
    float twinkleSpeed;
};

struct AtmosphereKeyframe {
    float hour;             // 24-hour mark (0.0 - 24.0)
    Color zenithColor;      // Top of the sky
    Color horizonColor;     // Bottom of the sky
    Color ambientLight;     // Light reflecting on the world
    float starVisibility;   // 0.0 (Invisible) to 1.0 (Fully bright)
};

// =====================================================================================
// CELESTIAL MANAGEMENT CLASS
// =====================================================================================
class CelestialEngine {
private:
    float currentHour;
    float cycleSpeed; // How fast time moves relative to real-time
    int screenWidth;
    int screenHeight;
    
    Vector2 pivotCenter;
    float orbitRadius;
    
    std::vector<Star> starfield;
    std::vector<AtmosphereKeyframe> timeline;

    // Helper: Linear Interpolation for Colors
    Color LerpColor(Color c1, Color c2, float t) {
        return Color{
            (unsigned char)(c1.r + (c2.r - c1.r) * t),
            (unsigned char)(c1.g + (c2.g - c1.g) * t),
            (unsigned char)(c1.b + (c2.b - c1.b) * t),
            (unsigned char)(c1.a + (c2.a - c1.a) * t)
        };
    }

    void SetupAtmosphereTimeline() {
        // Precise atmospheric keyframes mapped directly to 24 hours
        timeline.push_back({0.0f,  {10, 10, 25, 255},  {5, 5, 15, 255},    {20, 20, 35, 255},  1.0f}); // Midnight
        timeline.push_back({5.0f,  {25, 30, 65, 255},  {70, 45, 95, 255},   {40, 35, 50, 255},  0.7f}); // Pre-Dawn
        timeline.push_back({6.5f,  {40, 80, 140, 255}, {245, 130, 60, 255}, {130, 90, 80, 255}, 0.0f}); // Sunrise / Dawn
        timeline.push_back({12.0f, {30, 110, 200, 255},{110, 180, 240, 255},{255, 255, 250, 255},0.0f}); // Noon
        timeline.push_back({17.5f, {50, 60, 120, 255}, {230, 90, 40, 255},  {140, 90, 80, 255}, 0.1f}); // Golden Hour
        timeline.push_back({19.0f, {20, 25, 60, 255},  {75, 30, 90, 255},   {50, 40, 65, 255},  0.6f}); // Dusk
        timeline.push_back({24.0f, {10, 10, 25, 255},  {5, 5, 15, 255},    {20, 20, 35, 255},  1.0f}); // Loop Close
    }

    void GenerateStarfield(int count) {
        for (int i = 0; i < count; i++) {
            starfield.push_back({
                {(float)GetRandomValue(0, screenWidth), (float)GetRandomValue(0, (int)(screenHeight * 0.65f))},
                (float)GetRandomValue(10, 25) / 10.0f, // Scale size precision
                (float)GetRandomValue(50, 255) / 255.0f,
                (float)GetRandomValue(0, 360) * (PI / 180.0f), // Random start position in sine cycle
                (float)GetRandomValue(2, 6) // Twinkle frequency
            });
        }
    }

public:
    CelestialEngine(int width, int height, float startHour, float speed) 
        : screenWidth(width), screenHeight(height), currentHour(startHour), cycleSpeed(speed) {
        
        pivotCenter = { screenWidth / 2.0f, (float)screenHeight + 100.0f };
        orbitRadius = screenHeight * 0.95f;

        SetupAtmosphereTimeline();
        GenerateStarfield(250); // High-density procedural background
    }

    void Update(float deltaTime) {
        // Increment time based on real seconds elapsed
        currentHour += (deltaTime * cycleSpeed);
        if (currentHour >= 24.0f) currentHour -= 24.0f;
    }

    // Dynamic State Interpolator
    AtmosphereKeyframe GetCurrentAtmosphere() {
        for (size_t i = 0; i < timeline.size() - 1; i++) {
            if (currentHour >= timeline[i].hour && currentHour <= timeline[i+1].hour) {
                float segmentLength = timeline[i+1].hour - timeline[i].hour;
                float t = (currentHour - timeline[i].hour) / segmentLength;
                
                return AtmosphereKeyframe{
                    currentHour,
                    LerpColor(timeline[i].zenithColor, timeline[i+1].zenithColor, t),
                    LerpColor(timeline[i].horizonColor, timeline[i+1].horizonColor, t),
                    LerpColor(timeline[i].ambientLight, timeline[i+1].ambientLight, t),
                    timeline[i].starVisibility + (timeline[i+1].starVisibility - timeline[i].starVisibility) * t
                };
            }
        }
        return timeline[0];
    }

    void Render() {
        AtmosphereKeyframe env = GetCurrentAtmosphere();

        // 1. Draw High-Precision Horizon Gradient (Atmosphere Skybox)
        DrawRectangleGradientV(0, 0, screenWidth, screenHeight, env.zenithColor, env.horizonColor);

        // 2. Render Twinkling Stars
        if (env.starVisibility > 0.01f) {
            float timeRunning = GetTime();
            for (const auto& star : starfield) {
                // Precise math calculation for pulse twinkle cycle
                float wave = sinf(timeRunning * star.twinkleSpeed + star.twinklePhase);
                float finalAlpha = star.brightness * (0.4f + 0.6f * wave) * env.starVisibility;
                
                if (finalAlpha > 0.0f) {
                    DrawCircleV(star.position, star.baseRadius, ColorAlpha(WHITE, finalAlpha));
                }
            }
        }

        // 3. Mathematical Orbit Trajectory Calculation (Radians)
        // Offset mapping: 6.0 (Sunrise) is at Angle PI (Left horizon)
        float solarAngle = ((currentHour - 6.0f) / 24.0f) * 2.0f * PI;

        Vector2 sunPos = {
            pivotCenter.x + cosf(solarAngle) * orbitRadius,
            pivotCenter.y + sinf(solarAngle) * orbitRadius
        };

        Vector2 moonPos = {
            pivotCenter.x + cosf(solarAngle + PI) * orbitRadius,
            pivotCenter.y + sinf(solarAngle + PI) * orbitRadius
        };

        // 4. Render Sun with Atmospheric Flare Halo
        if (sunPos.y < screenHeight + 50) {
            DrawCircleV(sunPos, 55.0f, ColorAlpha(ORANGE, 0.2f)); // Outer Flare
            DrawCircleV(sunPos, 45.0f, ColorAlpha(YELLOW, 0.5f)); // Inner Glow
            DrawCircleV(sunPos, 35.0f, RAYWHITE);                // Core Cornea
        }

        // 5. Render Architectural Moon Crescent 
        if (moonPos.y < screenHeight + 50) {
            DrawCircleV(moonPos, 28.0f, ColorAlpha(LIGHTGRAY, 0.9f));
            // Mathematical overlay to create crisp structural moon phase cutout
            Vector2 maskOffset = { moonPos.x + 9.0f, moonPos.y - 4.0f };
            // Mix with zenith sky color interpolator to make masking perfect
            Color currentSkyAtMoonHeight = LerpColor(env.zenithColor, env.horizonColor, moonPos.y / screenHeight);
            DrawCircleV(maskOffset, 26.0f, currentSkyAtMoonHeight);
        }

        // 6. Draw Environment Geometry affected by Dynamic Ambient Shadows
        Color layer1Color = LerpColor(DARKGREEN, Color{10, 35, 15, 255}, (24.0f - currentHour)/24.0f);
        Color layer2Color = LerpColor(GREEN,     Color{15, 55, 20, 255}, (24.0f - currentHour)/24.0f);

        // Background Layer Mountains
        DrawTriangle({-100, (float)screenHeight}, {350, (float)screenHeight - 300}, {800, (float)screenHeight}, LerpColor(layer1Color, BLACK, 0.3f));
        DrawTriangle({400, (float)screenHeight}, {850, (float)screenHeight - 350}, {1400, (float)screenHeight}, LerpColor(layer1Color, BLACK, 0.2f));
        
        // Foreground Grass Deck
        DrawRectangle(0, screenHeight - 120, screenWidth, 120, env.ambientLight == WHITE ? GREEN : LerpColor(layer2Color, env.ambientLight, 0.4f));

        // 7. Core HUD Interface Overlay Data display
        int displayHours = (int)currentHour;
        int displayMinutes = (int)((currentHour - displayHours) * 60.0f);
        std::string timeString = (displayHours < 10 ? "0" : "") + std::to_string(displayHours) + ":" + (displayMinutes < 10 ? "0" : "") + std::to_string(displayMinutes);
        
        DrawRectangle(15, 15, 240, 95, ColorAlpha(BLACK, 0.6f));
        DrawRectangleLines(15, 15, 240, 95, ColorAlpha(WHITE, 0.3f));
        DrawText(("SIMULATION TIME: " + timeString).c_str(), 30, 30, 16, GREEN);
        DrawText(TextFormat("TIME SPEED multiplier: %.1fx", cycleSpeed * 3600.0f), 30, 55, 13, LIGHTGRAY);
        DrawText("Controls: UP/DOWN changes speed", 30, 78, 11, GRAY);
    }

    void AdjustSpeed(float amount) {
        cycleSpeed += amount;
        if (cycleSpeed < 0.001f) cycleSpeed = 0.001f;
    }
};

// =====================================================================================
// APPLICATION ENTRY POINT
// =====================================================================================
int main() {
    const int width = 1280;
    const int height = 760;
    
    SetConfigFlags(FLAG_MSAA_4X_HINT); // Enable Anti-Aliasing for smooth celestial shapes
    InitWindow(width, height, "Advanced Time-Engine Sandbox architecture v2.0");

    // Params: Width, Height, Start Hour (06.00 = Morning), Multiplier Speed
    // 0.005f translates to roughly 1 hour passing every few real seconds
    CelestialEngine dayNightSystem(width, height, 6.0f, 0.005f);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // Input adjustments hooks
        if (IsKeyDown(KEY_UP))   dayNightSystem.AdjustSpeed(0.001f);
        if (IsKeyDown(KEY_DOWN)) dayNightSystem.AdjustSpeed(-0.001f);

        // Core Pipeline Execution
        dayNightSystem.Update(GetFrameTime());

        BeginDrawing();
            dayNightSystem.Render();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
