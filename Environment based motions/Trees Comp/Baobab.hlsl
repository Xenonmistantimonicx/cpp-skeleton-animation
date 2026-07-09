#ifndef BAOBAB_RESERVOIR_WIND_SHADING_INCLUDED
#define BAOBAB_RESERVOIR_WIND_SHADING_INCLUDED

cbuffer SavannaClimateControlBuffer : register(b2)
{
    float3 g_HarmattanWindVector;
    float  g_WindForceVelocity;
    float  g_AbsoluteRunningTime;
    float  g_TrunkHydrationFactor;    // 0.0 (Severe Drought: Deflated Trunk) -> 1.0 (Post-Monsoon Peak: Full Core Swell)
    float2 g_InertiaDampeningParams; 
};

struct VertexInputDigitaleLeaf
{
    float3 MeshPosition       : POSITION;
    float3 SurfaceNormal      : NORMAL;
    float2 UV                 : TEXCOORD0;
    float3 ClusterPivotPoint  : TEXCOORD1;
    float  ExpansionChannel   : BLENDWEIGHT0; // 1.0 = Caudex Core Trunk, 0.0 = Distal Twigs
    float  BranchMassWeight   : BLENDWEIGHT1; // Direct mass dampening scaler passed from CPU compiler
};

struct VertexOutputDigitaleLeaf
{
    float4 ProjectedPosition : SV_POSITION;
    float3 ComputedNormal    : NORMAL;
    float2 UV                : TEXCOORD0;
};

float3 CalculateAAABaobabDynamicDisplacement(float3 localPos, float3 surfaceNormal, float3 pivot, float expandChannel, float massWeight)
{
    // STEP 1: REAL-TIME HYDRATION CORE SWELL (Radial Trunk Scale Inflation)
    // Simulates the physical swelling of the fibrous trunk expanding as it stores wet season monsoon waters
    float proceduralSwellIntensity = 0.25f; // Max displacement boundary limits in meters
    float continuousSwellOffset = (g_TrunkHydrationFactor - 0.5f) * proceduralSwellIntensity;
    
    // Displace vertex positions outwards along their direct normals if flagged within the expansion channel
    float3 hydrationSwellVector = surfaceNormal * (expandChannel * continuousSwellOffset);

    // STEP 2: HIGH-INERTIA MASS DAMPENING (Trunk and heavy limbs resist swaying)
    float lowFreqPhase = g_AbsoluteRunningTime * 0.4f + pivot.y * 0.02f;
    float3 broadSwayOffset = g_HarmattanWindVector * (sin(lowFreqPhase) * g_WindForceVelocity * 0.05f);

    // STEP 3: DISTAL TWIG LEAF SHIVER (Only light outer finger leaves ripple)
    float highFreqTime = g_AbsoluteRunningTime * 24.50f + localPos.x * 4.0f;
    float leafVibration = sin(highFreqTime) * cos(highFreqTime * 0.75f) * g_WindForceVelocity;
    
    float3 crossWindAxis = cross(g_HarmattanWindVector, float3(0.0f, 1.0f, 0.0f));
    float3 leafRippleOffset = crossWindAxis * (leafVibration * 0.14f);

    // Blend wind states: Massive base structures use broad dampened sways, outer leaf tips take high tremors
    float windFlexAlpha = saturate(1.0f - massWeight); // Invert mass multiplier to filter flexible node assets
    float3 combinedWindDisplacement = lerp(broadSwayOffset, leafRippleOffset * localPos.y, windFlexAlpha);

    return hydrationSwellVector + combinedWindDisplacement;
}

#endif // BAOBAB_RESERVOIR_WIND_SHADING_INCLUDED
