// Copyright AAA Studios, 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAAUnifiedAvianPhysicsEngine.generated.h"

// ============================================================================
// 1. ATMOSPHERIC & ENVIRONMENTAL STRUCTURES
// ============================================================================

USTRUCT(BlueprintType)
struct FAAAAtmosphericEnvironment
{
    GENERATED_BODY()

    /** World Sun Direction for Shadow Thermal Calculations */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere - Valley")
    FVector SunDirectionVector = FVector(0.6f, 0.2f, -0.75f).GetSafeNormal();

    /** Thermal sink rate in shadowed mountain areas (m/s) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere - Valley")
    float ShadowThermalCollapseRateMs = 5.2f;

    /** Orographic Katabatic peak sink rate (m/s) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere - Valley")
    float PeakKatabaticDowndraftMs = 9.8f;

    /** Thickness of mountain cold air pool (meters) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere - Valley")
    float ColdAirPoolThicknessMeters = 80.0f;

    /** Thermal Updraft Core Velocity (m/s) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere - Thermal Soaring")
    float ThermalCoreUpdraftMs = 6.5f;

    /** Thermal Core Radius in meters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere - Thermal Soaring")
    float ThermalRadiusMeters = 50.0f;

    /** Thermal Core World Location */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere - Thermal Soaring")
    FVector ThermalCenterWorld = FVector(0.0f, 0.0f, 0.0f);

    /** Ridge Orographic Updraft Speed (m/s) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere - Ridge Lift")
    float RidgeUpdraftSpeedMs = 4.5f;

    /** Dynamic Soaring Wind Shear Gradient (dV/dZ) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere - Wind Shear")
    float WindShearGradient = 0.18f;

    /** Boundary Layer Wind Shear Max Height (meters) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere - Wind Shear")
    float BoundaryLayerHeightMeters = 35.0f;
};

// ============================================================================
// 2. BIOMECHANICAL & STRUCTURAL BONE PROPERTIES
// ============================================================================

USTRUCT(BlueprintType)
struct FAAABiomechanicalStructure
{
    GENERATED_BODY()

    /** Body mass in kg (0.045kg Swallow -> 12.0kg Condor) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morphometrics", meta = (ClampMin = "0.01", ClampMax = "50.0"))
    float BodyMassKg = 2.5f;

    /** Wingspan (Tip to Tip) in meters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morphometrics")
    float WingspanMeters = 1.8f;

    /** Total Wing Planform Area ($m^2$) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morphometrics")
    float WingAreaSqMeters = 0.45f;

    /** Root Chord Length in meters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morphometrics")
    float RootChordMeters = 0.25f;

    /** Young's Modulus of Pneumatic Avian Bone ($E \approx 1.8 \times 10^{10} \text{ Pa}$) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Biophysics")
    float BoneYoungsModulusPa = 1.8e10f;

    /** Bone Second Moment of Area ($I = \frac{\pi(D^4 - d^4)}{64}$) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Biophysics")
    float BoneSecondMomentOfArea = 1.4e-9f;

    /** Skeleton Damping Ratio ($\zeta$) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Biophysics")
    float BoneDampingRatio = 0.05f;

    /** Rachis Feather Shaft Flexural Rigidity ($E I_{feather}$) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feather Biophysics")
    float RachisFlexuralRigidity = 0.0042f;
};

// ============================================================================
// 3. THERMODYNAMIC & METABOLIC ENERGY CORE
// ============================================================================

USTRUCT(BlueprintType)
struct FAAAMetabolicThermodynamics
{
    GENERATED_BODY()

    /** Total ATP / Glycogen Reserves in Joules ($J$) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism")
    double MetabolicEnergyReserveJoules = 65000.0;

    /** Maximum Metabolic Storage Capacity in Joules */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism")
    double MaxMetabolicReserveJoules = 65000.0;

