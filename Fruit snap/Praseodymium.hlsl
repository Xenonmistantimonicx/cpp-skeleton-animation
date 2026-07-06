// AAA-Tier Ultra-Complex Praseodymium Green Spallation & Crumbling Shader
void EvaluatePraseodymiumDecay_float(
    float3 RawSilverMetalBase,    // Polished, ductile rare-earth silver (RGB: 0.90, 0.90, 0.92)
    float3 SlateGreyTarnish,      // Dull, dark charcoal-grey sub-oxide film (RGB: 0.22, 0.24, 0.26)
    float3 PaleGreenOxideCrust,   // Voluminous praseodymium(III) oxide (RGB: 0.58, 0.68, 0.52)
    float3 GeometricNormalWS,     // Vertex surface normal vectors in world space
    float2 UV,                    // Main texture layout coordinates
    float GlobalDecayFactor,      // Master progress float from CPU (0.0 to 1.0)
    float MicroCrystallineNoise,  // Cellular Voronoi noise representing brittle crystal boundaries
    float SpallationFlakeNoise,   // High-contrast stepped fractal noise for spalling scales
    out float3 OutFinalAlbedo,    // Final computed PBR base color
    out float OutMetallicPct,     // Dynamic metallic attenuation scale
    out float OutSurfaceRough,    // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS  // Volumetric geometric displacement vector
)
{
    // --- STEP 1: OXIDATION PHASE MASKING ---
    // Stage A: Early sub-oxide film dulls the pristine silver luster (0.0 to 0.20)
    float tarnishPhase = smoothstep(0.0, 0.18, GlobalDecayFactor);
    float activeSlateMask = saturate(tarnishPhase * (1.0 - smoothstep(0.22, 0.58, GlobalDecayFactor)));

    // Stage B: Pale green oxide crust erupts and expands through mid-to-late stages (0.20 to 1.0)
    float oxideProgression = smoothstep(0.16, 0.52, GlobalDecayFactor);
    float activeOxideMask = saturate((oxideProgression * 1.5) - (MicroCrystallineNoise * 0.35));

    // --- STEP 2: MULTI-LAYER COMPOSITE ALBEDO BLENDING ---
    // Polished Silver -> Dark Slate Grey -> Chalky Pale-Green Crust
    float3 metalToSlate = lerp(RawSilverMetalBase, SlateGreyTarnish, tarnishPhase);
    float3 baselineComposite = lerp(metalToSlate, PaleGreenOxideCrust, activeOxideMask);
    
    // AAA Detail: Add a raw, slightly discolored tan transition halo right where the oxide lifts
    float spallationEdge = smoothstep(0.01, 0.14, activeOxideMask) * (1.0 - smoothstep(0.14, 0.35, activeOxideMask));
    OutFinalAlbedo = lerp(baselineComposite, float3(0.52, 0.54, 0.46), spallationEdge * 0.4);

    // --- STEP 3: PBR REFLECTION SPECTRUM ATTEMUATION ---
    // Praseodymium is metallic. Trivalent oxides are completely non-reflective dielectrics.
    float nativeMetal = lerp(1.0, 0.12, tarnishPhase); // Sub-oxides retain a very faint greasy trace
    OutMetallicPct = lerp(nativeMetal, 0.0, activeOxideMask);

    // High-specular metal (Roughness 0.15) transitions to a dry matte satin (0.60),
    // and locks into a highly scattering, rough, porous ceramic powder finish (0.96)
    float baseRoughness = lerp(0.15, 0.60, tarnishPhase);
    OutSurfaceRough = lerp(baseRoughness, lerp(0.92, 0.99, SpallationFlakeNoise), activeOxideMask);

    // --- STEP 4: VOLUMETRIC SPALLATION DISPLACEMENT ---
    // Oxide growth builds outward drastically due to an aggressive volume expansion ratio (+4.5cm)
    float oxideSwellHeight = SpallationFlakeNoise * activeOxideMask * 0.045; 
    
    // Simulates sharp crumbling recess pits where brittle green scales have detached and fallen away
    float crumblingPits = step(0.70, MicroCrystallineNoise) * activeOxideMask * GlobalDecayFactor * -0.02;
    
    OutVertexOffsetWS = GeometricNormalWS * (oxideSwellHeight + crumblingPits);
}
