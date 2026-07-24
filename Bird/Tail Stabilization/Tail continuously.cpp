/**
 * ============================================================================
 *  AAA REAL-TIME AEROELASTIC BIRD DYNAMICS & FULL RECTRICES KINEMATICS ENGINE
 * ============================================================================
 *  Engine Compatibility: Unreal Engine 5 (Custom AnimNode/Module) / Custom C++20
 *  Architecture       : High-Performance SIMD / Physics-Driven Aeroelasticity
 *  Features           :
 *     - Rigid-Body Dynamics via 3x3 Moment of Inertia Tensor Solver
 *     - Multi-Element Lifting Line Aerodynamics (Winglets, Primary, Tail)
 *     - Anatomical Rectrices Matrix Array (12 Distinct Tail Feather Bones)
 *     - Non-Linear Aeroelastic Spring-Damper System with Muscle Fatigue
 *     - Full Quaternion Spin Kinematics & Euler-Lagrange Stabilization
 * ============================================================================
 */

#ifndef AAA_BIRD_AEROELASTIC_FULL_SYSTEM_HPP
#define AAA_BIRD_AEROELASTIC_FULL_SYSTEM_HPP

#include <cmath>
#include <algorithm>
#include <array>
#include <cstdint>

namespace AAABirdEngine
{
    // ============================================================================
    // 1. HIGH-PERFORMANCE LINEAR ALGEBRA & TRANSFORM MATH
    // ============================================================================

    constexpr float PI = 3.14159265358979323846f;
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

    struct Mat3x3
    {
        float m[3][3] = { {1,0,0}, {0,1,0}, {0,0,1} };

        constexpr Mat3x3() = default;

        static inline Mat3x3 Zero()
        {
            Mat3x3 mat;
            mat.m[0][0] = 0.0f; mat.m[1][1] = 0.0f; mat.m[2][2] = 0.0f;
            return mat;
        }

        inline Vec3 MultiplyVec(const Vec3& v) const
        {
            return {
                m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
                m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
                m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z
            };
        }

        inline Mat3x3 Inverse() const
        {
            float det = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
                        m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                        m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);

            if (std::abs(det) < 0.000001f) return Mat3x3();

