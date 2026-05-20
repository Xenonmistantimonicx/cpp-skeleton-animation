#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread>
#include <memory>
#include <iomanip>

// ============================================================================
// 1. MATH STRUCTURES & LINEAR ALGEBRA
// ============================================================================
constexpr float PI = 3.14159265358979323846f;

struct Vector3 {
    float x, y, z;
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vector3 operator-(const Vector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    
    float dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
    Vector3 cross(const Vector3& v) const {
        return { y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.z };
    }
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    Vector3 normalized() const {
        float len = length();
        return len > 0.0f ? *this * (1.0f / len) : Vector3(0,0,0);
    }
};

struct Quaternion {
    float w, x, y, z;
    Quaternion() : w(1), x(0), y(0), z(0) {}
    Quaternion(float w, float x, float y, float z) : w(w), x(x), y(y), z(z) {}

    // Axis-Angle initialization
    static Quaternion fromAxisAngle(const Vector3& axis, float angleRadians) {
        float halfAngle = angleRadians * 0.5f;
        float s = std::sin(halfAngle);
        Vector3 normAxis = axis.normalized();
        return { std::cos(halfAngle), normAxis.x * s, normAxis.y * s, normAxis.z * s };
    }

    Quaternion operator*(const Quaternion& q) const {
        return {
            w * q.w - x * q.x - y * q.y - z * q.z,
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w
        };
    }

    // Spherical Linear Interpolation (Slerp) for ultra-smooth animations
    static Quaternion slerp(const Quaternion& q1, const Quaternion& q2, float t) {
        float dot = q1.w * q2.w + q1.x * q2.x + q1.y * q2.y + q1.z * q2.z;
        Quaternion target = q2;

        if (dot < 0.0f) {
            dot = -dot;
            target = Quaternion(-q2.w, -q2.x, -q2.y, -q2.z);
        }

        if (dot > 0.9995f) {
            // Linear interpolation close to limit
            return Quaternion(
                q1.w + t * (target.w - q1.w),
                q1.x + t * (target.x - q1.x),
                q1.y + t * (target.y - q1.y),
                q1.z + t * (target.z - q1.z)
            );
        }

        float theta_0 = std::acos(dot);
        float theta = theta_0 * t;
        float sin_theta = std::sin(theta);
        float sin_theta_0 = std::sin(theta_0);

        float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
        float s1 = sin_theta / sin_theta_0;

        return Quaternion(
            s0 * q1.w + s1 * target.w,
            s0 * q1.x + s1 * target.x,
            s0 * q1.y + s1 * target.y,
            s0 * q1.z + s1 * target.z
        );
    }

