// AAA-Tier Ultra-Complex Galvanized Steel Sacrificial White/Red Rust Shader
void EvaluateGalvanizedSteelDecay_float(
    float3 PristineZincSpangle, // Shiny bluish-silver crystalline metal base (0.85, 0.88, 0.90)
    float3 WhiteRustHydroxide,  // Chalky, powdery snowy-white zinc salt (0.95, 0.95, 0.95)
    float3 RedIronRustOxide,    // Voluminous, rough orange-crimson iron rust (0.45, 0.18, 0.08)
    float3 GeometricNormalWS,   // Vertex surface normal vectors in world space
    float2 UV,                  // Main texture coordinates
    float GlobalDecayFactor,    // Master progress float from CPU (0.0 to 1.0)
    float SpangleCrystalNoise,  // Crystalline faceted noise map for zinc flake grain
    float PerlinRustGrunge,     // High-frequency organic noise for localized rust pits
    float BubblingHeightSample, // Flaky/bumpy noise map for red rust macro-topology
    out float3 OutFinalAlbedo,  // Final computed PBR base color
    out float OutMetallicPct,   // Dynamic metallic attenuation scale
    out float OutSurfaceRough,  // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS // Volumetric geometric displacement vector
)
{
    // --- STEP 1: ELECTROCHEMICAL STAGE MASKING ---
    // Stage A: Zinc passivates and creates white rust early on (0.0 to 0.5)
    float zincDecayPhase = smoothstep(0.0, 0.5, GlobalDecayFactor);
    float whiteRustMask = saturate((zincDecayPhase * 1.5) - (PerlinRustGrunge * 0.4));

    // Stage B: Zinc layer breaches completely, exposing iron to active red rust (0.4 to 1.0)
    float ironExposurePhase = smoothstep(0.4, 0.9, GlobalDecayFactor);
    float redRustMask = saturate((ironExposurePhase * 1.6) - (PerlinRustGrunge * 0.35));

    // --- STEP 2: MULTI-STAGE LAYERED ALBEDO BLENDING ---
    // Dull down the shiny spangle crystal texture map into a weathered gray patina
    float3 passivatedZinc = lerp(PristineZincSpangle * (SpangleCrystalNoise * 0.3 + 0.7), float3(0.5, 0.52, 0.54), zincDecayPhase);
    
    // Layer the white zinc crust over the passivated gray zinc base
    float3 zincLayerComposite = lerp(passivatedZinc, WhiteRustHydroxide, whiteRustMask);
    
    // Burn the deep red/orange iron oxide completely through the zinc shield
    float3 finalAlbedoComposite = lerp(zincLayerComposite, RedIronRustOxide, redRustMask);
    
    // AAA Detail: Add a bright yellowish-cyan reaction border right at the bleeding edge where zinc meets iron rust
    float galvanicBorderMask = smoothstep(0.05, 0.2, redRustMask) * (1.0 - smoothstep(0.2, 0.5, redRustMask));
    OutFinalAlbedo = lerp(finalAlbedoComposite, float3(0.75, 0.65, 0.25), galvanicBorderMask * 0.4);

    // --- STEP 3: PBR TRANSFORMATION MATRIX ---
    // Pristine steel/zinc is 100% metallic. Both white zinc salts and red iron rust are dielectric insulators.
    float nativeMetal = lerp(1.0, 0.6, zincDecayPhase);
    float metalStrippedByWhiteRust = lerp(nativeMetal, 0.0, whiteRustMask);
    OutMetallicPct = lerp(metalStrippedByWhiteRust, 0.0, redRustMask);

    // Shiny spangle facet reflections (Roughness 0.1) turn into flat satin patina (0.55),
    // powdery white powder (0.92), and finally bubbling coarse red rust flakes (0.98)
    float zincRough = lerp(0.1, 0.55, zincDecayPhase);
    float whiteRustRough = lerp(zincRough, 0.92, whiteRustMask);
    OutSurfaceRough = lerp(whiteRustRough, lerp(0.85, 0.99, BubblingHeightSample), redRustMask);

    // --- STEP 4: TWO-TIER VOLUMETRIC TOPOLOGY DISPLACEMENT ---
    // White rust expands slightly outward (+3cm). Red iron rust swells massively (+10cm) and bubbles up.
    float whiteSwell = whiteRustMask * 0.025;
    float redSwell = redRustMask * BubblingHeightSample * 0.095;
    
    // Combine swelling offsets along vertex normals, adding jagged scaling to mimic structural flaking
    float combinedHeight = lerp(whiteSwell, redSwell, redRustMask);
    float3 microPittingWobble = GeometricNormalWS * (redRustMask * sin(UV.x * 220.0) * -0.015); // Deep iron core pitting craters
    
    OutVertexOffsetWS = (GeometricNormalWS * combinedHeight) + microPittingWobble;
}
