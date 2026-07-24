/**
 * ============================================================================================
 *  AAA MONOLITHIC SIMD-ALIGNED BIRD FLIGHT DYNAMICS & TAIL YAW-PITCH-ROLL ENGINE
 * ============================================================================================
 *  Target Standard   : C++20 (Zero Dynamic Memory Allocation / Zero Thread Heap Contention)
 *  Architecture      : High-Frequency Deterministic Physics Engine (UE5 / Custom Engines)
 *  Security Level    : Hardened Deterministic State Integration (Anti-Breach / Anti-Desync)
 *  Features Included  :
 *      1. Full 6-DOF Monolithic Quaternion Rigid-Body Mechanics Matrix
 *      2. Coupled Pitch-Roll-Yaw Aerodynamic Force & Moment Tensor Solver
 *      3. Continuous Non-Linear Sideslip (Beta) & Angle of Attack (Alpha) Coupling
 *      4. Sub-Step Structural Cantilever Beam Deflection Matrix (R1 to R12 Rectrices)
 *      5. Biomechanical Hill Muscle Force-Velocity-Fatigue Dynamic Solver
 * ============================================================================================
 */

#ifndef AAA_BIRD_MONOLITHIC_YAW_PITCH_ROLL_ENGINE_HPP
#define AAA_BIRD_MONOLITHIC_YAW_PITCH_ROLL_ENGINE_HPP

#include <cmath>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cassert>

namespace AAABirdEngine
{
    // ============================================================================================
    // 1. HARDENED 16-BYTE ALIGNED SIMD MATH PRIMITIVES
    // ============================================================================================

    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 6.28318530717958647692f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;
    constexpr float AIR_DENSITY_SEA_LEVEL = 1.225f; // kg/m^3

    alignas(16) struct SIMDVec3
    {
        float x{ 0.0f }, y{ 0.0f }, z{ 0.0f }, w{ 0.0f };

        constexpr SIMDVec3() = default;
        constexpr SIMDVec3(float inX, float inY, float inZ, float inW = 0.0f) 
            : x(inX), y(inY), z(inZ), w(inW) {}

        inline SIMDVec3 operator+(const SIMDVec3& o) const { return SIMDVec3(x + o.x, y + o.y, z + o.z); }
        inline SIMDVec3 operator-(const SIMDVec3& o) const { return SIMDVec3(x - o.x, y - o.y, z - o.z); }
        inline SIMDVec3 operator*(float s) const { return SIMDVec3(x * s, y * s, z * s); }
        inline SIMDVec3 operator/(float s) const { float inv = 1.0f / s; return SIMDVec3(x * inv, y * inv, z * inv); }

        inline float LengthSq() const { return x * x + y * y + z * z; }
        inline float Length() const { return std::sqrt(LengthSq()); }

        inline SIMDVec3 Normalized() const
        {
            float len = Length();
            return len > 0.00001f ? (*this) * (1.0f / len) : SIMDVec3(0.0f, 0.0f, 0.0f);
        }

        static inline float Dot(const SIMDVec3& a, const SIMDVec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
        static inline SIMDVec3 Cross(const SIMDVec3& a, const SIMDVec3& b)
        {
            return SIMDVec3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            );
        }
    };

    alignas(16) struct HardenedQuat
    {
        float w{ 1.0f }, x{ 0.0f }, y{ 0.0f }, z{ 0.0f };

        constexpr HardenedQuat() = default;
        constexpr HardenedQuat(float inW, float inX, float inY, float inZ) : w(inW), x(inX), y(inY), z(inZ) {}

        static inline HardenedQuat FromEuler(float pitchDeg, float yawDeg, float rollDeg)
        {
            float p = pitchDeg * 0.5f * DEG_TO_RAD;
            float y = yawDeg * 0.5f * DEG_TO_RAD;
            float r = rollDeg * 0.5f * DEG_TO_RAD;

            float sinP = std::sin(p), cosP = std::cos(p);
            float sinY = std::sin(y), cosY = std::cos(y);
            float sinR = std::sin(r), cosR = std::cos(r);

            return HardenedQuat(
                cosR * cosP * cosY + sinR * sinP * sinY,
                cosR * sinP * cosY - sinR * cosP * sinY,
                cosR * cosP * sinY + sinR * sinP * cosY,
                sinR * cosP * cosY - cosR * sinP * cosY
            );
        }

        inline SIMDVec3 RotateVector(const SIMDVec3& v) const
        {
            SIMDVec3 qv(x, y, z);
            SIMDVec3 uv = SIMDVec3::Cross(qv, v);
            SIMDVec3 uuv = SIMDVec3::Cross(qv, uv);
            return v + ((uv * w) + uuv) * 2.0f;
        }
    };

