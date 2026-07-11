// =========================================================================================
// REGISTER BUFFER BINDINGS FOR FICUS KRISHNAE PIPELINE CONTEXTS
// =========================================================================================
cbuffer FicusPerObjectMaterialConstants : register(b1)
{
    float3 g_MainDirectionalLightDirWS  : packoffset(c0.x);
    float  g_GlobalRainIntensitySystem  : packoffset(c0.w); // Scales wetness specular responses dynamically

    float3 g_FicusFolliageAlbedoColor   : packoffset(c1.x); // Medium waxy bright green tone
    float  g_CupLeafInteriorOcclusionScale : packoffset(c1.w); // Controls darkness depth inside pockets

    float3 g_PropRootFibrousAlbedo      : packoffset(c2.x); // Earthy grey-brown multi-string bark hue
    float  g_TranslucencySpillMultiplier : packoffset(c2.w); // Light transmission modifier through leaf layers
};

struct VertexShaderToPixelShaderInput
{
    float4 ScreenProjectedCoordsCS  : SV_POSITION;
    float3 InterpolatedNormalWS     : NORMAL;
    float3 SpatialWorldPositionWS   : TEXCOORD0;
    float3 PackedFicusChannelData   : TEXCOORD1; // X = CupPocketDepth, Y = DynamicRainLoad, Z = ComponentTypeID
};

// =========================================================================================
// LIGHTWEIGHT DYNAMIC PBR PIXEL SHADER OPERATOR
// =========================================================================================
float4 PS_FicusKrishnaeMasterPipeline(VertexShaderToPixelShaderInput input) : SV_Target
{
    float3 N = normalize(input.InterpolatedNormalWS);
    float3 L = normalize(g_MainDirectionalLightDirWS);
    float3 V = normalize(float3(0.0f, 10.0f, -15.0f) - input.SpatialWorldPositionWS);
    float3 H = normalize(L + V);

    float ndotl = saturate(dot(N, L));
    float ndoth = saturate(dot(N, H));
    float ndotv = saturate(dot(N, V));

    float leafCupStructuralDepth = input.PackedFicusChannelData.x;
    float individualComponentTag = input.PackedFicusChannelData.z;

    float3 computedMaterialDiffuseRGB  = (float3)0.0f;
    float3 computedMaterialSpecularRGB = (float3)0.0f;

    // -------------------------------------------------------------------------------------
    // EXECUTION SEGMENT 1.0: POCKET-CUP LEAF STRUCTURE (KRISHNA'S BUTTERCUP FUNCTION)
    // -------------------------------------------------------------------------------------
    if (individualComponentTag > 0.5f && individualComponentTag < 1.5f)
    {
        float3 foliageAlbedo = g_FicusFolliageAlbedoColor;

        // POCKET INTERIOR SHADOW TRAPPING MATH (AMBIENT OCCLUSION SIMULATION)
        // Deeper interior folds of the cup block ambient environmental light reflections completely
        float proceduralCupAO = lerp(1.0f, max(0.02f, 1.0f - g_CupLeafInteriorOcclusionScale), leafCupStructuralDepth);

        // THIN-WALL TRANSLUCENCY BACK-LIGHTING LOOP
        // When looking against light rays, leaf membranes glow with vibrant scattered chlorophyll profiles
        float lightTransmissionIntensity = pow(saturate(dot(V, -L)), 4.0f) * g_TranslucencySpillMultiplier;
        
        // Dynamic water layer wetness specular enhancement inside cup wells
        float wetSpecularAddition = leafCupStructuralDepth * g_GlobalRainIntensitySystem * 0.45f;
        float operationalGlossiness = lerp(32.0f, 256.0f, wetSpecularAddition);

        float3 translucencyTint = float3(0.55f, 0.88f, 0.12f) * lightTransmissionIntensity;

        computedMaterialDiffuseRGB  = (foliageAlbedo * (ndotl + 0.1f) * proceduralCupAO) + translucencyTint;
        computedMaterialSpecularRGB = float3(1.0f, 1.0f, 1.0f) * (pow(ndoth, operationalGlossiness) * (0.08f + wetSpecularAddition));
    }
    // -------------------------------------------------------------------------------------
    // EXECUTION SEGMENT 2.0: AERIAL PROP ROOT CURTAINS (PENDULAR STRINGS)
    // -------------------------------------------------------------------------------------
    else if (individualComponentTag > 1.5f)
    {
        float3 rootAlbedo = g_PropRootFibrousAlbedo;

        // High-frequency anisotropic specular illusion to mimic complex interwoven root fibers
        float rootAnisotropicSheen = saturate(sqrt(1.0f - ndoth * ndoth));
        float fibrousSpecularLobe  = pow(rootAnisotropicSheen, 16.0f) * 0.04f;

        computedMaterialDiffuseRGB  = rootAlbedo * (ndotl + 0.08f);
        computedMaterialSpecularRGB = float3(0.75f, 0.72f, 0.68f) * fibrousSpecularLobe;
    }
    // -------------------------------------------------------------------------------------
    // EXECUTION SEGMENT 0.0: CORRUGATED CONSOLIDATED TRUNK CORE
    // -------------------------------------------------------------------------------------
    else
    {
        // Bark textures utilize flat Lambertian profiles with deep shadow attenuation values
        computedMaterialDiffuseRGB  = g_PropRootFibrousAlbedo * 0.8f * ndotl;
        computedMaterialSpecularRGB = (float3)(pow(ndoth, 6.0f) * 0.01f);
    }

    // Combine shading lobes and clamp execution values inside legal bounds
    float3 finalPixelOutputRGB = computedMaterialDiffuseRGB + computedMaterialSpecularRGB;
    return float4(max((float3)0.0f, finalPixelOutputRGB), 1.0f);
}
