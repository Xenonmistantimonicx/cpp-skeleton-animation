/**
 * ============================================================================
 *  AAA BIRD DYNAMIC GUST RESPONSE & BALANCE RECOVERY SYSTEM
 * ============================================================================
 *  Target Architecture: Unreal Engine 5 / Custom C++ Game Engine
 *  Features: Aeroelastic Physics, Asymmetric IK Dihedral, VOR Head Stabilizer
 * ============================================================================
 */

#ifndef AAA_BIRD_GUST_RECOVERY_SYSTEM_HPP
#define AAA_BIRD_GUST_RECOVERY_SYSTEM_HPP

#include <cmath>
#include <algorithm>

namespace AAABirdSystem
{
    struct Vector3
    {
        float x{ 0.0f }, y{ 0.0f }, z{ 0.0f };

        inline Vector3 operator+(const Vector3& o) const { return { x + o.x, y + o.y, z + o.z }; }
        inline Vector3 operator-(const Vector3& o) const { return { x - o.x, y - o.y, z - o.z }; }
        inline Vector3 operator*(float s) const { return { x * s, y * s, z * s }; }
        inline Vector3& operator+=(const Vector3& o) { x += o.x; y += o.y; z += o.z; return *this; }
        
        inline float LengthSquared() const { return x * x + y * y + z * z; }
        inline float Length() const { return std::sqrt(LengthSquared()); }
        
        inline Vector3 Normalized() const
        {
            float len = Length();
            return len > 0.00001f ? Vector3{ x / len, y / len, z / len } : Vector3{ 0.0f, 0.0f, 0.0f };
        }

        static inline float Dot(const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
        static inline Vector3 Cross(const Vector3& a, const Vector3& b)
        {
            return {
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            };
        }
    };

    struct Quaternion
    {
        float w{ 1.0f }, x{ 0.0f }, y{ 0.0f }, z{ 0.0f };

