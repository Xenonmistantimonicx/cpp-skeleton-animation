/**
 * ============================================================================================
 *  AAA HIGH-PRECISION WING-WALL IMPACT, PASSIVE FOLDING & TRAJECTORY ADJUSTMENT ENGINE
 * ============================================================================================
 *  Pipeline Phase : Rigid-Body Wing Kinematics -> Multi-Node Wall Collision Solver 
 *                  -> Biomechanical Tendon/Spring-Damper Passivity -> Asymmetric Aero Dynamics 
 *                  -> Continuous Closed-Loop Flight Trajectory Adaptation
 *  Language Std   : C++20 (Zero Heap Allocations / SIMD Aligned / Deterministic / Thread-Safe)
 * ============================================================================================
 */

#ifndef AAA_BIRD_WING_COLLISION_FOLD_ADJUST_ENGINE_HPP
#define AAA_BIRD_WING_COLLISION_FOLD_ADJUST_ENGINE_HPP

#include <cmath>
#include <algorithm>
#include <array>
#include <cstdint>

namespace AAABirdEngine
{
    // ============================================================================================
    // 1. SIMD-ALIGNED MATH & KINEMATIC TRANSFORM PRIMITIVES
    // ============================================================================================

    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 6.28318530717958647692f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;
    constexpr float RHO_AIR_DENSITY = 1.225f; // Sea level air density kg/m^3

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

    alignas(16) struct SIMDQuat
    {
        float w{ 1.0f }, x{ 0.0f }, y{ 0.0f }, z{ 0.0f };

        constexpr SIMDQuat() = default;
        constexpr SIMDQuat(float inW, float inX, float inY, float inZ) : w(inW), x(inX), y(inY), z(inZ) {}

        static inline SIMDQuat FromAxisAngle(const SIMDVec3& axis, float angleRad)
        {
            float halfAngle = angleRad * 0.5f;
            float s = std::sin(halfAngle);
            SIMDVec3 normAxis = axis.Normalized();
            return SIMDQuat(std::cos(halfAngle), normAxis.x * s, normAxis.y * s, normAxis.z * s);
        }

        inline SIMDVec3 RotateVector(const SIMDVec3& v) const
        {
            SIMDVec3 qv(x, y, z);
            SIMDVec3 t = SIMDVec3::Cross(qv, v) * 2.0f;
            return v + (t * w) + SIMDVec3::Cross(qv, t);
        }
    };

    // Obstacle Surface Geometry
    struct PlanarObstacleWall
    {
        SIMDVec3 WallPointWorld{ 0.0f, 0.0f, 0.0f };
        SIMDVec3 SurfaceNormalWorld{ -1.0f, 0.0f, 0.0f }; // Outward facing vector
        float StaticFrictionCoef{ 0.45f };                // Feather shaft/wall static friction
        float KineticFrictionCoef{ 0.28f };               // Dynamic sliding friction
        float WallComplianceStiffness{ 6500.0f };          // Wall material reaction stiffness (N/m)
        float WallDampingCoefficient{ 180.0f };           // Wall normal velocity dampening
        bool bIsActive{ false };
    };

    enum class EWingSide : uint8_t
    {
        Left = 0,
        Right = 1
    };

    // ============================================================================================
    // 2. BIOMECHANICAL WING STRUCTURE & KINEMATIC STATE
    // ============================================================================================

    constexpr uint32_t COLLISION_NODES_PER_WING = 4; // Wrist, Metacarpal, Primary Feather Mid, Primary Feather Tip

    struct WingJointState
    {
        // Skeletal Articulation Angles (Degrees)
        float ShoulderElevationDeg{ 0.0f };
        float ShoulderAbductionDeg{ 0.0f };
        float ElbowFlexionDeg{ 0.0f };       // Humerus-Radius/Ulna Joint Angle
        float WristFlexionDeg{ 0.0f };       // Radius-Carpometacarpus Joint Angle

