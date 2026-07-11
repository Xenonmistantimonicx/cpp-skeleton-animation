// =========================================================================================
// INTERACTIVE MULTI-PASS PHOTOMETRIC ENGINE REGISTERS MATRIX
// =========================================================================================
cbuffer EmmenopterysSystemConstants : register(b1)
{
    // Global Ambient & Solar Parameters Configuration Channels
    float3 g_SunDirectionVectorWS       : packoffset(c0.x);
    float  g_DeltaGlobalTime            : packoffset(c0.w); // Linked directly to engine timer tickers for wind physics

    // Bark / Timber Core Profiles
    float3 g_AlbedoOldExfoliatingGrey   : packoffset(c1.x); // External dead greyish-brown bark flakes color
    float  g_BarkRoughnessFactor        : packoffset(c1.w);
    float3 g_AlbedoNewInnerYellowBark   : packoffset(c2.x); // Inner fresh yellow sapwood color
    float  g_AutumnSheddingProgress     : packoffset(c2.w); // Interpolation slider index parameter [0.0 = Summer green, 1.0 = Autumn leaf drop]

    // Foliage & Master Velvet Bract Profiles
    float3 g_SummerGreenFoliageAlbedo  : packoffset(c3.x);
    float  g_BractVelvetSheenGloss      : packoffset(c3.w);
    float3 g_PureWhiteVelvetBractAlbedo : packoffset(c4.x); // Glowing snow-white calyx flags color
    float  g_WindSwayVelocityIntensity  : packoffset(c4.w);
};

struct PixelInputFragmentCache
{
    float4 HardwareSVPositionCS : SV_POSITION;
    float3 NormalWS             : NORMAL;
    float3 PositionWS           : TEXCOORD0;
    float2 UVCoord              : TEXCOORD1;
};

// --- PROCEDURAL WIND AND CANOPY FLUTTER SIMULATOR ---
float3 CalculateDynamicWindDisplacement(float3 worldPos, float nodeType, float timeFactor)
{
    // Bract structures (Type 2.0) are light, paper-like velvet sheets that oscillate violently at low drag frequencies
    float scaleFactor = 0.0f;
    if (nodeType > 1.5f)      scaleFactor = 0.45f * g_WindSwayVelocityIntensity; // Bract flags flutter heavily
    else if (nodeType > 0.5f) scaleFactor = 0.15f * g_WindSwayVelocityIntensity; // Standard leaf rustle
    
    float waveOscillation = sin(worldPos.x * 2.0f + timeFactor * 3.5f) * cos(worldPos.z * 1.5f + timeFactor * 2.8f);
    return float3(waveOscillation, waveOscillation * 0.3f, -waveOscillation * 0.5f) * scaleFactor;
}

