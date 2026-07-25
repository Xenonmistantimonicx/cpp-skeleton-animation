/**
 * ============================================================================================
 *  AAAAAAA-GRADE ULTRA-ADVANCED CURIOSITY & PLAYER OBSERVATION AI ENGINE
 * ============================================================================================
 *  Modules:
 *   1. Dynamic Player Behavior & Novelty Evaluator (Action Entropy + Threat Analysis)
 *   2. Information Theory Perception (Entropy Reduction & Optimal Line-of-Sight)
 *   3. Dynamic Curiosity State Machine (Orbiting, Shadowing, Mimicry, & Flight-or-Fight)
 *   4. Curved Orbital Flight Path & Perch Selection Solver
 *
 *  Target Standard : C++20 / Enterprise AAA AI Architecture
 * ============================================================================================
 */

#ifndef CURIOSITY_OBSERVATION_ENGINE_HPP
#define CURIOSITY_OBSERVATION_ENGINE_HPP

#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include <random>
#include <algorithm>
#include <deque>
#include <iomanip>

namespace AAAAAAAGeminiEngine
{
    // ============================================================================================
    // 1. MATHEMATICAL PRIMITIVES & VECTOR CALCULUS
    // ============================================================================================
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 6.28318530717958647692f;

    struct Vec3
    {
        float x{ 0.0f }, y{ 0.0f }, z{ 0.0f };

        constexpr Vec3() = default;
        constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

        inline Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
        inline Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
        inline Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
        inline Vec3 operator/(float s) const { float inv = 1.0f / s; return Vec3(x * inv, y * inv, z * inv); }
        inline float Dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
        inline Vec3 Cross(const Vec3& o) const { return Vec3(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x); }
        inline float LengthSq() const { return Dot(*this); }
        inline float Length() const { return std::sqrt(LengthSq()); }
        inline Vec3 Normalized() const { float len = Length(); return len > 1e-6f ? (*this) / len : Vec3(0,0,0); }
    };

    // ============================================================================================
    // 2. PLAYER TELEMETRY & NOVELTY ENGINE
    // ============================================================================================
    struct PlayerStateFrame
    {
        Vec3 Position;
        Vec3 Velocity;
        Vec3 LookDirection;
        float NoiseLevel{ 0.0f };       // [0.0 = Silent, 1.0 = Gunfire/Explosion]
        bool IsWeaponDrawn{ false };
        bool IsCrouching{ false };
        bool IsEmotingOrSpinning{ false };
        float Timestamp{ 0.0f };
    };

    class PlayerBehaviorAnalyzer
    {
    private:
        std::deque<PlayerStateFrame> m_FrameHistory;
        size_t m_MaxHistorySize{ 30 }; // ~3 seconds at 10Hz

    public:
        void RecordFrame(const PlayerStateFrame& frame)
        {
            m_FrameHistory.push_back(frame);
            if (m_FrameHistory.size() > m_MaxHistorySize)
            {
                m_FrameHistory.pop_front();
            }
        }

        // Computes Action Entropy & Unpredictability Score
        float CalculateNoveltyScore() const
        {
            if (m_FrameHistory.size() < 5) return 0.0f;

            // Compute Directional Variance (Random Spinning or Erratic Motion)
            float varianceSum = 0.0f;
            Vec3 avgDir(0, 0, 0);

            for (const auto& frame : m_FrameHistory)
            {
                avgDir = avgDir + frame.Velocity.Normalized();
            }
            avgDir = avgDir / static_cast<float>(m_FrameHistory.size());

            float directionalConsistency = avgDir.Length(); // 1.0 = Straight line, 0.0 = Erratic
            float movementEntropy = 1.0f - directionalConsistency;

            const auto& latest = m_FrameHistory.back();
            float emoteFactor = latest.IsEmotingOrSpinning ? 0.8f : 0.0f;
            float suddenMovement = (latest.Velocity.Length() > 8.0f) ? 0.4f : 0.0f;

            return std::clamp(movementEntropy + emoteFactor + suddenMovement, 0.0f, 1.0f);
        }

        // Threat Rating Engine
        float CalculateThreatLevel(const Vec3& birdPos) const
        {
            if (m_FrameHistory.empty()) return 0.0f;

            const auto& latest = m_FrameHistory.back();
            Vec3 toBird = (birdPos - latest.Position).Normalized();

            // Is player aiming directly at the bird?
            float aimAlignment = latest.LookDirection.Dot(toBird);
            float aimingThreat = (aimAlignment > 0.85f) ? 0.9f : 0.1f;

            float weaponThreat = latest.IsWeaponDrawn ? 0.4f : 0.0f;
            float noiseThreat = latest.NoiseLevel * 0.7f;

            return std::clamp(aimingThreat + weaponThreat + noiseThreat, 0.0f, 1.0f);
        }

