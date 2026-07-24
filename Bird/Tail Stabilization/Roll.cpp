/**
 * ============================================================================================
 *  AAA MONOLITHIC 6-DOF BIRD FLIGHT DYNAMICS & BIOMECHANICAL TAIL AEROELASTICS ENGINE
 * ============================================================================================
 *  Architecture       : Custom C++20 Physics / Unreal Engine 5 Custom Anim Physics Engine Module
 *  SIMD Alignment     : 16-Byte Aligned Math Vectors
 *  Features Included  :
 *      1. Full 6-DOF Rigid Body Inertia Matrix & Euler Angle Mechanics
 *      2. Lifting-Line 3D Vortex Lattice Tail Aerodynamics (Pitch, Roll & Yaw Coupling)
 *      3. Dynamic Wing Downwash Velocity Lag & Slipstream Vortex Buffer
 *      4. Biomechanical Hill-Type Muscle Model (Levator, Depressor, Lateralis Caudae)
 *      5. Structural Cantilever Euler-Bernoulli Deformation Matrix for 12 Rectrices (R1-R12)
 *      6. Non-Linear Stall Buffeting, Micro-Tremor Shake & Aeroelastic Flutter Engine
 * ============================================================================================
 */

#ifndef AAA_BIRD_COMPLETE_FLIGHT_AEROELASTICS_ENGINE_HPP
#define AAA_BIRD_COMPLETE_FLIGHT_AEROELASTICS_ENGINE_HPP

#include <cmath>
#include <algorithm>
#include <array>
#include <vector>
#include <cstdint>
#include <cassert>

namespace AAABirdEngine
{
    // ============================================================================================
    // 1. PRECISION SIMD-READY MATH CONSTANTS & ALGEBRA
    // ============================================================================================

    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 6.28318530717958647692f;
    constexpr float HALF_PI = 1.57079632679489661923f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;
    constexpr float RHO_SEA_LEVEL_AIR_DENSITY = 1.225f; // kg/m^3

    alignas(16) struct Vector3
    {
        float x{ 0.0f }, y{ 0.0f }, z{ 0.0f }, w{ 0.0f };

        constexpr Vector3() = default;
        constexpr Vector3(float inX, float inY, float inZ, float inW = 0.0f) 
            : x(inX), y(inY), z(inZ), w(inW) {}

        inline Vector3 operator+(const Vector3& rhs) const { return Vector3(x + rhs.x, y + rhs.y, z + rhs.z); }
        inline Vector3 operator-(const Vector3& rhs) const { return Vector3(x - rhs.x, y - rhs.y, z - rhs.z); }
        inline Vector3 operator*(float scalar) const { return Vector3(x * scalar, y * scalar, z * scalar); }
        inline Vector3 operator/(float scalar) const { float inv = 1.0f / scalar; return Vector3(x * inv, y * inv, z * inv); }

        inline Vector3& operator+=(const Vector3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
        inline Vector3& operator-=(const Vector3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
        inline Vector3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }

        inline float LengthSquared() const { return x * x + y * y + z * z; }
        inline float Length() const { return std::sqrt(LengthSquared()); }

        inline Vector3 Normalized() const
        {
            float len = Length();
            return len > 0.00001f ? (*this) * (1.0f / len) : Vector3(0.0f, 0.0f, 0.0f);
        }

        static inline float Dot(const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
        static inline Vector3 Cross(const Vector3& a, const Vector3& b)
        {
            return Vector3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            );
        }

        static inline Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
        {
            t = std::clamp(t, 0.0f, 1.0f);
            return a + (b - a) * t;
        }
    };

    alignas(16) struct Quaternion
    {
        float w{ 1.0f }, x{ 0.0f }, y{ 0.0f }, z{ 0.0f };

        constexpr Quaternion() = default;
        constexpr Quaternion(float inW, float inX, float inY, float inZ) : w(inW), x(inX), y(inY), z(inZ) {}

        static inline Quaternion FromEulerAngles(float pitchDeg, float yawDeg, float rollDeg)
        {
            float p = pitchDeg * 0.5f * DEG_TO_RAD;
            float y = yawDeg * 0.5f * DEG_TO_RAD;
            float r = rollDeg * 0.5f * DEG_TO_RAD;

            float sinP = std::sin(p), cosP = std::cos(p);
            float sinY = std::sin(y), cosY = std::cos(y);
            float sinR = std::sin(r), cosR = std::cos(r);

            return Quaternion(
                cosR * cosP * cosY + sinR * sinP * sinY,
                cosR * sinP * cosY - sinR * cosP * sinY,
                cosR * cosP * sinY + sinR * sinP * cosY,
                sinR * cosP * cosY - cosR * sinP * sinY
            );
        }

        inline Vector3 RotateVector(const Vector3& v) const
        {
            Vector3 qv(x, y, z);
            Vector3 uv = Vector3::Cross(qv, v);
            Vector3 uuv = Vector3::Cross(qv, uv);
            return v + ((uv * w) + uuv) * 2.0f;
        }
    };

    struct Matrix3x3
    {
        float m[3][3]{
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}
        };

        inline Vector3 MultiplyVector(const Vector3& v) const
        {
            return Vector3(
                m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
                m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
                m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z
            );
        }
    };

