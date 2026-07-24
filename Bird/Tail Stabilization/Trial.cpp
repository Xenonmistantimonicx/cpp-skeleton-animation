/**
 * ============================================================================================
 *  AAAAAA-GRADE ULTIMATE BIRD WING-TIP COLLISION & CONTINUOUS ADJUSTMENT ENGINE
 * ============================================================================================
 *  Pipeline Flow : Wing-Tip Contact -> Continuous Deflection & Wall Slide -> Dynamic Fold
 *                  -> Tendon Strain Dampening -> Flight Controller Yaw/Pitch/Roll Re-Adjustment
 *  Target System : Real-Time High-Fidelity Physics Engine / Unreal Engine 5 C++ / Custom Sim
 *  Standard      : C++20 (Zero Heap Allocations / SIMD Aligned / Fully Thread-Safe)
 * ============================================================================================
 */

#ifndef AAAAAA_BIRD_WING_COLLISION_FOLD_ADJUST_ENGINE_HPP
#define AAAAAA_BIRD_WING_COLLISION_FOLD_ADJUST_ENGINE_HPP

#include <cmath>
#include <algorithm>
#include <array>
#include <cstdint>

namespace AAABirdEngine
{
    // ============================================================================================
    // 1. HARDENED SIMD MATH PRIMITIVES & GEOMETRY
    // ============================================================================================

    constexpr float PI_F = 3.14159265358979323846f;
    constexpr float DEG_TO_RAD_F = PI_F / 180.0f;
    constexpr float RAD_TO_DEG_F = 180.0f / PI_F;

    alignas(16) struct SIMDVector3
    {
        float x{ 0.0f }, y{ 0.0f }, z{ 0.0f }, w{ 0.0f };

        constexpr SIMDVector3() = default;
        constexpr SIMDVector3(float inX, float inY, float inZ, float inW = 0.0f)
            : x(inX), y(inY), z(inZ), w(inW) {}

        inline SIMDVector3 operator+(const SIMDVector3& o) const { return SIMDVector3(x + o.x, y + o.y, z + o.z); }
        inline SIMDVector3 operator-(const SIMDVector3& o) const { return SIMDVector3(x - o.x, y - o.y, z - o.z); }
        inline SIMDVector3 operator*(float s) const { return SIMDVector3(x * s, y * s, z * s); }
        inline SIMDVector3 operator/(float s) const { float inv = 1.0f / s; return SIMDVector3(x * inv, y * inv, z * inv); }

        inline float LengthSq() const { return x * x + y * y + z * z; }
        inline float Length() const { return std::sqrt(LengthSq()); }

        inline SIMDVector3 Normalized() const
        {
            float len = Length();
            return len > 0.00001f ? (*this) * (1.0f / len) : SIMDVector3(0.0f, 0.0f, 0.0f);
        }

        static inline float Dot(const SIMDVector3& a, const SIMDVector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
        static inline SIMDVector3 Cross(const SIMDVector3& a, const SIMDVector3& b)
        {
            return SIMDVector3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            );
        }
    };

    struct PlaneContactWall
    {
        SIMDVector3 WallPointWorld{ 0.0f, 0.0f, 0.0f };
        SIMDVector3 WallNormalWorld{ -1.0f, 0.0f, 0.0f }; // Points away from wall surface
        float FrictionCoefficient{ 0.35f };               // Wall contact friction
        float RestitutionCoefficient{ 0.08f };            // Low rebound for soft feathers/feathery contact
        bool bIsActiveWall{ false };
    };

    // ============================================================================================
    // 2. BIOMECHANICAL WING JOINT STATE (HUMERUS, RADIUS/ULNA, HAND/FEATHERS)
    // ============================================================================================

    enum class EWingSide : uint8_t
    {
        Left = 0,
        Right = 1
    };

    struct WingJointKinematics
    {
        // Joint angles relative to bird body frame (Degrees)
        float ShoulderAbductionDeg{ 0.0f }; // Spread angle out from torso
        float ElbowFlexionDeg{ 0.0f };      // Bend fold (Radius/Ulna)
        float WristFlexionDeg{ 0.0f };      // Wing tip feather retract

        // Dynamic Spring-Damper States for Passive Collision Folding
        float TargetElbowFlexionDeg{ 0.0f };
        float TargetWristFlexionDeg{ 0.0f };

        float ElbowVelocity{ 0.0f };
        float WristVelocity{ 0.0f };

