#include "AAABirdSpeedLiftEngine.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetMathLibrary.h"

UAAABirdSpeedLiftEngine::UAAABirdSpeedLiftEngine()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;

    AirDensityKgPerM3 = 1.225f;
    PhysicsSubStepsPerFrame = 8;
    SpeedToLiftMultiplier = 1.45f;
    PitchAoASensitivity = 1.2f;
    DynamicLiftDampingFactor = 0.42f;

    CurrentGroundSpeedKmh = 0.0f;
    CurrentAirSpeedMs = 0.0f;
    CurrentDynamicPressureQ = 0.0f;
    EffectiveLiftForceNewtons = 0.0f;
    CurrentAngleOfAttackDegrees = 0.0f;

    CalculatedLiftVectorWorld = FVector::ZeroVector;
    CalculatedInducedDragVectorWorld = FVector::ZeroVector;
    PreviousVelocity = FVector::ZeroVector;
    FilteredLiftForce = FVector::ZeroVector;
}

void UAAABirdSpeedLiftEngine::BeginPlay()
{
    Super::BeginPlay();

    OwningPawn = Cast<APawn>(GetOwner());
    if (OwningPawn)
    {
        RigidBodyMesh = Cast<UPrimitiveComponent>(OwningPawn->GetRootComponent());
        if (RigidBodyMesh && RigidBodyMesh->IsSimulatingPhysics())
        {
            RigidBodyMesh->SetEnableGravity(true);
            RigidBodyMesh->SetLinearDamping(0.005f);
            RigidBodyMesh->SetAngularDamping(0.2f);
        }
    }
}

void UAAABirdSpeedLiftEngine::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!RigidBodyMesh || !RigidBodyMesh->IsSimulatingPhysics()) return;

    const int32 ValidSubSteps = FMath::Clamp(PhysicsSubStepsPerFrame, 1, 16);
    const float SubstepDeltaTime = DeltaTime / static_cast<float>(ValidSubSteps);

    for (int32 StepIndex = 0; StepIndex < ValidSubSteps; ++StepIndex)
    {
        ExecuteAerodynamicSubstep(SubstepDeltaTime);
    }
}

void UAAABirdSpeedLiftEngine::ExecuteAerodynamicSubstep(float SubstepDeltaTime)
{
    FVector LinearVelocityWorld = RigidBodyMesh->GetLinearVelocity();
    CurrentAirSpeedMs = LinearVelocityWorld.Size();
    CurrentGroundSpeedKmh = CurrentAirSpeedMs * 3.6f;

    if (CurrentAirSpeedMs < 0.05f)
    {
        EffectiveLiftForceNewtons = 0.0f;
        CalculatedLiftVectorWorld = FVector::ZeroVector;
        CalculatedInducedDragVectorWorld = FVector::ZeroVector;
        return;
    }

    FVector ForwardVector = RigidBodyMesh->GetForwardVector();
    FVector UpVector = RigidBodyMesh->GetUpVector();
    FVector RightVector = RigidBodyMesh->GetRightVector();
    FVector VelocityNormalized = LinearVelocityWorld.GetSafeNormal();

    float ForwardVelocityProjection = FVector::DotProduct(VelocityNormalized, ForwardVector);
    float UpVelocityProjection = FVector::DotProduct(VelocityNormalized, UpVector);

    CurrentAngleOfAttackDegrees = FMath::RadiansToDegrees(FMath::Atan2(-UpVelocityProjection, ForwardVelocityProjection)) * PitchAoASensitivity;
    float AoARadians = FMath::DegreesToRadians(CurrentAngleOfAttackDegrees);

    // Bernoulli Dynamic Pressure: q = 0.5 * rho * v^2
    CurrentDynamicPressureQ = 0.5f * AirDensityKgPerM3 * FMath::Square(CurrentAirSpeedMs);

    float LiftCoeff = ComputeLiftCoefficient(AoARadians);
    float InducedDragCoeff = ComputeInducedDragCoefficient(LiftCoeff);

    float RawLiftMagnitude = CurrentDynamicPressureQ * WingProfile.SurfaceAreaSquareMeters * LiftCoeff * SpeedToLiftMultiplier;
    EffectiveLiftForceNewtons = FMath::Clamp(RawLiftMagnitude, 0.0f, WingProfile.MaxDynamicLiftThreshold);

    float InducedDragMagnitude = CurrentDynamicPressureQ * WingProfile.SurfaceAreaSquareMeters * InducedDragCoeff;

    FVector IdealLiftDirection = FVector::CrossProduct(VelocityNormalized, RightVector).GetSafeNormal();
    if (IdealLiftDirection.IsNearlyZero())
    {
        IdealLiftDirection = UpVector;
    }

    FVector TargetLiftVector = IdealLiftDirection * EffectiveLiftForceNewtons;
    FilteredLiftForce = FMath::VInterpTo(FilteredLiftForce, TargetLiftVector, SubstepDeltaTime, 1.0f / DynamicLiftDampingFactor);
    CalculatedLiftVectorWorld = FilteredLiftForce;

    CalculatedInducedDragVectorWorld = -VelocityNormalized * InducedDragMagnitude;

    ApplyDynamicForces(CalculatedLiftVectorWorld, CalculatedInducedDragVectorWorld);
}

float UAAABirdSpeedLiftEngine::ComputeLiftCoefficient(float AngleOfAttackRads) const
{
    float CriticalStallRads = FMath::DegreesToRadians(16.0f);

    if (FMath::Abs(AngleOfAttackRads) <= CriticalStallRads)
    {
        return WingProfile.ZeroAlphaLiftCoefficient + (WingProfile.LiftCurveSlopePerRad * AngleOfAttackRads);
    }
    
    float StallSign = FMath::Sign(AngleOfAttackRads);
    float PostStallCos = FMath::Cos(AngleOfAttackRads);
    float MaxLinearLift = WingProfile.ZeroAlphaLiftCoefficient + (WingProfile.LiftCurveSlopePerRad * CriticalStallRads);
    
    return StallSign * MaxLinearLift * FMath::Square(PostStallCos);
}

float UAAABirdSpeedLiftEngine::ComputeInducedDragCoefficient(float LiftCoefficient) const
{
    if (WingProfile.SurfaceAreaSquareMeters <= 0.0f) return 0.0f;

    float AspectRatio = FMath::Square(WingProfile.WingspanMeters) / WingProfile.SurfaceAreaSquareMeters;
    float EfficiencyTerm = PI * AspectRatio * WingProfile.OswaldEfficiencyFactor;

    if (EfficiencyTerm <= 0.0f) return 0.0f;

    return FMath::Square(LiftCoefficient) / EfficiencyTerm;
}

void UAAABirdSpeedLiftEngine::ApplyDynamicForces(const FVector& LiftVector, const FVector& DragVector)
{
    FVector TotalAerodynamicForce = LiftVector + DragVector;
    RigidBodyMesh->AddForce(TotalAerodynamicForce, NAME_None, false);
}
