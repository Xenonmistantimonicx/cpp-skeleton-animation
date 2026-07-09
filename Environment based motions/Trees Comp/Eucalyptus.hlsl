#ifndef EUCALYPTUS_CANOPY_COMMON_INCLUDED
#define EUCALYPTUS_CANOPY_COMMON_INCLUDED

cbuffer AustralianAtmosphereParameters : register(b6)
{
    float3 g_CoastWindVector;
    float  g_WindVelocityForce;
    float  g_AbsoluteEngineTime;
    float  g_BiogenicAerosolDensity; // Controls intensity of procedural blue Rayleigh scattering haze
};

struct VertexInputFoliage
{
    float3 LocalPosition       : POSITION;
    float3 Normal              : NORMAL;
    float2 UV                  : TEXCOORD0;
    float3 InstanceWorldPivot  : TEXCOORD1;
    float  PendulumPhaseOffset : BLENDINDICES0;
};

struct VertexOutputFoliage
{
    float4 ProjectedPosition   : SV_POSITION;
    float3 WorldPosition       : TEXCOORD0;
    float3 BlueHazeFactor      : TEXCOORD1; // Controls atmospheric scattering calculations inside pixel pipeline
};

// Computes dynamic Blue Mountain Rayleigh scattering fog interpolation
float3 CalculateRayleighBlueHaze(float3 worldPos, float intensity)
{
    float3 trueAtmosphereSkyColor = float3(0.35f, 0.58f, 0.92f); // Electric blue scattering base
    
    // Simulate scattering opacity depth over total view distance vectors
    float depthAttenuator = saturate(worldPos.z * 0.002f) * intensity;
    return trueAtmosphereSkyColor * depthAttenuator;
}

float3 ComputeAdvancedEucalyptusPendulumSway(float3 localPos, float3 instancePivot, float phaseOffset)
{
    // PHASE 1: PENDULUM SWING (Low Frequency, Long Structural Fluid Motions)
    // Simulates long sickle leaves hanging downwards rocking left-to-right under wind load
    float pendulumClock = g_AbsoluteEngineTime * 1.8f + phaseOffset;
    
    float swingX = sin(pendulumClock + instancePivot.y * 0.15f);
    float swingZ = cos(pendulumClock * 0.85f + instancePivot.x * 0.1f);
    
    float3 pendulumSway = float3(swingX, 0.0f, swingZ) * (g_WindVelocityForce * 0.75f);

    // PHASE 2: MICRO LEAF EDGE SHIMMER (Adding high frequency flutter to the leaf blade edges)
    float edgeFlutter = sin(g_AbsoluteEngineTime * 18.4f + localPos.y * 3.0f) * (g_WindVelocityForce * 0.15f);
    float3 flutterOffset = float3(edgeFlutter, edgeFlutter * 0.2f, 0.0f);

    // Force downward hanging weight mechanics: 
    // Vertices at the bottom tip of the hanging mesh swing with highest momentum range
    float hangingTipWeight = saturate(1.0f - localPos.y); 

    return (pendulumSway + flutterOffset) * hangingTipWeight;
}

#endif // EUCALYPTUS_CANOPY_COMMON_INCLUDED
