// ============================================================================
// AAA WALKING PALM TREE (*Socratea exorrhiza*) HIGH-GRADE HLSL SHADER
// Pipeline Target: Shader Model 5.0 / 6.0 (HLSL / Direct3D / Vulkan / Unity / UE)
// Features: Dynamic Subsurface Scattering, Wind Trunk Bending & Frond Flutter,
//           Procedural Root Decay/Tension, Wetness & Moss Layering.
// ============================================================================

// ----------------------------------------------------------------------------
// CONSTANT BUFFERS & UNIFORMS
// ----------------------------------------------------------------------------

cbuffer PerFrameBuffer : register(b0)
{
    float4x4 g_ViewProjectionMatrix;
    float3   g_CameraWorldPosition;
    float    g_EngineTime;
    
    float3   g_SunDirection;
    float    g_SunIntensity;
    float3   g_SunColor;
    float    g_Pad0;
};

cbuffer PerDrawBuffer : register(b1)
{
    float4x4 g_WorldMatrix;
    float4x4 g_WorldITMatrix; // Inverse Transpose World Matrix
};

cbuffer WindAndMaterialParameters : register(b2)
{
    // Wind Parameters
    float3   g_WindDirection;          // Normalized world space direction
    float    g_WindStrength;           // 0.0 (calm) to 1.0+ (storm)
    float    g_WindFrequency;          // Speed of gusting
    float    g_TrunkFlexibility;       // Trunk bending resistance multiplier
    float    g_FrondFlutterSpeed;      // Leaf micro-vibration speed
    float    g_Pad1;

    // Biological / Aging Parameters
    float    g_RootDecayFactor;        // 0.0 (Healthy) to 1.0 (Rotted/Decayed)
    float    g_MossAccumulation;       // 0.0 (Clean) to 1.0 (Heavy rain-forest moss)
    float    g_FrondTranslucencyScale; // SSS intensity scale
    float    g_LeafGlossiness;         // Waxy coat reflectivity
};

// ----------------------------------------------------------------------------
// TEXTURES AND SAMPLERS
// ----------------------------------------------------------------------------

Texture2D g_AlbedoArray          : register(t0); // Tex0: Trunk/Root, Tex1: Frond
Texture2D g_NormalMap            : register(t1); 
Texture2D g_RoughnessMetallicAO  : register(t2); // R: Roughness, G: Metallic, B: AO
Texture2D g_SubsurfaceColorMap   : register(t3); // SSS Transmission Color for leaves
Texture2D g_MossAlbedoMap        : register(t4); // High-detail rainforest moss
Texture2D g_WindNoiseMap         : register(t5); // Seamless 2D simplex noise

SamplerState g_LinearWrapSampler : register(s0);
SamplerState g_LinearClampSampler: register(s1);

// ----------------------------------------------------------------------------
// PIPELINE STRUCTURES
// ----------------------------------------------------------------------------

struct VSInput
{
    float3 Position     : POSITION;
    float3 Normal       : NORMAL;
    float4 Tangent      : TANGENT;
    float2 TexCoord     : TEXCOORD0;
    
    // Vertex Color Semantic Masking:
    // Color.r = Trunk/Root Height Weight (1.0 = Canopy/Tip, 0.0 = Base)
    // Color.g = Leaf/Frond Mask (1.0 = Foliage, 0.0 = Bark/Wood)
    // Color.b = Stilt Root Mask (1.0 = Root, 0.0 = Main Trunk)
    // Color.a = Micro-flutter frequency offset
    float4 Color        : COLOR0;
};

struct PSInput
{
    float4 PositionCS   : SV_POSITION;
    float3 WorldPos     : TEXCOORD0;
    float3 NormalWS     : TEXCOORD1;
    float3 TangentWS    : TEXCOORD2;
    float3 BitangentWS  : TEXCOORD3;
    float2 TexCoord     : TEXCOORD4;
    float4 VertexColor  : COLOR0;
};

// ----------------------------------------------------------------------------
// NOISE & ANIMATION HELPER FUNCTIONS
// ----------------------------------------------------------------------------

float SampleWindNoise(float2 uv)
{
    return g_WindNoiseMap.SampleLevel(g_LinearWrapSampler, uv, 0).r;
}