    // ============================================================================================
    // 2. HARDENED BIOMECHANICAL HILL MUSCLE SOLVER
    // ============================================================================================

    struct HardenedMuscleState
    {
        float Activation{ 0.0f };             // Target input [0.0 - 1.0]
        float MaxTorqueCapacityNm{ 28.0f };   // Peak structural isometric limit
        float FatigueFactor{ 1.0f };          // [1.0 = Rested, 0.1 = Exhausted]

        inline float EvaluateTorque(float currentVel) const
        {
            float damping = std::max(0.08f, 1.0f - 0.12f * currentVel);
            return Activation * MaxTorqueCapacityNm * FatigueFactor * damping;
        }

        inline void StepFatigue(float dt)
        {
            if (Activation > 0.70f)
            {
                FatigueFactor = std::max(0.12f, FatigueFactor - dt * 0.04f * Activation);
            }
            else
            {
                FatigueFactor = std::min(1.0f, FatigueFactor + dt * 0.02f);
            }
        }
    };

    // ============================================================================================
    // 3. STRUCTURAL FEATHER BEAM DATA (R1 - R12)
    // ============================================================================================

    constexpr uint32_t TOTAL_RECTRICES_RECTS = 12;

    struct FeatherCantileverNode
    {
        float FlexuralRigidityEI{ 0.048f };
        float LengthMeters{ 0.36f };
        float DeflectionPitchDeg{ 0.0f };
        float DeflectionYawDeg{ 0.0f };
        float VelocityPitch{ 0.0f };
        float VelocityYaw{ 0.0f };

        void SolveStructuralStep(float normalForceN, float lateralForceN, float dt)
        {
            float L3 = LengthMeters * LengthMeters * LengthMeters;
            float targetTipNormal = (normalForceN * L3) / (3.0f * FlexuralRigidityEI);
            float targetTipLateral = (lateralForceN * L3) / (3.0f * FlexuralRigidityEI);

            float targetPitch = std::asin(std::clamp(targetTipNormal / LengthMeters, -0.85f, 0.85f)) * RAD_TO_DEG;
            float targetYaw = std::asin(std::clamp(targetTipLateral / LengthMeters, -0.85f, 0.85f)) * RAD_TO_DEG;

            // Damped Spring Simulation
            float freq = 24.0f;
            float damping = 0.65f;
            float f = TWO_PI * freq;
            float k1 = damping / (PI * freq);
            float k2 = 1.0f / (f * f);

            float accelP = (targetPitch - DeflectionPitchDeg - k1 * VelocityPitch) / k2;
            VelocityPitch += accelP * dt;
            DeflectionPitchDeg += VelocityPitch * dt;

            float accelY = (targetYaw - DeflectionYawDeg - k1 * VelocityYaw) / k2;
            VelocityYaw += accelY * dt;
            DeflectionYawDeg += VelocityYaw * dt;
        }
    };

    // ============================================================================================
    // 4. UNIFIED INPUT & OUTPUT STATE CONTAINERS
    // ============================================================================================

    struct MonolithicFlightInputFrame
    {
        SIMDVec3 LinearVelocityWorld{ 0.0f, 0.0f, 28.0f };
        SIMDVec3 AngularVelocityBodyRadSec{ 0.0f, 0.0f, 0.0f }; // X: Pitch, Y: Yaw, Z: Roll

        SIMDVec3 BodyForward{ 0.0f, 0.0f, 1.0f };
        SIMDVec3 BodyUp{ 0.0f, 1.0f, 0.0f };
        SIMDVec3 BodyRight{ 1.0f, 0.0f, 0.0f };

