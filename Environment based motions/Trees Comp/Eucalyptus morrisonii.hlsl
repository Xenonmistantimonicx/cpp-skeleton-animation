// =========================================================================================
// PRODUCTION-GRADE RE-ENGINERED GRAPHICS PIPELINE INPUT REGISTER MATRIX
// =========================================================================================
cbuffer PerMaterialConstantBuffer : register(b1)
{
    // Directional Lighting Config
    float3 g_WorldSunLightDir           : packoffset(c0.x); 
    float  g_GlobalEnvironmentAmbient   : packoffset(c0.w);

    // Color Profiling Channels
    float3 g_FibrousBaseBarkColor       : packoffset(c1.x); // Deep rugged grey-brown base albedo
    float  g_BarkOrenNayarRoughness     : packoffset(c1.w); // Micro-roughness factor for physical wood shading
    
    float3 g_SmoothCanopyBarkColor      : packoffset(c2.x); // Light cream/yellowish branches color
    float  g_LeafWaxyGGXRoughness       : packoffset(c2.w); // Roughness value for the protective waxy layer (0.1 - 0.45)

    float3 g_LanceolateLeafAlbedo       : packoffset(c3.x); // Waxy grey-green foliage tint
    float  g_SubsurfaceTransmissionScale : packoffset(c3.w); // Foliage internal light scattering power

    float3 g_SpecularLobeColorTint      : packoffset(c4.x); // Pure clean highlight sheen reflection color
    float  g_IsLeafMeshOverrideFlag     : packoffset(c4.w); // Hard static toggle: 0.0 = Bark Geometry, 1.0 = Foliage Card
};

struct PixelInputVertexCache
{
    float4 SVPositionCS   : SV_POSITION;  // Clip-space hardware mapping coordinates
    float3 WorldNormal    : NORMAL;       // Transformed world-space interpolation vector
    float3 WorldPosition  : TEXCOORD0;    // Absolute world space vector for view dependency
    float2 UVCoordinates  : TEXCOORD1;    // Mapping track channel data
    float  BarkTypeFactor : BLENDWEIGHT;  // Procedural gradient tracker (0.0=base trunk, 1.0=canopy twigs)
};

// =========================================================================================
// CORE ANALYTICAL MATHEMATICS ENGINE SUBROUTINES
// =========================================================================================

// GGX Microfacet Distribution Function (D) for realistic waxy foliage highlights
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = 3.14159265359f * denom * denom;

    return nom / max(denom, 0.000001f);
}

// Geometric Attenuation Function (G) - Smith's method for microfacet shadowing
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float nom   = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / max(denom, 0.000001f);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Fresnel-Schlick Approximation (F) for specular interface transitions
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

// Advanced Oren-Nayar Shading Model for micro-porous rough organic bark structures
float3 ComputeOrenNayarDiffuse(float3 N, float3 L, float3 V, float3 albedo, float roughness)
{
    float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    
    float angleVN = acos(saturate(NdotV));
    float angleLN = acos(saturate(NdotL));
    
    float alpha = max(angleVN, angleLN);
    float beta  = min(angleVN, angleLN);
    
    float sigma2 = roughness * roughness;
    float A = 1.0f - 0.5f * (sigma2 / (sigma2 + 0.33f));
    float B = 0.45f * (sigma2 / (sigma2 + 0.09f));
    
    // Calculate azimuthal projection alignment component
    float3 gammaL = normalize(L - N * NdotL);
    float3 gammaV = normalize(V - N * NdotV);
    float cosPhiDiff = max(0.0f, dot(gammaL, gammaV));
    
    float3 diffuseIntensity = albedo * (max(0.0f, NdotL) * (A + B * cosPhiDiff * sin(alpha) * tan(beta)));
    return diffuseIntensity;
}

