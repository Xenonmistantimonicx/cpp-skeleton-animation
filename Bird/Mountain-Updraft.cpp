#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAABirdMountainUpdraftEngine.generated.h"

USTRUCT(BlueprintType)
struct FMountainSolarPosition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar Mechanics")
    FVector SunDirectionVector = FVector(0.5f, 0.5f, -0.707f).GetSafeNormal(); // Direction pointing FROM sun TO earth

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar Mechanics")
    float DirectSolarIrradianceWPerSqM = 1000.0f; // Peak high-altitude insolation (~1000 W/m²)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Solar Mechanics")
    float AmbientAirTemperatureCelsius = 15.0f;
};

USTRUCT(BlueprintType)
struct FMountainThermalPlumeParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Column Dynamics")
    float BaseThermalRadiusMeters = 45.0f; // Core thermal width at ground level

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Column Dynamics")
    float ThermalExpansionRate = 0.08f; // Thermal expansion angle as it rises (dRadius / dHeight)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Column Dynamics")
    float PeakUpdraftVelocityMs = 12.5f; // Maximum core vertical velocity in alpine conditions

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Column Dynamics")
    float SinkingSinkShellMultiplier = 0.35f; // Downdraft strength surrounding the rising thermal core

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thermal Column Dynamics")
    float MaximumThermalCeilingMeters = 3500.0f; // Thermal inversion height (ASL)
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAABirdMountainUpdraftEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAABirdMountainUpdraftEngine()
    {
        PrimaryComponentTick.bCanEverTick = true;
        PrimaryComponentTick.TickGroup = TG_PrePhysics;

        SubStepCount = 8;
        WingspanMeters = 2.4f;
        WingAreaSqMeters = 0.92f;
        AirDensityStandard = 1.225f;

        CurrentTimeAccumulator = 0.0f;
        CurrentLocalAltitudeASLMeters = 0.0f;
        CurrentCalculatedThermalUpdraftSpeedMs = 0.0f;
        CurrentSlopeInsolationFactor = 0.0f;
        bIsInThermalCore = false;
        bIsInSinkingShell = false;

        CalculatedMountainUpdraftForceWorld = FVector::ZeroVector;
        CalculatedThermalTurbulenceTorqueWorld = FVector::ZeroVector;
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics Configuration")
    int32 SubStepCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Geometry")
    float WingspanMeters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Geometry")
    float WingAreaSqMeters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Atmosphere")
    float AirDensityStandard;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Mountain Solar")
    FMountainSolarPosition SolarConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Mountain Thermals")
    FMountainThermalPlumeParameters ThermalConfig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentLocalAltitudeASLMeters;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentCalculatedThermalUpdraftSpeedMs;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentSlopeInsolationFactor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    bool bIsInThermalCore;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    bool bIsInSinkingShell;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedMountainUpdraftForceWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedThermalTurbulenceTorqueWorld;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
    {
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (!RigidBodyMesh || !RigidBodyMesh->IsSimulatingPhysics()) return;

        CurrentTimeAccumulator += DeltaTime;

        const int32 Steps = FMath::Clamp(SubStepCount, 1, 16);
        const float SubstepDeltaTime = DeltaTime / static_cast<float>(Steps);

        for (int32 Step = 0; Step < Steps; ++Step)
        {
            EvaluateMountainUpdraftPhysicsSubstep(SubstepDeltaTime);
        }
    }

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    float CurrentTimeAccumulator;

    void EvaluateMountainUpdraftPhysicsSubstep(float SubstepDeltaTime)
    {
        FVector ActorLocation = RigidBodyMesh->GetComponentLocation();
        CurrentLocalAltitudeASLMeters = ActorLocation.Z * 0.01f;

        // 1. TERRAIN RAYCAST & SLOPE INSOLATION COMPUTATION
        FHitResult MountainHit;
        FVector TraceStart = ActorLocation;
        FVector TraceEnd = ActorLocation - FVector(0.0f, 0.0f, 100000.0f); // 1km downcast

        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(GetOwner());

        CurrentSlopeInsolationFactor = 0.0f;
        FVector GroundImpactPoint = ActorLocation;
        bool bGroundHitFound = GetWorld()->LineTraceSingleByChannel(MountainHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

        if (bGroundHitFound)
        {
            GroundImpactPoint = MountainHit.ImpactPoint;
            FVector TerrainNormal = MountainHit.Normal;

            // Lambert's Cosine Law for solar irradiance on inclined mountain slopes: $I = I_0 \cdot (\vec{N} \cdot \vec{L})$
            FVector LightVector = -SolarConfig.SunDirectionVector.GetSafeNormal();
            float InsolationCosTheta = FVector::DotProduct(TerrainNormal, LightVector);

            CurrentSlopeInsolationFactor = FMath::Clamp(InsolationCosTheta, 0.0f, 1.0f);
        }

        // 2. BAROMETRIC AIR DENSITY LAPSE AT ALTITUDE
        // Air density decreases with height: $\rho(z) = \rho_0 \cdot e^{-z / H}$
        float BarometricScaleHeight = 8500.0f; // Atmospheric scale height in meters
        float LocalAirDensity = AirDensityStandard * FMath::Exp(-CurrentLocalAltitudeASLMeters / BarometricScaleHeight);

        // 3. THERMAL COLUMN GEOMETRY & GAUSSIAN VELOCITY PROFILE
        CurrentCalculatedThermalUpdraftSpeedMs = 0.0f;
        bIsInThermalCore = false;
        bIsInSinkingShell = false;

        if (CurrentLocalAltitudeASLMeters < ThermalConfig.MaximumThermalCeilingMeters)
        {
            // Deterministic placement of mountain thermal chimneys using spatial noise mapped to ridge hotspots
            FVector ThermalOriginGround = FVector(
                FMath::GridSnap(GroundImpactPoint.X, 3000.0f),
                FMath::GridSnap(GroundImpactPoint.Y, 3000.0f),
                GroundImpactPoint.Z
            );

            // Distance from bird to central vertical axis of thermal plume
            FVector HorizontalBirdOffset = FVector(ActorLocation.X - ThermalOriginGround.X, ActorLocation.Y - ThermalOriginGround.Y, 0.0f);
            float RadialDistanceMeters = HorizontalBirdOffset.Size() * 0.01f;

            float HeightAboveGroundMeters = FMath::Max(0.0f, CurrentLocalAltitudeASLMeters - (ThermalOriginGround.Z * 0.01f));
            
            // Thermal radius grows with altitude: $R(z) = R_0 + k \cdot z$
            float LocalThermalRadiusMeters = ThermalConfig.BaseThermalRadiusMeters + (HeightAboveGroundMeters * ThermalConfig.ThermalExpansionRate);
            
            // Peak thermal buoyancy driven by solar slope heating and altitude gradient
            float HeightBuoyancyFactor = FMath::Sin(FMath::Clamp(HeightAboveGroundMeters / ThermalConfig.MaximumThermalCeilingMeters, 0.0f, 1.0f) * PI);
            float CorePeakVelocity = ThermalConfig.PeakUpdraftVelocityMs * CurrentSlopeInsolationFactor * HeightBuoyancyFactor;

            // Gaussian Distribution for Thermal Core Velocity: $W(r) = W_{max} \cdot \exp\left(-\frac{r^2}{R^2}\right)$
            float NormalizedRadiusRatio = RadialDistanceMeters / FMath::Max(LocalThermalRadiusMeters, 1.0f);

            if (NormalizedRadiusRatio <= 1.0f)
            {
                // INSIDE THERMAL CORE (Updraft)
                bIsInThermalCore = true;
                CurrentCalculatedThermalUpdraftSpeedMs = CorePeakVelocity * FMath::Exp(-2.0f * (NormalizedRadiusRatio * NormalizedRadiusRatio));
            }
            else if (NormalizedRadiusRatio > 1.0f && NormalizedRadiusRatio <= 1.45f)
            {
                // SINKING SHELL BOUNDARY (Cooler air descending around thermal perimeter)
                bIsInSinkingShell = true;
                float SinkingRingFactor = FMath::Sin((NormalizedRadiusRatio - 1.0f) / 0.45f * PI);
                CurrentCalculatedThermalUpdraftSpeedMs = -CorePeakVelocity * ThermalConfig.SinkingSinkShellMultiplier * SinkingRingFactor;
            }
        }

        // 4. THERMAL BOUNDARY SHEAR TURBULENCE (Asymmetric Wing Rolling Moment)
        FVector ShearTurbulenceTorque = FVector::ZeroVector;

        if (bIsInThermalCore || bIsInSinkingShell)
        {
            // Rapid transition across thermal boundary creates severe rolling torque across wingspan
            float ShearNoise = FMath::PerlinNoise3D(FVector(CurrentTimeAccumulator * 6.0f, ActorLocation.Y * 0.005f, 0.0f));
            float AsymmetricLiftForce = 0.5f * LocalAirDensity * FMath::Square(CurrentCalculatedThermalUpdraftSpeedMs) * (WingAreaSqMeters * 0.5f);
            
            float RollTorqueScalar = AsymmetricLiftForce * (WingspanMeters * 0.5f) * ShearNoise;
            ShearTurbulenceTorque = FVector(RollTorqueScalar, 0.0f, 0.0f);
        }

        // 5. AERODYNAMIC FORCE INTEGRATION
        // Lift generated by vertical wind velocity: $F_z = \frac{1}{2} \rho V_z^2 S C_L$
        float UpdraftSign = (CurrentCalculatedThermalUpdraftSpeedMs >= 0.0f) ? 1.0f : -1.0f;
        float DynamicUpdraftPressure = 0.5f * LocalAirDensity * FMath::Square(CurrentCalculatedThermalUpdraftSpeedMs);
        float AerodynamicUpdraftForceScalar = DynamicUpdraftPressure * WingAreaSqMeters * 1.55f * UpdraftSign;

        CalculatedMountainUpdraftForceWorld = FVector(0.0f, 0.0f, AerodynamicUpdraftForceScalar);
        CalculatedThermalTurbulenceTorqueWorld = ShearTurbulenceTorque;

        // 6. APPLY PHYSICAL FORCES TO RIGID BODY
        RigidBodyMesh->AddForce(CalculatedMountainUpdraftForceWorld, NAME_None, false);
        RigidBodyMesh->AddTorqueInRadians(CalculatedThermalTurbulenceTorqueWorld, NAME_None, true);
    }
};
