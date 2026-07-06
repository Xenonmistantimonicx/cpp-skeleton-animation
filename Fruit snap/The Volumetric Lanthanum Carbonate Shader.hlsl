// AAA-Tier Ultra-Complex Lanthanum Carbonate Scaling & Exfoliation Shader
void EvaluateLanthanumDecay_float(
    float3 RawSilverMetalBase,    // Polished, ductile rare-earth silver (RGB: 0.92, 0.92, 0.94)
    float3 IndigoGreyTarnish,     // Dull, velvety dark indigo-charcoal skin (RGB: 0.16, 0.18, 0.22)
    float3 ChalkyCarbonateWhite,  // Voluminous lanthanum carbonate crust (RGB: 0.95, 0.95, 0.92)
    float3 GeometricNormalWS,     // Vertex surface normal vectors in world space
    float2 UV,                    // Main texture layout coordinates
    float GlobalDecayFactor,      // Master progress float from CPU (0.0 to 1.0)
    float CrystallineGrainNoise,  // Cellular Voronoi noise representing structural metal grain borders
    float HydrationSwellNoise,    // High-frequency organic noise for chalky oxide topology
    out float3 OutFinalAlbedo,    // Final computed PBR base color
    out float OutMetallicPct,     // Dynamic metallic attenuation scale
    out float OutSurfaceRough,    // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS  // Volumetric geometric displacement vector
)
{
    // --- STEP 1: REACTION PHASE MASKING ---
    // Stage A: Early sub-oxide passivation dulls the silver luster (0.0 to 0.25)
    float tarnishPhase = smoothstep(0.0, 0.22, GlobalDecayFactor);
    float activeIndigoMask = saturate(tarnishPhase * (1.0 - smoothstep(0.28, 0.65, GlobalDecayFactor)));

    // Stage B: White carbonate scaling bursts forth heavily at mid-to-late stages (0.25 to 1.0)
    float carbonateProgression = smoothstep(0.2, 0.55, GlobalDecayFactor);
    float activeWhiteMask = saturate((carbonateProgression * 1.6) - (CrystallineGrainNoise * 0.35));

    // --- STEP 2: MULTI-STAGE LAYER COMPOSITE BLENDING ---
    // Polished Silver -> Dark Indigo Grey -> Snow-White Carbonate Crust
    float3 metalToIndigo = lerp(RawSilverMetalBase, IndigoGreyTarnish, tarnishPhase);
    float3 baselineComposite = lerp(metalToIndigo, ChalkyCarbonateWhite, activeWhiteMask);
    
    // AAA Detail: Add a raw, slightly discolored tan transition halo right where the oxide lifts
    float exfoliationEdge = smoothstep(0.01, 0.12, activeWhiteMask) * (1.0 - smoothstep(0.12, 0.35, activeWhiteMask));
    OutFinalAlbedo = lerp(baselineComposite, float3(0.62, 0.58, 0.50), exfoliationEdge * 0.4);

    // --- STEP 3: PBR TEXTURAL ATTENUATION MATRIX ---
    // Lanthanum is highly conductive. Carbonate scales are excellent electrical insulators (dielectric).
    float nativeMetal = lerp(1.0, 0.12, tarnishPhase); // Sub-oxides retain a very faint greasy trace
    OutMetallicPct = lerp(nativeMetal, 0.0, activeWhiteMask);

    // High-specular metal (Roughness 0.12) transitions to a dry matte satin (0.62),
    // and locks into a highly scattering, flat, porous chalky white ceramic finish (0.96)
    float baseRoughness = lerp(0.12, 0.62, tarnishPhase);
    OutSurfaceRough = lerp(baseRoughness, lerp(0.91, 0.98, HydrationSwellNoise), activeWhiteMask);

    // --- STEP 4: INTERGRANULAR VOLUMETRIC DISPLACEMENT ---
    // Carbonate growth builds outward drastically due to an aggressive volume expansion ratio.
    float whiteSwellHeight = HydrationSwellNoise * activeWhiteMask * 0.045; // Carbonate swells out up to 4.5cm
    
    // Simulates sharp wafer-like flat step displacement where sheets exfolinate away along grain paths
    float grainStep = step(0.68, CrystallineGrainNoise) * activeWhiteMask * 0.02;
    
    OutVertexOffsetWS = GeometricNormalWS * (whiteSwellHeight + grainStep);
}