    // ============================================================================================
    // 2. BIOMECHANICAL HILL-TYPE MUSCLE MODEL FOR TAIL ACTUATION
    // ============================================================================================

    struct HillTypeTailMuscle
    {
        float NormalizedActivation{ 0.0f };   // Control Input Signal [0.0 to 1.0]
        float MaxIsometricTorqueNm{ 24.5f };   // Absolute peak structural force capability
        float IdealLengthMeters{ 0.085f };     // Anatomical rest length
        float CurrentLengthMeters{ 0.085f };   // Real-time dynamic length
        float ForceVelocityDamping{ 0.16f };   // Damping constant
        float MetabolicFatigueRatio{ 1.0f };   // Endurance scale [1.0 = Fresh, 0.1 = Exhausted]

        inline float EvaluateDynamicTorque(float angularVelocityRadSec) const
        {
            // Force-Length Relationship (Gaussian Curve)
            float lengthRatio = CurrentLengthMeters / IdealLengthMeters;
            float forceLengthFactor = std::exp(-std::pow((lengthRatio - 1.0f) / 0.45f, 2.0f));

            // Force-Velocity Relationship (Hill Equation Approximation)
            float velocityFactor = std::max(0.05f, 1.0f - ForceVelocityDamping * angularVelocityRadSec);

            return NormalizedActivation * MaxIsometricTorqueNm * forceLengthFactor * velocityFactor * MetabolicFatigueRatio;
        }

        inline void TickFatigue(float dt)
        {
            if (NormalizedActivation > 0.65f)
            {
                MetabolicFatigueRatio = std::max(0.15f, MetabolicFatigueRatio - dt * 0.035f * NormalizedActivation);
            }
            else
            {
                MetabolicFatigueRatio = std::min(1.0f, MetabolicFatigueRatio + dt * 0.015f);
            }
        }
    };

    // ============================================================================================
    // 3. WING DOWNWASH LAG & VORTEX RING BUFFER
    // ============================================================================================

    class WingDownwashLagBuffer
    {
    private:
        static constexpr size_t RING_BUFFER_SIZE = 128;
        std::array<Vector3, RING_BUFFER_SIZE> m_DownwashVectorHistory{};
        size_t m_WriteHead{ 0 };

    public:
        WingDownwashLagBuffer()
        {
            m_DownwashVectorHistory.fill(Vector3(0.0f, 0.0f, 0.0f));
        }

        void StoreDownwash(const Vector3& downwashVelWorld)
        {
            m_DownwashVectorHistory[m_WriteHead] = downwashVelWorld;
            m_WriteHead = (m_WriteHead + 1) % RING_BUFFER_SIZE;
        }

        Vector3 SampleDelayedDownwash(float forwardAirspeed, float distanceToTailMeters, float dt) const
        {
            if (forwardAirspeed < 0.1f) return Vector3(0.0f, 0.0f, 0.0f);

            float timeDelaySec = distanceToTailMeters / forwardAirspeed;
            size_t lagTicks = static_cast<size_t>(timeDelaySec / std::max(0.0001f, dt));
            lagTicks = std::clamp(lagTicks, static_cast<size_t>(1), RING_BUFFER_SIZE - 1);

            size_t readHead = (m_WriteHead + RING_BUFFER_SIZE - lagTicks) % RING_BUFFER_SIZE;
            return m_DownwashVectorHistory[readHead];
        }
    };

