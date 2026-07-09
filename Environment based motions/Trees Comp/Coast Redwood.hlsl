#ifndef COAST_REDWOOD_GIANT_WIND_INCLUDED
#define COAST_REDWOOD_GIANT_WIND_INCLUDED

cbuffer PacificMarineClimateControlBuffer : register(b3)
{
    float3 g_MarineWindVectorDirection;
    float  g_WindVelocitySpeed;
    float  g_AbsoluteRunningClock;
    float  g_CanopyDampingConstant; // High-altitude wood dampening thresholds tracking matrix
    float2 g_RedwoodSwayWaveFreq;
};

struct VertexInputFlatSpray
{
    float3 PositionMeshSpace : POSITION;
    float3 NormalMeshSpace   : NORMAL;
    float2 UV                : TEXCOORD0;
    float3 InstancePivot     : TEXCOORD1;
    float  HeightAttenuation : BLENDWEIGHT0; // Core calculation pipeline: Height index scale vector
    float  FurrowChannel     : BLENDWEIGHT1; // Passes structural displacement indices directly to vertex stages
};

struct VertexOutputFlatSpray
{
    float4 ProjectedPosition : SV_POSITION;
    float4 ShadingFactor      : COLOR0;
    float2 UVCoordinates     : TEXCOORD0;
};

float3 CalculateSequoiaDynamicWindDisplacement(float3 vertexPos, float3 instancePivot, float heightScale)
{
    // PHASE 1: HYPER-SCALE COLUMNAR LEVERAGE (Linear High-Altitude Sway Scaling)
    // The top canopy at 90 meters travels in vast horizontal pathways compared to the locked solid trunk base base
    float altitudeClock = g_AbsoluteRunningClock * g_RedwoodSwayWaveFreq.x + (instancePivot.y * 0.015f);
    float basicLeaverSway = sin(altitudeClock) * cos(altitudeClock * 0.48f);
    
    // Leverage scales quadratically over the total height parameter
    float exponentialLeverageFactor = pow(heightScale, 2.0f);
    float3 macroColumnSway = g_MarineWindVectorDirection * (basicLeaverSway * g_WindVelocitySpeed * 3.8f * exponentialLeverageFactor);

    // PHASE 2: FLAT TIER HORIZONTAL TRAY FLUTTER (Aerodynamic Low-Drag Flat Shimmers)
    // Flat spray leaf arrays flutter rapidly up-and-down rather than side-to-side
    float trayClock = g_AbsoluteRunningClock * 16.80f + instancePivot.x * 1.5f + instancePivot.z * 1.2f;
    float verticalFlutter = sin(trayClock) * cos(trayClock * 0.92f) * g_WindVelocitySpeed;

    // Direct vertical Y deflection vector mappings mimicking flat aerodynamic tray sheets
    float3 microTrayDisplacement = float3(0.0f, verticalFlutter * 0.15f * heightScale, 0.0f);

    // Vertex length calculation limits displacement ranges on outmost pine branch needles tip rings
    float branchNeedleEdgeWeight = saturate(vertexPos.y * 1.4f);

    return macroColumnSway + (microTrayDisplacement * branchNeedleEdgeWeight);
}

#endif // COAST_REDWOOD_GIANT_WIND_INCLUDED
