// AAA-Tier Ultra-Complex Thorium Oxide Phase & Encrustation Shader
void EvaluateThoriumDecay_float(
    float3 RawSilverThorium,     // Bright, specular actinide silver base (RGB: 0.82, 0.84, 0.85)
    float3 MatteDioxideBlack,    // Dull, sub-metallic pitchy charcoal black (RGB: 0.10, 0.10, 0.11)
    float3 ChalkyWhiteCarbonate, // Dead, powdery gray-white carbonate crust (RGB: 0.88, 0.88, 0.84)
    float3 GeometricNormalWS,    // Vertex surface normal vectors in world space
    float2 UV,                   // Main layout texture coordinates
    float GlobalDecayFactor,     // Master progress float from CPU (0.0 to 1.0)
    float OxidePittingNoise,     // High-frequency fractal noise for localized oxide decay pits
    float CarbonateScaleSample,  // Brittle flaking noise map for structural carbonate lifting
    out float3 OutFinalAlbedo,   // Final computed PBR base color
    out float OutMetallicPct,    // Dynamic metallic attenuation scale
    out float OutSurfaceRough,   // Multi-tier surface roughness mapping
    out float3 OutVertexOffsetWS, // Volumetric geometric displacement vector
    out float OutAlphaGlowFactor // Dynamic emission profile tracking isotopic energy release
)
{
    // --- STEP 1: OXIDATION STATE MASKS ---
    // Stage A: Thorium turns to pitch black dioxide instantly at early decay (0.0 to 0.35)
    float dioxidePhase = smoothstep(0.0, 0.35, GlobalDecayFactor);
    
    // Stage B: White carbonate crusting breaks out over the black oxide later on (0.4 to 1.0)
    float carbonateProgression = saturate((GlobalDecayFactor * 1.7) - 0.45);
    float activeCrustMask = saturate((carbonateProgression * 1.4) - (OxidePittingNoise * 0.3));

    // --- STEP 2: MULTI-STAGE LAYER COMPOSITE BLENDING ---
    // Pristine Silver -> Dull Matte Charcoal Black -> Chalky Powdered Gray-White Crust
    float3 passivatedCore = lerp(RawSilverThorium, MatteDioxideBlack, dioxidePhase);
    OutFinalAlbedo = lerp(passivatedCore, ChalkyWhiteCarbonate, activeCrustMask);

    // --- STEP 3: PBR MOLECULAR PROPERTIES TRANSITION ---
    // Fresh thorium is a heavy metal conductor. The dioxide and carbonate states are pure dielectrics.
    float baseMetallic = lerp(1.0, 0.25, dioxidePhase); // Dioxide retains a faint greasy sheen
    OutMetallicPct = lerp(baseMetallic, 0.0, activeCrustMask);

    // Polished silver (Roughness 0.12) transforms into a greasy, pitchy satin black (0.65),
    // and terminates into an intensely light-scattering, dry, chalky white carbonate dust (0.96)
    float baseRoughness = lerp(0.12, 0.65, dioxidePhase);
    OutSurfaceRough = lerp(baseRoughness, lerp(0.85, 0.98, CarbonateScaleSample), activeCrustMask);

    // --- STEP 4: VOLUMETRIC HYDROXIDE SWELLING DISPLACEMENT ---
    // Thorium oxidation expands the layout slightly, creating bumpy, irregular surface changes.
    float crustSwellHeight = CarbonateScaleSample * activeCrustMask * 0.045; // Crust lifts out up to 4.5cm
    float3 oxidePittingWobble = GeometricNormalWS * (dioxidePhase * sin(UV.y * 240.0) * -0.004); // Early micro-pitting craters
    
    OutVertexOffsetWS = (GeometricNormalWS * crustSwellHeight) + oxidePittingWobble;

    // --- STEP 5: ISOTOPIC DISSIPATION EMISSION ---
    // Thorium is radioactive but undergoes clean alpha decay (non-fluorescent). 
    // We add a faint thermal infrared/blue Cherenkov-style accent within the deep oxide fissures 
    // to give it a distinctive gameplay readability cue.
    OutAlphaGlowFactor = dioxidePhase * (1.0 - activeCrustMask) * OxidePittingNoise * 0.25;
}