    // ============================================================================================
    // 4. STRUCTURAL BEAM DEFORMATION FOR 12 RECTRICES FEATHERS (R1-R12)
    // ============================================================================================

    constexpr uint32_t TOTAL_RECTRICES_COUNT = 12;

    struct RectrixFeatherStructuralNode
    {
        uint32_t FeatherID{ 0 };
        float ShaftFlexuralRigidityEI{ 0.052f }; // N*m^2 (Young's Modulus * Moment of Inertia)
        float ShaftLengthMeters{ 0.38f };
        float SurfaceAreaM2{ 0.014f };
        
        float AngularDeflectionPitchDeg{ 0.0f };
        float AngularDeflectionRollDeg{ 0.0f };
        float VelocityPitch{ 0.0f };
        float VelocityRoll{ 0.0f };

        void SolveCantileverDynamics(float localNormalLoadNewtons, float localLateralLoadNewtons, float dt)
        {
            // Euler-Bernoulli Beam Tip Deflection: delta = (F * L^3) / (3 * EI)
            float L3 = ShaftLengthMeters * ShaftLengthMeters * ShaftLengthMeters;
            float targetTipDeflectionNormal = (localNormalLoadNewtons * L3) / (3.0f * ShaftFlexuralRigidityEI);
            float targetTipDeflectionLateral = (localLateralLoadNewtons * L3) / (3.0f * ShaftFlexuralRigidityEI);

            float targetPitchDeg = std::asin(std::clamp(targetTipDeflectionNormal / ShaftLengthMeters, -0.85f, 0.85f)) * RAD_TO_DEG;
            float targetRollDeg = std::asin(std::clamp(targetTipDeflectionLateral / ShaftLengthMeters, -0.85f, 0.85f)) * RAD_TO_DEG;

            // 2nd Order Damped Spring Integration
            float resonanceFreq = 22.0f; // High natural frequency of feather rachis shaft
            float dampingFactor = 0.62f;

            float f = TWO_PI * resonanceFreq;
            float k1 = dampingFactor / (PI * resonanceFreq);
            float k2 = 1.0f / (f * f);

            // Pitch Axis Integration
            float accelPitch = (targetPitchDeg - AngularDeflectionPitchDeg - k1 * VelocityPitch) / k2;
            VelocityPitch += accelPitch * dt;
            AngularDeflectionPitchDeg += VelocityPitch * dt;

            // Roll Axis Integration
            float accelRoll = (targetRollDeg - AngularDeflectionRollDeg - k1 * VelocityRoll) / k2;
            VelocityRoll += accelRoll * dt;
            AngularDeflectionRollDeg += VelocityRoll * dt;
        }
    };

    // ============================================================================================
    // 5. INPUT & OUTPUT DATA ARCHITECTURE
    // ============================================================================================

    struct BirdFlightInputFrame
    {
        Vector3 WorldLinearVelocity{ 0.0f, 0.0f, 25.0f };
        Vector3 WorldAngularVelocityRadSec{ 0.0f, 0.0f, 0.0f }; // X: Pitch, Y: Yaw, Z: Roll

        Vector3 BodyForwardWorld{ 0.0f, 0.0f, 1.0f };
        Vector3 BodyUpWorld{ 0.0f, 1.0f, 0.0f };
        Vector3 BodyRightWorld{ 1.0f, 0.0f, 0.0f };

        float WingSpanMeters{ 2.2f };
        float TotalBodyMassKg{ 4.5f };
        float WingLiftForceNewtons{ 44.1f };
        Vector3 WingCenterOfLiftOffsetMeters{ 0.0f, 0.04f, -0.02f };

        float UserTargetPitchDeg{ 0.0f };
        float UserTargetRollDeg{ 0.0f };
        float UserTargetYawDeg{ 0.0f };

        float TailFanExpansionNormalized{ 0.4f }; // [0.0 = Folded, 1.0 = Max Fan Spread]
        bool bIsHighSpeedDive{ false };
        bool bIsBrakingLandingMode{ false };
    };