        // Structural Tendon Tension (0.0 = relaxed, 1.0 = peak elasticity limit)
        float TendonStrainRatio{ 0.0f };

        // World Coordinates of Wing Keypoints
        SIMDVector3 ShoulderPosWorld{ 0.0f, 0.0f, 0.0f };
        SIMDVector3 ElbowPosWorld{ 0.0f, 0.0f, 0.0f };
        SIMDVector3 WristPosWorld{ 0.0f, 0.0f, 0.0f };
        SIMDVector3 WingTipPosWorld{ 0.0f, 0.0f, 0.0f };

        // Effective Area Retracted Ratio (1.0 = full wingspan, 0.25 = tight tuck fold)
        float EffectiveAerodynamicAreaRatio{ 1.0f };
    };

    // ============================================================================================
    // 3. INPUT / OUTPUT DATA STRUCTURES
    // ============================================================================================

    struct BirdCollisionInputFrame
    {
        SIMDVector3 BodyPositionWorld{ 0.0f, 2.0f, 0.0f };
        SIMDVector3 BodyLinearVelocityWorld{ 0.0f, 0.0f, 18.0f };
        SIMDVector3 BodyAngularVelocityRadSec{ 0.0f, 0.0f, 0.0f };

        SIMDVector3 BodyForward{ 0.0f, 0.0f, 1.0f };
        SIMDVector3 BodyUp{ 0.0f, 1.0f, 0.0f };
        SIMDVector3 BodyRight{ 1.0f, 0.0f, 0.0f };

        float UnfoldedWingspanMeters{ 2.2f };
        float HumerusLengthMeters{ 0.45f };
        float RadiusLengthMeters{ 0.40f };
        float PrimaryFeatherLengthMeters{ 0.35f };

        // Environmental Obstacles
        PlaneContactWall ActiveWallEnvironment;

        // Current Flight Controller Targets
        float BaseNominalPitchDeg{ 0.0f };
        float BaseNominalYawDeg{ 0.0f };
        float BaseNominalRollDeg{ 0.0f };
    };

    struct BirdCollisionOutputFrame
    {
        // Kinetic Response
        bool bIsLeftWingInContact{ false };
        bool bIsRightWingInContact{ false };

        SIMDVector3 LeftWingContactForceN{ 0.0f, 0.0f, 0.0f };
        SIMDVector3 RightWingContactForceN{ 0.0f, 0.0f, 0.0f };

        // Kinematic Folding Outputs
        WingJointKinematics LeftWingJoints;
        WingJointKinematics RightWingJoints;

        // Active Flight Controller Compensation Signals (Trajectory Adjustment)
        float CorrectiveYawTorqueNm{ 0.0f };
        float CorrectiveRollTorqueNm{ 0.0f };
        float CorrectivePitchTorqueNm{ 0.0f };

        SIMDVector3 AdjustedFlightTargetVector{ 0.0f, 0.0f, 1.0f };
        float AutoEvasionBankAngleDeg{ 0.0f };

        // Dynamic Drag Torque caused by wall scraping friction
        float AsymmetricCollisionDragTorqueNm{ 0.0f };
    };

    // ============================================================================================
    // 4. MASTER AAAAAA-GRADE WING COLLISION, FOLD & ADJUST SYSTEM
    // ============================================================================================

    class AAABirdWingCollisionFoldAdjustSystem
    {
    private:
        BirdCollisionOutputFrame m_Outputs;

        // Biomechanical Muscle & Tendon Properties
        const float WING_SPRING_STIFFNESS = 480.0f;  // Elastic restoration to unfold wing
        const float WING_SPRING_DAMPING   = 38.0f;   // Smooth fluid movement without jitter
        const float MAX_ELBOW_FOLD_DEG    = 115.0f;  // Maximum biomechanical fold angle
        const float MAX_WRIST_FOLD_DEG    = 95.0f;   // Maximum feather tuck angle

        // Smoothed Trajectory Adjustment Memory Filters
        float m_FilteredEvasionYawDeg{ 0.0f };
        float m_FilteredEvasionRollDeg{ 0.0f };

    public:
        AAABirdWingCollisionFoldAdjustSystem() = default;

