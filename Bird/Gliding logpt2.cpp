// Copyright AAA Studios, 2026. All Rights Reserved.

#include "AAAUnifiedEnergyRecoveryEngine.h"

UAAAUnifiedEnergyRecoveryEngine::UAAAUnifiedEnergyRecoveryEngine()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;

    SubStepCount = 8;
    AirDensityStandard = 1.225f;

    BirdMassKg = 3.5f; // 3.5 kg Large Soaring Eagle / Albatross
    WingspanMeters = 2.4f;
    WingAreaSqMeters = 0.65f;

    PitchInput = 0.0f;
    RollInput = 0.0f;
    bEngageOptimalGlideMode = true;

    BaselineGroundAltitudeMeters = 0.0;
    PreviousTotalEnergyJoules = 0.0;

    CalculatedAerodynamicForceWorld = FVector::ZeroVector;
    CalculatedControlTorqueWorld = FVector::ZeroVector;
}

void UAAAUnifiedEnergyRecoveryEngine::BeginPlay()
{
    Super::BeginPlay();

    OwningPawn = Cast<APawn>(GetOwner());
    if (OwningPawn)
    {
        RigidBodyMesh = Cast<UPrimitiveComponent>(OwningPawn->GetRootComponent());
        if (RigidBodyMesh && RigidBodyMesh->IsSimulatingPhysics())
        {
            RigidBodyMesh->SetEnableGravity(true);
            RigidBodyMesh->SetMassOverrideOrDefault(NAME_None, BirdMassKg, true);
            BaselineGroundAltitudeMeters = RigidBodyMesh->GetComponentLocation().Z * 0.01;
        }
    }
}

void UAAAUnifiedEnergyRecoveryEngine::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!RigidBodyMesh || !RigidBodyMesh->IsSimulatingPhysics()) return;

    const int32 Steps = FMath::Clamp(SubStepCount, 1, 16);
    const float SubstepDeltaTime = DeltaTime / static_cast<float>(Steps);

    for (int32 Step = 0; Step < Steps; ++Step)
    {
        EvaluateEnergyRecoverySubstep(SubstepDeltaTime);
    }
}

float UAAAUnifiedEnergyRecoveryEngine::CalculateUpdraftVelocityField(const FVector& CurrentLocationWorld, float& OutThermalProximity)
{
    OutThermalProximity = 0.0f;

    // 1. Thermal Core Model (Gaussian Profile)
    FVector HorizontalDistanceVec = CurrentLocationWorld - Atmosphere.ThermalCenterWorld;
    HorizontalDistanceVec.Z = 0.0f; // Radial distance on horizontal plane
    
    float RadialDistanceMeters = HorizontalDistanceVec.Size() * 0.01f; // UE cm to meters
    float GaussianFactor = FMath::Exp(-FMath::Square(RadialDistanceMeters / Atmosphere.ThermalRadiusMeters));
    
    float ThermalUpdraftSpeed = Atmosphere.ThermalCoreUpdraftMs * GaussianFactor;
    OutThermalProximity = FMath::Clamp(GaussianFactor, 0.0f, 1.0f);

    // 2. Orographic Ridge Updraft Model (Terrain Line Trace)
    FHitResult GroundHit;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());

    float RidgeUpdraftSpeed = 0.0f;
    if (GetWorld()->LineTraceSingleByChannel(GroundHit, CurrentLocationWorld, CurrentLocationWorld - FVector(0, 0, 50000.0f), ECC_Visibility, QueryParams))
    {
        float AltitudeAboveGroundM = (CurrentLocationWorld.Z - GroundHit.ImpactPoint.Z) * 0.01f;
        FVector SlopeNormal = GroundHit.ImpactNormal;

        // Updraft scales with terrain slope inclination and proximity to ridge shoulder
        if (SlopeNormal.Z < 0.85f && AltitudeAboveGroundM < 150.0f)
        {
            float SlopeInclination = 1.0f - SlopeNormal.Z;
            float AltitudeDecay = FMath::Clamp(1.0f - (AltitudeAboveGroundM / 150.0f), 0.0f, 1.0f);
            RidgeUpdraftSpeed = Atmosphere.RidgeUpdraftSpeedMs * SlopeInclination * AltitudeDecay;
        }
    }

    return ThermalUpdraftSpeed + RidgeUpdraftSpeed;
}

