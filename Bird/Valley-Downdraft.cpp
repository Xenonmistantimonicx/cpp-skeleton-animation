#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAABirdValleyDowndraftEngine.generated.h"

USTRUCT(BlueprintType)
struct FValleyShadowParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Shadow Mechanics")
    FVector SunDirectionVector = FVector(0.6f, 0.2f, -0.75f).GetSafeNormal(); // Light vector

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Shadow Mechanics")
    float ShadowThermalCollapseRateMs = 5.2f; // Downward sink velocity when entering ridge shadows

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Shadow Mechanics")
    float ShadowShearTurbulenceIntensity = 4.8f; // Roll/pitch kick when crossing sun/shadow boundary
};

USTRUCT(BlueprintType)
struct FKatabaticDrainageParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Katabatic Sinking Dynamics")
    float GroundSurfaceCoolingDeltaCelsius = -8.5f; // Negative temperature differential driving density sink

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Katabatic Sinking Dynamics")
    float PeakKatabaticDowndraftMs = 9.8f; // Max vertical cold-air flushing velocity

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Katabatic Sinking Dynamics")
    float ColdAirPoolThicknessMeters = 80.0f; // Height of dense stagnant cold layer at valley bottom

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Katabatic Sinking Dynamics")
    float ValleyFloorSinkingSuctionMultiplier = 1.45f; // Extra downward pull near valley floor
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAABirdValleyDowndraftEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAABirdValleyDowndraftEngine()
    {
        PrimaryComponentTick.bCanEverTick = true;
        PrimaryComponentTick.TickGroup = TG_PrePhysics;

        SubStepCount = 8;
        WingspanMeters = 2.4f;
        WingAreaSqMeters = 0.92f;
        AirDensityStandard = 1.225f;

        CurrentTimeAccumulator = 0.0f;
        CurrentBirdHeightAboveValleyFloorMeters = 0.0f;
        CurrentCalculatedDowndraftSpeedMs = 0.0f;
        bIsInRidgeShadow = false;
        bIsInColdAirPool = false;

        CalculatedValleyDowndraftForceWorld = FVector::ZeroVector;
        CalculatedValleyDowndraftTorqueWorld = FVector::ZeroVector;
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Valley Downdraft")
    FValleyShadowParameters ShadowConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Valley Downdraft")
    FKatabaticDrainageParameters KatabaticConfig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentBirdHeightAboveValleyFloorMeters;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentCalculatedDowndraftSpeedMs;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    bool bIsInRidgeShadow;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    bool bIsInColdAirPool;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedValleyDowndraftForceWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedValleyDowndraftTorqueWorld;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
    {
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (!RigidBodyMesh || !RigidBodyMesh->IsSimulatingPhysics()) return;

        CurrentTimeAccumulator += DeltaTime;

        const int32 Steps = FMath::Clamp(SubStepCount, 1, 16);
        const float SubstepDeltaTime = DeltaTime / static_cast<float>(Steps);

        for (int32 Step = 0; Step < Steps; ++Step)
        {
            EvaluateValleyDowndraftPhysicsSubstep(SubstepDeltaTime);
        }
    }

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    float CurrentTimeAccumulator;

    void EvaluateValleyDowndraftPhysicsSubstep(float SubstepDeltaTime)
    {
        FVector ActorLocation = RigidBodyMesh->GetComponentLocation();
        FVector LinearVelocityWorld = RigidBodyMesh->GetLinearVelocity();

        // 1. TERRAIN & VALLEY FLOOR DETECTION (Vertical Downward Raycast)
        FHitResult ValleyHit;
        FVector TraceStart = ActorLocation;
        FVector TraceEnd = ActorLocation - FVector(0.0f, 0.0f, 100000.0f); // 1km downcast

        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(GetOwner());

        float ValleyFloorAltitudeMeters = 0.0f;
        bool bGroundFound = GetWorld()->LineTraceSingleByChannel(ValleyHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

        if (bGroundFound)
        {
            ValleyFloorAltitudeMeters = ValleyHit.ImpactPoint.Z * 0.01f;
        }

        CurrentBirdHeightAboveValleyFloorMeters = FMath::Max(0.0f, (ActorLocation.Z * 0.01f) - ValleyFloorAltitudeMeters);

        // 2. SHADOW-INDUCED THERMAL COLLAPSE CHECK (Raycast to Sun)
        // Checks if ridge blocking sun causes sudden negative buoyancy
        FHitResult SunShadowHit;
        FVector SunTraceEnd = ActorLocation - (ShadowConfig.SunDirectionVector * 200000.0f); // 2km trace toward sun

        bIsInRidgeShadow = GetWorld()->LineTraceSingleByChannel(SunShadowHit, TraceStart, SunTraceEnd, ECC_Visibility, QueryParams);

        float ShadowDowndraftSpeedMs = 0.0f;
        if (bIsInRidgeShadow)
        {
            // Rapid cooling in mountain shadows causes air to instantly sink
            ShadowDowndraftSpeedMs = ShadowConfig.ShadowThermalCollapseRateMs;
        }

        // 3. KATABATIC COLD AIR DRAINAGE & POOL SINKING
        bIsInColdAirPool = (CurrentBirdHeightAboveValleyFloorMeters <= KatabaticConfig.ColdAirPoolThicknessMeters);

        float KatabaticDowndraftSpeedMs = 0.0f;
        if (bIsInColdAirPool)
        {
            // Negatively buoyant cold air accelerates downward as it approaches valley floor
            float SinkingProximityRatio = 1.0f - (CurrentBirdHeightAboveValleyFloorMeters / KatabaticConfig.ColdAirPoolThicknessMeters);
            
            KatabaticDowndraftSpeedMs = KatabaticConfig.PeakKatabaticDowndraftMs * 
                FMath::Pow(SinkingProximityRatio, 1.2f) * KatabaticConfig.ValleyFloorSinkingSuctionMultiplier;
        }

        // Net Downdraft Velocity
        CurrentCalculatedDowndraftSpeedMs = ShadowDowndraftSpeedMs + KatabaticDowndraftSpeedMs;

        // 4. BOUNDARY LAYER DOWNDRAFT SHEAR TURBULENCE
        FVector DowndraftTorqueWorld = FVector::ZeroVector;

        if (CurrentCalculatedDowndraftSpeedMs > 0.1f)
        {
            // Turbulent shear experienced when passing through shadow line or plunging into cold air pool
            float NoiseTime = CurrentTimeAccumulator * 5.0f;
            float RollNoise = FMath::PerlinNoise3D(FVector(NoiseTime, ActorLocation.Y * 0.01f, 0.0f));
            float PitchNoise = FMath::PerlinNoise3D(FVector(0.0f, NoiseTime, ActorLocation.Z * 0.01f));
            float YawNoise = FMath::PerlinNoise3D(FVector(ActorLocation.X * 0.01f, 0.0f, NoiseTime));

            float DynamicDowndraftPressure = 0.5f * AirDensityStandard * FMath::Square(CurrentCalculatedDowndraftSpeedMs);
            float TurbulenceScalar = DynamicDowndraftPressure * WingAreaSqMeters * ShadowConfig.ShadowShearTurbulenceIntensity;

            DowndraftTorqueWorld = FVector(RollNoise, PitchNoise, YawNoise) * TurbulenceScalar;
        }

        // 5. AERODYNAMIC FORCE INTEGRATION
        // Downdraft produces downward relative wind velocity vector
        FVector DowndraftWindVectorWorld = FVector(0.0f, 0.0f, -CurrentCalculatedDowndraftSpeedMs);
        FVector ApparentWindVelocityWorld = LinearVelocityWorld - DowndraftWindVectorWorld;

        float DynamicPressure = 0.5f * AirDensityStandard * ApparentWindVelocityWorld.SizeSquared();
        
        // Downward force acting on bird's wing surface
        float DowndraftForceScalar = 0.5f * AirDensityStandard * FMath::Square(CurrentCalculatedDowndraftSpeedMs) * WingAreaSqMeters * 1.65f;

        CalculatedValleyDowndraftForceWorld = FVector(0.0f, 0.0f, -DowndraftForceScalar);
        CalculatedValleyDowndraftTorqueWorld = DowndraftTorqueWorld;

        // 6. APPLY PHYSICAL FORCES TO RIGID BODY
        RigidBodyMesh->AddForce(CalculatedValleyDowndraftForceWorld, NAME_None, false);
        RigidBodyMesh->AddTorqueInRadians(CalculatedValleyDowndraftTorqueWorld, NAME_None, true);
    }
};
