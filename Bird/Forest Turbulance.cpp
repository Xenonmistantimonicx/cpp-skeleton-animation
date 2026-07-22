#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAABirdForestTurbulenceEngine.generated.h"

USTRUCT(BlueprintType)
struct FForestAerodynamicStructure
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Aerodynamics")
    float CanopyTopHeightMeters = 24.0f; // Average height of forest tree crowns ($h_c$)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Aerodynamics")
    float ZeroPlaneDisplacementMeters = 16.8f; // Displacement height ($d \approx 0.7 \cdot h_c$)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Aerodynamics")
    float AerodynamicRoughnessLengthMeters = 2.4f; // Roughness length ($z_0 \approx 0.1 \cdot h_c$)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Aerodynamics")
    float LeafAreaIndex = 5.2f; // LAI: Canopy foliage density index

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Aerodynamics")
    float SubCanopyAttenuationCoefficient = 2.8f; // Extinction coefficient ($\alpha$) for wind decay below crown
};

USTRUCT(BlueprintType)
struct FForestTurbulenceParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turbulence Mechanics")
    float FreeStreamWindSpeedKmh = 40.0f; // Wind velocity well above forest canopy

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turbulence Mechanics")
    FVector FreeStreamWindDirection = FVector(1.0f, 0.0f, 0.0f); // Prevailing wind direction

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turbulence Mechanics")
    float CanopyShearTurbulenceIntensity = 7.5f; // Roll/pitch/yaw disturbance at tree-top boundary

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turbulence Mechanics")
    float WakeVortexDecayRate = 0.85f; // Atmospheric dissipation rate of crown eddies

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turbulence Mechanics")
    float VonKarmanConstant = 0.40f; // Standard physical constant ($k$)
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAABirdForestTurbulenceEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAABirdForestTurbulenceEngine()
    {
        PrimaryComponentTick.bCanEverTick = true;
        PrimaryComponentTick.TickGroup = TG_PrePhysics;

        SubStepCount = 8;
        WingspanMeters = 2.4f;
        WingAreaSqMeters = 0.92f;
        AirDensityStandard = 1.225f;

        CurrentTimeAccumulator = 0.0f;
        CurrentBirdHeightAboveGroundMeters = 0.0f;
        CurrentLocalWindSpeedMs = 0.0f;
        CurrentFrictionVelocityMs = 0.0f;
        bIsSubCanopy = false;
        bIsInShearBoundaryLayer = false;

        CalculatedForestTurbulenceForceWorld = FVector::ZeroVector;
        CalculatedForestTurbulenceTorqueWorld = FVector::ZeroVector;
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Forest Turbulence")
    FForestAerodynamicStructure ForestStructure;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Forest Turbulence")
    FForestTurbulenceParameters TurbulenceConfig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentBirdHeightAboveGroundMeters;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentLocalWindSpeedMs;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentFrictionVelocityMs;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    bool bIsSubCanopy;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    bool bIsInShearBoundaryLayer;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedForestTurbulenceForceWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedForestTurbulenceTorqueWorld;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
    {
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (!RigidBodyMesh || !RigidBodyMesh->IsSimulatingPhysics()) return;

        CurrentTimeAccumulator += DeltaTime;

        const int32 Steps = FMath::Clamp(SubStepCount, 1, 16);
        const float SubstepDeltaTime = DeltaTime / static_cast<float>(Steps);

        for (int32 Step = 0; Step < Steps; ++Step)
        {
            EvaluateForestTurbulencePhysicsSubstep(SubstepDeltaTime);
        }
    }

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    float CurrentTimeAccumulator;

    void EvaluateForestTurbulencePhysicsSubstep(float SubstepDeltaTime)
    {
        FVector ActorLocation = RigidBodyMesh->GetComponentLocation();
        FVector LinearVelocityWorld = RigidBodyMesh->GetLinearVelocity();

        // 1. TERRAIN & GROUND ALTITUDE TRACE
        FHitResult GroundHit;
        FVector TraceStart = ActorLocation;
        FVector TraceEnd = ActorLocation - FVector(0.0f, 0.0f, 50000.0f); // 500m downcast

        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(GetOwner());

        float GroundAltitudeMeters = 0.0f;
        if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
        {
            GroundAltitudeMeters = GroundHit.ImpactPoint.Z * 0.01f;
        }

        CurrentBirdHeightAboveGroundMeters = FMath::Max(0.0f, (ActorLocation.Z * 0.01f) - GroundAltitudeMeters);
        float CanopyTopZ = ForestStructure.CanopyTopHeightMeters;

        bIsSubCanopy = (CurrentBirdHeightAboveGroundMeters < CanopyTopZ);
        
        // Boundary shear layer spans from 80% to 160% of canopy height
        bIsInShearBoundaryLayer = (CurrentBirdHeightAboveGroundMeters >= (CanopyTopZ * 0.8f) && CurrentBirdHeightAboveGroundMeters <= (CanopyTopZ * 1.6f));

        // 2. LOGARITHMIC WIND PROFILE & SUB-CANOPY EXTINCTION
        float BaseWindMs = (TurbulenceConfig.FreeStreamWindSpeedKmh / 3.6f);
        FVector NormalizedWindDir = TurbulenceConfig.FreeStreamWindDirection.GetSafeNormal();

        // Calculate Friction Velocity ($u_* = \frac{u(z) \cdot k}{\ln(\frac{z - d}{z_0})}$)
        float ReferenceHeight = CanopyTopZ * 2.0f;
        CurrentFrictionVelocityMs = (BaseWindMs * TurbulenceConfig.VonKarmanConstant) / 
            FMath::Loge((ReferenceHeight - ForestStructure.ZeroPlaneDisplacementMeters) / ForestStructure.AerodynamicRoughnessLengthMeters);

        if (!bIsSubCanopy)
        {
            // ABOVE CANOPY: Logarithmic Wind Velocity Profile ($u(z) = \frac{u_*}{k} \cdot \ln(\frac{z - d}{z_0})$)
            float HeightOffset = FMath::Max(CurrentBirdHeightAboveGroundMeters - ForestStructure.ZeroPlaneDisplacementMeters, ForestStructure.AerodynamicRoughnessLengthMeters + 0.1f);
            CurrentLocalWindSpeedMs = (CurrentFrictionVelocityMs / TurbulenceConfig.VonKarmanConstant) * FMath::Loge(HeightOffset / ForestStructure.AerodynamicRoughnessLengthMeters);
        }
        else
        {
            // SUB-CANOPY: Exponential Canopy Wind Decay Profile ($u(z) = u(h_c) \cdot \exp\left(-\alpha \cdot (1 - \frac{z}{h_c})\right)$)
            float WindAtCanopyTop = (CurrentFrictionVelocityMs / TurbulenceConfig.VonKarmanConstant) * 
                FMath::Loge((CanopyTopZ - ForestStructure.ZeroPlaneDisplacementMeters) / ForestStructure.AerodynamicRoughnessLengthMeters);

            float HeightRatio = CurrentBirdHeightAboveGroundMeters / FMath::Max(CanopyTopZ, 1.0f);
            CurrentLocalWindSpeedMs = WindAtCanopyTop * FMath::Exp(-ForestStructure.SubCanopyAttenuationCoefficient * (1.0f - HeightRatio));
        }

        // 3. MONIN-OBUKHOV CANOPY SHEAR LAYER & WAKE TURBULENCE
        FVector TurbulenceTorqueWorld = FVector::ZeroVector;
        FVector RandomGustVectorWorld = FVector::ZeroVector;

        if (bIsInShearBoundaryLayer)
        {
            // Inflection point in the wind profile creates strong Kelvin-Helmholtz billows (mixing layer eddies)
            float ShearProximityFactor = 1.0f - (FMath::Abs(CurrentBirdHeightAboveGroundMeters - CanopyTopZ) / (CanopyTopZ * 0.6f));
            ShearProximityFactor = FMath::Clamp(ShearProximityFactor, 0.0f, 1.0f);

            // 3D Perlin Noise for directional vortex fluctuations
            float NoiseTime = CurrentTimeAccumulator * 4.5f;
            float RollNoise = FMath::PerlinNoise3D(FVector(NoiseTime, ActorLocation.Y * 0.01f, 0.0f));
            float PitchNoise = FMath::PerlinNoise3D(FVector(0.0f, NoiseTime, ActorLocation.Z * 0.01f));
            float YawNoise = FMath::PerlinNoise3D(FVector(ActorLocation.X * 0.01f, 0.0f, NoiseTime));

            float DynamicShearTorqueScalar = 0.5f * AirDensityStandard * FMath::Square(CurrentLocalWindSpeedMs) * WingAreaSqMeters * TurbulenceConfig.CanopyShearTurbulenceIntensity;

            TurbulenceTorqueWorld = FVector(RollNoise, PitchNoise, YawNoise) * DynamicShearTorqueScalar * ShearProximityFactor;

            // Turbulent gust velocity vector
            float GustMagnitude = CurrentFrictionVelocityMs * 2.5f * ShearProximityFactor;
            RandomGustVectorWorld = FVector(RollNoise, PitchNoise, YawNoise) * GustMagnitude;
        }
        else if (bIsSubCanopy)
        {
            // Sub-canopy micro-turbulences driven by tree trunk drag wakes
            float TrunkNoise = FMath::PerlinNoise3D(FVector(CurrentTimeAccumulator * 2.0f, ActorLocation.X * 0.05f, ActorLocation.Y * 0.05f));
            float SubCanopyTorqueScalar = 0.5f * AirDensityStandard * FMath::Square(CurrentLocalWindSpeedMs) * WingAreaSqMeters * 1.5f;
            
            TurbulenceTorqueWorld = FVector(TrunkNoise, TrunkNoise, TrunkNoise) * SubCanopyTorqueScalar;
        }

        // 4. AERODYNAMIC RELATIVE AIRSPEED INTEGRATION
        FVector MacroWindVectorWorld = NormalizedWindDir * CurrentLocalWindSpeedMs;
        FVector EffectiveTotalWindWorld = MacroWindVectorWorld + RandomGustVectorWorld;

        FVector RelativeAirspeedWorld = LinearVelocityWorld - EffectiveTotalWindWorld;
        float DynamicPressure = 0.5f * AirDensityStandard * RelativeAirspeedWorld.SizeSquared();

        FVector AerodynamicDragForceWorld = -RelativeAirspeedWorld.GetSafeNormal() * DynamicPressure * WingAreaSqMeters * 0.065f;

        CalculatedForestTurbulenceForceWorld = AerodynamicDragForceWorld;
        CalculatedForestTurbulenceTorqueWorld = TurbulenceTorqueWorld;

        // 5. APPLY PHYSICAL FORCES TO RIGID BODY
        RigidBodyMesh->AddForce(CalculatedForestTurbulenceForceWorld, NAME_None, false);
        RigidBodyMesh->AddTorqueInRadians(CalculatedForestTurbulenceTorqueWorld, NAME_None, true);
    }
};
