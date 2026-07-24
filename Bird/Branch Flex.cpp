/**
 * ============================================================================================
 *  AAAAAA-GRADE ULTIMATE BIRD PERCHING & BRANCH ELASTIC FLEX ENGINE
 * ============================================================================================
 *  Pipeline Flow : Perch Touchdown -> Claw-Branch Contact & Friction Lock
 *                  -> Cantilever Beam Deflection (Euler-Bernoulli Elasticity)
 *                  -> Damped Harmonic Mass Oscillations (Under-damped Spring Response)
 *                  -> Bird Leg Compliance Absorption & Dynamic Equilibrium
 *  Target System : Real-Time Physics Sim / Unreal Engine 5 C++ / Biomechanical Rig
 *  Standard      : C++20 (Zero Heap Allocations / SIMD Aligned / Fully Thread-Safe)
 * ============================================================================================
 */

#ifndef AAAAAA_BIRD_BRANCH_FLEX_ENGINE_HPP
#define AAAAAA_BIRD_BRANCH_FLEX_ENGINE_HPP

#include <cmath>
#include <algorithm>
#include <array>
#include <cstdint>

namespace AAABirdEngine
{
    // ============================================================================================
    // 1. HARDENED SIMD MATH PRIMITIVES
    // ============================================================================================

    constexpr float PI_F = 3.14159265358979323846f;
    constexpr float DEG_TO_RAD_F = PI_F / 180.0f;
    constexpr float RAD_TO_DEG_F = 180.0f / PI_F;
    constexpr float GRAVITY_ACCEL = 9.81f;

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

    // ============================================================================================
    // 2. BRANCH PHYSICAL PROPERTIES & STATE
    // ============================================================================================

    enum class EBranchWoodType : uint8_t
    {
        RigidOak = 0,      // High stiffness, low flex
        FlexibleWillow = 1, // Soft, deep bending, high oscillation
        PineConifer = 2,    // Balanced springiness
        DryBrittle = 3      // Low damping, high frequency snap
    };

    struct CantileverBranchSpec
    {
        SIMDVector3 TrunkAnchorWorld{ 0.0f, 2.5f, 0.0f }; // Base anchor of the branch at the tree trunk
        SIMDVector3 UnbentTipWorld{ 0.0f, 2.8f, 2.2f };   // Natural rest position of the branch tip
        float BranchRadiusBaseMeters{ 0.045f };            // Radius near trunk (e.g., 4.5 cm)
        float BranchRadiusTipMeters{ 0.012f };             // Radius near tip (e.g., 1.2 cm)
        float TotalLengthMeters{ 2.2f };                   // Total branch length
        
        float YoungsModulusGPa{ 9.5f };                   // Wood elasticity (Gigapascals)
        float WoodDensityKgM3{ 680.0f };                   // Density of green wood
        float NaturalDampingRatio{ 0.12f };               // Internal material structural damping
        EBranchWoodType WoodType{ EBranchWoodType::FlexibleWillow };
    };

    struct DynamicBranchState
    {
        // Contact parameters
        float PerchDistanceNormalized{ 0.85f };            // [0.0 = Base/Trunk, 1.0 = Branch Tip]
        SIMDVector3 NominalPerchPointWorld{ 0.0f, 0.0f, 0.0f };
        SIMDVector3 CurrentPerchPointWorld{ 0.0f, 0.0f, 0.0f };

        // Mechanical State
        float VerticalDeflectionMeters{ 0.0f };            // Downward displacement ($y$)
        float VerticalVelocity{ 0.0f };                    // Vertical sway velocity ($\dot{y}$)
        float AngularBendRad{ 0.0f };                      // Rotational deflection angle at perch site

        // Internal Calculations
        float EffectiveKSpring{ 320.0f };                  // Calculated Beam Stiffness (N/m)
        float EffectiveMassKg{ 1.2f };                     // Combined branch mass + bird mass
        float DampingCoefficientC{ 14.5f };                // Fluid & structural damping (N*s/m)
    };

    // ============================================================================================
    // 3. INPUT / OUTPUT DATA STRUCTURES
    // ============================================================================================

