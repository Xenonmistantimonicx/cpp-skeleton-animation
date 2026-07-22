#include "AAABirdFlightComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetMathLibrary.h"

UAAABirdFlightComponent::UAAABirdFlightComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;

    CurrentFlightState = EBirdFlightState::Gliding;
    ControlInputVector = FVector::ZeroVector;
    FlapImpulseQueue = 0.0f;
    LastFlapTime = 0.0f;
    CurrentSpeedKmh = 0.0f;
    AngleOfAttackDeg = 0.0f;
    DynamicNormalizedBanking = 0.0f;
}

void UAAABirdFlightComponent::BeginPlay()
{
    Super::BeginPlay();

    OwningPawn = Cast<APawn>(GetOwner());
    if (OwningPawn)
    {
        PhysicsMesh = Cast<UPrimitiveComponent>(OwningPawn->GetRootComponent());
        if (PhysicsMesh && PhysicsMesh->IsSimulatingPhysics())
        {
            PhysicsMesh->SetMassOverrideScale(NAME_None, Mass);
            PhysicsMesh->SetEnableGravity(true);
            PhysicsMesh->SetLinearDamping(0.01f);
            PhysicsMesh->SetAngularDamping(0.5f);
        }
    }
}

void UAAABirdFlightComponent::InjectFlightInputs(float PitchInput, float YawInput, float RollInput, float FlapThrustInput)
{
    ControlInputVector.X = FMath::Clamp(PitchInput, -1.0f, 1.0f);
    ControlInputVector.Y = FMath::Clamp(YawInput, -1.0f, 1.0f);
    ControlInputVector.Z = FMath::Clamp(RollInput, -1.0f, 1.0f);

    if (FlapThrustInput > 0.1f)
    {
        TriggerFlapImpulse();
    }
}

void UAAABirdFlightComponent::TriggerFlapImpulse()
{
    const float CurrentTime = GetWorld()->GetTimeSeconds();
    const float MinFlapInterval = 1.0f / FlapFrequencyHz;

    if (CurrentTime - LastFlapTime >= MinFlapInterval)
    {
        FlapImpulseQueue += 1.0f;
        LastFlapTime = CurrentTime;
    }
}

void UAAABirdFlightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!PhysicsMesh || !PhysicsMesh->IsSimulatingPhysics()) return;

    const int32 SubSteps = 4;
    const float SubstepDeltaTime = DeltaTime / static_cast<float>(SubSteps);

    for (int32 i = 0; i < SubSteps; ++i)
    {
        EvaluatePhysicsSubstep(SubstepDeltaTime);
    }
}

void UAAABirdFlightComponent::EvaluatePhysicsSubstep(float SubstepDeltaTime)
{
    FVector WorldVelocity = PhysicsMesh->GetLinearVelocity();
    FVector ForwardVector = PhysicsMesh->GetForwardVector();
    FVector UpVector = PhysicsMesh->GetUpVector();
    FVector RightVector = PhysicsMesh->GetRightVector();

    float Speed = WorldVelocity.Size();
    CurrentSpeedKmh = Speed * 3.6f;

    FVector LocalVelocity = PhysicsMesh->GetComponentTransform().InverseTransformVector(WorldVelocity);

    if (Speed > 0.5f)
    {
        FVector NormalizedVel = WorldVelocity.GetSafeNormal();
        float ForwardDot = FVector::DotProduct(NormalizedVel, ForwardVector);
        float UpDot = FVector::DotProduct(NormalizedVel, UpVector);
        AngleOfAttackDeg = FMath::RadiansToDegrees(FMath::Atan2(-UpDot, ForwardDot));
    }
    else
    {
        AngleOfAttackDeg = 0.0f;
    }

    float AlphaRads = FMath::DegreesToRadians(AngleOfAttackDeg);
    float DynamicPressure = 0.5f * AirDensity * (Speed * Speed);

    float CL = CalculateLiftCoefficient(AlphaRads);
    float CD = CalculateDragCoefficient(CL, AlphaRads);

    float LiftMagnitude = DynamicPressure * WingArea * CL;
    float DragMagnitude = DynamicPressure * WingArea * CD;

    FVector LiftDirection = FVector::CrossProduct(WorldVelocity.GetSafeNormal(), RightVector).GetSafeNormal();
    if (LiftDirection.IsNearlyZero()) { LiftDirection = UpVector; }

    ComputedLiftForce = LiftDirection * LiftMagnitude;
    ComputedDragForce = -WorldVelocity.GetSafeNormal() * DragMagnitude;

    PhysicsMesh->AddForce(ComputedLiftForce, NAME_None, false);
    PhysicsMesh->AddForce(ComputedDragForce, NAME_None, false);

    if (FlapImpulseQueue > 0.0f)
    {
        FVector ThrustVector = ForwardVector * MaxFlapThrustForce;
        FVector FlapLiftImpulse = UpVector * (MaxFlapThrustForce * 0.35f);

        PhysicsMesh->AddImpulse(ThrustVector + FlapLiftImpulse, NAME_None, true);
        FlapImpulseQueue -= 1.0f;
    }

    ApplyDynamicControlTorques(SubstepDeltaTime, LocalVelocity);
    UpdateStateTelemetry(LocalVelocity);
}

