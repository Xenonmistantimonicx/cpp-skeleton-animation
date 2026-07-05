// AAA-Tier Ultra-Complex Titanium Thermal Iridescence & Ceramic Scale Shader
void EvaluateTitaniumDecay_float(
    float3 RawSilverTitanium,    // Polished, bright aerospace silver base (RGB: 0.78, 0.80, 0.82)
    float3 WhiteRutileScale,     // Dry, chalky, dead-white titanium dioxide crust (RGB: 0.94, 0.94, 0.92)
    float3 SurfaceNormalWS,      // Vertex surface normal vectors in world space
    float3 CameraViewDirWS,      // Direction vector pointing from pixel to camera position
    float2 UV,                   // Main texture coordinates
    float GlobalDecayFactor,     // Master progress float from CPU (0.0 to 1.0)
    float ThermalStressNoise,    // Organic grunge noise mapping heat-affected zones
    float ScaleDisplaceSample,   // Brittle flaking noise map for structural ceramic lifting
    out float3 OutFinalAlbedo,   // Final computed PBR base color
    out float OutMetallicPct,    // Dynamic metallic attenuation scale
    out float OutSurfaceRough,   // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS, // Volumetric geometric displacement vector
    out float3 OutThermalGlow    // Incandescent heat residual emission map
)
{
    // --- STEP 1: THERMAL PASSIVATION & BURST MASKS ---
    // Stage A: Iridescent thermal oxide scaling peaks at mid-decay (0.15 to 0.55)
    float heatPhase = smoothstep(0.05, 0.35, GlobalDecayFactor) * (1.0 - smoothstep(0.45, 0.8, GlobalDecayFactor));
    
    // Stage B: Severe ceramic oxide scale buildup tears out at terminal levels (0.5 to 1.0)
    float scalingProgression = saturate((GlobalDecayFactor * 1.8) - 0.65);
    float activeScaleMask = saturate((scalingProgression * 1.5) - (ThermalStressNoise * 0.35));

    // --- STEP 2: AAA THERMAL ANODIZATION SPECTRUM (THIN-FILM) ---
    // Recreates the precise structural wave interference profile of heated titanium.
    // Shifting bands depend on the heat factor profile mixed with camera incident vectors.
    float viewAngleFactor = saturate(dot(SurfaceNormalWS, CameraViewDirWS));
    float waveInterferenceDriver = saturate(heatPhase * 0.7 + viewAngleFactor * 0.3 + ThermalStressNoise * 0.15);
    
    // Procedural color array mimicking real titanium thermal coloration scales
    float3 tStrawYellow = float3(0.85, 0.65, 0.35);
    float3 tPurpleIndigo = float3(0.32, 0.08, 0.75);
    float3 tCyanBlue     = float3(0.05, 0.62, 0.88);
    
    float3 spectrumTransition = lerp(tStrawYellow, tPurpleIndigo, smoothstep(0.1, 0.5, waveInterferenceDriver));
    float3 finalHeatIridescence = lerp(spectrumTransition, tCyanBlue, smoothstep(0.5, 0.9, waveInterferenceDriver));

    // --- STEP 3: MULTI-STAGE LAYER COMPOSITE BLENDING ---
    // Polished Silver -> Vibrant Chromatic Heat Interference -> Chalky Brittle White Rutile Scale
    float3 pristineToHeated = lerp(RawSilverTitanium, finalHeatIridescence, heatPhase);
    OutFinalAlbedo = lerp(pristineToHeated, WhiteRutileScale, activeScaleMask);

    // --- STEP 4: PBR THERMODYNAMIC SHIFTING ---
    // Fresh titanium is a dense metallic element. Rutile oxide scales are high-insulation ceramics.
    float baselineMetallic = lerp(1.0, 0.85, heatPhase);
    OutMetallicPct = lerp(baselineMetallic, 0.0, activeScaleMask);

    // Mirror aerospace surface (Roughness 0.08) shifts to slightly brushed satin during iridescence (0.3),
    // and turns into an intensely scattering, crumbling dry ceramic surface (0.98)
    float baseRoughness = lerp(0.08, 0.30, heatPhase);
    OutSurfaceRough = lerp(baseRoughness, lerp(0.88, 0.99, ScaleDisplaceSample), activeScaleMask);

    // --- STEP 5: VOLUMETRIC SCALE FLAKING GEOMETRY ---
    // Oxidized titanium ceramic scales buckle up and split cleanly away from the metal.
    // Vertices are pushed outward sharply along normal loops, layered with directional shearing.
    float macroScaleHeight = ScaleDisplaceSample * activeScaleMask * 0.045; // Ceramic lifts out up to 4.5cm
    float3 heatBuckleWobble = SurfaceNormalWS * (heatPhase * sin(UV.x * 280.0) * 0.002); // Early thermal expansion warping
    
    OutVertexOffsetWS = (SurfaceNormalWS * macroScaleHeight) + heatBuckleWobble;

    // --- STEP 6: RESIDUAL INCANDESCENT EMISSION ---
    // If decay is driven by high thermal stress, add a hot, molten crimson glow peeking 
    // from under the cracks of the crumbling white rutile scale.
    float internalMeltGlow = smoothstep(0.4, 0.8, ScaleDisplaceSample) * activeScaleMask;
    OutThermalGlow = internalMeltGlow * float3(0.95, 0.18, 0.02) * 2.5; // Emissive intensity boost
}
