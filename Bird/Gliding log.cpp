// Copyright AAA Studios, 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAAUnifiedEnergyRecoveryEngine.generated.h"

/**
 * Atmospheric updraft and wind shear configuration for passive energy harvesting.
 */
USTRUCT(BlueprintType)
struct FEnvironmentalSoaringField
{
    GENERATED_BODY()

    /** Thermal Updraft Core Vertical Velocity (m/s) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Lift")
    float ThermalCoreUpdraftMs = 5.5f;

    /** Thermal Core Radius in meters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Lift")
    float ThermalRadiusMeters = 45.0f;

    /** World location of thermal column center */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Lift")
    FVector ThermalCenterWorld = FVector(0.0f, 0.0f, 0.0f);

    /** Ridge Orographic Updraft Magnitude (m/s) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ridge Lift")
    float RidgeUpdraftSpeedMs = 4.0f;

    /** Dynamic Soaring Wind Shear Gradient ($dV/dZ$ per second) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Soaring")
    float WindShearGradient = 0.15f;

    /** Reference altitude boundary layer thickness (meters) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Soaring")
    float BoundaryLayerHeightMeters = 30.0f;
};

/**
 * Total Mechanical and Energy Recovery State Tracking.
 */
USTRUCT(BlueprintType)
struct FEnergyHarvestingTelemetry
{
    GENERATED_BODY()

    /** Total Kinetic Energy ($E_k = \frac{1}{2} m v^2$) in Joules */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Energy Metrics")
    double KineticEnergyJoules = 0.0;

    /** Gravitational Potential Energy ($E_p = m g h$) in Joules */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Energy Metrics")
    double PotentialEnergyJoules = 0.0;

    /** Total Mechanical Energy ($E_{total} = E_k + E_p$) in Joules */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Energy Metrics")
    double TotalMechanicalEnergyJoules = 0.0;

    /** Instantaneous Rate of Mechanical Energy Gain/Loss in Watts ($J/s$) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Energy Metrics")
    float NetMechanicalPowerHarvestedWatts = 0.0f;

    /** Metabolic Stamina Recovery Percentage (0.0 to 100.0) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Recovery")
    float MetabolicStaminaPercent = 100.0f;

    /** Thermal core proximity factor (0.0 = Outside, 1.0 = Dead Center) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lift State")
    float ThermalCoreProximity = 0.0f;
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAAUnifiedEnergyRecoveryEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAAUnifiedEnergyRecoveryEngine();

protected:
    virtual void BeginPlay() override;

public:    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Configuration")
    int32 SubStepCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Atmosphere")
    float AirDensityStandard;

    /** Glider Morphometrics */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morphometrics")
    float BirdMassKg;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morphometrics")
    float WingspanMeters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morphometrics")
    float WingAreaSqMeters;

    /** Environmental energy sources */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soaring Atmosphere")
    FEnvironmentalSoaringField Atmosphere;

    // Flight Inputs
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soaring Controls")
    float PitchInput;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soaring Controls")
    float RollInput;

    /** Set TRUE to lock wings in optimal glide geometry (Maximizes L/D ratio) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soaring Controls")
    bool bEngageOptimalGlideMode;

    // Outputs
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
    FEnergyHarvestingTelemetry EnergyTelemetry;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
    FVector CalculatedAerodynamicForceWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telemetry")
    FVector CalculatedControlTorqueWorld;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    double BaselineGroundAltitudeMeters;
    double PreviousTotalEnergyJoules;

    float CalculateUpdraftVelocityField(const FVector& CurrentLocationWorld, float& OutThermalProximity);
    float CalculateDynamicWindShearVector(const FVector& CurrentLocationWorld, FVector& OutWindVelocityWorld);
    void EvaluateEnergyRecoverySubstep(float SubstepDeltaTime);
};