        // Targets driven by active flight & passive impact response
        float TargetElbowFlexionDeg{ 0.0f };
        float TargetWristFlexionDeg{ 0.0f };

        // Angular Velocities (Rad/s)
        float ElbowAngularVelocity{ 0.0f };
        float WristAngularVelocity{ 0.0f };

        // Viscoelastic Tendon Non-linear Strain Factor [0.0 = Relaxed, 1.0 = Strain Cap Limit]
        float TendonStrainRatio{ 0.0f };

        // World Coordinates of Skeletal Points
        SIMDVec3 ShoulderPosWorld{ 0.0f, 0.0f, 0.0f };
        SIMDVec3 ElbowPosWorld{ 0.0f, 0.0f, 0.0f };
        SIMDVec3 WristPosWorld{ 0.0f, 0.0f, 0.0f };
        SIMDVec3 WingTipPosWorld{ 0.0f, 0.0f, 0.0f };

        // Discretized Collision Probe Nodes along the outer wing span
        std::array<SIMDVec3, COLLISION_NODES_PER_WING> ProbeNodesWorld{};

        // Effective Aero Metrics
        float EffectiveWingAreaM2{ 0.18f };
        float EffectiveSpanRatio{ 1.0f };
    };

    // ============================================================================================
    // 3. INPUT / OUTPUT DATA FRAMEWORKS
    // ============================================================================================

    struct BirdWingCollisionInputFrame
    {
        SIMDVec3 BodyPositionWorld{ 0.0f, 2.5f, 0.0f };
        SIMDVec3 BodyLinearVelocityWorld{ 0.0f, 0.0f, 22.0f };
        SIMDVec3 BodyAngularVelocityRadSec{ 0.0f, 0.0f, 0.0f };

        SIMDVec3 BodyForward{ 0.0f, 0.0f, 1.0f };
        SIMDVec3 BodyUp{ 0.0f, 1.0f, 0.0f };
        SIMDVec3 BodyRight{ 1.0f, 0.0f, 0.0f };

        float TotalBirdMassKg{ 1.85f };
        float HumerusLengthMeters{ 0.38f };
        float RadiusLengthMeters{ 0.35f };
        float PrimaryFeatherLengthMeters{ 0.32f };

        PlanarObstacleWall ObstacleWall;

        // Base Trajectory Directives
        float RequestedPitchDeg{ 0.0f };
        float RequestedYawDeg{ 0.0f };
        float RequestedRollDeg{ 0.0f };
    };

    struct BirdWingCollisionOutputFrame
    {
        bool bLeftWingInContact{ false };
        bool bRightWingInContact{ false };

        SIMDVec3 LeftWingTotalContactForceN{ 0.0f, 0.0f, 0.0f };
        SIMDVec3 RightWingTotalContactForceN{ 0.0f, 0.0f, 0.0f };

        WingJointState LeftWingState;
        WingJointState RightWingState;

        // Aerodynamic Loss Coefficients
        float LeftWingLiftLossFactor{ 0.0f };  // [0.0 = Normal, 1.0 = Complete Lift Collapse]
        float RightWingLiftLossFactor{ 0.0f };

        // Closed-Loop Flight Control Trajectory Directives
        float CorrectiveYawTorqueNm{ 0.0f };
        float CorrectiveRollTorqueNm{ 0.0f };
        float CorrectivePitchTorqueNm{ 0.0f };

        SIMDVec3 EvasionTargetVectorWorld{ 0.0f, 0.0f, 1.0f };
        float TargetBankAngleDeg{ 0.0f };
    };

    // ============================================================================================
    // 4. MASTER ENGINE SYSTEM CLASS
    // ============================================================================================

    class AAABirdWingCollisionEngine
    {
    private:
        BirdWingCollisionOutputFrame m_Outputs;

        // Biomechanical Stiffness and Viscoelastic Muscle Dampers
        const float MUSCLE_SPRING_STIFFNESS = 520.0f; // N*m/rad return force
        const float MUSCLE_SPRING_DAMPING   = 42.0f;  // Critical damping parameter
        const float MAX_ELBOW_FOLD_ANGLE_DEG = 118.0f;
        const float MAX_WRIST_FOLD_ANGLE_DEG = 92.0f;

