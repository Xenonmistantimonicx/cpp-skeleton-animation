// AAA Procedural Carcass Decomposition and Liquefaction Function
void AdvancedFleshDecay_float(
    float3 BaseSkinColor,   // Original texture color of the animal fur/skin
    float3 RawMeatColor,    // Internal exposed muscle/blood texture
    float3 RotToneColor,    // Rot color overlay (sickly dark greenish-black)
    float3 VertexNormal,    // Carcass mesh normal vectors
    float NoiseTexture,     // High-frequency Perlin noise texture
    float DecayStage,       // Input parameter from CPU (0.0 to 1.0)
    out float3 FinalColor,  // Combined output surface color
    out float3 VertexOffset // Output offset vector to distort the body shape
)
{
    // --- STAGE 1: BLOATING PHASE (DecayStage between 0.0 and 0.3) ---
    // Simulates the accumulation of gases inside the carcass body
    float bloatMask = saturate(DecayStage / 0.3) * (1.0 - saturate((DecayStage - 0.3) / 0.2));
    float3 bloatOffset = VertexNormal * (bloatMask * 0.18); // Expands outward cleanly

    // --- STAGE 2: COLLAPSE & LIQUEFACTION PHASE (DecayStage 0.3 to 1.0) ---
    // The gas escapes, the flesh collapses down flat toward the ground terrain
    float collapseMask = saturate((DecayStage - 0.3) / 0.7);
    float3 collapseOffset = float3(0, -1.0, 0) * (collapseMask * NoiseTexture * 0.25);

    // Combine individual physical deformation profiles
    VertexOffset = bloatOffset + collapseOffset;

    // --- STAGE 3: PROCEDURAL COLOR SKIN DISCOLORATION ---
    // Generates uneven spreading patches of necrosis based on noise data
    float necrosisMask = saturate((DecayStage * 1.4) - (NoiseTexture * 0.5));
    
    // Smoothly transition from skin, to rotted flesh tone, to dark putrefaction
    float3 deadFlesh = lerp(BaseSkinColor, RotToneColor, necrosisMask);
    
    // Expose internal raw structures at late stages where skin bursts open
    float structuralRupture = saturate((DecayStage - 0.5) * 2.0) * NoiseTexture;
    FinalColor = lerp(deadFlesh, RawMeatColor, step(0.65, structuralRupture));
}