// =========================================================================================
// CORE AAA PHOTOMETRIC PIXEL FRAGMENT EXPERT EXECUTION PASS
// =========================================================================================
float4 PS_EmmenopterysMasterPipeline(PixelInputFragmentCache input) : SV_Target
{
    // Recover unpacked geometric structures data maps encoded inside normal tracks
    float3 N = normalize(input.NormalWS);
    
    // Unpack structural context flags hidden inside vector channels to reduce memory footpaths
    float nodeTypeEvaluator = input.NormalWS.z; 
    N.z = sqrt(max(0.0f, 1.0f - N.x * N.x - N.y * N.y)); // Reconstruct true normal coordinate spatial bounds

    float3 L = normalize(g_SunDirectionVectorWS);
    float3 V = normalize(float3(0.0f, 15.0f, -12.0f) - input.PositionWS); // Real-time camera matrix lookup approximation
    float3 H = normalize(L + V);

    float ndotl = saturate(dot(N, L));
    float ndotv = saturate(dot(N, V));
    float ndoth = saturate(dot(N, H));

    // Dynamic processing variables setup matrix
    float3 finalDiffuseOutput = (float3)0.0f;
    float3 finalSpecularOutput = (float3)0.0f;
    float  fragmentAlphaOutputValue = 1.0f;

    // -------------------------------------------------------------------------------------
    // PASSTHROUGH ZONE 0: GEOMETRY ENGINE DETERMINES DATA TRACK IS TIMBER / BRANCH WOOD
    // -------------------------------------------------------------------------------------
    if (nodeTypeEvaluator < 0.5f)
    {
        // Reconstruct the Exfoliating peeling bark profile using raw layout index variables
        float proceduralPeelingMask = saturate(input.NormalWS.y); // Extracted exfoliation tracker channel map
        
        // Blend between external grey-brown dead plates and internal fresh yellow wood layer
        float3 compositeWoodAlbedo = lerp(g_AlbedoOldExfoliatingGrey, g_AlbedoNewInnerYellowBark, proceduralPeelingMask);
        
        // High-density rough Oren-Nayar diffuse approximation model
        finalDiffuseOutput = compositeWoodAlbedo * (ndotl + 0.08f);
        finalSpecularOutput = (float3)pow(ndoth, 16.0f) * 0.02f; // Dead organic wood matter has almost zero specular output
    }
    // -------------------------------------------------------------------------------------
    // PASSTHROUGH ZONE 1: GEOMETRY DATA IS BROAD OPOSITE CANOPY LEAVES
    // -------------------------------------------------------------------------------------
    else if (nodeTypeEvaluator < 1.5f)
    {
        // Real-life Deciduous Lifecycle Simulator: Transitioning leaf albedo from green to bright autumn gold-orange
        float3 autumnLeafColor = float3(0.85f, 0.48f, 0.12f); // Vibrant carotenoid cell decomposition color profile
        float3 currentLifecycleAlbedo = lerp(g_SummerGreenFoliageAlbedo, autumnLeafColor, g_AutumnSheddingProgress);

        // Standard foliage wrap diffuse pass
        finalDiffuseOutput = currentLifecycleAlbedo * (ndotl + 0.15f);
        finalSpecularOutput = (float3)pow(ndoth, 64.0f) * 0.12f;
        
        // Leaves shed down completely during high autumn cycle settings
        fragmentAlphaOutputValue = saturate(1.0f - (g_AutumnSheddingProgress * 0.95f)); 
    }
    // -------------------------------------------------------------------------------------
    // PASSTHROUGH ZONE 2: GEOMETRY IS THE GIANT SNOW-WHITE VELVET BRACT FLAGS
    // -------------------------------------------------------------------------------------
    else
    {
        // Bract sheets feature massive translucent cells designed to pass backlight directly into the flower clusters
        float3 bractBaseColor = g_PureWhiteVelvetBractAlbedo;

        // Subsurface scattering wrapped alignment calculation formula
        float thinTissueTransmission = pow(saturate(dot(V, -L)), 4.0f) * 2.5f;
        float3 subsurfaceGlow = bractBaseColor * thinTissueTransmission * float3(0.95f, 0.98f, 0.92f);

        // High-fidelity velvet sheen specular modeling pass via specialized micro-facet distribution profiles
        float velvetSheenHighlight = pow(1.0f - ndotv, 4.0f) * ndotl * g_BractVelvetSheenGloss;

        finalDiffuseOutput = (bractBaseColor * (ndotl + 0.25f)) + subsurfaceGlow;
        finalSpecularOutput = float3(1.0f, 1.0f, 1.0f) * velvetSheenHighlight;
        
        // Edge blending to match soft paper-thin margins of biological structures
        float borderFade = saturate(sin(input.UVCoord.x * 3.14159f) * sin(input.UVCoord.y * 3.14159f));
        fragmentAlphaOutputValue = pow(borderFade, 0.35f);
    }

    // Final color rendering accumulation block
    float3 finalCompositedRGB = finalDiffuseOutput + finalSpecularOutput;
    return float4(max(0.0f, finalCompositedRGB), fragmentAlphaOutputValue);
}