        // Filtered Memory Buffers for Smooth Evasion Trajectories
        float m_SmoothEvasionYawRad{ 0.0f };
        float m_SmoothEvasionRollRad{ 0.0f };
        float m_SmoothEvasionPitchRad{ 0.0f };

    public:
        AAABirdWingCollisionEngine() = default;

        /**
         * Runs complete forward physical simulation tick for impact, folding, and flight re-adjustment.
         */
        void TickCollisionAndAdjustment(const BirdWingCollisionInputFrame& inputFrame, float deltaTime)
        {
            // SUB-STEPPING INTEGRATOR: Prevents tunneling or energy spikes at high flight speeds
            constexpr uint32_t SUB_STEP_COUNT = 4;
            float subDt = deltaTime / static_cast<float>(SUB_STEP_COUNT);

            for (uint32_t step = 0; step < SUB_STEP_COUNT; ++step)
            {
                // --------------------------------------------------------------------------------
                // STEP 1: SOLVE FORWARD SKELETAL GEOMETRY & PROBE NODES
                // --------------------------------------------------------------------------------
                ComputeWingKinematics(inputFrame, EWingSide::Left, m_Outputs.LeftWingState);
                ComputeWingKinematics(inputFrame, EWingSide::Right, m_Outputs.RightWingState);

                // --------------------------------------------------------------------------------
                // STEP 2: MULTI-NODE PENETRATION RESOLUTION & CONTACT IMPULSE SOLVER
                // --------------------------------------------------------------------------------
                m_Outputs.bLeftWingInContact = false;
                m_Outputs.bRightWingInContact = false;
                m_Outputs.LeftWingTotalContactForceN = SIMDVec3(0.0f, 0.0f, 0.0f);
                m_Outputs.RightWingTotalContactForceN = SIMDVec3(0.0f, 0.0f, 0.0f);

                if (inputFrame.ObstacleWall.bIsActive)
                {
                    EvaluateWallImpactForWing(inputFrame, EWingSide::Left, m_Outputs.LeftWingState, m_Outputs.bLeftWingInContact, m_Outputs.LeftWingTotalContactForceN, subDt);
                    EvaluateWallImpactForWing(inputFrame, EWingSide::Right, m_Outputs.RightWingState, m_Outputs.bRightWingInContact, m_Outputs.RightWingTotalContactForceN, subDt);
                }

                // --------------------------------------------------------------------------------
                // STEP 3: SOLVE VISCOELASTIC TENDON & PASSIVE FOLDING MECHANICS
                // --------------------------------------------------------------------------------
                IntegrateBiomechanicalFolding(m_Outputs.LeftWingState, subDt);
                IntegrateBiomechanicalFolding(m_Outputs.RightWingState, subDt);

                // --------------------------------------------------------------------------------
                // STEP 4: SOLVE ACTIVE FLIGHT TRAJECTORY RE-TARGETING & EVASION
                // --------------------------------------------------------------------------------
                SolveTrajectoryAdaptation(inputFrame, subDt);
            }
        }

        inline const BirdWingCollisionOutputFrame& GetOutputs() const { return m_Outputs; }

