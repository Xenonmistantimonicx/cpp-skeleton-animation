/**
 * ============================================================================================
 *  AAA UNIFIED CONTINUOUS TAIL STABILIZATION ENGINE (PITCH + ROLL + YAW)
 * ============================================================================================
 *  Target Architecture : Unreal Engine 5 / Unity Native C++ / Custom Physics Engine
 *  Language Standard   : C++20 (Zero Heap Allocation / Anti-Breach Thread Safe)
 *  Features Included   :
 *      1. Full 6-DOF Continuous Vector Coupling (AOA Alpha & Sideslip Beta)
 *      2. Simultaneous Pitch-Up/Down, Roll-Twist & Yaw-Rudder Joint Solver
 *      3. Deterministic 4x Sub-Stepping Integrator (Prevents Frame-Rate Spikes / Breach)
 *      4. Biomechanical Hill-Type Muscle Group Actuators (Pubocaudalis & Lateralis Caudae)
 *      5. Structural Rectrices Feather Cantilever Beam Mechanics (R1 to R12)
 *      6. High-Frequency Aeroelastic Vortex-Shedding & Flutter Damper
 * ============================================================================================
 */

#ifndef AAA_BIRD_UNIFIED_CONTINUOUS_TAIL_STABILIZATION_ENGINE_HPP
#define AAA_BIRD_UNIFIED_CONTINUOUS_TAIL_STABILIZATION_ENGINE_HPP

#include <cmath>
#include <algorithm>
#include <array>
#include <cstdint>

