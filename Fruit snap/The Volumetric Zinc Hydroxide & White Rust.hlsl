// AAA-Tier Complex Zinc Passivation & White Rust Hydroxide Pipeline
void EvaluateZincAdvancedDecay_float(
    float3 RawZincColor,         // Shiny, slightly blue-tinted metallic silver (0.85, 0.88, 0.90)
    float3 WhiteRustAshColor,    // Brilliant, chalky snowy-white corrosion byproduct (0.96, 0.96, 0.96)
    float3 MattePatinaColor,     // Dull, weathered gray passivation shade (0.55, 0.57, 0.58)
    float3 VertexNormalWS,       // Surface geometry normal vectors in world space
    float2 UV,                   // Object texture mapping channel
    float GlobalDecayFactor,     // Master decay variable passed from CPU (0.0 to 1.0)
    float FractalNoiseSample,    // High-density fractal/grunge noise texture for patchy corrosion
    float CrystallographySample, // Hexagonal/crystalline normal map representing zinc spangle grain
    out float3 OutFinalAlbedo,   // Final computed PBR base color
    out float OutMetallicScale,  // Dynamic metallic channel scale
    out float OutSurfaceRough,   // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS // Volumetric geometric displacement vector
)
{
    // --- STEP 1: MATTE PATINA PASSIVATION STAGE ---
    // Early stage: Zinc creates a thin gray oxide film. This dampens the metallic sheen.
    float passivationMask = saturate(GlobalDecayFactor * 3.0);
    
    // --- STEP 2: WHITE RUST FLAKING & BLOOM MASK ---
    // Hydroxide corrosion spreads in erratic, highly porous clusters.
    // We isolate the upper values of a high-frequency fractal noise map to simulate patchy blooms.
    float whiteRustProgress = saturate((GlobalDecayFactor * 1.5) - 0.35);
    float activeCrustMask = saturate((whiteRustProgress * 1.4) - (FractalNoiseSample * 0.4));

    // --- STEP 3: COLOR MATRIX INTERPOLATION ---
    // Pristine Blue-Silver -> Weathered Dull Gray -> High-Brightness Crusty White
    float3 weatheredBase = lerp(RawZincColor, MattePatinaColor, passivationMask);
    OutFinalAlbedo = lerp(weatheredBase, WhiteRustAshColor, activeCrustMask);

    // --- STEP 4: PBR CRYSTALLINE PROPERTY TRANSITION ---
    // Pure zinc metal has a unique crystalline "spangle" texture map. 
    // As passivation and crust take over, we wipe out the metal reflections.
    float metallicDrop = lerp(1.0, 0.7, passivationMask);
    OutMetallicScale = lerp(metallicDrop, 0.0, activeCrustMask);

    // Shiny raw spangle metal (Roughness 0.12) turns into flat gray satin (0.55) 
    // and terminates into an intensely scattering, chalky, dry white powder (0.98)
    float baseRoughness = lerp(0.12, 0.55, passivationMask);
    OutSurfaceRough = lerp(baseRoughness, lerp(0.9, 0.99, FractalNoiseSample), activeCrustMask);

    // --- STEP 5: VOLUMETRIC GEOMETRIC HEIGHT EXFOLIATION ---
    // Zinc hydroxide expands up to several times its physical atomic thickness, forming micro-mounds.
    // We push the vertices outward along their normals, adding sharp grain edge perturbations.
    float microCrustHeight = FractalNoiseSample * activeCrustMask * 0.045; // Sells out up to 4.5cm crusting
    float3 crystallineMicroWobble = CrystallographySample * (passivationMask * (1.0 - activeCrustMask) * 0.002); // Micro grain weathering
    
    OutVertexOffsetWS = (VertexNormalWS * microCrustHeight) + crystallineMicroWobble;
}