    private:
        /**
         * Computes precise 3D positions of Shoulder, Elbow, Wrist, Tip and mid-feather probe points.
         */
        void ComputeWingKinematics(const BirdWingCollisionInputFrame& input, EWingSide side, WingJointState& wing)
        {
            float sideSign = (side == EWingSide::Left) ? -1.0f : 1.0f;

            // Shoulder position attached to bird torso frame
            SIMDVec3 shoulderOffset = input.BodyRight * (sideSign * 0.16f);
            wing.ShoulderPosWorld = input.BodyPositionWorld + shoulderOffset;

            float elbowFoldRad = wing.ElbowFlexionDeg * DEG_TO_RAD;
            float wristFoldRad = wing.WristFlexionDeg * DEG_TO_RAD;

            // 1. Humerus Orientation & Position
            SIMDVec3 humerusDir = (input.BodyRight * sideSign * std::cos(elbowFoldRad * 0.25f)) -
                                     (input.BodyForward * std::sin(elbowFoldRad * 0.15f));
            wing.ElbowPosWorld = wing.ShoulderPosWorld + (humerusDir.Normalized() * input.HumerusLengthMeters);

            // 2. Forearm (Radius/Ulna) Vector (Sweeps backwards toward tail upon impact)
            SIMDVec3 forearmDir = (input.BodyRight * sideSign * std::cos(elbowFoldRad)) -
                                     (input.BodyForward * std::sin(elbowFoldRad));
            wing.WristPosWorld = wing.ElbowPosWorld + (forearmDir.Normalized() * input.RadiusLengthMeters);

            // 3. Carpometacarpus & Primary Feather Vector (Tucks closely inward)
            SIMDVec3 featherDir = (forearmDir * std::cos(wristFoldRad)) -
                                     (input.BodyForward * std::sin(wristFoldRad));
            wing.WingTipPosWorld = wing.WristPosWorld + (featherDir.Normalized() * input.PrimaryFeatherLengthMeters);

            // 4. Discretize probe nodes along the distal wing section (Most vulnerable to wall strike)
            wing.ProbeNodesWorld[0] = wing.WristPosWorld;
            wing.ProbeNodesWorld[1] = wing.WristPosWorld + (featherDir.Normalized() * (input.PrimaryFeatherLengthMeters * 0.33f));
            wing.ProbeNodesWorld[2] = wing.WristPosWorld + (featherDir.Normalized() * (input.PrimaryFeatherLengthMeters * 0.66f));
            wing.ProbeNodesWorld[3] = wing.WingTipPosWorld;

            // Compute current wing span contraction ratio
            float totalExtendedLength = input.HumerusLengthMeters + input.RadiusLengthMeters + input.PrimaryFeatherLengthMeters;
            float currentSpan = (wing.WingTipPosWorld - wing.ShoulderPosWorld).Length();
            wing.EffectiveSpanRatio = std::clamp(currentSpan / totalExtendedLength, 0.22f, 1.0f);
            wing.EffectiveWingAreaM2 = 0.18f * (wing.EffectiveSpanRatio * wing.EffectiveSpanRatio);
        }

