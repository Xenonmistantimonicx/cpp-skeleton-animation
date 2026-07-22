#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAABirdFlightModeComponent.generated.h"

UENUM(BlueprintType)
enum class EAAAFlapStateMode : uint8
{
    ContinuousFlapping    UMETA(DisplayName = "Continuous High-Frequency Flapping"),
    IntermittentFlapping  UMETA(DisplayName = "Intermittent Flap-Bounding Flight"),
    SteadyGliding         UMETA(DisplayName = "Steady Airfoil Gliding"),
    HighSpeedSoaring      UMETA(DisplayName = "High-Speed Dynamic Soaring")
};

USTRUCT(BlueprintType)
struct FFlightModeThresholdConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Thresholds")
    float GlidingTransitionSpeedKmh = 38.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Thresholds")
    float FlappingResumeSpeedKmh = 32.0f; // Lower threshold to prevent mode flickering (Hysteresis)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Thresholds")
    float SoaringTransitionSpeedKmh = 65.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Thresholds")
    float MinFlapFrequencyHz = 2.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Thresholds")
    float MaxFlapFrequencyHz = 8.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Thresholds")
    float TargetStrouhalNumber = 0.30f; // St = (f * A) / V (Biomechanical flight efficiency constant)
};

USTRUCT(BlueprintType)
struct FFlapKinematicsProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinematics")
    float WingStrokeAmplitudeMeters = 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinematics")
    float DownstrokeToUpstrokeRatio = 1.35f; // Power stroke vs recovery stroke duration

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinematics")
    float BaseFlapThrustForceNewtons = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinematics")
    float MaxStamina = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinematics")
    float FlapStaminaDrainRate = 4.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinematics")
    float GlideStaminaRegenRate = 8.0f;
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAABirdFlightModeComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAABirdFlightModeComponent()
    {
        PrimaryComponentTick.bCanEverTick = true;
        PrimaryComponentTick.TickGroup = TG_PrePhysics;

        SubStepsPerFrame = 8;
        AirDensity = 1.225f;

        CurrentFlightMode = EAAAFlapStateMode::ContinuousFlapping;
        CurrentAirspeedKmh = 0.0f;
        CurrentFlapFrequencyHz = 0.0f;
        CurrentFlapAmplitudeNorm = 1.0f;
        CurrentWingPhaseRads = 0.0f;
        CurrentStamina = 100.0f;
        NormalizedBlendWeight = 0.0f;

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Substepping")
    int32 SubStepsPerFrame;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Flight Config")
    FFlightModeThresholdConfig ThresholdConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Flight Config")
    FFlapKinematicsProfile KinematicsProfile;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    EAAAFlapStateMode CurrentFlightMode;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentAirspeedKmh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentFlapFrequencyHz;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentFlapAmplitudeNorm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentWingPhaseRads;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentStamina;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float NormalizedBlendWeight; // 0.0 = Full Flap, 1.0 = Full Glide (for Anim Blueprint)

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedFlapThrustWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedFlapLiftWorld;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
    {
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (!RigidBodyMesh || !RigidBodyMesh->IsSimulatingPhysics()) return;

        const int32 ValidSteps = FMath::Clamp(SubStepsPerFrame, 1, 16);
        const float SubstepDeltaTime = DeltaTime / static_cast<float>(ValidSteps);

        for (int32 Step = 0; Step < ValidSteps; ++Step)
        {
            EvaluateFlightModePhysicsSubstep(SubstepDeltaTime);
        }

        // Interpolate blend weights smoothly for main thread rendering
        float TargetBlend = (CurrentFlightMode == EAAAFlapStateMode::SteadyGliding || CurrentFlightMode == EAAAFlapStateMode::HighSpeedSoaring) ? 1.0f : 0.0f;
        NormalizedBlendWeight = FMath::FInterpTo(NormalizedBlendWeight, TargetBlend, DeltaTime, 4.5f);
    }

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    void EvaluateFlightModePhysicsSubstep(float SubstepDeltaTime)
    {
        FVector LinearVelocityWorld = RigidBodyMesh->GetLinearVelocity();
        float SpeedMs = LinearVelocityWorld.Size();
        CurrentAirspeedKmh = SpeedMs * 3.6f;

        FVector ForwardVector = RigidBodyMesh->GetForwardVector();
        FVector UpVector = RigidBodyMesh->GetUpVector();

        // 1. STATE MACHINE EVALUATION WITH HYSTERESIS
        EvaluateStateTransitions(CurrentAirspeedKmh);

        // 2. STROUHAL NUMBER BASED FREQUENCY CALCULATIONS ($f = \frac{\text{St} \cdot V}{A}$)
        if (CurrentFlightMode == EAAAFlapStateMode::ContinuousFlapping || CurrentFlightMode == EAAAFlapStateMode::IntermittentFlapping)
        {
            float ComputedHz = (ThresholdConfig.TargetStrouhalNumber * FMath::Max(SpeedMs, 3.0f)) / KinematicsProfile.WingStrokeAmplitudeMeters;
            
            // Speed inverse scaling: slower speed requires higher flap frequency & amplitude
            if (CurrentAirspeedKmh < ThresholdConfig.FlappingResumeSpeedKmh)
            {
                float SlowSpeedRatio = 1.0f - FMath::Clamp(CurrentAirspeedKmh / ThresholdConfig.FlappingResumeSpeedKmh, 0.0f, 1.0f);
                ComputedHz += SlowSpeedRatio * 3.5f;
                CurrentFlapAmplitudeNorm = FMath::Clamp(1.0f + (SlowSpeedRatio * 0.4f), 1.0f, 1.5f);
            }
            else
            {
                CurrentFlapAmplitudeNorm = 1.0f;
            }

            CurrentFlapFrequencyHz = FMath::Clamp(ComputedHz, ThresholdConfig.MinFlapFrequencyHz, ThresholdConfig.MaxFlapFrequencyHz);

            // Phase Integration $\omega = 2\pi f$
            CurrentWingPhaseRads += (2.0f * PI * CurrentFlapFrequencyHz) * SubstepDeltaTime;
            if (CurrentWingPhaseRads >= 2.0f * PI)
            {
                CurrentWingPhaseRads -= 2.0f * PI;
            }

            // 3. FLAP IMPULSE DYNAMICS & ASYMMETRIC STROKE FORCE
            float PhaseSin = FMath::Sin(CurrentWingPhaseRads);
            bool bIsDownstroke = (PhaseSin > 0.0f);

            float StrokeForceMultiplier = bIsDownstroke ? KinematicsProfile.DownstrokeToUpstrokeRatio : 0.25f;
            float InstantaneousFlapForce = FMath::Max(0.0f, PhaseSin) * KinematicsProfile.BaseFlapThrustForceNewtons * StrokeForceMultiplier * CurrentFlapAmplitudeNorm;

            if (CurrentStamina <= 5.0f)
            {
                InstantaneousFlapForce *= 0.35f; // Exhaustion penalty
            }

            CalculatedFlapThrustWorld = ForwardVector * InstantaneousFlapForce;
            CalculatedFlapLiftWorld = UpVector * (InstantaneousFlapForce * 0.45f);

            RigidBodyMesh->AddForce(CalculatedFlapThrustWorld + CalculatedFlapLiftWorld, NAME_None, false);

            // Stamina Consumption
            CurrentStamina = FMath::Clamp(CurrentStamina - (KinematicsProfile.FlapStaminaDrainRate * CurrentFlapFrequencyHz * 0.1f * SubstepDeltaTime), 0.0f, KinematicsProfile.MaxStamina);
        }
        else
        {
            // Gliding / Soaring Mode
            CurrentFlapFrequencyHz = FMath::FInterpTo(CurrentFlapFrequencyHz, 0.0f, SubstepDeltaTime, 3.0f);
            CurrentFlapAmplitudeNorm = FMath::FInterpTo(CurrentFlapAmplitudeNorm, 0.0f, SubstepDeltaTime, 3.0f);
            CalculatedFlapThrustWorld = FVector::ZeroVector;
            CalculatedFlapLiftWorld = FVector::ZeroVector;

            // Stamina Recovery
            CurrentStamina = FMath::Clamp(CurrentStamina + (KinematicsProfile.GlideStaminaRegenRate * SubstepDeltaTime), 0.0f, KinematicsProfile.MaxStamina);
        }
    }

    void EvaluateStateTransitions(float SpeedKmh)
    {
        switch (CurrentFlightMode)
        {
            case EAAAFlapStateMode::ContinuousFlapping:
            {
                if (SpeedKmh >= ThresholdConfig.GlidingTransitionSpeedKmh && CurrentStamina > 15.0f)
                {
                    CurrentFlightMode = EAAAFlapStateMode::SteadyGliding;
                }
                else if (SpeedKmh > 20.0f && SpeedKmh < ThresholdConfig.GlidingTransitionSpeedKmh)
                {
                    CurrentFlightMode = EAAAFlapStateMode::IntermittentFlapping;
                }
                break;
            }

            case EAAAFlapStateMode::IntermittentFlapping:
            {
                if (SpeedKmh >= ThresholdConfig.GlidingTransitionSpeedKmh)
                {
                    CurrentFlightMode = EAAAFlapStateMode::SteadyGliding;
                }
                else if (SpeedKmh < 18.0f || CurrentStamina < 10.0f)
                {
                    CurrentFlightMode = EAAAFlapStateMode::ContinuousFlapping;
                }
                break;
            }

            case EAAAFlapStateMode::SteadyGliding:
            {
                if (SpeedKmh <= ThresholdConfig.FlappingResumeSpeedKmh)
                {
                    CurrentFlightMode = EAAAFlapStateMode::ContinuousFlapping;
                }
                else if (SpeedKmh >= ThresholdConfig.SoaringTransitionSpeedKmh)
                {
                    CurrentFlightMode = EAAAFlapStateMode::HighSpeedSoaring;
                }
                break;
            }

            case EAAAFlapStateMode::HighSpeedSoaring:
            {
                if (SpeedKmh < ThresholdConfig.SoaringTransitionSpeedKmh - 5.0f)
                {
                    CurrentFlightMode = EAAAFlapStateMode::SteadyGliding;
                }
                break;
            }
        }
    }
};
