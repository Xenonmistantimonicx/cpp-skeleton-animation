#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAABirdBankingEngine.generated.h"

USTRUCT(BlueprintType)
struct FAerodynamicControlSurfaceLimits
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control Surfaces")
    float MaxAileronDeflectionDegrees = 28.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control Surfaces")
    float MaxRudderDeflectionDegrees = 22.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control Surfaces")
    float ControlSurfaceResponseRate = 14.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control Surfaces")
    float AdverseYawFactor = 0.35f;
};

USTRUCT(BlueprintType)
struct FBankingPhysicsParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banking Mechanics")
    float RollTorqueCoefficient = 18.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banking Mechanics")
    float YawTorqueCoefficient = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banking Mechanics")
    float PitchTorqueCoefficient = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banking Mechanics")
    float MaxAutoBankAngleDegrees = 65.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banking Mechanics")
    float YawToRollAutoBankRatio = 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banking Mechanics")
    float DihedralRestoringMomentCoeff = 8.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banking Mechanics")
    float RollDampingCoefficient = 4.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banking Mechanics")
    float YawDampingCoefficient = 3.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banking Mechanics")
    float CentripetalForceMultiplier = 1.25f;
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAABirdBankingEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAABirdBankingEngine()
    {
        PrimaryComponentTick.bCanEverTick = true;
        PrimaryComponentTick.TickGroup = TG_PrePhysics;

        AirDensity = 1.225f;
        Wingspan = 2.4f;
        WingArea = 0.92f;
        SubStepCount = 8;

        CurrentRollAngleDeg = 0.0f;
        CurrentYawRateDegSec = 0.0f;
        CurrentTurnRadiusMeters = 0.0f;
        CurrentCentripetalForceG = 0.0f;

        CalculatedBankingTorqueWorld = FVector::ZeroVector;
        CalculatedCentripetalForceWorld = FVector::ZeroVector;
        CurrentControlDeflection = FVector::ZeroVector;
        FilteredControlDeflection = FVector::ZeroVector;
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
                RigidBodyMesh->SetAngularDamping(0.1f);
            }
        }
    }