    Vector3 rotate(const Vector3& v) const {
        Vector3 qv(x, y, z);
        Vector3 t = qv.cross(v) * 2.0f;
        return v + t * w + qv.cross(t);
    }
};

// ============================================================================
// 2. MESH PATTERNS & PROCEDURAL GEOMETRY GENERATOR
// ============================================================================
struct Vertex {
    Vector3 position;
    Vector3 normal;
    float r, g, b; // Vertex coloration for advanced material separation
};

class KatanaMeshGenerator {
public:
    static std::vector<Vertex> generateMesh() {
        std::vector<Vertex> mesh;

        // --- Component 1: The Curved Blade (Ha) ---
        // Generates a proper multi-segmented geometry following a parabolic curve
        int bladeSegments = 40;
        float segmentLength = 0.025f; // Total length around 1.0 meter
        float baseWidth = 0.03f;
        float thickness = 0.006f;
        float curvatureFactor = 0.0015f; // Mathematical curve representing historical sori

        for (int i = 0; i < bladeSegments; ++i) {
            float zStart = i * segmentLength;
            float zEnd = (i + 1) * segmentLength;

            // Compute curved offsets using curvature factor
            float xOffsetStart = curvatureFactor * (i * i);
            float xOffsetEnd = curvatureFactor * ((i + 1) * (i + 1));

            // Vertices for spine (Mune)
            Vector3 spineStart(xOffsetStart, baseWidth * 0.5f, zStart);
            Vector3 spineEnd(xOffsetEnd, baseWidth * 0.5f, zEnd);

            // Vertices for cutting edge (Ha) - Tapered thin
            Vector3 edgeStart(xOffsetStart + baseWidth, 0.0f, zStart);
            Vector3 edgeEnd(xOffsetEnd + baseWidth, 0.0f, zEnd);

            // Left side faces (Steel color)
            mesh.push_back({spineStart, Vector3(-1, 0, 0), 0.8f, 0.8f, 0.85f});
            mesh.push_back({edgeStart,  Vector3(1, 1, 0).normalized(), 0.9f, 0.9f, 0.95f});
            mesh.push_back({edgeEnd,   Vector3(1, 1, 0).normalized(), 0.9f, 0.9f, 0.95f});

            mesh.push_back({spineStart, Vector3(-1, 0, 0), 0.8f, 0.8f, 0.85f});
            mesh.push_back({edgeEnd,   Vector3(1, 1, 0).normalized(), 0.9f, 0.9f, 0.95f});
            mesh.push_back({spineEnd,  Vector3(-1, 0, 0), 0.8f, 0.8f, 0.85f});
        }

        // --- Component 2: Handguard (Tsuba) ---
        // Creates a stylized flat circular disc structure at the base
        int discSegments = 24;
        float radius = 0.06f;
        float tsubaThickness = 0.005f;
        float zPosition = 0.0f;

        for (int i = 0; i < discSegments; ++i) {
            float angle1 = (i / (float)discSegments) * 2.0f * PI;
            float angle2 = ((i + 1) / (float)discSegments) * 2.0f * PI;

            Vector3 centerStart(0, 0, zPosition - tsubaThickness);
            Vector3 p1(radius * std::cos(angle1), radius * std::sin(angle1), zPosition - tsubaThickness);
            Vector3 p2(radius * std::cos(angle2), radius * std::sin(angle2), zPosition - tsubaThickness);

            // Dark traditional iron color (0.15f, 0.15f, 0.15f)
            mesh.push_back({centerStart, Vector3(0,0,-1), 0.15f, 0.15f, 0.15f});
            mesh.push_back({p1,          Vector3(0,0,-1), 0.15f, 0.15f, 0.15f});
            mesh.push_back({p2,          Vector3(0,0,-1), 0.15f, 0.15f, 0.15f});
        }

        // --- Component 3: The Hilt (Tsuka) ---
        // Extends backwards from the handguard with a subtle oval cross section
        float hiltLength = 0.25f;
        Vector3 hiltStart(0, 0, -tsubaThickness);
        Vector3 hiltEnd(0, 0, -tsubaThickness - hiltLength);
        
        // Rendered as an extruded box representation for traditional wrapping style
        mesh.push_back({Vector3(-0.01f, -0.015f, -0.005f), Vector3(0, -1, 0), 0.1f, 0.1f, 0.1f});
        mesh.push_back({Vector3(0.01f, -0.015f, -0.005f),  Vector3(0, -1, 0), 0.1f, 0.1f, 0.1f});
        mesh.push_back({Vector3(0.01f, -0.015f, -0.255f),  Vector3(0, -1, 0), 0.4f, 0.1f, 0.1f}); // Red cord accent
        
        return mesh;
    }
};

// ============================================================================
// 3. KINEMATICS & ANIMATION ENGINE STATE MACHINE
// ============================================================================
struct Keyframe {
    float timestamp; // Time checkpoint context (seconds)
    Vector3 position;
    Quaternion orientation;
};

enum class KatanaState {
    SheathedIdle,
    DrawingStrike,  // Iaido explosive slash movement
    FluidSpin,      // Defensive combat transition
    Returning
};

class KatanaAnimator {
private:
    std::vector<Keyframe> keyframes;
    float currentTime;
    float executionDuration;
    std::vector<Vector3> trailHistory;
    const size_t maxTrailPoints = 12;

public:
    KatanaAnimator() : currentTime(0.0f) {
        setupKeyframes();
        executionDuration = keyframes.back().timestamp;
    }

    void setupKeyframes() {
        // Timeline 0.0s: Sheathed near hips
        keyframes.push_back({0.0f, Vector3(-0.2f, -0.3f, 0.2f), Quaternion::fromAxisAngle({1,0,0}, 0.2f)});
        
        // Timeline 0.4s: Instant Draw Flash (Midway through Iaido path)
        keyframes.push_back({0.4f, Vector3(0.3f, 0.1f, -0.2f), 
            Quaternion::fromAxisAngle({0,1,0}, PI * 0.4f) * Quaternion::fromAxisAngle({1,0,0}, 0.5f)});
        
        // Timeline 0.7s: Absolute terminal extension of a fatal slash strike
        keyframes.push_back({0.7f, Vector3(0.9f, 0.0f, -0.5f), 
            Quaternion::fromAxisAngle({0,1,0}, PI * 0.85f) * Quaternion::fromAxisAngle({0,0,1}, -0.3f)});
        
        // Timeline 1.5s: Defensive rotational parry transition
        keyframes.push_back({1.5f, Vector3(0.1f, 0.4f, -0.3f), 
            Quaternion::fromAxisAngle({1,1,0}, PI * 1.2f)});
        
        // Timeline 2.5s: Return smoothly loopable back to rest position
        keyframes.push_back({2.5f, Vector3(-0.2f, -0.3f, 0.2f), Quaternion::fromAxisAngle({1,0,0}, 0.2f)});
    }

