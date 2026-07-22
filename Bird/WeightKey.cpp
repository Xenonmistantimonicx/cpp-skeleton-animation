// Copyright AAA Studios, 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAABirdBiomechanicsEngine.generated.h"

/**
 * Defines the physical morphometrics, wing geometry, and structural mass distribution.
 */
USTRUCT(BlueprintType)
struct FAviarMorphometrics
{
    GENERATED_BODY()

    /** Total body mass of the bird in kilograms (e.g., 0.08 kg for Swift, 1.2 kg for Falcon, 12.0 kg for Albatross/Condor) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morphometrics", meta = (ClampMin = "0.01", ClampMax = "50.0"))
    float BodyMassKg = 3.5f;

    /** Tip-to-tip wingspan in meters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morphometrics", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float WingspanMeters = 2.2f;

    /** Total planform area of both wings combined in square meters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morphometrics", meta = (ClampMin = "0.01", ClampMax = "50.0"))
    float WingAreaSqMeters = 0.65f;

    /** Chord length at the wing root in meters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morphometrics")
    float RootChordMeters = 0.35f;

    /** Aspect Ratio ($AR = b^2 / S$) calculated or manually assigned */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Morphometrics")
    float AspectRatio = 7.44f;

    /** Wing Loading ($WL = m \cdot g / S$) in $N/m^2$ */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Morphometrics")
    float WingLoadingNmsq = 52.8f;
};

/**
 * Muscle physiology, metabolic energy, fatigue, and instantaneous power generation limits.
 */
USTRUCT(BlueprintType)
struct FAviarPhysiology
{
    GENERATED_BODY()

    /** Maximum sustainable muscle power output in Watts */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physiology")
    float MaxPectoralisPowerWatts = 180.0f;

    /** Current stamina reservoir (0.0 to 100.0) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physiology")
    float StaminaPercent = 100.0f;

    /** Stamina depletion rate scalar per Flap cycle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physiology")
    float FatigueAccumulationRate = 0.15f;

    /** Recovery rate per second when gliding or soaring */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physiology")
    float StaminaRecoveryRate = 2.5f;

    /** Muscle efficiency curve scalar based on mass and core fatigue */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physiology")
    float CurrentMuscleEfficiency = 1.0f;
};

/**
 * Calculated 3D Rigid Body Inertia Tensor and Agility Dynamic Modifiers.
 */
USTRUCT(BlueprintType)
struct FAviarInertiaDynamics
{
    GENERATED_BODY()

    /** Moment of Inertia along Roll axis (X) in $kg \cdot m^2$ */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inertia")
    float Ixx = 0.0f;

    /** Moment of Inertia along Pitch axis (Y) in $kg \cdot m^2$ */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inertia")
    float Iyy = 0.0f;

    /** Moment of Inertia along Yaw axis (Z) in $kg \cdot m^2$ */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inertia")
    float Izz = 0.0f;

    /** Inverse Agility Coefficient ($A_c \propto m^{-1/3}$) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agility")
    float AgilityScaleFactor = 1.0f;

    /** Max Roll Rate limit in Radians/Sec */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agility")
    float MaxRollRateRadSec = 6.28f;

    /** Max Pitch Rate limit in Radians/Sec */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agility")
    float MaxPitchRateRadSec = 3.14f;
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAABirdBiomechanicsEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAABirdBiomechanicsEngine();

protected:
    virtual void BeginPlay() override;

public:    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Configuration")
    int32 SubStepCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Configuration")
    float AirDensityStandard;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Biomechanics")
    FAviarMorphometrics Morphometrics;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Biomechanics")
    FAviarPhysiology Physiology;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Dynamics")
    FAviarInertiaDynamics Inertia;

    // Flight Controls Inputs (-1.0 to +1.0)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Control Inputs")
    float PitchInput;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Control Inputs")
    float RollInput;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Control Inputs")
    float YawInput;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Control Inputs")
    bool bFlapFlappingTriggered;

    // Telemetry Outputs
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedAerodynamicForceWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedControlTorqueWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentInducedDrag;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentParasiticDrag;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentLiftForceN;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    float InternalTimeAccumulator;

    void RecalculateMorphometricInertia();
    void EvaluatePhysicsSubstep(float SubstepDeltaTime);
};
