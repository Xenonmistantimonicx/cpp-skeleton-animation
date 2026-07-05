// AAA-Tier Ultra-Complex Plutonium Surface Passivation & Thermal Fissure Shader
void EvaluatePlutoniumDecay_float(
    float3 RawSilverMetal,       // Bright, specular silver base alloy (RGB: 0.76, 0.77, 0.78)
    float3 OliveUranylOxide,     // Sickly intermediate yellow/olive-green film (RGB: 0.42, 0.48, 0.22)
    float3 DioxideCharcoalBlack, // Matte, powdery dioxide finish (RGB: 0.12, 0.13, 0.13)
    float3 SurfaceNormalWS,      // Vertex surface normal vectors in world space
    float2 UV,                   // Object layout mapping coordinates
    float GlobalDecayFactor,     // Master decay floating point from CPU (0.0 to 1.0)
    float MicroPittingNoise,     // High-frequency fractal noise representing oxide corrosion pits
    float FlakeDisplaceSample,   // Brittle cracking noise map for scaling oxide layers
    out float3 OutFinalAlbedo,   // Final computed PBR base color
    out float OutMetallicPct,    // Dynamic metallic attenuation scale
    out float OutSurfaceRough,   // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS, // Volumetric geometric displacement vector
    out float3 OutThermalGlow     // Mass-driven alpha-decay heat emission map
)
{
    // --- STEP 1: MATERIAL STAGE MASKING ---
    // Stage A: Quick conversion from silver to olive-green patina peaks early (0.0 to 0.4)
    float olivePhase = smoothstep(0.0, 0.4, GlobalDecayFactor) * (1.0 - smoothstep(0.45, 0.85, GlobalDecayFactor));
    
    // Stage B: Heavy, loose charcoal dioxide shell takes over completely at later stages (0.45 to 1.0)
    float dioxideProgression = saturate((GlobalDecayFactor * 1.8) - 0.5);
    float activeBlackShellMask = saturate((dioxideProgression * 1.5) - (MicroPittingNoise * 0.3));

    // --- STEP 2: MULTI-LAYER COLOR MIXING MATRIX ---
    // Polished Silver -> Dull Olive/Yellow Patina -> Matte Charcoal Dioxide Black
    float3 silverToOlive = lerp(RawSilverMetal, OliveUranylOxide, smoothstep(0.0, 0.35, GlobalDecayFactor));
    float3 colorComposite = lerp(silverToOlive, DioxideCharcoalBlack, activeBlackShellMask);
    
    // Add micro-pitting coloration variance along oxide boundary borders
    OutFinalAlbedo = lerp(colorComposite, float3(0.22, 0.20, 0.18), olivePhase * MicroPittingNoise * 0.3);

    // --- STEP 3: PBR MOLECULAR TRANSITION ---
    // Pure unoxidized plutonium is an alloy conductor. The dioxide state is a dielectric mineral shell.
    float baseMetallic = lerp(1.0, 0.4, smoothstep(0.0, 0.4, GlobalDecayFactor));
    OutMetallicPct = lerp(baseMetallic, 0.0, activeBlackShellMask);

    // Smooth polished metal (Roughness 0.1) shifts to a waxy, matte finish during intermediate oxidation (0.55),
    // and locks into a highly scattering, rough, completely dry powdery dust surface at termination (0.97).
    float baseRoughness = lerp(0.1, 0.55, smoothstep(0.0, 0.4, GlobalDecayFactor));
    OutSurfaceRough = lerp(baseRoughness, lerp(0.88, 0.99, FlakeDisplaceSample), activeBlackShellMask);

    // --- STEP 4: VOLUMETRIC OXIDE SCALE DISPLACEMENT ---
    // As the metal passivates, the structural volume expands slightly, causing loose flakes to buckle up.
    float macroScaleHeight = FlakeDisplaceSample * activeBlackShellMask * 0.035; // Oxide lifts out up to 3.5cm
    float3 microWarpingWobble = SurfaceNormalWS * (olivePhase * sin(UV.x * 200.0) * 0.002); 
    
    OutVertexOffsetWS = (SurfaceNormalWS * macroScaleHeight) + microWarpingWobble;

    // --- STEP 5: ALPHA-DECAY THERMAL GLOW ---
    // Simulates the physical property of self-heating. Crevices where oxide flakes have cracked open 
    // display a subtle infrared thermal emission signature driven by the master decay scale.
    float creviceCrackMask = smoothstep(0.5, 0.8, FlakeDisplaceSample) * activeBlackShellMask;
    float pulseDriver = sin(UV.y * 30.0 + GlobalDecayFactor * 2.0) * 0.05 + 0.95;
    
    OutThermalGlow = creviceCrackMask * float3(0.88, 0.22, 0.05) * 1.2 * pulseDriver;
}