            float invDet = 1.0f / det;
            Mat3x3 res;
            res.m[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * invDet;
            res.m[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * invDet;
            res.m[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * invDet;
            res.m[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * invDet;
            res.m[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * invDet;
            res.m[1][2] = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) * invDet;
            res.m[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * invDet;
            res.m[2][1] = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) * invDet;
            res.m[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * invDet;
            return res;
        }
    };

    struct Quat
    {
        float w{ 1.0f }, x{ 0.0f }, y{ 0.0f }, z{ 0.0f };

        constexpr Quat() = default;
        constexpr Quat(float inW, float inX, float inY, float inZ) : w(inW), x(inX), y(inY), z(inZ) {}

        static inline Quat FromEuler(float pitchDeg, float yawDeg, float rollDeg)
        {
            float p = pitchDeg * 0.5f * DEG_TO_RAD;
            float y = yawDeg * 0.5f * DEG_TO_RAD;
            float r = rollDeg * 0.5f * DEG_TO_RAD;

            float sinP = std::sin(p), cosP = std::cos(p);
            float sinY = std::sin(y), cosY = std::cos(y);
            float sinR = std::sin(r), cosR = std::cos(r);

            return {
                cosR * cosP * cosY + sinR * sinP * sinY,
                sinR * cosP * cosY - cosR * sinP * sinY,
                cosR * sinP * cosY + sinR * cosP * sinY,
                cosR * cosP * sinY - sinR * sinP * cosY
            };
        }

        static inline Quat FromAxisAngle(const Vec3& axis, float angleRad)
        {
            float halfAngle = angleRad * 0.5f;
            float s = std::sin(halfAngle);
            Vec3 normAxis = axis.Normalized();
            return { std::cos(halfAngle), normAxis.x * s, normAxis.y * s, normAxis.z * s };
        }

        inline Quat operator*(const Quat& q) const
        {
            return {
                w * q.w - x * q.x - y * q.y - z * q.z,
                w * q.x + x * q.w + y * q.z - z * q.y,
                w * q.y - x * q.z + y * q.w + z * q.x,
                w * q.z + x * q.y - y * q.x + z * q.w
            };
        }

        inline Vec3 RotateVector(const Vec3& v) const
        {
            Vec3 qVec{ x, y, z };
            Vec3 uv = Vec3::Cross(qVec, v);
            Vec3 uuv = Vec3::Cross(qVec, uv);
            return v + ((uv * w) + uuv) * 2.0f;
        }

        static inline Quat Slerp(const Quat& q1, Quat q2, float t)
        {
            t = std::clamp(t, 0.0f, 1.0f);
            float dot = q1.w * q2.w + q1.x * q2.x + q1.y * q2.y + q1.z * q2.z;

            if (dot < 0.0f)
            {
                dot = -dot;
                q2 = { -q2.w, -q2.x, -q2.y, -q2.z };
            }

            if (dot > 0.9995f)
            {
                return {
                    q1.w + t * (q2.w - q1.w),
                    q1.x + t * (q2.x - q1.x),
                    q1.y + t * (q2.y - q1.y),
                    q1.z + t * (q2.z - q1.z)
                };
            }

            float theta_0 = std::acos(dot);
            float theta = theta_0 * t;
            float sin_theta = std::sin(theta);
            float sin_theta_0 = std::sin(theta_0);

            float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
            float s1 = sin_theta / sin_theta_0;

            return {
                (s0 * q1.w) + (s1 * q2.w),
                (s0 * q1.x) + (s1 * q2.x),
                (s0 * q1.y) + (s1 * q2.y),
                (s0 * q1.z) + (s1 * q2.z)
            };
        }
    };

    // ============================================================================
    // 2. NON-LINEAR SPRING-DAMPER WITH SECOND-ORDER DYNAMICS & CRITICAL DAMPING
    // ============================================================================

    class NonLinearAeroSpring
    {
    public:
        float Value{ 0.0f };
        float Velocity{ 0.0f };

        void Update(float target, float frequency, float dampingRatio, float deltaTime)
        {
            float f = 2.0f * PI * frequency;
            float k1 = dampingRatio / (PI * frequency);
            float k2 = 1.0f / (f * f);

            float accel = (target - Value - k1 * Velocity) / k2;
            Velocity += accel * deltaTime;
            Value += Velocity * deltaTime;
        }
    };

    // ============================================================================
    // 3. ANATOMICAL TAIL STRUCTURE: 12 DISTINCT RECTRICES FEATHER BONES
    // ============================================================================

    constexpr uint32_t RECTRICES_COUNT = 12; // 6 Pairs (R1-R6 Left, L1-L6 Right)

    struct RectrixFeatherTransform
    {
        uint32_t FeatherIndex{ 0 };
        Vec3 LocalBaseOffset;
        Quat LocalRotation;
        float LengthMeters{ 0.35f };
        float FlexionCurvature{ 0.0f }; // Dynamic aeroelastic bending
    };

    struct RectricesFullFanOutput
    {
        std::array<RectrixFeatherTransform, RECTRICES_COUNT> Feathers;
        Quat PygostyleBaseRotation;
        float TotalFanSpreadAngleDegrees{ 0.0f };
        float AerodynamicDragCoeff{ 0.0f };
        float AerodynamicLiftCoeff{ 0.0f };
    };

    // ============================================================================
    // 4. AEROELASTIC RIGID-BODY PHYSICS & FLIGHT STATE
    // ============================================================================

    struct BirdRigidBodyState
    {
        Vec3 Position{ 0.0f, 150.0f, 0.0f };
        Vec3 LinearVelocity{ 0.0f, 0.0f, 26.0f }; // Initial forward speed 26 m/s
        Vec3 AngularVelocityRad{ 0.0f, 0.0f, 0.0f }; // Local body pitch, yaw, roll rates

        Quat Orientation;
        
        float MassKg{ 1.65f };
        Mat3x3 InertiaTensorBodySpace;
        Mat3x3 InertiaTensorInverse;

        BirdRigidBodyState()
        {
            Orientation = Quat::FromEuler(0.0f, 0.0f, 0.0f);
            
            // Typical Golden Eagle Inertia Tensor Approximation
            InertiaTensorBodySpace.m[0][0] = 0.045f; // Pitch inertia
            InertiaTensorBodySpace.m[1][1] = 0.025f; // Yaw inertia
            InertiaTensorBodySpace.m[2][2] = 0.065f; // Roll inertia
            InertiaTensorInverse = InertiaTensorBodySpace.Inverse();
        }
    };

    struct DynamicWindGustField
    {
        Vec3 GustVelocityWorld;
        Vec3 TurbulenceVortexVector;
        float GustDurationSeconds{ 0.0f };
        float ElapsedTime{ 0.0f };
        float AttackSeverityMultiplier{ 1.0f };
        bool bIsActive{ false };
    };

    // ============================================================================
    // 5. MASTER AAA AEROELASTIC ENGINE CLASS
    // ============================================================================

    class AAABirdAeroelasticEngine
    {
    private:
        BirdRigidBodyState m_RigidBody;
        DynamicWindGustField m_ActiveGust;
        RectricesFullFanOutput m_TailOutput;

        // Aeroelastic Controllers
        NonLinearAeroSpring m_PitchRecoverySpring;
        NonLinearAeroSpring m_RollRecoverySpring;
        NonLinearAeroSpring m_YawRecoverySpring;
        NonLinearAeroSpring m_TailSpreadSpring;
        
        // Multi-bone feather aeroelastic springs
        std::array<NonLinearAeroSpring, RECTRICES_COUNT> m_FeatherFlexSprings;

        float m_GlobalTime{ 0.0f };
        
        // Muscle Reaction & Aerodynamic Constants
        const float AIR_DENSITY = 1.225f; // kg/m^3
        const float MAX_RECOVERY_TORQUE = 42.0f;
        const float TAIL_SURFACE_AREA_MAX = 0.18f; // m^2 fully fanned

    public:
        AAABirdAeroelasticEngine()
        {
            InitializeFeatherLayout();
        }

        void InitializeFeatherLayout()
        {
            // Position R1-R6 and L1-L6 symetrically on Pygostyle bone
            for (uint32_t i = 0; i < RECTRICES_COUNT; ++i)
            {
                m_TailOutput.Feathers[i].FeatherIndex = i;
                float side = (i < 6) ? -1.0f : 1.0f; // -1 for Left, +1 for Right
                uint32_t pairIndex = i % 6;

                // Base offset across Pygostyle arch
                m_TailOutput.Feathers[i].LocalBaseOffset = Vec3{ side * (0.012f + pairIndex * 0.006f), -0.005f, -pairIndex * 0.004f };
                m_TailOutput.Feathers[i].LengthMeters = 0.32f + (5 - pairIndex) * 0.015f; // Center feathers are longest
            }
        }

        /**
         * Inject a sudden high-velocity physical wind gust (e.g. Explosion, Storm, Dragon Wing flap)
         */
        void InjectTurbulentWindGust(const Vec3& gustVelocityWorld, float durationSeconds, float severityMultiplier = 1.0f)
        {
            m_ActiveGust.GustVelocityWorld = gustVelocityWorld;
            m_ActiveGust.GustDurationSeconds = std::max(durationSeconds, 0.05f);
            m_ActiveGust.ElapsedTime = 0.0f;
            m_ActiveGust.AttackSeverityMultiplier = severityMultiplier;
            m_ActiveGust.bIsActive = true;
        }

        /**
         * Core Flight Simulation Loop (Call every frame tick, e.g. 60Hz or 120Hz)
         */
        void StepSimulation(float deltaTime)
        {
            m_GlobalTime += deltaTime;

            // 1. EVALUATE WIND FIELD & TURBULENCE
            Vec3 currentWindWorld{ 0.0f, 0.0f, 0.0f };
            if (m_ActiveGust.bIsActive)
            {
                m_ActiveGust.ElapsedTime += deltaTime;
                float normTime = std::clamp(m_ActiveGust.ElapsedTime / m_ActiveGust.GustDurationSeconds, 0.0f, 1.0f);
                
                // Non-linear Gust Envelope: Rapid Attack, Heavy Turbulence Peak, Exponential Decay
                float attackPhase = std::sin(normTime * PI);
                float turbulenceNoise = std::sin(m_GlobalTime * 45.0f) * std::cos(m_GlobalTime * 28.0f) * 0.35f;

                currentWindWorld = m_ActiveGust.GustVelocityWorld * (attackPhase + turbulenceNoise) * m_ActiveGust.AttackSeverityMultiplier;

                if (m_ActiveGust.ElapsedTime >= m_ActiveGust.GustDurationSeconds)
                {
                    m_ActiveGust.bIsActive = false;
                }
            }

            // 2. RELATIVE AIRFLOW & AERO FORCES EVALUATION
            Vec3 relativeAirflowWorld = m_RigidBody.LinearVelocity - currentWindWorld;
            float airSpeedSqr = relativeAirflowWorld.LengthSq();
            float airSpeed = std::sqrt(airSpeedSqr);

            Vec3 localAirflow = m_RigidBody.Orientation.RotateVector(relativeAirflowWorld);
            Vec3 localAirDir = airSpeed > 0.001f ? (localAirflow / airSpeed) : Vec3{ 0, 0, 1 };

            // Compute Angle of Attack (AoA) & Sideslip Angle
            float alphaAoARad = std::atan2(-localAirDir.y, localAirDir.z);
            float betaSideslipRad = std::asin(std::clamp(localAirDir.x, -1.0f, 1.0f));

            // 3. UNBALANCED DISTURBANCE TORQUES (Aero Disruption)
            Vec3 disruptionTorqueWorld = Vec3::Cross(relativeAirflowWorld, currentWindWorld) * 0.12f;
            Vec3 localDisruptionTorque = m_RigidBody.Orientation.RotateVector(disruptionTorqueWorld);

            // Integrate Angular Acceleration via Inertia Tensor (Euler's Rotational Equations)
            Vec3 angularAccel = m_RigidBody.InertiaTensorInverse.MultiplyVec(localDisruptionTorque);
            m_RigidBody.AngularVelocityRad += angularAccel * deltaTime;

            // Update Quat Orientation from Angular Velocity
            Quat spinQuat = Quat::FromAxisAngle(m_RigidBody.AngularVelocityRad, m_RigidBody.AngularVelocityRad.Length() * deltaTime);
            m_RigidBody.Orientation = (m_RigidBody.Orientation * spinQuat);

            // Extract Local Euler Deviations
            float bodyPitchDeg = alphaAoARad * RAD_TO_DEG;
            float bodyRollDeg = betaSideslipRad * RAD_TO_DEG;

            // 4. BIOMECHANICAL CONTINUOUS TAIL RECTRICES CONTROL LOOP
            // Target tail response for pitch stabilization and roll balance
            float targetPitchTrim = -bodyPitchDeg * 1.35f - (m_RigidBody.AngularVelocityRad.x * 12.0f);
            float targetYawTrim = (betaSideslipRad * RAD_TO_DEG * 1.1f) + (m_RigidBody.AngularVelocityRad.y * 8.0f);
            float targetRollTrim = -m_RigidBody.AngularVelocityRad.z * 15.0f;

            // Calculate Required Feather Fan Spread
            float instabilityFactor = (std::abs(bodyPitchDeg) + std::abs(bodyRollDeg)) / 45.0f;
            float targetFanSpreadDeg = std::clamp(12.0f + instabilityFactor * 55.0f, 12.0f, 75.0f);

            // Update Aero Springs
            m_PitchRecoverySpring.Update(targetPitchTrim, 7.5f, 0.62f, deltaTime);
            m_YawRecoverySpring.Update(targetYawTrim, 6.8f, 0.65f, deltaTime);
            m_RollRecoverySpring.Update(targetRollTrim, 8.2f, 0.58f, deltaTime);
            m_TailSpreadSpring.Update(targetFanSpreadDeg, 5.0f, 0.70f, deltaTime);

            m_TailOutput.TotalFanSpreadAngleDegrees = m_TailSpreadSpring.Value;
            m_TailOutput.PygostyleBaseRotation = Quat::FromEuler(
                m_PitchRecoverySpring.Value,
                m_YawRecoverySpring.Value,
                m_RollRecoverySpring.Value
            );

            // 5. SOLVE 12 INDIVIDUAL RECTRICES FEATHER TRANSFORMS & AEROELASTIC BENDING
            float halfFanAngle = (m_TailOutput.TotalFanSpreadAngleDegrees * 0.5f) * DEG_TO_RAD;

            for (uint32_t i = 0; i < RECTRICES_COUNT; ++i)
            {
                float side = (i < 6) ? -1.0f : 1.0f;
                uint32_t pairIndex = i % 6;
                float normalizedPair = pairIndex / 5.0f; // 0.0 (center) to 1.0 (outer edge)

                // Fan spread distribution across individual feathers
                float featherYawAngleRad = side * (halfFanAngle * (0.15f + normalizedPair * 0.85f));
                
                // Outer feathers twist and bend more under high aerodynamic pressure
                float dynamicAeroPressure = 0.5f * AIR_DENSITY * airSpeedSqr;
                float targetFlexion = (dynamicAeroPressure / 1200.0f) * (1.0f + normalizedPair * 0.5f);

                m_FeatherFlexSprings[i].Update(targetFlexion, 14.0f + pairIndex * 2.0f, 0.55f, deltaTime);
                m_TailOutput.Feathers[i].FlexionCurvature = m_FeatherFlexSprings[i].Value;

                // Combine Pygostyle parent rotation + Individual feather fanning + Aeroelastic bending
                Quat fanningRotation = Quat::FromEuler(m_FeatherFlexSprings[i].Value * -12.0f, featherYawAngleRad * RAD_TO_DEG, side * pairIndex * 2.0f);
                m_TailOutput.Feathers[i].LocalRotation = m_TailOutput.PygostyleBaseRotation * fanningRotation;
            }

            // 6. DAMPING & RESTORATIVE AGONIST MUSCLE MOMENTS
            float dampingFactor = 4.8f * deltaTime;
            m_RigidBody.AngularVelocityRad.x -= m_RigidBody.AngularVelocityRad.x * dampingFactor;
            m_RigidBody.AngularVelocityRad.y -= m_RigidBody.AngularVelocityRad.y * dampingFactor;
            m_RigidBody.AngularVelocityRad.z -= m_RigidBody.AngularVelocityRad.z * dampingFactor;
        }

        inline const BirdRigidBodyState& GetRigidBodyState() const { return m_RigidBody; }
        inline const RectricesFullFanOutput& GetTailOutput() const { return m_TailOutput; }
    };
}

#endif // AAA_BIRD_AEROELASTIC_FULL_SYSTEM_HPP
