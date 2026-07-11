// =========================================================================================
// REGISTER UNIFORM BINDINGS FOR HARD-LEAF FIG ENGINE PIPELINES
// =========================================================================================
cbuffer FicusCallosaMaterialProperties : register(b1)
{
    float3 g_MainSunDirectionalWS         : packoffset(c0.x);
    float  g_ScabrousMicroScatteringScale : packoffset(c0.w); // Controls sandpaper diffuse spread

    float3 g_CallosaDryLeafAlbedoRGB      : packoffset(c1.x); // Tough matte dark yellow-green tint
    float  g_MottledBarkPaleGreyScale     : packoffset(c1.w);

    float3 g_PaleGreyColumnarTrunkAlbedo  : packoffset(c2.x); // Ash-grey chalky bark hue
    float  g_TerminalTwigRigidityScaler   : packoffset(c2.w);
};

struct PixelPipelineShaderInput
{
    float4 ProjectionCoordsCS         : SV_POSITION;
    float3 WorldNormalWS              : NORMAL;
    float3 FragmentPositionWS         : TEXCOORD0;
    float4 CustomPackedFicusMetrics   : TEXCOORD1; // X=ScabrousRoughness, Y=TwigMask, Z=HeightIndex
};

// =========================================================================================
// PRODUCTION-GRADE RENDER EXECUTION ENGINE MODULE
// =========================================================================================
float4 PS_FicusCallosaMaterialPipeline(PixelPipelineShaderInput input) : SV_Target
{
    float3 N = normalize(input.WorldNormalWS);
    float3 L = normalize(g_MainSunDirectionalWS);
    float3 V = normalize(float3(0.0f, 12.0f, -20.0f) - input.FragmentPositionWS);
    float3 H = normalize(L + V);

    float ndotl = saturate(dot(N, L));
    float ndoth = saturate(dot(N, H));
    float ndotv = saturate(dot(N, V));

    float scabrousRoughnessMultiplier = input.CustomPackedFicusMetrics.x;
    float swollenTwigNoduleIdentity   = input.CustomPackedFicusMetrics.y;
    float verticalHeightFactor        = input.CustomPackedFicusMetrics.z;

    float3 finalDiffuseOutputRGB  = (float3)0.0f;
    float3 finalSpecularOutputRGB = (float3)0.0f;

    // -------------------------------------------------------------------------------------
    // PIPELINE INTERACTION SEGMENT 1.0: SCABROUS SANDPAPER LEAF SYSTEM (OREN-NAYAR BASE)
    // -------------------------------------------------------------------------------------
    if (swollenTwigNoduleIdentity > 0.5f)
    {
        // High frequency sandpaper rough surface approximation loop
        float roughnessSquare = scabrousRoughnessMultiplier * scabrousRoughnessMultiplier;
        
        // Oren-Nayar diffuse coefficients for micro-calcified rough leaves mapping
        float A_Factor = 1.0f - 0.5f * (roughnessSquare / (roughnessSquare + 0.33f));
        float B_Factor = 0.45f * (roughnessSquare / (roughnessSquare + 0.09f));

        float viewAngleCosine  = acos(ndotv);
        float lightAngleCosine = acos(ndotl);
        
        float alphaAngle = max(viewAngleCosine, lightAngleCosine);
        float betaAngle  = min(viewAngleCosine, lightAngleCosine);
        
        // Finalized retroreflective diffuse scattering intensity
        float scabrousDiffuseScatter = ndotl * (A_Factor + B_Factor * max(0.0f, dot(normalize(V - N * ndotv), normalize(L - N * ndotl))) * sin(alphaAngle) * tan(betaAngle));

        // Callosa leaves completely suppress glossy metallic mirror-like highlights
        float hardLeafSpecularSuppress = pow(ndoth, 8.0f) * (1.0f - scabrousRoughnessMultiplier) * 0.02f;

        finalDiffuseOutputRGB  = g_CallosaDryLeafAlbedoRGB * (scabrousDiffuseScatter * g_ScabrousMicroScatteringScale);
        finalSpecularOutputRGB = float3(0.9f, 0.95f, 0.9f) * hardLeafSpecularSuppress;
    }
    // -------------------------------------------------------------------------------------
    // PIPELINE INTERACTION SEGMENT 2.0: MOTTLED PALE GREY COLUMNAR TRUNK CORE
    // -------------------------------------------------------------------------------------
    else
    {
        float3 basicTrunkBaseAlbedo = g_PaleGreyColumnarTrunkAlbedo;

        // Simulate dynamic vertical chalky ridges across the columnar axis using micro-noise distributions
        float ridgeFlickerNoise = saturate(sin(input.FragmentPositionWS.y * 8.0f) * cos(input.FragmentPositionWS.x * 4.0f));
        basicTrunkBaseAlbedo = lerp(basicTrunkBaseAlbedo, basicTrunkBaseAlbedo * 0.85f, ridgeFlickerNoise * g_MottledBarkPaleGreyScale);

        // Standard flat Lambertian profile for rough chalky surfaces
        finalDiffuseOutputRGB  = basicTrunkBaseAlbedo * (ndotl + 0.05f);
        finalSpecularOutputRGB = (float3)(pow(ndoth, 2.0f) * 0.005f); // Microscopic specular reflection setup
    }

    float3 renderedCompositeOutputRGB = finalDiffuseOutputRGB + finalSpecularOutputRGB;
    return float4(max((float3)0.0f, renderedCompositeOutputRGB), 1.0f);
}
