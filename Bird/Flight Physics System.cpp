#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "AAABirdFlightComponent.generated.h"

UENUM(BlueprintType)
enum class EBirdFlightState : uint8
{
    Grounded     UMETA(DisplayName = "Grounded"),
    Takeoff      UMETA(DisplayName = "Takeoff"),
    Flapping     UMETA(DisplayName = "Flapping Flight"),
    Gliding      UMETA(DisplayName = "Gliding / Soaring"),
    Diving       UMETA(DisplayName = "High-Speed Dive (Stoop)"),
    Stalled      UMETA(DisplayName = "Aerodynamic Stall"),
    Landing      UMETA(DisplayName = "Landing Flare")
};

UCLASS(ClassGroup = (AAA_Physics), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UAAABirdFlightComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAAABirdFlightComponent();

protected:
    virtual void BeginPlay() override;

public:    
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics|Wing Profile")
    float WingArea = 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics|Wing Profile")
    float Wingspan = 2.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics|Mass & Inertia")
    float Mass = 4.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics|Atmosphere")
    float AirDensity = 1.225f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics|Aerodynamics")
    float ZeroAoALiftCoefficient = 0.28f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics|Aerodynamics")
    float LiftSlope = 5.73f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics|Aerodynamics")
    float CriticalStallAngleDeg = 15.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics|Aerodynamics")
    float ParasiticDragCoefficient = 0.025f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics|Thrust Mechanics")
    float MaxFlapThrustForce = 85.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics|Thrust Mechanics")
    float FlapFrequencyHz = 3.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics|Control Dynamics")
    FVector PitchYawRollTorqueSensitivity = FVector(12.0f, 8.0f, 25.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAA Physics|Control Dynamics")
    float DampingFactor = 2.5f;

    UFUNCTION(BlueprintCallable, Category = "AAA Flight Controls")
    void InjectFlightInputs(float PitchInput, float YawInput, float RollInput, float FlapThrustInput);

    UFUNCTION(BlueprintCallable, Category = "AAA Flight Controls")
    void TriggerFlapImpulse();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Flight Telemetry")
    EBirdFlightState CurrentFlightState;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Flight Telemetry")
    float CurrentSpeedKmh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Flight Telemetry")
    float AngleOfAttackDeg;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Flight Telemetry")
    float DynamicNormalizedBanking;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Flight Telemetry")
    FVector ComputedLiftForce;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAA Flight Telemetry")
    FVector ComputedDragForce;

private:
    APawn* OwningPawn;
    UPrimitiveComponent* PhysicsMesh;

    FVector ControlInputVector;
    float FlapImpulseQueue;
    float LastFlapTime;

    void EvaluatePhysicsSubstep(float SubstepDeltaTime);
    float CalculateLiftCoefficient(float AlphaRads) const;
    float CalculateDragCoefficient(float LiftCoeff, float AlphaRads) const;
    void ApplyDynamicControlTorques(float SubstepDeltaTime, const FVector& LocalVelocity);
    void UpdateStateTelemetry(const FVector& LocalVelocity);
};