        static inline Quaternion FromEuler(float pitchDeg, float yawDeg, float rollDeg)
        {
            constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
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

        static inline Quaternion Slerp(const Quaternion& q1, Quaternion q2, float t)
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

    struct SecondOrderSpringDamper
    {
        float Value{ 0.0f };
        float Velocity{ 0.0f };

        inline void Update(float target, float frequency, float dampingRatio, float deltaTime)
        {
            constexpr float PI = 3.14159265358979323846f;
            float f = 2.0f * PI * frequency;
            float k1 = dampingRatio / (PI * frequency);
            float k2 = 1.0f / (f * f);

            float accel = (target - Value - k1 * Velocity) / k2;
            Velocity += accel * deltaTime;
            Value += Velocity * deltaTime;
        }
    };

    struct FlightRigState
    {
        Vector3 Position{ 0.0f, 120.0f, 0.0f };
        Vector3 ForwardVector{ 0.0f, 0.0f, 1.0f };
        Vector3 UpVector{ 0.0f, 1.0f, 0.0f };
        Vector3 RightVector{ 1.0f, 0.0f, 0.0f };
        
        Vector3 LinearVelocity{ 0.0f, 0.0f, 32.0f };
        Vector3 AngularVelocity{ 0.0f, 0.0f, 0.0f };

        float PitchDegrees{ 0.0f };
        float RollDegrees{ 0.0f };
        float YawDegrees{ 0.0f };
    };

    struct AnimationIKOutputs
    {
        float LeftWingDihedralAngle{ 0.0f };
        float RightWingDihedralAngle{ 0.0f };
        float LeftWingSweepOffset{ 0.0f };
        float RightWingSweepOffset{ 0.0f };
        
        float TailElevatorPitch{ 0.0f };
        float TailRudderYaw{ 0.0f };
        float TailSpreadMorphWeight{ 0.0f };

        Quaternion HeadWorldRotation;
        Quaternion SpineOffsetRotation;
    };

    struct WindGustImpulse
    {
        Vector3 DirectionalForce;
        float PeakIntensity{ 1.0f };
        float TotalDuration{ 0.5f };
        float ElapsedTime{ 0.0f };
        bool bIsActive{ false };
    };

    class BirdGustRecoveryController
    {
    private:
        FlightRigState m_RigState;
        AnimationIKOutputs m_IKOutputs;
        WindGustImpulse m_ActiveGust;

        SecondOrderSpringDamper m_PitchSpring;
        SecondOrderSpringDamper m_RollSpring;
        SecondOrderSpringDamper m_LeftWingSpring;
        SecondOrderSpringDamper m_RightWingSpring;
        SecondOrderSpringDamper m_TailPitchSpring;
        SecondOrderSpringDamper m_TailYawSpring;

        const float BIRD_MASS = 1.45f;
        const float WINGSPAN_METERS = 2.1f;
        const float RECOVERY_TORQUE_STRENGTH = 18.5f;

    public:
        BirdGustRecoveryController()
        {
            m_IKOutputs.HeadWorldRotation = Quaternion::FromEuler(0.0f, 0.0f, 0.0f);
            m_IKOutputs.SpineOffsetRotation = Quaternion::FromEuler(0.0f, 0.0f, 0.0f);
        }

        void TriggerSuddenGust(const Vector3& gustVector, float durationSeconds)
        {
            m_ActiveGust.DirectionalForce = gustVector;
            m_ActiveGust.TotalDuration = (durationSeconds > 0.01f) ? durationSeconds : 0.01f;
            m_ActiveGust.ElapsedTime = 0.0f;
            m_ActiveGust.PeakIntensity = gustVector.Length();
            m_ActiveGust.bIsActive = true;
        }

        void TickSimulation(float deltaTime)
        {
            Vector3 gustVelocity{ 0.0f, 0.0f, 0.0f };

            if (m_ActiveGust.bIsActive)
            {
                m_ActiveGust.ElapsedTime += deltaTime;
                float progress = std::clamp(m_ActiveGust.ElapsedTime / m_ActiveGust.TotalDuration, 0.0f, 1.0f);
                
                constexpr float PI = 3.14159265358979323846f;
                float envelope = std::sin(progress * PI) * (1.0f - (progress * 0.4f));
                gustVelocity = m_ActiveGust.DirectionalForce * envelope;

                if (m_ActiveGust.ElapsedTime >= m_ActiveGust.TotalDuration)
                {
                    m_ActiveGust.bIsActive = false;
                }
            }

            Vector3 relativeAirVelocity = m_RigState.LinearVelocity - gustVelocity;
            
            float pitchTorque = (gustVelocity.y * 2.2f) - (gustVelocity.z * 0.8f);
            float rollTorque  = (gustVelocity.x * 3.4f);
            float yawTorque   = (gustVelocity.x * 0.9f);

            m_RigState.AngularVelocity.x += (pitchTorque / BIRD_MASS) * deltaTime;
            m_RigState.AngularVelocity.z += (rollTorque / BIRD_MASS) * deltaTime;
            m_RigState.AngularVelocity.y += (yawTorque / BIRD_MASS) * deltaTime;

            constexpr float RAD_TO_DEG = 180.0f / 3.14159265358979323846f;
            m_RigState.PitchDegrees += m_RigState.AngularVelocity.x * RAD_TO_DEG * deltaTime;
            m_RigState.RollDegrees  += m_RigState.AngularVelocity.z * RAD_TO_DEG * deltaTime;
            m_RigState.YawDegrees   += m_RigState.AngularVelocity.y * RAD_TO_DEG * deltaTime;

            float targetPitchRecovery = -m_RigState.PitchDegrees * 1.1f;
            float targetRollRecovery  = -m_RigState.RollDegrees * 1.6f;
            float targetYawRecovery   = -m_RigState.YawDegrees * 0.8f;

            float targetLeftWingDihedral  = std::clamp(targetRollRecovery * 1.8f - targetPitchRecovery * 0.4f, -45.0f, 55.0f);
            float targetRightWingDihedral = std::clamp(-targetRollRecovery * 1.8f - targetPitchRecovery * 0.4f, -45.0f, 55.0f);
            float targetTailPitch         = std::clamp(-m_RigState.PitchDegrees * 1.5f, -45.0f, 45.0f);
            float targetTailYaw           = std::clamp(targetYawRecovery * 1.2f, -30.0f, 30.0f);

            m_LeftWingSpring.Update(targetLeftWingDihedral, 5.8f, 0.60f, deltaTime);
            m_RightWingSpring.Update(targetRightWingDihedral, 5.8f, 0.60f, deltaTime);
            m_TailPitchSpring.Update(targetTailPitch, 4.2f, 0.65f, deltaTime);
            m_TailYawSpring.Update(targetTailYaw, 4.2f, 0.65f, deltaTime);

            m_IKOutputs.LeftWingDihedralAngle  = m_LeftWingSpring.Value;
            m_IKOutputs.RightWingDihedralAngle = m_RightWingSpring.Value;
            m_IKOutputs.TailElevatorPitch     = m_TailPitchSpring.Value;
            m_IKOutputs.TailRudderYaw         = m_TailYawSpring.Value;

            float totalInstability = std::abs(m_RigState.PitchDegrees) + std::abs(m_RigState.RollDegrees);
            m_IKOutputs.TailSpreadMorphWeight = std::clamp(totalInstability / 60.0f, 0.0f, 1.0f);

            m_IKOutputs.LeftWingSweepOffset  = std::clamp(m_RigState.AngularVelocity.z * 1.2f, -15.0f, 20.0f);
            m_IKOutputs.RightWingSweepOffset = std::clamp(-m_RigState.AngularVelocity.z * 1.2f, -15.0f, 20.0f);

            float dragDampingFactor = 5.2f * deltaTime;
            m_RigState.AngularVelocity.x -= m_RigState.AngularVelocity.x * dragDampingFactor;
            m_RigState.AngularVelocity.y -= m_RigState.AngularVelocity.y * dragDampingFactor;
            m_RigState.AngularVelocity.z -= m_RigState.AngularVelocity.z * dragDampingFactor;

            float recoveryRate = RECOVERY_TORQUE_STRENGTH * deltaTime;
            m_RigState.PitchDegrees = std::lerp(m_RigState.PitchDegrees, 0.0f, recoveryRate);
            m_RigState.RollDegrees  = std::lerp(m_RigState.RollDegrees, 0.0f, recoveryRate);
            m_RigState.YawDegrees   = std::lerp(m_RigState.YawDegrees, 0.0f, recoveryRate);

            m_IKOutputs.HeadWorldRotation = Quaternion::FromEuler(0.0f, m_RigState.YawDegrees, 0.0f);
            m_IKOutputs.SpineOffsetRotation = Quaternion::FromEuler(m_RigState.PitchDegrees * 0.4f, 0.0f, m_RigState.RollDegrees * 0.4f);
        }

        inline const FlightRigState& GetRigState() const { return m_RigState; }
        inline const AnimationIKOutputs& GetIKOutputs() const { return m_IKOutputs; }
    };
}

#endif
