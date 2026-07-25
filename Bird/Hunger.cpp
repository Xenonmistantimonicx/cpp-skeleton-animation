/**
 * ============================================================================================
 *  AAAAAAA-GRADE ULTRA-ADVANCED BIRD FORAGING & HUNGER SEARCH ENGINE
 * ============================================================================================
 *  Modules:
 *   1. Bio-Metabolic Energy Simulation (Caloric Burn Rate Model)
 *   2. Saccadic 3D Vision Cone Perception System
 *   3. Spatial Memory Engine with Time-Decaying Confidence
 *   4. Lévy Flight Stochastic Search Engine (Optimal Nature Foraging Strategy)
 *   5. Multi-Attribute Utility Food Selection Engine
 * 
 *  Target Standard : C++20 / Production Enterprise AAA AI System
 * ============================================================================================
 */

#ifndef HUNGER_BIRD_FORAGING_ENGINE_HPP
#define HUNGER_BIRD_FORAGING_ENGINE_HPP

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
    constexpr float DEG2RAD = PI / 180.0f;

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
    // 2. WORLD ENVIRONMENT & FOOD ENTITY DEFINITION
    // ============================================================================================
    struct FoodEntity
    {
        uint64_t ID{ 0 };
        Vec3 Position;
        float Calories{ 150.0f };      // Total energy yield
        float Freshness{ 1.0f };       // Decay multiplier [0.0 to 1.0]
        float PredationRisk{ 0.1f };   // Threat level near food source [0.0 to 1.0]
        bool IsConsumed{ false };
    };

    // ============================================================================================
    // 3. BIO-METABOLIC SIMULATION SYSTEM
    // ============================================================================================
    class MetabolicEngine
    {
    private:
        float m_MaxEnergyReserve{ 1000.0f }; // Joules / Kcal
        float m_CurrentEnergy{ 800.0f };
        float m_BasalMetabolicRate{ 2.5f };   // Base energy burn per second
        float m_MassKg{ 0.8f };               // Bird body mass

    public:
        void UpdateMetabolism(float dt, float currentSpeed, float flapFrequency)
        {
            // Kinetic energy cost equation: P = BMR + k1 * v^2 + k2 * freq^3
            float flightCost = 0.5f * m_MassKg * (currentSpeed * currentSpeed) * 0.1f;
            float flappingCost = 0.08f * std::pow(flapFrequency, 3.0f);
            float totalBurn = (m_BasalMetabolicRate + flightCost + flappingCost) * dt;

            m_CurrentEnergy = std::max(0.0f, m_CurrentEnergy - totalBurn);
        }

        void ConsumeFood(float calories)
        {
            m_CurrentEnergy = std::min(m_MaxEnergyReserve, m_CurrentEnergy + calories);
        }

        float GetHungerNormalized() const
        {
            // 0.0 = Full / Satiated, 1.0 = Starving
            return 1.0f - (m_CurrentEnergy / m_MaxEnergyReserve);
        }

        float GetCurrentEnergy() const { return m_CurrentEnergy; }
        bool IsStarving() const { return m_CurrentEnergy < 200.0f; }
    };

    // ============================================================================================
    // 4. SPATIAL MEMORY ENGINE (EPISODIC MEMORY MAP WITH DECAY)
    // ============================================================================================
    struct MemoryEntry
    {
        Vec3 Position;
        float EstimatedCalories;
        float Confidence{ 1.0f }; // Decays over time
        float LastSeenTimestamp{ 0.0f };
    };

    class SpatialMemoryMap
    {
    private:
        std::vector<MemoryEntry> m_Memories;
        float m_MemoryDecayRate{ 0.02f }; // Confidence decay rate per second

    public:
        void StoreOrUpdateMemory(const Vec3& pos, float calories, float currentTime)
        {
            for (auto& mem : m_Memories)
            {
                if ((mem.Position - pos).LengthSq() < 4.0f) // Within 2 meters
                {
                    mem.Position = pos;
                    mem.EstimatedCalories = calories;
                    mem.Confidence = 1.0f;
                    mem.LastSeenTimestamp = currentTime;
                    return;
                }
            }
            m_Memories.push_back({ pos, calories, 1.0f, currentTime });
        }

        void DecayMemories(float dt)
        {
            for (auto& mem : m_Memories)
            {
                mem.Confidence -= m_MemoryDecayRate * dt;
            }
            // Remove forgotten memories
            m_Memories.erase(
                std::remove_if(m_Memories.begin(), m_Memories.end(), [](const MemoryEntry& m) {
                    return m.Confidence <= 0.05f;
                }),
                m_Memories.end()
            );
        }

        const std::vector<MemoryEntry>& GetMemories() const { return m_Memories; }
    };

    // ============================================================================================
    // 5. LÉVY FLIGHT STOCHASTIC SEARCH ENGINE
    // ============================================================================================
    class LevyFlightSearchEngine
    {
    private:
        std::mt19937 m_Rng{ 1337 };
        Vec3 m_CurrentTargetDestination;
        bool m_HasActiveDestination{ false };
        float m_LevyExponentMu{ 1.8f }; // 1 < mu <= 3 for optimal Levy flight

    public:
        Vec3 GenerateNextLevyStep(const Vec3& currentPos)
        {
            std::uniform_real_distribution<float> dist(0.0001f, 0.9999f);
            
            // Generate Levy distributed step length: L = u^(-1 / (mu - 1))
            float u = dist(m_Rng);
            float stepLength = std::pow(u, -1.0f / (m_LevyExponentMu - 1.0f)) * 2.0f;
            stepLength = std::clamp(stepLength, 5.0f, 150.0f); // Bounds for bird flight

            // Uniform random 3D direction
            float theta = dist(m_Rng) * TWO_PI;
            float phi = (dist(m_Rng) - 0.5f) * PI;

            Vec3 direction(
                std::cos(phi) * std::cos(theta),
                std::sin(phi) * 0.3f, // Flatten pitch flight slightly
                std::cos(phi) * std::sin(theta)
            );

            m_CurrentTargetDestination = currentPos + direction.Normalized() * stepLength;
            m_HasActiveDestination = true;

            return m_CurrentTargetDestination;
        }

        bool HasDestination() const { return m_HasActiveDestination; }
        void InvalidateDestination() { m_HasActiveDestination = false; }
    };

    // ============================================================================================
    // 6. MASTER HUNGER BIRD AI SYSTEM
    // ============================================================================================
    enum class BirdBehaviorState
    {
        PERCHING_IDLE,
        LEVY_SEARCHING,
        NAVIGATING_TO_MEMORY,
        DIRECT_FORAGING_ATTACK,
        EATING
    };

    class HungerBirdAI
    {
    private:
        Vec3 m_Position{ 0.0f, 20.0f, 0.0f };
        Vec3 m_Velocity{ 0.0f, 0.0f, 0.0f };
        Vec3 m_ForwardDir{ 0.0f, 0.0f, 1.0f };

        float m_VisionRange{ 60.0f };         // Meters
        float m_VisionFOVAngle{ 120.0f };      // Degrees Field Of View
        float m_MaxSpeed{ 12.0f };             // m/s

        MetabolicEngine m_Metabolism;
        SpatialMemoryMap m_Memory;
        LevyFlightSearchEngine m_LevyEngine;

        BirdBehaviorState m_CurrentState{ BirdBehaviorState::PERCHING_IDLE };
        std::optional<FoodEntity> m_TargetFood;
        Vec3 m_CurrentNavWaypoint;

    public:
        HungerBirdAI(const Vec3& startPos) : m_Position(startPos) {}

        // Saccadic Perception Cone Check
        bool CanSeeEntity(const Vec3& targetPos) const
        {
            Vec3 toTarget = targetPos - m_Position;
            float dist = toTarget.Length();
            if (dist > m_VisionRange) return false;

            Vec3 dirToTarget = toTarget / dist;
            float dot = m_ForwardDir.Dot(dirToTarget);
            float angleDeg = std::acos(std::clamp(dot, -1.0f, 1.0f)) * (180.0f / PI);

            return angleDeg <= (m_VisionFOVAngle * 0.5f);
        }

        // Multi-Factor Utility Function for Food Selection
        float EvaluateFoodUtility(const FoodEntity& food) const
        {
            float dist = (food.Position - m_Position).Length();
            float netEnergyGain = food.Calories * food.Freshness;
            
            // Utility Curve: U = Energy / (Distance^1.2) * SafetyFactor
            float distancePenalty = std::pow(dist + 1.0f, 1.2f);
            float safetyFactor = 1.0f - food.PredationRisk;

            return (netEnergyGain / distancePenalty) * safetyFactor;
        }

        void TickSimulation(float dt, float totalSimulationTime, std::vector<FoodEntity>& worldFoodPool)
        {
            // 1. Update Biological Metabolism
            float currentSpeed = m_Velocity.Length();
            float flapFreq = (m_CurrentState == BirdBehaviorState::PERCHING_IDLE) ? 0.0f : 4.5f;
            m_Metabolism.UpdateMetabolism(dt, currentSpeed, flapFreq);
            m_Memory.DecayMemories(dt);

            float hunger = m_Metabolism.GetHungerNormalized();

            // 2. Scan Vision Field for Food & Store in Spatial Memory
            std::vector<FoodEntity> visibleFood;
            for (const auto& food : worldFoodPool)
            {
                if (!food.IsConsumed && CanSeeEntity(food.Position))
                {
                    visibleFood.push_back(food);
                    m_Memory.StoreOrUpdateMemory(food.Position, food.Calories, totalSimulationTime);
                }
            }

            // 3. Finite State Decision Tree based on Hunger & Utility
            switch (m_CurrentState)
            {
            case BirdBehaviorState::PERCHING_IDLE:
                if (hunger > 0.35f) // Hunger threshold triggered
                {
                    std::cout << ">> [AI Decision] Hunger Threshold Exceeded (" << hunger * 100.0f 
                              << "%). Initiating Food Search Strategy!\n";
                    m_CurrentState = BirdBehaviorState::LEVY_SEARCHING;
                }
                break;

            case BirdBehaviorState::LEVY_SEARCHING:
                // Check if direct vision spotted food
                if (!visibleFood.empty())
                {
                    // Pick highest utility food
                    auto bestIt = std::max_element(visibleFood.begin(), visibleFood.end(),
                        [this](const FoodEntity& a, const FoodEntity& b) {
                            return EvaluateFoodUtility(a) < EvaluateFoodUtility(b);
                        });

                    m_TargetFood = *bestIt;
                    m_CurrentState = BirdBehaviorState::DIRECT_FORAGING_ATTACK;
                    m_LevyEngine.InvalidateDestination();
                    std::cout << ">> [AI Decision] Visual Lock on Food ID: " << bestIt->ID << "! Diving in.\n";
                }
                // Else check memory map
                else if (!m_Memory.GetMemories().empty())
                {
                    const auto& bestMem = m_Memory.GetMemories().front();
                    m_CurrentNavWaypoint = bestMem.Position;
                    m_CurrentState = BirdBehaviorState::NAVIGATING_TO_MEMORY;
                    std::cout << ">> [AI Decision] Navigating to Remembered Food Location.\n";
                }
                // Else continue Levy Flight Random Search Pattern
                else
                {
                    if (!m_LevyEngine.HasDestination())
                    {
                        m_CurrentNavWaypoint = m_LevyEngine.GenerateNextLevyStep(m_Position);
                    }

                    SteerTowards(m_CurrentNavWaypoint, dt);

                    if ((m_Position - m_CurrentNavWaypoint).LengthSq() < 9.0f)
                    {
                        m_LevyEngine.InvalidateDestination();
                    }
                }
                break;

            case BirdBehaviorState::DIRECT_FORAGING_ATTACK:
                if (m_TargetFood.has_value())
                {
                    // Find actual food item in world pool
                    auto it = std::find_if(worldFoodPool.begin(), worldFoodPool.end(),
                        [this](const FoodEntity& f) { return f.ID == m_TargetFood->ID; });

                    if (it != worldFoodPool.end() && !it->IsConsumed)
                    {
                        SteerTowards(it->Position, dt);

                        // Capture/Consumption distance check
                        if ((m_Position - it->Position).Length() < 1.2f)
                        {
                            it->IsConsumed = true;
                            m_Metabolism.ConsumeFood(it->Calories * it->Freshness);
                            m_CurrentState = BirdBehaviorState::EATING;
                            std::cout << ">> [AI Decision] Food Consumed! Energy Recovered: " 
                                      << it->Calories << " Joules.\n";
                        }
                    }
                    else
                    {
                        // Food was lost or eaten by another creature
                        m_TargetFood.reset();
                        m_CurrentState = BirdBehaviorState::LEVY_SEARCHING;
                    }
                }
                break;

            case BirdBehaviorState::EATING:
                m_Velocity = Vec3(0, 0, 0);
                if (m_Metabolism.GetHungerNormalized() < 0.15f)
                {
                    std::cout << ">> [AI Decision] Bird Fully Satiated. Returning to Perch.\n";
                    m_CurrentState = BirdBehaviorState::PERCHING_IDLE;
                }
                break;

            case BirdBehaviorState::NAVIGATING_TO_MEMORY:
                SteerTowards(m_CurrentNavWaypoint, dt);
                if ((m_Position - m_CurrentNavWaypoint).Length() < 3.0f)
                {
                    std::cout << ">> [AI Decision] Arrived at Memory Location. Searching visually...\n";
                    m_CurrentState = BirdBehaviorState::LEVY_SEARCHING;
                }
                break;
            }
        }

        void SteerTowards(const Vec3& target, float dt)
        {
            Vec3 desiredVel = (target - m_Position).Normalized() * m_MaxSpeed;
            Vec3 steeringForce = desiredVel - m_Velocity;

            m_Velocity = m_Velocity + steeringForce * 3.0f * dt;
            if (m_Velocity.LengthSq() > m_MaxSpeed * m_MaxSpeed)
            {
                m_Velocity = m_Velocity.Normalized() * m_MaxSpeed;
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
                      << " | Energy: " << m_Metabolism.GetCurrentEnergy() << " J"
                      << " | Hunger: " << m_Metabolism.GetHungerNormalized() * 100.0f << "%\n";
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
    std::cout << " RUNNING AAA HUNGER BIRD FORAGING & LÉVY FLIGHT SEARCH SIMULATION               \n";
    std::cout << "================================================================================\n\n";

    HungerBirdAI bird(Vec3(0.0f, 15.0f, 0.0f));

    // World Food Environment
    std::vector<FoodEntity> worldFoodPool = {
        { 101, Vec3(85.0f, 0.0f, 40.0f), 350.0f, 1.0f, 0.05f, false }, // Far away food
        { 102, Vec3(15.0f, 0.0f, 25.0f), 200.0f, 0.8f, 0.20f, false }  // Closer food
    };

    constexpr float dt = 0.1f; // 10 Hz Simulation Loop
    float simTime = 0.0f;

    for (int step = 0; step < 120; ++step)
    {
        simTime += dt;
        bird.TickSimulation(dt, simTime, worldFoodPool);

        if (step % 10 == 0)
        {
            bird.PrintStatus();
        }
    }

    return 0;
}
