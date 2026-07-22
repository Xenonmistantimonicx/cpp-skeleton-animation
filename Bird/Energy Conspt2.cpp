// Copyright AAA Studios, 2026. All Rights Reserved.

#include "AAAUnifiedFlightEnergyEngine.h"

UAAAUnifiedFlightEnergyEngine::UAAAUnifiedFlightEnergyEngine()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;

    SubStepCount = 8;
    BirdMassKg = 1.2f;            // 1.2 kg falcon/hawk
    WingspanMeters = 1.1f;
    WingAreaSqMeters = 0.22f;

    PitchInput = 0.0f;
    ThrottleFlapInput = 0.0f;

    CurrentFlapPowerDemandWatts = 0.0f;
    InstantaneousEnergyLossRateWatts = 0.0f;
    InitialAltitudeReferenceMeters = 0.0;
    NetCalculatedForceWorld = FVector::ZeroVector;
}

void UAAAUnifiedFlightEnergyEngine::BeginPlay()
{
    Super::BeginPlay();

    OwningPawn = Cast<APawn>(GetOwner());
    if (OwningPawn)
    {
        RigidBodyMesh = Cast<UPrimitiveComponent>(OwningPawn->GetRootComponent());
        if (RigidBodyMesh && RigidBodyMesh->IsSimulatingPhysics())
        {
            RigidBodyMesh->SetEnableGravity(true);
            RigidBodyMesh->SetMassOverrideOrDefault(NAME_None, BirdMassKg, true);
            InitialAltitudeReferenceMeters = RigidBodyMesh->GetComponentLocation().Z * 0.01; // Convert cm to meters
        }
    }
}

void UAAAUnifiedFlightEnergyEngine::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!RigidBodyMesh || !RigidBodyMesh->IsSimulatingPhysics()) return;

    const int32 Steps = FMath::Clamp(SubStepCount, 1, 16);
    const float SubstepDeltaTime = DeltaTime / static_cast<float>(Steps);

    for (int32 Step = 0; Step < Steps; ++Step)
    {
        CalculateSubstepEnergyConservation(SubstepDeltaTime);
    }
}

