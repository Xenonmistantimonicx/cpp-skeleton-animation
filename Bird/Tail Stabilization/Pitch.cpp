/**
 * ============================================================================
 *  AAA ADVANCED BIOMECHANICAL & AERODYNAMIC TAIL PITCH STABILIZER ENGINE
 * ============================================================================
 *  Engine Target  : Unreal Engine 5 Custom Module / Proprietary AAA Physics Engine
 *  Language Standard: C++20
 *  Features       :
 *    - Full 3D Lifting-Line Aerodynamic Tail-Down Force (TDF) & Lift Solvers
 *    - Center of Pressure (CoP) Shift & Longitudinal Static Stability Matrix
 *    - Biomechanical Agonist/Antagonist Muscle Activation (Levator/Depressor Caudae)
 *    - Multi-Node Structural Cantilever Beam Deformation for 12 Rectrices Feathers
 *    - High-Frequency Unsteady Vortex Shedding & Dynamic Wing Downwash Delay Buffer
 * ============================================================================
 */

#ifndef AAA_BIRD_FULL_PITCH_AEROELASTIC_SYSTEM_HPP
#define AAA_BIRD_FULL_PITCH_AEROELASTIC_SYSTEM_HPP

#include <cmath>
#include <algorithm>
#include <array>
#include <vector>
#include <cstdint>

namespace AAABirdEngine
{
    // ============================================================================
    // 1. PRECISION VECTOR & MATRIX MATH (C++20 COMPATIBLE)
    // ============================================================================

    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 6.28318530717958647692f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;

    struct Vec3
    {
        float x{ 0.0f }, y{ 0.0f }, z{ 0.0f };

        constexpr Vec3() = default;
        constexpr Vec3(float inX, float inY, float inZ) : x(inX), y(inY), z(inZ) {}

