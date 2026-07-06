// AAA-Tier Ultra-Complex Rubidium Pyrophoric Combustion & Liquefaction Shader
void EvaluateRubidiumDecay_float(
    float3 RawSilveryAlkaliBase,  // Soft, bright metallic silver (RGB: 0.92, 0.93, 0.94)
    float3 DarkBrownSuperoxide,   // Blistered, burnt superoxide crust (RGB: 0.22, 0.14, 0.10)
    float3 LiquidHydroxideSlag,   // Corrosive, dissolved fluid pool (RGB: 0.40, 0.38, 0.32)
    float3 GeometricNormalWS,     // Vertex surface normal vectors in world space
    float2 UV,                    // Main texture coordinate maps
    float GlobalDecayFactor,      // Master progress float from CPU (0.0 to 1.0)
    float ExothermicBlisterNoise, // Cellular Voronoi noise representing boiling, burning crusts
    float DissolutionMeltNoise,   // Smooth, fluid organic noise for melting liquid boundaries
    out float3 OutFinalAlbedo,    // Final computed PBR base color
    out float OutMetallicPct,     // Dynamic metallic attenuation scale
    out float OutSurfaceRough,    // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS, // Volumetric geometric meltdown displacement vector
    out float3 OutAlkaliEmission  // Pyrophoric violet-pink combustion flame glow
)
{
    // --- STEP 1: ALKALI REACTION PHASE MASKING ---
    // Stage A: Instantly tarnish silver into flat sub-oxides (0.0 to 0.15)
    float tarnishPhase = smoothstep(0.0, 0.15, GlobalDecayFactor);
    
    // Stage B: Pyrophoric superoxide burning peaks mid-timeline (0.15 to 0.6)
    float burningPhase = smoothstep(0.1, 0.4, GlobalDecayFactor) * (1.0 - smoothstep(0.55, 0.85, GlobalDecayFactor));
    float activeCrustMask = saturate((burningPhase * 1.6) - (ExothermicBlisterNoise * 0.35));

    // Stage C: Terminal deliquescent liquefaction melts the asset away (0.5 to 1.0)
    float liquefactionPhase = smoothstep(0.45, 0.9, GlobalDecayFactor);
    float activeMeltMask = saturate((liquefactionPhase * 1.5) - (DissolutionMeltNoise * 0.3));

    // --- STEP 2: MULTI-PHASE VISUAL BLENDING MATRIX ---
    // Silvery Metal -> Burnt Dark Brown Superoxide -> Translucent Hydroxide Liquid
    float3 mutedSilver = lerp(RawSilveryAlkaliBase, float3(0.35, 0.36, 0.38), tarnishPhase * 0.7);
    float3 crustComposite = lerp(mutedSilver, DarkBrownSuperoxide, activeCrustMask);
    
    // AAA Detail: Inject a vibrant chrome-yellow boundary layer (Rb2O2 peroxide edge)
    float peroxideEdge = smoothstep(0.02, 0.15, activeCrustMask) * (1.0 - smoothstep(0.15, 0.4, activeCrustMask));
    crustComposite = lerp(crustComposite, float3(0.88, 0.72, 0.12), peroxideEdge * 0.6);

    // Final albedo shift into pooling liquid hydroxide slag
    OutFinalAlbedo = lerp(crustComposite, LiquidHydroxideSlag, activeMeltMask);

    // --- STEP 3: PBR REFLECTION SPECTRUM ---
    // Rubidium is metallic. Superoxides are dielectrics, and hydroxides are glassy caustic fluids.
    float nativeMetal = lerp(1.0, 0.0, activeCrustMask);
    OutMetallicPct = lerp(nativeMetal, 0.0, activeMeltMask);

    // Pure metal (Roughness 0.12) turns to dry satin (0.45), jumps to an insanely rough 
    // blistering shell (0.94), then collapses into a ultra-slick liquid surface (0.05)
    float crustRough = lerp(0.45, 0.94, activeCrustMask);
    OutSurfaceRough = lerp(crustRough, 0.05, activeMeltMask);

    // --- STEP 4: VOLUMETRIC MELTDOWN & COLLAPSE ---
    // Burning swells the oxide outward (+4cm), then deliquescence slumps the asset downward (-6cm)
    float blisterSwell = ExothermicBlisterNoise * activeCrustMask * 0.04;
    float liquefactionSlump = activeMeltMask * (DissolutionMeltNoise * 0.02 - 0.06);
    
    // Add a chaotic boiling shimmer to the liquefied nodes
    float boilingWobble = sin(UV.y * 250.0 + GlobalDecayFactor * 35.0) * activeMeltMask * 0.003;
    OutVertexOffsetWS = GeometricNormalWS * (blisterSwell + liquefactionSlump + boilingWobble);

    // --- STEP 5: PYROPHORIC VIOLET ELEMENTAL EMISSION ---
    // Pure rubidium reacts with intense incandescence. We emit a rich elemental violet-pink glow
    // right along the expanding reaction threshold borders.
    float flameGlowMask = smoothstep(0.1, 0.5, ExothermicBlisterNoise) * burningPhase;
    OutAlkaliEmission = flameGlowMask * float3(0.68, 0.22, 0.98) * 4.5; // High HDR glow intensity
}