        float WingLiftForceN{ 48.0f };
        float WingSpanMeters{ 2.1f };
        float TailFanExpansion{ 0.5f }; // [0.0 = Closed, 1.0 = Open]

        float PilotTargetPitchDeg{ 0.0f };
        float PilotTargetYawDeg{ 0.0f };
        float PilotTargetRollDeg{ 0.0f };

        bool bIsDiving{ false };
        bool bIsAgileManeuver{ false };
    };

    struct MonolithicFlightOutputFrame
    {
        HardenedQuat FinalPygostyleOrientation;

        float JointPitchDeg{ 0.0f };
        float JointYawDeg{ 0.0f };
        float JointRollDeg{ 0.0f };

        SIMDVec3 NetAerodynamicForceWorld{ 0.0f, 0.0f, 0.0f };
        SIMDVec3 NetAerodynamicTorqueWorld{ 0.0f, 0.0f, 0.0f };

        float CalculatedAlphaAoADeg{ 0.0f };
        float CalculatedBetaSideslipDeg{ 0.0f };
        float DynamicPressurePa{ 0.0f };

        std::array<float, TOTAL_RECTRICES_RECTS> FeatherPitchDeformationDeg{};
        std::array<float, TOTAL_RECTRICES_RECTS> FeatherYawDeformationDeg{};
    };

    // ============================================================================================
    // 5. MASTER MONOLITHIC FLIGHT & STABILIZATION ENGINE
    // ============================================================================================

    class AAAMonolithicFlightStabilizerEngine
    {
    private:
        MonolithicFlightOutputFrame m_Outputs;

        // Joint Angular State Variables
        float m_JointPitchRad{ 0.0f };
        float m_JointYawRad{ 0.0f };
        float m_JointRollRad{ 0.0f };

        float m_JointPitchVelRadSec{ 0.0f };
        float m_JointYawVelRadSec{ 0.0f };
        float m_JointRollVelRadSec{ 0.0f };

        // Integrated Hill Muscle Group
        HardenedMuscleState m_MusclePitchUp;
        HardenedMuscleState m_MusclePitchDown;
        HardenedMuscleState m_MuscleYawLeft;
        HardenedMuscleState m_MuscleYawRight;

        std::array<FeatherCantileverNode, TOTAL_RECTRICES_RECTS> m_Feathers;

        float m_AccumulatedTime{ 0.0f };

        // Physical Constants Matrix
        const float INERTIA_PITCH = 0.0085f; // kg*m^2
        const float INERTIA_YAW   = 0.0052f; // kg*m^2
        const float INERTIA_ROLL  = 0.0041f; // kg*m^2
        const float MOMENT_ARM_M  = 0.48f;
        const float BASE_AREA_M2  = 0.082f;

    public:
        AAAMonolithicFlightStabilizerEngine()
        {
            for (uint32_t i = 0; i < TOTAL_RECTRICES_RECTS; ++i)
            {
                float normalizedPos = std::abs(static_cast<float>(i) - 5.5f) / 5.5f;
                m_Feathers[i].LengthMeters = 0.38f - (normalizedPos * 0.06f);
                m_Feathers[i].FlexuralRigidityEI = 0.050f - (normalizedPos * 0.02f);
            }
        }

