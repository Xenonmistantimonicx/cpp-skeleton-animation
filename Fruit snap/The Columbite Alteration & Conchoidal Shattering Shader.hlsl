// AAA-Tier Ultra-Complex Columbite Alteration & Conchoidal Shattering Shader
void EvaluateColumbiteDecay_float(
    float3 RawSubMetallicBlack,   // Deep, heavy iron-black mineral base (RGB: 0.11, 0.11, 0.12)
    float3 EarthyLeachedOchre,    // Dull, muddy reddish-brown/yellow clay patina (RGB: 0.48, 0.32, 0.18)
    float3 GeometricNormalWS,     // Vertex surface normal vectors in world space
    float3 CameraViewDirWS,       // Direction vector pointing from pixel to camera position
    float2 UV,                    // Main texture layout coordinates
    float GlobalDecayFactor,      // Master progress float from CPU (0.0 to 1.0)
    float ConchoidalFractureNoise,// High-contrast stepped Voronoi noise representing glass-like shell fractures
    float HydrothermalGrungeNoise,// Fine organic fractal noise for earthy clay accumulation
    out float3 OutFinalAlbedo,    // Final computed PBR base color
    out float OutMetallicPct,     // Dynamic metallic attenuation scale
    out float OutSurfaceRough,    // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS  // Volumetric geometric displacement vector
)
{
    // --- STEP 1: GEOLOGICAL WEATHERING MASKS ---
    // Stage A: Muted iridescent tarnish film peaks early on (0.05 to 0.4)
    float tarnishPhase = smoothstep(0.05, 0.25, GlobalDecayFactor) * (1.0 - smoothstep(0.4, 0.75, GlobalDecayFactor));
    
    // Stage B: Earthy clay ochre leaching spreads across surfaces at mid-to-late stages (0.35 to 1.0)
    float leachingProgression = saturate((GlobalDecayFactor * 1.6) - 0.4);
    float activePatinaMask = saturate((leachingProgression * 1.4) - (HydrothermalGrungeNoise * 0.3));

    // --- STEP 2: MULTI-LAYER COMPOSITE BLENDING ---
    // Model the subtle sub-metallic iridescent tarnish via surface incident angles
    float viewAngleFactor = saturate(dot(GeometricNormalWS, CameraViewDirWS));
    float3 bronzeTint = float3(0.28, 0.22, 0.15);
    float3 purpleSteel = float3(0.18, 0.15, 0.25);
    float3 shiftingTarnish = lerp(bronzeTint, purpleSteel, viewAngleFactor);
    
    float3 pristineToTarnished = lerp(RawSubMetallicBlack, shiftingTarnish, tarnishPhase * 0.4);
    
    // Final albedo blend: transition from tarnished dark core to highly diffuse earthy ochre clay
    OutFinalAlbedo = lerp(pristineToTarnished, EarthyLeachedOchre, activePatinaMask);

    // --- STEP 3: PBR LUSTER SHIFT MATRIX ---
    // Pristine columbite sits on the boundary of metallic and dielectric (sub-metallic, ~0.4).
    // Earthy leached clay patinas are completely non-reflective dielectrics (0.0).
    float baselineMetallic = lerp(0.42, 0.25, tarnishPhase);
    OutMetallicPct = lerp(baselineMetallic, 0.0, activePatinaMask);

    // Greasy mineral sheen (Roughness 0.38) turns into a dry, scattering satin during tarnish (0.55),
    // and shifts into an intensely matte, powdery, coarse clay surface at late decay (0.95)
    float baseRoughness = lerp(0.38, 0.55, tarnishPhase);
    OutSurfaceRough = lerp(baseRoughness, lerp(0.88, 0.98, HydrothermalGrungeNoise), activePatinaMask);

    // --- STEP 4: METAMICT SUB-CONCHOIDAL DISPLACEMENT ---
    // Mineral crystalline matrix weathering splits the asset inward along curved fracture planes.
    // Vertices are stepped inward or outward based on stepped shell layers.
    float patinaClayBuildUp = HydrothermalGrungeNoise * activePatinaMask * 0.015; // Clay accumulates up to 1.5cm
    
    // Simulates razor-sharp, concentric curved steps characteristic of conchoidal mineral fracturing
    float crystalFractureStep = step(0.7, ConchoidalFractureNoise) * (1.0 - activePatinaMask) * GlobalDecayFactor * -0.025;
    
    OutVertexOffsetWS = GeometricNormalWS * (patinaClayBuildUp + crystalFractureStep);
}
