// --- ENGINE BINDING REGISTERS ---
cbuffer AquaticMaterialUniforms : register(b1)
{
    float3 g_SunDirectionWorldVector : packoffset(c0.x); // Directional Light Vector
    float  g_CausticsIntensityScale  : packoffset(c0.w); // Underwater refracted light intensity multiplier
    float3 g_ChlorophyllTissueBase   : packoffset(c1.x); // Pure translucent aquatic green base color
    float  g_GlobalTissueAlphaValue  : packoffset(c1.w); // Base transparency density modifier
    float3 g_MidribVeinOpaqueColor   : packoffset(c2.x); // Denser, less transparent green for the leaf structural veins
    float  g_MicroSurfaceRoughness   : packoffset(c2.w); // Material specular specular power adjustments
};

struct PixelInputVertexCache
{
    float4 HardwareSVPosition : SV_POSITION;
    float3 NormalWorldSystem  : NORMAL;
    float3 PositionWorldSystem: TEXCOORD0;
    float2 UVCoordinates      : TEXCOORD1;
};

// --- AAA CELLOPHANE PHOTOMETRIC COMPLEX MODEL ---
float4 PS_EchinodorusCellophaneMaster(PixelInputVertexCache input) : SV_Target
{
    // Resolve geometric world parameters
    float3 N = normalize(input.NormalWorldSystem);
    float3 L = normalize(g_SunDirectionWorldVector);
    float3 V = normalize(float3(0.0f, 8.0f, 6.0f) - input.PositionWorldSystem); // Camera Position target approximation

    // Dynamic double sided rendering transformation alignment logic
    float faceAlignment = dot(N, L);
    float absoluteLambertianDiffuse = max(faceAlignment, -faceAlignment);

    // EXTRACT PROCEDURAL MASK FROM GEOMETRY: Calculate high fidelity multi-vein paths
    // We isolate micro-slits using high-frequency sine waves along the UV coordinate mapping plane
    float horizontalVeinPattern = abs(sin(input.UVCoordinates.x * 64.0f + input.UVCoordinates.y * 12.0f));
    float longitudinalVeinPattern = saturate(1.0f - (abs(input.UVCoordinates.x - 0.5f) * 2.0f));
    
    // Combine texture map tracks into a single unified high fidelity vein modifier channel
    float leafStructuralVeinMask = saturate(pow(horizontalVeinPattern * longitudinalVeinPattern, 8.0f));

    // Interpolate surface coloration based on vein density maps
    // Veins are denser and block light, while cell tissue is almost completely transparent grey-green
    float3 baseTissueColor = lerp(g_ChlorophyllTissueBase, g_MidribVeinOpaqueColor, leafStructuralVeinMask);

    // 1. Double-Sided Light Transmission Shading Model (Translucency)
    // Cellophane leaves filter light instantly. Sunlight behind the asset causes intense glow emission.
    float transmissionRayAlignment = saturate(dot(V, -L));
    float transmissionFactor = pow(transmissionRayAlignment, 5.0f) * 1.85f;
    // Structural veins absorb more light, reducing transmission in high vein density masks
    float3 finalSubsurfaceScatterColor = baseTissueColor * transmissionFactor * (1.0f - leafStructuralVeinMask * 0.7f);

    // 2. Dual-Lobe Glass Specular Modeling (Cellophane Plastic Sheen)
    // Underwater leaves exhibit two light reflection coats: smooth cell membrane + wet outer shell gloss
    float3 H = normalize(L + V);
    float NdotH = saturate(dot(N, H));
    
    float specularLobeA = pow(NdotH, 128.0f) * 0.6f; // Sharp glass point highlight
    float specularLobeB = pow(NdotH, 16.0f) * (g_MicroSurfaceRoughness * 0.3f); // Blurred subsurface skin coating gloss
    float totalSpecularHighlight = specularLobeA + specularLobeB;

    // 3. Complete Fresnel Reflection Interface Multiplier
    float baseFresnelIndexOfRefraction = 0.05f; // Index value matching cell membrane tissue inside fluid
    float activeFresnelPass = baseFresnelIndexOfRefraction + (1.0f - baseFresnelIndexOfRefraction) * pow(1.0f - saturate(dot(V, N)), 5.0f);

    // Final color accumulation block
    float3 finalCompositedRGB = (baseTissueColor * (absoluteLambertianDiffuse + 0.15f)) + 
                                 finalSubsurfaceScatterColor + 
                                 ((float3)totalSpecularHighlight * activeFresnelPass);

    // Alpha Transparency Pipeline Mapping Calculation
    // Veins increase opacity, while the thin tissue scales down to maximum transparency levels
    float finalDynamicAlpha = lerp(g_GlobalTissueAlphaValue, 0.92f, leafStructuralVeinMask);
    // Fresnel reflections on edge angles boost alpha profile visibility
    finalDynamicAlpha = saturate(finalDynamicAlpha + (activeFresnelPass * 0.4f));

    return float4(finalCompositedRGB, finalDynamicAlpha);
}
