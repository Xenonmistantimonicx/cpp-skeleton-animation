// ============================================================================
// AAA WEEPING BIRCH (*Betula pendula 'Tristis'*) HIGH-GRADE HLSL SHADER
// Pipeline Target: Shader Model 5.0 / 6.0 (Direct3D 11/12 / Vulkan / UE / Unity)
// Features: White Chalky Bark vs Base Fissuring, Petiole Twisting Wind Physics,
//           Catkin & Leaf Subsurface Scattering, Seasonal Phenology Blending.
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

cbuffer WeepingBirchParameters : register(b2)
{
    // Aerodynamic Wind Controls
    float3   g_WindDirection;          // Normalized direction vector
    float    g_WindStrength;           // Global wind strength
    float    g_PetioleTwistSpeed;      // High-frequency leaf flutter speed
    float    g_BranchletSwayFrequency; // Drooping branch sway rate
    float    g_Pad1;

    // Phenology & Material Parameters
    float3   g_SeasonalLeafTint;       // Spring (Lime), Summer (Green), Autumn (Gold)
    float    g_LeafDensityCutoff;      // Clip threshold for seasonal leaf drop
    float    g_SubsurfaceIntensity;    // Transmission scale for thin leaves & catkins
    float    g_BarkWhitenessScale;     // Betulin white bark brightness
    float    g_BaseFissureHeight;      // Height cutoff for dark diamond bark fissures
};

// ----------------------------------------------------------------------------
// TEXTURES & SAMPLERS
// ----------------------------------------------------------------------------

Texture2D g_WhiteBarkAlbedoMap     : register(t0); // Smooth white birch bark
Texture2D g_DarkFissureAlbedoMap   : register(t1); // Dark fractured base bark
Texture2D g_LeafAlbedoMap          : register(t2); // Triangular leaf cutout & color
Texture2D g_CatkinAlbedoMap        : register(t3); // Pendulous male/female catkins
Texture2D g_NormalMap              : register(t4); // Surface normal map
Texture2D g_RoughnessMetallicAO    : register(t5); // R: Roughness, G: Metallic, B: AO
Texture2D g_SubsurfaceMap          : register(t6); // SSS transmission mask
Texture2D g_WindTurbulenceNoise     : register(t7); // Simplex noise for flutter

SamplerState g_LinearWrapSampler   : register(s0);
SamplerState g_LinearClampSampler  : register(s1);

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
    // Color.r = Branch Mass Weight (1.0 = Rigid Trunk, 0.0 = Whip Tip)
    // Color.g = Geometry Mask (0.0 = Bark, 0.5 = Leaf, 1.0 = Hanging Catkin)
    // Color.b = Petiole Twist Mask (1.0 = Outer Leaf Edge, 0.0 = Stem Attachment)
    // Color.a = Height Gradient (0.0 = Root Base, 1.0 = Top Canopy)
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
// PROCEDURAL PETIOLE TWIST & WEEPING WIND PHYSICS
// ----------------------------------------------------------------------------

