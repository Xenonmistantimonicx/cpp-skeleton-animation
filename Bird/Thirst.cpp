/**
 * ============================================================================================
 *  AAAAAAA-GRADE ULTRA-ADVANCED WATER FORAGING & HYDRATION ENGINE
 * ============================================================================================
 *  Modules:
 *   1. Dynamic Hydration & Physiological Debuff System
 *   2. Multi-Sensory Environment Sensing Engine (Olfactory Wind Gradients, Acoustics, Vision)
 *   3. Hydrological Quality & Risk Utility Evaluator (Purity vs. Ambush Risk vs. Distance)
 *   4. Wind-Vector Olfactory Trailing Algorithm (Surfactant/Humidity Navigation)
 *   5. State-Driven Safe Approach & Drinking Behavior Machine
 *
 *  Target Standard : C++20 / Enterprise AAA AI Architecture
 * ============================================================================================
 */

#ifndef THIRST_WATER_FORAGING_ENGINE_HPP
#define THIRST_WATER_FORAGING_ENGINE_HPP

#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include <random>
#include <algorithm>
#include <optional>
#include <string>
#include <iomanip>

namespace AAAAAAAGeminiEngine
{
    // ============================================================================================
    // 1. MATHEMATICAL PRIMITIVES & VECTOR OPERATIONS
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

    struct EnvironmentalWind
    {
        Vec3 Direction{ 1.0f, 0.0f, 0.5f }; // Primary wind vector
        float Speed{ 6.5f };                // m/s
    };

    // ============================================================================================
    // 2. WATER SOURCE TYPES & SENSORY SIGNATURES
    // ============================================================================================
    enum class WaterType
    {
        FRESH_RIVER,       // High sound, high humidity, safe purity
        STAGNANT_PUDDLE,   // Low sound, low humidity, high pathogen risk
        DEEP_LAKE,          // High visibility, low sound, medium ambush risk
        SALT_OCEAN         // Unpotable
    };

    struct WaterSource
    {
        uint64_t ID{ 0 };
        Vec3 Position;
        WaterType Type{ WaterType::FRESH_RIVER };
        float VolumeLiters{ 5000.0f };
        float Purity{ 0.95f };            // [0.0 = Contaminated, 1.0 = Pure]
        float AmbushDanger{ 0.15f };      // Risk factor near shore
        float AcousticOutput{ 0.8f };      // Waterfall/river sound strength
        float HumidityRadius{ 120.0f };   // Distance humidity plume travels downwind
    };

    // ============================================================================================
    // 3. PHYSIOLOGICAL HYDRATION ENGINE
    // ============================================================================================
    class HydrationMetabolism
    {
    private:
        float m_MaxHydrationLiters{ 0.5f };  // Total capacity (500ml for avian spec)
        float m_CurrentHydration{ 0.4f };    // Start at 80%
        float m_DehydrationRatePerSec{ 0.0015f }; // Base loss rate

    public:
        void UpdateHydration(float dt, float ambientTemp, float exertionLevel)
        {
            // Hydration burn rate scales exponentially with ambient temperature and flight exertion
            float tempFactor = std::pow(ambientTemp / 25.0f, 1.8f);
            float totalLoss = m_DehydrationRatePerSec * tempFactor * (1.0f + exertionLevel * 1.5f) * dt;

            m_CurrentHydration = std::max(0.0f, m_CurrentHydration - totalLoss);
        }

        void Drink(float liters)
        {
            m_CurrentHydration = std::min(m_MaxHydrationLiters, m_CurrentHydration + liters);
        }

        float GetThirstNormalized() const
        {
            // 0.0 = Hydrated, 1.0 = Critically Dehydrated
            return 1.0f - (m_CurrentHydration / m_MaxHydrationLiters);
        }

        float GetSpeedDebuffMultiplier() const
        {
            float thirst = GetThirstNormalized();
            if (thirst > 0.8f) return 0.5f; // Extreme dehydration drops speed by 50%
            if (thirst > 0.5f) return 0.8f; // Moderate dehydration drops speed by 20%
            return 1.0f;
        }

