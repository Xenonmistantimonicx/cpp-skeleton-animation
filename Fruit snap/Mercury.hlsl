// AAA-Tier Ultra-Complex Liquid Mercury Surface Tension & Oxide Shader
void EvaluateMercuryFluidDecay_float(
    float3 RawLiquidMirrorSilver, // Flawless metallic silver reflection base (RGB: 0.95, 0.96, 0.97)
    float3 DullAmalgamMushGrey,  // Stiff, frosty silver-grey alloy crust (RGB: 0.45, 0.47, 0.48)
    float3 CalcinedOxideRed,      // Crystalline mercuric oxide powder (RGB: 0.85, 0.18, 0.08)
    float3 GeometricNormalWS,     // Vertex surface normal vectors in world space
    float3 CameraViewDirWS,       // Direction vector pointing from pixel to camera position
    float2 UV,                    // Main texture layout coordinates
    float GlobalDecayFactor,      // Master progress float from CPU (0.0 to 1.0)
    float FluidWaveTime,          // Dynamic game time variable passed to drive liquid rippling
    float AmalgamWrinkleNoise,    // High-frequency directional noise representing wrinkled skins
    float OxideCrystallineNoise,  // Sharp fractal noise for powdery oxide topology
    out float3 OutFinalAlbedo,    // Final computed PBR base color
    out float OutMetallicPct,     // Dynamic metallic attenuation scale
    out float OutSurfaceRough,    // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS  // Volumetric geometric fluid rippling displacement vector
)
{
    // --- STEP 1: LIQUID PHASE MASKING ---
    // Stage A: Pristine moving liquid mirror ripples early on (0.0 to 0.25)
    float fluidRipplePhase = 1.0 - smoothstep(0.25, 0.6, GlobalDecayFactor);

    // Stage B: Frosty, stiff amalgam wrinkling takes over mid-timeline (0.25 to 0.7)
    float amalgamPhase = smoothstep(0.2, 0.45, GlobalDecayFactor) * (1.0 - smoothstep(0.55, 0.85, GlobalDecayFactor));
    float activeAmalgamMask = saturate((amalgamPhase * 1.5) - (AmalgamWrinkleNoise * 0.3));

    // Stage C: Solid, bright red calcined oxide powder crust takes over at termination (0.6 to 1.0)
    float terminalOxidePhase = smoothstep(0.55, 0.9, GlobalDecayFactor);
    float activeOxideMask = saturate((terminalOxidePhase * 1.7) - (OxideCrystallineNoise * 0.4));

    // --- STEP 2: MULTI-PHASE ALBEDO BLENDING MATRIX ---
    // Moving Liquid Mirror -> Frosty Amalgam Mush -> Bright Red Crystalline Oxide
    float3 fluidToAmalgam = lerp(RawLiquidMirrorSilver, DullAmalgamMushGrey, activeAmalgamMask);
    float3 baselineComposite = lerp(fluidToAmalgam, CalcinedOxideRed, activeOxideMask);
    
    // AAA Detail: Add a bright orange-yellow reaction rim right where the red oxide consumes the fluid
    float reactionRim = smoothstep(0.02, 0.12, activeOxideMask) * (1.0 - smoothstep(0.12, 0.32, activeOxideMask));
    OutFinalAlbedo = lerp(baselineComposite, float3(0.92, 0.55, 0.05), reactionRim * 0.7);

    // --- STEP 3: PBR REFLECTION MATRICES ---
    // Mercury is an excellent liquid conductor. Amalgam is sub-metallic. Oxide is a pure ceramic dielectric.
    float nativeMetal = lerp(1.0, 0.65, activeAmalgamMask);
    OutMetallicPct = lerp(nativeMetal, 0.0, activeOxideMask);

    // Perfect fluid mirror (Roughness 0.0) transforms into a velvety satin wrinkle mush (0.52),
    // and locks into a highly scattering, coarse, powdery chalky red ceramic crust at termination (0.95)
    float dynamicFluidRough = saturate(0.01 + sin(FluidWaveTime * 2.0 + UV.x * 10.0) * 0.02);
    float amalgamRough = lerp(dynamicFluidRough, 0.52, activeAmalgamMask);
    OutSurfaceRough = lerp(amalgamRough, lerp(0.88, 0.98, OxideCrystallineNoise), activeOxideMask);

    // --- STEP 4: VOLUMETRIC SURFACE TENSION DISPLACEMENT ---
    // Compute fluid wave behaviors that transition into stiff, crystalline expansion crusts.
    float sineRipple = sin(UV.x * 40.0 + FluidWaveTime * 4.0) * cos(UV.y * 40.0 + FluidWaveTime * 3.0) * 0.012;
    float3 liquidMovement = GeometricNormalWS * (sineRipple * fluidRipplePhase);
    
    // Solid red oxide conversion swells up into rigid, crisp crystalline clusters (+3cm)
    float3 oxideSwell = GeometricNormalWS * (OxideCrystallineNoise * activeOxideMask * 0.03);
    
    // Stiff amalgam wrinkling forces jagged micro-creases along normal axes
    float3 amalgamWrinkle = GeometricNormalWS * (AmalgamWrinkleNoise * activeAmalgamMask * 0.006);

    OutVertexOffsetWS = liquidMovement + amalgamWrinkle + oxideSwell;
}