// Hierarchical procedural wind motion system
float3 CalculateWalkingPalmWindOffset(float3 worldPos, float heightWeight, float isFrond, float isRoot, float flutterPhase)
{
    float3 windDir = normalize(g_WindDirection);
    
    // 1. Primary Trunk Bending (Low-frequency macro sway)
    float2 windUV = (worldPos.xz * 0.05f) + (g_EngineTime * g_WindFrequency * 0.1f * windDir.xz);
    float macroNoise = SampleWindNoise(windUV) * 2.0f - 1.0f;
    
    // Trunk bends exponentially with height (h^2 curve)
    float trunkBendMagnitude = pow(heightWeight, 2.0f) * g_TrunkFlexibility * g_WindStrength;
    float3 trunkOffset = windDir * (macroNoise * trunkBendMagnitude);

    // 2. Secondary Stilt Root Micro-Flexing
    // Stilt roots flex subtly under wind load; load shifts through the root cone
    float rootFlex = sin(g_EngineTime * 2.0f + worldPos.y * 3.0f) * (1.0f - heightWeight) * isRoot * 0.05f * g_WindStrength;
    float3 rootOffset = float3(rootFlex, -abs(rootFlex) * 0.2f, rootFlex);

    // 3. Tertiary Frond Fluttering (High-frequency turbulence for palm leaves)
    float flutterFrequency = g_EngineTime * g_FrondFlutterSpeed + flutterPhase;
    float flutterNoise = sin(flutterFrequency) * cos(flutterFrequency * 0.73f);
    
    // High wind causes rapid fluttering along leaf blade edges
    float3 frondOffset = float3(0.0f, flutterNoise * 0.15f * g_WindStrength, 0.0f) * isFrond;

    return trunkOffset + rootOffset + frondOffset;
}

// ----------------------------------------------------------------------------
// VERTEX SHADER
// ----------------------------------------------------------------------------

PSInput VSMain(VSInput input)
{
    PSInput output;

    // Transform initial position to World Space
    float4 worldPos = mul(g_WorldMatrix, float4(input.Position, 1.0f));
    
    // Extract Masking attributes from Vertex Colors
    float heightWeight = input.Color.r;
    float isFrond      = input.Color.g;
    float isRoot       = input.Color.b;
    float flutterPhase = input.Color.a * 6.28318f;

    // Compute Procedural Wind Displacement
    float3 windDisplacement = CalculateWalkingPalmWindOffset(worldPos.xyz, heightWeight, isFrond, isRoot, flutterPhase);
    
    // Apply displacement to World Position
    worldPos.xyz += windDisplacement;

    // Output Clip Space position
    output.PositionCS = mul(g_ViewProjectionMatrix, worldPos);
    output.WorldPos   = worldPos.xyz;

    // Transform Vectors to World Space
    output.NormalWS   = normalize(mul((float3x3)g_WorldITMatrix, input.Normal));
    output.TangentWS  = normalize(mul((float3x3)g_WorldMatrix, input.Tangent.xyz));
    output.BitangentWS = cross(output.NormalWS, output.TangentWS) * input.Tangent.w;

    output.TexCoord    = input.TexCoord;
    output.VertexColor = input.Color;

    return output;
}

// ----------------------------------------------------------------------------
// LIGHTING & SUBSURFACE SCATTERING EVALUATION
// ----------------------------------------------------------------------------

// Forward Subsurface Scattering Approximation (for thin fronds/leaves)
float3 CalculateLeafTranslucency(float3 N, float3 L, float3 V, float3 transmissionColor, float thickness)
{
    // Distorted backlight vector pointing toward sun through the leaf thickness
    float3 distortedLight = L + N * 0.3f;
    float lightTranslucencyDot = pow(saturate(dot(-V, distortedLight)), 4.0f);
    
    // Attenuate backlit glow by leaf thickness
    float attenuation = exp(-thickness * 2.0f);
    return transmissionColor * lightTranslucencyDot * attenuation * g_FrondTranslucencyScale;
}

// Cook-Torrance Microfacet BRDF for physical trunk and leaf shading
float3 EvaluatePBRDirectLighting(float3 N, float3 V, float3 L, float3 albedo, float roughness, float metallic, float3 F0)
{
    float3 H = normalize(V + L);
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    if (NdotL <= 0.0f) return float3(0.0f, 0.0f, 0.0f);

    // Fresnel (Schlick approximation)
    float3 F = F0 + (1.0f - F0) * pow(1.0f - VdotH, 5.0f);

    // Normal Distribution Function (GGX / Trowbridge-Reitz)
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float denom = (NdotH * NdotH * (alphaSq - 1.0f) + 1.0f);
    float D = alphaSq / (3.14159f * denom * denom);

    // Geometry Shadowing (Schlick-GGX)
    float k = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
    float G = (NdotV / (NdotV * (1.0f - k) + k)) * (NdotL / (NdotL * (1.0f - k) + k));

    // Specular BRDF
    float3 specular = (D * F * G) / max(0.0001f, 4.0f * NdotV * NdotL);
    
    // Energy Conservation Diffuse
    float3 kD = (1.0f - F) * (1.0f - metallic);
    float3 diffuse = kD * albedo / 3.14159f;

    return (diffuse + specular) * g_SunColor * g_SunIntensity * NdotL;
}

