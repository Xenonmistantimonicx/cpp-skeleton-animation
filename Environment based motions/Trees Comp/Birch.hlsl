#ifndef BIRCH_KINETIC_WIND_SHADING_INCLUDED
#define BIRCH_KINETIC_WIND_SHADING_INCLUDED

cbuffer NordicWoodlandWeatherBuffer : register(b4)
{
    float3 g_BorealWindVector;
    float  g_WindVelocitySpeed;
    float  g_AbsoluteTimeClock;
    float  g_TranslucencySunbeamFilter; // Hook for god-ray density modifications inside pixel pipeline
    float2 g_WhipFrequencyScale;
};

struct VertexInputSerratedLeaf
{
    float3 MeshPosition       : POSITION;
    float3 NormalSpace        : NORMAL;
    float2 UV                 : TEXCOORD0;
    float3 ClusterPivot       : TEXCOORD1;
    float  LenticelScarWeight : BLENDWEIGHT0; // Passes scarring thresholds directly to pixel albedo lerps
    float  StemElasticityAlpha: BLENDWEIGHT1; // Controls linear height vertical whipping scales
};

struct VertexOutputSerratedLeaf
{
    float4 ProjectedPosition  : SV_POSITION;
    float3 ShadingNormal      : NORMAL;
    float2 UVCoordinates      : TEXCOORD0;
    float  SunbeamScatterPass : TEXCOORD1; // Drives dynamic subsurface scattering inside thin leaves
};

float3 CalculateAAABirchKineticWhippingSway(float3 localPos, float3 instancePivot, float elasticity)
{
    // PHASE 1: HIGH-FLEX WHIPPING STEM (Medium-High Frequency, High Lateral Displacement)
    // Slender trunks bend heavily under wind load like a whip or fishing rod
    float whipClock = g_AbsoluteTimeClock * g_WhipFrequencyScale.x + (instancePivot.y * 0.12f);
    float sinusoidalBend = sin(whipClock) * cos(whipClock * 0.62f);
    
    // Deform paths scale quadratically using the pre-computed height elasticity alpha channel
    float exponentialFlexFactor = pow(elasticity, 2.0f);
    float3 macroStemWhip = g_BorealWindVector * (sinusoidalBend * g_WindVelocitySpeed * 1.65f * exponentialFlexFactor);

    // PHASE 2: AIRY LEAF CLUSTER SHIMMER (High Frequency Micro-Oscillations)
    // Small serrated leaves tremor rapidly to catch speculative specular sun glints
    float leafShimmerClock = g_AbsoluteTimeClock * 28.40f + localPos.y * 5.0f;
    float highFreqTremor = sin(leafShimmerClock) * cos(leafShimmerClock * 1.15f) * g_WindVelocitySpeed;

    float3 perpendicularWindAxis = cross(g_BorealWindVector, float3(0.0f, 1.0f, 0.0f));
    float3 microLeafRipple = perpendicularWindAxis * (highFreqTremor * 0.09f * elasticity);

    // Terminal branch tip nodes flex maximally, stem attachments stay relative
    float leafEdgeFlexWeight = saturate(localPos.y * 1.5f);

    return macroStemWhip + (microLeafRipple * leafEdgeFlexWeight);
}

#endif // BIRCH_KINETIC_WIND_SHADING_INCLUDED
