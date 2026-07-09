#ifndef CORAL_ASPEN_WIND_COMMON_INCLUDED
#define CORAL_ASPEN_WIND_COMMON_INCLUDED

// Constant Buffer bindings mapping real-time wind fields from engine weather system
cbuffer WindSimulationBuffer : register(b2)
{
    float3 g_GlobalWindDirection;
    float  g_WindVelocityIntensity;
    float  g_EngineAbsoluteTime;
    float2 g_WindTurbulenceFrequency; 
};

struct VertexInput
{
    float3 Position       : POSITION;
    float3 Normal         : NORMAL;
    float4 Tangent        : TANGENT;
    float2 UV             : TEXCOORD0;
    float  QuakingWeight  : BLENDWEIGHT0; // Alpha weight from procedural generation
    float  BranchID       : BLENDINDICES0; // Keeps leaf clusters grouped on same branch together
};

struct VertexOutput
{
    float4 ProjectedPosition : SV_POSITION;
    float3 WorldNormal       : NORMAL;
    float2 TexCoord          : TEXCOORD0;
};

// Fluid Aerodynamic Wind Projection calculation
float3 CalculateAAAQuakingAspenDisplacement(float3 worldPos, float quakingWeight, float branchID)
{
    // Phase 1: Branch/Trunk Macro-Sway (Low Frequency, Complex Phase Shifts)
    float branchPhaseOffset = branchID * 0.423f;
    float macroSwayPhase = g_EngineAbsoluteTime * g_WindTurbulenceFrequency.x + (worldPos.y * 0.08f) + branchPhaseOffset;
    
    float3 macroSwayDisplacement = g_GlobalWindDirection * (sin(macroSwayPhase) * cos(macroSwayPhase * 0.41f) * (g_WindVelocityIntensity * 0.28f));

    // Phase 2: THE FLAT-PETIOLE FLUTTER EQUATIONS
    // Prime numbers mismatch logic avoids repetitive looping patterns across massive clonal forest landscapes
    float highFreqTimeA = g_EngineAbsoluteTime * (27.85f + sin(branchID));
    float highFreqTimeB = g_EngineAbsoluteTime * (33.14f + cos(branchID));

    float microQuakeSignalA = sin(highFreqTimeA + (worldPos.x * 1.15f));
    float microQuakeSignalB = cos(highFreqTimeB + (worldPos.z * 1.42f));

    // Combine dual waves into a rapid fluttering frequency simulation
    float combinedQuakingVelocity = (microQuakeSignalA * microQuakeSignalB) * g_WindVelocityIntensity;

    // Cross-product evaluation vectors to establish fluttering movement perpendicular to leaf orientation
    float3 crossWindAxis = cross(g_GlobalWindDirection, float3(0.0f, 1.0f, 0.0f));
    
    float3 microQuakeDisplacement = (crossWindAxis * (combinedQuakingVelocity * 1.4f)) + 
                                    (float3(0.0f, 1.0f, 0.0f) * (abs(combinedQuakingVelocity) * 0.35f));

    // Combine pipelines weighted completely by the leaf vertex's vertex alpha channel mask
    // Stiff inner stem stays at zero displacement, outer tip quakes heavily
    return macroSwayDisplacement + (microQuakeDisplacement * quakingWeight);
}

#endif // CORAL_ASPEN_WIND_COMMON_INCLUDED
