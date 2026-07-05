// AAA-Tier Ultra-Complex Uraninite Radiolytic & Radioactive Bloom Shader
void EvaluateUraniniteDecay_float(
    float3 RawUraniniteBlack,    // Greasy, sub-metallic pitch-black base (RGB: 0.08, 0.08, 0.09)
    float3 NeonUranylYellow,     // High-saturation toxic yellow bloom (RGB: 0.92, 0.88, 0.05)
    float3 RadioactiveGreen,     // Torbernite-style emerald/lime green crust (RGB: 0.12, 0.85, 0.22)
    float3 SurfaceNormalWS,      // Vertex surface normal vectors in world space
    float2 UV,                   // Main layout texture coordinates
    float GlobalDecayFactor,     // Master progress float from CPU (0.0 to 1.0)
    float BotryoidalNoiseSample, // Smooth, cellular noise map representing grape-like clusters
    float RadiolyticCrackNoise,  // Sharp, web-like micro-fracture noise map
    out float3 OutFinalAlbedo,   // Final computed PBR base color
    out float OutMetallicPct,    // Dynamic metallic attenuation scale
    out float OutSurfaceRough,   // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS, // Volumetric geometric displacement vector
    out float OutEmissionValue   // Dynamic emissive map for glowing radioactive salts
)
{
    // --- STEP 1: RADIOLYTIC FAILURE & BLOOM MASKS ---
    // Stage A: Internal alpha decay shatters the lattice, turning it into matte charcoal (0.0 - 0.4)
    float radiolyticDullMask = saturate(GlobalDecayFactor * 2.5);
    
    // Stage B: Uranyl bloom breaks out violently along the sharp radiolytic structural fractures
    float bloomProgression = saturate((GlobalDecayFactor * 1.6) - 0.35);
    float activeBloomMask = saturate((bloomProgression * 1.4) - (RadiolyticCrackNoise * 0.4));
    
    // Stage C: Green torbernite copper-uranium salts grow in the deepest moisture-retaining seams
    float greenCrustMask = saturate(activeBloomMask * (1.0 - BotryoidalNoiseSample) * 1.5);

    // --- STEP 2: MULTI-STAGE LAYERED COLOR MATRIX BLENDING ---
    // Greasy Black -> Matte Charcoal -> Toxic Neon Yellow -> Radioactive Green Highlights
    float3 dulledCore = lerp(RawUraniniteBlack, float3(0.15, 0.14, 0.15), radiolyticDullMask);
    float3 yellowBloom = lerp(dulledCore, NeonUranylYellow, activeBloomMask);
    OutFinalAlbedo = lerp(yellowBloom, RadioactiveGreen, greenCrustMask);

    // --- STEP 3: PBR TEXTURAL ATTENUATION ---
    // Uraninite is sub-metallic/pitchy. Uranyl mineral secondary crusts are completely dielectric salts.
    OutMetallicPct = lerp(0.4, 0.0, activeBloomMask);

    // Greasy, low-roughness pitch (0.22) turns into flat, amorphous dry carbon (0.75),
    // and terminates into an intensely rough, porous, chalky mineral crust (0.98).
    float baseRoughness = lerp(0.22, 0.75, radiolyticDullMask);
    OutSurfaceRough = lerp(baseRoughness, lerp(0.9, 0.99, RadiolyticCrackNoise), activeBloomMask);

    // --- STEP 4: VOLUMETRIC RADIOLYTIC SWELLING DISPLACEMENT ---
    // Uranium oxidation swells the crystal structure up to double its original thickness.
    // Vertices push outward along the normal, adding a jagged, micro-wrinkling texture to mimic crusty gummite.
    float outwardSwellHeight = RadiolyticCrackNoise * activeBloomMask * 0.075; // Swells outward up to 7.5cm
    float3 grapeClusterWobble = SurfaceNormalWS * (BotryoidalNoiseSample * (1.0 - activeBloomMask) * 0.01); // Botryoidal relief
    
    OutVertexOffsetWS = (SurfaceNormalWS * outwardSwellHeight) + grapeClusterWobble;

    // --- STEP 5: AAA GLOW POLISH (URANIUM LUMINESCENCE) ---
    // Secondary uranyl minerals fluoresce intensely under UV or look naturally vibrant.
    // We pass an emission driver to make the yellow/green crust edges look menacingly toxic.
    OutEmissionValue = activeBloomMask * 0.35 + (greenCrustMask * sin(UV.y * 50.0 + GlobalDecayFactor * 5.0) * 0.15);
}