    /** Resting Basal Metabolic Rate in Watts ($J/s$) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism")
    float BasalMetabolicRateWatts = 5.5f;

    /** Muscle Mechanical Efficiency ($\eta \approx 0.20$) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metabolism")
    float MuscleMechanicalEfficiency = 0.20f;

    /** Core Body Temperature in Kelvin ($K$) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Thermodynamics")
    float CoreTemperatureKelvin = 313.15f; // 40 degrees C

    /** Thermal Cooling Rate to surrounding atmosphere */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermodynamics")
    float ThermalCoolingRateCoeff = 0.12f;

    /** Lactic Acid Buildup Fatigue Factor ($0.0 = \text{Fresh}, 1.0 = \text{Exhausted}$) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fatigue")
    float LacticAcidFatigueFactor = 0.0f;
};

// ============================================================================
// 4. UNIFIED TELEMETRY OUTPUT
// ============================================================================

USTRUCT(BlueprintType)
struct FAAAUnifiedAvianTelemetry
{
    GENERATED_BODY()

    // Mechanical Energy Tracking
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Energy Telemetry")
    double KineticEnergyJoules = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Energy Telemetry")
    double PotentialEnergyJoules = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Energy Telemetry")
    double TotalMechanicalEnergyJoules = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Energy Telemetry")
    float NetPowerHarvestedOrLostWatts = 0.0f;

    // Morphometrics & Agility
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight Telemetry")
    float AspectRatio = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight Telemetry")
    float WingLoadingNmsq = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight Telemetry")
    float AgilityScaleFactor = 1.0f;

    // Structural Deformity
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Structural Telemetry")
    float WingTipBoneDeflectionMeters = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Structural Telemetry")
    float PrimaryFeatherDeflectionDeg = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Structural Telemetry")
    float FeatherTorsionalWashoutDeg = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Structural Telemetry")
    float HumerusBendingStressPa = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Structural Telemetry")
    float LoadFactorG = 1.0f;

    // Environmental Lift States
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Telemetry")
    float CurrentUpdraftSpeedMs = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Telemetry")
    float CurrentDowndraftSpeedMs = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment Telemetry")
    bool bInMountainShadow = false;

    // Forces & Torques
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vector Output")
    FVector NetCalculatedForceWorld = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vector Output")
    FVector NetCalculatedTorqueWorld = FVector::ZeroVector;
};

// ============================================================================
// MAIN UNIFIED ENGINE CLASS
// ============================================================================

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAAUnifiedAvianPhysicsEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAAUnifiedAvianPhysicsEngine();

protected:
    virtual void BeginPlay() override;

public:    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Engine Config")
    int32 SubStepCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Engine Config")
    float AirDensityStandard;

    // Core Modular Configurations
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Avian Model")
    FAAABiomechanicalStructure Anatomy;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Avian Model")
    FAAAMetabolicThermodynamics EnergyCore;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Environment Model")
    FAAAAtmosphericEnvironment Atmosphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Engine Telemetry")
    FAAAUnifiedAvianTelemetry Telemetry;

    // Flight Control Inputs
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Controls")
    float PitchInput;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Controls")
    float RollInput;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Controls")
    float YawInput;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Controls")
    float ThrottleFlapInput; // 0.0 (Glide/Soar) to 1.0 (Full Flap Thrust)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Controls")
    bool bEngageOptimalGlideRatio;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Controls")
    bool bExecuteRapidSnapTurn;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    // Internal Calculations & States
    double BaselineGroundAltitudeMeters;
    double PreviousSubstepTotalEnergyJoules;

    float Ixx;
    float Iyy;
    float Izz;

    float LeftWingMorphRatio;
    float RightWingMorphRatio;
    float ThrustAsymmetryBias;

    // Bone Spring-Mass Dynamic State
    float BoneTipDeflectionPosition;
    float BoneTipDeflectionVelocity;

    void RecalculateAllometricAndInertialProperties();
    float SampleUpdraftVelocityField(const FVector& LocationWorld, float& OutThermalProximity);
    float SampleWindShearGradient(const FVector& LocationWorld, FVector& OutWindVelocityWorld);
    
    void EvaluateSingleUnifiedPhysicsSubstep(float SubstepDeltaTime);
};
