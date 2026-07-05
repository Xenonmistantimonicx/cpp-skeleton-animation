// AAA-Tier Ultra-Complex Pyrite Decay & Crystalline Cleavage Shader
void EvaluatePyriteDisease_float(
    float3 RawPyriteGold,       // Brilliant brassy-gold metallic crystalline color (0.85, 0.73, 0.41)
    float3 AcidSulfateAsh,      // Dull, chalky yellow-gray acidic rot color (0.68, 0.65, 0.52)
    float3 GeometricNormalWS,   // Vertex normal vector in world space
    float3 FaceTangentWS,       // Sharp geometric tangent for cubic crystal splitting
    float2 UV,                  // Object coordinate texture channel
    float GlobalDecayFactor,    // Master progress floating point from CPU (0.0 to 1.0)
    float CrystalFractureNoise, // Sharp, jagged blocky/cellular noise map representing crystal boundaries
    float HeightCrustSample,    // Fine noise map for the powdery sulfate buildup
    out float3 OutFinalAlbedo,  // Dynamic PBR diffuse color output
    out float OutMetallicPct,   // Metallic map calculation
    out float OutSurfaceRough,  // Micro-surface roughness profile
    out float3 OutVertexDisplace // Structural 3D geometric mesh distortion vector
)
{
    // --- STEP 1: CRYSTALLINE DECOHERENCE MASK ---
    // Pyrite rot attacks crystal seams first. We use a high-contrast blocky noise map 
    // to simulate localized chemical failure spreading across cubic faces.
    float crystalRotMask = saturate((GlobalDecayFactor * 1.5) - (CrystalFractureNoise * 0.4));

    // --- STEP 2: MULTI-CHANNEL COLOR MATRIX BLENDING ---
    // Smoothly dissolves brilliant reflective gold into dead, acidic, chalky gray-yellow powder
    float3 rottedBase = lerp(RawPyriteGold, AcidSulfateAsh, crystalRotMask);
    
    // Add subtle neon-yellow sulfuric acid tint highlights at the bleeding edges of decay
    float acidEdgeMask = smoothstep(0.1, 0.4, crystalRotMask) * (1.0 - smoothstep(0.5, 0.8, crystalRotMask));
    OutFinalAlbedo = lerp(rottedBase, float3(0.82, 0.85, 0.15), acidEdgeMask * 0.4);

    // --- STEP 3: PBR TRANSITION (METAL TO CERAMIC CONVERSION) ---
    // Raw pyrite is highly metallic. Rotted iron sulfate salts are non-metallic dielectrics.
    OutMetallicPct = lerp(1.0, 0.0, crystalRotMask);

    // Flawless crystalline gold facets (Roughness ~ 0.15) turn into completely flat, 
    // light-scattering acidic sulfur dust crusts (Roughness ~ 0.95)
    OutSurfaceRough = lerp(0.15, lerp(0.85, 0.98, HeightCrustSample), crystalRotMask);

    // --- STEP 4: GEOMETRIC CRYSTAL CLEAVAGE EXPANSION ---
    // As pyrite rots, it swells violently. This creates outward thrust along 
    // the crystal faces and jagged, blocky splitting along tangent vectors.
    float3 outwardSwellVector = GeometricNormalWS * (crystalRotMask * HeightCrustSample * 0.08); // Swells outward up to 8cm
    
    // Simulates sharp, brittle snapping of cubic crystal structures along the tangent axis
    float sharpSplitNoise = step(0.6, CrystalFractureNoise) * sin(UV.x * 250.0);
    float3 crystalShearVector = FaceTangentWS * (crystalRotMask * sharpSplitNoise * 0.035); 
    
    OutVertexDisplace = outwardSwellVector + crystalShearVector;
}