        float GetCurrentHydration() const { return m_CurrentHydration; }
    };

    // ============================================================================================
    // 4. MULTI-SENSORY WATER PERCEPTION ENGINE
    // ============================================================================================
    struct WaterPerceptionResult
    {
        const WaterSource* Source{ nullptr };
        bool DetectedVisually{ false };
        bool DetectedAcoustically{ false };
        bool DetectedOlfactorily{ false }; // Via humidity downwind plume
        float ConfidenceScore{ 0.0f };
    };

    class WaterPerceptionSystem
    {
    private:
        float m_VisualRange{ 100.0f };
        float m_HearingThreshold{ 0.2f };
        float m_OlfactorySensitivity{ 0.05f };

    public:
        std::vector<WaterPerceptionResult> ScanEnvironment(
            const Vec3& birdPos, 
            const Vec3& forwardDir, 
            const EnvironmentalWind& wind, 
            const std::vector<WaterSource>& waterSources) const
        {
            std::vector<WaterPerceptionResult> results;

            for (const auto& water : waterSources)
            {
                if (water.Type == WaterType::SALT_OCEAN || water.VolumeLiters <= 0.1f) 
                    continue;

                Vec3 toWater = water.Position - birdPos;
                float dist = toWater.Length();
                Vec3 dirToWater = toWater / dist;

                WaterPerceptionResult perception;
                perception.Source = &water;

                // 1. Visual Detection (Direct Line of Sight & Reflection)
                if (dist <= m_VisualRange)
                {
                    float dot = forwardDir.Dot(dirToWater);
                    if (dot > 0.2f) // Within FOV
                    {
                        perception.DetectedVisually = true;
                        perception.ConfidenceScore += 0.5f;
                    }
                }

                // 2. Acoustic Sensing (Sound of flowing/splashing water)
                float soundIntensityAtBird = water.AcousticOutput / (1.0f + 0.05f * dist * dist);
                if (soundIntensityAtBird >= m_HearingThreshold)
                {
                    perception.DetectedAcoustically = true;
                    perception.ConfidenceScore += std::clamp(soundIntensityAtBird, 0.1f, 0.3f);
                }

                // 3. Olfactory Downwind Humidity Plume Detection
                // Plume stretches along the direction of the wind FROM the water source
                Vec3 plumeCenterDir = wind.Direction.Normalized();
                Vec3 waterToBird = birdPos - water.Position;
                float downwindDistance = waterToBird.Dot(plumeCenterDir);

                if (downwindDistance > 0.0f && downwindDistance <= water.HumidityRadius)
                {
                    Vec3 perpendicularOffset = waterToBird - (plumeCenterDir * downwindDistance);
                    float crossWindDist = perpendicularOffset.Length();

                    // Plume expands like a cone downwind
                    float plumeWidth = 5.0f + 0.25f * downwindDistance;
                    if (crossWindDist <= plumeWidth)
                    {
                        perception.DetectedOlfactorily = true;
                        perception.ConfidenceScore += 0.35f;
                    }
                }

                if (perception.ConfidenceScore > 0.0f)
                {
                    results.push_back(perception);
                }
            }

            return results;
        }
    };

    // ============================================================================================
    // 5. MASTER WATER SEARCH & DRINKING AI
    // ============================================================================================
    enum class WaterAIState
    {
        CRUISING,
        FOLLOWING_HUMIDITY_PLUME,
        APPROACHING_SHORELINE,
        ASSESSING_AMBUSH_RISK,
        DRINKING,
        SATIATED
    };

    class ThirstWaterAI
    {
    private:
        Vec3 m_Position{ 0.0f, 30.0f, 0.0f };
        Vec3 m_Velocity{ 0.0f, 0.0f, 0.0f };
        Vec3 m_ForwardDir{ 0.0f, 0.0f, 1.0f };

        float m_BaseSpeed{ 14.0f };
        HydrationMetabolism m_Hydration;
        WaterPerceptionSystem m_Perception;
        WaterAIState m_CurrentState{ WaterAIState::CRUISING };

