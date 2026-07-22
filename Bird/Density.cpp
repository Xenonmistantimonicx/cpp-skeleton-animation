#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAABirdAirDensityEngine.generated.h"

USTRUCT(BlueprintType)
struct FAtmosphericState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere Environment")
    float SeaLevelAirDensity = 1.225f; // kg/m^3 (Standard International Atmosphere ISA at 15°C)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere Environment")
    float SeaLevelTemperatureCelsius = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere Environment")
    float RelativeHumidityPercent = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere Environment")
    float TemperatureLapseRate = 0.0065f; // -6.5°C per 1000m elevation drop

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere Environment")
    float AtmosphericScaleHeightMeters = 8500.0f; // Barometric altitude scale height
};

USTRUCT(BlueprintType)
struct FGroundEffectParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Effect")
    bool bEnableGroundEffect = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Effect")
    float MaximumGroundEffectAltitudeMeters = 2.4f; // Typically ~1x Wingspan

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Effect")
    float MaxLiftAugmentationFactor = 1.35f; // +35% Lift close to ground

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Effect")
    float MaxInducedDragReductionFactor = 0.55f; // -45% Induced Drag close to ground
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAABirdAirDensityEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAABirdAirDensityEngine()
    {
        PrimaryComponentTick.bCanEverTick = true;
        PrimaryComponentTick.TickGroup = TG_PrePhysics;

        SubStepCount = 8;
        WingspanMeters = 2.4f;
        WingAreaSqMeters = 0.92f;

        CurrentAltitudeMeters = 0.0f;
        CurrentLocalTemperatureCelsius = 15.0f;
        CurrentAirDensity = 1.225f;
        CurrentGroundEffectLiftFactor = 1.0f;
        CurrentGroundEffectDragFactor = 1.0f;

        CalculatedEnvironmentLiftWorld = FVector::ZeroVector;
        CalculatedEnvironmentDragWorld = FVector::ZeroVector;
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
    FAtmosphericState AtmosphereConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Ground Dynamics")
    FGroundEffectParameters GroundEffectConfig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentAltitudeMeters;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentLocalTemperatureCelsius;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentAirDensity; // Dynamic $\rho$ in kg/m^3

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentGroundEffectLiftFactor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentGroundEffectDragFactor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedEnvironmentLiftWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedEnvironmentDragWorld;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
    {
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (!RigidBodyMesh || !RigidBodyMesh->IsSimulatingPhysics()) return;

        const int32 Steps = FMath::Clamp(SubStepCount, 1, 16);
        const float SubstepDeltaTime = DeltaTime / static_cast<float>(Steps);

        for (int32 Step = 0; Step < Steps; ++Step)
        {
            EvaluateAirDensityPhysicsSubstep(SubstepDeltaTime);
        }
    }

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    void EvaluateAirDensityPhysicsSubstep(float SubstepDeltaTime)
    {
        FVector ActorLocation = RigidBodyMesh->GetComponentLocation();
        FVector LinearVelocityWorld = RigidBodyMesh->GetLinearVelocity();
        float ForwardSpeed = LinearVelocityWorld.Size();

        if (ForwardSpeed < 0.1f)
        {
            CalculatedEnvironmentLiftWorld = FVector::ZeroVector;
            CalculatedEnvironmentDragWorld = FVector::ZeroVector;
            return;
        }

        // 1. ALTITUDE & DENSITY COMPUTATION ($\rho(z) = \rho_0 \cdot e^{-z / H}$)
        CurrentAltitudeMeters = FMath::Max(0.0f, ActorLocation.Z * 0.01f); // Unreal units (cm) -> Meters
        CurrentLocalTemperatureCelsius = AtmosphereConfig.SeaLevelTemperatureCelsius - (CurrentAltitudeMeters * AtmosphereConfig.TemperatureLapseRate);

        // Barometric Exponential Decay
        float BarometricDensityRatio = FMath::Exp(-CurrentAltitudeMeters / AtmosphereConfig.AtmosphericScaleHeightMeters);
        
        // Temperature Modulation Effect ($P = \rho R T$)
        float AbsoluteTempKelvin = CurrentLocalTemperatureCelsius + 273.15f;
        float SeaLevelTempKelvin = AtmosphereConfig.SeaLevelTemperatureCelsius + 273.15f;
        float TemperatureCorrectionRatio = SeaLevelTempKelvin / AbsoluteTempKelvin;

        CurrentAirDensity = AtmosphereConfig.SeaLevelAirDensity * BarometricDensityRatio * TemperatureCorrectionRatio;

        // 2. GROUND EFFECT DETECTION & CALCULATION (Wieselsberger Equation)
        CurrentGroundEffectLiftFactor = 1.0f;
        CurrentGroundEffectDragFactor = 1.0f;

        if (GroundEffectConfig.bEnableGroundEffect)
        {
            FHitResult GroundHit;
            FVector TraceStart = ActorLocation;
            FVector TraceEnd = ActorLocation - FVector(0.0f, 0.0f, GroundEffectConfig.MaximumGroundEffectAltitudeMeters * 100.0f);

            FCollisionQueryParams QueryParams;
            QueryParams.AddIgnoredActor(GetOwner());

            if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
            {
                float GroundHeightMeters = GroundHit.Distance * 0.01f;
                float HeightToWingspanRatio = GroundHeightMeters / WingspanMeters;

                if (HeightToWingspanRatio < 1.0f)
                {
                    // Wieselsberger Ground Effect Lift Coefficient Augmentation
                    float InfluenceFactor = 1.0f / (16.0f * FMath::Square(HeightToWingspanRatio) + 1.0f);
                    
                    CurrentGroundEffectLiftFactor = 1.0f + ((GroundEffectConfig.MaxLiftAugmentationFactor - 1.0f) * InfluenceFactor);
                    CurrentGroundEffectDragFactor = 1.0f - ((1.0f - GroundEffectConfig.MaxInducedDragReductionFactor) * InfluenceFactor);
                }
            }
        }

        // 3. AERODYNAMIC FORCES WITH DENSITY & GROUND EFFECT INTEGRATION
        // Dynamic Pressure: $q = \frac{1}{2} \cdot \rho \cdot V^2$
        float DynamicPressure = 0.5f * CurrentAirDensity * (ForwardSpeed * ForwardSpeed);

        FVector VelocityDirection = LinearVelocityWorld.GetUnsafeNormal();
        FVector UpVector = RigidBodyMesh->GetUpVector();
        FVector RightVector = RigidBodyMesh->GetRightVector();

        // Base Aerodynamic Coefficients
        float BaseLiftCoeff = 0.85f;
        float BaseDragCoeff = 0.045f;

        // Apply Atmospheric Modifications
        float FinalLiftForce = DynamicPressure * WingAreaSqMeters * BaseLiftCoeff * CurrentGroundEffectLiftFactor;
        float FinalDragForce = DynamicPressure * WingAreaSqMeters * BaseDragCoeff * CurrentGroundEffectDragFactor;

        FVector LiftDirection = FVector::CrossProduct(VelocityDirection, RightVector).GetSafeNormal();
        FVector DragDirection = -VelocityDirection;

        CalculatedEnvironmentLiftWorld = LiftDirection * FinalLiftForce;
        CalculatedEnvironmentDragWorld = DragDirection * FinalDragForce;

        // 4. APPLY PHYSICAL ATMOSPHERIC FORCES
        RigidBodyMesh->AddForce(CalculatedEnvironmentLiftWorld + CalculatedEnvironmentDragWorld, NAME_None, false);
    }
};
