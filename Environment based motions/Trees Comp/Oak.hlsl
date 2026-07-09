#ifndef OAK_CANOPY_WIND_SHADING_INCLUDED
#define OAK_CANOPY_WIND_SHADING_INCLUDED

cbuffer TemperateWoodlandClimateControl : register(b0)
{
    float3 g_GaleWindDirection;
    float  g_WindVelocityForce;
    float  g_AbsoluteRunningClock;
    float  g_CanopyDensityFactor; // Controls shadowing contrast multipliers
    float2 g_TrunkSwayWaveFreq;
};

struct VertexInputLobedLeaf
{
    float3 LocalPosition    : POSITION;
    float3 Normal           : NORMAL;
    float2 UV               : TEXCOORD0;
    float3 InstanceAnchor   : TEXCOORD1;
    float  BlockyPlating    : BLENDWEIGHT0; // 1.0 = Rigid Trunk, 0.0 = Secondary Limbs
    float  ClumpPhaseOffset : BLENDINDICES0; // Synchronizes leaf groups on the same sub-branch
};

struct VertexOutputLobedLeaf
{
    float4 SVPosition       : SV_POSITION;
    float3 PassWorldNormal  : NORMAL;
    float2 UVCoord          : TEXCOORD0;
    float  AmbientOcclusion : TEXCOORD1; // Passes calculated depth multipliers directly to pixel shading
};

float3 CalculateAAAOakSkeletalDisplacement(float3 localPos, float3 instanceAnchor, float phaseOffset, float plating)
{
    // PHASE 1: HEAVY RIGID LIMB ROCKING (Low Frequency, Heavy Drag Phase)
    // Massive gnarled limbs swing slowly with massive inertial resistance
    float limbClock = g_AbsoluteRunningClock * g_TrunkSwayWaveFreq.y + (instanceAnchor.x * 0.05f + instanceAnchor.z * 0.03f);
    float heavyRocking = sin(limbClock) * cos(limbClock * 0.35f);
    
    // Scale motion down vertically based on bark rigidity thickness (plating weight)
    float3 macroLimbSway = g_GaleWindDirection * (heavyRocking * g_WindVelocityForce * 0.28f * saturate(1.0f - plating));

    // PHASE 2: DENSE CLUMP ROTATIONAL FLUTTER (High Frequency Group Turbulence)
    // Synchronized group shaking using the unique branch phase offset parameters
    float clusterClock = g_AbsoluteRunningClock * 14.50f + phaseOffset;
    float clumpVibration = sin(clusterClock) * cos(clusterClock * 0.85f) * g_WindVelocityForce;

    // Cross-axis displacement vector loops to generate circular shaking behaviors inside rosettes
    float3 crossAxisWind = cross(g_GaleWindDirection, float3(0.0f, 1.0f, 0.0f));
    float3 microClumpDisplacement = (g_GaleWindDirection * clumpVibration * 0.12f) + (crossAxisWind * cos(clusterClock) * 0.08f);

    // Leaf vertex boundary radius limit check (Outer tip regions flex, base clusters stay anchored)
    float leafEdgeWeight = saturate(length(localPos.123));

    return macroLimbSway + (microClumpDisplacement * leafEdgeWeight);
}

#endif // OAK_CANOPY_WIND_SHADING_INCLUDED