float UAAAUnifiedEnergyRecoveryEngine::CalculateDynamicWindShearVector(const FVector& CurrentLocationWorld, FVector& OutWindVelocityWorld)
{
    float AltitudeAGLMeters = FMath::Max(0.0f, static_cast<float>((CurrentLocationWorld.Z * 0.01) - BaselineGroundAltitudeMeters));

    // Boundary Layer Wind Shear ($V(z) = V_{ref} \cdot \frac{z}{z_0}$)
    if (AltitudeAGLMeters < Atmosphere.BoundaryLayerHeightMeters)
    {
        float WindMagnitudeMs = Atmosphere.WindShearGradient * AltitudeAGLMeters * 10.0f;
        OutWindVelocityWorld = FVector(1.0f, 0.0f, 0.0f) * WindMagnitudeMs; // Directional shear horizontal vector
        return WindMagnitudeMs;
    }

    OutWindVelocityWorld = FVector(1.0f, 0.0f, 0.0f) * (Atmosphere.WindShearGradient * Atmosphere.BoundaryLayerHeightMeters * 10.0f);
    return OutWindVelocityWorld.Size();
}

void UAAAUnifiedEnergyRecoveryEngine::EvaluateEnergyRecoverySubstep(float SubstepDeltaTime)
{
    FTransform ActorTransform = RigidBodyMesh->GetComponentTransform();
    FVector ActorLocation = ActorTransform.GetLocation();
    FVector GroundRelativeVelocity = RigidBodyMesh->GetLinearVelocity();
    FVector ForwardVector = ActorTransform.GetRotation().GetForwardVector();
    FVector UpVector = ActorTransform.GetRotation().GetUpVector();

    // =========================================================================
    // 1. ATMOSPHERIC ENERGY FIELD SAMPLING
    // =========================================================================
    FVector EnvironmentWindVelocityWorld = FVector::ZeroVector;
    CalculateDynamicWindShearVector(ActorLocation, EnvironmentWindVelocityWorld);

    float VerticalUpdraftMs = CalculateUpdraftVelocityField(ActorLocation, EnergyTelemetry.ThermalCoreProximity);
    FVector TotalEnvironmentAirMotionWorld = EnvironmentWindVelocityWorld + FVector(0.0f, 0.0f, VerticalUpdraftMs * 100.0f); // convert m/s to cm/s

    // True Airspeed Vector ($\vec{V}_{apparent} = \vec{V}_{ground} - \vec{V}_{wind}$)
    FVector ApparentAirspeedVector = GroundRelativeVelocity - TotalEnvironmentAirMotionWorld;
    float ApparentAirspeedCmS = ApparentAirspeedVector.Size();
    float ApparentAirspeedMs = ApparentAirspeedCmS * 0.01f;

    FVector VelocityNormalized = (ApparentAirspeedMs > 0.05f) ? (ApparentAirspeedVector / ApparentAirspeedCmS) : ForwardVector;

    // =========================================================================
    // 2. AERODYNAMIC OPTIMIZATION (HIGH GLIDE RATIO $L/D$)
    // =========================================================================
    // Optimal Glide Mode locks angle of attack at maximum Lift-to-Drag efficiency ($\left(\frac{L}{D}\right)_{max}$)
    float AspectRatio = FMath::Square(WingspanMeters) / WingAreaSqMeters;
    float OswaldEfficiency = 0.92f;

    float OptLiftCoeff = bEngageOptimalGlideMode ? FMath::Sqrt(PI * AspectRatio * OswaldEfficiency * 0.025f) : 0.65f;
    float ParasiticDragCoeff = 0.022f;
    float InducedDragCoeff = FMath::Square(OptLiftCoeff) / (PI * AspectRatio * OswaldEfficiency);
    float TotalDragCoeff = ParasiticDragCoeff + InducedDragCoeff;

    // Dynamic Pressure ($q = \frac{1}{2} \rho V^2$)
    float DynamicPressure = 0.5f * AirDensityStandard * FMath::Square(ApparentAirspeedMs);

    float LiftForceN = DynamicPressure * WingAreaSqMeters * OptLiftCoeff;
    float DragForceN = DynamicPressure * WingAreaSqMeters * TotalDragCoeff;

    // Lift & Drag Vectoring
    FVector LiftDirection = FVector::CrossProduct(FVector::CrossProduct(VelocityNormalized, UpVector), VelocityNormalized).GetSafeNormal();
    if (LiftDirection.IsNearlyZero()) LiftDirection = UpVector;

    FVector LiftForceWorld = LiftDirection * LiftForceN;
    FVector DragForceWorld = -VelocityNormalized * DragForceN;

    CalculatedAerodynamicForceWorld = LiftForceWorld + DragForceWorld;

    // =========================================================================
    // 3. MECHANICAL ENERGY & STAMINA RECOVERY CALCULATIONS
    // =========================================================================
    const double GravityAcc = 9.81;
    double CurrentAltitudeMeters = ActorLocation.Z * 0.01;
    double GroundSpeedMs = GroundRelativeVelocity.Size() * 0.01;

    EnergyTelemetry.KineticEnergyJoules = 0.5 * static_cast<double>(BirdMassKg) * FMath::Square(GroundSpeedMs);
    EnergyTelemetry.PotentialEnergyJoules = static_cast<double>(BirdMassKg) * GravityAcc * (CurrentAltitudeMeters - BaselineGroundAltitudeMeters);
    
    double CurrentTotalEnergyJoules = EnergyTelemetry.KineticEnergyJoules + EnergyTelemetry.PotentialEnergyJoules;
    EnergyTelemetry.TotalMechanicalEnergyJoules = CurrentTotalEnergyJoules;

    // Net Power Gain/Loss ($\Delta E / \Delta t$)
    if (PreviousTotalEnergyJoules > 0.0)
    {
        double EnergyDeltaJoules = CurrentTotalEnergyJoules - PreviousTotalEnergyJoules;
        EnergyTelemetry.NetMechanicalPowerHarvestedWatts = static_cast<float>(EnergyDeltaJoules / static_cast<double>(SubstepDeltaTime));
    }
    PreviousTotalEnergyJoules = CurrentTotalEnergyJoules;

    // Passive Metabolic Recovery during Glide
    if (EnergyTelemetry.NetMechanicalPowerHarvestedWatts > 0.0f || VerticalUpdraftMs > 1.0f)
    {
        // Regenerate stamina when soaring inside lift fields
        EnergyTelemetry.MetabolicStaminaPercent = FMath::Clamp(EnergyTelemetry.MetabolicStaminaPercent + (4.0f * SubstepDeltaTime), 0.0f, 100.0f);
    }

    // =========================================================================
    // 4. LOW-INERTIA ATTITUDE CONTROL TORQUES
    // =========================================================================
    float MomentOfInertiaX = 0.12f * BirdMassKg * FMath::Square(WingspanMeters * 0.5f);
    float MomentOfInertiaY = 0.08f * BirdMassKg * FMath::Square(0.2f);

    float ControlRollTorque = RollInput * 45.0f * MomentOfInertiaX;
    float ControlPitchTorque = PitchInput * 45.0f * MomentOfInertiaY;

    // Aerodynamic Angular Damping
    FVector AngularVelocity = RigidBodyMesh->GetAngularVelocityInRadians();
    FVector DampingTorque = -AngularVelocity * DynamicPressure * WingAreaSqMeters * WingspanMeters * 0.2f;

    CalculatedControlTorqueWorld = (ForwardVector * ControlRollTorque) + (ActorTransform.GetRotation().GetRightVector() * ControlPitchTorque) + DampingTorque;

    // =========================================================================
    // 5. APPLY PHYSICAL FORCES TO RIGID BODY
    // =========================================================================
    RigidBodyMesh->AddForce(CalculatedAerodynamicForceWorld, NAME_None, false);
    RigidBodyMesh->AddTorqueInRadians(CalculatedControlTorqueWorld, NAME_None, true);
}
