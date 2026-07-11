// =========================================================================================
// INTERACTIVE FICUS RECONSTRUCTION CONSTANT REGISTERS BUFFER
// =========================================================================================
cbuffer FicusMaterialConstants : register(b1)
{
    float3 g_SunDirectionWS             : packoffset(c0.x);
    float  g_CanopyLeafRustleTimer      : packoffset(c0.w);

    float3 g_AlbedoWhiteColumnarBark    : packoffset(c1.x); // Pristine pale chalky-grey bark albedo
    float  g_BarkBaseRoughness          : packoffset(c1.w);

    float3 g_MilkyLatexSapColor         : packoffset(c2.x); // High opaque liquid white pigment
    float  g_LatexGlossViscosity        : packoffset(c2.w); // Controls the wet mirror factor of fluid tracks

    float3 g_GlossyFicusLeafAlbedo      : packoffset(c3.x); // Rich thick green canopy shade
    float  g_DeciduousSheddingState     : packoffset(c3.w); // [0.0 = Full Green Canopy, 1.0 = Bare winter branches]
};

struct PixelInputCache
{
    float4 PositionCS           : SV_POSITION;
    float3 NormalWS             : NORMAL;
    float3 PositionWS           : TEXCOORD0;
    float2 UVMapping            : TEXCOORD1;
};

// =========================================================================================
// PRODUCTION GRAPHICS RUNTIME EXECUTION ENTRY POINT
// =========================================================================================
float4 PS_FicusMasterShadingPipeline(PixelInputCache input) : SV_Target
{
    // Recover custom structured attributes encoded by the generator logic
    float3 N = normalize(input.NormalWS);
    
    // Extract hidden channel variables
    float latexBleedFactorValue = saturate(input.NormalWS.z); 
    N.z = sqrt(max(0.0f, 1.0f - N.x * N.x - N.y * N.y)); // Reconstruct vector alignments

    float3 L = normalize(g_SunDirectionWS);
    float3 V = normalize(float3(0.0f, 20.0f, -15.0f) - input.PositionWS);
    float3 H = normalize(L + V);

    float ndotl = saturate(dot(N, L));
    float ndoth = saturate(dot(N, H));

    float3 finalDiffuseAccumulator = (float3)0.0f;
    float3 finalSpecularAccumulator = (float3)0.0f;
    float  alphaPassValue = 1.0f;

    // DETECT STRUCTURAL OVERRIDE VIA HEIGHT TEXTURE MATRIX BOUNDS
    bool isLeafNodeGeometry = (input.UVMapping.y > 9.5f); // Terminal threshold flag check

    // -------------------------------------------------------------------------------------
    // PIPELINE NODE A: RENDERING LEAF CARDS FROM THE HIGH RAINFOREST UMBRELLA
    // -------------------------------------------------------------------------------------
    if (isLeafNodeGeometry)
    {
        // Simple harmonic flutter for high-altitude wind response simulation
        float leafFlutter = sin(input.PositionWS.x * 4.0f + g_CanopyLeafRustleTimer * 5.0f);
        float3 tweakedLeafNormal = normalize(N + float3(leafFlutter * 0.1f, 0.0f, 0.0f));

        finalDiffuseAccumulator = g_GlossyFicusLeafAlbedo * (max(0.0f, dot(tweakedLeafNormal, L)) + 0.15f);
        finalSpecularAccumulator = (float3)pow(saturate(dot(tweakedLeafNormal, H)), 128.0f) * 0.25f; // Glossy leather-like cuticle coating
        
        // Handle seasonal leaf-drop cycle execution
        alphaPassValue = saturate(1.0f - g_DeciduousSheddingState);
    }
    // -------------------------------------------------------------------------------------
    // PIPELINE NODE B: RENDERING MASSIVE WHITE COLUMNAR BARK & BUTTRESS FLANGES
    // -------------------------------------------------------------------------------------
    else
    {
        // 1. DYNAMIC BARK TEXTURE AND LATEX SAP MIXTURE
        // Check if this pixel lies on a bark fracture that leaks latex
        float3 surfaceColor = lerp(g_AlbedoWhiteColumnarBark, g_MilkyLatexSapColor, latexBleedFactorValue);
        
        // Sap tracks are extremely smooth and wet, while white bark is rough and powdery
        float activeRoughness = lerp(g_BarkBaseRoughness, 0.02f, latexBleedFactorValue);
        float specularPowerExp = lerp(16.0f, 512.0f, latexBleedFactorValue); // Wet mirror finish for liquid sap

        // 2. APPLY ILLUMINATION
        finalDiffuseAccumulator = surfaceColor * (ndotl + 0.1f);
        
        float microFacetHighlight = pow(ndoth, specularPowerExp) * lerp(0.04f, g_LatexGlossViscosity, latexBleedFactorValue);
        finalSpecularAccumulator = float3(1.0f, 1.0f, 1.0f) * microFacetHighlight;
    }

    float3 compositedOutputColor = finalDiffuseAccumulator + finalSpecularAccumulator;
    return float4(max(0.0f, compositedOutputColor), alphaPassValue);
}
