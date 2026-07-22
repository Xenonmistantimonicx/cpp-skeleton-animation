#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAABirdTurningEngine.generated.h"

USTRUCT(BlueprintType)
struct FTailFeatherControlParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tail Control")
    float MaxTailRudderAngleDeg = 35.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tail Control")
    float MaxTailElevatorAngleDeg = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tail Control")
    float TailSurfaceAreaSqMeters = 0.22f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tail Control")
    float TailResponseRate = 12.0f;
};

USTRUCT(BlueprintType)
struct FAsymmetricWingTurningParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wing Asymmetry")
    float MaxAsymmetricFlapDifferential = 0.65f; // Power gap between left & right wing

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wing Asymmetry")
    float InnerWingTuckDragCoeff = 1.45f; // Extra drag on inner turning wing

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wing Asymmetry")
    float YawRollCouplingRatio = 0.75f; // Yaw naturally forces bank/roll

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wing Asymmetry")
    float RollYawCouplingRatio = 0.5f; // Roll naturally forces yaw/turn
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAABirdTurningEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAABirdTurningEngine()
    {
        PrimaryComponentTick.bCanEverTick = true;
        PrimaryComponentTick.TickGroup = TG_PrePhysics;

        AirDensity = 1.225f;
        WingspanMeters = 2.4f;
        SubStepCount = 8;

        TurnInputRaw = 0.0f;
        TurnInputFiltered = 0.0f;
        CurrentYawRateDegSec = 0.0f;
        CurrentTurnRadiusMeters = 0.0f;
        CurrentLeftWingPower = 1.0f;
        CurrentRightWingPower = 1.0f;

        CalculatedTurningTorqueWorld = FVector::ZeroVector;
        CalculatedTurningForceWorld = FVector::ZeroVector;
    }

protected:
    virtual void BeginPlay() override
    {
        Super::BeginPlay();

        OwningPawn = Cast<APawn>(GetOwner());
        if (OwningPawn)
        {
            RigidBodyMesh = Cast<UPrimitiveComponent>(OwningPawn->GetRootComponent());
            if (RigidBodyMesh && RigidBodyMesh->IsSimulatingPhysics())
            {
                RigidBodyMesh->SetEnableGravity(true);
                RigidBodyMesh->SetAngularDamping(0.2f);
            }
        }
    }

