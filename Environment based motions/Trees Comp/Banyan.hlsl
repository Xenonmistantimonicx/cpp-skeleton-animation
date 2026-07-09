#ifndef VATAVRIKSHA_WIND_PIPELINE_INCLUDED
#define VATAVRIKSHA_WIND_PIPELINE_INCLUDED

cbuffer MonsoonWeatherBuffer : register(b1)
{
    float3 g_GlobalWeatherVector;
    float  g_WindVelocitySpeed;
    float  g_EngineAbsoluteTime;
    float  g_SeasonalFlushProgress; // 0.0 (Green Summer) -> 1.0 (Pink Spring Flush)
    float2 g_MacroWaveNoise; 
};

struct VertexInputBanyan
{
    float3 MeshPosition   : POSITION;
    float3 Normal         : NORMAL;
    float2 TexCoord       : TEXCOORD0;
    float  PropRootWeight : BLENDWEIGHT0; // 1.0 = Grounded Pillar, 0.0 = Flexible Branch
    float  BranchUID      : BLENDINDICES0; // Keeps leaf clusters grouped on same branch together
};

struct VertexOutputBanyan
{
    float4 ProjectedPosition : SV_POSITION;
    float4 FragColor          : COLOR0;
    float2 TexCoord          : TEXCOORD0;
};

// Computes sacred Banyan Semi-Evergreen budding cycle (Coppery Pink -> Deep Green)
float4 ComputeAdvancedBanyanPhenology(float progress)
{
    float4 summerDeepGreen = float4(0.08f, 0.32f, 0.16f, 1.0f);
    float4 copperyPinkBuds = float4(0.98f, 0.61f, 0.58f, 1.0f); // Bright pink flush
    
    // Smooth linear interpolation calculation
    return lerp(summerDeepGreen, copperyPinkBuds, progress);
}

float3 CalculateAAAQuakingBanyanSway(float3 worldPos, float propWeight, float branchID)
{
    // Phase 1: Branch/Canopy Macro-Sway (Low Frequency, Long Amplitude)
    float branchPhase = (branchID * 0.423f) + (worldPos.y * 0.08f);
    float macroSwayPhase = g_EngineAbsoluteTime * g_MacroWaveNoise.x + branchPhase;
    
    float3 macroSwaydispl = g_GlobalWeatherVector * (sin(macroSwayPhase) * cos(macroSwayPhase * 0.41f) * (g_WindVelocitySpeed * 0.35f));

    // Phase 2: Glossy Leaf Shimmer Vibrations (High Frequency, Rotational Micro-Oscillations)
    // Applied perpendicular to leaf bone structure only
    float highFreqTimeA = g_EngineAbsoluteTime * 32.50f;
    float highFreqTimeB = g_EngineAbsoluteTime * 38.15f;

    float microTrembleX = sin(highFreqTimeA + (worldPos.x * 2.15f));
    float microTrembleZ = cos(highFreqTimeB + (worldPos.z * 1.82f));
    float combinedShimmer = (microTrembleX * microTrembleZ) * g_WindVelocitySpeed;

    float3 perpendicularWindAxis = cross(g_GlobalWeatherVector, float3(0.0f, 1.0f, 0.0f));
    float3 microDisplacement = perpendicularWindAxis * (combinedShimmer * 0.15f);

    // Apply strict stiffness factor based on prop root anchoring
    // Vertices mapped as grounded supporting pillars stay locked at near-zero displacement limits.
    float dynamicWeight = saturate(1.0f - propWeight); // Supporting pillars stay zero, branches stay flexible

    return (macroSwaydispl + microDisplacement) * dynamicWeight;
}

#endif // VATAVRIKSHA_WIND_PIPELINE_INCLUDED