        /**
         * Evaluates multi-point wall penetration, penalty reaction forces, and friction impulses.
         */
        void EvaluateWallImpactForWing(const BirdWingCollisionInputFrame& input, EWingSide side, WingJointState& wing, bool& outContact, SIMDVec3& outTotalForce, float dt)
        {
            const PlanarObstacleWall& wall = input.ObstacleWall;
            outContact = false;
            outTotalForce = SIMDVec3(0.0f, 0.0f, 0.0f);

            float maxPenetrationDepth = 0.0f;

            for (uint32_t i = 0; i < COLLISION_NODES_PER_WING; ++i)
            {
                const SIMDVec3& probePos = wing.ProbeNodesWorld[i];
                SIMDVec3 wallToProbe = probePos - wall.WallPointWorld;
                float penetrationDepth = -SIMDVec3::Dot(wallToProbe, wall.SurfaceNormalWorld);

                if (penetrationDepth > 0.0f) // Penetration into wall solid interior
                {
                    outContact = true;
                    if (penetrationDepth > maxPenetrationDepth)
                    {
                        maxPenetrationDepth = penetrationDepth;
                    }

                    // Probe Node World Velocity
                    SIMDVec3 rArm = probePos - input.BodyPositionWorld;
                    SIMDVec3 nodeVelocity = input.BodyLinearVelocityWorld + SIMDVec3::Cross(input.BodyAngularVelocityRadSec, rArm);

                    // Normal Penalty Force = Stiffness * Depth - Damping * NormalVelocity
                    float normalVel = SIMDVec3::Dot(nodeVelocity, wall.SurfaceNormalWorld);
                    float springForceMag = penetrationDepth * wall.WallComplianceStiffness;
                    float dampingForceMag = -normalVel * wall.WallDampingCoefficient;
                    float netNormalForceMag = std::max(0.0f, springForceMag + dampingForceMag);

                    SIMDVec3 normalForce = wall.SurfaceNormalWorld * netNormalForceMag;

                    // Surface Tangent Sliding Friction
                    SIMDVec3 tangentVel = nodeVelocity - (wall.SurfaceNormalWorld * normalVel);
                    float tangentSpeed = tangentVel.Length();
                    SIMDVec3 frictionForce(0.0f, 0.0f, 0.0f);

                    if (tangentSpeed > 0.001f)
                    {
                        SIMDVec3 tangentDir = tangentVel / tangentSpeed;
                        float frictionMag = netNormalForceMag * wall.KineticFrictionCoef;
                        frictionForce = tangentDir * (-frictionMag);
                    }

                    outTotalForce = outTotalForce + (normalForce + frictionForce);
                }
            }

            if (outContact)
            {
                // Force Wing Folding proportional to impact force intensity & depth
                float contactForceMagnitude = outTotalForce.Length();
                float foldMoment = std::max(0.12f, contactForceMagnitude * 0.038f + maxPenetrationDepth * 40.0f);

                wing.TargetElbowFlexionDeg = std::clamp(wing.TargetElbowFlexionDeg + foldMoment * 15.0f, 0.0f, MAX_ELBOW_FOLD_ANGLE_DEG);
                wing.TargetWristFlexionDeg = std::clamp(wing.TargetWristFlexionDeg + foldMoment * 12.0f, 0.0f, MAX_WRIST_FOLD_ANGLE_DEG);

                // Compute Tendon Strain Ratio
                wing.TendonStrainRatio = std::clamp(maxPenetrationDepth / 0.28f, 0.0f, 1.0f);
            }
            else
            {
                // Passive Elastic Unfolding (Returns to active flight posture)
                wing.TargetElbowFlexionDeg = std::max(0.0f, wing.TargetElbowFlexionDeg - dt * 160.0f);
                wing.TargetWristFlexionDeg = std::max(0.0f, wing.TargetWristFlexionDeg - dt * 130.0f);
                wing.TendonStrainRatio = std::max(0.0f, wing.TendonStrainRatio - dt * 4.0f);
            }
        }

        /**
         * Solves 2nd-order differential equations for viscoelastic muscle-joint passive folding.
         */
        void IntegrateBiomechanicalFolding(WingJointState& wing, float dt)
        {
            // 1. Elbow Joint Differential Solver
            float elbowError = wing.TargetElbowFlexionDeg - wing.ElbowFlexionDeg;
            float elbowAccel = (elbowError * MUSCLE_SPRING_STIFFNESS) - (wing.ElbowAngularVelocity * MUSCLE_SPRING_DAMPING);
            wing.ElbowAngularVelocity += elbowAccel * dt;
            wing.ElbowFlexionDeg += wing.ElbowAngularVelocity * dt;
            wing.ElbowFlexionDeg = std::clamp(wing.ElbowFlexionDeg, 0.0f, MAX_ELBOW_FOLD_ANGLE_DEG);

            // 2. Wrist Joint Differential Solver
            float wristError = wing.TargetWristFlexionDeg - wing.WristFlexionDeg;
            float wristAccel = (wristError * MUSCLE_SPRING_STIFFNESS * 1.15f) - (wing.WristAngularVelocity * MUSCLE_SPRING_DAMPING * 1.1f);
            wing.WristAngularVelocity += wristAccel * dt;
            wing.WristFlexionDeg += wing.WristAngularVelocity * dt;
            wing.WristFlexionDeg = std::clamp(wing.WristFlexionDeg, 0.0f, MAX_WRIST_FOLD_ANGLE_DEG);
        }