// =========================================================================================
// MAIN PHOTOMETRIC PIXEL SHADER COMPILER PASS
// =========================================================================================
float4 PS_EucalyptusAdvancedMaster(PixelInputVertexCache input) : SV_Target
{
    // Standard normal mechanics resolutions
    float3 N = normalize(input.WorldNormal);
    float3 L = normalize(g_WorldSunLightDir);
    float3 V = normalize(float3(0.0f, 12.0f, -10.0f) - input.PositionWorldSpace); // Dynamic view projection array
    float3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);

    // Initial Material Setup Branching
    float3 baseAlbedo = (float3)0.0f;
    float3 evaluatedLightingRGB = (float3)0.0f;
    float alphaChannelOutput = 1.0f;

    // Conductor vs Dielectric base initialization factor for foliage cuticle waxy layer
    float3 surfaceF0 = float3(0.04f, 0.04f, 0.04f); 

    // -------------------------------------------------------------------------------------
    // PIPELINE BRANCH A: GEOMETRY IS AN INJECTED LANCEOLATE LEAF SYSTEM
    // -------------------------------------------------------------------------------------
    if (g_IsLeafMeshOverrideFlag > 0.5f)
    {
        baseAlbedo = g_LanceolateLeafAlbedo;

        // 1. High-Precision Cuticle Specular Reflection via GGX
        float  D = DistributionGGX(N, H, g_LeafWaxyGGXRoughness);
        float  G = GeometrySmith(N, V, L, g_LeafWaxyGGXRoughness);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0f), surfaceF0);

        float3 nominator   = D * G * F;
        float  denominator = 4.0f * NdotV * NdotL;
        float3 waxySpecularComponent = nominator / max(denominator, 0.001f); // Safe ceiling optimization

        // 2. Translucent Light Wrapping Subsurface Simulation
        // For the long hanging Grey Mallee leaves passing rays directly down through thin membranes
        float backscatterRayAlignment = saturate(dot(V, -L));
        float wrapProfile = pow(backscatterRayAlignment, 6.0f) * g_SubsurfaceTransmissionScale;
        float3 leafTranslucency = g_LanceolateLeafAlbedo * wrapProfile * float3(0.85f, 0.95f, 0.4f);

        // Standard diffuse calculation for organic foliage tissue
        float3 leafDiffuse = baseAlbedo * (NdotL + g_GlobalEnvironmentAmbient);

        // Final composition balance for foliage node architecture
        evaluatedLightingRGB = leafDiffuse + waxySpecularComponent + leafTranslucency;
        
        // Soft alpha gradient blend near outer bounds to eliminate pixel popping on leaf mesh cards
        alphaChannelOutput = saturate(1.2f - (abs(input.UVCoordinates.x - 0.5f) * 2.4f));
    }
    // -------------------------------------------------------------------------------------
    // PIPELINE BRANCH B: GEOMETRY IS THE DUAL-LAYER MALLEE BARK / WOOD NETWORK
    // -------------------------------------------------------------------------------------
    else
    {
        // Continuous structural blending from deep rough lower bark to smooth cream crown twigs
        baseAlbedo = lerp(g_FibrousBaseBarkColor, g_SmoothCanopyBarkColor, saturate(input.BarkTypeFactor));

        // 1. Apply Advanced Oren-Nayar Shading on Trunk
        // Traditional Lambertian calculations look flat on deep fibrous ridges; this preserves natural crevices shadow density
        float activeBarkRoughness = lerp(g_BarkOrenNayarRoughness, 0.1f, saturate(input.BarkTypeFactor));
        float3 orenNayarDiffuseReflection = ComputeOrenNayarDiffuse(N, L, V, baseAlbedo, activeBarkRoughness);

        // 2. Micro-Glint Specular for Smooth Upper Branch Coats
        // Upper zones are smoother and display slight waxy sheen before transition passes terminate
        float smoothSpecularHighlight = pow(saturate(dot(N, H)), 32.0f) * saturate(input.BarkTypeFactor) * 0.15f;

        // Final composition accumulation for trunk/branch geometry node
        evaluatedLightingRGB = orenNayarDiffuseReflection + (g_SpecularLobeColorTint * smoothSpecularHighlight) + (baseAlbedo * g_GlobalEnvironmentAmbient);
    }

    // Return unified high fidelity fragment payload buffer
    return float4(max(0.0f, evaluatedLightingRGB), alphaChannelOutput);
}
