// =========================================================================================
// REGISTER BUFFER DECLARATIONS FOR THE COLOSSAL FIG PIPELINE CONTEXT
// =========================================================================================
cbuffer MoretonBayMaterialProperties : register(b1)
{
    float3 g_SunDirectionalLightWS       : packoffset(c0.x);
    float  g_SoilWeatheringIntensity     : packoffset(c0.w); // Controls global moss/dirt crawl values

    float3 g_LeafUpperDeepGreenAlbedo    : packoffset(c1.x); // Dark polished forest emerald green
    float  g_LeafUpperCuticleGlossiness  : packoffset(c1.w);

    float3 g_LeafLowerRustyBrownAlbedo   : packoffset(c2.x); // Golden ochre velvet/rusty undertone
    float  g_LeafLowerVelvetMicroRoughness : packoffset(c2.w);

    float3 g_EarthyButtressBarkAlbedo    : packoffset(c3.x); // Ash silver-grey weathered wood color
    float  g_GroundDirtColorPayload      : packoffset(c3.w);
};

struct PixelPipelineInputPayload
{
    float4 ScreenSpaceViewportPositionCS : SV_POSITION;
    float3 InterpolatedNormalWS          : NORMAL;
    float3 WorldCoordinatesWS            : TEXCOORD0;
    float4 PackedVertexChannelAttributes : TEXCOORD1; // X=TensionScore, Y=GroundProximity, Z=LeafSideMask
};

// =========================================================================================
// MASSIVE RUNTIME OPTIMIZED PIXEL SHADER COMPUTE MODULE
// =========================================================================================
float4 PS_MoretonBayFigMaterialPipeline(PixelPipelineInputPayload input) : SV_Target
{
    float3 N = normalize(input.InterpolatedNormalWS);
    float3 L = normalize(g_SunDirectionalLightWS);
    float3 V = normalize(float3(0.0f, 15.0f, -22.0f) - input.WorldCoordinatesWS);
    float3 H = normalize(L + V);

    float ndotl = saturate(dot(N, L));
    float ndoth = saturate(dot(N, H));
    float ndotv = saturate(dot(N, V));

    float mechanicalTensionStress = input.PackedVertexChannelAttributes.x;
    float groundProximityFactor   = input.PackedVertexChannelAttributes.y;
    float globalElementIdentity   = input.PackedVertexChannelAttributes.z;

    float3 finalPixelDiffuseOutRGB  = (float3)0.0f;
    float3 finalPixelSpecularOutRGB = (float3)0.0f;

    // -------------------------------------------------------------------------------------
    // MODULE 1.0: BICOLOR LEAF SYSTEM (WAXY GREEN TOP VS RUSTY VELVET BACK)
    // -------------------------------------------------------------------------------------
    if (globalElementIdentity > 0.5f)
    {
        // Branch flipping identification vector check: Evaluates if face normal geometry points down or up
        float facingOrientationEvaluator = N.y; 
        
        // Dynamically shift color spectrum depending on geometric side exposure
        float sideInversionLerper = saturate(facingOrientationEvaluator * 2.0f + 0.5f);

        float3 activeLeafAlbedoRGB = lerp(g_LeafLowerRustyBrownAlbedo, g_LeafUpperDeepGreenAlbedo, sideInversionLerper);

        // Calculate dual lobe spec values for opposite leaf sides
        // Top Side: High sheen waxy gloss response
        float topSpecularLobe = pow(ndoth, lerp(32.0f, 512.0f, g_LeafUpperCuticleGlossiness)) * 0.15f;
        
        // Bottom Side: Soft fuzzy retroreflective scattering with near-zero mirror gloss
        float bottomRetroFuzzyLobe = pow(1.0f - ndoth, 3.0f) * g_LeafLowerVelvetMicroRoughness * 0.08f;

        float3 combinedSpecularResponse = lerp(float3(0.85f, 0.82f, 0.78f) * bottomRetroFuzzyLobe, 
                                               float3(1.0f, 1.0f, 1.0f) * topSpecularLobe, 
                                               sideInversionLerper);

        finalPixelDiffuseOutRGB  = activeLeafAlbedoRGB * (ndotl + 0.08f);
        finalPixelSpecularOutRGB = combinedSpecularResponse;
    }
    // -------------------------------------------------------------------------------------
    // MODULE 2.0: COLOSSAL BARK WALLS & GROUND WEATHERING SYSTEMS
    // -------------------------------------------------------------------------------------
    else
    {
        float3 baselineBarkColor = g_EarthyButtressBarkAlbedo;

        // Visualise mechanical high-stress tension wood lines via procedural brightness shifts
        baselineBarkColor = lerp(baselineBarkColor, baselineBarkColor * 1.15f, mechanicalTensionStress * 0.4f);

        // LAYER MODULE: GROUND PROXIMITY MOSS/DIRT INJECTION
        // Gradually blends soil-level mud textures over base bark parameters near the ground
        float3 environmentalSoilRGB = float3(0.18f, 0.14f, 0.08f); // Deep damp compost mud tone
        float activeSoilMixWeight   = groundProximityFactor * g_SoilWeatheringIntensity;

        float3 finishedBarkAlbedoRGB = lerp(baselineBarkColor, environmentalSoilRGB, activeSoilMixWeight);
        float activeSurfaceRoughness = lerp(0.82f, 0.98f, activeSoilMixWeight);

        finalPixelDiffuseOutRGB  = finishedBarkAlbedoRGB * ndotl;
        finalPixelSpecularOutRGB = (float3)(pow(ndoth, 4.0f) * (0.02f * (1.0f - activeSurfaceRoughness)));
    }

    float3 finalUnifiedCompositionRGB = finalPixelDiffuseOutRGB + finalPixelSpecularOutRGB;
    return float4(max((float3)0.0f, finalUnifiedCompositionRGB), 1.0f);
}
