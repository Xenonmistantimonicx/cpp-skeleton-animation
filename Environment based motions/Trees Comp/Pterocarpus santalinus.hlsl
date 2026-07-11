// =========================================================================================
// PRODUCTION GRAPHICS SYSTEM: RED SANDERS LOGIC CONTROLLER
// =========================================================================================
cbuffer RedSandersPipelineConstants : register(b1)
{
    float3 g_SunLightDirectionWS          : packoffset(c0.x);
    float  g_BarkWeatheringIntensity      : packoffset(c0.w); // Scales the charcoal/grey oxidation dust overlay

    float3 g_OuterBarkAlbedoCharcoal     : packoffset(c1.x); // Blackish-brown dark weathered coat
    float  g_CrocodilePlateRoughness      : packoffset(c1.w);

    float3 g_InnerHeartwoodAlbedoCrimson  : packoffset(c2.x); // Rich Blood-Red/Deep Maroon core signature
    float  g_HeartwoodResinGloss          : packoffset(c2.w); // High essential oil density provides natural sheen

    float3 g_SapwoodAlbedoCream          : packoffset(c3.x); // Outer layer pale whitish-yellow meat ring
    float  g_DamageExposureFactor         : packoffset(c3.w); // 0.0 = Intact bark surface, 1.0 = Core cut open exposed
};

struct FragmentInputCache
{
    float4 SVPositionCS        : SV_POSITION;
    float3 NormalWS            : NORMAL;
    float3 SpatialPositionWS   : TEXCOORD0;
    float2 TexCoordUV          : TEXCOORD1;
};

// =========================================================================================
// RENDER PASS FOR HIGH-SECURITY EXOTIC TIMBER MATRIX DESIGN
// =========================================================================================
float4 PS_RedSandersMasterMaterial(FragmentInputCache input) : SV_Target
{
    float3 N = normalize(input.NormalWS);
    
    // Unpack data variables tracked through our procedural vertex layout channels
    float internalHeartwoodRatio = input.NormalWS.z; 
    N.z = sqrt(max(0.0f, 1.0f - N.x * N.x - N.y * N.y)); // Maintain unit hemisphere bounds

    float3 L = normalize(g_SunLightDirectionWS);
    float3 V = normalize(float3(5.0f, 4.0f, -10.0f) - input.SpatialPositionWS);
    float3 H = normalize(L + V);

    float ndotl = saturate(dot(N, L));
    float ndoth = saturate(dot(N, H));

    // 1. SURFACE LEVEL: COMPUTE CROCODILE SKIN BARK LAYER
    // Deep cracked lines get high micro-shadow occlusion values
    float microFissureOcclusion = saturate(abs(cos(input.TexCoordUV.y * 14.0f) * sin(input.TexCoordUV.x * 10.0f)));
    float3 baseBarkSurfaceColor = lerp(g_OuterBarkAlbedoCharcoal * 0.4f, g_OuterBarkAlbedoCharcoal, microFissureOcclusion);
    
    // Add realistic environmental dust oxidation over ancient plate peaks
    baseBarkSurfaceColor = lerp(baseBarkSurfaceColor, float3(0.24f, 0.22f, 0.20f), g_BarkWeatheringIntensity * 0.35f);

    // 2. INTERNAL LEVEL: PROCEDURAL DUAL-RING TRANSITION (SAPWOOD TO HEARTWOOD)
    // Core Santalin rings computation containing fine noise growth patterns
    float growthRingNoise = abs(sin(internalHeartwoodRatio * 42.0f + input.SpatialPositionWS.y * 3.0f)) * 0.15f;
    float3 trueExposedInternalWoodAlbedo = lerp(g_SapwoodAlbedoCream, g_InnerHeartwoodAlbedoCrimson - growthRingNoise, internalHeartwoodRatio);

    // 3. DAMAGE STRATIFICATION MATRIX
    // Blend smoothly from weathered dead exterior bark to prized red core based on localized structural cuts
    float3 dynamicTargetAlbedo = lerp(baseBarkSurfaceColor, trueExposedInternalWoodAlbedo, g_DamageExposureFactor);
    float  computedRoughness     = lerp(g_CrocodilePlateRoughness, saturate(1.0f - g_HeartwoodResinGloss), g_DamageExposureFactor);

    // Light Accumulation Engine
    float3 diffuseReflection = dynamicTargetAlbedo * (ndotl + 0.08f); // Soft ambient bounce
    
    // Specular response model changing from completely dead rough charcoal bark to deep oil gloss core resin sheen
    float specularDistribution = pow(ndoth, lerp(12.0f, 128.0f, g_DamageExposureFactor * g_HeartwoodResinGloss));
    float3 specularReflection  = lerp((float3)0.01f, float3(1.0f, 0.88f, 0.85f), g_DamageExposureFactor) * specularDistribution * computedRoughness;

    return float4(max(0.0f, diffuseReflection + specularReflection), 1.0f);
}