float3 CalculateBirchWindDisplacement(float3 worldPos, float branchWeight, float geoMask, float twistMask, float heightGrad)
{
    float3 windDir = normalize(g_WindDirection);

    // 1. Primary Bough Sway (Low Frequency)
    float2 boughUV = (worldPos.xz * 0.03f) + (g_EngineTime * 0.15f * windDir.xz);
    float macroNoise = g_WindTurbulenceNoise.SampleLevel(g_LinearWrapSampler, boughUV, 0).r * 2.0f - 1.0f;
    float3 boughOffset = windDir * (macroNoise * (1.0f - branchWeight) * g_WindStrength * 0.5f);

    // 2. Pendulous Drooping Branchlet Sway
    float swayPhase = g_EngineTime * g_BranchletSwayFrequency + (worldPos.y * 2.0f);
    float3 perpendicularWind = normalize(cross(windDir, float3(0.0f, 1.0f, 0.0f)));
    float3 branchletOffset = (windDir * sin(swayPhase) * 0.4f + perpendicularWind * cos(swayPhase) * 0.3f) * (1.0f - branchWeight) * g_WindStrength;

    // 3. Leaf Petiole Micro-Twisting (High Frequency "Fluttering" Motion)
    float isLeaf = step(0.25f, geoMask) * step(geoMask, 0.75f);
    float twistPhase = g_EngineTime * g_PetioleTwistSpeed + dot(worldPos, float3(11.0f, 17.0f, 13.0f));
    float petioleAngle = sin(twistPhase) * 0.35f * twistMask * isLeaf * g_WindStrength;
    
    // Twist leaves around their local stem axis
    float3 leafTwistOffset = perpendicularWind * petioleAngle;

    return boughOffset + branchletOffset + leafTwistOffset;
}

// ----------------------------------------------------------------------------
// VERTEX SHADER
// ----------------------------------------------------------------------------

PSInput VSMain(VSInput input)
{
    PSInput output;

    // Transform to World Space
    float4 worldPos = mul(g_WorldMatrix, float4(input.Position, 1.0f));

    // Unpack Vertex Color Semantic Masks
    float branchWeight = input.Color.r;
    float geoMask      = input.Color.g;
    float twistMask    = input.Color.b;
    float heightGrad   = input.Color.a;

    // Calculate Wind Displacement
    float3 windDisplacement = CalculateBirchWindDisplacement(worldPos.xyz, branchWeight, geoMask, twistMask, heightGrad);
    worldPos.xyz += windDisplacement;

    // Project to Clip Space
    output.PositionCS  = mul(g_ViewProjectionMatrix, worldPos);
    output.WorldPos    = worldPos.xyz;

    // Transform Normal, Tangent, Bitangent to World Space
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

float3 CalculateBirchSSS(float3 N, float3 L, float3 V, float3 transmissionColor)
{
    float3 distortedLight = L + N * 0.3f;
    float backlitDot = pow(saturate(dot(-V, distortedLight)), 3.0f);
    return transmissionColor * backlitDot * g_SubsurfaceIntensity;
}

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

    // GGX Distribution
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float denom = (NdotH * NdotH * (alphaSq - 1.0f) + 1.0f);
    float D = alphaSq / (3.14159f * denom * denom);

    // Smith Geometric Shadowing
    float k = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
    float G = (NdotV / (NdotV * (1.0f - k) + k)) * (NdotL / (NdotL * (1.0f - k) + k));

    // Specular Highlight
    float3 specular = (D * F * G) / max(0.0001f, 4.0f * NdotV * NdotL);

    // Diffuse Energy Conservation
    float3 kD = (1.0f - F) * (1.0f - metallic);
    float3 diffuse = kD * albedo / 3.14159f;

    return (diffuse + specular) * g_SunColor * g_SunIntensity * NdotL;
}

// ----------------------------------------------------------------------------
// PIXEL / FRAGMENT SHADER
// ----------------------------------------------------------------------------