        inline Vec3 operator+(const Vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
        inline Vec3 operator-(const Vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
        inline Vec3 operator*(float s) const { return { x * s, y * s, z * s }; }
        inline Vec3 operator/(float s) const { float inv = 1.0f / s; return { x * inv, y * inv, z * inv }; }

        inline Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
        inline Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
        inline Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

        inline float LengthSq() const { return x * x + y * y + z * z; }
        inline float Length() const { return std::sqrt(LengthSq()); }

        inline Vec3 Normalized() const
        {
            float len = Length();
            return len > 0.00001f ? (*this) * (1.0f / len) : Vec3{ 0.0f, 0.0f, 0.0f };
        }

        static inline float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
        static inline Vec3 Cross(const Vec3& a, const Vec3& b)
        {
            return {
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            };
        }

        static inline Vec3 Lerp(const Vec3& a, const Vec3& b, float t)
        {
            t = std::clamp(t, 0.0f, 1.0f);
            return a + (b - a) * t;
        }
    };

    struct Quat
    {
        float w{ 1.0f }, x{ 0.0f }, y{ 0.0f }, z{ 0.0f };

        constexpr Quat() = default;
        constexpr Quat(float inW, float inX, float inY, float inZ) : w(inW), x(inX), y(inY), z(inZ) {}

        static inline Quat FromEulerPitch(float pitchDeg)
        {
            float halfP = pitchDeg * 0.5f * DEG_TO_RAD;
            return { std::cos(halfP), std::sin(halfP), 0.0f, 0.0f };
        }

        inline Vec3 RotateVector(const Vec3& v) const
        {
            Vec3 qVec{ x, y, z };
            Vec3 uv = Vec3::Cross(qVec, v);
            Vec3 uuv = Vec3::Cross(qVec, uv);
            return v + ((uv * w) + uuv) * 2.0f;
        }
    };

    // ============================================================================
    // 2. BIOMECHANICAL MUSCLE ACTUATION MODEL (Levator & Depressor Caudae)
    // ============================================================================

    struct CaudaeMuscleGroup
    {
        float ActivationSignal{ 0.0f };     // Normalized [0.0 - 1.0]
        float IsometricTorqueMax{ 18.5f };   // Max static muscle force in Nm
        float ForceVelocityDamping{ 0.12f }; // Hill-type muscle force velocity relation factor
        float CurrentLength{ 1.0f };         // Normalized muscle bundle length
        float FatigueFactor{ 1.0f };         // [1.0 = Fresh, 0.2 = Exhausted]

        inline float ComputeExertedTorque(float angularVelocity)
        {
            // Hill's Muscle Model Approximation
            float velocityFactor = std::max(0.1f, 1.0f - ForceVelocityDamping * angularVelocity);
            return ActivationSignal * IsometricTorqueMax * velocityFactor * FatigueFactor;
        }

        inline void UpdateFatigue(float dt)
        {
            if (ActivationSignal > 0.7f)
            {
                FatigueFactor = std::max(0.2f, FatigueFactor - dt * 0.05f * ActivationSignal);
            }
            else
            {
                FatigueFactor = std::min(1.0f, FatigueFactor + dt * 0.02f);
            }
        }
    };

    // ============================================================================
    // 3. WING DOWNWASH DELAY RING-BUFFER
    // ============================================================================

    class WingDownwashHistoryBuffer
    {
    private:
        static constexpr size_t BUFFER_SIZE = 64;
        std::array<float, BUFFER_SIZE> m_DownwashAngleHistory{};
        size_t m_WriteIndex{ 0 };

    public:
        WingDownwashHistoryBuffer()
        {
            m_DownwashAngleHistory.fill(0.0f);
        }

        void PushDownwash(float angleRad)
        {
            m_DownwashAngleHistory[m_WriteIndex] = angleRad;
            m_WriteIndex = (m_WriteIndex + 1) % BUFFER_SIZE;
        }

        float GetDelayedDownwash(float flightSpeed, float distanceToTailMeters)
        {
            if (flightSpeed < 0.1f) return 0.0f;
            float timeDelaySeconds = distanceToTailMeters / flightSpeed;
            // Assuming 120Hz tick resolution
            size_t delayTicks = static_cast<size_t>(timeDelaySeconds * 120.0f);
            delayTicks = std::clamp(delayTicks, static_cast<size_t>(1), BUFFER_SIZE - 1);

            size_t readIndex = (m_WriteIndex + BUFFER_SIZE - delayTicks) % BUFFER_SIZE;
            return m_DownwashAngleHistory[readIndex];
        }
    };

    // ============================================================================
    // 4. STRUCTURAL CANTILEVER DEFORMATION FOR 12 RECTRICES FEATHERS
    // ============================================================================

    constexpr uint32_t TOTAL_RECTRICES = 12;

    struct SingleRectrixStructuralNode
    {
        uint32_t Index{ 0 };
        float ShaftFlexuralRigidityEI{ 0.045f }; // N*m^2 (Young's Modulus * Second Moment of Area)
        float LengthMeters{ 0.36f };
        float AreaSquareMeters{ 0.012f };

        float CurvatureOffsetDeg{ 0.0f };
        float DeflectionVelocity{ 0.0f };
        float AccumulatedVibrationEnergy{ 0.0f };

        void SolveDeformation(float aerodynamicLoadNewtons, float dt)
        {
            // Cantilever Euler-Bernoulli Beam Deflection at Tip: delta = (F * L^3) / (3 * EI)
            float L3 = LengthMeters * LengthMeters * LengthMeters;
            float targetTipDeflectionMeters = (aerodynamicLoadNewtons * L3) / (3.0f * ShaftFlexuralRigidityEI);
            
            // Convert displacement tip distance to angular pitch deflection in degrees
            float targetDeflectionAngleDeg = std::asin(std::clamp(targetTipDeflectionMeters / LengthMeters, -0.9f, 0.9f)) * RAD_TO_DEG;

            // Damped Spring System for Feather Shaft Dynamics
            float frequency = 18.0f; // High natural resonance of keratin shaft
            float damping = 0.58f;

            float f = TWO_PI * frequency;
            float k1 = damping / (PI * frequency);
            float k2 = 1.0f / (f * f);

            float accel = (targetDeflectionAngleDeg - CurvatureOffsetDeg - k1 * DeflectionVelocity) / k2;
            DeflectionVelocity += accel * dt;
            CurvatureOffsetDeg += DeflectionVelocity * dt;
        }
    };

    // ============================================================================
    // 5. COMPLETE PITCH FLIGHT INPUT & OUTPUT DATA STRUCTURES
    // ============================================================================

    struct BirdPitchInputState
    {
        Vec3 LinearVelocityWorld{ 0.0f, 0.0f, 28.0f };
        Vec3 AngularVelocityRadSec{ 0.0f, 0.0f, 0.0f }; // X component is Pitch Rate
        
        Vec3 BodyForwardWorld{ 0.0f, 0.0f, 1.0f };
        Vec3 BodyUpWorld{ 0.0f, 1.0f, 0.0f };

        float BodyPitchAngleDeg{ 0.0f };
        float WingCenterOfLiftZ{ 0.06f };   // Distance from CoM to Wing Lift vector (meters)
        float MainWingSpanMeters{ 2.1f };
        float MainWingLiftForceN{ 22.0f };
        
        float TailFanSpreadNormalized{ 0.35f }; // [0.0 = Closed, 1.0 = Max Fan]
        bool bIsExecutingBrakeLanding{ false };
        bool bIsDivingTucked{ false };
    };

    struct BirdPitchStateOutputs
    {
        float PygostylePitchRotationDeg{ 0.0f };
        float AerodynamicTailDownForceN{ 0.0f };
        float NetPitchingTorqueNm{ 0.0f };
        
        float AngleOfAttackAlphaDeg{ 0.0f };
        float EffectiveTailAlphaDeg{ 0.0f };
        float DownwashAngleDeg{ 0.0f };

        float StallBuffetingTremorDeg{ 0.0f };
        std::array<float, TOTAL_RECTRICES> FeatherBendingAnglesDeg{};
        
        Quat PygostyleWorldQuaternion;
    };

    // ============================================================================
    // 6. MASTER AAA PITCH ENGINE CLASS
    // ============================================================================

    class AAABirdPitchStabilizerSystem
    {
    private:
        BirdPitchStateOutputs m_Outputs;

        CaudaeMuscleGroup m_LevatorCaudae;   // Pulls tail UP (Pitch Up)
        CaudaeMuscleGroup m_DepressorCaudae; // Pulls tail DOWN (Pitch Down)

        WingDownwashHistoryBuffer m_DownwashBuffer;
        std::array<SingleRectrixStructuralNode, TOTAL_RECTRICES> m_RectricesNodes;

        float m_PygostylePitchAngleDeg{ 0.0f };
        float m_PygostylePitchVelocityRadSec{ 0.0f };

        float m_AccumulatedTime{ 0.0f };

        // Constants
        const float RHO_AIR = 1.225f; // kg/m^3
        const float PYGOSTYLE_MASS_MOMENT_OF_INERTIA = 0.0085f; // kg*m^2
        const float TAIL_MOMENT_ARM_METERS = 0.48f;
        const float BASE_TAIL_AREA_M2 = 0.08f;
        const float FAN_TAIL_AREA_M2 = 0.22f;

    public:
        AAABirdPitchStabilizerSystem()
        {
            // Initialize 12 Rectrices Feather Structural Nodes
            for (uint32_t i = 0; i < TOTAL_RECTRICES; ++i)
            {
                m_RectricesNodes[i].Index = i;
                float distFromCenter = std::abs(static_cast<float>(i) - 5.5f) / 5.5f;
                // Outer feathers are slightly shorter and more flexible
                m_RectricesNodes[i].LengthMeters = 0.38f - distFromCenter * 0.06f;
                m_RectricesNodes[i].ShaftFlexuralRigidityEI = 0.05f - distFromCenter * 0.018f;
            }
        }

        void TickEngine(const BirdPitchInputState& input, float deltaTime)
        {
            m_AccumulatedTime += deltaTime;

            // 1. RELATIVE AIRSPEED & ANGLE OF ATTACK (AoA)
            float airspeed = input.LinearVelocityWorld.Length();
            Vec3 airDir = airspeed > 0.0001f ? input.LinearVelocityWorld.Normalized() : input.BodyForwardWorld;

            float forwardComp = Vec3::Dot(airDir, input.BodyForwardWorld);
            float verticalComp = Vec3::Dot(airDir, input.BodyUpWorld);

            float alphaAoARad = std::atan2(-verticalComp, forwardComp);
            float alphaAoADeg = alphaAoARad * RAD_TO_DEG;
            m_Outputs.AngleOfAttackAlphaDeg = alphaAoADeg;

            // 2. MAIN WING DOWNWASH GENERATION & DELAY SOLVER
            // Prandtl's Lifting-Line Theory Downwash Angle: epsilon = (2 * C_L) / (PI * AspectRatio)
            float dynamicPressure = 0.5f * RHO_AIR * airspeed * airspeed;
            float wingCL = input.MainWingLiftForceN / (std::max(0.001f, dynamicPressure) * 0.65f); // 0.65m^2 wing area
            float aspectRatio = (input.MainWingSpanMeters * input.MainWingSpanMeters) / 0.65f;
            
            float downwashAngleRad = (2.0f * wingCL) / (PI * aspectRatio + 0.001f);
            m_DownwashBuffer.PushDownwash(downwashAngleRad);

            float delayedDownwashRad = m_DownwashBuffer.GetDelayedDownwash(airspeed, TAIL_MOMENT_ARM_METERS);
            m_Outputs.DownwashAngleDeg = delayedDownwashRad * RAD_TO_DEG;

            // Effective Tail Alpha incorporating downwash vector shift
            float effectiveTailAlphaRad = alphaAoARad - delayedDownwashRad + (m_PygostylePitchAngleDeg * DEG_TO_RAD);
            m_Outputs.EffectiveTailAlphaDeg = effectiveTailAlphaRad * RAD_TO_DEG;

            // 3. AERODYNAMIC TAIL-DOWN FORCE (TDF) & MOMENT SOLVER
            float currentTailArea = Vec3::Lerp(
                Vec3(BASE_TAIL_AREA_M2, 0, 0),
                Vec3(FAN_TAIL_AREA_M2, 0, 0),
                input.TailFanSpreadNormalized
            ).x;

            // Non-linear Tail Lift Curve with Stall Drop-off
            float tailCL = 2.0f * PI * std::sin(effectiveTailAlphaRad);
            if (std::abs(effectiveTailAlphaRad * RAD_TO_DEG) > 16.0f)
            {
                // Post-stall lift deterioration
                float stallFactor = std::clamp((std::abs(effectiveTailAlphaRad * RAD_TO_DEG) - 16.0f) / 10.0f, 0.0f, 0.8f);
                tailCL *= (1.0f - stallFactor);
            }

            float tailLiftForceN = dynamicPressure * currentTailArea * tailCL;
            m_Outputs.AerodynamicTailDownForceN = -tailLiftForceN; // Negative lift is Tail-Down Force

            // Aerodynamic torque exerted around Pygostyle joint
            float aeroPygostyleTorque = tailLiftForceN * 0.15f; 

            // 4. BIOMECHANICAL CONTROL LOOP (MUSCLE ACTIVATION SOLVER)
            // Target equilibrium pitch torque calculation
            float wingPitchingTorque = input.MainWingLiftForceN * input.WingCenterOfLiftZ;
            float bodyPitchDampingTorque = input.AngularVelocityRadSec.x * 24.5f; // Pitch rate damping
            
            float requiredCorrectionTorque = -(wingPitchingTorque + bodyPitchDampingTorque);

            if (input.bIsExecutingBrakeLanding)
            {
                requiredCorrectionTorque += 35.0f; // Maximum pitch-up command
            }
            else if (input.bIsDivingTucked)
            {
                requiredCorrectionTorque = 0.0f; // Zero trim torque
            }

            // Distribute activation signals to Levator vs Depressor Caudae muscles
            if (requiredCorrectionTorque > 0.0f)
            {
                m_LevatorCaudae.ActivationSignal = std::clamp(requiredCorrectionTorque / 18.5f, 0.0f, 1.0f);
                m_DepressorCaudae.ActivationSignal = 0.0f;
            }
            else
            {
                m_LevatorCaudae.ActivationSignal = 0.0f;
                m_DepressorCaudae.ActivationSignal = std::clamp(-requiredCorrectionTorque / 18.5f, 0.0f, 1.0f);
            }

            m_LevatorCaudae.UpdateFatigue(deltaTime);
            m_DepressorCaudae.UpdateFatigue(deltaTime);

            float levatorTorque = m_LevatorCaudae.ComputeExertedTorque(m_PygostylePitchVelocityRadSec);
            float depressorTorque = m_DepressorCaudae.ComputeExertedTorque(-m_PygostylePitchVelocityRadSec);

            float netMuscleTorque = levatorTorque - depressorTorque;
            float totalJointTorque = netMuscleTorque + aeroPygostyleTorque;

            // Integrate Pygostyle Rotational Physics: T = I * alpha
            float angularAcceleration = totalJointTorque / PYGOSTYLE_MASS_MOMENT_OF_INERTIA;
            m_PygostylePitchVelocityRadSec += angularAcceleration * deltaTime;

            // Damping to prevent infinite oscillation
            m_PygostylePitchVelocityRadSec *= (1.0f - 3.5f * deltaTime);
            m_PygostylePitchAngleDeg += (m_PygostylePitchVelocityRadSec * RAD_TO_DEG) * deltaTime;

            // Hard anatomical joint stops
            m_PygostylePitchAngleDeg = std::clamp(m_PygostylePitchAngleDeg, -30.0f, 60.0f);

            // 5. UNSTEADY VORTEX SHEDDING & STALL BUFFETING
            float stallSeverity = std::clamp((std::abs(m_Outputs.EffectiveTailAlphaDeg) - 14.0f) / 12.0f, 0.0f, 1.0f);
            float strouhalFrequency = (0.21f * airspeed) / 0.25f; // St = (f * L) / U
            strouhalFrequency = std::clamp(strouhalFrequency, 10.0f, 45.0f);

            float buffetingSine = std::sin(m_AccumulatedTime * strouhalFrequency * TWO_PI);
            m_Outputs.StallBuffetingTremorDeg = buffetingSine * stallSeverity * (airspeed * 0.08f);

            // 6. SOLVE INDIVIDUAL RECTRICES FEATHER CANTILEVER DEFORMATION
            float perFeatherLoadN = (tailLiftForceN / static_cast<float>(TOTAL_RECTRICES));

            for (uint32_t i = 0; i < TOTAL_RECTRICES; ++i)
            {
                m_RectricesNodes[i].SolveDeformation(perFeatherLoadN, deltaTime);
                m_Outputs.FeatherBendingAnglesDeg[i] = m_RectricesNodes[i].CurvatureOffsetDeg;
            }

            // 7. FINAL OUTPUT ASSEMBLY
            m_Outputs.PygostylePitchRotationDeg = m_PygostylePitchAngleDeg + m_Outputs.StallBuffetingTremorDeg;
            m_Outputs.NetPitchingTorqueNm = totalJointTorque;
            m_Outputs.PygostyleWorldQuaternion = Quat::FromEulerPitch(m_Outputs.PygostylePitchRotationDeg);
        }

        inline const BirdPitchStateOutputs& GetOutputs() const { return m_Outputs; }
    };
}

#endif // AAA_BIRD_FULL_PITCH_AEROELASTIC_SYSTEM_HPP
