// AAA-Tier Ultra-Complex Neodymium Spallation & Crumbling Shader
void EvaluateNeodymiumDecay_float(
    float3 RawSilverMetalBase,    // Bright, ductile rare-earth silver (RGB: 0.88, 0.88, 0.90)
    float3 SlateGreyTarnish,      // Dull, dark charcoal-blue sub-oxide film (RGB: 0.20, 0.22, 0.25)
    float3 ChalkyPurpleGreyOxide, // Voluminous neodymium(III) oxide crust (RGB: 0.64, 0.61, 0.66)
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
    // Stage A: Early sub-oxide film dulls the pristine silver luster (0.0 to 0.22)
    float tarnishPhase = smoothstep(0.0, 0.20, GlobalDecayFactor);
    float activeSlateMask = saturate(tarnishPhase * (1.0 - smoothstep(0.25, 0.60, GlobalDecayFactor)));

    // Stage B: Grey-purple oxide crust erupts and expands through mid-to-late stages (0.22 to 1.0)
    float oxideProgression = smoothstep(0.18, 0.55, GlobalDecayFactor);
    float activeOxideMask = saturate((oxideProgression * 1.5) - (MicroCrystallineNoise * 0.35));

    // --- STEP 2: MULTI-LAYER COMPOSITE ALBEDO BLENDING ---
    // Polished Silver -> Dark Slate Grey -> Chalky Purple-Grey Crust
    float3 metalToSlate = lerp(RawSilverMetalBase, SlateGreyTarnish, tarnishPhase);
    float3 baselineComposite = lerp(metalToSlate, ChalkyPurpleGreyOxide, activeOxideMask);
    
    // AAA Detail: Add a raw, slightly discolored tan transition halo right where the oxide lifts
    float spallationEdge = smoothstep(0.01, 0.15, activeOxideMask) * (1.0 - smoothstep(0.15, 0.38, activeOxideMask));
    OutFinalAlbedo = lerp(baselineComposite, float3(0.55, 0.52, 0.48), spallationEdge * 0.4);

    // --- STEP 3: PBR REFLECTION SPECTRUM ATTEMUATION ---
    // Neodymium is highly metallic. Trivalent oxides are completely non-reflective dielectrics.
    float nativeMetal = lerp(1.0, 0.15, tarnishPhase); // Sub-oxides retain a very faint greasy trace
    OutMetallicPct = lerp(nativeMetal, 0.0, activeOxideMask);

    // High-specular metal (Roughness 0.14) transitions to a dry matte satin (0.58),
    // and locks into a highly scattering, rough, porous ceramic powder finish (0.95)
    float baseRoughness = lerp(0.14, 0.58, tarnishPhase);
    OutSurfaceRough = lerp(baseRoughness, lerp(0.90, 0.98, SpallationFlakeNoise), activeOxideMask);

    // --- STEP 4: VOLUMETRIC SPALLATION DISPLACEMENT ---
    // Oxide growth builds outward drastically due to an aggressive volume expansion ratio (+4.0cm)
    float oxideSwellHeight = SpallationFlakeNoise * activeOxideMask * 0.04; 
    
    // Simulates sharp crumbling recess pits where brittle oxide scales have detached and fallen away
    float crumblingPits = step(0.72, MicroCrystallineNoise) * activeOxideMask * GlobalDecayFactor * -0.018;
    
    OutVertexOffsetWS = GeometricNormalWS * (oxideSwellHeight + crumblingPits);
}
