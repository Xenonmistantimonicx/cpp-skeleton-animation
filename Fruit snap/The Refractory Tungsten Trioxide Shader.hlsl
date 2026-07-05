// AAA-Tier Ultra-Complex Tungsten Calcination & Grain Cleavage Shader
void EvaluateTungstenDecay_float(
    float3 RawGunmetalSteel,     // Brushed, dense metallic gunmetal gray (RGB: 0.38, 0.39, 0.40)
    float3 CanaryTrioxideYellow, // Highly vibrant calcined tungsten trioxide (RGB: 0.88, 0.85, 0.12)
    float3 GeometricNormalWS,    // Vertex surface normal vectors in world space
    float3 CameraViewDirWS,      // Direction vector pointing from pixel to camera position
    float2 UV,                   // Main texture coordinates
    float GlobalDecayFactor,     // Master progress float from CPU (0.0 to 1.0)
    float CrystalliteGrainNoise, // High-contrast Voronoi/cellular noise representing metal grain lattices
    float VolumetricCrustSample, // Fine, velvety noise map for the crumbly trioxide powder topology
    out float3 OutFinalAlbedo,   // Final computed PBR base color
    out float OutMetallicPct,    // Dynamic metallic attenuation scale
    out float OutSurfaceRough,   // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS, // Volumetric geometric displacement vector
    out float OutGrainGlint      // Crystalline micro-reflection specular glint
)
{
    // --- STEP 1: THERMAL REACTION MASKS ---
    // Stage A: Early heat tint iridescence peaks softly between 0.1 and 0.4
    float heatTintPhase = smoothstep(0.05, 0.3, GlobalDecayFactor) * (1.0 - smoothstep(0.35, 0.7, GlobalDecayFactor));
    
    // Stage B: Heavy yellow calcination scaling expands exponentially at later stages (0.35 to 1.0)
    float calcinationProgression = saturate((GlobalDecayFactor * 1.6) - 0.4);
    float activeCrustMask = saturate((calcinationProgression * 1.5) - (CrystalliteGrainNoise * 0.35));

    // --- STEP 2: HIGH-TEMPERATURE INTERFERENCEPATINA ---
    float viewAngleFactor = saturate(dot(GeometricNormalWS, CameraViewDirWS));
    float3 strawGold = float3(0.68, 0.55, 0.32);
    float3 steelBlue = float3(0.15, 0.32, 0.55);
    float3 shiftingPatina = lerp(strawGold, steelBlue, viewAngleFactor);
    
    // --- STEP 3: MULTI-STAGE LAYER COMPOSITE BLENDING ---
    // Brushed Gunmetal -> Iridescent Heat Tint -> Velvety Canary Yellow Trioxide
    float3 pristineToTinted = lerp(RawGunmetalSteel, shiftingPatina, heatTintPhase * 0.5);
    
    // Blend toward the intense yellow-green trioxide oxide shell
    float3 coreComposite = lerp(pristineToTinted, CanaryTrioxideYellow, activeCrustMask);
    
    // AAA Detail: Add a sickly chartreuse-green transition border right at the oxide bleeding edge
    float oxideEdgeMask = smoothstep(0.02, 0.15, activeCrustMask) * (1.0 - smoothstep(0.15, 0.4, activeCrustMask));
    OutFinalAlbedo = lerp(coreComposite, float3(0.55, 0.72, 0.10), oxideEdgeMask * 0.5);

    // --- STEP 4: PBR PROPERTY ATTENUATION ---
    // Tungsten is a high-density metallic conductor. Calcined trioxide is a pure dielectric ceramic.
    float baseMetallic = lerp(1.0, 0.9, heatTintPhase);
    OutMetallicPct = lerp(baseMetallic, 0.0, activeCrustMask);

    // Hard brushed metal (Roughness 0.22) shifts to a dry satin during tempering (0.45),
    // and locks into an intensely light-scattering, chalky, powdery velvety oxide surface at termination (0.98)
    float baseRoughness = lerp(0.22, 0.45, heatTintPhase);
    OutSurfaceRough = lerp(baseRoughness, lerp(0.92, 0.99, VolumetricCrustSample), activeCrustMask);

    // --- STEP 5: VOLUMETRIC OXIDE SWELLING DISPLACEMENT ---
    // Tungsten trioxide scales create an aggressive volume expansion that lifts off the metal.
    // Vertices are pushed outward along normal loops, creating jagged, bulbous irregularities.
    float calcinedSwellHeight = VolumetricCrustSample * activeCrustMask * 0.065; // Powder swells up to 6.5cm
    
    // Simulates severe geometric crystalline shearing along grain boundary vectors
    float grainShearNoise = step(0.7, CrystalliteGrainNoise) * activeCrustMask * 0.015;
    OutVertexOffsetWS = (GeometricNormalWS * calcinedSwellHeight) + (GeometricNormalWS * grainShearNoise);

    // --- STEP 6: CRYSTALLINE SPECULAR GLINT ---
    // Generates bright, razor-sharp micro-glints along the cracked metallic grain facets
    OutGrainGlint = step(0.94, frac(CrystalliteGrainNoise * 25.0)) * (1.0 - activeCrustMask) * (1.0 - baseRoughness);
}
