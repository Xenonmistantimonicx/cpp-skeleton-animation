#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAABirdGlidingEngine.generated.h"

USTRUCT(BlueprintType)
struct FGlidingPolarParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airfoil Polar")
    float ZeroLiftDragCoefficient = 0.022f; // Parasite Drag ($C_{D0}$)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airfoil Polar")
    float InducedDragFactor = 0.045f; // Induced Drag Multiplier ($k = \frac{1}{\pi e AR}$)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airfoil Polar")
    float MaxLiftCoefficient = 1.65f; // $C_{L,max}$ before stall

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airfoil Polar")
    float StallAngleDegrees = 16.5f; // Critical Angle of Attack ($\alpha_{crit}$)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airfoil Polar")
    float PostStallDragMultiplier = 3.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airfoil Polar")
    float AspectRatio = 9.8f; // Wing Aspect Ratio ($AR = \frac{b^2}{S}$)
};

USTRUCT(BlueprintType)
struct FWingMorphologyState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wing Morphology")
    float FullyExtendedWingspanMeters = 2.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wing Morphology")
    float FullyTuckedWingspanMeters = 0.95f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wing Morphology")
    float ExtendedWingAreaSqMeters = 0.92f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wing Morphology")
    float TuckedWingAreaSqMeters = 0.38f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wing Morphology")
    float MorphResponseRate = 6.0f; // Speed of wing folding/extending
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAABirdGlidingEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAABirdGlidingEngine()
    {
        PrimaryComponentTick.bCanEverTick = true;
        PrimaryComponentTick.TickGroup = TG_PrePhysics;

        AirDensity = 1.225f;
        SubStepCount = 8;

        CurrentWingTuckRatio = 0.0f; // 0.0 = Fully Extended (Max Glide), 1.0 = Fully Tucked (Dive)
        TargetWingTuckRatio = 0.0f;
        CurrentAngleOfAttackDeg = 0.0f;
        CurrentSideslipAngleDeg = 0.0f;
        CurrentGlideRatio = 0.0f;
        bIsStalled = false;

        ThermalUpdraftVelocityWorld = FVector::ZeroVector;
        CalculatedGlideLiftWorld = FVector::ZeroVector;
        CalculatedGlideDragWorld = FVector::ZeroVector;
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
                RigidBodyMesh->SetLinearDamping(0.01f); // Managed manually by aerodynamic drag
            }
        }
    }