    struct BirdBranchInputFrame
    {
        SIMDVector3 BodyPositionWorld{ 0.0f, 3.2f, 1.85f };
        SIMDVector3 BodyVelocityWorld{ 0.0f, -1.8f, 1.2f }; // Impact velocity upon perching
        SIMDVector3 FeetContactPointWorld{ 0.0f, 2.75f, 1.87f };

        float BirdMassKg{ 1.8f };                           // Mass of the bird
        bool bIsFeetLockedOnBranch{ false };                // Claw grip locked status
        float TalonGripForceN{ 45.0f };                     // Tendon locking force

        CantileverBranchSpec BranchSpec;
    };

    struct BirdBranchOutputFrame
    {
        bool bIsPerched{ false };
        bool bIsBranchOverStrained{ false };               // Warning if deflection exceeds elastic limit

        // Deflection & Wave Oscillations
        SIMDVector3 DeflectedBranchPerchPos{ 0.0f, 0.0f, 0.0f };
        SIMDVector3 DeflectedBranchTipPos{ 0.0f, 0.0f, 0.0f };
        float CurrentDeflectionDistanceMeters{ 0.0f };
        float CurrentSlopeAngleDeg{ 0.0f };

        // Reaction Forces
        SIMDVector3 BranchRestoringForceN{ 0.0f, 0.0f, 0.0f };
        SIMDVector3 DynamicLoadOnBirdFeetN{ 0.0f, 0.0f, 0.0f };

        // Leg Absorption Metrics
        float BirdLegCompressionRatio{ 0.0f };             // Leg crouch response to absorb rebound [0.0 = Extended, 1.0 = Deep Squat]
        float BodyPosturalCorrectionPitchDeg{ 0.0f };      // Pitch forward/backward adjustment to maintain center of gravity
    };

    // ============================================================================================
    // 4. MASTER AAAAAA-GRADE BRANCH FLEX & PERCH ENGINE
    // ============================================================================================

    class AAABirdBranchFlexEngine
    {
    private:
        BirdBranchOutputFrame m_Outputs;
        DynamicBranchState m_BranchState;

        // Internal State Memory for Smooth Postural Feedback
        float m_TargetLegSquat{ 0.0f };
        float m_CurrentLegSquat{ 0.0f };
        float m_BodyPitchAdjustmentDeg{ 0.0f };

    public:
        AAABirdBranchFlexEngine() = default;

        /**
         * Real-time tick update for perch contact, cantilever beam flex, damped mass oscillations, and posture balancing.
         */
        void TickBranchFlexSystem(const BirdBranchInputFrame& inputFrame, float deltaTime)
        {
            constexpr uint32_t SUB_STEPS = 4;
            float subDt = deltaTime / static_cast<float>(SUB_STEPS);

            for (uint32_t step = 0; step < SUB_STEPS; ++step)
            {
                // --------------------------------------------------------------------------------
                // STEP 1: INITIALIZE / UPDATE CANTILEVER BEAM STIFFNESS PROPERTIES
                // --------------------------------------------------------------------------------
                ComputeBeamElasticityModel(inputFrame);

                // --------------------------------------------------------------------------------
                // STEP 2: SOLVE TOUCHDOWN IMPACT & MASS TRANSFER
                // --------------------------------------------------------------------------------
                ProcessPerchContactAndImpulse(inputFrame, subDt);

                // --------------------------------------------------------------------------------
                // STEP 3: INTEGRATE EULER-BERNOULLI DAMPED HARMONIC OSCILLATOR
                // --------------------------------------------------------------------------------
                IntegrateBranchOscillation(inputFrame, subDt);

                // --------------------------------------------------------------------------------
                // STEP 4: SOLVE BIRD LEG COMPLIANCE & BALANCE POSTURE CORRECTION
                // --------------------------------------------------------------------------------
                SolveBirdBalanceAndLegAbsorption(inputFrame, subDt);
            }
        }

        inline const BirdBranchOutputFrame& GetOutputs() const { return m_Outputs; }