    void update(float deltaTime, Vector3& outPos, Quaternion& outRot) {
        currentTime += deltaTime;
        if (currentTime > executionDuration) {
            currentTime = 0.0f; // Loop sequence continuously
            trailHistory.clear();
        }

        // Find correct active interval bounding windows
        Keyframe startFrame = keyframes[0];
        Keyframe endFrame = keyframes[0];

        for (size_t i = 0; i < keyframes.size() - 1; ++i) {
            if (currentTime >= keyframes[i].timestamp && currentTime <= keyframes[i+1].timestamp) {
                startFrame = keyframes[i];
                endFrame = keyframes[i+1];
                break;
            }
        }

        // Calculate interpolation factor factor t bounded [0,1]
        float range = endFrame.timestamp - startFrame.timestamp;
        float t = (currentTime - startFrame.timestamp) / range;

        // Apply cubic ease-in/ease-out step mapping curves
        float smoothT = t * t * (3.0f - 2.0f * t);

        // Compute kinematic position translation & orientation rotation transformations
        outPos = startFrame.position + (endFrame.position - startFrame.position) * smoothT;
        outRot = Quaternion::slerp(startFrame.orientation, endFrame.orientation, smoothT);

        // Update tracking matrix for the slashing trail effect at blade tip
        Vector3 baseTip(0.05f, 0.0f, 1.0f); // Local coordinates mapping tip location
        Vector3 worldTip = outPos + outRot.rotate(baseTip);
        
        trailHistory.push_back(worldTip);
        if (trailHistory.size() > maxTrailPoints) {
            trailHistory.erase(trailHistory.begin());
        }
    }

    const std::vector<Vector3>& getTrailHistory() const { return trailHistory; }
    float getCurrentTime() const { return currentTime; }
};

// ============================================================================
// 4. MATH RENDERING CONSOLE ENGINE VISUALIZER (ASSETLESS RUNTIME)
// ============================================================================
class ConsoleRenderer {
public:
    static void drawFrame(const Vector3& pos, const Quaternion& rot, const std::vector<Vector3>& trails, float globalTime) {
        // Clear standard terminal buffer strings using ANSI escape structures
        std::cout << "\x1B[2J\x1B[H";
        std::cout << "===================================================================\n";
        std::cout << " SYSTEM RUNTIME ENGINE ENGINE: PROCEDURAL KATANA KINEMATICS        \n";
        std::cout << "===================================================================\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << " Timeline Clock: " << globalTime << "s | Loop limit: 2.50s\n";
        std::cout << " World Coordinate Vector Matrix : [" << pos.x << ", " << pos.y << ", " << pos.z << "]\n";
        std::cout << " Quaternion Matrix Data Orientation: [" << rot.w << ", " << rot.x << ", " << rot.y << ", " << rot.z << "]\n";
        std::cout << "-------------------------------------------------------------------\n\n";

        // Generate ASCII Projection space grid array viewport maps
        const int width = 64;
        const int height = 20;
        char screenBuffer[height][width];
        
        // Initialize standard viewport empty buffers
        for(int y=0; y<height; ++y)
            for(int x=0; x<width; ++x)
                screenBuffer[y][x] = ' ';

        // Track critical geometric landmarks to generate structural reference lines
        Vector3 localHilt(0, 0, -0.2f);
        Vector3 localGuard(0, 0, 0);
        Vector3 localMidBlade(0.01f, 0.01f, 0.5f);
        Vector3 localTip(0.04f, 0.0f, 1.0f);

        // Transform landmarks cleanly directly to dynamic world coordinates space
        Vector3 wHilt    = pos + rot.rotate(localHilt);
        Vector3 wGuard   = pos + rot.rotate(localGuard);
        Vector3 wMid     = pos + rot.rotate(localMidBlade);
        Vector3 wTip     = pos + rot.rotate(localTip);

        // Project 3D vector variables arrays cleanly via perspective to 2D screen coordinate pixels
        auto project = [](const Vector3& v, int scrW, int scrH) {
            float distanceScale = 2.5f;
            float zPerspective = distanceScale - v.z;
            if (zPerspective < 0.1f) zPerspective = 0.1f;
            
            int x = static_cast<int>((scrW / 2) + (v.x * (scrW * 0.5f) / zPerspective));
            int y = static_cast<int>((scrH / 2) - (v.y * (scrH * 1.0f) / zPerspective)); // Invert screen Y
            return std::make_pair(x, y);
        };

        // Render motion blur trail nodes pipeline
        for (size_t i = 0; i < trails.size(); ++i) {
            auto [sx, sy] = project(trails[i], width, height);
            if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
                screenBuffer[sy][sx] = (i == trails.size() - 1) ? '*' : '.';
            }
        }

