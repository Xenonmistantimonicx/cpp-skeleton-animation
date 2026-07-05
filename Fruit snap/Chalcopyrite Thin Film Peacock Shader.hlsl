// AAA-Tier Ultra-Complex Chalcopyrite Thin-Film Peacock Iridescence Shader
void EvaluateChalcopyriteDecay_float(
    float3 RawGoldChalcopyrite, // Brassy golden-yellow with greenish tint (RGB: 0.80, 0.70, 0.30)
    float3 EarthyLimoniteCrust, // Dull, porous yellow-brown ochre crust (RGB: 0.48, 0.35, 0.18)
    float3 SurfaceNormalWS,     // Vertex surface normal vectors in world space
    float3 CameraViewDirWS,     // Direction vector pointing from pixel to camera position
    float2 UV,                  // Main texture coordinates
    float GlobalDecayFactor,    // Master progress float from CPU (0.0 to 1.0)
    float MicroCrystalGrunge,   // Fractal noise texture representing tetragonal crystal grain boundaries
    float CrustHeightSample,    // Fine noise map for the powdery limonite buildup topology
    out float3 OutFinalAlbedo,  // Final computed PBR base color
    out float OutMetallicPct,   // Dynamic metallic attenuation scale
    out float OutSurfaceRough,  // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS // Volumetric geometric displacement vector
)
{
    // --- STEP 1: MULTI-STAGE REACTION MASKS ---
    // Stage A: Iridescent Peacock thin-film coating peaks sharply at mid-decay (0.4 - 0.6)
    float peacockPhase = smoothstep(0.1, 0.45, GlobalDecayFactor) * (1.0 - smoothstep(0.55, 0.85, GlobalDecayFactor));
    
    // Stage B: Earthy Limonite crusting grows exponentially at later stages (0.5 - 1.0)
    float crustProgression = saturate((GlobalDecayFactor * 1.8) - 0.55);
    float activeCrustMask = saturate((crustProgression * 1.4) - (MicroCrystalGrunge * 0.3));

    // --- STEP 2: AAA PEACOCK IRIDESCENCE (THIN-FILM INTERFERENCE) ---
    // Simulates photon wave phase-shifting off the copper sulfide oxide film.
    // The shifting color gradient depends directly on the player's viewing angle (Fresnel).
    float fresnelViewFactor = saturate(dot(SurfaceNormalWS, CameraViewDirWS));
    
    // Procedural color array mimicking real copper sulfide optical wave refraction
    float3 peacockBlue = float3(0.02, 0.25, 0.95);
    float3 peacockPurple = float3(0.52, 0.05, 0.88);
    float3 peacockMagenta = float3(0.92, 0.02, 0.45);
    
    // Shift colors smoothly based on viewing angle and overall film thickness growth
    float3 intermediatePeacock = lerp(peacockBlue, peacockPurple, fresnelViewFactor);
    float3 finalPeacockColor = lerp(intermediatePeacock, peacockMagenta, saturate(peacockPhase * 1.5));

    // --- STEP 3: MULTI-LAYER COLOR MIXING MATRIX ---
    // Fresh Crystalline Gold -> Vibrant Iridescent Peacock Ore -> Matte Yellow-Brown Limonite Dust
    float3 pristineToPeacock = lerp(RawGoldChalcopyrite, finalPeacockColor, peacockPhase);
    OutFinalAlbedo = lerp(pristineToPeacock, EarthyLimoniteCrust, activeCrustMask);

    // --- STEP 4: PBR PROPERTY TRANSMUTATION ---
    // Chalcopyrite and Bornite are highly metallic conductors. Limonite is a pure dielectric mineral.
    float baseMetallic = lerp(1.0, 0.85, peacockPhase);
    OutMetallicPct = lerp(baseMetallic, 0.0, activeCrustMask);

    // Shiny pristine crystals (Roughness 0.15) shift to a slick satin luster during iridescence (0.35),
    // and lock into a highly scattering, chalky, dry ochre powder surface at termination (0.96).
    float baseRoughness = lerp(0.15, 0.35, peacockPhase);
    OutSurfaceRough = lerp(baseRoughness, lerp(0.85, 0.97, CrustHeightSample), activeCrustMask);

    // --- STEP 5: VOLUMETRIC HYDROXIDE DISPLACEMENT ---
    // While the copper leaches away, the iron converts into a porous, sprawling hydroxide crust.
    // Vertices are pushed outward along their normals, adding jagged grain edge variations.
    float microCrustHeight = CrustHeightSample * activeCrustMask * 0.055; // Sells out up to 5.5cm crusting
    float3 crystalGrainWobble = SurfaceNormalWS * (peacockPhase * sin(UV.x * 180.0) * 0.003); // Early micro-wrinkling
    
    OutVertexOffsetWS = (SurfaceNormalWS * microCrustHeight) + crystalGrainWobble;
}
