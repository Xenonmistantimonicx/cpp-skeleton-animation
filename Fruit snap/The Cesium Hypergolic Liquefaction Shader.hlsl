// AAA-Tier Ultra-Complex Cesium Phase-Change Hypergolic Shader
void EvaluateCesiumDecay_float(
    float3 RawGoldenMetalBase,    // Pristine pale golden-silver metal (RGB: 0.88, 0.84, 0.68)
    float3 OrangeSuperoxideAshes, // Blistering, burnt superoxide crust (RGB: 0.82, 0.42, 0.05)
    float3 TranslucentCausticSlag,// Dissolved caustic hydroxide fluid (RGB: 0.44, 0.42, 0.35)
    float3 GeometricNormalWS,     // Vertex surface normal vectors in world space
    float2 UV,                    // Main texture layout coordinates
    float GlobalDecayFactor,      // Master progress float from CPU (0.0 to 1.0)
    float FluidWaveTime,          // Game time variable driving liquid waves
    float PyrophoricBlisterNoise, // Cellular Voronoi noise representing boiling, burning crusts
    float DeliquescenceMeltNoise, // Smooth, fluid organic noise for melting boundaries
    out float3 OutFinalAlbedo,    // Final computed PBR base color
    out float OutMetallicPct,     // Dynamic metallic attenuation scale
    out float OutSurfaceRough,    // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS, // Volumetric geometric meltdown displacement vector
    out float3 OutAlkaliEmission  // Pyrophoric lilac combustion flame glow
)
{
    // --- STEP 1: HYPERGOLIC REACTION PHASE MASKING ---
    // Stage A: Golden metal melts and ripples early on (0.0 to 0.3)
    float liquidRipplePhase = 1.0 - smoothstep(0.3, 0.65, GlobalDecayFactor);

    // Stage B: Spontaneous ignition and orange superoxide ash crust formation (0.2 to 0.7)
    float ignitionPhase = smoothstep(0.15, 0.4, GlobalDecayFactor) * (1.0 - smoothstep(0.6, 0.85, GlobalDecayFactor));
    float activeCrustMask = saturate((ignitionPhase * 1.6) - (PyrophoricBlisterNoise * 0.35));

    // Stage C: Terminal deliquescent melting into liquid hydroxide (0.55 to 1.0)
    float liquefactionPhase = smoothstep(0.5, 0.95, GlobalDecayFactor);
    float activeMeltMask = saturate((liquefactionPhase * 1.5) - (DeliquescenceMeltNoise * 0.3));

    // --- STEP 2: MULTI-PHASE ALBEDO COLOR BLENDING ---
    // Pale Golden Mirror -> Blistering Orange Superoxide -> Translucent Hydroxide Liquid
    float3 crustComposite = lerp(RawGoldenMetalBase, OrangeSuperoxideAshes, activeCrustMask);
    
    // AAA Detail: Inject a dark reddish-brown reaction edge right at the burning threshold
    float reactionEdge = smoothstep(0.02, 0.12, activeCrustMask) * (1.0 - smoothstep(0.12, 0.35, activeCrustMask));
    crustComposite = lerp(crustComposite, float3(0.32, 0.12, 0.08), reactionEdge * 0.7);

    // Final composite shifting into the pooling caustic liquid slag
    OutFinalAlbedo = lerp(crustComposite, TranslucentCausticSlag, activeMeltMask);

    // --- STEP 3: PBR REFLECTION SPECTRUM ---
    // Cesium metal is highly reflective. Superoxide is a rough dielectric; hydroxide is a glassy fluid.
    float nativeMetal = lerp(1.0, 0.0, activeCrustMask);
    OutMetallicPct = lerp(nativeMetal, 0.0, activeMeltMask);

    // Liquid gold mirror (Roughness ~0.02 with wave offsets) turns into an intensely rough
    // bubbling ash shell (0.95), then collapses into an ultra-slick fluid surface (0.04)
    float sineWave = saturate(0.02 + sin(FluidWaveTime * 2.5 + UV.x * 12.0) * 0.02) * liquidRipplePhase;
    float crustRough = lerp(0.35 + sineWave, 0.95, activeCrustMask);
    OutSurfaceRough = lerp(crustRough, 0.04, activeMeltMask);

    // --- STEP 4: VOLUMETRIC MELTDOWN & SLUMP ---
    // Compute dynamic fluid waves that transition into a blistering swell, before slumping flat.
    float fluidRipple = sin(UV.x * 35.0 + FluidWaveTime * 5.0) * cos(UV.y * 35.0 + FluidWaveTime * 4.0) * 0.01 * liquidRipplePhase;
    float blisterSwell = PyrophoricBlisterNoise * activeCrustMask * 0.035;
    float liquefactionSlump = activeMeltMask * (DeliquescenceMeltNoise * 0.015 - 0.05);
    
    // Add micro-boiling shivers to liquefied nodes
    float boilingWobble = sin(UV.y * 300.0 + FluidWaveTime * 20.0) * activeMeltMask * 0.002;
    OutVertexOffsetWS = GeometricNormalWS * (fluidRipple + blisterSwell + liquefactionSlump + boilingWobble);

    // --- STEP 5: HYPERGOLIC LILAC COMPULSIVE EMISSION ---
    // Emit a highly characteristic reddish-violet/lilac flame glow along burning boundaries.
    float flameGlowMask = smoothstep(0.1, 0.6, PyrophoricBlisterNoise) * ignitionPhase;
    OutAlkaliEmission = flameGlowMask * float3(0.72, 0.35, 0.98) * 5.0; // High HDR glow intensity
}