        /**
         * Real-time flight controller adaptation: adjusts trajectory vector, bank angle, and aerodynamic balance.
         */
        void SolveTrajectoryAdaptation(const BirdWingCollisionInputFrame& input, float dt)
        {
            float targetEvasionYaw = 0.0f;
            float targetEvasionRoll = 0.0f;
            float targetEvasionPitch = 0.0f;

            float leftFoldFactor  = m_Outputs.LeftWingState.ElbowFlexionDeg / MAX_ELBOW_FOLD_ANGLE_DEG;
            float rightFoldFactor = m_Outputs.RightWingState.ElbowFlexionDeg / MAX_ELBOW_FOLD_ANGLE_DEG;

            m_Outputs.LeftWingLiftLossFactor  = std::clamp(leftFoldFactor * 1.2f, 0.0f, 0.90f);
            m_Outputs.RightWingLiftLossFactor = std::clamp(rightFoldFactor * 1.2f, 0.0f, 0.90f);

            // Calculate Asymmetric Aerodynamic Rolling Moment caused by folded wing lift collapse
            float airspeed = input.BodyLinearVelocityWorld.Length();
            float dynamicPressure = 0.5f * RHO_AIR_DENSITY * (airspeed * airspeed);
            float asymmetricLiftForceN = dynamicPressure * (m_Outputs.LeftWingState.EffectiveWingAreaM2 - m_Outputs.RightWingState.EffectiveWingAreaM2) * 1.2f;
            float asymmetricRollMomentNm = asymmetricLiftForceN * 0.45f;

            // Determine Wall Evasion Routing
            if (m_Outputs.bLeftWingInContact || leftFoldFactor > 0.04f)
            {
                // Left Wing Hit Wall -> Turn Right (Yaw +), Bank Right (Roll +), Pitch Up to counter height loss
                targetEvasionYaw += (0.65f * leftFoldFactor);
                targetEvasionRoll += (0.85f * leftFoldFactor);
                targetEvasionPitch += (0.18f * leftFoldFactor);
            }

            if (m_Outputs.bRightWingInContact || rightFoldFactor > 0.04f)
            {
                // Right Wing Hit Wall -> Turn Left (Yaw -), Bank Left (Roll -), Pitch Up
                targetEvasionYaw -= (0.65f * rightFoldFactor);
                targetEvasionRoll -= (0.85f * rightFoldFactor);
                targetEvasionPitch += (0.18f * rightFoldFactor);
            }

            // Continuous Low-Pass Smoothing (Filters out twitching and ensures ultra-fluid trajectory shift)
            m_SmoothEvasionYawRad   += (targetEvasionYaw   - m_SmoothEvasionYawRad)   * (dt * 14.0f);
            m_SmoothEvasionRollRad  += (targetEvasionRoll  - m_SmoothEvasionRollRad)  * (dt * 11.0f);
            m_SmoothEvasionPitchRad += (targetEvasionPitch - m_SmoothEvasionPitchRad) * (dt * 10.0f);

            // Torque Correction Signals sent to Flight Control Actuators
            m_Outputs.CorrectiveYawTorqueNm   = (m_SmoothEvasionYawRad * 24.0f);
            m_Outputs.CorrectiveRollTorqueNm  = (m_SmoothEvasionRollRad * 30.0f) + asymmetricRollMomentNm;
            m_Outputs.CorrectivePitchTorqueNm = (m_SmoothEvasionPitchRad * 18.0f);

            // Re-calculate World Space Trajectory Target Vector
            SIMDVec3 modifiedForward = input.BodyForward + 
                                       (input.BodyRight * m_SmoothEvasionYawRad) + 
                                       (input.BodyUp * m_SmoothEvasionPitchRad);
            m_Outputs.EvasionTargetVectorWorld = modifiedForward.Normalized();
            m_Outputs.TargetBankAngleDeg = input.RequestedRollDeg + (m_SmoothEvasionRollRad * RAD_TO_DEG);
        }
    };
}

#endif // AAA_BIRD_WING_COLLISION_FOLD_ADJUST_ENGINE_HPP
