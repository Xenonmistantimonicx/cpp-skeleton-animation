// ============================================================================
// AAA WEEPING BEECH (*Fagus sylvatica 'Pendula'*) HIGH-GRADE HLSL SHADER
// Pipeline Target: Shader Model 5.0 / 6.0 (HLSL / Direct3D 11/12 / Vulkan / UE / Unity)
// Features: Cascading Pendulous Wind Physics, Double-Sided Thin-Foliage SSS,
//           Smooth Beech Elephant Bark BRDF, Procedural Phenological Blending.
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

cbuffer WeepingBeechParameters : register(b2)
{
    // Aerodynamic Wind Controls
    float3   g_WindDirection;          // Normalized direction vector
    float    g_WindStrength;           // Wind speed scale
    float    g_BoughRigidity;          // Major structural limb resistance
    float    g_CurtainPendulumSpeed;   // Pendulous branch sway rate
    float    g_LeafTurbulenceScale;    // High-frequency flutter magnitude
    float    g_Pad1;

    // Phenology & Material Parameters
    float3   g_SeasonalLeafTint;       // Spring (Lime), Summer (Forest Green), Autumn (Copper)
    float    g_PhenologyLeafDensity;   // 0.0 (Winter Bare) to 1.0 (Summer Full Canopy)
    float    g_SubsurfaceIntensity;    // Leaf backlight transmission strength
    float    g_BarkSmoothness;         // Glossiness of smooth beech bark
};

// ----------------------------------------------------------------------------
// TEXTURES & SAMPLERS
// ----------------------------------------------------------------------------

Texture2D g_BarkAlbedoMap         : register(t0); // Smooth grey beech bark
Texture2D g_LeafAlbedoMap         : register(t1); // Beech leaf cutout & color
Texture2D g_NormalMap            : register(t2); // Combined Normal Map
Texture2D g_RoughnessMetallicAO  : register(t3); // R: Roughness, G: Metallic, B: Ambient Occlusion
Texture2D g_LeafSubsurfaceMap    : register(t4); // SSS Transmission mask
Texture2D g_WindTurbulenceNoise   : register(t5); // 2D Simplex noise

SamplerState g_LinearWrapSampler  : register(s0);
SamplerState g_LinearClampSampler : register(s1);

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
    // Color.r = Major Branch Weight (1.0 = Rigid Bough, 0.0 = Outer Tips)
    // Color.g = Weeping Curtain Mask (1.0 = Pendulous Drooping Branch, 0.0 = Main Trunk)
    // Color.b = Leaf vs Bark Mask (1.0 = Leaf Mesh, 0.0 = Wood/Bark Mesh)
    // Color.a = Vertical Distance along Drooping Chain (0.0 = Top Anchor, 1.0 = Ground Tip)
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
// PROCEDURAL PENDULOUS WIND PHYSICS ANIMATION
// ----------------------------------------------------------------------------

float3 CalculateWeepingBeechWindDisplacement(float3 worldPos, float boughWeight, float isCurtain, float isLeaf, float chainProgress)
{
    float3 windDir = normalize(g_WindDirection);

    // 1. Structural Bough Oscillation (Low-frequency macro sway)
    float2 boughNoiseUV = (worldPos.xz * 0.02f) + (g_EngineTime * 0.2f * windDir.xz);
    float macroSway = g_WindTurbulenceNoise.SampleLevel(g_LinearWrapSampler, boughNoiseUV, 0).r * 2.0f - 1.0f;
    float3 boughOffset = windDir * (macroSway * (1.0f - boughWeight) * g_WindStrength * 0.4f);

    // 2. Pendulous Curtain Sway (Inverted Pendulum Physics)
    // Vertical-drooping branches sway perpendicular to wind with phase lag along chain length
    float pendulumPhase = g_EngineTime * g_CurtainPendulumSpeed + (chainProgress * 3.14159f);
    float pendulumSway = sin(pendulumPhase) * pow(chainProgress, 1.5f) * isCurtain;
    
    // Cross vector creates horizontal pendulum motion perpendicular to wind direction
    float3 perpendicularWind = normalize(cross(windDir, float3(0.0f, 1.0f, 0.0f)));
    float3 curtainOffset = (windDir * pendulumSway * 0.6f + perpendicularWind * cos(pendulumPhase) * 0.4f) * g_WindStrength;

    // 3. Leaf Micro-Turbulence (High-frequency flutter for beech leaves)
    float flutterPhase = g_EngineTime * 12.0f + (worldPos.x + worldPos.y + worldPos.z);
    float leafFlutter = sin(flutterPhase) * cos(flutterPhase * 0.65f) * isLeaf * g_LeafTurbulenceScale * g_WindStrength;
    float3 leafOffset = float3(0.0f, leafFlutter, leafFlutter * 0.5f);

    return boughOffset + curtainOffset + leafOffset;
}

