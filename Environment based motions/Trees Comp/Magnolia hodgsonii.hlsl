// =========================================================================================
// MAGNOLIA HODGSONII MASTER PRODUCTION MATERIAL SHADING REGISTER REGIONS
// =========================================================================================
cbuffer MagnoliaMaterialPipelineConstants : register(b1)
{
    float3 g_MainLightSourceDirectionWS  : packoffset(c0.x);
    float  g_MicroScaleCavityOcclusion   : packoffset(c0.w);

    float3 g_FlutedBarkBaseAlbedoColor   : packoffset(c1.x); // Dark ash brown weathered trunk tone
    float  g_BarkBaseRoughnessScale      : packoffset(c1.w);

    float3 g_ThickLeatheryLeafAlbedo     : packoffset(c2.x); // Deep forest green waxy shade
    float  g_LeafUpperCuticleGlossPower  : packoffset(c2.w); // High-frequency specular response coefficient

    float3 g_IvoryBlossomPetalAlbedo     : packoffset(c3.x); // Pure creamy milky white flower tone
    float  g_SubsurfacePetalScatterDepth : packoffset(c3.w); // Controls light bleeding depth inside giant blooms
};

struct PixelShadingPipelineInput
{
    float4 ProjectedScreenSVPositionCS : SV_POSITION;
    float3 SpatialNormalWS             : NORMAL;
    float3 SpatialWorldCoordinatesWS   : TEXCOORD0;
    float2 SurfaceMappingUV            : TEXCOORD1;
};

// =========================================================================================
// ADVANCED MASTER PIXEL PIPELINE EXECUTION MODULE
// =========================================================================================
float4 PS_MagnoliaHodgsoniiMaterialPipeline(PixelShadingPipelineInput input) : SV_Target
{
    float3 N = normalize(input.SpatialNormalWS);
    
    // Extract classification parameters injected directly into normal tracking lines
    float elementClassificationIndex = input.SpatialNormalWS.z;
    N.z = sqrt(max(0.0f, 1.0f - N.x * N.x - N.y * N.y)); // Hemispherical reconstruction loop

    float3 L = normalize(g_MainLightSourceDirectionWS);
    float3 V = normalize(float3(0.0f, 12.0f, -16.0f) - input.SpatialWorldCoordinatesWS);
    float3 H = normalize(L + V);

    float ndotl = saturate(dot(N, L));
    float ndoth = saturate(dot(N, H));
    float ndotv = saturate(dot(N, V));

    float3 finalComputedDiffuseRGB  = (float3)0.0f;
    float3 finalComputedSpecularRGB = (float3)0.0f;

    // -------------------------------------------------------------------------------------
    // TRACK CLASS 1.0: MASSIVE LEATHERY LEAF BLADES (HIGH INERTIA WAXY CUTICLE)
    // -------------------------------------------------------------------------------------
    if (elementClassificationIndex > 0.5f && elementClassificationIndex < 1.5f)
    {
        float3 baseLeafRGB = g_ThickLeatheryLeafAlbedo;

        // DUAL-LOBE SPECULAR CONFIGURATION FOR THICK MAGNOLIA LEAVES
        // First Lobe: Sharp waxy cuticle mirror sheen reflection layer
        float sharpSpecularLobe = pow(ndoth, lerp(64.0f, 512.0f, g_LeafUpperCuticleGlossPower)) * g_LeafUpperCuticleGlossPower;
        
        // Second Lobe: Broad diffuse structural wax blur reflection layer
        float blurrySpecularLobe = pow(ndoth, 16.0f) * 0.12f;

        // Leaf Backlight Absorption Factor (Thick leather walls pass minimal subsurface light)
        float lightTransmissionFactor = pow(saturate(dot(V, -L)), 12.0f) * 0.04f; // Extreme high absorption rate

        finalComputedDiffuseRGB  = baseLeafRGB * (ndotl + 0.05f) + (baseLeafRGB * lightTransmissionFactor * float3(0.4f, 0.8f, 0.2f));
        finalComputedSpecularRGB = float3(0.92f, 1.0f, 0.95f) * (sharpSpecularLobe + blurrySpecularLobe);
    }
    // -------------------------------------------------------------------------------------
    // TRACK CLASS 2.0: TERMINAL SOLITARY BLOOMING FLOWERS (GIANT IVORY BLOSSOMS WITH SSS)
    // -------------------------------------------------------------------------------------
    else if (elementClassificationIndex > 1.5f)
    {
        float3 petalBaseColor = g_IvoryBlossomPetalAlbedo;

        // HIGH-FIDELITY SUBSURFACE SCATTERING (SSS) PROXIMAL MODEL
        // Light deeply penetrates thick magnolia ivory petals, scattering internally to create an organic glow
        float internalTranslucencyGlow = pow(saturate(dot(V, -L)), 3.0f) * g_SubsurfacePetalScatterDepth;
        float3 sssColorPayload = float3(1.0f, 0.88f, 0.76f) * internalTranslucencyGlow * 0.65f; // Creamy warm internal bounce tint

        // Velvet-like soft microfacet surface sheen scattering
        float softVelvetSheen = pow(1.0f - ndoth, 4.0f) * 0.25f;

        finalComputedDiffuseRGB  = petalBaseColor * (ndotl + 0.15f) + sssColorPayload;
        finalComputedSpecularRGB = float3(1.0f, 0.98f, 0.95f) * (pow(ndoth, 32.0f) * 0.05f + softVelvetSheen);
    }
    // -------------------------------------------------------------------------------------
    // TRACK CLASS 0.0: FLUTED CROWN TIMBER & STIPULE SCAR ANCHOR RINGS
    // -------------------------------------------------------------------------------------
    else
    {
        // Extract procedural stipule rings tracked from the generator pass variables
        float stipuleRingMask = saturate(input.SurfaceMappingUV.y);
        
        // Annular rings are rougher and darker than surrounding wood fibers
        float3 activeWoodAlbedo = lerp(g_FlutedBarkBaseAlbedoColor, g_FlutedBarkBaseAlbedoColor * 0.7f, stipuleRingMask);
        float  activeRoughness  = lerp(g_BarkBaseRoughnessScale, 0.95f, stipuleRingMask);

        finalComputedDiffuseRGB  = activeWoodAlbedo * (ndotl + g_MicroScaleCavityOcclusion * 0.1f);
        finalComputedSpecularRGB = (float3)pow(ndoth, 8.0f) * (0.02f * (1.0f - activeRoughness));
    }

    float3 finalUnifiedOutputRGB = finalComputedDiffuseRGB + finalComputedSpecularRGB;
    return float4(max(0.0f, finalUnifiedOutputRGB), 1.0f);
}
