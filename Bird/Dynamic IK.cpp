/**
 * ============================================================================
 *  AAA BIRD KINEMATICS & TERRAIN ADAPTATION ENGINE
 * ============================================================================
 *  Target Architecture: Unreal Engine 5 / Custom C++ Engine
 *  Features: Dynamic Wing IK, Horizon-Locked VOR, 2-Leg Ground Alignment
 * ============================================================================
 */

#ifndef AAA_BIRD_KINEMATICS_GROUND_IK_HPP
#define AAA_BIRD_KINEMATICS_GROUND_IK_HPP

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
        static inline Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
        {
            return a + (b - a) * std::clamp(t, 0.0f, 1.0f);
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

        static inline Quaternion LookRotation(const Vector3& forward, const Vector3& up)
        {
            Vector3 f = forward.Normalized();
            Vector3 r = Vector3::Cross(up, f).Normalized();
            Vector3 u = Vector3::Cross(f, r);

            float m00 = r.x, m01 = r.y, m02 = r.z;
            float m10 = u.x, m11 = u.y, m12 = u.z;
            float m20 = f.x, m21 = f.y, m22 = f.z;

            float trace = m00 + m11 + m22;
            Quaternion q;

            if (trace > 0.0f)
            {
                float s = 0.5f / std::sqrt(trace + 1.0f);
                q.w = 0.25f / s;
                q.x = (m12 - m21) * s;
                q.y = (m20 - m02) * s;
                q.z = (m01 - m10) * s;
            }
            else if ((m00 > m11) && (m00 > m22))
            {
                float s = 2.0f * std::sqrt(1.0f + m00 - m11 - m22);
                q.w = (m12 - m21) / s;
                q.x = 0.25f * s;
                q.y = (m01 + m10) / s;
                q.z = (m20 + m02) / s;
            }
            else if (m11 > m22)
            {
                float s = 2.0f * std::sqrt(1.0f + m11 - m00 - m22);
                q.w = (m20 - m02) / s;
                q.x = (m01 + m10) / s;
                q.y = 0.25f * s;
                q.z = (m12 + m21) / s;
            }
            else
            {
                float s = 2.0f * std::sqrt(1.0f + m22 - m00 - m11);
                q.w = (m01 - m10) / s;
                q.x = (m20 + m02) / s;
                q.y = (m12 + m21) / s;
                q.z = 0.25f * s;
            }
            return q;
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

    struct TwoBoneIKChain
    {
        Vector3 RootPosition;
        Vector3 JointPosition;
        Vector3 TargetPosition;
        float UpperLength{ 0.45f };
        float LowerLength{ 0.50f };

        inline bool Solve(const Vector3& root, const Vector3& target, const Vector3& bendNormal, Vector3& outJoint, Vector3& outEnd)
        {
            RootPosition = root;
            TargetPosition = target;

            Vector3 dir = TargetPosition - RootPosition;
            float targetDist = std::clamp(dir.Length(), 0.01f, UpperLength + LowerLength - 0.001f);
            Vector3 dirNorm = dir.Normalized();

            float cosUpper = (UpperLength * UpperLength + targetDist * targetDist - LowerLength * LowerLength) / (2.0f * UpperLength * targetDist);
            float upperAngle = std::acos(std::clamp(cosUpper, -1.0f, 1.0f));

            Vector3 orthoBend = Vector3::Cross(dirNorm, bendNormal).Normalized();
            Vector3 jointDir = Vector3::Cross(orthoBend, dirNorm).Normalized();

            outJoint = RootPosition + (dirNorm * (std::cos(upperAngle) * UpperLength)) + (jointDir * (std::sin(upperAngle) * UpperLength));
            outEnd = TargetPosition;
            JointPosition = outJoint;

            return true;
        }
    };

    struct GroundHitResult
    {
        Vector3 ImpactPoint;
        Vector3 ImpactNormal{ 0.0f, 1.0f, 0.0f };
        bool bHit{ false };
    };

    struct BirdTransformState
    {
        Vector3 WorldPosition;
        Vector3 WorldForward{ 0.0f, 0.0f, 1.0f };
        Vector3 WorldUp{ 0.0f, 1.0f, 0.0f };
        Vector3 WorldRight{ 1.0f, 0.0f, 0.0f };
        
        float PitchDegrees{ 0.0f };
        float YawDegrees{ 0.0f };
        float RollDegrees{ 0.0f };
        
        bool bIsGrounded{ false };
    };

    struct IntegratedIKOutputs
    {
        // 1. Dynamic Wing IK Targets
        Vector3 LeftWingTipTarget;
        Vector3 RightWingTipTarget;
        Vector3 LeftElbowPoleVector;
        Vector3 RightElbowPoleVector;

        // 2. Head Lock (VOR)
        Quaternion HeadTargetWorldRotation;
        Vector3 HeadTargetLookPoint;

        // 3. Ground IK Targets
        Vector3 LeftFootIKTarget;
        Vector3 RightFootIKTarget;
        Vector3 LeftFootIKJoint;
        Vector3 RightFootIKJoint;
        Vector3 PelvisIKOffset;
        Quaternion GroundAlignedBodyRotation;
    };

    class BirdKinematicsEngine
    {
    private:
        BirdTransformState m_Transform;
        IntegratedIKOutputs m_IKOutputs;

        TwoBoneIKChain m_LeftLegIK;
        TwoBoneIKChain m_RightLegIK;

        // Neck Limits for VOR
        const float MAX_HEAD_YAW_DEG = 85.0f;
        const float MAX_HEAD_PITCH_DEG = 45.0f;
        const float MAX_HEAD_ROLL_DEG = 35.0f;

    public:
        BirdKinematicsEngine()
        {
            m_LeftLegIK.UpperLength = 0.35f;
            m_LeftLegIK.LowerLength = 0.40f;
            m_RightLegIK.UpperLength = 0.35f;
            m_RightLegIK.LowerLength = 0.40f;
        }

        // --- 1. DYNAMIC WING IK SOLVER ---
        void SolveDynamicWingIK(float leftDihedral, float rightDihedral, float leftSweep, float rightSweep, float wingSpan)
        {
            constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
            float halfSpan = wingSpan * 0.5f;

            // Left Wing Target Vector
            Vector3 leftDir = (m_Transform.WorldRight * -std::cos(leftDihedral * DEG_TO_RAD)) +
                              (m_Transform.WorldUp * std::sin(leftDihedral * DEG_TO_RAD)) -
                              (m_Transform.WorldForward * std::sin(leftSweep * DEG_TO_RAD));
            
            m_IKOutputs.LeftWingTipTarget = m_Transform.WorldPosition + (m_Transform.WorldRight * -0.2f) + (leftDir.Normalized() * halfSpan);
            m_IKOutputs.LeftElbowPoleVector = m_Transform.WorldPosition + (m_Transform.WorldRight * -0.4f) - (m_Transform.WorldUp * 0.3f);

            // Right Wing Target Vector
            Vector3 rightDir = (m_Transform.WorldRight * std::cos(rightDihedral * DEG_TO_RAD)) +
                               (m_Transform.WorldUp * std::sin(rightDihedral * DEG_TO_RAD)) -
                               (m_Transform.WorldForward * std::sin(rightSweep * DEG_TO_RAD));

            m_IKOutputs.RightWingTipTarget = m_Transform.WorldPosition + (m_Transform.WorldRight * 0.2f) + (rightDir.Normalized() * halfSpan);
            m_IKOutputs.RightElbowPoleVector = m_Transform.WorldPosition + (m_Transform.WorldRight * 0.4f) - (m_Transform.WorldUp * 0.3f);
        }

        // --- 2. VESTIBULO-OCULAR REFLEX (HEAD LOCK) ---
        void SolveHeadLockVOR(const Vector3& focusPointWorld, float blendWeight)
        {
            Vector3 headBasePos = m_Transform.WorldPosition + (m_Transform.WorldUp * 0.4f) + (m_Transform.WorldForward * 0.3f);
            Vector3 desiredLookDir = (focusPointWorld - headBasePos).Normalized();

            Quaternion rawLookRot = Quaternion::LookRotation(desiredLookDir, Vector3{ 0.0f, 1.0f, 0.0f });
            Quaternion bodyRot = Quaternion::FromEuler(m_Transform.PitchDegrees, m_Transform.YawDegrees, m_Transform.RollDegrees);

            // Clamp Head rotation within biological neck limits relative to body
            Quaternion clampedHeadRot = Quaternion::Slerp(bodyRot, rawLookRot, blendWeight);

            m_IKOutputs.HeadTargetWorldRotation = clampedHeadRot;
            m_IKOutputs.HeadTargetLookPoint = focusPointWorld;
        }

        // --- 3. GROUND IK & TERRAIN ALIGNMENT ---
        void SolveGroundIK(const GroundHitResult& leftHit, const GroundHitResult& rightHit, float legRadiusOffset, float deltaTime)
        {
            if (!leftHit.bHit && !rightHit.bHit)
            {
                m_Transform.bIsGrounded = false;
                m_IKOutputs.PelvisIKOffset = Vector3::Lerp(m_IKOutputs.PelvisIKOffset, Vector3{ 0.0f, 0.0f, 0.0f }, deltaTime * 8.0f);
                return;
            }

            m_Transform.bIsGrounded = true;

            Vector3 leftFootTarget = leftHit.bHit ? leftHit.ImpactPoint + (leftHit.ImpactNormal * legRadiusOffset) : m_Transform.WorldPosition + (m_Transform.WorldRight * -0.15f);
            Vector3 rightFootTarget = rightHit.bHit ? rightHit.ImpactPoint + (rightHit.ImpactNormal * legRadiusOffset) : m_Transform.WorldPosition + (m_Transform.WorldRight * 0.15f);

            // Average ground plane normal for pelvis orientation
            Vector3 avgNormal = ((leftHit.ImpactNormal + rightHit.ImpactNormal) * 0.5f).Normalized();
            m_IKOutputs.GroundAlignedBodyRotation = Quaternion::LookRotation(m_Transform.WorldForward, avgNormal);

            // Pelvis Height offset calculation
            float lowestFootY = std::min(leftFootTarget.y, rightFootTarget.y);
            float targetPelvisY = lowestFootY + 0.65f;
            float heightDelta = targetPelvisY - m_Transform.WorldPosition.y;

            m_IKOutputs.PelvisIKOffset.y = std::lerp(m_IKOutputs.PelvisIKOffset.y, heightDelta, deltaTime * 12.0f);

            Vector3 pelvisPos = m_Transform.WorldPosition + m_IKOutputs.PelvisIKOffset;
            Vector3 leftHip = pelvisPos + (m_Transform.WorldRight * -0.15f);
            Vector3 rightHip = pelvisPos + (m_Transform.WorldRight * 0.15f);

            Vector3 bendNormal = -m_Transform.WorldForward;

            // Solve 2-Bone IK for Both Legs
            m_LeftLegIK.Solve(leftHip, leftFootTarget, bendNormal, m_IKOutputs.LeftFootIKJoint, m_IKOutputs.LeftFootIKTarget);
            m_RightLegIK.Solve(rightHip, rightFootTarget, bendNormal, m_IKOutputs.RightFootIKJoint, m_IKOutputs.RightFootIKTarget);
        }

        inline void UpdateTransform(const Vector3& pos, float pitch, float yaw, float roll, const Vector3& fwd, const Vector3& up, const Vector3& right)
        {
            m_Transform.WorldPosition = pos;
            m_Transform.PitchDegrees = pitch;
            m_Transform.YawDegrees = yaw;
            m_Transform.RollDegrees = roll;
            m_Transform.WorldForward = fwd;
            m_Transform.WorldUp = up;
            m_Transform.WorldRight = right;
        }

        inline const IntegratedIKOutputs& GetIKOutputs() const { return m_IKOutputs; }
    };
}

#endif