// ----------------------------------------------------------------------------
// VERTEX SHADER
// ----------------------------------------------------------------------------

PSInput VSMain(VSInput input)
{
    PSInput output;

    // Convert to World Space
    float4 worldPos = mul(g_WorldMatrix, float4(input.Position, 1.0f));

    // Extract Vertex Masking Data
    float boughWeight   = input.Color.r;
    float isCurtain     = input.Color.g;
    float isLeaf        = input.Color.b;
    float chainProgress = input.Color.a;

    // Evaluate Wind Physics
    float3 windOffset = CalculateWeepingBeechWindDisplacement(worldPos.xyz, boughWeight, isCurtain, isLeaf, chainProgress);
    
    // Apply Displacement
    worldPos.xyz += windOffset;

    // Transform to Projection Clip Space
    output.PositionCS  = mul(g_ViewProjectionMatrix, worldPos);
    output.WorldPos    = worldPos.xyz;

    // Transform Tangent Space Basis
    output.NormalWS    = normalize(mul((float3x3)g_WorldITMatrix, input.Normal));
    output.TangentWS   = normalize(mul((float3x3)g_WorldMatrix, input.Tangent.xyz));
    output.BitangentWS = cross(output.NormalWS, output.TangentWS) * input.Tangent.w;

    output.TexCoord    = input.TexCoord;
    output.VertexColor = input.Color;

    return output;
}

// ----------------------------------------------------------------------------
// LIGHTING & SUBSURFACE SCATTERING EVALUATION
// ----------------------------------------------------------------------------

// Subsurface Scattering for thin translucent beech foliage backlit by the sun
float3 CalculateBeechLeafSSS(float3 N, float3 L, float3 V, float3 transmissionColor)
{
    float3 distortedLight = L + N * 0.25f;
    float backlitDot = pow(saturate(dot(-V, distortedLight)), 3.5f);
    return transmissionColor * backlitDot * g_SubsurfaceIntensity;
}

// Cook-Torrance Microfacet BRDF for Smooth Elephant-Skin Beech Bark & Waxy Foliage
float3 EvaluatePBRDirectLighting(float3 N, float3 V, float3 L, float3 albedo, float roughness, float metallic, float3 F0)
{
    float3 H = normalize(V + L);
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    if (NdotL <= 0.0f) return float3(0.0f, 0.0f, 0.0f);

    // Fresnel Schlick
    float3 F = F0 + (1.0f - F0) * pow(1.0f - VdotH, 5.0f);

    // GGX Normal Distribution
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float denom = (NdotH * NdotH * (alphaSq - 1.0f) + 1.0f);
    float D = alphaSq / (3.14159f * denom * denom);

    // Smith Geometric Shadowing
    float k = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
    float G = (NdotV / (NdotV * (1.0f - k) + k)) * (NdotL / (NdotL * (1.0f - k) + k));

    // Specular Highlight
    float3 specular = (D * F * G) / max(0.0001f, 4.0f * NdotV * NdotL);

    // Diffuse Component
    float3 kD = (1.0f - F) * (1.0f - metallic);
    float3 diffuse = kD * albedo / 3.14159f;

    return (diffuse + specular) * g_SunColor * g_SunIntensity * NdotL;
}

