// Copyright AAA Studios, 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAAUnifiedBirdDynamicsEngine.generated.h"

USTRUCT(BlueprintType)
struct FValleyAtmosphereParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Atmosphere")
    FVector SunDirectionVector = FVector(0.6f, 0.2f, -0.75f).GetSafeNormal();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Atmosphere")
    float ShadowThermalCollapseRateMs = 5.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Atmosphere")
    float ShadowShearTurbulenceIntensity = 4.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Atmosphere")
    float PeakKatabaticDowndraftMs = 9.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Atmosphere")
    float ColdAirPoolThicknessMeters = 80.0f;
};

USTRUCT(BlueprintType)
struct FUnifiedMorphometrics
{
    GENERATED_BODY()

    /** Body mass in kg (0.045kg for Swift/Songbird -> 12.0kg for Condor) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morphometrics", meta = (ClampMin = "0.01", ClampMax = "50.0"))
    float BodyMassKg = 0.045f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morphometrics")
    float WingspanMeters = 0.32f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morphometrics")
    float WingAreaSqMeters = 0.016f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morphometrics")
    float RootChordMeters = 0.08f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Morphometrics")
    float AspectRatio = 6.4f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Morphometrics")
    float WingLoadingNmsq = 27.5f;
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAAUnifiedBirdDynamicsEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAAUnifiedBirdDynamicsEngine();

protected:
    virtual void BeginPlay() override;

public:    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Configuration")
    int32 SubStepCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Atmosphere")
    float AirDensityStandard;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics")
    FUnifiedMorphometrics Morphometrics;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics")
    FValleyAtmosphereParameters ValleyConfig;

    // Flight Inputs (-1.0 to +1.0)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Controls")
    float PitchInput;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Controls")
    float RollInput;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Controls")
    float YawInput;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Controls")
    bool bFlapTriggered;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Controls")
    bool bExecuteRapidSnapTurn;

    // Output Telemetry
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
    float CurrentStaminaPercent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
    float AgilityScaleFactor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
    float CurrentCalculatedDowndraftSpeedMs;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
    bool bIsInRidgeShadow;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
    FVector TotalCalculatedForceWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
    FVector TotalCalculatedTorqueWorld;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    float InternalTimeAccumulator;

    // Calculated Moments of Inertia
    float Ixx;
    float Iyy;
    float Izz;

    // Morphing Wing State (Inner/Outer Area ratios)
    float LeftWingAreaRatio;
    float RightWingAreaRatio;
    float ThrustAsymmetryBias;
    float CirculationLiftBoost;

    void RecalculateMassAndInertiaProperties();
    void EvaluateUnifiedPhysicsSubstep(float SubstepDeltaTime);
};
