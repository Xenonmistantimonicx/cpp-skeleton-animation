// --- RENDER REGISTERS & UNIFORMS ---
cbuffer PerFrameLightingBuffer : register(b1)
{
    float3 LightDirection       : packoffset(c0.x); // World Space normalized vector of main sun line
    float  AmbientIntensity     : packoffset(c0.w); 
    float3 PrimaryScarletAlbedo : packoffset(c1.x); // Base deep crimson texture tint rgb
    float  SubsurfaceThickness  : packoffset(c1.w); // Material thickness control override
    float3 BacklightGlowColor   : packoffset(c2.x); // Translucent burning orange hue rgb
};

struct VS_OUTPUT
{
    float4 Position   : SV_POSITION;
    float3 WorldNormal: NORMAL;
    float3 WorldPos   : TEXCOORD0;
    float2 TexCoord   : TEXCOORD1;
};

// --- CORE TRANSLUCENCY SHADING GRAPH ---
float4 PS_ErythrinaSubsurfaceMain(VS_OUTPUT input) : SV_Target
{
    // Normalize geometric space maps
    float3 N = normalize(input.WorldNormal);
    float3 L = normalize(LightDirection);
    float3 V = normalize(float3(0.0f, 10.0f, 5.0f) - input.WorldPos); // Eye reference view line

    // 1. Primary Direct Diffuse Lighting Pass (Standard Lambertian model)
    float diffuseIntensity = saturate(dot(N, L));
    float3 baseDiffuseColor = PrimaryScarletAlbedo * (diffuseIntensity + AmbientIntensity);

    // 2. High-Fidelity Subsurface Scattering (SSS) Backlight Transmission
    // Calculate light passing directly through thin edges of the claw petals
    float3 distortionVector = N * 0.28f; // Offsets normals slightly to avoid hard shadow artifacts
    float3 SSSLightVector = normalize(L + distortionVector);
    
    // Dot product evaluating alignment between viewer and backlight rays passing through organic body
    float sssAlignment = saturate(dot(V, -SSSLightVector));
    
    // High exponent curve generates sharp bloom concentrated tightly at thin geometric bounds
    float subsurfaceTransmission = pow(sssAlignment, 6.0f) * SubsurfaceThickness;
    float3 finalTranslucentGlow = BacklightGlowColor * subsurfaceTransmission;

    // 3. Specular Highlight Highlight Pass (Simulates thin organic dew/waxy coating coating)
    float3 halfVector = normalize(L + V);
    float specularHighlight = pow(saturate(dot(N, halfVector)), 32.0f) * 0.4f;

    // Output unified composition sequence
    float3 finalCompositedColor = baseDiffuseColor + finalTranslucentGlow + (float3)specularHighlight;
    return float4(finalCompositedColor, 1.0f);
}