public:    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Atmosphere")
    float AirDensity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics Configuration")
    int32 SubStepCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Gliding Physics")
    FGlidingPolarParameters PolarParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Gliding Physics")
    FWingMorphologyState Morphology;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentWingTuckRatio;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentAngleOfAttackDeg;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentSideslipAngleDeg;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentGlideRatio; // $L/D$ Ratio

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    bool bIsStalled;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedGlideLiftWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedGlideDragWorld;

    UFUNCTION(BlueprintCallable, Category = "AAA Gliding Controls")
    void SetWingTuckInput(float TuckAmount)
    {
        TargetWingTuckRatio = FMath::Clamp(TuckAmount, 0.0f, 1.0f);
    }

    UFUNCTION(BlueprintCallable, Category = "AAA Environment Interaction")
    void SetThermalUpdraftVector(FVector UpdraftVelocity)
    {
        ThermalUpdraftVelocityWorld = UpdraftVelocity;
    }

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
    {
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (!RigidBodyMesh || !RigidBodyMesh->IsSimulatingPhysics()) return;

        // Smoothly morph wings between fully extended (glide) and tucked (dive)
        CurrentWingTuckRatio = FMath::FInterpTo(CurrentWingTuckRatio, TargetWingTuckRatio, DeltaTime, Morphology.MorphResponseRate);

        const int32 Steps = FMath::Clamp(SubStepCount, 1, 16);
        const float SubstepDeltaTime = DeltaTime / static_cast<float>(Steps);

        for (int32 Step = 0; Step < Steps; ++Step)
        {
            EvaluatePureGlidingPhysicsSubstep(SubstepDeltaTime);
        }
    }

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    float TargetWingTuckRatio;
    FVector ThermalUpdraftVelocityWorld;

    void EvaluatePureGlidingPhysicsSubstep(float SubstepDeltaTime)
    {
        // 1. RELATIVE WIND COMPUTATION (Velocity + Thermal / Wind Updrafts)
        FVector BodyLinearVelocity = RigidBodyMesh->GetLinearVelocity();
        FVector RelativeWindVelocityWorld = BodyLinearVelocity - ThermalUpdraftVelocityWorld;
        
        float Airspeed = RelativeWindVelocityWorld.Size();
        if (Airspeed < 0.1f)
        {
            CurrentGlideRatio = 0.0f;
            CurrentAngleOfAttackDeg = 0.0f;
            bIsStalled = false;
            CalculatedGlideLiftWorld = FVector::ZeroVector;
            CalculatedGlideDragWorld = FVector::ZeroVector;
            return;
        }

        FVector RelativeWindDirection = RelativeWindVelocityWorld.GetUnsafeNormal();

        FVector ForwardVector = RigidBodyMesh->GetForwardVector();
        FVector UpVector = RigidBodyMesh->GetUpVector();
        FVector RightVector = RigidBodyMesh->GetRightVector();

        // 2. MORPHOLOGY CALCULATION (Dynamic Wingspan & Surface Area)
        float ActiveWingspan = FMath::Lerp(Morphology.FullyExtendedWingspanMeters, Morphology.FullyTuckedWingspanMeters, CurrentWingTuckRatio);
        float ActiveWingArea = FMath::Lerp(Morphology.ExtendedWingAreaSqMeters, Morphology.TuckedWingAreaSqMeters, CurrentWingTuckRatio);

        // 3. ANGLE OF ATTACK ($\alpha$) & SIDESLIP ($\beta$)
        float ForwardProjectedSpeed = FVector::DotProduct(RelativeWindDirection, ForwardVector);
        float UpProjectedSpeed = FVector::DotProduct(RelativeWindDirection, UpVector);
        float RightProjectedSpeed = FVector::DotProduct(RelativeWindDirection, RightVector);

        CurrentAngleOfAttackDeg = FMath::RadiansToDegrees(FMath::Atan2(-UpProjectedSpeed, ForwardProjectedSpeed));
        CurrentSideslipAngleDeg = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(RightProjectedSpeed, -1.0f, 1.0f)));

        // 4. LIFT AND DRAG COEFFICIENTS ($C_L$ and $C_D$ Curve Calculations)
        float LiftCoefficient = 0.0f;
        float DragCoefficient = 0.0f;

        float AlphaRads = FMath::DegreesToRadians(CurrentAngleOfAttackDeg);
        float StallAlphaRads = FMath::DegreesToRadians(PolarParams.StallAngleDegrees);

        if (CurrentAngleOfAttackDeg <= PolarParams.StallAngleDegrees)
        {
            // Linear Lift Regime: $C_L = 2\pi \cdot \alpha$
            bIsStalled = false;
            LiftCoefficient = 2.0f * PI * AlphaRads;
            LiftCoefficient = FMath::Clamp(LiftCoefficient, -PolarParams.MaxLiftCoefficient, PolarParams.MaxLiftCoefficient);
        }
        else
        {
            // Post-Stall Regime: Flow Separation Loss
            bIsStalled = true;
            float StallExcess = CurrentAngleOfAttackDeg - PolarParams.StallAngleDegrees;
            float DropoffFactor = FMath::Exp(-StallExcess * 0.15f);
            LiftCoefficient = PolarParams.MaxLiftCoefficient * DropoffFactor * FMath::Cos(FMath::DegreesToRadians(StallExcess));
        }

        // Induced Drag: $C_{Di} = k \cdot C_L^2$
        float InducedDragCoeff = PolarParams.InducedDragFactor * (LiftCoefficient * LiftCoefficient);
        
        // Parasite Drag + Wing Tuck Profile Drag Adjustment
        float DynamicParasiteDrag = PolarParams.ZeroLiftDragCoefficient * (1.0f - (CurrentWingTuckRatio * 0.45f));

        if (bIsStalled)
        {
            DragCoefficient = DynamicParasiteDrag + InducedDragCoeff + (PolarParams.PostStallDragMultiplier * FMath::Square(FMath::Sin(AlphaRads)));
        }
        else
        {
            DragCoefficient = DynamicParasiteDrag + InducedDragCoeff;
        }

        // 5. AERODYNAMIC FORCE VECTORS COMPUTATION
        float DynamicPressure = 0.5f * AirDensity * (Airspeed * Airspeed);

        float LiftForceMagnitude = LiftCoefficient * DynamicPressure * ActiveWingArea;
        float DragForceMagnitude = DragCoefficient * DynamicPressure * ActiveWingArea;

        // Lift is perpendicular to relative wind, Drag is parallel to relative wind
        FVector LiftDirection = FVector::CrossProduct(RelativeWindDirection, RightVector).GetSafeNormal();
        FVector DragDirection = -RelativeWindDirection;

        CalculatedGlideLiftWorld = LiftDirection * LiftForceMagnitude;
        CalculatedGlideDragWorld = DragDirection * DragForceMagnitude;

        // 6. APPLY PHYSICAL GLIDE FORCES
        RigidBodyMesh->AddForce(CalculatedGlideLiftWorld + CalculatedGlideDragWorld, NAME_None, false);

        // 7. TELEMETRY COMPUTATION ($L/D$ Ratio)
        CurrentGlideRatio = (DragForceMagnitude > 0.001f) ? (LiftForceMagnitude / DragForceMagnitude) : 0.0f;
    }
};
