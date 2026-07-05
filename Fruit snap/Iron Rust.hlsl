// AAA Procedural Iron Rusting and Structural Flaking Function
void EvaluateIronRust_float(
    float3 CleanIronColor,     // Polished/painted iron metal base color
    float3 CoreRustColor,      // Deep, crusty reddish-brown raw rust color
    float3 YellowRustColor,    // Lighter, powdery yellow-orange outer rust highlight
    float3 VertexNormal,       // Mesh surface normal vectors
    float DynamicRustProgress, // Input value driven by CPU timeline (0.0 to 1.0)
    float NoiseTextureSample,  // Tiling structural/perlin noise map for patchiness
    float HeightMapSample,     // Micro-height displacement map for crust texture
    out float3 OutBaseColor,   // Final blended PBR base color
    out float OutMetallic,     // Metallic attenuation profile
    out float OutRoughness,    // Roughness progression profile
    out float3 OutNormalOffset // Normal vector deformation for blistering bumps
)
{
    // 1. Rust Growth Distribution Mask
    // Creates jagged, organic creeping patches instead of a linear dissolve
    float rustMask = saturate((DynamicRustProgress * 1.5) - (NoiseTextureSample * 0.5));

    // 2. Multi-Tone Color Gradient
    // Real rust isn't one solid brown color. It has a deep, dark inner core 
    // and a powdery, brighter yellow-orange edge where moisture recently dried.
    float edgeHighlightMask = smoothstep(0.1, 0.5, rustMask) * (1.0 - smoothstep(0.6, 1.0, rustMask));
    float3 activeRustColor = lerp(CoreRustColor, YellowRustColor, edgeHighlightMask);
    
    OutBaseColor = lerp(CleanIronColor, activeRustColor, rustMask);

    // 3. PBR Property Inversion
    // Polished iron is highly metallic and smooth. 
    // Rusted iron flakes convert to a dry, heavily non-metallic oxide powder.
    OutMetallic = lerp(1.0, 0.0, rustMask);
    OutRoughness = lerp(0.2, lerp(0.85, 1.0, HeightMapSample), rustMask);

    // 4. Structural Blistering and Pitting
    // Using the height map, we deform the mesh outward/inward to simulate 
    // the rough, flaky, bubbling physical texture of oxidized iron.
    float blisterProfile = HeightMapSample * rustMask * 0.05; // Up to 5cm micro-blistering
    OutNormalOffset = VertexNormal * blisterProfile;
}