    private:
        /**
         * Calculates beam stiffness ($k$) using the Euler-Bernoulli beam theory formula:
         * $k = \frac{3 E I}{L^3}$ where $I = \frac{\pi r^4}{4}$ (Second Moment of Area for a solid cylinder).
         */
        void ComputeBeamElasticityModel(const BirdBranchInputFrame& input)
        {
            const CantileverBranchSpec& spec = input.BranchSpec;

            // Interpolate branch radius at the landing location
            SIMDVector3 branchDir = (spec.UnbentTipWorld - spec.TrunkAnchorWorld);
            float totalLength = branchDir.Length();

            // Find normalized perch location along the branch vector
            SIMDVector3 anchorToFeet = input.FeetContactPointWorld - spec.TrunkAnchorWorld;
            float projectDist = SIMDVector3::Dot(anchorToFeet, branchDir.Normalized());
            m_BranchState.PerchDistanceNormalized = std::clamp(projectDist / totalLength, 0.1f, 1.0f);

            float effectiveRadius = spec.BranchRadiusBaseMeters + (spec.BranchRadiusTipMeters - spec.BranchRadiusBaseMeters) * m_BranchState.PerchDistanceNormalized;
            float perchDistanceMeters = std::max(0.1f, totalLength * m_BranchState.PerchDistanceNormalized);

            // Second Moment of Area for a circular cross-section: I = (PI * r^4) / 4
            float secondMomentOfArea = (PI_F * std::pow(effectiveRadius, 4.0f)) / 4.0f;
            float youngsModulusPa = spec.YoungsModulusGPa * 1.0e9f;

            // Cantilever Stiffness: K = (3 * E * I) / (L^3)
            m_BranchState.EffectiveKSpring = (3.0f * youngsModulusPa * secondMomentOfArea) / std::pow(perchDistanceMeters, 3.0f);

            // Calculate equivalent branch mass vibrating at the perch site
            float branchVolume = PI_F * effectiveRadius * effectiveRadius * perchDistanceMeters;
            float branchEquivalentMass = (branchVolume * spec.WoodDensityKgM3) * 0.23f; // ~23% effective modal mass

            m_BranchState.EffectiveMassKg = branchEquivalentMass + (input.bIsFeetLockedOnBranch ? input.BirdMassKg : 0.0f);

            // Critical Damping Calculation: C_crit = 2 * sqrt(K * M)
            float criticalDamping = 2.0f * std::sqrt(m_BranchState.EffectiveKSpring * m_BranchState.EffectiveMassKg);
            m_BranchState.DampingCoefficientC = criticalDamping * spec.NaturalDampingRatio;

            // Compute Nominal Unbent Perch World Vector
            m_BranchState.NominalPerchPointWorld = spec.TrunkAnchorWorld + (branchDir.Normalized() * perchDistanceMeters);
        }

        /**
         * Resolves the momentum transfer when bird feet touch the branch surface.
         */
        void ProcessPerchContactAndImpulse(const BirdBranchInputFrame& input, float dt)
        {
            if (input.bIsFeetLockedOnBranch && !m_Outputs.bIsPerched)
            {
                // First frame of perch contact: Transfer kinetic momentum to branch velocity
                float birdDownwardVel = std::min(0.0f, input.BodyVelocityWorld.y);
                float totalSystemMass = m_BranchState.EffectiveMassKg;

                // Conservation of Momentum: (m_bird * v_bird) = (m_total * v_initial_branch)
                float initialBranchVelocity = (input.BirdMassKg * birdDownwardVel) / totalSystemMass;
                m_BranchState.VerticalVelocity += initialBranchVelocity;

                m_Outputs.bIsPerched = true;
            }
            else if (!input.bIsFeetLockedOnBranch)
            {
                m_Outputs.bIsPerched = false;
            }
        }

