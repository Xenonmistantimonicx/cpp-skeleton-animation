// AAA Procedural Brass Tarnishing and Calcification Function
void EvaluateBrassCorrosion_float(
    float3 FreshGoldBrassColor,// High-reflectivity golden yellow base metal
    float3 DarkTarnishColor,   // Deep chocolate brown/black oxidized tarnish
    float3 CalcifiedZincColor, // Chalky pale turquoise-white zinc salt deposit
    float3 VertexNormal,       // Surface geometry normals
    float MaterialRoughness,   // Base roughness of pristine brass (e.g., 0.1)
    float CorrosionProgress,   // Driven by CPU timeline (0.0 to 1.0)
    float GrungeNoiseSample,   // High-frequency cellular/spotted grunge noise texture
    float HeightMapSample,     // Height map for crusty buildup texture
    out float3 OutBaseColor,   // Final blended PBR color map
    out float OutMetallic,     // Metallic map value
    out float OutRoughness,    // Roughness map value
    out float3 OutVertexOffset // Vertex displacement for crusty salt buildup
)
{
    // 1. Dual-Stage Corrosion Masks
    // Stage A: Early Tarnish spreads smoothly across the whole surface
    float tarnishMask = saturate(CorrosionProgress * 1.5);
    
    // Stage B: Late Zinc Leaching spreads in crusty, spotted clusters based on cellular noise
    float zincLeachMask = saturate((CorrosionProgress * 2.0) - 1.0) * step(0.4, GrungeNoiseSample);
    zincLeachMask = saturate(zincLeachMask * GrungeNoiseSample);

    // 2. Multi-Layer Color Shifting
    // Blend from Fresh Gold -> Dark Tarnish -> Pale Zinc/Turquoise Crust
    float3 tarnishedMetal = lerp(FreshGoldBrassColor, DarkTarnishColor, tarnishMask);
    OutBaseColor = lerp(tarnishedMetal, CalcifiedZincColor, zincLeachMask);

    // 3. AAA PBR Property Transmutation
    // Fresh brass is 100% metallic. The tarnished layer stays metallic but gets darker.
    // The calcified zinc crust is a non-metallic (dielectric) mineral salt layer.
    float targetMetallic = lerp(1.0, 0.0, zincLeachMask);
    OutMetallic = saturate(targetMetallic);

    // Pristine brass is shiny and smooth. Tarnish is satin/dull. Zinc crust is completely matte and rough.
    float midRoughness = lerp(MaterialRoughness, 0.5, tarnishMask);
    OutRoughness = lerp(midRoughness, lerp(0.9, 1.0, HeightMapSample), zincLeachMask);

    // 4. Procedural Calcified Buildup (3D Micro-Topology)
    // Zinc oxide/carbonate builds up physically on the surface like crusty sea salt.
    // We displace the vertices outward along their normals only where leaching occurs.
    float microBuildupHeight = HeightMapSample * zincLeachMask * 0.03; // Up to 3cm crust thickness
    OutVertexOffset = VertexNormal * microBuildupHeight;
}