void UAAAUnifiedFlightEnergyEngine::CalculateSubstepEnergyConservation(float SubstepDeltaTime)
{
    FTransform ActorTransform = RigidBodyMesh->GetComponentTransform();
    FVector Location = ActorTransform.GetLocation();
    FVector LinearVelocity = RigidBodyMesh->GetLinearVelocity();
    FVector ForwardVector = ActorTransform.GetRotation().GetForwardVector();
    FVector UpVector = ActorTransform.GetRotation().GetUpVector();

    float AirspeedMs = LinearVelocity.Size();
    FVector VelocityNormalized = (AirspeedMs > 0.05f) ? (LinearVelocity / AirspeedMs) : ForwardVector;

    // =========================================================================
    // 1. MECHANICAL ENERGY TRACKING ($E_k, E_p, E_{total}$)
    // =========================================================================
    const double GravityAcc = 9.81;
    double CurrentAltitudeMeters = Location.Z * 0.01; // UE units (cm) -> meters

    Mechanics.KineticEnergyJoules = 0.5 * static_cast<double>(BirdMassKg) * FMath::Square(static_cast<double>(AirspeedMs));
    Mechanics.GravitationalPotentialEnergyJoules = static_cast<double>(BirdMassKg) * GravityAcc * (CurrentAltitudeMeters - InitialAltitudeReferenceMeters);
    Mechanics.TotalMechanicalEnergyJoules = Mechanics.KineticEnergyJoules + Mechanics.GravitationalPotentialEnergyJoules;

    // =========================================================================
    // 2. METABOLIC FLAPPING DYNAMICS & CHEMICAL ENERGY DRAIN ($P_{chem} \to P_{thrust} + P_{heat}$)
    // =========================================================================
    float MechanicalThrustForceN = 0.0f;
    CurrentFlapPowerDemandWatts = 0.0f;

    if (ThrottleFlapInput > 0.01f && Metabolism.MetabolicEnergyReserveJoules > 0.0)
    {
        // Maximum Mechanical Power Output derived from muscle mass scaling ($P_{max} \approx 80-120 W/kg$)
        float MaxMechanicalPowerWatts = BirdMassKg * 95.0f; // ~114 Watts max for 1.2kg bird
        
        // Fatigue degradation reduces power output capacity
        float EffectiveMechanicalPowerWatts = MaxMechanicalPowerWatts * ThrottleFlapInput * (1.0f - (0.65f * Metabolism.LacticAcidFatigueFactor));

        // Chemical Metabolic Power Input Required: $P_{chem} = \frac{P_{mech}}{\eta}$
        float ChemicalPowerDemandWatts = EffectiveMechanicalPowerWatts / FMath::Max(Metabolism.MuscleMechanicalEfficiency, 0.05f);

        // Deplete Metabolic Energy Reserve ($E_{chem} = E_{chem} - P_{chem} \cdot \Delta t$)
        double EnergyDepletedThisSubstep = static_cast<double>(ChemicalPowerDemandWatts) * static_cast<double>(SubstepDeltaTime);
        Metabolism.MetabolicEnergyReserveJoules = FMath::Max(0.0, Metabolism.MetabolicEnergyReserveJoules - EnergyDepletedThisSubstep);

        // Calculate Mechanical Thrust Force: $F_{thrust} = \frac{P_{mech}}{v}$
        float EffectiveAirspeed = FMath::Max(AirspeedMs, 3.0f); // Avoid divide-by-zero at low speed
        MechanicalThrustForceN = EffectiveMechanicalPowerWatts / EffectiveAirspeed;

        // Thermal Waste Heat Generation ($P_{heat} = P_{chem} - P_{mech}$)
        float HeatPowerGenerationWatts = ChemicalPowerDemandWatts - EffectiveMechanicalPowerWatts;
        
        // Core Temperature Increase ($d T = \frac{Q}{m \cdot c}$)
        float SpecificHeatCapacityMuscle = 3470.0f; // $J / (kg \cdot K)$
        float TempIncreaseKelvin = (HeatPowerGenerationWatts * SubstepDeltaTime) / (BirdMassKg * SpecificHeatCapacityMuscle);
        Metabolism.CoreTemperatureKelvin += TempIncreaseKelvin;

        // Lactic Acid Accumulation during hard flapping
        Metabolism.LacticAcidFatigueFactor = FMath::Clamp(Metabolism.LacticAcidFatigueFactor + (0.08f * ThrottleFlapInput * SubstepDeltaTime), 0.0f, 1.0f);

        CurrentFlapPowerDemandWatts = ChemicalPowerDemandWatts;
    }
    else
    {
        // Basal Metabolic Drain during gliding/resting
        double BasalDrainJoules = static_cast<double>(Metabolism.BasalMetabolicRateWatts) * static_cast<double>(SubstepDeltaTime);
        Metabolism.MetabolicEnergyReserveJoules = FMath::Max(0.0, Metabolism.MetabolicEnergyReserveJoules - BasalDrainJoules);

        // Lactic Acid Recovery during Glide
        Metabolism.LacticAcidFatigueFactor = FMath::Clamp(Metabolism.LacticAcidFatigueFactor - (0.04f * SubstepDeltaTime), 0.0f, 1.0f);
    }

    // Thermal Cooling / Heat Dissipation to Environment ($T_{env} = 288.15 K / 15^\circ C$)
    float EnvironmentTempKelvin = 288.15f;
    float TemperatureDelta = Metabolism.CoreTemperatureKelvin - EnvironmentTempKelvin;
    Metabolism.CoreTemperatureKelvin -= (TemperatureDelta * Metabolism.ThermalCoolingRateCoeff * SubstepDeltaTime);

    // =========================================================================
    // 3. AERODYNAMIC DRAG LOSSES & ENERGY DISSIPATION ($P_{drag} = F_{drag} \cdot v$)
    // =========================================================================
    float AirDensity = 1.225f;
    float DynamicPressure = 0.5f * AirDensity * FMath::Square(AirspeedMs);

    // Dynamic Lift & Drag Coefficients
    float AngleOfAttackRad = FMath::Acos(FMath::Clamp(FVector::DotProduct(VelocityNormalized, ForwardVector), -1.0f, 1.0f));
    float LiftCoeff = 0.4f + (2.0f * PI * AngleOfAttackRad);
    
    float AspectRatio = FMath::Square(WingspanMeters) / WingAreaSqMeters;
    float InducedDragCoeff = FMath::Square(LiftCoeff) / (PI * AspectRatio * 0.8f);
    float ParasiticDragCoeff = 0.03f;
    float TotalDragCoeff = ParasiticDragCoeff + InducedDragCoeff;

    float LiftForceN = DynamicPressure * WingAreaSqMeters * LiftCoeff;
    float DragForceN = DynamicPressure * WingAreaSqMeters * TotalDragCoeff;

    // Aerodynamic Energy Dissipation Rate (Watts lost to surrounding fluid turbulences)
    Mechanics.AerodynamicPowerDissipationWatts = DragForceN * AirspeedMs;

    InstantaneousEnergyLossRateWatts = Mechanics.AerodynamicPowerDissipationWatts + CurrentFlapPowerDemandWatts;

    // =========================================================================
    // 4. VECTORIAL FORCE APPLICATION
    // =========================================================================
    FVector LiftForceVector = UpVector * LiftForceN;
    FVector DragForceVector = -VelocityNormalized * DragForceN;
    FVector ThrustForceVector = ForwardVector * MechanicalThrustForceN;

    NetCalculatedForceWorld = LiftForceVector + DragForceVector + ThrustForceVector;

    // Apply Physical Force
    RigidBodyMesh->AddForce(NetCalculatedForceWorld, NAME_None, false);
}
