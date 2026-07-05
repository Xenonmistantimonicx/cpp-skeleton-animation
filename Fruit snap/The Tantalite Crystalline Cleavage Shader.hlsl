// AAA-Tier Ultra-Complex Tantalite Crystalline Cleavage & Oxide Leaching Shader
void EvaluateTantaliteDecay_float(
    float3 RawAdamantineBlack,    // Deep, heavy black mineral base color (RGB: 0.08, 0.08, 0.09)
    float3 ChalkyLeachedTan,      // Muted, earthy ivory/tan oxide crust (RGB: 0.72, 0.65, 0.54)
    float3 GeometricNormalWS,     // Vertex surface normal vectors in world space
    float3 CameraViewDirWS,       // Direction vector pointing from pixel to camera position
    float2 UV,                    // Main texture layout coordinates
    float GlobalDecayFactor,      // Master progress float from CPU (0.0 to 1.0)
    float CrystallineGrainNoise,  // Cellular Voronoi noise representing sharp structural grain borders
    float OxideSwellNoise,        // High-frequency powdery noise for earthy clay topology
    out float3 OutFinalAlbedo,    // Final computed PBR base color
    out float OutMetallicPct,     // Dynamic metallic attenuation scale
    out float OutSurfaceRough,    // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS  // Volumetric geometric displacement vector
)
{
    // --- STEP 1: GEOLOGICAL WEATHERING MASKS ---
    // Stage A: Early sub-metallic iridescent tarnish rings (0.02 to 0.3)
    float tarnishPhase = smoothstep(0.02, 0.2, GlobalDecayFactor) * (1.0 - smoothstep(0.32, 0.7, GlobalDecayFactor));
    
    // Stage B: Earthy tan pentoxide leaching blooms at mid-to-late stages (0.35 to 1.0)
    float leachingProgression = saturate((GlobalDecayFactor * 1.6) - 0.45);
    float activeCrustMask = saturate((leachingProgression * 1.5) - (OxideSwellNoise * 0.35));

    // --- STEP 2: MULTI-LAYER COMPOSITE COLOR BLENDING ---
    // Model heavy sub-metallic iridescent tarnish via surface incident angles
    float viewAngleFactor = saturate(dot(GeometricNormalWS, CameraViewDirWS));
    float3 deepViolet = float3(0.14, 0.08, 0.22);
    float3 darkBronze = float3(0.24, 0.18, 0.12);
    float3 shiftingTarnish = lerp(deepViolet, darkBronze, viewAngleFactor);
    
    float3 pristineToTarnished = lerp(RawAdamantineBlack, shiftingTarnish, tarnishPhase * 0.5);
    
    // Final albedo blend: transition from tarnished black crystal core to highly diffuse earthy ivory-tan
    OutFinalAlbedo = lerp(pristineToTarnished, ChalkyLeachedTan, activeCrustMask);

    // --- STEP 3: PBR LUSTER ATOMIZATION ---
    // Pristine tantalite displays a strong sub-metallic to near-metallic adamantine luster (~0.55).
    // Earthy leached clay patinas are completely flat, dielectric mineral structures (0.0).
    float baselineMetallic = lerp(0.55, 0.35, tarnishPhase);
    OutMetallicPct = lerp(baselineMetallic, 0.0, activeCrustMask);

    // Brilliantly reflective sheen (Roughness 0.26) shifts to a muted satin during tarnish (0.48),
    // and terminates into an intensely scattering, chalky, porous clay finish (0.96)
    float baseRoughness = lerp(0.26, 0.48, tarnishPhase);
    OutSurfaceRough = lerp(baseRoughness, lerp(0.88, 0.98, OxideSwellNoise), activeCrustMask);

    // --- STEP 4: METAMICT CRYSTALLINE VOLUMETRIC STEPPING ---
    // Internal metamict swelling expands the structural volume outward, creating geometric cracks.
    float tanCrustSwell = OxideSwellNoise * activeCrustMask * 0.02; // Clay expands up to 2.0cm
    
    // Simulates sharp, flat, stepped splitting along rigid crystal grain cleavages
    float grainShearStep = step(0.65, CrystallineGrainNoise) * (1.0 - activeCrustMask) * GlobalDecayFactor * -0.03;
    
    OutVertexOffsetWS = GeometricNormalWS * (tanCrustSwell + grainShearStep);
}
