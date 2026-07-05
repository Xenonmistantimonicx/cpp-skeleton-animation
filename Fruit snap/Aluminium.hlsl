// AAA-Tier Ultra-Complex Aluminum Anodization & Exfoliation Pipeline
void EvaluateAluminumAdvancedDecay_float(
    float3 RawAluminiumColor,   // Highly reflective, slightly blue-tinted silver base (0.91, 0.92, 0.95)
    float3 WhiteOxideAshColor,  // Chalky, powdery white-gray corrosion byproduct (0.88, 0.88, 0.88)
    float3 GeometricNormalWS,   // Vertex normal in world space
    float3 TangentWS,           // Surface tangent for directional crystal flaking
    float2 UV,                  // Primary texture mapping channel
    float GlobalDecayFactor,    // Master progress floating point from CPU (0.0 to 1.0)
    float VoronoiNoiseSample,   // High-frequency cellular noise map representing pitting nodes
    float StructHeightSample,   // Micro-texture height map for flaky crusting
    out float3 OutFinalAlbedo,  // Dynamic PBR diffuse map output
    out float OutMetallicPct,   // Continuous metallic scale calculation
    out float OutSurfaceRough,  // Multi-tier micro-surface roughness profile
    out float3 OutVertexDisplace // Structural 3D geometric mesh distortion vector
)
{
    // --- STEP 1: INITIAL PASSIVATION & ANODIZATION LEVEL ---
    // Aluminum turns dull before it crusts. We calculate the early oxide shield layer.
    float passivationMask = saturate(GlobalDecayFactor * 2.5);
    
    // --- STEP 2: STOCHASTIC CELLULAR PITTING MASK ---
    // Microscopic galvanic cells attack the crystal boundaries. 
    // We isolate the dense core of the Voronoi cells to simulate localized volcanic pitting.
    float localizedPitMask = smoothstep(0.4, 0.1, VoronoiNoiseSample) * step(0.2, GlobalDecayFactor);
    
    // --- STEP 3: VOLUMETRIC EXFOLIATION & BLISTERING ---
    // White hydroxide ash expands up to 3x the volume of the original metal matrix.
    // It breaks out violently along structural grain boundaries (driven by noise & heights).
    float exfoliationProgress = saturate((GlobalDecayFactor * 1.6) - 0.4);
    float activeCrustMask = saturate((exfoliationProgress * 1.3) - (StructHeightSample * 0.35));
    activeCrustMask = max(activeCrustMask, localizedPitMask * 0.7);

    // --- STEP 4: MULTI-CHANNEL COLOR MATRIX BLENDING ---
    // Blends shiny silver to dull anodized gray, pits to dark oxide cavities, and crusts to raw white powder
    float3 dulledBase = lerp(RawAluminiumColor, RawAluminiumColor * 0.65, passivationMask);
    float3 pittedBase = lerp(dulledBase, float3(0.18, 0.18, 0.2), localizedPitMask);
    OutFinalAlbedo = lerp(pittedBase, WhiteOxideAshColor, activeCrustMask);

    // --- STEP 5: PBR THERMODYNAMIC SHIFTING ---
    // Pure aluminum is highly conductive (Metallic = 1.0). 
    // Rotted aluminum hydroxide is a highly insulating ceramic compound (Metallic = 0.0).
    float continuousMetal = lerp(1.0, 0.5, passivationMask);
    OutMetallicPct = lerp(continuousMetal, 0.0, activeCrustMask);

    // Fresh aluminum has mirror-like reflections (Roughness ~ 0.08). 
    // Passivation turns it into a satin sheet (0.45). The white crust scatters photons randomly (0.98).
    float baseRough = lerp(0.08, 0.45, passivationMask);
    OutSurfaceRough = lerp(baseRough, lerp(0.85, 0.98, StructHeightSample), activeCrustMask);

    // --- STEP 6: GEOMETRIC MICRO-TOPOLOGY DISPLACEMENT ---
    // Pits dig deep into the mesh (-Normal), while exfoliation blisters push outward (+Normal) 
    // layered with anisotropic structural directional scaling along the surface tangent.
    float3 pitDepthVector = -GeometricNormalWS * (localizedPitMask * 0.025); // Pits cave in up to 2.5cm
    float3 blisterHeightVector = GeometricNormalWS * (activeCrustMask * StructHeightSample * 0.06); // Ash swells out 6cm
    float3 grainFlakingVector = TangentWS * (activeCrustMask * sin(UV.x * 100.0) * 0.015); // Layered structural splitting
    
    OutVertexDisplace = pitDepthVector + blisterHeightVector + grainFlakingVector;
}