float4 PSMain(PSInput input) : SV_TARGET
{
    // Re-normalize Basis Vectors
    float3 N = normalize(input.NormalWS);
    float3 T = normalize(input.TangentWS);
    float3 B = normalize(input.BitangentWS);
    float3 V = normalize(g_CameraWorldPosition - input.WorldPos);
    float3 L = normalize(-g_SunDirection);

    // Tangent Space Normal Mapping
    float3 mapNormal = g_NormalMap.Sample(g_LinearWrapSampler, input.TexCoord).rgb * 2.0f - 1.0f;
    float3x3 TBN = float3x3(T, B, N);
    N = normalize(mul(mapNormal, TBN));

    // Unpack Geometry Type Mask: 0.0 = Bark, 0.5 = Leaf, 1.0 = Catkin
    float geoMask    = input.VertexColor.g;
    float heightGrad = input.VertexColor.a;

    float isBark   = step(geoMask, 0.25f);
    float isLeaf   = step(0.25f, geoMask) * step(geoMask, 0.75f);
    float isCatkin = step(0.75f, geoMask);

    // Double-Sided Normal Adjustment for Thin Leaf Cards
    if (isLeaf > 0.5f && dot(input.NormalWS, V) < 0.0f)
    {
        N = -N;
    }

    // 1. Surface Albedo Evaluation
    float3 finalAlbedo = float3(0.0f, 0.0f, 0.0f);
    float alphaCutout = 1.0f;

    if (isBark > 0.5f)
    {
        // Blend White Chalky Bark (Top) with Dark Fractured Fissure Bark (Base)
        float3 whiteBark = g_WhiteBarkAlbedoMap.Sample(g_LinearWrapSampler, input.TexCoord).rgb * g_BarkWhitenessScale;
        float3 darkFissure = g_DarkFissureAlbedoMap.Sample(g_LinearWrapSampler, input.TexCoord).rgb;
        
        float fissureBlend = saturate(1.0f - (input.WorldPos.y / g_BaseFissureHeight));
        fissureBlend = pow(fissureBlend, 2.0f); // Sharp transition near trunk base
        
        finalAlbedo = lerp(whiteBark, darkFissure, fissureBlend);
    }
    else if (isLeaf > 0.5f)
    {
        float4 leafSample = g_LeafAlbedoMap.Sample(g_LinearWrapSampler, input.TexCoord);
        alphaCutout = leafSample.a;
        
        // Seasonal Leaf Cutout & Tinting
        clip(alphaCutout - g_LeafDensityCutoff);
        finalAlbedo = leafSample.rgb * g_SeasonalLeafTint;
    }
    else // Catkins
    {
        float4 catkinSample = g_CatkinAlbedoMap.Sample(g_LinearWrapSampler, input.TexCoord);
        alphaCutout = catkinSample.a;
        clip(alphaCutout - 0.33f);
        
        finalAlbedo = catkinSample.rgb;
    }

    // 2. Material Parameter Sampling
    float3 rma = g_RoughnessMetallicAO.Sample(g_LinearWrapSampler, input.TexCoord).rgb;
    float roughness = rma.r;
    float metallic  = rma.g;
    float ao        = rma.b;

    // Adjust roughness for chalky betulin bark vs glossy leaves
    roughness = lerp(roughness, 0.92f, isBark); // Betulin bark has a dry matte finish

    // 3. Direct PBR Lighting
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), finalAlbedo, metallic);
    float3 directLighting = EvaluatePBRDirectLighting(N, V, L, finalAlbedo, roughness, metallic, F0);

    // 4. Subsurface Scattering (Leaves & Catkins)
    float3 sssTransmission = float3(0.0f, 0.0f, 0.0f);
    if (isLeaf > 0.5f || isCatkin > 0.5f)
    {
        float3 sssColor = g_SubsurfaceMap.Sample(g_LinearWrapSampler, input.TexCoord).rgb;
        sssTransmission = CalculateBirchSSS(N, L, V, sssColor * finalAlbedo) * g_SunColor * g_SunIntensity;
    }

    // 5. Ambient Sky/Ground Lighting
    float3 skyColor = float3(0.2f, 0.32f, 0.48f);
    float3 groundColor = float3(0.09f, 0.07f, 0.05f);
    float skyHemisphere = saturate(dot(N, float3(0.0f, 1.0f, 0.0f)) * 0.5f + 0.5f);
    float3 ambientLighting = lerp(groundColor, skyColor, skyHemisphere) * finalAlbedo * ao * 0.3f;

    // Composite Final Radiance Output
    float3 finalRadiance = directLighting + sssTransmission + ambientLighting;

    return float4(finalRadiance, 1.0f);
}
