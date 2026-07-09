#ifndef BRISTLECONE_PINE_WIND_SHADING_INCLUDED
#define BRISTLECONE_PINE_WIND_SHADING_INCLUDED

cbuffer AlpineWeatherSimulationBuffer : register(b5)
{
    float3 g_AlpineStormDirection;
    float  g_StormGaleForceVelocity;
    float  g_GlobalEngineClockTime;
    float4 g_AmberResinMaterialParams; // Packed vector: x=Roughness, y=Specular, z=SubsurfaceScattering
};

struct VertexInputNeedle
{
    float3 MeshLocalPosition  : POSITION;
    float3 VertexNormal       : NORMAL;
    float2 TexCoord           : TEXCOORD0;
    float3 InstanceCoreAnchor : TEXCOORD1; // Individual point position of bottle-brush attachment
    float  WindFlexAlpha      : BLENDWEIGHT0;
};

struct VertexOutputNeedle
{
    float4 SVPosition         : SV_POSITION;
    float3 PassWorldNormal    : NORMAL;
    float2 UVCoord            : TEXCOORD0;
    float  MaterialMask       : TEXCOORD1; // Passes structural dynamic live/dead weights to pixel stage
};

// Computes material split shading on pixel pipeline without overhead textures
float4 ComputeBristleconeBarkShading(float liveStripWeight, float3 worldNormal)
{
    float3 deadErodedAmberWood = float3(0.68f, 0.44f, 0.23f); // Polished yellow-orange core
    float3 liveFissuredDarkBark = float3(0.28f, 0.16f, 0.12f); // Dark weathered crust channel

    float3 finalAlbedo = lerp(deadErodedAmberWood, liveFissuredDarkBark, liveStripWeight);
    return float4(finalAlbedo, 1.0f);
}

float3 CalculateAdvancedBristleconeNeedleDisplacement(float3 localPos, float3 instanceAnchor, float flexAlpha)
{
    // PHASE 1: Rigid Stunted Tree Sway (Very Low Frequency, Minimal Amplitude Offset)
    float baseTrunkPhase = (instanceAnchor.y * 0.04f);
    float macroSwayWaves = sin(g_GlobalEngineClockTime * 0.6f + baseTrunkPhase);
    
    // Low deformation multiplier due to massive wood density parameters
    float3 macroSwayDisplacement = g_AlpineStormDirection * (macroSwayWaves * (g_StormGaleForceVelocity * 0.08f));

    // PHASE 2: BOTTLE-BRUSH STIFF HIGH-RESONANCE TREMBLE
    // Simulates continuous high-frequency vibration of dense short pine needles during alpine storms
    float highVelocityTimeA = g_GlobalEngineClockTime * 32.40f;
    float highVelocityTimeB = g_GlobalEngineClockTime * 38.15f;

    float microTrembleX = sin(highVelocityTimeA + (instanceAnchor.x * 4.2f));
    float microTrembleZ = cos(highVelocityTimeB + (instanceAnchor.z * 4.8f));
    float combinedVibration = microTrembleX * microTrembleZ * g_StormGaleForceVelocity;

    // Perpendicular vector shifts generating crystalline high-speed shivers instead of organic bends
    float3 stormPerpendicularVector = cross(g_AlpineStormDirection, float3(0.0f, 1.0f, 0.0f));
    float3 microVibrationDisplacement = stormPerpendicularVector * (combinedVibration * 0.22f);

    // Apply strict stiffness factor based on vertex distance vector
    // Needle root coordinates stay locked to branch, outer tips tremble under wind load
    float sharpNeedleWeight = saturate(localPos.y * 2.0f);

    return macroSwayDisplacement + (microVibrationDisplacement * sharpNeedleWeight * flexAlpha);
}

#endif // BRISTLECONE_PINE_WIND_SHADING_INCLUDED
