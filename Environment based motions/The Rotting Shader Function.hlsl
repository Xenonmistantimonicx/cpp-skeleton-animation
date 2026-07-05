// AAA Procedural Rotting and Structural Decay Function
void EvaluateFruitRot_float(
    float3 BaseColor,       // The original pristine fruit texture color
    float3 RotColor,        // The target rotten/decayed color (e.g., dark brown/black)
    float3 VertexNormal,    // Normal vector of the fruit mesh
    float NoiseTexSample,   // A tiling Perlin or Simplex noise texture map
    float RotProgress,      // Progress value from 0.0 (Fresh) to 1.0 (Completely Rot)
    float CavityDepth,      // How deeply the rotten spots cave in (e.g., 0.15)
    out float3 OutColor,    // Final output pixel color
    out float3 OutPositionOffset // Final vertex offset to deflate the mesh
)
{
    // 1. Calculate a dynamic, organic threshold for where the rot spreads
    // Adding the noise texture map creates erratic, non-uniform mold patches
    float rotMask = saturate((RotProgress * 1.3) - (NoiseTexSample * 0.4));

    // 2. Color Lerp: Blend smoothly from fresh fruit color to standard rot color
    // We darken the rot mask at its deepest edges to simulate mold borders
    float3 finalRotColor = lerp(RotColor, RotColor * 0.2, rotMask);
    OutColor = lerp(BaseColor, finalRotColor, rotMask);

    // 3. Vertex Deflation: Make rotten spots cave inward to simulate biological decay
    // Deforming along the negative vertex normal shrinks the mesh specifically at rot locations
    float3 deflation = -VertexNormal * (rotMask * CavityDepth);
    
    // Add a secondary micro-wobble to make the rotting look organic instead of mathematically flat
    deflation.y -= (rotMask * CavityDepth * 0.5); 

    OutPositionOffset = deflation;
}
