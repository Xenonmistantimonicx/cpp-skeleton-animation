// =========================================================================================
// UNIFORM DEFINITIONS FOR TROPICAL BANYAN COUNCIL TREES
// =========================================================================================
cbuffer FicusAltissimaMaterialConstants : register(b1)
{
    float3 g_SunDirectionWS              : packoffset(c0.x);
    float  g_VeinVibrancyIntensity       : packoffset(c0.w); // Controls brightness of yellow midribs

    float3 g_LeafBaseDarkGreenRGB        : packoffset(c1.x); // High gloss polished deep emerald
    float  g_SyconiaOrangeGlossiness     : packoffset(c1.w);

    float3 g_BrightLemonYellowVeinRGB    : packoffset(c2.x); // Distinct ivory/yellow vein color
    float  g_SubsurfaceCanopyScattering  : packoffset(c2.w);

    float3 g_VividOrangeSyconiaFruitRGB  : packoffset(c3.x); // Axillary circular fruit pairs hue
};

struct VertexShaderToPixelShader
{
    float4 ProjectionSpacePosCS       : SV_POSITION;
    float3 NormalWS                   : NORMAL;
    float3 PositionWS                 : TEXCOORD0;
    float4 PackedAltissimaAttributes  : TEXCOORD1; // X=LoadStress, Y=VeinMask, Z=FruitWeight
};

// =========================================================================================
// RUNTIME HIGH-FIDELITY CANOPY PIXEL PROCESSING COMPUTE
// =========================================================================================
float4 PS_FicusAltissimaMaterialPipeline(VertexShaderToPixelShader input) : SV_Target
{
    float3 N = normalize(input.NormalWS);
    float3 L = normalize(g_SunDirectionWS);
    float3 V = normalize(float3(0.0f, 10.0f, -18.0f) - input.PositionWS);
    float3 H = normalize(L + V);

    float ndotl = saturate(dot(N, L));
    float ndoth = saturate(dot(N, H));
    float ndotv = saturate(dot(N, V));

    float dynamicCompressionLoad  = input.PackedAltissimaAttributes.x;
    float leafVeinPatternMask     = input.PackedAltissimaAttributes.y;
    float fruitIdentityFactor     = input.PackedAltissimaAttributes.z;

    float3 computedDiffuseRGB  = (float3)0.0f;
    float3 computedSpecularRGB = (float3)0.0f;

    // -------------------------------------------------------------------------------------
    // MODULE 1.0: AXILLARY FRUIT RENDERING ENGINE (VIVID ORANGE SYCONIA)
    // -------------------------------------------------------------------------------------
    if (fruitIdentityFactor > 0.7f)
    {
        // Syconia display a dense, waxy opaque skin texture profile
        float3 fruitAlbedo = g_VividOrangeSyconiaFruitRGB;
        
        // Adding micro subsurface skin scattering effects for realistic organic weight look
        float organicFruitSSS = pow(1.0f - ndotl, 2.0f) * g_SubsurfaceCanopyScattering * 0.4f;

        computedDiffuseRGB  = (fruitAlbedo * ndotl) + (float3(0.9f, 0.3f, 0.0f) * organicFruitSSS);
        computedSpecularRGB = (float3)(pow(ndoth, 16.0f) * g_SyconiaOrangeGlossiness * 0.06f);
    }
    // -------------------------------------------------------------------------------------
    // MODULE 2.0: POLISHED LEAF BLADE WITH HIGH-CONTRAST LEMON-YELLOW VEINS
    // -------------------------------------------------------------------------------------
    else
    {
        // Blend high contrast ivory-yellow branch lines onto the base shiny emerald green blade
        float3 masterLeafAlbedo = lerp(g_LeafBaseDarkGreenRGB, g_BrightLemonYellowVeinRGB, leafVeinPatternMask * g_VeinVibrancyIntensity);

        // Highly polished waxy layer creates a tight, sharp specular highlight response loop
        float highGlossCuticleSheen = pow(ndoth, 128.0f) * 0.22f;

        // Dynamic translucency scattering as ambient sun passes down through the canopy cluster sheets
        float canopyTranslucencyScatter = pow(saturate(dot(-L, V)), 4.0f) * g_SubsurfaceCanopyScattering;

        computedDiffuseRGB  = (masterLeafAlbedo * (ndotl + 0.05f)) + (g_BrightLemonYellowVeinRGB * canopyTranslucencyScatter * 0.3f);
        computedSpecularRGB = float3(1.0f, 1.0f, 1.0f) * highGlossCuticleSheen;
    }

    float3 finalMaterialOutputRGB = computedDiffuseRGB + computedSpecularRGB;
    return float4(max((float3)0.0f, finalMaterialOutputRGB), 1.0f);
}
