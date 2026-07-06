// AAA-Tier Ultra-Complex Europium Lanthanide Oxidation & Crumbling Shader
void EvaluateEuropiumDecay_float(
    float3 RawSilverYellowMetal, // Bright, ductile silver-yellow base (RGB: 0.90, 0.88, 0.78)
    float3 MatteSlateGreyTarnish, // Dull, dark charcoal passivating film (RGB: 0.22, 0.24, 0.26)
    float3 ChalkyWhiteHydroxide, // Fully hydrated porous ivory scale (RGB: 0.94, 0.93, 0.88)
    float3 PowderyYellowOxide,   // Brittle terminal trivalent oxide shell (RGB: 0.88, 0.84, 0.62)
    float3 GeometricNormalWS,    // Vertex surface normal vectors in world space
    float2 UV,                   // Main texture mapping layout coordinates
    float GlobalDecayFactor,     // Master progress float from CPU (0.0 to 1.0)
    float PorousBloomNoise,      // High-frequency cellular noise representing expanding blooms
    float OxideFlakeNoise,       // Sharp fractal noise map for brittle structural shell cracking
    out float3 OutFinalAlbedo,   // Final computed PBR base color
    out float OutMetallicPct,    // Dynamic metallic attenuation scale
    out float OutSurfaceRough,   // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS  // Volumetric geometric displacement vector
)
{
    // --- STEP 1: LANTHANIDE PHASE MASKING ---
    // Stage A: Fast conversion from silver-yellow metal to slate tarnish (0.0 to 0.25)
    float tarnishPhase = smoothstep(0.0, 0.22, GlobalDecayFactor);
    float activeSlateMask = saturate(tarnishPhase * (1.0 - smoothstep(0.25, 0.6, GlobalDecayFactor)));

    // Stage B: White hydroxide bloom tears through the tarnish layer (0.2 to 0.7)
    float hydroxideProgression = smoothstep(0.18, 0.55, GlobalDecayFactor);
    float activeWhiteMask = saturate((hydroxideProgression * 1.5) - (PorousBloomNoise * 0.35));

    // Stage C: Brittle yellow oxide takes over completely at the end (0.55 to 1.0)
    float terminalOxidePhase = smoothstep(0.5, 0.9, GlobalDecayFactor);
    float activeYellowMask = saturate((terminalOxidePhase * 1.8) - (OxideFlakeNoise * 0.4));

    // --- STEP 2: MULTI-STAGE ALBEDO BLENDING MATRIX ---
    // Silver-Yellow -> Dark Slate -> Chalky White -> Powdery Pastel Yellow
    float3 metalToSlate = lerp(RawSilverYellowMetal, MatteSlateGreyTarnish, tarnishPhase);
    float3 whiteComposite = lerp(metalToSlate, ChalkyWhiteHydroxide, activeWhiteMask);
    float3 baselineComposite = lerp(whiteComposite, PowderyYellowOxide, activeYellowMask);
    
    // AAA Detail: Add a faint pinkish reaction gradient along the advancing yellow oxide borders (characteristic of Eu2O3 traces)
    float reactionBorderMask = smoothstep(0.02, 0.15, activeYellowMask) * (1.0 - smoothstep(0.15, 0.38, activeYellowMask));
    OutFinalAlbedo = lerp(baselineComposite, float3(0.92, 0.78, 0.78), reactionBorderMask * 0.4);

    // --- STEP 3: PBR REFLECTION SPECTRUM ---
    // Europium is metallic. Its hydroxide and trivalent oxide forms are entirely dielectric insulators.
    float nativeMetal = lerp(1.0, 0.15, tarnishPhase); // Sub-oxides retain a very faint metallic trace
    OutMetallicPct = lerp(nativeMetal, 0.0, activeWhiteMask);

    // Polished metal (Roughness 0.15) shifts to a flat satin (0.55), jumps to a highly
    // scattering white bloom (0.88), and locks into a dry, chalky powder finish (0.97)
    float baseRoughness = lerp(0.15, 0.55, tarnishPhase);
    float hydroxideRough = lerp(baseRoughness, 0.88, activeWhiteMask);
    OutSurfaceRough = lerp(hydroxideRough, lerp(0.92, 0.99, OxideFlakeNoise), activeYellowMask);

    // --- STEP 4: VOLUMETRIC HYDROXIDE ACCRETION & CORROSION ---
    // White hydroxide conversion features severe volumetric expansion, buckling the mesh surface.
    float whiteSwellHeight = PorousBloomNoise * activeWhiteMask * 0.035; // Hydroxide swells out up to 3.5cm
    
    // Terminal trivalent oxide step displacement representing brittle, crumbling pits where sections break off
    float crumblingPits = OxideFlakeNoise * activeYellowMask * -0.015;
    
    OutVertexOffsetWS = GeometricNormalWS * (whiteSwellHeight + crumblingPits);
}
