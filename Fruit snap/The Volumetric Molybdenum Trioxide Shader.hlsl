// AAA-Tier Ultra-Complex Molybdenum Needle Calcination & Sublimation Shader
void EvaluateMolybdenumDecay_float(
    float3 RawPlatinumGrey,       // Bright, dense metallic molybdenum base (RGB: 0.62, 0.64, 0.65)
    float3 PaleNeedleYellow,       // Crystalline orthorhombic MoO3 color (RGB: 0.90, 0.88, 0.68)
    float3 LiquidSlagGreen,       // Molten, glassy trioxide fluid tone (RGB: 0.35, 0.45, 0.20)
    float3 GeometricNormalWS,     // Vertex surface normal vectors in world space
    float3 CameraViewDirWS,       // Direction vector pointing from pixel to camera position
    float2 UV,                    // Main coordinate layout texture
    float GlobalDecayFactor,      // Master progress float from CPU (0.0 to 1.0)
    float NeedleCrystalNoise,     // High-frequency directional strand noise for needle growth
    float SlagLiquefactionNoise,  // Smooth, organic noise map for the melting liquid slag topology
    out float3 OutFinalAlbedo,    // Final computed PBR base color
    out float OutMetallicPct,     // Dynamic metallic attenuation scale
    out float OutSurfaceRough,    // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS,  // Volumetric geometric displacement vector
    out float3 OutSlagEmission    // Molten fluid thermal glow map
)
{
    // --- STEP 1: THERMODYNAMIC STAGE MASKING ---
    // Stage A: Thin-film rainbow tarnish dominates early on (0.05 to 0.35)
    float heatTintPhase = smoothstep(0.05, 0.25, GlobalDecayFactor) * (1.0 - smoothstep(0.35, 0.6, GlobalDecayFactor));
    
    // Stage B: Yellow crystalline needle accumulation peaks mid-timeline (0.3 to 0.75)
    float needlePhase = smoothstep(0.25, 0.5, GlobalDecayFactor) * (1.0 - smoothstep(0.65, 0.9, GlobalDecayFactor));
    float activeNeedleMask = saturate((needlePhase * 1.6) - (NeedleCrystalNoise * 0.4));

    // Stage C: Terminal liquidation and sublimation where the crust evaporates/melts (0.65 to 1.0)
    float terminalPhase = smoothstep(0.6, 0.95, GlobalDecayFactor);
    float activeSlagMask = saturate((terminalPhase * 1.5) - (SlagLiquefactionNoise * 0.35));

    // --- STEP 2: MULTI-STAGE LAYER COMPOSITE BLENDING ---
    // Recreate early thin-film anodization spectrum via view angles
    float viewAngleFactor = saturate(dot(GeometricNormalWS, CameraViewDirWS));
    float3 tGoldenMagenta = float3(0.72, 0.35, 0.52);
    float3 tPeacockBlue   = float3(0.12, 0.48, 0.72);
    float3 shiftingPatina = lerp(tGoldenMagenta, tPeacockBlue, viewAngleFactor);
    
    float3 pristineToTinted = lerp(RawPlatinumGrey, shiftingPatina, heatTintPhase * 0.6);
    
    // Layer the woolly, needle-like trioxide crust over the tempered core
    float3 crustComposite = lerp(pristineToTinted, PaleNeedleYellow, activeNeedleMask);
    
    // Melt the needle crust into a glassy, greenish-yellow liquefying slag at the end of the line
    OutFinalAlbedo = lerp(crustComposite, LiquidSlagGreen, activeSlagMask);

    // --- STEP 3: PBR TEXTURAL PROPERTIES MATRICES ---
    // Molybdenum is highly metallic. Needle crystals are dielectric salts, and liquid slag is glassy.
    float baselineMetallic = lerp(1.0, 0.85, heatTintPhase);
    float metalStrippedByNeedles = lerp(baselineMetallic, 0.0, activeNeedleMask);
    OutMetallicPct = lerp(metalStrippedByNeedles, 0.0, activeSlagMask);

    // Polished metal (Roughness 0.15) turns into a heavily scattering, anisotropic woolly needle mesh (0.94),
    // which drops down into a glossy, highly reflective wet fluid slag layer (0.22)
    float metalRough = lerp(0.15, 0.4, heatTintPhase);
    float needleRough = lerp(metalRough, 0.94, activeNeedleMask);
    OutSurfaceRough = lerp(needleRough, 0.22, activeSlagMask);

    // --- STEP 4: VOLUMETRIC EXTENSION AND SUBLIMATION REVERSION ---
    // Trioxide needles sprout outward heavily (+8cm). 
    // When terminal sublimation hits, the volume drops back down as the metal evaporates into air.
    float needleSwell = activeNeedleMask * NeedleCrystalNoise * 0.08;
    float slagRecession = activeSlagMask * (SlagLiquefactionNoise * 0.03 - 0.04); // Core mass erodes away
    
    float3 combinedOffset = GeometricNormalWS * (needleSwell + slagRecession);
    // Add an erratic shivering along the edges to simulate boiling fluid slag
    float boilingWobble = sin(UV.x * 300.0 + GlobalDecayFactor * 20.0) * activeSlagMask * 0.002;
    
    OutVertexOffsetWS = combinedOffset + (GeometricNormalWS * boilingWobble);

    // --- STEP 5: MOLTEN LIQUID THERMAL GLOW ---
    // Because the liquid oxide phase occurs at high operating temperatures (~800°C),
    // the glassy pooling slag generates an intense incandescent white-orange heat emission.
    float slagMeltGlow = smoothstep(0.3, 0.7, SlagLiquefactionNoise) * activeSlagMask;
    OutSlagEmission = slagMeltGlow * float3(0.98, 0.35, 0.04) * 3.0; // Seared core glow intensity
}