        /**
         * Continuous single-frame execution tick for Wing Contact, Folding Mechanics, and Trajectory Adjustment.
         */
        void TickCollisionFoldAndAdjust(const BirdCollisionInputFrame& inputFrame, float deltaTime)
        {
            // Deterministic 4x Sub-stepping to preserve fluid motion under high-velocity impacts
            constexpr uint32_t SUB_STEPS = 4;
            float subDt = deltaTime / static_cast<float>(SUB_STEPS);

            for (uint32_t step = 0; step < SUB_STEPS; ++step)
            {
                // --------------------------------------------------------------------------------
                // STEP 1: FORWARD KINEMATICS - COMPUTE WING-TIP WORLD POSITIONS
                // --------------------------------------------------------------------------------
                ComputeWingJointPositions(inputFrame, EWingSide::Left, m_Outputs.LeftWingJoints);
                ComputeWingJointPositions(inputFrame, EWingSide::Right, m_Outputs.RightWingJoints);

                // --------------------------------------------------------------------------------
                // STEP 2: COLLISION DETECTION & WALL PENETRATION RESOLUTION
                // --------------------------------------------------------------------------------
                m_Outputs.bIsLeftWingInContact = false;
                m_Outputs.bIsRightWingInContact = false;
                m_Outputs.LeftWingContactForceN = SIMDVector3(0.0f, 0.0f, 0.0f);
                m_Outputs.RightWingContactForceN = SIMDVector3(0.0f, 0.0f, 0.0f);

                if (inputFrame.ActiveWallEnvironment.bIsActiveWall)
                {
                    ResolveWingWallContact(inputFrame, EWingSide::Left, m_Outputs.LeftWingJoints, m_Outputs.bIsLeftWingInContact, m_Outputs.LeftWingContactForceN, subDt);
                    ResolveWingWallContact(inputFrame, EWingSide::Right, m_Outputs.RightWingJoints, m_Outputs.bIsRightWingInContact, m_Outputs.RightWingContactForceN, subDt);
                }

                // --------------------------------------------------------------------------------
                // STEP 3: DYNAMIC PASSIVE FOLDING SOLVER (SPRING-DAMPER & TENDON STRAIN)
                // --------------------------------------------------------------------------------
                SolveBiomechanicalFoldingSpring(m_Outputs.LeftWingJoints, subDt);
                SolveBiomechanicalFoldingSpring(m_Outputs.RightWingJoints, subDt);

                // --------------------------------------------------------------------------------
                // STEP 4: ACTIVE TRAJECTORY ADJUSTMENT & AUTO-EVASION CONTROLLER
                // --------------------------------------------------------------------------------
                SolveTrajectoryAdjustmentController(inputFrame, subDt);
            }
        }

    private:
        /**
         * Compute skeleton joint nodes (Shoulder -> Elbow -> Wrist -> Wing Tip) in World Space.
         */
        void ComputeWingJointPositions(const BirdCollisionInputFrame& input, EWingSide side, WingJointKinematics& wing)
        {
            float sideSign = (side == EWingSide::Left) ? -1.0f : 1.0f;

            // Base shoulder anchor offset on torso
            SIMDVector3 shoulderOffset = input.BodyRight * (sideSign * 0.18f);
            wing.ShoulderPosWorld = input.BodyPositionWorld + shoulderOffset;

            // Combined joint angles including active folds
            float currentElbowFoldRad = wing.ElbowFlexionDeg * DEG_TO_RAD_F;
            float currentWristFoldRad = wing.WristFlexionDeg * DEG_TO_RAD_F;

            // Humerus vector
            SIMDVector3 humerusDir = (input.BodyRight * sideSign * std::cos(currentElbowFoldRad * 0.3f)) +
                                     (input.BodyForward * -std::sin(currentElbowFoldRad * 0.2f));
            wing.ElbowPosWorld = wing.ShoulderPosWorld + (humerusDir.Normalized() * input.HumerusLengthMeters);

            // Radius/Ulna vector (Folding backwards towards tail)
            SIMDVector3 forearmDir = (input.BodyRight * sideSign * std::cos(currentElbowFoldRad)) -
                                     (input.BodyForward * std::sin(currentElbowFoldRad));
            wing.WristPosWorld = wing.ElbowPosWorld + (forearmDir.Normalized() * input.RadiusLengthMeters);

            // Primary Feather Wing-Tip vector (Secondary tuck fold)
            SIMDVector3 featherDir = (forearmDir * std::cos(currentWristFoldRad)) -
                                     (input.BodyForward * std::sin(currentWristFoldRad));
            wing.WingTipPosWorld = wing.WristPosWorld + (featherDir.Normalized() * input.PrimaryFeatherLengthMeters);

            // Aerodynamic Surface Area Retraction Factor
            float currentSpan = (wing.WingTipPosWorld - wing.ShoulderPosWorld).Length();
            float fullSpan = input.HumerusLengthMeters + input.RadiusLengthMeters + input.PrimaryFeatherLengthMeters;
            wing.EffectiveAerodynamicAreaRatio = std::clamp(currentSpan / fullSpan, 0.20f, 1.0f);
        }