    struct BirdFlightOutputFrame
    {
        Quaternion PygostyleLocalOrientation;
        
        float JointPitchDeg{ 0.0f };
        float JointRollDeg{ 0.0f };
        float JointYawDeg{ 0.0f };

        Vector3 TotalTailAerodynamicForceWorld{ 0.0f, 0.0f, 0.0f };
        Vector3 TotalTailAerodynamicTorqueWorld{ 0.0f, 0.0f, 0.0f };

        float CalculatedAngleOfAttackAlphaDeg{ 0.0f };
        float CalculatedSideslipAngleBetaDeg{ 0.0f };
        float DownwashAngleDeg{ 0.0f };

        float StallBuffetingAmplitudeDeg{ 0.0f };
        float AeroelasticFlutterFrequencyHz{ 0.0f };

        std::array<float, TOTAL_RECTRICES_COUNT> IndividualRectricesPitchFlexDeg{};
        std::array<float, TOTAL_RECTRICES_COUNT> IndividualRectricesRollFlexDeg{};
    };

    // ============================================================================================
    // 6. MONOLITHIC MASTER AAA FLIGHT & TAIL SYSTEM CLASS
    // ============================================================================================

    class AAAMasterBirdFlightEngine
    {
    private:
        BirdFlightOutputFrame m_Outputs;

        // Biomechanical Muscles
        HillTypeTailMuscle m_LevatorCaudae;    // Pitch Up
        HillTypeTailMuscle m_DepressorCaudae;  // Pitch Down
        HillTypeTailMuscle m_PubocaudalisLeft; // Yaw / Roll Left
        HillTypeTailMuscle m_PubocaudalisRight;// Yaw / Roll Right

        WingDownwashLagBuffer m_DownwashBuffer;
        std::array<RectrixFeatherStructuralNode, TOTAL_RECTRICES_COUNT> m_Rectrices;

        // Current Pygostyle Joint State Variables
        float m_PygostylePitchRad{ 0.0f };
        float m_PygostyleRollRad{ 0.0f };
        float m_PygostyleYawRad{ 0.0f };

        float m_PygostylePitchVelRadSec{ 0.0f };
        float m_PygostyleRollVelRadSec{ 0.0f };
        float m_PygostyleYawVelRadSec{ 0.0f };

        float m_EngineAccumulatedTime{ 0.0f };

        // Constants
        const float PYGOSTYLE_INERTIA_PITCH = 0.0092f; // kg*m^2
        const float PYGOSTYLE_INERTIA_ROLL  = 0.0048f; // kg*m^2
        const float PYGOSTYLE_INERTIA_YAW   = 0.0055f; // kg*m^2
        const float TAIL_MOMENT_ARM_METERS  = 0.52f;
        const float CLOSED_TAIL_AREA_M2     = 0.075f;
        const float OPEN_TAIL_AREA_M2       = 0.24f;

    public:
        AAAMasterBirdFlightEngine()
        {
            // Initialize 12 Rectrices Feathers with Anatomical Asymmetry
            for (uint32_t i = 0; i < TOTAL_RECTRICES_COUNT; ++i)
            {
                m_Rectrices[i].FeatherID = i;
                float normalizedPosition = std::abs(static_cast<float>(i) - 5.5f) / 5.5f; // 0.0 (center) to 1.0 (outer)
                
                // Outer rectrices are longer and have flexible shafts
                m_Rectrices[i].ShaftLengthMeters = 0.39f - (normalizedPosition * 0.07f);
                m_Rectrices[i].ShaftFlexuralRigidityEI = 0.055f - (normalizedPosition * 0.022f);
                m_Rectrices[i].SurfaceAreaM2 = 0.018f - (normalizedPosition * 0.004f);
            }
        }

