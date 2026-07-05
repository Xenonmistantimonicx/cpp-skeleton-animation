// AAA Procedural Tree Decay and Bark Peeling Function
void EvaluateTreeRot_float(
    float3 OuterBarkColor,   // Clean, healthy outer bark texture
    float3 InnerRottenWood,  // Dark, crumbly, decayed inner wood texture
    float3 FungiColor,       // Bright shelf-fungi/moss coloration
    float2 UV,               // Mesh Texture Coordinates
    float NoiseTexture,      // Tiling Perlin noise representing wood rot patterns
    float DecayProgress,     // Input driven by CPU timeline (0.0 to 1.0)
    float AlphaThreshold,    // Cutout parameter for structural hollows
    out float3 OutBaseColor, // Combined output color
    out float OutAlpha       // Output opacity for structural holes
)
{
    // --- STAGE 1: BARK PEELING MASK ---
    // Higher noise threshold means the bark cracks open unevenly along its grain
    float barkPeelMask = saturate((DecayProgress * 1.5) - (NoiseTexture * 0.4));
    
    // Smoothly transition from outer bark to rotten interior pulpy wood
    float3 woodBase = lerp(OuterBarkColor, InnerRottenWood, barkPeelMask);

    // --- STAGE 2: PROCEDURAL FUNGAL COLONIZATION ---
    // Fungi clusters grow specifically at the borders where the bark is breaking away
    float fungalMask = smoothstep(0.3, 0.7, barkPeelMask) * (1.0 - barkPeelMask);
    fungalMask *= step(0.6, NoiseTexture); // Cluster them instead of smooth lines
    
    OutBaseColor = lerp(woodBase, FungiColor, fungalMask * DecayProgress);

    // --- STAGE 3: STRUCTURAL HOLLOWING (ERODE MESH HOLES) ---
    // In late stages of rot, parts of the trunk turn completely hollow/powdery.
    // We pass this into the shader clip() function or opacity mask slot.
    float alphaMask = saturate((DecayProgress - 0.6) * 2.5) * NoiseTexture;
    OutAlpha = (alphaMask > AlphaThreshold) ? 0.0 : 1.0; 
}
