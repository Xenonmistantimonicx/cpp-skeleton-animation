#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "AAABirdSpeedLiftEngine.generated.h"

USTRUCT(BlueprintType)
struct FAerodynamicWingProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wing Geometry")
    float WingspanMeters = 2.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wing Geometry")
    float SurfaceAreaSquareMeters = 0.92f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wing Geometry")
    float MeanAerodynamicChord = 0.38f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airfoil Parameters")
    float ZeroAlphaLiftCoefficient = 0.32f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airfoil Parameters")
    float LiftCurveSlopePerRad = 5.85f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airfoil Parameters")
    float OswaldEfficiencyFactor = 0.88f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airfoil Parameters")
    float MaxDynamicLiftThreshold = 850.0f;
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAABirdSpeedLiftEngine : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAABirdSpeedLiftEngine();

protected:
    virtual void BeginPlay() override;

public:    
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Atmosphere")
    float AirDensityKgPerM3 = 1.225f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Atmosphere")
    float GravityScaleOverride = 9.81f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Wing Profile")
    FAerodynamicWingProfile WingProfile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Performance")
    int32 PhysicsSubStepsPerFrame = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Velocity Scaling")
    float SpeedToLiftMultiplier = 1.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Velocity Scaling")
    float PitchAoASensitivity = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Damping & Stability")
    float DynamicLiftDampingFactor = 0.42f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentGroundSpeedKmh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentAirSpeedMs;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentDynamicPressureQ;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float EffectiveLiftForceNewtons;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    float CurrentAngleOfAttackDegrees;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedLiftVectorWorld;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Telemetry")
    FVector CalculatedInducedDragVectorWorld;

private:
    APawn* OwningPawn;
    UPrimitiveComponent* RigidBodyMesh;

    FVector PreviousVelocity;
    FVector FilteredLiftForce;

    void ExecuteAerodynamicSubstep(float SubstepDeltaTime);
    float ComputeLiftCoefficient(float AngleOfAttackRads) const;
    float ComputeInducedDragCoefficient(float LiftCoefficient) const;
    void ApplyDynamicForces(const FVector& LiftVector, const FVector& DragVector);
    void UpdateTelemetryData(const FVector& LocalVelocity);
};
