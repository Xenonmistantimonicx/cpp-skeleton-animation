#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAABirdFlappingEngine.generated.h"

USTRUCT(BlueprintType)
struct FFlapKinematicsParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flap Kinematics")
    float MaxStrokeFrequencyHz = 8.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flap Kinematics")
    float MinStrokeFrequencyHz = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flap Kinematics")
    float MaxWingTipAmplitudeMeters = 1.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flap Kinematics")
    float DownstrokeToUpstrokeTimeRatio = 1.4f; // Asymmetric timing ratio (Downstroke is faster/stronger)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flap Kinematics")
    float DownstrokeLiftCoeff = 2.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flap Kinematics")
    float DownstrokeThrustCoeff = 1.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flap Kinematics")
    float UpstrokeFeatheringDragReduction = 0.2f; // Wing feathering lowers drag during recovery
};

USTRUCT(BlueprintType)
struct FFlapStaminaSystem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina & Fatigue")
    float MaxMuscleStamina = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina & Fatigue")
    float BaseStaminaDrainPerFlap = 1.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina & Fatigue")
    float HighFrequencyFatigueMultiplier = 1.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina & Fatigue")
    float PassiveRecoveryRatePerSec = 3.5f;
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAABirdFlappingEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAABirdFlappingEngine()
    {
        PrimaryComponentTick.bCanEverTick = true;
        PrimaryComponentTick.TickGroup = TG_PrePhysics;

        AirDensity = 1.225f;
        WingspanMeters = 2.4f;
        WingAreaSqMeters = 0.92f;
        SubStepCount = 8;

        CurrentWingPhaseRads = 0.0f;
        TargetFlapIntensity = 0.0f;
        CurrentFlapFrequencyHz = 0.0f;
        CurrentWingtipVelocityMs = 0.0f;
        CurrentStamina = 100.0f;
        bIsDownstroke = false;

        CalculatedFlapThrustWorld = FVector::ZeroVector;
        CalculatedFlapLiftWorld = FVector::ZeroVector;
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
            }
        }
    }