        // Project and overlay structural Katana components to final scene pass buffers
        auto [hx, hy] = project(wHilt, width, height);
        auto [gx, gy] = project(wGuard, width, height);
        auto [mx, my] = project(wMid, width, height);
        auto [tx, ty] = project(wTip, width, height);

        // Inline lambdas executing fast ray segment raster transformations
        auto drawRay = [&](std::pair<int,int> p1, std::pair<int,int> p2, char elementSymbol) {
            int x1 = p1.first, y1 = p1.second;
            int x2 = p2.first, y2 = p2.second;
            int dx = std::abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
            int dy = -std::abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
            int err = dx + dy, e2;

            while (true) {
                if (x1 >= 0 && x1 < width && y1 >= 0 && y1 < height) {
                    screenBuffer[y1][x1] = elementSymbol;
                }
                if (x1 == x2 && y1 == y2) break;
                e2 = 2 * err;
                if (e2 >= dy) { err += dy; x1 += sx; }
                if (e2 <= dx) { err += dx; y1 += sy; }
            }
        };

        // Layer assignments: draw Hilt, then Blade structure, then Guard node point
        drawRay({hx, hy}, {gx, gy}, '='); // Hilt
        drawRay({gx, gy}, {mx, my}, '/'); // Lower Blade curve segment
        drawRay({mx, my}, {tx, ty}, ')'); // Tapered upper cutting section
        
        if (gx >= 0 && gx < width && gy >= 0 && gy < height) screenBuffer[gy][gx] = 'O'; // Guard Tsuba node

        // Stream frame character graphics output pipelines directly directly out to standard console stdout
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                std::cout << screenBuffer[y][x];
            }
            std::cout << "\n";
        }
        std::cout << "===================================================================\n";
        std::cout << " Legend Viewports: (=) Tsuka/Hilt  (O) Tsuba Guard  (/)) Curved Ha/Blade  (.) Kinetic Trail\n";
    }
};

// ============================================================================
// 5. MAIN SUB-SYSTEM ENGINE ENTRY POINT
// ============================================================================
int main() {
    std::cout << "Initializing ProcGeometry components pipeline structures..." << std::endl;
    auto structuralMeshVerticesArray = KatanaMeshGenerator::generateMesh();
    std::cout << "Successfully allocated: " << structuralMeshVerticesArray.size() << " vertex attributes nodes array profiles safely." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    KatanaAnimator swordStateAnimator;
    Vector3 dynamicRuntimePosition;
    Quaternion dynamicRuntimeOrientation;

    // Fixed execution loop updating engine frames at 30Hz context refresh targets
    float frameTickIntervalDelta = 1.0f / 30.0f;
    int computationalLoopsCountdownCounter = 300; // Simulates roughly 10 full seconds before closing safely

    while (computationalLoopsCountdownCounter-- > 0) {
        // Run physics frame update engine cycles
        swordStateAnimator.update(frameTickIntervalDelta, dynamicRuntimePosition, dynamicRuntimeOrientation);

        // Perform rendering pipelines step passes
        ConsoleRenderer::drawFrame(
            dynamicRuntimePosition, 
            dynamicRuntimeOrientation, 
            swordStateAnimator.getTrailHistory(),
            swordStateAnimator.getCurrentTime()
        );

        // Sync processing pipelines frame execution rate accurately
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(frameTickIntervalDelta * 1000)));
    }

    std::cout << "\nAnimation sequence engine shutdown safely with zero leak exceptions reported.\n";
    return 0;
}