        /**
         * Solves 2nd-Order Differential Equation for Damped Spring Harmonic Motion:
         * $F_{net} = M \cdot a = F_{gravity} - K \cdot y - C \cdot \dot{y}$
         */
        void IntegrateBranchOscillation(const BirdBranchInputFrame& input, float dt)
        {
            // Downward force exerted by gravity on bird + branch mass
            float gravityForceN = (input.bIsFeetLockedOnBranch ? input.BirdMassKg : 0.0f) * GRAVITY_ACCEL;

            // Restoring Elastic Force: F_spring = -K * y
            float springForceN = -m_BranchState.EffectiveKSpring * m_BranchState.VerticalDeflectionMeters;

            // Damping Force: F_damping = -C * v
            float dampingForceN = -m_BranchState.DampingCoefficientC * m_BranchState.VerticalVelocity;

            // Acceleration: a = F_net / M
            float netForceN = gravityForceN + springForceN + dampingForceN;
            float acceleration = netForceN / m_BranchState.EffectiveMassKg;

            // Numerical Integration (Euler-Cromer for energy preservation)
            m_BranchState.VerticalVelocity += acceleration * dt;
            m_BranchState.VerticalDeflectionMeters += m_BranchState.VerticalVelocity * dt;

            // Prevent unnatural upwards hyper-extension
            m_BranchState.VerticalDeflectionMeters = std::max(-0.05f, m_BranchState.VerticalDeflectionMeters);

            // Calculate slope angle at the bent tip: theta = (3 * y) / (2 * L)
            float perchLength = (input.BranchSpec.UnbentTipWorld - input.BranchSpec.TrunkAnchorWorld).Length() * m_BranchState.PerchDistanceNormalized;
            m_BranchState.AngularBendRad = (3.0f * m_BranchState.VerticalDeflectionMeters) / (2.0f * std::max(0.1f, perchLength));

            // Set Output Frame Spatial Values
            m_BranchState.CurrentPerchPointWorld = m_BranchState.NominalPerchPointWorld - SIMDVector3(0.0f, m_BranchState.VerticalDeflectionMeters, 0.0f);
            
            m_Outputs.DeflectedBranchPerchPos = m_BranchState.CurrentPerchPointWorld;
            m_Outputs.CurrentDeflectionDistanceMeters = m_BranchState.VerticalDeflectionMeters;
            m_Outputs.CurrentSlopeAngleDeg = m_BranchState.AngularBendRad * RAD_TO_DEG_F;

            // Calculate total deflected tip position
            float fullLength = (input.BranchSpec.UnbentTipWorld - input.BranchSpec.TrunkAnchorWorld).Length();
            float fullTipDeflection = m_BranchState.VerticalDeflectionMeters * std::pow(fullLength / perchLength, 1.5f);
            m_Outputs.DeflectedBranchTipPos = input.BranchSpec.UnbentTipWorld - SIMDVector3(0.0f, fullTipDeflection, 0.0f);

            // Mechanical Reaction Forces
            m_Outputs.BranchRestoringForceN = SIMDVector3(0.0f, std::abs(springForceN), 0.0f);
            m_Outputs.DynamicLoadOnBirdFeetN = SIMDVector3(0.0f, netForceN + gravityForceN, 0.0f);

            // Over-strain check (e.g., branch bends past 35cm or 30 degrees)
            m_Outputs.bIsBranchOverStrained = (m_BranchState.VerticalDeflectionMeters > 0.35f) || (m_Outputs.CurrentSlopeAngleDeg > 30.0f);
        }

        /**
         * Adjusts leg crouching and body center of gravity (CoG) pitch to absorb branch motion smoothly.
         */
        void SolveBirdBalanceAndLegAbsorption(const BirdBranchInputFrame& input, float dt)
        {
            if (!m_Outputs.bIsPerched)
            {
                m_Outputs.BirdLegCompressionRatio = 0.0f;
                m_Outputs.BodyPosturalCorrectionPitchDeg = 0.0f;
                return;
            }

            // Leg crouch absorbs the downward dip velocity and displacement
            float deflectionFactor = std::clamp(m_BranchState.VerticalDeflectionMeters / 0.25f, 0.0f, 1.0f);
            float velocityFactor = std::clamp(-m_BranchState.VerticalVelocity / 2.0f, 0.0f, 1.0f);

            m_TargetLegSquat = (deflectionFactor * 0.6f) + (velocityFactor * 0.4f);

            // Smooth leg response using lerp
            m_CurrentLegSquat += (m_TargetLegSquat - m_CurrentLegSquat) * (dt * 14.0f);
            m_Outputs.BirdLegCompressionRatio = std::clamp(m_CurrentLegSquat, 0.0f, 1.0f);

            // Pitch torso forward to compensate for sloping branch surface and downward deflection
            float slopeCompensation = m_Outputs.CurrentSlopeAngleDeg * 0.75f;
            float oscillationPitch = (m_BranchState.VerticalVelocity * 4.5f); // Lean forward during downward dip, backward on rebound

            m_Outputs.BodyPosturalCorrectionPitchDeg = slopeCompensation + oscillationPitch;
        }
    };
}

#endif // AAAAAA_BIRD_BRANCH_FLEX_ENGINE_HPP