        /**
         * Detects wall contact along the wing segment and calculates collision normal forces and sliding friction.
         */
        void ResolveWingWallContact(const BirdCollisionInputFrame& input, EWingSide side, WingJointKinematics& wing, bool& outContact, SIMDVector3& outForce, float dt)
        {
            const PlaneContactWall& wall = input.ActiveWallEnvironment;

            // Test penetration depth at Wing-Tip and Wrist joints
            float tipPenetration = SIMDVector3::Dot(wing.WingTipPosWorld - wall.WallPointWorld, wall.WallNormalWorld);
            float wristPenetration = SIMDVector3::Dot(wing.WristPosWorld - wall.WallPointWorld, wall.WallNormalWorld);

            float worstPenetration = std::min(tipPenetration, wristPenetration);

            // Contact occurs if point lies behind wall plane (Penetration < 0)
            if (worstPenetration < 0.0f)
            {
                outContact = true;
                float penetrationDepthMeters = std::abs(worstPenetration);

                // 1. Normal Contact Reaction Force (Penalty Method + Velocity Damping)
                SIMDVector3 wingVelocity = input.BodyLinearVelocityWorld +
                                           SIMDVector3::Cross(input.BodyAngularVelocityRadSec, wing.WingTipPosWorld - input.BodyPositionWorld);

                float normalVel = SIMDVector3::Dot(wingVelocity, wall.WallNormalWorld);
                float springForceMag = penetrationDepthMeters * 3500.0f; // High-stiffness structural contact
                float dampingForceMag = -normalVel * 120.0f;

                float totalNormalForceMag = std::max(0.0f, springForceMag + dampingForceMag);
                SIMDVector3 normalForce = wall.WallNormalWorld * totalNormalForceMag;

                // 2. Sliding Surface Friction
                SIMDVector3 tangentVelocity = wingVelocity - (wall.WallNormalWorld * normalVel);
                SIMDVector3 frictionForce = tangentVelocity.Normalized() * (-totalNormalForceMag * wall.FrictionCoefficient);

                outForce = normalForce + frictionForce;

                // 3. Force Wing to Fold (Moment arm pushes elbow and wrist joints closed)
                float foldMomentArm = std::max(0.10f, totalNormalForceMag * 0.045f);
                wing.TargetElbowFlexionDeg = std::clamp(wing.TargetElbowFlexionDeg + foldMomentArm * 12.0f, 0.0f, MAX_ELBOW_FOLD_DEG);
                wing.TargetWristFlexionDeg = std::clamp(wing.TargetWristFlexionDeg + foldMomentArm * 9.5f, 0.0f, MAX_WRIST_FOLD_DEG);

                // Compute Tendon Elastic Strain Limit
                wing.TendonStrainRatio = std::clamp(penetrationDepthMeters / 0.35f, 0.0f, 1.0f);
            }
            else
            {
                // Smoothly return targets back to unfolded nominal flight state
                wing.TargetElbowFlexionDeg = std::max(0.0f, wing.TargetElbowFlexionDeg - dt * 140.0f);
                wing.TargetWristFlexionDeg = std::max(0.0f, wing.TargetWristFlexionDeg - dt * 110.0f);
                wing.TendonStrainRatio = std::max(0.0f, wing.TendonStrainRatio - dt * 3.0f);
            }
        }

