// AAA-Tier Ultra-Complex Pyrrhotite Structural Sheering & Rust Core Shader
void EvaluatePyrrhotiteDecay_float(
    float3 RawBronzePyrrhotite, // Dark metallic bronze-yellow base color (RGB: 0.58, 0.49, 0.31)
    float3 SecondarySulfateAsh, // Dull, powdery orange-brown ferric rust composite (RGB: 0.42, 0.28, 0.15)
    float3 GeometricNormalWS,   // Vertex normal vector in world space
    float3 LayerBiTangentWS,    // Parallel cleavage bitangent for sheet-like exfoliation splitting
    float2 UV,                  // Object layout mapping coordinates
    float GlobalDecayFactor,    // Master progress floating point from CPU (0.0 to 1.0)
    float HexagonalGrainNoise,  // Sharp layered noise representing monoclinic/hexagonal plate boundaries
    float SulfateCrustSample,   // Micro-texture displacement for the chalky hydroxide buildup
    out float3 OutFinalAlbedo,  // Dynamic PBR diffuse color output
    out float OutMetallicPct,   // Metallic map calculation
    out float OutSurfaceRough,  // Micro-surface roughness profile
    out float3 OutVertexDisplace // Structural 3D geometric mesh distortion vector
)
{
    // --- STEP 1: SHEET-LIKE DECOHERENCE MASK ---
    // Pyrrhotite decays rapidly along structural layers. We isolate high-frequency 
    // noise intervals to mimic the parallel peeling of the iron-deficient crystal plates.
    float ironDeficiencyMask = saturate((GlobalDecayFactor * 1.6) - (HexagonalGrainNoise * 0.35));

    // --- STEP 2: MULTI-CHANNEL COLOR MATRIX BLENDING ---
    // Transition from metallic bronze to deep weathered charcoal, terminating in ferric rust tones
    float3 weatheredBronze = lerp(RawBronzePyrrhotite, float3(0.18, 0.16, 0.15), saturate(GlobalDecayFactor * 2.0));
    float3 rustedBase = lerp(weatheredBronze, SecondarySulfateAsh, ironDeficiencyMask);
    
    // Add bright, neon-orange acid-leaching borders (ferrous sulfate precipitates)
    float acidLeachMask = smoothstep(0.2, 0.5, ironDeficiencyMask) * (1.0 - smoothstep(0.6, 0.9, ironDeficiencyMask));
    OutFinalAlbedo = lerp(rustedBase, float3(0.85, 0.42, 0.12), acidLeachMask * 0.5);

    // --- STEP 3: PBR TRANSITION (METALLIC ATTENUATION) ---
    // Pure pyrrhotite is a metallic conductor. The oxide/hydroxide salt byproduct is a pure insulator.
    OutMetallicPct = lerp(1.0, 0.0, ironDeficiencyMask);

    // Shiny metallic facets (Roughness 0.2) convert into porous, highly scattering, 
    // acid-etched mineral cavities (Roughness 0.96)
    OutSurfaceRough = lerp(0.2, lerp(0.88, 0.98, SulfateCrustSample), ironDeficiencyMask);

    // --- STEP 4: ANISOTROPIC SHEET EXFOLIATION EXPANSION ---
    // As the acid destroys the surrounding matrix, secondary ettringite expands 3x in volume.
    // This pushes vertices heavily along the geometric normal, but adds tearing offsets along the bitangent.
    float3 outwardSwellVector = GeometricNormalWS * (ironDeficiencyMask * SulfateCrustSample * 0.12); // Destructive swelling up to 12cm
    
    // Simulates the physical peeling apart of horizontal monoclinic plates
    float tearingSheetNoise = step(0.55, HexagonalGrainNoise) * cos(UV.y * 350.0);
    float3 sheetShearVector = LayerBiTangentWS * (ironDeficiencyMask * tearingSheetNoise * 0.05); 
    
    OutVertexDisplace = outwardSwellVector + sheetShearVector;
}