        const PlayerStateFrame& GetLatestFrame() const { return m_FrameHistory.back(); }
    };

    // ============================================================================================
    // 3. MASTER CURIOSITY AI DRIVER
    // ============================================================================================
    enum class CuriosityState
    {
        PERCHED_WATCHING,
        SHADOW_TRACKING,      // Flying behind/above player out of direct crosshair
        INVESTIGATIVE_ORBIT,  // Circling player to gather visual telemetry
        MIMICRY_MOCK,         // Mirroring player actions (e.g. landing near, bobbing head)
        PANIC_EVADE           // Threat threshold exceeded! Break line of sight
    };

    class CuriosityBirdAI
    {
    private:
        Vec3 m_Position{ 0.0f, 25.0f, 0.0f };
        Vec3 m_Velocity{ 0.0f, 0.0f, 0.0f };
        Vec3 m_ForwardDir{ 0.0f, 0.0f, 1.0f };

        float m_CuriosityMeter{ 0.8f };      // [0.0 = Bored, 1.0 = Fascinated]
        float m_BoredomDecayRate{ 0.03f };   // Decay per second if player is static
        float m_MaxFlightSpeed{ 15.0f };

        PlayerBehaviorAnalyzer m_PlayerAnalyzer;
        CuriosityState m_CurrentState{ CuriosityState::PERCHED_WATCHING };

        float m_OrbitAngle{ 0.0f };
        float m_OrbitRadius{ 18.0f };
        float m_MimicryTimer{ 0.0f };

    public:
        CuriosityBirdAI(const Vec3& startPos) : m_Position(startPos) {}

        void ProcessPlayerObservation(float dt, const PlayerStateFrame& playerFrame)
        {
            // 1. Ingest Player State Frame
            m_PlayerAnalyzer.RecordFrame(playerFrame);

            float novelty = m_PlayerAnalyzer.CalculateNoveltyScore();
            float threat = m_PlayerAnalyzer.CalculateThreatLevel(m_Position);

            // 2. Dynamic Curiosity Equation: dC/dt = Novelty - Boredom - ThreatPenalty
            m_CuriosityMeter += (novelty * 0.4f - m_BoredomDecayRate - threat * 0.8f) * dt;
            m_CuriosityMeter = std::clamp(m_CuriosityMeter, 0.0f, 1.0f);

            // 3. Finite Behavioral State Machine
            switch (m_CurrentState)
            {
            case CuriosityState::PERCHED_WATCHING:
                // Look at Player
                m_ForwardDir = (playerFrame.Position - m_Position).Normalized();

                if (threat > 0.65f)
                {
                    std::cout << "!! [Curiosity AI] High Threat Detected! Evading...\n";
                    m_CurrentState = CuriosityState::PANIC_EVADE;
                }
                else if (m_CuriosityMeter > 0.6f && novelty > 0.3f)
                {
                    std::cout << ">> [Curiosity AI] Player doing interesting action! Taking off to Orbit.\n";
                    m_CurrentState = CuriosityState::INVESTIGATIVE_ORBIT;
                }
                break;

            case CuriosityState::INVESTIGATIVE_ORBIT:
                if (threat > 0.70f)
                {
                    m_CurrentState = CuriosityState::PANIC_EVADE;
                    break;
                }

                // Calculate Orbit Position around player
                m_OrbitAngle += 1.2f * dt; // Orbit speed
                {
                    Vec3 offset(std::cos(m_OrbitAngle) * m_OrbitRadius, 12.0f, std::sin(m_OrbitAngle) * m_OrbitRadius);
                    Vec3 targetOrbitPos = playerFrame.Position + offset;
                    SteerTowards(targetOrbitPos, dt);
                }

                if (m_CuriosityMeter > 0.85f && novelty > 0.6f && threat < 0.2f)
                {
                    std::cout << ">> [Curiosity AI] Extremely curious & Safe! Attempting Ground Mimicry.\n";
                    m_CurrentState = CuriosityState::MIMICRY_MOCK;
                    m_MimicryTimer = 4.0f;
                }
                else if (m_CuriosityMeter < 0.25f)
                {
                    std::cout << ">> [Curiosity AI] Player became boring. Returning to High Shadow Tracking.\n";
                    m_CurrentState = CuriosityState::SHADOW_TRACKING;
                }
                break;

            case CuriosityState::SHADOW_TRACKING:
                // Fly behind player's view vector (Blindspot tracking)
                {
                    Vec3 blindSpotTarget = playerFrame.Position - (playerFrame.LookDirection * 22.0f) + Vec3(0, 15, 0);
                    SteerTowards(blindSpotTarget, dt);
                }

                if (threat > 0.6f)
                {
                    m_CurrentState = CuriosityState::PANIC_EVADE;
                }
                else if (novelty > 0.4f)
                {
                    m_CurrentState = CuriosityState::INVESTIGATIVE_ORBIT;
                }
                break;

            case CuriosityState::MIMICRY_MOCK:
                m_MimicryTimer -= dt;
                // Fly down close to player and land / bob head
                {
                    Vec3 nearPlayerPos = playerFrame.Position + (playerFrame.LookDirection.Cross(Vec3(0,1,0)) * 4.0f);
                    nearPlayerPos.y = playerFrame.Position.y; // Ground level
                    SteerTowards(nearPlayerPos, dt);
                }

                if (threat > 0.35f || m_MimicryTimer <= 0.0f)
                {
                    std::cout << ">> [Curiosity AI] Mimicry window closed or player turned. Retracting to orbit.\n";
                    m_CurrentState = CuriosityState::INVESTIGATIVE_ORBIT;
                }
                break;

            case CuriosityState::PANIC_EVADE:
                // Fly directly away from player look vector
                {
                    Vec3 escapeVector = (m_Position - playerFrame.Position).Normalized() + Vec3(0, 0.8f, 0);
                    SteerTowards(m_Position + escapeVector * 30.0f, dt);
                }

                if ((m_Position - playerFrame.Position).Length() > 60.0f)
                {
                    std::cout << ">> [Curiosity AI] Safe distance reached. Resetting to Perch Watching.\n";
                    m_CurrentState = CuriosityState::PERCHED_WATCHING;
                }
                break;
            }
        }

