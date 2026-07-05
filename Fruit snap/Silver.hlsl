// AAA Procedural Silver Tarnishing and Iridescent Tarnish Function
void EvaluateSilverTarnish_float(
    float3 FreshSilverColor,   // Ultra-bright white metallic base color (RGB near 0.95, 0.95, 0.95)
    float3 MatteBlackSulfide,  // Heavy dead-black tarnish color (RGB near 0.05, 0.05, 0.05)
    float3 SurfaceNormal,      // Geometry surface normals
    float3 ViewDirection,      // Direction from pixel to camera (for angular iridescence)
    float BaseRoughness,       // Flawless silver smoothness (e.g., 0.02)
    float TarnishProgress,     // Driven by CPU timeline (0.0 to 1.0)
    float GrungeNoiseSample,   // High-frequency procedural noise map for organic spreading
    out float3 OutBaseColor,   // Final blended PBR color map
    out float OutMetallic,     // Metallic map value
    out float OutRoughness     // Roughness map value
)
{
    // 1. Organic Tarnish Accumulation Mask
    // Tarnish hits exposed outer surfaces and stagnant details differently
    float tarnishMask = saturate((TarnishProgress * 1.3) - (GrungeNoiseSample * 0.2));

    // 2. AAA Feature: Thin-Film Interference / Iridescence Calculation
    // As the sulfide film builds up, light waves bounce off the top and bottom of the layer, 
    // creating shifting colors based on the viewing angle (Fresnel effect)
    float viewAngleFactor = saturate(dot(SurfaceNormal, ViewDirection));
    float iridescencePhase = saturate(tarnishMask * 2.0) * (1.0 - step(0.7, tarnishMask));
    
    // Procedural color shift mimicking real optical light interference (Amber -> Purple -> Royal Blue)
    float3 iridescentColor = float3(
        lerp(0.85, 0.20, iridescencePhase), // Red drops rapidly
        lerp(0.70, 0.05, iridescencePhase), // Green drops to leave purple
        lerp(0.40, 0.90, iridescencePhase)  // Blue peaks mid-transition
    );

    // 3. Multi-Stage Visual Blending
    // Stage A: Fresh Silver -> Stage B: Beautiful Iridescent Shift -> Stage C: Matte Black Crusting
    float3 midStageColor = lerp(FreshSilverColor, iridescentColor, iridescencePhase);
    OutBaseColor = lerp(midStageColor, MatteBlackSulfide, smoothstep(0.5, 0.8, tarnishMask));

    // 4. Structural PBR Transmutation
    // Fresh silver is perfectly metallic. Silver sulfide crust is a mineral salt (0% metallic).
    // We smoothly dip the metallic slider out as the heavy black crust solidifies.
    OutMetallic = lerp(1.0, 0.0, smoothstep(0.6, 0.9, tarnishMask));

    // Silver sulfide is rough, dry, and scatters light instead of reflecting it cleanly
    float crustRoughness = lerp(0.75, 0.90, GrungeNoiseSample);
    OutRoughness = lerp(BaseRoughness, crustRoughness, smoothstep(0.4, 0.8, tarnishMask));
}
