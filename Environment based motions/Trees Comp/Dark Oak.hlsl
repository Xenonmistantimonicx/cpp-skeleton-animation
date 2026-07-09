#ifndef DARK_OAK_ROOF_SHADING_INCLUDED
#define DARK_OAK_ROOF_SHADING_INCLUDED

cbuffer BlackForestAtmosphereControlBuffer : register(b0)
{
    float3 g_StormWindVectorDirection;
    float  g_WindGaleVelocity;
    float  g_EngineClockSystemTime;
    float  g_CanopyAmbientOcclusionWeight; // Massive self-shadowing factor underneath the flat roof
    float2 g_RoofSwayFrequencyWave;
};

struct VertexInputRoofLeaf
{
    float3 PositionMeshSpace  : POSITION;
    float3 NormalSurfaceSpace : NORMAL;
    float2 TexCoord           : TEXCOORD0;
    float3 GlobalInstancePivot: TEXCOORD1;
    float  FusedCoreWeight    : BLENDWEIGHT0; // 1.0 = Ground Fused Trunk Base, 0.0 = Hanging Branches
    float  RoofFlexWeights    : BLENDWEIGHT1; // Triggers flat platform aerodynamic vertical oscillations
};

struct VertexOutputRoofLeaf
{
    float4 SVPosition        : SV_POSITION;
    float3 PassWorldPosition : NORMAL;
    float2 UVCoord           : TEXCOORD0;
    float  ShadowMaskValue   : TEXCOORD1; // Used for calculating global real-time dark shadow attenuation
};

float3 CalculateAAADarkOakPlatformDisplacement(float3 localPos, float3 instancePivot, float fusedWeight, float roofWeight)
{
    // PHASE 1: VERTICAL PRESSURE SAG (Low Frequency, Heavy Aero Drag Compression)
    // Flat ceiling structures act like wings under wind loads, executing up-and-down vertical flexes rather than side tilts
    float ceilingWaveClock = g_EngineClockSystemTime * g_RoofSwayFrequencyWave.y + (instancePivot.x * 0.03f + instancePivot.z * 0.02f);
    float verticalAeroSag = sin(ceilingWaveClock) * cos(ceilingWaveClock * 0.44f) * g_WindGaleVelocity;
    
    // Displace vertices primarily on the Y-Axis to simulate structural canopy bounce under wind pressure
    float3 macroRoofPlatformSway = float3(0.0f, verticalAeroSag * 0.35f * roofWeight, 0.0f);

    // PHASE 2: INTERLOCKING EDGE RIFFLE (High Frequency Aerodynamic Surface Flutter)
    // Individual perimeter leaf arrays on the edge of the flat platform ripple rapidly
    float edgeWaveClock = g_EngineClockSystemTime * 18.25f + localPos.x * 3.5f;
    float edgeRiffle = sin(edgeWaveClock) * cos(edgeWaveClock * 0.88f) * g_WindGaleVelocity;

    float3 horizontalWindCross = cross(g_StormWindVectorDirection, float3(0.0f, 1.0f, 0.0f));
    float3 microEdgeRipple = (float3(0.0f, 1.0f, 0.0f) * edgeRiffle * 0.12f * roofWeight);

    // Enforce strict zero-flex constraint for the thick fused multi-trunk core foundation
    float structuralInertiaWeight = saturate(1.0f - fusedWeight);

    return (macroRoofPlatformSway + microEdgeRipple) * structuralInertiaWeight;
}

#endif // DARK_OAK_ROOF_SHADING_INCLUDED
