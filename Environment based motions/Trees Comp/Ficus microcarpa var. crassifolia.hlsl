// =========================================================================================
// HIGH-SPECIFICATION FRAGMENT RENDERING MATRICES REGISTER BUFFER
// =========================================================================================
cbuffer CrassifoliaMaterialConstants : register(b1)
{
    float3 g_SunDirectionWS             : packoffset(c0.x);
    float  g_CoastalMistMoistureScale   : packoffset(c0.w); // Controls dynamic wetness glints on leaf cuticles

    float3 g_AlbedoBaseChalkyGreyBark   : packoffset(c1.x); // Underlying ash-grey trunk pigment
    float  g_CaudexRoughnessCoefficient : packoffset(c1.w);

    float3 g_AlbedoWaxyWhiteSaltCrust   : packoffset(c2.x); // High salinity atmospheric deposit color profile
    float  g_LeafCuticleMirrorSpecular  : packoffset(c2.w); // Master microfacet scale for mirror-like sheen reflection

    float3 g_ThickObovateFoliageAlbedo  : packoffset(c3.x); // Deep opaque leather-green color
    float  g_InternalTissueDensityScale : packoffset(c3.w); // Thick leaves block backlight; lower transmission rates
};

struct FragmentInputStreamCache
{
    float4 PositionCS           : SV_POSITION;
    float3 NormalWS             : NORMAL;
    float3 PositionWS           : TEXCOORD0;
    float2 UVTracks             : TEXCOORD1;
};

// =========================================================================================
// MAIN CORE PIXEL PASS FOR LEATHERY EXOTIC COASTAL CANOPIES
// =========================================================================================
float4 PS_FicusCrassifoliaMasterPipeline(FragmentInputStreamCache input) : SV_Target
{
    float3 N = normalize(input.NormalWS);
    
    // Unpack structure categorization tags passed down through normal vectors
    float architecturalID = input.NormalWS.z; 
    N.z = sqrt(max(0.0f, 1.0f - N.x * N.x - N.y * N.y));

    float3 L = normalize(g_SunDirectionWS);
    float3 V = normalize(float3(0.0f, 8.0f, -10.0f) - input.PositionWS);
    float3 H = normalize(L + V);

    float ndotl = saturate(dot(N, L));
    float ndoth = saturate(dot(N, H));
    float ndotv = saturate(dot(N, V));

    float3 outputColorPayloadDiffuse  = (float3)0.0f;
    float3 outputColorPayloadSpecular = (float3)0.0f;

    // -------------------------------------------------------------------------------------
    // PIPELINE STRATUM 1: HIGH-GLOSS OBOVATE THICK LEAF MEMBRANES
    // -------------------------------------------------------------------------------------
    if (architecturalID > 0.5f)
    {
        float3 leafBaseAlbedo = g_ThickObovateFoliageAlbedo;

        // MULTI-LOBE GGX SPECTRAL APPROXIMATION
        // Thick Crassifolia cuticles possess double-layer reflections: 
        // 1. A sharp glass-like surface glare layer + 2. Deep chlorophyll tissue scattering
        float dynamicRoughness = lerp(0.06f, 0.22f, saturate(1.0f - g_LeafCuticleMirrorSpecular));
        
        // Sharpen reflection matrices if coastal mist makes the surface wet
        dynamicRoughness = lerp(dynamicRoughness, 0.01f, g_CoastalMistMoistureScale);

        float  a2 = dynamicRoughness * dynamicRoughness;
        float  denomDistribution = (ndoth * ndoth * (a2 - 1.0f) + 1.0f);
        float  specularGGXDistribution = a2 / (3.14159265f * denomDistribution * denomDistribution);

        // Low Subsurface Transmission (Thick cross-section walls absorb photons rapidly)
        float backwardTranslucencyGlow = pow(saturate(dot(V, -L)), 8.0f) * (0.1f * g_InternalTissueDensityScale);
        float3 scatteringComponent = leafBaseAlbedo * backwardTranslucencyGlow * float3(0.6f, 0.9f, 0.3f);

        outputColorPayloadDiffuse  = leafBaseAlbedo * (ndotl + 0.05f) + scatteringComponent;
        outputColorPayloadSpecular = float3(1.0f, 1.0f, 1.0f) * specularGGXDistribution * ndotl * 0.4f;
    }
    // -------------------------------------------------------------------------------------
    // PIPELINE STRATUM 0: SWOLLEN CAUDEX BASES & CODES WITH SALINE CRUSTS
    // -------------------------------------------------------------------------------------
    else
    {
        // Unpack dynamic salinity surface tracking masks computed by core generator loops
        float localizedCrustWeight = saturate(input.UVTracks.y * 0.15f); // Extract crust factor alignment
        
        // Transition from pure charcoal bark to salt-crusted marine boundary protection coats
        float3 compositedBarkAlbedo = lerp(g_AlbedoBaseChalkyGreyBark, g_AlbedoWaxyWhiteSaltCrust, localizedCrustWeight);
        float  activeRoughness      = lerp(g_CaudexRoughnessCoefficient, 0.85f, localizedCrustWeight); // Salt crust is completely diffuse

        outputColorPayloadDiffuse  = compositedBarkAlbedo * (ndotl + 0.12f);
        outputColorPayloadSpecular = (float3)pow(ndoth, 8.0f) * 0.01f; // High rough scattering profiles
    }

    float3 finalFidelityRGB = outputColorPayloadDiffuse + outputColorPayloadSpecular;
    return float4(max(0.0f, finalFidelityRGB), 1.0f);
}
