#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "AAABirdValleyTurbulenceEngine.generated.h"

UENUM(BlueprintType)
enum class EValleyDiurnalCycle : uint8
{
    Day_Anabatic    UMETA(DisplayName = "Daytime Upslope Thermal Flow"),
    Night_Katabatic UMETA(DisplayName = "Nighttime Downslope Cold Sinking Flow")
};

USTRUCT(BlueprintType)
struct FValleyGeometryParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Geometry")
    FVector ValleyAxisDirection = FVector(1.0f, 0.0f, 0.0f); // Direction along the valley floor

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Geometry")
    float NominalValleyWidthMeters = 250.0f; // Unconstricted entrance width

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Geometry")
    float ConstrictedValleyWidthMeters = 80.0f; // Narrow gorge section (triggers Venturi acceleration)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Geometry")
    float ValleyWallRayDistanceMeters = 300.0f; // Raycast check distance for canyon walls
};

USTRUCT(BlueprintType)
struct FValleyAeroParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Dynamics")
    EValleyDiurnalCycle DiurnalFlowMode = EValleyDiurnalCycle::Day_Anabatic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Dynamics")
    float BaseValleyWindSpeedKmh = 25.0f; // Ambient wind entering valley

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Dynamics")
    float AnabaticThermalUpdraftSpeedMs = 4.8f; // Daytime solar heated updrafts along walls

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Dynamics")
    float KatabaticDowndraftSpeedMs = 6.2f; // Nighttime cold heavy air dumping down valley

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valley Dynamics")
    float WallSeparationEddyIntensity = 5.5f; // Turbulent shear torque near cliff walls
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAABirdValleyTurbulenceEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAABirdValleyTurbulenceEngine()
    {
        PrimaryComponentTick.bCanEverTick = true;
        PrimaryComponentTick.TickGroup = TG_PrePhysics;

        SubStepCount = 8;
        WingspanMeters = 2.4f;
        WingAreaSqMeters = 0.92f;
        AirDensity = 1.225f;

        CurrentTimeAccumulator = 0.0f;
        CurrentVenturiMultiplier = 1.0f;
        CurrentEffectiveValleyWindMs = 0.0f;
        CurrentAnabaticKatabaticVerticalMs = 0.0f;
        bIsInsideConstrictedGorge = false;

        CalculatedValleyAerodynamicForceWorld = FVector::ZeroVector;
        CalculatedWallEddyTorqueWorld = FVector::ZeroVector;
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Valley Environment")
    FValleyGeometryParameters ValleyGeo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Valley Environment")
    FValleyAeroParameters ValleyAero;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentVenturiMultiplier;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentEffectiveValleyWindMs;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentAnabaticKatabaticVerticalMs;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    bool bIsInsideConstrictedGorge;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedValleyAerodynamicForceWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedWallEddyTorqueWorld;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
    {
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (!RigidBodyMesh || !RigidBodyMesh->IsSimulatingPhysics()) return;

        CurrentTimeAccumulator += DeltaTime;

        const int32 Steps = FMath::Clamp(SubStepCount, 1, 16);
        const float SubstepDeltaTime = DeltaTime / static_cast<float>(Steps);

        for (int32 Step = 0; Step < Steps; ++Step)
        {
            EvaluateValleyTurbulencePhysicsSubstep(SubstepDeltaTime);
        }
    }

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    float CurrentTimeAccumulator;

    void EvaluateValleyTurbulencePhysicsSubstep(float SubstepDeltaTime)
    {
        FVector ActorLocation = RigidBodyMesh->GetComponentLocation();
        FVector LinearVelocityWorld = RigidBodyMesh->GetLinearVelocity();

        // 1. CANYON WALL DETECTION (Cross-valley raycasting to compute local channel width)
        FVector NormalizedValleyAxis = ValleyGeo.ValleyAxisDirection.GetSafeNormal();
        FVector ValleyPerpendicularRight = FVector::CrossProduct(NormalizedValleyAxis, FVector::UpVector).GetSafeNormal();

        FHitResult LeftWallHit, RightWallHit;
        FVector TraceStart = ActorLocation;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(GetOwner());

        float LeftWallDist = ValleyGeo.ValleyWallRayDistanceMeters;
        float RightWallDist = ValleyGeo.ValleyWallRayDistanceMeters;

        if (GetWorld()->LineTraceSingleByChannel(LeftWallHit, TraceStart, TraceStart - (ValleyPerpendicularRight * ValleyGeo.ValleyWallRayDistanceMeters * 100.0f), ECC_Visibility, QueryParams))
        {
            LeftWallDist = LeftWallHit.Distance * 0.01f;
        }

        if (GetWorld()->LineTraceSingleByChannel(RightWallHit, TraceStart, TraceStart + (ValleyPerpendicularRight * ValleyGeo.ValleyWallRayDistanceMeters * 100.0f), ECC_Visibility, QueryParams))
        {
            RightWallDist = RightWallHit.Distance * 0.01f;
        }

        float LocalChannelWidthMeters = LeftWallDist + RightWallDist;

        // 2. VENTURI EFFECT CALCULATIONS ($A_1 \cdot V_1 = A_2 \cdot V_2$)
        // Continuity equation: Narrower gorges compress mass flow, exponentially accelerating velocity
        CurrentVenturiMultiplier = FMath::Clamp(ValleyGeo.NominalValleyWidthMeters / FMath::Max(LocalChannelWidthMeters, 10.0f), 1.0f, 4.0f);
        bIsInsideConstrictedGorge = (CurrentVenturiMultiplier > 1.35f);

        float BaseWindMs = (ValleyAero.BaseValleyWindSpeedKmh / 3.6f);
        CurrentEffectiveValleyWindMs = BaseWindMs * CurrentVenturiMultiplier;

        FVector VenturiWindVectorWorld = NormalizedValleyAxis * CurrentEffectiveValleyWindMs;

        // 3. DIURNAL FLOW (Anabatic Thermal vs. Katabatic Drainage Winds)
        CurrentAnabaticKatabaticVerticalMs = 0.0f;
        float ProximityToCliffWallMeters = FMath::Min(LeftWallDist, RightWallDist);

        if (ValleyAero.DiurnalFlowMode == EValleyDiurnalCycle::Day_Anabatic)
        {
            // DAYTIME: Sun bakes cliff walls -> Strong thermal boundary layer currents rushing UP the walls
            float WallProximityFactor = 1.0f - FMath::Clamp(ProximityToCliffWallMeters / 120.0f, 0.0f, 1.0f);
            CurrentAnabaticKatabaticVerticalMs = ValleyAero.AnabaticThermalUpdraftSpeedMs * WallProximityFactor;
        }
        else
        {
            // NIGHTTIME: Cold air sinks off mountain peaks -> Heavy downdraft flushing DOWN the valley floor
            CurrentAnabaticKatabaticVerticalMs = -ValleyAero.KatabaticDowndraftSpeedMs;
        }

        FVector VerticalFlowVectorWorld = FVector(0.0f, 0.0f, CurrentAnabaticKatabaticVerticalMs);

        // 4. WALL SEPARATION & CORNER TURBULENCE EDDIES
        // Boundary layer separation off sharp cliff projections creates violent erratic vortex loops
        FVector WallSeparationTorque = FVector::ZeroVector;

        if (ProximityToCliffWallMeters < 45.0f)
        {
            float WallShearIntensity = (1.0f - (ProximityToCliffWallMeters / 45.0f)) * ValleyAero.WallSeparationEddyIntensity;

            // Turbulent Perlin Noise Vector simulating rotational vortex shear
            float EddyRoll = FMath::PerlinNoise3D(FVector(CurrentTimeAccumulator * 5.0f, ActorLocation.Y * 0.01f, 0.0f));
            float EddyPitch = FMath::PerlinNoise3D(FVector(0.0f, CurrentTimeAccumulator * 5.0f, ActorLocation.Z * 0.01f));
            float EddyYaw = FMath::PerlinNoise3D(FVector(ActorLocation.X * 0.01f, 0.0f, CurrentTimeAccumulator * 5.0f));

            WallSeparationTorque = FVector(EddyRoll, EddyPitch, EddyYaw) * WallShearIntensity * CurrentVenturiMultiplier * AirDensity;
        }

        // 5. AERODYNAMIC FORCE INTEGRATION
        FVector TotalValleyWindVectorWorld = VenturiWindVectorWorld + VerticalFlowVectorWorld;
        FVector RelativeAirspeedVelocityWorld = LinearVelocityWorld - TotalValleyWindVectorWorld;

        float DynamicPressure = 0.5f * AirDensity * RelativeAirspeedVelocityWorld.SizeSquared();
        FVector DragForceWorld = -RelativeAirspeedVelocityWorld.GetSafeNormal() * DynamicPressure * WingAreaSqMeters * 0.08f;

        CalculatedValleyAerodynamicForceWorld = DragForceWorld;
        CalculatedWallEddyTorqueWorld = WallSeparationTorque;

        // 6. APPLY PHYSICAL FORCES
        RigidBodyMesh->AddForce(CalculatedValleyAerodynamicForceWorld, NAME_None, false);
        RigidBodyMesh->AddTorqueInRadians(CalculatedWallEddyTorqueWorld, NAME_None, true);
    }
};