// ----------------------------------------------------------------------------
// PIXEL / FRAGMENT SHADER
// ----------------------------------------------------------------------------

float4 PSMain(PSInput input) : SV_TARGET
{
    // Re-normalize world vectors
    float3 N = normalize(input.NormalWS);
    float3 T = normalize(input.TangentWS);
    float3 B = normalize(input.BitangentWS);
    float3 V = normalize(g_CameraWorldPosition - input.WorldPos);
    float3 L = normalize(-g_SunDirection);

    // 1. Tangent Space Normal Mapping
    float3 mapNormal = g_NormalMap.Sample(g_LinearWrapSampler, input.TexCoord).rgb * 2.0f - 1.0f;
    float3x3 TBN = float3x3(T, B, N);
    N = normalize(mul(mapNormal, TBN));

    // 2. Surface Material Samples
    float4 baseAlbedo = g_AlbedoArray.Sample(g_LinearWrapSampler, input.TexCoord);
    float3 rma        = g_RoughnessMetallicAO.Sample(g_LinearWrapSampler, input.TexCoord).rgb;
    
    float roughness   = rma.r;
    float metallic    = rma.g;
    float ambientOcclusion = rma.b;

    float isFrond = input.VertexColor.g;
    float isRoot  = input.VertexColor.b;

    // Waxy foliage reflectivity override
    roughness = lerp(roughness, roughness * (1.0f - g_LeafGlossiness * 0.5f), isFrond);

    // 3. Stilt Root Decay and Rot Procedural Blending
    if (isRoot > 0.1f)
    {
        // Procedural rotted root coloration (darkened, desaturated, rougher texture)
        float3 decayColor = baseAlbedo.rgb * float3(0.25f, 0.18f, 0.12f);
        baseAlbedo.rgb = lerp(baseAlbedo.rgb, decayColor, g_RootDecayFactor);
        roughness = lerp(roughness, 0.95f, g_RootDecayFactor); // Decayed wood loses specular gloss
    }

    // 4. Rainforest Moss Layering (Height-based slope blending for roots/lower trunk)
    float mossMask = saturate(dot(N, float3(0.0f, 1.0f, 0.0f))) * g_MossAccumulation * (1.0f - isFrond);
    // Moss concentrates primarily near ground level stilt roots
    mossMask *= saturate(2.5f - input.WorldPos.y * 0.5f); 

    if (mossMask > 0.05f)
    {
        float3 mossAlbedo = g_MossAlbedoMap.Sample(g_LinearWrapSampler, input.TexCoord * 3.0f).rgb;
        baseAlbedo.rgb = lerp(baseAlbedo.rgb, mossAlbedo, mossMask);
        roughness = lerp(roughness, 0.85f, mossMask); // Velvet moss texture
    }

    // 5. Direct PBR Lighting Calculation
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), baseAlbedo.rgb, metallic);
    float3 directLighting = EvaluatePBRDirectLighting(N, V, L, baseAlbedo.rgb, roughness, metallic, F0);

    // 6. Foliage Subsurface Scattering (Leaves only)
    float3 sssTransmission = float3(0.0f, 0.0f, 0.0f);
    if (isFrond > 0.5f)
    {
        float3 sssColor = g_SubsurfaceColorMap.Sample(g_LinearWrapSampler, input.TexCoord).rgb;
        float leafThickness = 0.2f; // Thin tropical frond
        sssTransmission = CalculateLeafTranslucency(N, L, V, sssColor, leafThickness) * g_SunColor * g_SunIntensity;
    }

    // 7. Ambient Lighting (Simplified Hemispheric Sky/Ground Ambient)
    float3 skyColor = float3(0.2f, 0.35f, 0.5f);
    float3 groundColor = float3(0.1f, 0.08f, 0.05f);
    float skyUpHemisphere = saturate(dot(N, float3(0.0f, 1.0f, 0.0f)) * 0.5f + 0.5f);
    float3 ambientLighting = lerp(groundColor, skyColor, skyUpHemisphere) * baseAlbedo.rgb * ambientOcclusion * 0.3f;

    // Composite Final Radiance Output
    float3 finalColor = directLighting + sssTransmission + ambientLighting;

    return float4(finalColor, baseAlbedo.a);
}