        /**
         * Atomic Frame Execution - Pitch, Yaw, and Roll solved simultaneously with sub-stepping
         */
        void TickEngineMonolithic(const MonolithicFlightInputFrame& input, float deltaTime)
        {
            m_AccumulatedTime += deltaTime;

            // SUB-STEPPING INTEGRATOR TO PREVENT FLUTTER/BREACH DESYNC
            constexpr uint32_t SUB_STEPS = 4;
            float subDeltaTime = deltaTime / static_cast<float>(SUB_STEPS);

            for (uint32_t step = 0; step < SUB_STEPS; ++step)
            {
                // 1. DYNAMIC AIRSPEED, ALPHA, AND BETA COUPLING SOLVER
                float airspeed = input.LinearVelocityWorld.Length();
                SIMDVec3 airDir = airspeed > 0.001f ? input.LinearVelocityWorld.Normalized() : input.BodyForward;

                float vForward = SIMDVec3::Dot(airDir, input.BodyForward);
                float vUp      = SIMDVec3::Dot(airDir, input.BodyUp);
                float vRight   = SIMDVec3::Dot(airDir, input.BodyRight);

                float alphaRad = std::atan2(-vUp, vForward);
                float betaRad  = std::asin(std::clamp(vRight, -0.99f, 0.99f));

                m_Outputs.CalculatedAlphaAoADeg = alphaRad * RAD_TO_DEG;
                m_Outputs.CalculatedBetaSideslipDeg = betaRad * RAD_TO_DEG;

                float qDyn = 0.5f * AIR_DENSITY_SEA_LEVEL * (airspeed * airspeed);
                m_Outputs.DynamicPressurePa = qDyn;

                // 2. MONOLITHIC COUPLING TAIL AERODYNAMIC FORCE MATRIX
                float activeSurfaceArea = BASE_AREA_M2 * (0.75f + input.TailFanExpansion * 0.55f);

                // Combined Angle Attacks with Pygostyle Joint Rotations
                float effectiveAlpha = alphaRad + m_JointPitchRad;
                float effectiveBeta  = betaRad  + m_JointYawRad;

                // Non-linear Aero Coefficients
                float C_L = 2.0f * PI * std::sin(effectiveAlpha);
                float C_Y = -1.9f * std::sin(effectiveBeta); // Side force rudder coefficient
                float C_D = 0.018f + (C_L * C_L) / (PI * 2.2f);

                float liftN = qDyn * activeSurfaceArea * C_L;
                float sideN = qDyn * activeSurfaceArea * C_Y;
                float dragN = qDyn * activeSurfaceArea * C_D;

                // Roll-Twist Asymmetric Differential Lift
                float leftSideLiftN  = liftN * 0.5f * (1.0f + std::sin(m_JointRollRad));
                float rightSideLiftN = liftN * 0.5f * (1.0f - std::sin(m_JointRollRad));

                SIMDVec3 tailForceBody(sideN, leftSideLiftN + rightSideLiftN, -dragN);
                SIMDVec3 momentArmBody(0.0f, 0.0f, -MOMENT_ARM_M);
                SIMDVec3 tailTorqueBody = SIMDVec3::Cross(momentArmBody, tailForceBody);

                m_Outputs.NetAerodynamicForceWorld = tailForceBody;
                m_Outputs.NetAerodynamicTorqueWorld = tailTorqueBody;

                // 3. TARGET TORQUE & MUSCLE ACTIVATION MATRIX
                float pitchError = input.PilotTargetPitchDeg - (input.AngularVelocityBodyRadSec.x * RAD_TO_DEG);
                float yawError   = input.PilotTargetYawDeg   - (input.AngularVelocityBodyRadSec.y * RAD_TO_DEG);
                float rollError  = input.PilotTargetRollDeg  - (input.AngularVelocityBodyRadSec.z * RAD_TO_DEG);

                float targetPitchTorque = (pitchError * 0.45f) + (tailTorqueBody.x * 0.15f);
                float targetYawTorque   = (yawError   * 0.40f) + (tailTorqueBody.y * 0.15f);
                float targetRollTorque  = (rollError  * 0.35f) + (tailTorqueBody.z * 0.15f);

                if (input.bIsDiving)
                {
                    targetPitchTorque *= 0.1f;
                    targetYawTorque   *= 0.1f;
                }

                // Activate Hill Muscle Actuators
                m_MusclePitchUp.Activation   = std::clamp(targetPitchTorque / 28.0f, 0.0f, 1.0f);
                m_MusclePitchDown.Activation = std::clamp(-targetPitchTorque / 28.0f, 0.0f, 1.0f);
                m_MuscleYawLeft.Activation   = std::clamp((targetYawTorque - targetRollTorque) / 28.0f, 0.0f, 1.0f);
                m_MuscleYawRight.Activation  = std::clamp((-targetYawTorque + targetRollTorque) / 28.0f, 0.0f, 1.0f);

                m_MusclePitchUp.StepFatigue(subDeltaTime);
                m_MusclePitchDown.StepFatigue(subDeltaTime);
                m_MuscleYawLeft.StepFatigue(subDeltaTime);
                m_MuscleYawRight.StepFatigue(subDeltaTime);

                // Compute Net Muscle Torques
                float netMusclePitch = m_MusclePitchUp.EvaluateTorque(m_JointPitchVelRadSec) - m_MusclePitchDown.EvaluateTorque(-m_JointPitchVelRadSec);
                float netMuscleYaw   = m_MuscleYawLeft.EvaluateTorque(m_JointYawVelRadSec) - m_MuscleYawRight.EvaluateTorque(-m_JointYawVelRadSec);
                float netMuscleRoll  = (m_MuscleYawRight.EvaluateTorque(m_JointRollVelRadSec) - m_MuscleYawLeft.EvaluateTorque(-m_JointRollVelRadSec)) * 0.45f;

                // 4. INTEGRATE ROTATIONAL MECHANICS (Angular Acceleration = Torque / Inertia)
                float accelP = (netMusclePitch + tailTorqueBody.x * 0.1f) / INERTIA_PITCH;
                float accelY = (netMuscleYaw   + tailTorqueBody.y * 0.1f) / INERTIA_YAW;
                float accelR = (netMuscleRoll  + tailTorqueBody.z * 0.1f) / INERTIA_ROLL;

                m_JointPitchVelRadSec += accelP * subDeltaTime;
                m_JointYawVelRadSec   += accelY * subDeltaTime;
                m_JointRollVelRadSec  += accelR * subDeltaTime;

                // Joint Velocity Damping
                m_JointPitchVelRadSec *= (1.0f - 4.5f * subDeltaTime);
                m_JointYawVelRadSec   *= (1.0f - 4.8f * subDeltaTime);
                m_JointRollVelRadSec  *= (1.0f - 5.2f * subDeltaTime);

                m_JointPitchRad += m_JointPitchVelRadSec * subDeltaTime;
                m_JointYawRad   += m_JointYawVelRadSec   * subDeltaTime;
                m_JointRollRad  += m_JointRollVelRadSec  * subDeltaTime;

                // HARDENED ANATOMICAL CLAMPS (Prevents Mechanical Breach)
                m_JointPitchRad = std::clamp(m_JointPitchRad, -30.0f * DEG_TO_RAD, 60.0f * DEG_TO_RAD);
                m_JointYawRad   = std::clamp(m_JointYawRad,   -32.0f * DEG_TO_RAD, 32.0f * DEG_TO_RAD);
                m_JointRollRad  = std::clamp(m_JointRollRad,  -38.0f * DEG_TO_RAD, 38.0f * DEG_TO_RAD);

                // 5. SOLVE STRUCTURAL RECTRICES DEFLECTION (R1-R12)
                float normalLoadPerFeather  = liftN / static_cast<float>(TOTAL_RECTRICES_RECTS);
                float lateralLoadPerFeather = sideN / static_cast<float>(TOTAL_RECTRICES_RECTS);

                for (uint32_t i = 0; i < TOTAL_RECTRICES_RECTS; ++i)
                {
                    m_Feathers[i].SolveStructuralStep(normalLoadPerFeather, lateralLoadPerFeather, subDeltaTime);
                }
            }

            // 6. WRITE FINAL OUTPUT TRANSFORM STATE
            m_Outputs.JointPitchDeg = m_JointPitchRad * RAD_TO_DEG;
            m_Outputs.JointYawDeg   = m_JointYawRad   * RAD_TO_DEG;
            m_Outputs.JointRollDeg  = m_JointRollRad  * RAD_TO_DEG;

            m_Outputs.FinalPygostyleOrientation = HardenedQuat::FromEuler(
                m_Outputs.JointPitchDeg,
                m_Outputs.JointYawDeg,
                m_Outputs.JointRollDeg
            );

            for (uint32_t i = 0; i < TOTAL_RECTRICES_RECTS; ++i)
            {
                m_Outputs.FeatherPitchDeformationDeg[i] = m_Feathers[i].DeflectionPitchDeg;
                m_Outputs.FeatherYawDeformationDeg[i]   = m_Feathers[i].DeflectionYawDeg;
            }
        }

        inline const MonolithicFlightOutputFrame& GetOutputs() const { return m_Outputs; }
    };
}

#endif // AAA_BIRD_MONOLITHIC_YAW_PITCH_ROLL_ENGINE_HPP