float UAAABirdFlightComponent::CalculateLiftCoefficient(float AlphaRads) const
{
    float CriticalStallRads = FMath::DegreesToRadians(CriticalStallAngleDeg);

    if (FMath::Abs(AlphaRads) <= CriticalStallRads)
    {
        return ZeroAoALiftCoefficient + (LiftSlope * AlphaRads);
    }
    else
    {
        float Sign = FMath::Sign(AlphaRads);
        float StallFactor = FMath::Cos(AlphaRads);
        return Sign * (ZeroAoALiftCoefficient + (LiftSlope * CriticalStallRads)) * (StallFactor * StallFactor);
    }
}

float UAAABirdFlightComponent::CalculateDragCoefficient(float LiftCoeff, float AlphaRads) const
{
    float AspectRatio = (Wingspan * Wingspan) / WingArea;
    float OswaldEfficiency = 0.82f;

    float InducedDragCoeff = (LiftCoeff * LiftCoeff) / (PI * AspectRatio * OswaldEfficiency);

    float ExtraStallDrag = 0.0f;
    float CriticalStallRads = FMath::DegreesToRadians(CriticalStallAngleDeg);
    if (FMath::Abs(AlphaRads) > CriticalStallRads)
    {
        ExtraStallDrag = FMath::Square(FMath::Sin(AlphaRads - CriticalStallRads)) * 1.5f;
    }

    return ParasiticDragCoefficient + InducedDragCoeff + ExtraStallDrag;
}

void UAAABirdFlightComponent::ApplyDynamicControlTorques(float SubstepDeltaTime, const FVector& LocalVelocity)
{
    float SpeedRatio = FMath::Clamp(LocalVelocity.X / 15.0f, 0.0f, 1.0f);

    FVector TargetTorque;
    TargetTorque.X = ControlInputVector.X * PitchYawRollTorqueSensitivity.X;
    TargetTorque.Y = ControlInputVector.Y * PitchYawRollTorqueSensitivity.Y;
    TargetTorque.Z = ControlInputVector.Z * PitchYawRollTorqueSensitivity.Z;

    if (FMath::Abs(ControlInputVector.Y) > 0.1f && FMath::Abs(ControlInputVector.Z) < 0.1f)
    {
        TargetTorque.Z = -ControlInputVector.Y * (PitchYawRollTorqueSensitivity.Z * 0.75f);
    }

    FVector WorldTorque = PhysicsMesh->GetComponentTransform().TransformVector(TargetTorque * SpeedRatio);

    PhysicsMesh->AddTorqueInRadians(WorldTorque, NAME_None, true);

    FVector AngularVel = PhysicsMesh->GetPhysicsAngularVelocityInRadians();
    FVector DampingTorque = -AngularVel * DampingFactor * SpeedRatio;
    PhysicsMesh->AddTorqueInRadians(DampingTorque, NAME_None, true);
}

void UAAABirdFlightComponent::UpdateStateTelemetry(const FVector& LocalVelocity)
{
    float TargetBank = FVector::DotProduct(PhysicsMesh->GetRightVector(), FVector::UpVector);
    DynamicNormalizedBanking = FMath::FInterpTo(DynamicNormalizedBanking, TargetBank, GetWorld()->GetDeltaSeconds(), 6.0f);

    if (AngleOfAttackDeg > CriticalStallAngleDeg)
    {
        CurrentFlightState = EBirdFlightState::Stalled;
    }
    else if (LocalVelocity.Z < -15.0f && LocalVelocity.X > 20.0f)
    {
        CurrentFlightState = EBirdFlightState::Diving;
    }
    else if (FlapImpulseQueue > 0.0f)
    {
        CurrentFlightState = EBirdFlightState::Flapping;
    }
    else
    {
        CurrentFlightState = EBirdFlightState::Gliding;
    }
}