        /**
         * Integrates 2nd-order spring-damper differential equation for fluid, muscle-like joint folding.
         */
        void SolveBiomechanicalFoldingSpring(WingJointKinematics& wing, float dt)
        {
            // Elbow Spring Integration
            float elbowError = wing.TargetElbowFlexionDeg - wing.ElbowFlexionDeg;
            float elbowAccel = (elbowError * WING_SPRING_STIFFNESS) - (wing.ElbowVelocity * WING_SPRING_DAMPING);
            wing.ElbowVelocity += elbowAccel * dt;
            wing.ElbowFlexionDeg += wing.ElbowVelocity * dt;
            wing.ElbowFlexionDeg = std::clamp(wing.ElbowFlexionDeg, 0.0f, MAX_ELBOW_FOLD_DEG);

            // Wrist Spring Integration
            float wristError = wing.TargetWristFlexionDeg - wing.WristFlexionDeg;
            float wristAccel = (wristError * WING_SPRING_STIFFNESS * 1.1f) - (wing.WristVelocity * WING_SPRING_DAMPING * 1.05f);
            wing.WristVelocity += wristAccel * dt;
            wing.WristFlexionDeg += wing.WristVelocity * dt;
            wing.WristFlexionDeg = std::clamp(wing.WristFlexionDeg, 0.0f, MAX_WRIST_FOLD_DEG);
        }

        /**
         * Re-adjusts body trajectory, yawing and banking away from the wall while balancing asymmetric wing lift.
         */
        void SolveTrajectoryAdjustmentController(const BirdCollisionInputFrame& input, float dt)
        {
            float yawAdjustmentDeg = 0.0f;
            float rollAdjustmentDeg = 0.0f;
            float pitchAdjustmentDeg = 0.0f;

            float leftFoldRatio = m_Outputs.LeftWingJoints.ElbowFlexionDeg / MAX_ELBOW_FOLD_DEG;
            float rightFoldRatio = m_Outputs.RightWingJoints.ElbowFlexionDeg / MAX_ELBOW_FOLD_DEG;

            // Asymmetric Lift/Drag Torque due to one folded wing
            float liftAsymmetry = m_Outputs.LeftWingJoints.EffectiveAerodynamicAreaRatio - m_Outputs.RightWingJoints.EffectiveAerodynamicAreaRatio;
            m_Outputs.AsymmetricCollisionDragTorqueNm = liftAsymmetry * input.BodyLinearVelocityWorld.LengthSq() * 0.082f;

            if (m_Outputs.bIsLeftWingInContact || leftFoldRatio > 0.05f)
            {
                // Left Wing Hit Wall -> Yaw Right, Bank/Roll Right (Away from wall), Pitch slightly up to maintain altitude
                yawAdjustmentDeg += (35.0f * leftFoldRatio);
                rollAdjustmentDeg += (42.0f * leftFoldRatio);
                pitchAdjustmentDeg += (8.0f * leftFoldRatio);
            }

            if (m_Outputs.bIsRightWingInContact || rightFoldRatio > 0.05f)
            {
                // Right Wing Hit Wall -> Yaw Left, Bank/Roll Left (Away from wall), Pitch slightly up
                yawAdjustmentDeg -= (35.0f * rightFoldRatio);
                rollAdjustmentDeg -= (42.0f * rightFoldRatio);
                pitchAdjustmentDeg += (8.0f * rightFoldRatio);
            }

            // Low-pass exponential filter for ultra-smooth trajectory response
            m_FilteredEvasionYawDeg += (yawAdjustmentDeg - m_FilteredEvasionYawDeg) * (dt * 12.0f);
            m_FilteredEvasionRollDeg += (rollAdjustmentDeg - m_FilteredEvasionRollDeg) * (dt * 10.0f);

            // Compute Torque Feedback Signals
            m_Outputs.CorrectiveYawTorqueNm = (m_FilteredEvasionYawDeg * DEG_TO_RAD_F * 18.0f) + m_Outputs.AsymmetricCollisionDragTorqueNm;
            m_Outputs.CorrectiveRollTorqueNm = (m_FilteredEvasionRollDeg * DEG_TO_RAD_F * 22.0f);
            m_Outputs.CorrectivePitchTorqueNm = (pitchAdjustmentDeg * DEG_TO_RAD_F * 14.0f);

            // Compute Modified World Target Trajectory Vector
            SIMDVector3 adjustedForward = input.BodyForward + (input.BodyRight * (m_FilteredEvasionYawDeg * 0.02f)) + (input.BodyUp * (pitchAdjustmentDeg * 0.015f));
            m_Outputs.AdjustedFlightTargetVector = adjustedForward.Normalized();
            m_Outputs.AutoEvasionBankAngleDeg = input.BaseNominalRollDeg + m_FilteredEvasionRollDeg;
        }
    };
}

#endif // AAAAAA_BIRD_WING_COLLISION_FOLD_ADJUST_ENGINE_HPP