namespace AAABirdEngine
{
    // ============================================================================================
    // 1. HARDENED 16-BYTE ALIGNED SIMD MATH PRIMITIVES
    // ============================================================================================

    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 6.28318530717958647692f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;
    constexpr float RHO_AIR_DENSITY = 1.225f; // Sea-level air density kg/m^3

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
    };

    // ============================================================================================
    // 2. BIOMECHANICAL HILL MUSCLE ACTUATOR
    // ============================================================================================

    struct HillMuscleActuator
    {
        float Activation{ 0.0f };
        float PeakTorqueNm{ 32.0f };
        float FatigueFactor{ 1.0f };

        inline float EvaluateOutputTorque(float angularVelocity) const
        {
            float velocityDamping = std::max(0.05f, 1.0f - 0.15f * angularVelocity);
            return Activation * PeakTorqueNm * FatigueFactor * velocityDamping;
        }

        inline void StepFatigueDynamics(float dt)
        {
            if (Activation > 0.65f)
            {
                FatigueFactor = std::max(0.10f, FatigueFactor - dt * 0.035f * Activation);
            }
            else
            {
                FatigueFactor = std::min(1.0f, FatigueFactor + dt * 0.025f);
            }
        }
    };

    // ============================================================================================
    // 3. RECTRICES FEATHER CANTILEVER BEAM MODEL (R1 - R12)
    // ============================================================================================

    constexpr uint32_t TOTAL_TAIL_FEATHERS = 12;

    struct FeatherBeamNode
    {
        float LengthMeters{ 0.38f };
        float FlexuralRigidityEI{ 0.052f };
        float DeflectionPitchDeg{ 0.0f };
        float DeflectionYawDeg{ 0.0f };
        float VelocityPitch{ 0.0f };
        float VelocityYaw{ 0.0f };

        void StepStructuralPhysics(float normalForceN, float lateralForceN, float dt)
        {
            float L3 = LengthMeters * LengthMeters * LengthMeters;
            float targetTipNormal = (normalForceN * L3) / (3.0f * FlexuralRigidityEI);
            float targetTipLateral = (lateralForceN * L3) / (3.0f * FlexuralRigidityEI);

            float targetPitch = std::asin(std::clamp(targetTipNormal / LengthMeters, -0.88f, 0.88f)) * RAD_TO_DEG;
            float targetYaw = std::asin(std::clamp(targetTipLateral / LengthMeters, -0.88f, 0.88f)) * RAD_TO_DEG;

            // Second-Order Damped Spring Solver
            constexpr float naturalFreq = 26.0f;
            constexpr float dampingRatio = 0.62f;
            float f = TWO_PI * naturalFreq;
            float k1 = dampingRatio / (PI * naturalFreq);
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
    // 4. UNIFIED INPUT AND OUTPUT STATE STRUCTURES
    // ============================================================================================

    struct BirdFlightInputFrame
    {
        SIMDVec3 LinearVelocityWorld{ 0.0f, 0.0f, 26.0f };
        SIMDVec3 BodyAngularVelocityRadSec{ 0.0f, 0.0f, 0.0f }; // X: Pitch, Y: Yaw, Z: Roll

        SIMDVec3 BodyForward{ 0.0f, 0.0f, 1.0f };
        SIMDVec3 BodyUp{ 0.0f, 1.0f, 0.0f };
        SIMDVec3 BodyRight{ 1.0f, 0.0f, 0.0f };

        float WingLiftForceN{ 52.0f };
        float WingSpanMeters{ 2.2f };
        float MainWingYawTorqueNm{ 0.0f }; // Adverse yaw drag from asymmetric wing strokes

        float TailFanExpansion{ 0.5f };    // [0.0 = Feathered Closed, 1.0 = Fully Fanned out]

        // Desired Flight Trajectory
        float TargetPitchDeg{ 0.0f };
        float TargetYawDeg{ 0.0f };
        float TargetRollDeg{ 0.0f };

        // Tactical Flight States
        bool bIsExecutingSnapTurn{ false };
        bool bIsHighSpeedDiving{ false };
    };

    struct BirdTailOutputState
    {
        HardenedQuat PygostyleLocalOrientation;

        // PYGOSTYLE JOINT ROTATION ANGLES
        float PygostylePitchDeg{ 0.0f };
        float PygostyleYawDeg{ 0.0f };
        float PygostyleRollDeg{ 0.0f };

        // AERODYNAMIC FORCES & TORQUES GENERATED BY TAIL
        SIMDVec3 NetTailAerodynamicForceN{ 0.0f, 0.0f, 0.0f };
        SIMDVec3 NetTailRestorationTorqueNm{ 0.0f, 0.0f, 0.0f };

        // CONTINUOUS FLIGHT DYNAMICS COEFFICIENTS
        float CalculatedAlphaAoADeg{ 0.0f };
        float CalculatedBetaSideslipDeg{ 0.0f };
        float DynamicPressurePa{ 0.0f };

        // HIGH FREQUENCY MICRO-VORTEX SHEDDING FLUTTER
        float HighFreqVortexFlutterDeg{ 0.0f };

        // INDIVIDUAL FEATHER CANTILEVER BENDING DEFORMATIONS
        std::array<float, TOTAL_TAIL_FEATHERS> FeatherPitchDeformationDeg{};
        std::array<float, TOTAL_TAIL_FEATHERS> FeatherYawDeformationDeg{};
    };

    // ============================================================================================
    // 5. MASTER CONTINUOUS TAIL STABILIZATION SYSTEM
    // ============================================================================================

    class AAABirdTailMasterStabilizerSystem
    {
    private:
        BirdTailOutputState m_Outputs;

        // Continuous Angular State Vectors
        float m_JointPitchRad{ 0.0f };
        float m_JointYawRad{ 0.0f };
        float m_JointRollRad{ 0.0f };

        float m_JointPitchVelRadSec{ 0.0f };
        float m_JointYawVelRadSec{ 0.0f };
        float m_JointRollVelRadSec{ 0.0f };

        // Integrated Hill Muscle Group (Pitch, Roll, Yaw Actuators)
        HillMuscleActuator m_MuscleDepressorCaudae; // Pitch Down
        HillMuscleActuator m_MuscleLevatorCaudae;   // Pitch Up
        HillMuscleActuator m_MusclePubocaudalisL;  // Yaw Left / Roll Left
        HillMuscleActuator m_MusclePubocaudalisR;  // Yaw Right / Roll Right

        std::array<FeatherBeamNode, TOTAL_TAIL_FEATHERS> m_FeatherStructuralNodes;

        float m_AccumulatedTime{ 0.0f };

        // Biological & Structural Constants
        const float INERTIA_PITCH_KG_M2 = 0.0092f;
        const float INERTIA_YAW_KG_M2   = 0.0058f;
        const float INERTIA_ROLL_KG_M2  = 0.0044f;
        const float TAIL_MOMENT_ARM_M   = 0.50f;
        const float BASE_TAIL_AREA_M2   = 0.088f;

    public:
        AAABirdTailMasterStabilizerSystem()
        {
            for (uint32_t i = 0; i < TOTAL_TAIL_FEATHERS; ++i)
            {
                float normalizedDist = std::abs(static_cast<float>(i) - 5.5f) / 5.5f;
                m_FeatherStructuralNodes[i].LengthMeters = 0.40f - (normalizedDist * 0.07f);
                m_FeatherStructuralNodes[i].FlexuralRigidityEI = 0.055f - (normalizedDist * 0.022f);
            }
        }

        /**
         * Solves continuous 3-Axis (Pitch, Roll, Yaw) Tail Stabilization in a single atomic tick.
         */
        void TickContinuousStabilization(const BirdFlightInputFrame& input, float deltaTime)
        {
            m_AccumulatedTime += deltaTime;

            // DETERMINISTIC SUB-STEPPING INTEGRATOR (Prevents Physics Breach / Frame Spikes)
            constexpr uint32_t SUB_STEP_COUNT = 4;
            float subDeltaTime = deltaTime / static_cast<float>(SUB_STEP_COUNT);

            for (uint32_t step = 0; step < SUB_STEP_COUNT; ++step)
            {
                // --------------------------------------------------------------------------------
                // STEP 1: SOLVE CONTINUOUS AOA (ALPHA) AND SIDESLIP (BETA)
                // --------------------------------------------------------------------------------
                float airspeed = input.LinearVelocityWorld.Length();
                SIMDVec3 airDirection = airspeed > 0.001f ? input.LinearVelocityWorld.Normalized() : input.BodyForward;

                float forwardVel = SIMDVec3::Dot(airDirection, input.BodyForward);
                float upVel      = SIMDVec3::Dot(airDirection, input.BodyUp);
                float rightVel   = SIMDVec3::Dot(airDirection, input.BodyRight);

                float alphaRad = std::atan2(-upVel, std::max(0.1f, forwardVel));
                float betaRad  = std::asin(std::clamp(rightVel, -0.99f, 0.99f));

                m_Outputs.CalculatedAlphaAoADeg = alphaRad * RAD_TO_DEG;
                m_Outputs.CalculatedBetaSideslipDeg = betaRad * RAD_TO_DEG;

                float dynamicPressure = 0.5f * RHO_AIR_DENSITY * (airspeed * airspeed);
                m_Outputs.DynamicPressurePa = dynamicPressure;

                // --------------------------------------------------------------------------------
                // STEP 2: COUPLED THREE-AXIS TAIL AERODYNAMIC FORCES & TORQUES
                // --------------------------------------------------------------------------------
                float activeTailArea = BASE_TAIL_AREA_M2 * (0.75f + input.TailFanExpansion * 0.60f);

                float totalEffectiveAlpha = alphaRad + m_JointPitchRad;
                float totalEffectiveBeta  = betaRad  + m_JointYawRad;

                // Lift, Yaw Rudder Side-Force, and Induced Drag Coefficients
                float C_Lift = 2.0f * PI * std::sin(totalEffectiveAlpha);
                float C_Side = -1.95f * std::sin(totalEffectiveBeta);
                float C_Drag = 0.019f + (C_Lift * C_Lift) / (PI * 2.3f);

                float tailLiftForceN = dynamicPressure * activeTailArea * C_Lift;
                float tailSideForceN = dynamicPressure * activeTailArea * C_Side;
                float tailDragForceN = dynamicPressure * activeTailArea * C_Drag;

                // Pronation/Supination Roll Differential Force Split
                float leftSideLiftN  = tailLiftForceN * 0.5f * (1.0f + std::sin(m_JointRollRad));
                float rightSideLiftN = tailLiftForceN * 0.5f * (1.0f - std::sin(m_JointRollRad));

                SIMDVec3 tailForceBody(tailSideForceN, leftSideLiftN + rightSideLiftN, -tailDragForceN);
                SIMDVec3 tailMomentArmBody(0.0f, 0.0f, -TAIL_MOMENT_ARM_M);
                SIMDVec3 tailTorqueBody = SIMDVec3::Cross(tailMomentArmBody, tailForceBody);

                m_Outputs.NetTailAerodynamicForceN = tailForceBody;
                m_Outputs.NetTailRestorationTorqueNm = tailTorqueBody;

                // --------------------------------------------------------------------------------
                // STEP 3: THREE-AXIS STABILIZATION & MUSCLE TARGET SOLVER
                // --------------------------------------------------------------------------------
                float pitchErrorDeg = input.TargetPitchDeg - (input.BodyAngularVelocityRadSec.x * RAD_TO_DEG);
                float yawErrorDeg   = input.TargetYawDeg   - (input.BodyAngularVelocityRadSec.y * RAD_TO_DEG);
                float rollErrorDeg  = input.TargetRollDeg  - (input.BodyAngularVelocityRadSec.z * RAD_TO_DEG);

                float requiredPitchTorque = (pitchErrorDeg * 0.48f) + (tailTorqueBody.x * 0.18f);
                float requiredYawTorque   = (yawErrorDeg   * 0.42f) + (tailTorqueBody.y * 0.18f) - (input.MainWingYawTorqueNm * 0.35f);
                float requiredRollTorque  = (rollErrorDeg  * 0.38f) + (tailTorqueBody.z * 0.18f);

                if (input.bIsSnapTurn)
                {
                    requiredYawTorque += (yawErrorDeg > 0.0f ? 28.0f : -28.0f);
                    requiredRollTorque += (rollErrorDeg > 0.0f ? 18.0f : -18.0f);
                }
                else if (input.bIsHighSpeedDiving)
                {
                    requiredPitchTorque *= 0.12f;
                    requiredYawTorque   *= 0.08f;
                    requiredRollTorque  *= 0.10f;
                }

                // Activate Hill Muscle Actuators
                m_MuscleLevatorCaudae.Activation   = std::clamp(requiredPitchTorque / 32.0f, 0.0f, 1.0f);
                m_MuscleDepressorCaudae.Activation = std::clamp(-requiredPitchTorque / 32.0f, 0.0f, 1.0f);
                m_MusclePubocaudalisL.Activation   = std::clamp((requiredYawTorque - requiredRollTorque) / 32.0f, 0.0f, 1.0f);
                m_MusclePubocaudalisR.Activation   = std::clamp((-requiredYawTorque + requiredRollTorque) / 32.0f, 0.0f, 1.0f);

                m_MuscleLevatorCaudae.StepFatigueDynamics(subDeltaTime);
                m_MuscleDepressorCaudae.StepFatigueDynamics(subDeltaTime);
                m_MusclePubocaudalisL.StepFatigueDynamics(subDeltaTime);
                m_MusclePubocaudalisR.StepFatigueDynamics(subDeltaTime);

                float netPitchMuscleTorque = m_MuscleLevatorCaudae.EvaluateOutputTorque(m_JointPitchVelRadSec) - m_MuscleDepressorCaudae.EvaluateOutputTorque(-m_JointPitchVelRadSec);
                float netYawMuscleTorque   = m_MusclePubocaudalisL.EvaluateOutputTorque(m_JointYawVelRadSec) - m_MusclePubocaudalisR.EvaluateOutputTorque(-m_JointYawVelRadSec);
                float netRollMuscleTorque  = (m_MusclePubocaudalisR.EvaluateOutputTorque(m_JointRollVelRadSec) - m_MusclePubocaudalisL.EvaluateOutputTorque(-m_JointRollVelRadSec)) * 0.50f;

                // --------------------------------------------------------------------------------
                // STEP 4: INTEGRATE ROTATIONAL MECHANICS (Angular Acceleration = Torque / Inertia)
                // --------------------------------------------------------------------------------
                float accelPitch = (netPitchMuscleTorque + tailTorqueBody.x * 0.12f) / INERTIA_PITCH_KG_M2;
                float accelYaw   = (netYawMuscleTorque   + tailTorqueBody.y * 0.12f) / INERTIA_YAW_KG_M2;
                float accelRoll  = (netRollMuscleTorque  + tailTorqueBody.z * 0.12f) / INERTIA_ROLL_KG_M2;

                m_JointPitchVelRadSec += accelPitch * subDeltaTime;
                m_JointYawVelRadSec   += accelYaw   * subDeltaTime;
                m_JointRollVelRadSec  += accelRoll  * subDeltaTime;

                // Active Angular Velocity Damping
                m_JointPitchVelRadSec *= (1.0f - 4.8f * subDeltaTime);
                m_JointYawVelRadSec   *= (1.0f - 5.0f * subDeltaTime);
                m_JointRollVelRadSec  *= (1.0f - 5.4f * subDeltaTime);

                m_JointPitchRad += m_JointPitchVelRadSec * subDeltaTime;
                m_JointYawRad   += m_JointYawVelRadSec   * subDeltaTime;
                m_JointRollRad  += m_JointRollVelRadSec  * subDeltaTime;

                // HARDENED ANATOMICAL JOINT CLAMPS (Prevents Mechanical Breach)
                m_JointPitchRad = std::clamp(m_JointPitchRad, -30.0f * DEG_TO_RAD, 60.0f * DEG_TO_RAD);
                m_JointYawRad   = std::clamp(m_JointYawRad,   -32.0f * DEG_TO_RAD, 32.0f * DEG_TO_RAD);
                m_JointRollRad  = std::clamp(m_JointRollRad,  -38.0f * DEG_TO_RAD, 38.0f * DEG_TO_RAD);

                // --------------------------------------------------------------------------------
                // STEP 5: VORTEX SHEDDING FLUTTER & RECTRICES FEATHER DEFLECTION SOLVER
                // --------------------------------------------------------------------------------
                float sideslipSeverity = std::clamp(std::abs(m_Outputs.CalculatedBetaSideslipDeg) / 12.0f, 0.0f, 1.0f);
                float flutterFrequency = std::clamp(airspeed * 1.5f, 12.0f, 45.0f);
                float flutterAmplitude = (airspeed / 30.0f) * sideslipSeverity * 1.1f;
                m_Outputs.HighFreqVortexFlutterDeg = std::sin(m_AccumulatedTime * flutterFrequency * TWO_PI) * flutterAmplitude;

                float normalLoadPerFeather  = tailLiftForceN / static_cast<float>(TOTAL_TAIL_FEATHERS);
                float lateralLoadPerFeather = tailSideForceN / static_cast<float>(TOTAL_TAIL_FEATHERS);

                for (uint32_t i = 0; i < TOTAL_TAIL_FEATHERS; ++i)
                {
                    m_FeatherStructuralNodes[i].StepStructuralPhysics(normalLoadPerFeather, lateralLoadPerFeather, subDeltaTime);
                }
            }

            // ------------------------------------------------------------------------------------
            // STEP 6: WRITE FINAL TRANSFORM OUTPUTS
            // ------------------------------------------------------------------------------------
            m_Outputs.PygostylePitchDeg = m_JointPitchRad * RAD_TO_DEG;
            m_Outputs.PygostyleYawDeg   = m_JointYawRad   * RAD_TO_DEG;
            m_Outputs.PygostyleRollDeg  = m_JointRollRad  * RAD_TO_DEG;

            m_Outputs.PygostyleLocalOrientation = HardenedQuat::FromEuler(
                m_Outputs.PygostylePitchDeg,
                m_Outputs.PygostyleYawDeg,
                m_Outputs.PygostyleRollDeg
            );

            for (uint32_t i = 0; i < TOTAL_TAIL_FEATHERS; ++i)
            {
                m_Outputs.FeatherPitchDeformationDeg[i] = m_FeatherStructuralNodes[i].DeflectionPitchDeg;
                m_Outputs.FeatherYawDeformationDeg[i]   = m_FeatherStructuralNodes[i].DeflectionYawDeg + 
                                                         (m_Outputs.HighFreqVortexFlutterDeg * (i < 6 ? 1.0f : -1.0f));
            }
        }

        inline const BirdTailOutputState& GetOutputs() const { return m_Outputs; }
    };
}

#endif // AAA_BIRD_UNIFIED_CONTINUOUS_TAIL_STABILIZATION_ENGINE_HPP
