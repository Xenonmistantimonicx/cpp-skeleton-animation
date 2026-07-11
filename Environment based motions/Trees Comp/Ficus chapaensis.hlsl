// =========================================================================================
// PRODUCTION GRAPHICS MULTI-PASS REGISTER MANAGEMENT SYSTEM
// =========================================================================================
cbuffer FicusChapaPipelineConstants : register(b1)
{
    float3 g_SolarDirectionWS           : packoffset(c0.x);
    float  g_SeasonalSheddingValue      : packoffset(c0.w); // Range [0.0 = Full canopy density, 1.0 = Shed clean]

    float3 g_FusedTrunkBarkAlbedo       : packoffset(c1.x); // Light ash grey/tan adaptive wood coloration
    float  g_StranglerBarkRoughness     : packoffset(c1.w);

    float3 g_CaulifloryFruitRipenAlbedo : packoffset(c2.x); // Deep maroon/purplish-red fig fruit signature
    float  g_FruitWaxGlossinessLevel    : packoffset(c2.w); // High-frequency glossy coat factor for ripened fig skin

    float3 g_GlossyLeafAlbedoVector    : packoffset(c3.x); // Rich jade green sheen coating color
    float  g_GlobalAmbientOcclusionScale : packoffset(c3.w); 
};

struct PixelInputVertexCache
{
    float4 HardwareSVPositionCS : SV_POSITION;
    float3 TransformedNormalWS  : NORMAL;
    float3 SpatialPositionWS    : TEXCOORD0;
    float2 TexCoordUV           : TEXCOORD1;
};

// =========================================================================================
// MAIN AAA RASTER PASS FOR MULTI-LOBE ORGANIC ASSET ARCHITECTURES
// =========================================================================================
float4 PS_FicusChapaMasterPipeline(PixelInputVertexCache input) : SV_Target
{
    // Recover unpacked identification data streams cached within standard vector pipelines
    float3 N = normalize(input.TransformedNormalWS);
    float segmentID = input.TransformedNormalWS.z; // Classification tracking tag
    
    // Safety re-normalization loop bounds
    N.z = sqrt(max(0.0f, 1.0f - N.x * N.x - N.y * N.y));

    float3 L = normalize(g_SolarDirectionWS);
    float3 V = normalize(float3(0.0f, 10.0f, -12.0f) - input.SpatialPositionWS);
    float3 H = normalize(L + V);

    float ndotl = saturate(dot(N, L));
    float ndoth = saturate(dot(N, H));
    float ndotv = saturate(dot(N, V));

    float3 accumulatedDiffuseLighting  = (float3)0.0f;
    float3 accumulatedSpecularLighting = (float3)0.0f;
    float  activeOpacityOutputValue    = 1.0f;

    // -------------------------------------------------------------------------------------
    // TRACK FLAG 0.0: MASTER TIMBER & INTERLACED STRANGLER ROOT NETWORKS
    // -------------------------------------------------------------------------------------
    if (segmentID < 0.5f)
    {
        // Custom micro-shadow tracking calculation for complex braided geometry creases
        float rootCavityShadowFactor = saturate(abs(sin(input.TexCoordUV.x * 6.283f)));
        float3 adaptiveBarkColor = g_FusedTrunkBarkAlbedo * (rootCavityShadowFactor * 0.4f + 0.6f);

        accumulatedDiffuseLighting = adaptiveBarkColor * (ndotl + g_GlobalAmbientOcclusionScale * 0.12f);
        accumulatedSpecularLighting = (float3)pow(ndoth, 24.0f) * 0.03f; // Dull matte fiber response
    }
    // -------------------------------------------------------------------------------------
    // TRACK FLAG 2.0: CAULIFLORY FRUITS (TRUNK-BORN FIG BUNCHES)
    // -------------------------------------------------------------------------------------
    else if (segmentID > 1.5f)
    {
        // Deep purplish waxy fig skin logic modeling high skin density specular lobes
        float3 matureFruitColor = g_CaulifloryFruitRipenAlbedo;
        
        // High density waxy gloss highlight typical of smooth fig outer surfaces
        float waxySkinGloss = pow(ndoth, lerp(32.0f, 256.0f, g_FruitWaxGlossinessLevel)) * g_FruitWaxGlossinessLevel;
        
        accumulatedDiffuseLighting = matureFruitColor * (ndotl + 0.05f);
        accumulatedSpecularLighting = float3(0.95f, 1.0f, 0.92f) * waxySkinGloss;
    }
    // -------------------------------------------------------------------------------------
    // TRACK FLAG 1.0: HIGH CANOPY GLOSSY LEAF MEMBRANES
    // -------------------------------------------------------------------------------------
    else
    {
        accumulatedDiffuseLighting = g_GlossyLeafAlbedoVector * (ndotl + 0.2f);
        accumulatedSpecularLighting = (float3)pow(ndoth, 128.0f) * 0.35f; // Extremely shiny fresh leaf cuticle coating
        
        // Dynamic season drop controller interface integration loop
        activeOpacityOutputValue = saturate(1.0f - g_SeasonalSheddingValue);
    }

    float3 integratedColorPayload = accumulatedDiffuseLighting + accumulatedSpecularLighting;
    return float4(max(0.0f, integratedColorPayload), activeOpacityOutputValue);
}
