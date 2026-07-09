#ifndef SUGAR_MAPLE_PHENOLOGY_INCLUDED
#define SUGAR_MAPLE_PHENOLOGY_INCLUDED

cbuffer NewEnglandWeatherBuffer : register(b7)
{
    float3 g_GaleWindVector;
    float  g_WindVelocitySpeed;
    float  g_EngineDeltaTime;
    float  g_AutumnProgress;      // 0.0 (Pure Summer Emerald) -> 0.5 (Electric Orange) -> 1.0 (Deep Crimson Red)
    float2 g_MacroWaveFrequency;
};

struct VertexInputMapleLeaf
{
    float3 MeshLocalPosition   : POSITION;
    float3 Normal              : NORMAL;
    float2 UV                  : TEXCOORD0;
    float3 InstanceWorldPivot  : TEXCOORD1;
    float  VeinDistanceChannel : BLENDWEIGHT0; // Passes computed procedural vein distances to pixel interpolation
};

struct VertexOutputMapleLeaf
{
    float4 SVPosition         : SV_POSITION;
    float4 LeafColor          : COLOR0;
    float2 TexCoord           : TEXCOORD0;
};

// Computes dynamic inside-out vein color transitions based on botanical anthocyanin maps
float4 ComputeAdvancedMapleChromaticBleed(float veinDistance, float progress)
{
    float4 summerEmerald = float4(0.08f, 0.42f, 0.12f, 1.0f);
    float4 vibrantGold   = float4(0.95f, 0.65f, 0.02f, 1.0f);
    float4 crimsonRed    = float4(0.72f, 0.04f, 0.08f, 1.0f);

    // Dynamic wave front propagation matching real leaf vascular systems
    float veinWaveFront = saturate(progress * 1.5f - veinDistance);

    float4 finalColor;
    if (progress < 0.5f)
    {
        // Transition Stage 1: Emerald Green to Vibrant Autumn Orange-Gold
        finalColor = lerp(summerEmerald, vibrantGold, veinWaveFront);
    }
    else
    {
        // Transition Stage 2: Orange-Gold to Deep Blood-Red Crimson
        float redProgress = saturate((progress - 0.5f) * 2.0f);
        float redVeinWaveFront = saturate(redProgress * 1.5f - (1.0f - veinDistance));
        finalColor = lerp(vibrantGold, crimsonRed, redVeinWaveFront);
    }

    return finalColor;
}

float3 ComputeAdvancedMaplePalmateDisplacement(float3 localPos, float3 instancePivot, float veinDistance)
{
    // PHASE 1: Broad Branch Flapping (Medium Frequency, High Drag Resistance)
    // Palmate lobes capture huge wind columns, causing broad roll and pitch behaviors
    float canopyPhase = instancePivot.x * 0.08f + instancePivot.z * 0.05f;
    float flapClock = g_EngineDeltaTime * g_MacroWaveFrequency.x + canopyPhase;
    
    float3 macroFlapOffset = g_GaleWindVector * (sin(flapClock) * cos(flapClock * 0.52f) * g_WindVelocitySpeed * 0.48f);

    // PHASE 2: INDIVIDUAL LOBE REVOLUTION (High Frequency Torsional Rolling)
    // Individual lobe edges ripple under aerodynamic drag profiles
    float rollClock = g_EngineDeltaTime * 12.5f + localPos.x * 2.0f;
    float torsionalRipple = sin(rollClock) * cos(rollClock * 1.2f) * g_WindVelocitySpeed;
    
    float3 crossWindAxis = cross(g_GaleWindVector, float3(0.0f, 1.0f, 0.0f));
    float3 microLobeRipple = crossWindAxis * (torsionalRipple * 0.18f);

    // Leaf tip regions react strongly to drag vectors, attachment stems remain locked in space
    float palmateDragWeight = saturate(veinDistance * 1.8f);

    return (macroFlapOffset + microLobeRipple) * palmateDragWeight;
}

#endif // SUGAR_MAPLE_PHENOLOGY_INCLUDED