        const WaterSource* m_TargetWaterSource{ nullptr };
        Vec3 m_TargetApproachPoint;
        float m_RiskAssessmentTimer{ 0.0f };

    public:
        ThirstWaterAI(const Vec3& startPos) : m_Position(startPos) {}

        // Utility Evaluator: Purity vs. Ambush Risk vs. Distance
        float EvaluateWaterUtility(const WaterSource& source) const
        {
            float dist = (source.Position - m_Position).Length();
            float distancePenalty = std::pow(dist + 1.0f, 1.1f);
            
            float purityScore = source.Purity;
            float safetyScore = 1.0f - source.AmbushDanger;

            return (purityScore * safetyScore * 1000.0f) / distancePenalty;
        }

        void TickSimulation(float dt, float ambientTemp, const EnvironmentalWind& wind, const std::vector<WaterSource>& waterWorld)
        {
            // 1. Update Physiological System
            float exertion = m_Velocity.Length() / m_BaseSpeed;
            m_Hydration.UpdateHydration(dt, ambientTemp, exertion);

            float thirst = m_Hydration.GetThirstNormalized();
            float speedMult = m_Hydration.GetSpeedDebuffMultiplier();
            float currentMaxSpeed = m_BaseSpeed * speedMult;

            // 2. Multi-Sensory Environment Scan
            auto perceptions = m_Perception.ScanEnvironment(m_Position, m_ForwardDir, wind, waterWorld);

            // 3. Behavior Decision Tree
            switch (m_CurrentState)
            {
            case WaterAIState::CRUISING:
                if (thirst > 0.25f) // Trigger thirst threshold
                {
                    std::cout << ">> [Thirst AI] Thirst Level High (" << thirst * 100.0f 
                              << "%). Scanning sensory channels for Water...\n";

                    if (!perceptions.empty())
                    {
                        // Select best water source via utility evaluation
                        auto bestIt = std::max_element(perceptions.begin(), perceptions.end(),
                            [this](const WaterPerceptionResult& a, const WaterPerceptionResult& b) {
                                return EvaluateWaterUtility(*a.Source) < EvaluateWaterUtility(*b.Source);
                            });

                        m_TargetWaterSource = bestIt->Source;

                        if (bestIt->DetectedVisually)
                        {
                            m_TargetApproachPoint = m_TargetWaterSource->Position;
                            m_CurrentState = WaterAIState::APPROACHING_SHORELINE;
                            std::cout << ">> [Thirst AI] Visual Contact with Water Source ID: " << m_TargetWaterSource->ID << "\n";
                        }
                        else if (bestIt->DetectedOlfactorily)
                        {
                            m_CurrentState = WaterAIState::FOLLOWING_HUMIDITY_PLUME;
                            std::cout << ">> [Thirst AI] Caught Humidity Scent Plume! Tracking upwind.\n";
                        }
                    }
                }
                break;

            case WaterAIState::FOLLOWING_HUMIDITY_PLUME:
                if (m_TargetWaterSource)
                {
                    // Move UPWIND toward the source origin
                    Vec3 upwindDir = (wind.Direction * -1.0f).Normalized();
                    Vec3 targetDir = (m_TargetWaterSource->Position - m_Position).Normalized();
                    
                    Vec3 combinedSteering = (upwindDir * 0.4f + targetDir * 0.6f).Normalized();
                    SteerTowards(m_Position + combinedSteering * 20.0f, dt, currentMaxSpeed);

                    if ((m_Position - m_TargetWaterSource->Position).Length() < 50.0f)
                    {
                        m_CurrentState = WaterAIState::APPROACHING_SHORELINE;
                        std::cout << ">> [Thirst AI] Shoreline in sight. Beginning Descent.\n";
                    }
                }
                break;

            case WaterAIState::APPROACHING_SHORELINE:
                if (m_TargetWaterSource)
                {
                    SteerTowards(m_TargetWaterSource->Position, dt, currentMaxSpeed);

                    if ((m_Position - m_TargetWaterSource->Position).Length() < 3.0f)
                    {
                        m_Velocity = Vec3(0, 0, 0);
                        m_RiskAssessmentTimer = 2.0f; // Pause 2 seconds to scan for predators
                        m_CurrentState = WaterAIState::ASSESSING_AMBUSH_RISK;
                        std::cout << ">> [Thirst AI] Landed near water. Assessing Ambush Danger...\n";
                    }
                }
                break;

            case WaterAIState::ASSESSING_AMBUSH_RISK:
                m_RiskAssessmentTimer -= dt;
                if (m_RiskAssessmentTimer <= 0.0f)
                {
                    if (m_TargetWaterSource->AmbushDanger > 0.6f && thirst < 0.85f)
                    {
                        std::cout << ">> [Thirst AI] Danger too high! Aborting drinking maneuver.\n";
                        m_CurrentState = WaterAIState::CRUISING;
                    }
                    else
                    {
                        std::cout << ">> [Thirst AI] Area Clear. Drinking Water...\n";
                        m_CurrentState = WaterAIState::DRINKING;
                    }
                }
                break;

            case WaterAIState::DRINKING:
                // Drink 0.15 Liters per second
                m_Hydration.Drink(0.15f * dt);

                if (m_Hydration.GetThirstNormalized() <= 0.05f)
                {
                    std::cout << ">> [Thirst AI] Fully Hydrated! Taking off into Cruise mode.\n";
                    m_CurrentState = WaterAIState::SATIATED;
                }
                break;

            case WaterAIState::SATIATED:
                SteerTowards(m_Position + Vec3(0, 15, 20), dt, currentMaxSpeed); // Ascend
                if (m_Position.y > 25.0f)
                {
                    m_CurrentState = WaterAIState::CRUISING;
                }
                break;
            }
        }