public:    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Atmosphere")
    float AirDensity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Geometry")
    float WingspanMeters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Geometry")
    float WingAreaSqMeters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics Configuration")
    int32 SubStepCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Flapping Dynamics")
    FFlapKinematicsParameters KinematicsParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Flapping Dynamics")
    FFlapStaminaSystem FatigueSystem;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentWingPhaseRads;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentFlapFrequencyHz;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentWingtipVelocityMs;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentStamina;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    bool bIsDownstroke;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedFlapThrustWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedFlapLiftWorld;

    UFUNCTION(BlueprintCallable, Category = "AAA Flapping Controls")
    void SetFlapThrustInput(float FlapIntensity)
    {
        TargetFlapIntensity = FMath::Clamp(FlapIntensity, 0.0f, 1.0f);
    }

    UFUNCTION(BlueprintCallable, Category = "AAA Flapping Controls")
    void TriggerManualFlapImpulse()
    {
        if (CurrentStamina > 5.0f)
        {
            CurrentWingPhaseRads = 0.01f; // Force start of power downstroke
            FatigueSystem.MaxMuscleStamina = FMath::Clamp(CurrentStamina - FatigueSystem.BaseStaminaDrainPerFlap, 0.0f, FatigueSystem.MaxMuscleStamina);
        }
    }

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
    {
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (!RigidBodyMesh || !RigidBodyMesh->IsSimulatingPhysics()) return;

        const int32 Steps = FMath::Clamp(SubStepCount, 1, 16);
        const float SubstepDeltaTime = DeltaTime / static_cast<float>(Steps);

        for (int32 Step = 0; Step < Steps; ++Step)
        {
            EvaluateFlappingPhysicsSubstep(SubstepDeltaTime);
        }
    }

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    float TargetFlapIntensity;

    void EvaluateFlappingPhysicsSubstep(float SubstepDeltaTime)
    {
        FVector VelocityWorld = RigidBodyMesh->GetLinearVelocity();
        float ForwardSpeed = VelocityWorld.Size();

        FVector ForwardVector = RigidBodyMesh->GetForwardVector();
        FVector UpVector = RigidBodyMesh->GetUpVector();

        // 1. DYNAMIC FLAP FREQUENCY & STAMINA MODULATION
        float FatiguePenaltyMultiplier = (CurrentStamina < 20.0f) ? (CurrentStamina / 20.0f) : 1.0f;
        
        // Speed Inverse Scaling: Slower airspeed demands higher flap frequency to stay airborne
        float SpeedRequirementRatio = 1.0f - FMath::Clamp(ForwardSpeed / 15.0f, 0.0f, 0.75f);
        float RequiredFrequency = FMath::Lerp(KinematicsParams.MinStrokeFrequencyHz, KinematicsParams.MaxStrokeFrequencyHz, TargetFlapIntensity * SpeedRequirementRatio);
        
        CurrentFlapFrequencyHz = RequiredFrequency * FatiguePenaltyMultiplier;

        if (CurrentFlapFrequencyHz < 0.1f)
        {
            CalculatedFlapThrustWorld = FVector::ZeroVector;
            CalculatedFlapLiftWorld = FVector::ZeroVector;
            
            // Recover stamina when not flapping
            CurrentStamina = FMath::Clamp(CurrentStamina + (FatigueSystem.PassiveRecoveryRatePerSec * SubstepDeltaTime), 0.0f, FatigueSystem.MaxMuscleStamina);
            return;
        }

        // 2. PHASE INTEGRATION & ASYMMETRIC STROKE COMPUTATION
        float AngularFrequency = 2.0f * PI * CurrentFlapFrequencyHz;
        CurrentWingPhaseRads += AngularFrequency * SubstepDeltaTime;

        if (CurrentWingPhaseRads >= 2.0f * PI)
        {
            CurrentWingPhaseRads -= 2.0f * PI;

            // Drain stamina per completed stroke cycle
            float DrainAmount = FatigueSystem.BaseStaminaDrainPerFlap * (1.0f + (CurrentFlapFrequencyHz / KinematicsParams.MaxStrokeFrequencyHz) * FatigueSystem.HighFrequencyFatigueMultiplier);
            CurrentStamina = FMath::Clamp(CurrentStamina - DrainAmount, 0.0f, FatigueSystem.MaxMuscleStamina);
        }

        // Downstroke occurs in the first half of the cycle ($0 \rightarrow \pi$)
        bIsDownstroke = (CurrentWingPhaseRads < PI);

        // 3. WINGTIP VELOCITY & UNSTEADY AERODYNAMIC FORCE COMPUTATION
        // Wing position: $y(t) = A \cdot \sin(\phi)$
        // Wing velocity: $v_w(t) = A \cdot \omega \cdot \cos(\phi)$
        float Amplitude = KinematicsParams.MaxWingTipAmplitudeMeters * TargetFlapIntensity;
        CurrentWingtipVelocityMs = Amplitude * AngularFrequency * FMath::Cos(CurrentWingPhaseRads);

        float EffectiveRelativeAirspeed = FMath::Sqrt(FMath::Square(ForwardSpeed) + FMath::Square(CurrentWingtipVelocityMs));
        float DynamicPressure = 0.5f * AirDensity * (EffectiveRelativeAirspeed * EffectiveRelativeAirspeed);

        // 4. POWER DOWNSTROKE VS. FEATHERED UPSTROKE FORCE
        float InstantaneousLiftForce = 0.0f;
        float InstantaneousThrustForce = 0.0f;

        if (bIsDownstroke)
        {
            // Power Stroke: High thrust and positive lift generation
            float StrokeProgress = FMath::Sin(CurrentWingPhaseRads); // Peak force at mid-stroke
            
            InstantaneousLiftForce = DynamicPressure * WingAreaSqMeters * KinematicsParams.DownstrokeLiftCoeff * StrokeProgress;
            InstantaneousThrustForce = DynamicPressure * WingAreaSqMeters * KinematicsParams.DownstrokeThrustCoeff * StrokeProgress;
        }
        else
        {
            // Recovery Upstroke: Feathering reduces drag and minimizes negative lift
            float StrokeProgress = FMath::Abs(FMath::Sin(CurrentWingPhaseRads));
            
            InstantaneousLiftForce = DynamicPressure * WingAreaSqMeters * (KinematicsParams.DownstrokeLiftCoeff * 0.1f) * StrokeProgress;
            InstantaneousThrustForce = -DynamicPressure * WingAreaSqMeters * (KinematicsParams.DownstrokeThrustCoeff * KinematicsParams.UpstrokeFeatheringDragReduction) * StrokeProgress;
        }

        // Apply fatigue multiplier to final output forces
        InstantaneousLiftForce *= FatiguePenaltyMultiplier;
        InstantaneousThrustForce *= FatiguePenaltyMultiplier;

        // 5. VECTOR CONSTRUCTION & PHYSICAL FORCE APPLICATION
        CalculatedFlapThrustWorld = ForwardVector * InstantaneousThrustForce;
        CalculatedFlapLiftWorld = UpVector * InstantaneousLiftForce;

        RigidBodyMesh->AddForce(CalculatedFlapThrustWorld + CalculatedFlapLiftWorld, NAME_None, false);
    }
};
