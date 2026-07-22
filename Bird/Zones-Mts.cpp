#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAABirdMountainWindEngine.generated.h"

USTRUCT(BlueprintType)
struct FMountainMacroWindState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Macro Wind")
    FVector BaseWindDirection = FVector(1.0f, 0.0f, 0.0f); // Prevailing wind vector

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Macro Wind")
    float BaseWindSpeedKmh = 45.0f; // Base wind velocity

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Macro Wind")
    float GustFrequencyHz = 0.35f; // Frequency of wind speed variations

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Macro Wind")
    float GustAmplitudePercent = 0.30f; // ±30% wind speed fluctuation
};

USTRUCT(BlueprintType)
struct FRidgeLiftParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mountain Orographic Lift")
    float OrographicLiftMultiplier = 0.85f; // Vertical lift generated from slope gradient

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mountain Orographic Lift")
    float LeeSideRotorDownwashMultiplier = 1.25f; // Downward suck force on lee side

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mountain Orographic Lift")
    float MaxSlopeTraceDistanceMeters = 35.0f; // Ground proximity check for mountain ridges

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mountain Orographic Lift")
    float RotorTurbulenceIntensity = 4.5f; // Rotational chaos on lee side
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAABirdMountainWindEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAABirdMountainWindEngine()
    {
        PrimaryComponentTick.bCanEverTick = true;
        PrimaryComponentTick.TickGroup = TG_PrePhysics;

        SubStepCount = 8;
        WingspanMeters = 2.4f;
        WingAreaSqMeters = 0.92f;
        AirDensity = 1.225f;

        CurrentTimeAccumulator = 0.0f;
        CurrentLocalWindVectorWorld = FVector::ZeroVector;
        CurrentOrographicLiftSpeedMs = 0.0f;
        CurrentRotorDownwashSpeedMs = 0.0f;

        CalculatedWindAerodynamicForceWorld = FVector::ZeroVector;
        CalculatedWindTurbulenceTorqueWorld = FVector::ZeroVector;
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Mountain Wind")
    FMountainMacroWindState MacroWindConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Mountain Wind")
    FRidgeLiftParameters RidgeParams;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CurrentLocalWindVectorWorld; // Final wind vector including ridge up/downwash & gusts

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentOrographicLiftSpeedMs;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentRotorDownwashSpeedMs;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedWindAerodynamicForceWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedWindTurbulenceTorqueWorld;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
    {
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (!RigidBodyMesh || !RigidBodyMesh->IsSimulatingPhysics()) return;

        CurrentTimeAccumulator += DeltaTime;

        const int32 Steps = FMath::Clamp(SubStepCount, 1, 16);
        const float SubstepDeltaTime = DeltaTime / static_cast<float>(Steps);

        for (int32 Step = 0; Step < Steps; ++Step)
        {
            EvaluateMountainWindPhysicsSubstep(SubstepDeltaTime);
        }
    }

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    float CurrentTimeAccumulator;

    void EvaluateMountainWindPhysicsSubstep(float SubstepDeltaTime)
    {
        FVector ActorLocation = RigidBodyMesh->GetComponentLocation();
        FVector LinearVelocityWorld = RigidBodyMesh->GetLinearVelocity();

        // 1. MACRO WIND GUST COMPUTATION (Harmonic + Noise Fluctuation)
        FVector NormalizedMacroWindDir = MacroWindConfig.BaseWindDirection.GetSafeNormal();
        float BaseWindSpeedMs = (MacroWindConfig.BaseWindSpeedKmh / 3.6f);

        float GustSinFactor = FMath::Sin(CurrentTimeAccumulator * 2.0f * PI * MacroWindConfig.GustFrequencyHz);
        float NoiseVal = FMath::PerlinNoise3D(ActorLocation * 0.001f + FVector(CurrentTimeAccumulator * 0.2f, 0.0f, 0.0f));
        
        float DynamicWindSpeedMs = BaseWindSpeedMs * (1.0f + (GustSinFactor * MacroWindConfig.GustAmplitudePercent) + (NoiseVal * 0.15f));
        FVector MacroWindVectorWorld = NormalizedMacroWindDir * DynamicWindSpeedMs;

        // 2. OROGRAPHIC RIDGE LIFT & LEE-SIDE ROTOR TURBULENCE (Slope Raycasting)
        CurrentOrographicLiftSpeedMs = 0.0f;
        CurrentRotorDownwashSpeedMs = 0.0f;
        FVector RidgeVerticalWindVector = FVector::ZeroVector;
        FVector TurbulenceRotorTorque = FVector::ZeroVector;

        FHitResult TerrainHit;
        FVector TraceStart = ActorLocation;
        FVector TraceEnd = ActorLocation - FVector(0.0f, 0.0f, RidgeParams.MaxSlopeTraceDistanceMeters * 100.0f);

        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(GetOwner());

        if (GetWorld()->LineTraceSingleByChannel(TerrainHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
        {
            FVector GroundNormal = TerrainHit.Normal;

            // Dot product between wind direction and slope normal
            float WindSlopeAlignment = FVector::DotProduct(NormalizedMacroWindDir, GroundNormal);

            if (WindSlopeAlignment < -0.1f) 
            {
                // UPWIND SLOPE: Wind hits mountain wall -> Orographic Updraft
                float SlopeInclineFactor = FMath::Abs(WindSlopeAlignment);
                CurrentOrographicLiftSpeedMs = DynamicWindSpeedMs * SlopeInclineFactor * RidgeParams.OrographicLiftMultiplier;
                RidgeVerticalWindVector = FVector(0.0f, 0.0f, CurrentOrographicLiftSpeedMs);
            }
            else if (WindSlopeAlignment > 0.1f)
            {
                // LEE SIDE SLOPE: Wind blows over mountain peak -> Downwash & Violent Rotor Turbulence
                float LeeFactor = WindSlopeAlignment;
                CurrentRotorDownwashSpeedMs = DynamicWindSpeedMs * LeeFactor * RidgeParams.LeeSideRotorDownwashMultiplier;
                RidgeVerticalWindVector = FVector(0.0f, 0.0f, -CurrentRotorDownwashSpeedMs);

                // Generate turbulent torque vectors (wing shaking / wing tip instability)
                float TurbulencePitch = FMath::PerlinNoise3D(FVector(CurrentTimeAccumulator * 3.0f, 0.0f, 0.0f)) * RidgeParams.RotorTurbulenceIntensity;
                float TurbulenceRoll = FMath::PerlinNoise3D(FVector(0.0f, CurrentTimeAccumulator * 3.0f, 0.0f)) * RidgeParams.RotorTurbulenceIntensity;
                float TurbulenceYaw = FMath::PerlinNoise3D(FVector(0.0f, 0.0f, CurrentTimeAccumulator * 3.0f)) * RidgeParams.RotorTurbulenceIntensity;

                TurbulenceRotorTorque = FVector(TurbulenceRoll, TurbulencePitch, TurbulenceYaw) * AirDensity * DynamicWindSpeedMs;
            }
        }

        // Combine Total Environmental Wind
        CurrentLocalWindVectorWorld = MacroWindVectorWorld + RidgeVerticalWindVector;

        // 3. AERODYNAMIC FORCE INTEGRATION (Relative Airspeed $V_{rel} = V_{bird} - V_{wind}$)
        FVector ApparentWindVelocityWorld = LinearVelocityWorld - CurrentLocalWindVectorWorld;
        float ApparentAirspeed = ApparentWindVelocityWorld.Size();

        if (ApparentAirspeed < 0.1f)
        {
            CalculatedWindAerodynamicForceWorld = FVector::ZeroVector;
            CalculatedWindTurbulenceTorqueWorld = FVector::ZeroVector;
            return;
        }

        FVector ApparentWindDirection = ApparentWindVelocityWorld.GetUnsafeNormal();
        FVector RightVector = RigidBodyMesh->GetRightVector();

        // Drag opposes apparent wind direction
        float DynamicPressure = 0.5f * AirDensity * (ApparentAirspeed * ApparentAirspeed);
        float WindDragMagnitude = DynamicPressure * WingAreaSqMeters * 0.05f; // Wind pressure force

        FVector WindDragForceWorld = -ApparentWindDirection * WindDragMagnitude;

        CalculatedWindAerodynamicForceWorld = WindDragForceWorld;
        CalculatedWindTurbulenceTorqueWorld = TurbulenceRotorTorque;

        // 4. APPLY PHYSICAL MOUNTAIN WIND FORCES & TURBULENCE MOMENTS
        RigidBodyMesh->AddForce(CalculatedWindAerodynamicForceWorld, NAME_None, false);
        RigidBodyMesh->AddTorqueInRadians(CalculatedWindTurbulenceTorqueWorld, NAME_None, true);
    }
};
