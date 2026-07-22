#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAABirdForestUpdraftEngine.generated.h"

USTRUCT(BlueprintType)
struct FForestCanopyParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Canopy Config")
    float CanopyTopHeightMeters = 22.0f; // Average height of forest tree crowns

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Canopy Config")
    float LeafAreaIndex = 4.5f; // LAI: Density of foliage layer (1.0 - 8.0)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Canopy Config")
    float CanopyAttenuatedWindFactor = 0.18f; // Wind velocity reduction under canopy (Sub-canopy calm zone)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Canopy Config")
    float CanopyShearLayerTurbulence = 3.2f; // Shear turbulence intensity at crown surface
};

USTRUCT(BlueprintType)
struct FForestThermalMicroclimate
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Microclimate")
    float SunElevationAngleDeg = 65.0f; // High solar angle = maximum canopy heating

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Microclimate")
    float CanopySolarAbsorptionCoeff = 0.82f; // Crown heat trapping ratio

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Microclimate")
    float EvapotranspirationCoolingFactor = 0.35f; // Dense wet foliage creates localized cold air sinks

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Microclimate")
    float ClearingThermalBoostFactor = 2.4f; // Forest clearings/gaps generate massive concentrated updrafts

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Microclimate")
    float ThermalColumnRadiusMeters = 18.0f;
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAABirdForestUpdraftEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAABirdForestUpdraftEngine()
    {
        PrimaryComponentTick.bCanEverTick = true;
        PrimaryComponentTick.TickGroup = TG_PrePhysics;

        SubStepCount = 8;
        WingspanMeters = 2.4f;
        WingAreaSqMeters = 0.92f;
        AirDensity = 1.225f;

        CurrentTimeAccumulator = 0.0f;
        CurrentHeightAboveCanopyMeters = 0.0f;
        CurrentForestThermalUpdraftMs = 0.0f;
        CurrentEvapotranspirationalSinkMs = 0.0f;
        bIsInsideCanopy = false;

        CalculatedForestUpdraftForceWorld = FVector::ZeroVector;
        CalculatedCanopyShearTorqueWorld = FVector::ZeroVector;
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
    float AirDensity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Forest Environment")
    FForestCanopyParameters CanopyConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Forest Environment")
    FForestThermalMicroclimate MicroclimateConfig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentHeightAboveCanopyMeters;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentForestThermalUpdraftMs;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentEvapotranspirationalSinkMs;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    bool bIsInsideCanopy;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedForestUpdraftForceWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedCanopyShearTorqueWorld;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
    {
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (!RigidBodyMesh || !RigidBodyMesh->IsSimulatingPhysics()) return;

        CurrentTimeAccumulator += DeltaTime;

        const int32 Steps = FMath::Clamp(SubStepCount, 1, 16);
        const float SubstepDeltaTime = DeltaTime / static_cast<float>(Steps);

        for (int32 Step = 0; Step < Steps; ++Step)
        {
            EvaluateForestUpdraftPhysicsSubstep(SubstepDeltaTime);
        }
    }

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    float CurrentTimeAccumulator;

    void EvaluateForestUpdraftPhysicsSubstep(float SubstepDeltaTime)
    {
        FVector ActorLocation = RigidBodyMesh->GetComponentLocation();
        FVector LinearVelocityWorld = RigidBodyMesh->GetLinearVelocity();
        float ForwardSpeed = LinearVelocityWorld.Size();

        // 1. TERRAIN & CANOPY DETECTION (Raycast down to ground and forest floor)
        FHitResult GroundHit;
        FVector TraceStart = ActorLocation;
        FVector TraceEnd = ActorLocation - FVector(0.0f, 0.0f, 15000.0f); // 150m raycast

        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(GetOwner());

        float GroundAltitudeMeters = 0.0f;
        bool bHasGroundUnderneath = GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

        if (bHasGroundUnderneath)
        {
            GroundAltitudeMeters = GroundHit.ImpactPoint.Z * 0.01f;
        }

        float BirdAltitudeMeters = ActorLocation.Z * 0.01f;
        float HeightAboveGroundMeters = FMath::Max(0.0f, BirdAltitudeMeters - GroundAltitudeMeters);
        
        CurrentHeightAboveCanopyMeters = HeightAboveGroundMeters - CanopyConfig.CanopyTopHeightMeters;
        bIsInsideCanopy = (CurrentHeightAboveCanopyMeters <= 0.0f);

        // 2. THERMAL PLUME GENERATION OVER CANOPY & CLEARINGS
        CurrentForestThermalUpdraftMs = 0.0f;
        CurrentEvapotranspirationalSinkMs = 0.0f;

        if (!bIsInsideCanopy && CurrentHeightAboveCanopyMeters < 120.0f) // Thermals extend up to 120m above canopy
        {
            // Spatial Perlin Noise determines Forest Gaps vs. Dense Canopy Zones
            float ClearingPerlin = FMath::PerlinNoise3D(FVector(ActorLocation.X * 0.0002f, ActorLocation.Y * 0.0002f, 0.0f));
            
            // Solar Insolation Component: $I = I_0 \cdot \sin(\theta_{sun})$
            float SolarInsolation = FMath::Clamp(FMath::Sin(FMath::DegreesToRadians(MicroclimateConfig.SunElevationAngleDeg)), 0.0f, 1.0f);

            if (ClearingPerlin > 0.25f)
            {
                // FOREST CLEARING / GAP: Unshaded soil heats up rapidly -> Strong Hot Updraft Plume
                float PlumeCoreFactor = FMath::SmoothStep(0.25f, 0.8f, ClearingPerlin);
                
                // Thermal Decay with Altitude Above Canopy: $W_{up}(z) = W_0 \cdot \exp(-z / H_{thermal})$
                float AltitudeDecay = FMath::Exp(-CurrentHeightAboveCanopyMeters / 45.0f);

                CurrentForestThermalUpdraftMs = PlumeCoreFactor * MicroclimateConfig.ClearingThermalBoostFactor * SolarInsolation * AltitudeDecay * 6.5f;
            }
            else
            {
                // DENSE CANOPY: Transpiration releases moisture -> Localized Cool Sink Effect (Downdraft)
                float DenseCanopyFactor = FMath::SmoothStep(0.25f, -0.8f, ClearingPerlin);
                
                CurrentEvapotranspirationalSinkMs = DenseCanopyFactor * MicroclimateConfig.EvapotranspirationCoolingFactor * SolarInsolation * 2.2f;
            }
        }

        // 3. CANOPY SHEAR LAYER TURBULENCE & DRAG ATTENUATION
        FVector CanopyShearTorque = FVector::ZeroVector;

        if (FMath::Abs(CurrentHeightAboveCanopyMeters) < 4.0f) // Boundary layer right at canopy surface
        {
            // Boundary Layer Friction Velocity ($u_*$) creates violent roll/pitch instability at tree-top level
            float ShearNoiseX = FMath::PerlinNoise3D(FVector(CurrentTimeAccumulator * 4.0f, 0.0f, 0.0f));
            float ShearNoiseY = FMath::PerlinNoise3D(FVector(0.0f, CurrentTimeAccumulator * 4.0f, 0.0f));

            CanopyShearTorque = FVector(ShearNoiseX, ShearNoiseY, 0.0f) * CanopyConfig.CanopyShearLayerTurbulence * DynamicPressureCalculation(ForwardSpeed);
        }

        // 4. VECTOR INTEGRATION & FORCE COMPUTATION
        float NetVerticalVelocityMs = CurrentForestThermalUpdraftMs - CurrentEvapotranspirationalSinkMs;

        // Sub-Canopy Wind Attenuation: If bird dives below tree tops, vertical thermal forces drop to zero
        if (bIsInsideCanopy)
        {
            NetVerticalVelocityMs = 0.0f;
        }

        // Lift Force generated by Forest Thermal Velocity: $F_z = \frac{1}{2} \cdot \rho \cdot V_{thermal}^2 \cdot S \cdot C_L$
        float DynamicUpdraftPressure = 0.5f * AirDensity * (NetVerticalVelocityMs * NetVerticalVelocityMs);
        float UpdraftForceScalar = DynamicUpdraftPressure * WingAreaSqMeters * 1.45f;

        if (NetVerticalVelocityMs < 0.0f)
        {
            UpdraftForceScalar = -UpdraftForceScalar; // Convert to downdraft force for evapotranspirational sinks
        }

        CalculatedForestUpdraftForceWorld = FVector(0.0f, 0.0f, UpdraftForceScalar);
        CalculatedCanopyShearTorqueWorld = CanopyShearTorque;

        // 5. APPLY PHYSICAL FORCES
        RigidBodyMesh->AddForce(CalculatedForestUpdraftForceWorld, NAME_None, false);
        RigidBodyMesh->AddTorqueInRadians(CalculatedCanopyShearTorqueWorld, NAME_None, true);
    }

    float DynamicPressureCalculation(float SpeedMs) const
    {
        return 0.5f * AirDensity * (SpeedMs * SpeedMs);
    }
};
