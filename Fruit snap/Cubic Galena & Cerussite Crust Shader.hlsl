// AAA-Tier Ultra-Complex Galena Cubic Cleavage & Cerussite Crust Shader
void EvaluateGalenaDecay_float(
    float3 RawSilverGalena,     // Brilliant, bright metallic cubic silver base (0.88, 0.90, 0.92)
    float3 MatteCharcoalBase,   // Dull, weathered gray-black anglesite phase (0.22, 0.23, 0.25)
    float3 WhiteCerussiteCrust, // Creamy, chalky ivory-white carbonate crust (0.93, 0.92, 0.86)
    float3 SurfaceNormalWS,     // Vertex surface normal vectors in world space
    float3 FaceTangentWS,       // Sharp 90-degree orthogonal tangent vector for cubic chipping
    float2 UV,                  // Main layout texture coordinates
    float GlobalDecayFactor,    // Master progress floating point from CPU (0.0 to 1.0)
    float CubicBlockNoise,      // High-contrast stepped/cell noise representing cubic crystal boundaries
    float CrustHeightSample,    // Fine noise map for the powdery carbonate crust topology
    out float3 OutFinalAlbedo,  // Final computed PBR base color
    out float OutMetallicPct,   // Dynamic metallic attenuation scale
    out float OutSurfaceRough,  // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS // Volumetric geometric displacement vector
)
{
    // --- STEP 1: CUBIC REACTION MASKS ---
    // Stage A: Early passivation dulls the silver down to charcoal rapidly (0.0 - 0.3)
    float passivationMask = saturate(GlobalDecayFactor * 3.3);
    
    // Stage B: Cerussite white rot breaks out along the crisp 90-degree cubic seams
    float crustProgression = saturate((GlobalDecayFactor * 1.5) - 0.35);
    float activeCrustMask = saturate((crustProgression * 1.3) - (CubicBlockNoise * 0.4));

    // --- STEP 2: MULTI-STAGE COLOR BLENDING MATRIX ---
    // Pristine Mirror Silver -> Flat Weathered Charcoal -> Chalky Ivory Carbonate Crust
    float3 dulledMetal = lerp(RawSilverGalena, MatteCharcoalBase, passivationMask);
    OutFinalAlbedo = lerp(dulledMetal, WhiteCerussiteCrust, activeCrustMask);

    // --- STEP 3: PBR MOLECULAR TRANSITION ---
    // Galena is highly metallic. Anglesite and Cerussite are non-metallic lead salts.
    float targetMetallic = lerp(1.0, 0.5, passivationMask);
    OutMetallicPct = lerp(targetMetallic, 0.0, activeCrustMask);

    // Perfect mirror facets (Roughness 0.05) turn into flat gray satin (0.60),
    // and terminate into a heavily scattering, rough, completely dry white powder (0.97).
    float baseRoughness = lerp(0.05, 0.60, passivationMask);
    OutSurfaceRough = lerp(baseRoughness, lerp(0.88, 0.98, CrustHeightSample), activeCrustMask);

    // --- STEP 4: ORTHOGONAL CUBIC EXFOLIATION DISPLACEMENT ---
    // Carbonate weathering swells and shatters the cubic lattice.
    // We displace vertices outward along the normal, and add jagged, blocky steps along the tangent.
    float3 outwardSwellVector = SurfaceNormalWS * (activeCrustMask * CrustHeightSample * 0.05); // Swells outward up to 5cm
    
    // Simulates sharp 90-degree corner chipping along the crystal cleavage planes
    float stepChippingNoise = step(0.65, CubicBlockNoise) * sign(sin(UV.x * 300.0));
    float3 cubicShearVector = FaceTangentWS * (activeCrustMask * stepChippingNoise * 0.025); 
    
    OutVertexOffsetWS = outwardSwellVector + cubicShearVector;
}
