// AAA-Tier Ultra-Complex Zirconium Phase-Changing Oxide Shader
void EvaluateZirconiumDecay_float(
    float3 RawSilverMetal,       // Bright, specular zirconium silver base (RGB: 0.76, 0.78, 0.80)
    float3 MatteBlackDioxide,    // Dense, sub-stoichiometric black oxide skin (RGB: 0.05, 0.05, 0.06)
    float3 ChalkyWhiteZirconia,   // Fully oxidized porcelaneous monoclinic scale (RGB: 0.95, 0.94, 0.90)
    float3 GeometricNormalWS,    // Vertex surface normal vectors in world space
    float2 UV,                   // Main texture mapping layout coordinates
    float GlobalDecayFactor,     // Master progress float from CPU (0.0 to 1.0)
    float NoduleBlisterNoise,    // High-frequency cellular noise representing blistering nodules
    float CeramicFlakeSample,    // Sharp, brittle fractal noise map for structural shell cracking
    out float3 OutFinalAlbedo,   // Final computed PBR base color
    out float OutMetallicPct,    // Dynamic metallic attenuation scale
    out float OutSurfaceRough,   // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS  // Volumetric geometric displacement vector
)
{
    // --- STEP 1: OXIDATION PHASE MASKING ---
    // Stage A: Quick conversion from silver metal to black dioxide shell (0.0 to 0.4)
    float blackOxidePhase = smoothstep(0.0, 0.35, GlobalDecayFactor);
    float activeBlackMask = saturate(blackOxidePhase * (1.0 - smoothstep(0.4, 0.85, GlobalDecayFactor)));

    // Stage B: White zirconia transformation tears through the black oxide at later stages (0.35 to 1.0)
    float whiteScaleProgression = saturate((GlobalDecayFactor * 1.7) - 0.45);
    float activeWhiteMask = saturate((whiteScaleProgression * 1.5) - (CeramicFlakeSample * 0.35));

    // --- STEP 2: MULTI-STAGE ALBEDO BLENDING MATRIX ---
    // Polished Silver -> Jet Black Oxide Skin -> Chalky Monoclinic White Crust
    float3 silverToBlack = lerp(RawSilverMetal, MatteBlackDioxide, blackOxidePhase);
    float3 baselineComposite = lerp(silverToBlack, ChalkyWhiteZirconia, activeWhiteMask);
    
    // AAA Detail: Add a highly characteristic beige/creamy transition boundary where the black oxide is breaking down
    float reactionBorderMask = smoothstep(0.02, 0.12, activeWhiteMask) * (1.0 - smoothstep(0.12, 0.35, activeWhiteMask));
    OutFinalAlbedo = lerp(baselineComposite, float3(0.78, 0.72, 0.58), reactionBorderMask * 0.6);

    // --- STEP 3: PBR TRANSFORMATION MATRIX ---
    // Zirconium is entirely metallic. Both the black dioxide sub-layer and white monoclinic zirconia are dielectrics.
    float nativeMetal = lerp(1.0, 0.0, blackOxidePhase);
    OutMetallicPct = lerp(nativeMetal, 0.0, activeWhiteMask);

    // Mirror aerospace surface (Roughness 0.1) shifts to a tight, smooth satin black (0.42),
    // and terminates into an intensely scattering, chalky, rough porcelaneous white scale (0.96)
    float baseRoughness = lerp(0.1, 0.42, blackOxidePhase);
    OutSurfaceRough = lerp(baseRoughness, lerp(0.85, 0.99, NoduleBlisterNoise), activeWhiteMask);

    // --- STEP 4: PILLING-BEDWORTH VOLUMETRIC BLISTERING ---
    // White zirconia conversion features a massive volume increase, buckling the surface outward.
    // Vertices are pushed outward along normal loops, creating sharp, fractured macro-nodules.
    float whiteSwellHeight = NoduleBlisterNoise * activeWhiteMask * 0.055; // White crust swells out up to 5.5cm
    
    // Micro-cratering effect representing sections where brittle black oxide flakes have popped off
    float spallingPits = CeramicFlakeSample * activeBlackMask * -0.005;
    
    OutVertexOffsetWS = GeometricNormalWS * (whiteSwellHeight + spallingPits);
}