        void SteerTowards(const Vec3& target, float dt, float maxSpeed)
        {
            Vec3 desiredVel = (target - m_Position).Normalized() * maxSpeed;
            Vec3 steeringForce = desiredVel - m_Velocity;

            m_Velocity = m_Velocity + steeringForce * 2.5f * dt;
            if (m_Velocity.LengthSq() > maxSpeed * maxSpeed)
            {
                m_Velocity = m_Velocity.Normalized() * maxSpeed;
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
            std::cout << "[Bird Status] Pos: (" << m_Position.x << ", " << m_Position.y << ", " << m_Position.z << ")"
                      << " | Hydration: " << m_Hydration.GetCurrentHydration() << " L"
                      << " | Thirst: " << m_Hydration.GetThirstNormalized() * 100.0f << "%\n";
        }
    };
}

// ============================================================================================
// MAIN SIMULATION PIPELINE
// ============================================================================================
int main()
{
    using namespace AAAAAAAGeminiEngine;

    std::cout << "================================================================================\n";
    std::cout << " RUNNING AAA HYDRATION & MULTI-SENSORY WATER SEARCH SIMULATION                 \n";
    std::cout << "================================================================================\n\n";

    ThirstWaterAI bird(Vec3(0.0f, 35.0f, 0.0f));

    EnvironmentalWind wind{ Vec3(1.0f, 0.0f, 0.2f).Normalized(), 5.5f };

    // World Water Sources
    std::vector<WaterSource> worldWater = {
        // ID, Pos, Type, Vol, Purity, Ambush, Acoustic, HumidityRadius
        { 201, Vec3(120.0f, 0.0f, 45.0f), WaterType::FRESH_RIVER, 8000.0f, 0.98f, 0.10f, 0.9f, 150.0f },
        { 202, Vec3(30.0f, 0.0f, 10.0f),  WaterType::STAGNANT_PUDDLE, 50.0f, 0.20f, 0.70f, 0.1f, 20.0f }
    };

    constexpr float dt = 0.1f; // 10 Hz Loop
    constexpr float ambientTemp = 32.0f; // 32°C Hot Day

    for (int step = 0; step < 120; ++step)
    {
        bird.TickSimulation(dt, ambientTemp, wind, worldWater);

        if (step % 10 == 0)
        {
            bird.PrintStatus();
        }
    }

    return 0;
}
