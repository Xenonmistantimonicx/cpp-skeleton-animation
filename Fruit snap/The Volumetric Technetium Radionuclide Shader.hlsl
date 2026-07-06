// AAA-Tier Ultra-Complex Technetium Radionuclide Phase-Change Shader
void EvaluateTechnetiumDecay_float(
    float3 RawPlatinumSilverMetal, // Polished, reflective silver transition base (RGB: 0.85, 0.85, 0.87)
    float3 MatteDioxideSlate,      // Dull, dark charcoal-grey tarnish film (RGB: 0.25, 0.26, 0.28)
    float3 HeptoxidePinkPurple,    // Crystalline heptoxide oxide crust (RGB: 0.58, 0.28, 0.52)
    float3 CausticAcidPinkTrail,   // Weeping pertechnetic acid fluid channels (RGB: 0.85, 0.45, 0.65)
    float3 GeometricNormalWS,      // Vertex surface normal vectors in world space
    float2 UV,                     // Main texture layout coordinates
    float GlobalDecayFactor,       // Master progress float from CPU (0.0 to 1.0)
    float RadioactiveFlickerTime,  // Dynamic game time scalar layered with isotopic noise
    float CrystallineCrustNoise,   // High-frequency cellular noise representing heptoxide flakes
    float AcidWeepNoise,           // Smooth, vertically biased directional noise for liquid weeping
    out float3 OutFinalAlbedo,     // Final computed PBR base color
    out float OutMetallicPct,      // Dynamic metallic attenuation scale
    out float OutSurfaceRough,     // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS,  // Volumetric geometric displacement vector
    out float3 OutRadioactiveGlow  // Isotopic beta-decay self-irradiation luminescence
)
{
    // --- STEP 1: RADIONUCLIDE PHASE MASKING ---
    // Stage A: Early dioxide tarnish dulls the transition metal silver (0.0 to 0.25)
    float tarnishPhase = smoothstep(0.0, 0.22, GlobalDecayFactor);
    float activeDioxideMask = saturate(tarnishPhase * (1.0 - smoothstep(0.3, 0.65, GlobalDecayFactor)));

    // Stage B: Pink-purple crystalline heptoxide crust climbs the surface (0.22 to 0.75)
    float heptoxideProgression = smoothstep(0.2, 0.55, GlobalDecayFactor) * (1.0 - smoothstep(0.65, 0.9, GlobalDecayFactor));
    float activeCrustMask = saturate((heptoxideProgression * 1.6) - (CrystallineCrustNoise * 0.35));

    // Stage C: Terminal pertechnetic acid weeping dissolves the asset (0.6 to 1.0)
    float terminalAcidPhase = smoothstep(0.55, 0.95, GlobalDecayFactor);
    float activeAcidMask = saturate((terminalAcidPhase * 1.8) - (AcidWeepNoise * 0.3));

    // --- STEP 2: MULTI-PHASE ALBEDO COMPOSITING ---
    // Platinum-Silver -> Charcoal Slate -> Pink-Purple Crystals -> Caustic Fluid Trails
    float3 metalToSlate = lerp(RawPlatinumSilverMetal, MatteDioxideSlate, tarnishPhase);
    float3 crustComposite = lerp(metalToSlate, HeptoxidePinkPurple, activeCrustMask);
    
    // AAA Detail: Blending an erratic dark-yellow boundary ring where the heptoxide begins to sublime
    float sublimingEdge = smoothstep(0.02, 0.15, activeCrustMask) * (1.0 - smoothstep(0.15, 0.35, activeCrustMask));
    crustComposite = lerp(crustComposite, float3(0.78, 0.68, 0.22), sublimingEdge * 0.5);

    // Final albedo conversion shifting into translucent weeping fluid paths
    OutFinalAlbedo = lerp(crustComposite, CausticAcidPinkTrail, activeAcidMask);

    // --- STEP 3: PBR REFLECTION SPECTRUM ---
    // Technetium is an excellent transition conductor. Oxides are dielectric salts; acid is a glassy fluid.
    float nativeMetal = lerp(1.0, 0.2, tarnishPhase); 
    OutMetallicPct = lerp(nativeMetal, 0.0, max(activeCrustMask, activeAcidMask));

    // Polished metal (Roughness 0.12) turns to a dry satin (0.65), jumps to a coarse,
    // scattering crystal plate (0.88), and collapses into an ultra-slick fluid run (0.04)
    float baseRough = lerp(0.12, 0.65, tarnishPhase);
    float crustRough = lerp(baseRough, 0.88, activeCrustMask);
    OutSurfaceRough = lerp(crustRough, 0.04, activeAcidMask);

    // --- STEP 4: RADIONUCLIDE VOLUMETRIC PIT COMPOSITING ---
    // Crystals expand outward (+2.5cm) while acid channels erode the parent metal inward (-3.0cm)
    float crystalSwell = CrystallineCrustNoise * activeCrustMask * 0.025;
    float acidErosionRun = activeAcidMask * (AcidWeepNoise * -0.03);
    OutVertexOffsetWS = GeometricNormalWS * (crystalSwell + acidErosionRun);

    // --- STEP 5: BETA-DECAY ISOTOPIC GLOW SYSTEM ---
    // Simulates an erratic self-irradiation energy field centered along highly concentrated heptoxide cells.
    float erraticPulse = sin(RadioactiveFlickerTime * 8.0 + UV.x * 30.0) * cos(RadioactiveFlickerTime * 5.0 + UV.y * 30.0);
    float glowMask = saturate(activeCrustMask * (0.6 + erraticPulse * 0.4));
    OutRadioactiveGlow = glowMask * float3(0.35, 0.85, 0.98) * 2.2; // Eerie pale-cyan Cherenkov / Beta radiation glow simulation
}
