// AAA Procedural Copper Oxidation and Patina Growth Function
void EvaluateCopperOxidation_float(
    float3 RawCopperColor,    // Shiny orange/gold metallic color
    float3 PatinaColor,       // Chalky verdigris green (e.g., RGB: 0.4, 0.72, 0.63)
    float3 DeepCreviceColor,  // Dark oxidized brown/black for cavities
    float RoughnessFresh,     // 0.05 (Super smooth/shiny)
    float AmbientOcclusion,   // Baked or dynamic screen-space AO map
    float OxidationProgress,  // Driven by CPU timeline (0.0 to 1.0)
    float NoiseMapSample,     // Tiling cellular/grunge noise texture
    out float3 OutBaseColor,  // Final dynamic pixel color
    out float OutMetallic,    // Metallic map output (changes over time)
    out float OutRoughness    // Roughness map output (changes over time)
)
{
    // 1. Rust Distribution Mask: Moisture pools in corners first (Using Ambient Occlusion)
    // Patina grows from deep crevices outward, combined with random environmental grunge noise
    float patinaMask = saturate((OxidationProgress * 1.4) - (NoiseMapSample * 0.3));
    patinaMask = saturate(patinaMask + (1.0 - AmbientOcclusion) * OxidationProgress);

    // 2. Multi-Stage Color Blending
    // Transition from Fresh Copper -> Dull Brown (Early Oxidation) -> Verdigris Green
    float3 dullCopper = lerp(RawCopperColor, DeepCreviceColor, saturate(OxidationProgress * 1.5));
    OutBaseColor = lerp(dullCopper, PatinaColor, smoothstep(0.2, 0.7, patinaMask));

    // 3. AAA Material Property Shifting (PBR Logic)
    // Raw copper is 100% metallic. Rotted copper crust is an organic salt (0% metallic).
    OutMetallic = lerp(1.0, 0.0, step(0.4, patinaMask) * patinaMask);

    // Patina is incredibly rough, dry, and powdery compared to polished metal
    float targetRoughness = lerp(0.85, 0.95, NoiseMapSample); // Chalky texture noise
    OutRoughness = lerp(RoughnessFresh, targetRoughness, patinaMask);
}
