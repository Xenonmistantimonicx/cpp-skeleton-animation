// Copyright AAA Studios, 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAAUnifiedFlightEnergyEngine.generated.h"

/**
 * Thermodynamic and Metabolic state tracking system for bird flight.
 */
USTRUCT(BlueprintType)
struct FAviarThermodynamics
{
    GENERATED_BODY()

    /** Total ATP / Glycogen Energy Reserves in Joules ($J$) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy Core")
    double MetabolicEnergyReserveJoules = 45000.0; // 45 kJ baseline for medium/large bird

    /** Max ATP storage capacity in Joules */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy Core")
    double MaxMetabolicReserveJoules = 45000.0;

    /** Basal Metabolic Rate (BMR) resting power consumption in Watts ($J/s$) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy Core")
    float BasalMetabolicRateWatts = 4.2f;

    /** Pectoralis Muscle Efficiency Ratio ($\eta \approx 0.18 - 0.23$). 18-23% becomes thrust, 77-82% becomes heat */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermodynamic Efficiency")
    float MuscleMechanicalEfficiency = 0.20f;

    /** Core Body Temperature in Kelvin ($K$) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Thermodynamics")
    float CoreTemperatureKelvin = 313.15f; // 40 degrees Celsius baseline

    /** Thermal dissipation rate coefficient to surrounding air */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermodynamics")
    float ThermalCoolingRateCoeff = 0.12f;

    /** Muscle Fatigue Factor derived from Lactic Acid buildup ($0.0 = \text{Fresh}, 1.0 = \text{Exhausted}$) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fatigue")
    float LacticAcidFatigueFactor = 0.0f;
};

/**
 * Mechanical and Potential/Kinetic Mechanical Energy State of the Rigid Body.
 */
USTRUCT(BlueprintType)
struct FAviarMechanicalEnergy
{
    GENERATED_BODY()

    /** Kinetic Energy: $E_k = \frac{1}{2} \cdot m \cdot v^2$ (Joules) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mechanical Energy")
    double KineticEnergyJoules = 0.0;

    /** Gravitational Potential Energy: $E_p = m \cdot g \cdot h$ (Joules) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mechanical Energy")
    double GravitationalPotentialEnergyJoules = 0.0;

    /** Total Mechanical Energy: $E_{mech} = E_k + E_p$ (Joules) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mechanical Energy")
    double TotalMechanicalEnergyJoules = 0.0;

    /** Mechanical Power Lost to Aerodynamic Drag: $P_{drag} = F_{drag} \cdot v$ (Watts) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power Dissipation")
    float AerodynamicPowerDissipationWatts = 0.0f;
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAAUnifiedFlightEnergyEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAAUnifiedFlightEnergyEngine();

protected:
    virtual void BeginPlay() override;

public:    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Configuration")
    int32 SubStepCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics")
    float BirdMassKg;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics")
    float WingspanMeters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics")
    float WingAreaSqMeters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Energy Model")
    FAviarThermodynamics Metabolism;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Energy Model")
    FAviarMechanicalEnergy Mechanics;

    // Flight Controls
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls")
    float PitchInput;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls")
    float ThrottleFlapInput; // 0.0 (Glide) to 1.0 (Full Flap)

    // Telemetry Outputs
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Energy Telemetry")
    float CurrentFlapPowerDemandWatts;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Energy Telemetry")
    float InstantaneousEnergyLossRateWatts;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Energy Telemetry")
    FVector NetCalculatedForceWorld;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    double InitialAltitudeReferenceMeters;

    void CalculateSubstepEnergyConservation(float SubstepDeltaTime);
};