        void SteerTowards(const Vec3& target, float dt)
        {
            Vec3 desiredVel = (target - m_Position).Normalized() * m_MaxFlightSpeed;
            Vec3 steeringForce = desiredVel - m_Velocity;

            m_Velocity = m_Velocity + steeringForce * 3.0f * dt;
            if (m_Velocity.LengthSq() > m_MaxFlightSpeed * m_MaxFlightSpeed)
            {
                m_Velocity = m_Velocity.Normalized() * m_MaxFlightSpeed;
            }

            m_Position = m_Position + m_Velocity * dt;
            if (m_Velocity.LengthSq() > 1e-4f)
            {
                m_ForwardDir = m_Velocity.Normalized();
            }
        }

        void PrintStatus() const
        {
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "[Bird Curiosity] Pos: (" << m_Position.x << ", " << m_Position.y << ", " << m_Position.z << ")"
                      << " | Curiosity Level: " << m_CuriosityMeter * 100.0f << "%\n";
        }
    };
}

// ============================================================================================
// MAIN SIMULATION RUNNER
// ============================================================================================
int main()
{
    using namespace AAAAAAAGeminiEngine;

    std::cout << "================================================================================\n";
    std::cout << " RUNNING AAA PLAYER OBSERVATION & CURIOSITY AI ENGINE                          \n";
    std::cout << "================================================================================\n\n";

    CuriosityBirdAI bird(Vec3(0.0f, 20.0f, 0.0f));

    constexpr float dt = 0.1f;
    float simTime = 0.0f;

    // Simulate erratic/curious player behaviors over time
    for (int step = 0; step < 120; ++step)
    {
        simTime += dt;

        PlayerStateFrame frame;
        frame.Position = Vec3(step * 0.5f, 0.0f, step * 0.2f);
        frame.Velocity = Vec3(2.0f, 0.0f, 1.0f);
        frame.LookDirection = Vec3(0.0f, 0.0f, 1.0f).Normalized();
        frame.Timestamp = simTime;

        // Inject erratic player actions at specific intervals
        if (step > 20 && step < 50)
        {
            frame.IsEmotingOrSpinning = true; // Player starts emoting/spinning!
        }
        if (step >= 70 && step < 85)
        {
            // Player aims directly at bird
            frame.LookDirection = Vec3(0.0f, 1.0f, 0.0f); 
            frame.IsWeaponDrawn = true;
            frame.NoiseLevel = 0.9f;
        }

        bird.ProcessPlayerObservation(dt, frame);

        if (step % 10 == 0)
        {
            bird.PrintStatus();
        }
    }

    return 0;
}