// ----------------------------------------------------------------------------
// PIXEL / FRAGMENT SHADER
// ----------------------------------------------------------------------------

float4 PSMain(PSInput input) : SV_TARGET
{
    // Normalize Basis Vectors
    float3 N = normalize(input.NormalWS);
    float3 T = normalize(input.TangentWS);
    float3 B = normalize(input.BitangentWS);
    float3 V = normalize(g_CameraWorldPosition - input.WorldPos);
    float3 L = normalize(-g_SunDirection);

    // 1. Normal Mapping
    float3 mapNormal = g_NormalMap.Sample(g_LinearWrapSampler, input.TexCoord).rgb * 2.0f - 1.0f;
    float3x3 TBN = float3x3(T, B, N);
    N = normalize(mul(mapNormal, TBN));

    // 2. Double-Sided Normal Adjustment for Leaves
    float isLeaf = input.VertexColor.b;
    if (isLeaf > 0.5f && !IsFrontFace(input.NormalWS, V))
    {
        N = -N; // Flip normal for back-face foliage rendering
    }

    // 3. Texture Sampling & Material Blending
    float4 barkAlbedo = g_BarkAlbedoMap.Sample(g_LinearWrapSampler, input.TexCoord);
    float4 leafAlbedo = g_LeafAlbedoMap.Sample(g_LinearWrapSampler, input.TexCoord);

    // Alpha Cutout for Leaf Foliage Card Meshes
    if (isLeaf > 0.5f)
    {
        clip(leafAlbedo.a * g_PhenologyLeafDensity - 0.33f);
    }

    // Apply Seasonal Phenology Tint to Leaves
    leafAlbedo.rgb *= g_SeasonalLeafTint;

    // Blend Albedo between Bark and Leaf based on vertex mask
    float3 finalAlbedo = lerp(barkAlbedo.rgb, leafAlbedo.rgb, isLeaf);

    // Roughness / Metallic / AO Sampling
    float3 rma = g_RoughnessMetallicAO.Sample(g_LinearWrapSampler, input.TexCoord).rgb;
    float roughness = rma.r;
    float metallic  = rma.g;
    float ao        = rma.b;

    // Smooth Beech Elephant-Skin Bark Specular Tuning
    roughness = lerp(roughness * (1.0f - g_BarkSmoothness * 0.4f), roughness, isLeaf);

    // 4. Direct PBR Lighting
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), finalAlbedo, metallic);
    float3 directLighting = EvaluatePBRDirectLighting(N, V, L, finalAlbedo, roughness, metallic, F0);

    // 5. Subsurface Scattering (Leaves only)
    float3 sssTransmission = float3(0.0f, 0.0f, 0.0f);
    if (isLeaf > 0.5f)
    {
        float3 sssMap = g_LeafSubsurfaceMap.Sample(g_LinearWrapSampler, input.TexCoord).rgb * g_SeasonalLeafTint;
        sssTransmission = CalculateBeechLeafSSS(N, L, V, sssMap) * g_SunColor * g_SunIntensity;
    }

    // 6. Ambient Lighting Approximation
    float3 skyColor = float3(0.18f, 0.28f, 0.42f);
    float3 groundColor = float3(0.08f, 0.06f, 0.04f);
    float skyHemisphere = saturate(dot(N, float3(0.0f, 1.0f, 0.0f)) * 0.5f + 0.5f);
    float3 ambientLighting = lerp(groundColor, skyColor, skyHemisphere) * finalAlbedo * ao * 0.25f;

    // Composite Final Radiance Output
    float3 finalRadiance = directLighting + sssTransmission + ambientLighting;

    return float4(finalRadiance, 1.0f);
}

// ----------------------------------------------------------------------------
// HELPER UTILITY
// ----------------------------------------------------------------------------

bool IsFrontFace(float3 worldNormal, float3 viewDir)
{
    return dot(worldNormal, viewDir) > 0.0f;
}