public:    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Atmosphere")
    float AirDensity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Geometry")
    float Wingspan;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Geometry")
    float WingArea;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics Configuration")
    int32 SubStepCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics Configuration")
    FBankingPhysicsParameters BankingParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics Configuration")
    FAerodynamicControlSurfaceLimits SurfaceLimits;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentRollAngleDeg;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentYawRateDegSec;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentTurnRadiusMeters;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentCentripetalForceG;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedBankingTorqueWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedCentripetalForceWorld;

    UFUNCTION(BlueprintCallable, Category = "AAA Flight Input")
    void InjectSteeringInputs(float RollInput, float YawInput, float PitchInput)
    {
        CurrentControlDeflection.X = FMath::Clamp(RollInput, -1.0f, 1.0f);
        CurrentControlDeflection.Y = FMath::Clamp(YawInput, -1.0f, 1.0f);
        CurrentControlDeflection.Z = FMath::Clamp(PitchInput, -1.0f, 1.0f);
    }

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
    {
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (!RigidBodyMesh || !RigidBodyMesh->IsSimulatingPhysics()) return;

        FilteredControlDeflection = FMath::VInterpTo(FilteredControlDeflection, CurrentControlDeflection, DeltaTime, SurfaceLimits.ControlSurfaceResponseRate);

        const int32 Steps = FMath::Clamp(SubStepCount, 1, 16);
        const float SubstepDeltaTime = DeltaTime / static_cast<float>(Steps);

        for (int32 Step = 0; Step < Steps; ++Step)
        {
            EvaluateBankingPhysicsSubstep(SubstepDeltaTime);
        }
    }

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    FVector CurrentControlDeflection;
    FVector FilteredControlDeflection;

    void EvaluateBankingPhysicsSubstep(float SubstepDeltaTime)
    {
        FVector LinearVelocityWorld = RigidBodyMesh->GetLinearVelocity();
        float ForwardSpeed = LinearVelocityWorld.Size();

        if (ForwardSpeed < 0.1f)
        {
            CurrentRollAngleDeg = 0.0f;
            CurrentYawRateDegSec = 0.0f;
            CurrentTurnRadiusMeters = 0.0f;
            CurrentCentripetalForceG = 0.0f;
            CalculatedBankingTorqueWorld = FVector::ZeroVector;
            CalculatedCentripetalForceWorld = FVector::ZeroVector;
            return;
        }

        FVector ForwardVector = RigidBodyMesh->GetForwardVector();
        FVector RightVector = RigidBodyMesh->GetRightVector();
        FVector UpVector = RigidBodyMesh->GetUpVector();

        FVector LocalVelocity = RigidBodyMesh->GetComponentTransform().InverseTransformVector(LinearVelocityWorld);
        float DynamicPressure = 0.5f * AirDensity * (ForwardSpeed * ForwardSpeed);

        // 1. CALCULATE ROLL ANGLE & DIHEDRAL RESTORING MOMENT
        float ProjectRightOnUp = FVector::DotProduct(RightVector, FVector::UpVector);
        CurrentRollAngleDeg = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(ProjectRightOnUp, -1.0f, 1.0f)));

        float DihedralRestoringTorqueScalar = -FMath::DegreesToRadians(CurrentRollAngleDeg) * BankingParams.DihedralRestoringMomentCoeff * DynamicPressure;

        // 2. CONTROL TORQUE GENERATION (AILERON + RUDDER)
        float TargetRollDeflection = FilteredControlDeflection.X;
        float TargetYawDeflection = FilteredControlDeflection.Y;

        // Auto-Banking Coupling (Yaw input automatically generates coordinated Roll)
        if (FMath::Abs(TargetYawDeflection) > 0.05f && FMath::Abs(TargetRollDeflection) < 0.05f)
        {
            TargetRollDeflection = -TargetYawDeflection * BankingParams.YawToRollAutoBankRatio;
        }

        float RollTorque = TargetRollDeflection * BankingParams.RollTorqueCoefficient * DynamicPressure * SurfaceLimits.MaxAileronDeflectionDegrees;
        float YawTorque = TargetYawDeflection * BankingParams.YawTorqueCoefficient * DynamicPressure * SurfaceLimits.MaxRudderDeflectionDegrees;
        float PitchTorque = FilteredControlDeflection.Z * BankingParams.PitchTorqueCoefficient * DynamicPressure;

        // 3. ADVERSE YAW EFFECT (Differential wing drag causes opposite yaw when rolling)
        float AdverseYawTorque = -TargetRollDeflection * SurfaceLimits.AdverseYawFactor * DynamicPressure * SurfaceLimits.MaxAileronDeflectionDegrees;
        YawTorque += AdverseYawTorque;

        FVector LocalControlTorque = FVector(RollTorque + DihedralRestoringTorqueScalar, PitchTorque, YawTorque);

        // 4. AERODYNAMIC ROTATIONAL DAMPING
        FVector AngularVelocityRad = RigidBodyMesh->GetPhysicsAngularVelocityInRadians();
        FVector LocalAngularVelocity = RigidBodyMesh->GetComponentTransform().InverseTransformVector(AngularVelocityRad);

        FVector DampingTorque;
        DampingTorque.X = -LocalAngularVelocity.X * BankingParams.RollDampingCoefficient * DynamicPressure;
        DampingTorque.Y = -LocalAngularVelocity.Y * BankingParams.PitchTorqueCoefficient * 0.5f * DynamicPressure;
        DampingTorque.Z = -LocalAngularVelocity.Z * BankingParams.YawDampingCoefficient * DynamicPressure;

        FVector FinalLocalTorque = LocalControlTorque + DampingTorque;
        CalculatedBankingTorqueWorld = RigidBodyMesh->GetComponentTransform().TransformVector(FinalLocalTorque);

        // 5. CENTRIPETAL FORCE COMPUTATION (LIFT VECTOR TILTING)
        float RollAngleRads = FMath::DegreesToRadians(CurrentRollAngleDeg);
        float BankMagnitude = FMath::Abs(Sin(RollAngleRads));

        FVector HorizontalBankDirection = FVector::CrossProduct(ForwardVector, FVector::UpVector).GetSafeNormal();
        if (CurrentRollAngleDeg > 0.0f) HorizontalBankDirection = -HorizontalBankDirection;

        float CentripetalAcceleration = (ForwardSpeed * ForwardSpeed) * FMath::Tan(FMath::Abs(RollAngleRads)) / (Wingspan * 0.5f);
        float CentripetalForceMagnitude = RigidBodyMesh->GetMass() * CentripetalAcceleration * BankingParams.CentripetalForceMultiplier * BankMagnitude;

        CalculatedCentripetalForceWorld = HorizontalBankDirection * CentripetalForceMagnitude;

        // 6. APPLY PHYSICAL FORCES & TORQUES
        RigidBodyMesh->AddTorqueInRadians(CalculatedBankingTorqueWorld, NAME_None, true);
        RigidBodyMesh->AddForce(CalculatedCentripetalForceWorld, NAME_None, false);

        // 7. TELEMETRY COMPUTATIONS
        CurrentYawRateDegSec = FMath::RadiansToDegrees(LocalAngularVelocity.Z);
        if (FMath::Abs(CurrentYawRateDegSec) > 0.01f)
        {
            CurrentTurnRadiusMeters = ForwardSpeed / FMath::DegreesToRadians(FMath::Abs(CurrentYawRateDegSec));
        }
        else
        {
            CurrentTurnRadiusMeters = 9999.0f;
        }

        CurrentCentripetalForceG = CentripetalForceMagnitude / (RigidBodyMesh->GetMass() * 9.81f);
    }
};