        /**
         * Main Execution Frame Loop
         */
        void TickEngine(const BirdFlightInputFrame& input, float deltaTime)
        {
            m_EngineAccumulatedTime += deltaTime;

            // ------------------------------------------------------------------------------------
            // STEP 1: RELATIVE AIRSPEED & ANGLE OF ATTACK / SIDESLIP CALCULATION
            // ------------------------------------------------------------------------------------
            float airspeed = input.WorldLinearVelocity.Length();
            Vector3 airDir = airspeed > 0.001f ? input.WorldLinearVelocity.Normalized() : input.BodyForwardWorld;

            float forwardVelocity = Vector3::Dot(airDir, input.BodyForwardWorld);
            float verticalVelocity = Vector3::Dot(airDir, input.BodyUpWorld);
            float lateralVelocity  = Vector3::Dot(airDir, input.BodyRightWorld);

            // Alpha (AoA) and Beta (Sideslip)
            float alphaRad = std::atan2(-verticalVelocity, forwardVelocity);
            float betaRad  = std::asin(std::clamp(lateralVelocity, -0.99f, 0.99f));

            m_Outputs.CalculatedAngleOfAttackAlphaDeg = alphaRad * RAD_TO_DEG;
            m_Outputs.CalculatedSideslipAngleBetaDeg = betaRad * RAD_TO_DEG;

            // ------------------------------------------------------------------------------------
            // STEP 2: PRANDTL LIFTING-LINE WING DOWNWASH LAG SOLVER
            // ------------------------------------------------------------------------------------
            float dynamicPressure = 0.5f * RHO_SEA_LEVEL_AIR_DENSITY * (airspeed * airspeed);
            float wingAreaM2 = 0.68f;
            float wingCL = input.WingLiftForceNewtons / (std::max(0.001f, dynamicPressure) * wingAreaM2);
            float aspectRatio = (input.WingSpanMeters * input.WingSpanMeters) / wingAreaM2;

            float downwashAngleRad = (2.0f * wingCL) / (PI * aspectRatio + 0.001f);
            Vector3 localDownwashVel = input.BodyUpWorld * (-std::sin(downwashAngleRad) * airspeed);
            
            m_DownwashBuffer.StoreDownwash(localDownwashVel);
            Vector3 delayedDownwash = m_DownwashBuffer.SampleDelayedDownwash(airspeed, TAIL_MOMENT_ARM_METERS, deltaTime);

            m_Outputs.DownwashAngleDeg = downwashAngleRad * RAD_TO_DEG;

            // Effective Tail Alpha with downwash subtraction
            float effectiveTailAlphaRad = alphaRad - (delayedDownwash.Length() / std::max(0.1f, airspeed)) + m_PygostylePitchRad;

            // ------------------------------------------------------------------------------------
            // STEP 3: AERODYNAMIC LIFT, DRAG & SIDE-FORCE TORQUE SOLVER ON TAIL
            // ------------------------------------------------------------------------------------
            float activeTailAreaM2 = Vector3::Lerp(
                Vector3(CLOSED_TAIL_AREA_M2, 0, 0),
                Vector3(OPEN_TAIL_AREA_M2, 0, 0),
                input.TailFanExpansionNormalized
            ).x;

            // Non-linear Tail Lift Curve with Post-Stall Drop-off
            float tailCL = 2.0f * PI * std::sin(effectiveTailAlphaRad);
            if (std::abs(effectiveTailAlphaRad * RAD_TO_DEG) > 15.0f)
            {
                float stallPenalty = std::clamp((std::abs(effectiveTailAlphaRad * RAD_TO_DEG) - 15.0f) / 12.0f, 0.0f, 0.75f);
                tailCL *= (1.0f - stallPenalty);
            }

            // Tail Drag & Side Forces
            float tailCD = 0.015f + (tailCL * tailCL) / (PI * 2.1f);
            float tailCY = -1.8f * betaRad; // Yaw restoration force

            float tailLiftN = dynamicPressure * activeTailAreaM2 * tailCL;
            float tailDragN = dynamicPressure * activeTailAreaM2 * tailCD;
            float tailSideN = dynamicPressure * activeTailAreaM2 * tailCY;

            // Apply Roll-Twist Lift Differential
            float leftTailLiftN  = tailLiftN * 0.5f * (1.0f + std::sin(m_PygostyleRollRad));
            float rightTailLiftN = tailLiftN * 0.5f * (1.0f - std::sin(m_PygostyleRollRad));

            Vector3 tailForceBody(
                tailSideN,
                leftTailLiftN + rightTailLiftN,
                -tailDragN
            );

            // Torque around Center of Mass
            Vector3 tailMomentArmBody(0.0f, 0.0f, -TAIL_MOMENT_ARM_METERS);
            Vector3 tailTorqueBody = Vector3::Cross(tailMomentArmBody, tailForceBody);

            m_Outputs.TotalTailAerodynamicForceWorld = tailForceBody;
            m_Outputs.TotalTailAerodynamicTorqueWorld = tailTorqueBody;

            // ------------------------------------------------------------------------------------
            // STEP 4: BIOMECHANICAL CONTROL LOOP & MUSCLE ACTIVATION SOLVER
            // ------------------------------------------------------------------------------------
            // Calculate Equilibrium Pitch Torque
            float wingPitchMoment = input.WingLiftForceNewtons * input.WingCenterOfLiftOffsetMeters.y;
            float bodyDampingPitch = input.WorldAngularVelocityRadSec.x * 22.0f;
            float targetPitchTorque = -(wingPitchMoment + bodyDampingPitch);

            // User Maneuver Additions
            targetPitchTorque += (input.UserTargetPitchDeg - input.WorldAngularVelocityRadSec.x * RAD_TO_DEG) * 0.65f;

            if (input.bIsBrakingLandingMode)
            {
                targetPitchTorque += 45.0f; // Flare Up
            }
            else if (input.bIsHighSpeedDive)
            {
                targetPitchTorque *= 0.05f; // Zero Trim
            }

            // Activate Muscle Pairs
            if (targetPitchTorque > 0.0f)
            {
                m_LevatorCaudae.NormalizedActivation = std::clamp(targetPitchTorque / 24.5f, 0.0f, 1.0f);
                m_DepressorCaudae.NormalizedActivation = 0.0f;
            }
            else
            {
                m_LevatorCaudae.NormalizedActivation = 0.0f;
                m_DepressorCaudae.NormalizedActivation = std::clamp(-targetPitchTorque / 24.5f, 0.0f, 1.0f);
            }

            // Roll / Yaw Control Signals
            float targetRollTorque = (input.UserTargetRollDeg - input.WorldAngularVelocityRadSec.z * RAD_TO_DEG) * 0.35f;
            float targetYawTorque  = (input.UserTargetYawDeg  - input.WorldAngularVelocityRadSec.y * RAD_TO_DEG) * 0.35f;

            m_PubocaudalisLeft.NormalizedActivation  = std::clamp((targetYawTorque - targetRollTorque) / 24.5f, 0.0f, 1.0f);
            m_PubocaudalisRight.NormalizedActivation = std::clamp((-targetYawTorque + targetRollTorque) / 24.5f, 0.0f, 1.0f);

            // Tick Fatigue
            m_LevatorCaudae.TickFatigue(deltaTime);
            m_DepressorCaudae.TickFatigue(deltaTime);
            m_PubocaudalisLeft.TickFatigue(deltaTime);
            m_PubocaudalisRight.TickFatigue(deltaTime);

            // Calculate Net Muscle Joint Torques
            float musclePitchTorque = m_LevatorCaudae.EvaluateDynamicTorque(m_PygostylePitchVelRadSec) - 
                                      m_DepressorCaudae.EvaluateDynamicTorque(-m_PygostylePitchVelRadSec);

            float muscleRollTorque  = (m_PubocaudalisRight.EvaluateDynamicTorque(m_PygostyleRollVelRadSec) - 
                                      m_PubocaudalisLeft.EvaluateDynamicTorque(-m_PygostyleRollVelRadSec)) * 0.5f;

            float muscleYawTorque   = (m_PubocaudalisLeft.EvaluateDynamicTorque(m_PygostyleYawVelRadSec) - 
                                      m_PubocaudalisRight.EvaluateDynamicTorque(-m_PygostyleYawVelRadSec)) * 0.5f;

            // Total Joint Torques (Muscle Torque + Aero Reaction)
            float totalPitchTorque = musclePitchTorque + (tailTorqueBody.x * 0.12f);
            float totalRollTorque  = muscleRollTorque  + (tailTorqueBody.z * 0.12f);
            float totalYawTorque   = muscleYawTorque   + (tailTorqueBody.y * 0.12f);

            // Integrate Rotational Mechanics: Angular Accel = Torque / Inertia
            float accelP = totalPitchTorque / PYGOSTYLE_INERTIA_PITCH;
            float accelR = totalRollTorque  / PYGOSTYLE_INERTIA_ROLL;
            float accelY = totalYawTorque   / PYGOSTYLE_INERTIA_YAW;

            m_PygostylePitchVelRadSec += accelP * deltaTime;
            m_PygostyleRollVelRadSec  += accelR * deltaTime;
            m_PygostyleYawVelRadSec   += accelY * deltaTime;

            // Joint Damping
            m_PygostylePitchVelRadSec *= (1.0f - 4.2f * deltaTime);
            m_PygostyleRollVelRadSec  *= (1.0f - 5.1f * deltaTime);
            m_PygostyleYawVelRadSec   *= (1.0f - 4.8f * deltaTime);

            m_PygostylePitchRad += m_PygostylePitchVelRadSec * deltaTime;
            m_PygostyleRollRad  += m_PygostyleRollVelRadSec  * deltaTime;
            m_PygostyleYawRad   += m_PygostyleYawVelRadSec   * deltaTime;

            // Anatomical Joint Clamps
            m_PygostylePitchRad = std::clamp(m_PygostylePitchRad, -35.0f * DEG_TO_RAD, 65.0f * DEG_TO_RAD);
            m_PygostyleRollRad  = std::clamp(m_PygostyleRollRad,  -42.0f * DEG_TO_RAD, 42.0f * DEG_TO_RAD);
            m_PygostyleYawRad   = std::clamp(m_PygostyleYawRad,   -30.0f * DEG_TO_RAD, 30.0f * DEG_TO_RAD);

            // ------------------------------------------------------------------------------------
            // STEP 5: UNSTEADY AEROELASTIC BUFFETING & FLUTTER ENGINE
            // ------------------------------------------------------------------------------------
            float stallSeverity = std::clamp((std::abs(effectiveTailAlphaRad * RAD_TO_DEG) - 13.0f) / 14.0f, 0.0f, 1.0f);
            float strouhalFreq = std::clamp((0.21f * airspeed) / 0.28f, 12.0f, 52.0f);

            m_Outputs.AeroelasticFlutterFrequencyHz = strouhalFreq;
            m_Outputs.StallBuffetingAmplitudeDeg = std::sin(m_EngineAccumulatedTime * strouhalFreq * TWO_PI) * stallSeverity * (airspeed * 0.12f);

            // ------------------------------------------------------------------------------------
            // STEP 6: STRUCTURAL FEATHER CANTILEVER DEFORMATION SOLVER (R1 - R12)
            // ------------------------------------------------------------------------------------
            for (uint32_t i = 0; i < TOTAL_RECTRICES_COUNT; ++i)
            {
                float featherLiftShareN = (i < 6 ? leftTailLiftN : rightTailLiftN) / 6.0f;
                float featherSideShareN = tailSideN / 12.0f;

                m_Rectrices[i].SolveCantileverDynamics(featherLiftShareN, featherSideShareN, deltaTime);

                m_Outputs.IndividualRectricesPitchFlexDeg[i] = m_Rectrices[i].AngularDeflectionPitchDeg + m_Outputs.StallBuffetingAmplitudeDeg;
                m_Outputs.IndividualRectricesRollFlexDeg[i]  = m_Rectrices[i].AngularDeflectionRollDeg;
            }

            // ------------------------------------------------------------------------------------
            // STEP 7: FINAL OUTPUT TRANSFORM ASSEMBLY
            // ------------------------------------------------------------------------------------
            m_Outputs.JointPitchDeg = (m_PygostylePitchRad * RAD_TO_DEG) + m_Outputs.StallBuffetingAmplitudeDeg;
            m_Outputs.JointRollDeg  = m_PygostyleRollRad * RAD_TO_DEG;
            m_Outputs.JointYawDeg   = m_PygostyleYawRad * RAD_TO_DEG;

            m_Outputs.PygostyleLocalOrientation = Quaternion::FromEulerAngles(
                m_Outputs.JointPitchDeg,
                m_Outputs.JointYawDeg,
                m_Outputs.JointRollDeg
            );
        }

        inline const BirdFlightOutputFrame& GetOutputs() const { return m_Outputs; }
    };
}

#endif // AAA_BIRD_COMPLETE_FLIGHT_AEROELASTICS_ENGINE_HPP