public:    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Atmosphere")
    float AirDensity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Geometry")
    float WingspanMeters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics Configuration")
    int32 SubStepCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Turning Dynamics")
    FTailFeatherControlParameters TailParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Turning Dynamics")
    FAsymmetricWingTurningParameters AsymmetricParams;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float TurnInputRaw;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float TurnInputFiltered;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentYawRateDegSec;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentTurnRadiusMeters;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentLeftWingPower;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentRightWingPower;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedTurningTorqueWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedTurningForceWorld;

    UFUNCTION(BlueprintCallable, Category = "AAA Turning Controls")
    void SetMidAirTurnInput(float TurnAmount)
    {
        TurnInputRaw = FMath::Clamp(TurnAmount, -1.0f, 1.0f); // -1.0 = Full Left, +1.0 = Full Right
    }

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
    {
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (!RigidBodyMesh || !RigidBodyMesh->IsSimulatingPhysics()) return;

        // Smooth control surface response
        TurnInputFiltered = FMath::FInterpTo(TurnInputFiltered, TurnInputRaw, DeltaTime, TailParams.TailResponseRate);

        const int32 Steps = FMath::Clamp(SubStepCount, 1, 16);
        const float SubstepDeltaTime = DeltaTime / static_cast<float>(Steps);

        for (int32 Step = 0; Step < Steps; ++Step)
        {
            EvaluateMidAirTurningPhysicsSubstep(SubstepDeltaTime);
        }
    }

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    void EvaluateMidAirTurningPhysicsSubstep(float SubstepDeltaTime)
    {
        FVector LinearVelocityWorld = RigidBodyMesh->GetLinearVelocity();
        float ForwardSpeed = LinearVelocityWorld.Size();

        if (ForwardSpeed < 0.1f)
        {
            CurrentYawRateDegSec = 0.0f;
            CurrentTurnRadiusMeters = 0.0f;
            CalculatedTurningTorqueWorld = FVector::ZeroVector;
            CalculatedTurningForceWorld = FVector::ZeroVector;
            return;
        }

        FVector ForwardVector = RigidBodyMesh->GetForwardVector();
        FVector RightVector = RigidBodyMesh->GetRightVector();
        FVector UpVector = RigidBodyMesh->GetUpVector();

        float DynamicPressure = 0.5f * AirDensity * (ForwardSpeed * ForwardSpeed);

        // 1. ASYMMETRIC WING POWER & DIFFERENTIAL DRAG
        // When turning left (TurnInput < 0), right wing thrust increases, inner wing drags
        if (TurnInputFiltered < 0.0f) // Left Turn
        {
            CurrentLeftWingPower = 1.0f - (FMath::Abs(TurnInputFiltered) * AsymmetricParams.MaxAsymmetricFlapDifferential);
            CurrentRightWingPower = 1.0f + (FMath::Abs(TurnInputFiltered) * 0.25f);
        }
        else // Right Turn
        {
            CurrentLeftWingPower = 1.0f + (FMath::Abs(TurnInputFiltered) * 0.25f);
            CurrentRightWingPower = 1.0f - (FMath::Abs(TurnInputFiltered) * AsymmetricParams.MaxAsymmetricFlapDifferential);
        }

        float WingThrustDifferential = (CurrentRightWingPower - CurrentLeftWingPower);
        float AsymmetricYawTorque = WingThrustDifferential * DynamicPressure * (WingspanMeters * 0.5f);

        // Inner Wing Tuck Drag Force
        float InnerWingDragForceScalar = FMath::Abs(TurnInputFiltered) * AsymmetricParams.InnerWingTuckDragCoeff * DynamicPressure;
        FVector InnerWingDragDirection = (TurnInputFiltered < 0.0f) ? -RightVector : RightVector;

        // 2. TAIL FEATHER DEFLECTION (RUDDER MECHANICS)
        float DeflectedTailRudderAngleRad = FMath::DegreesToRadians(TurnInputFiltered * TailParams.MaxTailRudderAngleDeg);
        float TailLiftCoeff = 2.0f * PI * DeflectedTailRudderAngleRad;
        float TailSideForceMagnitude = TailLiftCoeff * DynamicPressure * TailParams.TailSurfaceAreaSqMeters;

        FVector TailRudderForceWorld = -RightVector * TailSideForceMagnitude;
        float TailRudderYawTorque = TailSideForceMagnitude * (WingspanMeters * 0.6f);

        // 3. YAW-ROLL COUPLED BANKING MOMENT
        float CoupledRollTorque = TurnInputFiltered * AsymmetricParams.YawRollCouplingRatio * DynamicPressure * TailParams.TailSurfaceAreaSqMeters;

        // 4. ROTATIONAL YAW DAMPING
        FVector AngularVelocityRad = RigidBodyMesh->GetPhysicsAngularVelocityInRadians();
        FVector LocalAngularVelocity = RigidBodyMesh->GetComponentTransform().InverseTransformVector(AngularVelocityRad);

        float YawDampingTorque = -LocalAngularVelocity.Z * DynamicPressure * 2.5f;

        // Combine Local Moments
        FVector FinalLocalTorque = FVector(CoupledRollTorque, 0.0f, AsymmetricYawTorque + TailRudderYawTorque + YawDampingTorque);

        CalculatedTurningTorqueWorld = RigidBodyMesh->GetComponentTransform().TransformVector(FinalLocalTorque);
        CalculatedTurningForceWorld = TailRudderForceWorld + (InnerWingDragDirection * InnerWingDragForceScalar);

        // 5. APPLY PHYSICAL TURNING MOMENTS
        RigidBodyMesh->AddTorqueInRadians(CalculatedTurningTorqueWorld, NAME_None, true);
        RigidBodyMesh->AddForce(CalculatedTurningForceWorld, NAME_None, false);

        // 6. TELEMETRY COMPUTATIONS
        CurrentYawRateDegSec = FMath::RadiansToDegrees(LocalAngularVelocity.Z);

        if (FMath::Abs(CurrentYawRateDegSec) > 0.01f)
        {
            CurrentTurnRadiusMeters = ForwardSpeed / FMath::DegreesToRadians(FMath::Abs(CurrentYawRateDegSec));
        }
        else
        {
            CurrentTurnRadiusMeters = 9999.0f;
        }
    }
};
