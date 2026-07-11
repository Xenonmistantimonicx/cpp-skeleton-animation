// --- REGISTERS PACK MATRIX BOUNDS ---
cbuffer EuonymusRenderParameters : register(b1)
{
    float3 g_WorldSunLightVector   : packoffset(c0.x);
    float  g_WartyRoughnessMultiplier: packoffset(c0.w); // Roughness value of outer pink shell
    float3 g_OuterCapsulePinkMagenta: packoffset(c1.x); // Distinct strawberry-pink shell hue
    float  g_SubsurfaceArilDensity : packoffset(c1.w); // Transparency density for scarlet seeds
    float3 g_InnerFleshyScarletAril: packoffset(c2.x); // Intense bright orange-red aril coloration
    float  g_FleshGlossReflection   : packoffset(c2.w); // Wet sheen calculation factor
};

struct PixelInputVertexData
{
    float4 SVPositionTarget : SV_POSITION;
    float3 NormalWorldSpace : NORMAL;
    float3 PositionWorldSpace: TEXCOORD0;
    float2 UVMappingChannel : TEXCOORD1;
};

// --- AAA EUONYMUS PHOTOMETRIC MASTER SCHEME ---
float4 PS_EuonymusCoreShadingPipeline(PixelInputVertexData input) : SV_Target
{
    float3 N = normalize(input.NormalWorldSpace);
    float3 L = normalize(g_WorldSunLightVector);
    float3 V = normalize(float3(0.0f, 6.0f, -4.0f) - input.PositionWorldSpace);

    // PROCEDURAL TEXTURING EXTENSION: Separate rough outer crust from wet inner aril seeds
    // We isolate structural splits by mapping deep localized coordinate falloffs
    float macroSplitLine = abs(sin(input.UVMappingChannel.x * 2.0f * 3.14159f));
    float arilSeedMask = saturate(pow(1.0f - macroSplitLine, 12.0f)); // 1.0 inside splitting gaps, 0.0 on shell

    // Compute Base Material Diffuse Properties
    float3 rawAlbedoColor = lerp(g_OuterCapsulePinkMagenta, g_InnerFleshyScarletAril, arilSeedMask);

    // Light dot computations
    float ndotl = dot(N, L);
    float standardDiffuse = max(0.0f, ndotl);

    // 1. HIGH-GRADE SUBSURFACE SCATTERING (For the internal seed arils)
    // When backlight matches seed positions, the organic tissue scatters light rays internally
    float scatteringProfile = saturate(dot(V, -L));
    float internalGlowScatter = pow(scatteringProfile, 8.0f) * g_SubsurfaceArilDensity;
    float3 sssColorComponent = g_InnerFleshyScarletAril * internalGlowScatter * arilSeedMask;

    // 2. DUAL INTERACTION SPECCING (Rough Shell vs High-Gloss Wet Seeds)
    float3 H = normalize(L + V);
    float ndoth = saturate(dot(N, H));
    
    // Outer shell has a dry, bumpy matte surface finish, while inside arils are highly reflective and slimy
    float microRoughnessPower = lerp(g_WartyRoughnessMultiplier, 0.08f, arilSeedMask); 
    float specSpecularPower = lerp(8.0f, 256.0f, arilSeedMask); // Sharp narrow peak reflection inside seeds

    float surfaceSpecularHighlight = pow(ndoth, specSpecularPower) * g_FleshGlossReflection;

    // 3. FRESNEL LIGHT ABSORPTION (Adding velvet properties to warty outer capsule walls)
    float activeFresnel = pow(1.0f - saturate(dot(V, N)), 5.0f);
    if (arilSeedMask < 0.5f) {
        // Boost color density along extreme profiles to match complex velvet botanical looks
        rawAlbedoColor += g_OuterCapsulePinkMagenta * activeFresnel * 0.35f;
    }

    // Combine all multi-layered parameters into standard lighting equation output
    float3 finalIlluminatedRGB = (rawAlbedoColor * (standardDiffuse + 0.08f)) + 
                                 sssColorComponent + 
                                 ((float3)surfaceSpecularHighlight * (activeFresnel + 0.1f));

    return float4(finalIlluminatedRGB, 1.0f); // Completely opaque asset output model
}
