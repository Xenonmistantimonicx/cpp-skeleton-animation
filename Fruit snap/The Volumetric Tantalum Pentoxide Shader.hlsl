// AAA-Tier Ultra-Complex Tantalum Pentoxide Blooming & Flaking Shader
void EvaluateTantalumDecay_float(
    float3 RawBlueGreyMetal,      // Dense, brushed blue-grey metallic base (RGB: 0.45, 0.48, 0.52)
    float3 MatteSubOxideSlate,    // Dull, dark charcoal sub-oxide skin (RGB: 0.15, 0.16, 0.18)
    float3 ChalkyPentoxideWhite,  // Voluminous tantalum pentoxide crystal (RGB: 0.96, 0.96, 0.93)
    float3 GeometricNormalWS,     // Vertex surface normal vectors in world space
    float2 UV,                    // Main texture coordinate maps
    float GlobalDecayFactor,      // Master progress float from CPU (0.0 to 1.0)
    float IntergranularGrainNoise,// Cellular Voronoi noise representing structural metal grain borders
    float PentoxideSwellSample,   // High-frequency organic noise for chalky oxide topology
    out float3 OutFinalAlbedo,    // Final computed PBR base color
    out float OutMetallicPct,     // Dynamic metallic attenuation scale
    out float OutSurfaceRough,    // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS  // Volumetric geometric displacement vector
)
{
    // --- STEP 1: REFRACTORY PHASE MASKING ---
    // Stage A: Early sub-oxide passivation dulls the metal skin (0.0 to 0.35)
    float subOxidePhase = smoothstep(0.0, 0.3, GlobalDecayFactor);
    float activeSlateMask = saturate(subOxidePhase * (1.0 - smoothstep(0.35, 0.8, GlobalDecayFactor)));

    // Stage B: White pentoxide blooming bursts forth heavily at mid-to-late stages (0.35 to 1.0)
    float pentoxideProgression = saturate((GlobalDecayFactor * 1.6) - 0.45);
    float activeWhiteMask = saturate((pentoxideProgression * 1.5) - (IntergranularGrainNoise * 0.3));

    // --- STEP 2: MULTI-STAGE LAYER COMPOSITE BLENDING ---
    // Brushed Blue-Grey -> Dark Matte Slate -> Snow-White Pentoxide Crust
    float3 metalToSlate = lerp(RawBlueGreyMetal, MatteSubOxideSlate, subOxidePhase);
    float3 baselineComposite = lerp(metalToSlate, ChalkyPentoxideWhite, activeWhiteMask);
    
    // AAA Detail: Add a raw, slightly discolored tan transition halo right where the oxide lifts
    float delaminationEdge = smoothstep(0.01, 0.1, activeWhiteMask) * (1.0 - smoothstep(0.1, 0.3, activeWhiteMask));
    OutFinalAlbedo = lerp(baselineComposite, float3(0.65, 0.60, 0.52), delaminationEdge * 0.4);

    // --- STEP 3: PBR TEXTURAL ATTENUATION MATRIX ---
    // Tantalum is a highly conductive heavy metal. Pentoxide scales are excellent electrical insulators (dielectric).
    float nativeMetal = lerp(1.0, 0.15, subOxidePhase); // Sub-oxides retain a very faint greasy sub-metallic trace
    OutMetallicPct = lerp(nativeMetal, 0.0, activeWhiteMask);

    // High-specular refractory metal (Roughness 0.18) transitions to a dry matte satin (0.55),
    // and locks into a highly scattering, flat, porous chalky white ceramic finish (0.97)
    float baseRoughness = lerp(0.18, 0.55, subOxidePhase);
    OutSurfaceRough = lerp(baseRoughness, lerp(0.90, 0.99, PentoxideSwellSample), activeWhiteMask);

    // --- STEP 4: INTERGRANULAR VOLUMETRIC DISPLACEMENT ---
    // Pentoxide growth builds outward drastically due to an aggressive expansion ratio.
    // Vertices are stepped outward cleanly along normal loops, broken up by grain vectors.
    float whiteSwellHeight = PentoxideSwellSample * activeWhiteMask * 0.05; // Pentoxide swells out up to 5.0cm
    
    // Simulates sharp wafer-like flat step displacement where sheets delaminate away along grain paths
    float grainStep = step(0.65, IntergranularGrainNoise) * activeWhiteMask * 0.015;
    
    OutVertexOffsetWS = GeometricNormalWS * (whiteSwellHeight + grainStep);
}
