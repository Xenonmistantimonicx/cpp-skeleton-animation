#ifndef GINKGO_BILOBA_WIND_PIPELINE_INCLUDED
#define GINKGO_BILOBA_WIND_PIPELINE_INCLUDED

cbuffer GinkgoPhenologyBuffer : register(b3)
{
    float3 g_WindVectorDirection;
    float  g_WindForceVelocity;
    float  g_AbsoluteGlobalTime;
    float  g_ChlorophyllFadeProgress; // 0.0 (Pure Green Jade) -> 1.0 (Pure Solid Gold)
    float  g_GinkgoDropTimeline;       // Global trigger threshold controlling catastrophic leaf detaching
};

struct VertexInputLeaf
{
    float3 MeshPosition   : POSITION;
    float3 Normal         : NORMAL;
    float2 UV             : TEXCOORD0;
    float3 InstancePivot  : TEXCOORD1; // Root coordinate of individual fan leaf instance
    float  SpurID         : BLENDINDICES0;
};

struct VertexOutputLeaf
{
    float4 SVPosition     : SV_POSITION;
    float4 FragColor      : COLOR0;
    float2 TexCoord       : TEXCOORD0;
};

// Continuous Linear Blend Matrix
float4 ComputeGinkgoPhenologyColor(float fadeProgress)
{
    float4 jadeGreen = float4(0.06f, 0.36f, 0.24f, 1.0f);
    float4 neonGold  = float4(0.91f, 0.70f, 0.03f, 1.0f);
    return lerp(jadeGreen, neonGold, fadeProgress);
}

float3 ComputeAdvancedGinkgoVertexDisplacement(float3 vertexPos, float3 instancePivot, float spurID)
{
    // PHASE 1: Macro Fan Sail Oscillation (Low Frequency, Broad Rotational Swing)
    float structuralPhase = spurID * 0.712f;
    float sailOscillation = sin(g_AbsoluteGlobalTime * 1.5f + instancePivot.y * 0.12f + structuralPhase);
    
    float3 macroSway = g_WindVectorDirection * (sailOscillation * (g_WindForceVelocity * 0.42f));

    // PHASE 2: Aerodynamic Edge Fluttering (Medium Frequency Side-To-Side Shimmer)
    float flutterPhaseX = sin(g_AbsoluteGlobalTime * 14.25f + instancePivot.x * 2.1f);
    float flutterPhaseZ = cos(g_AbsoluteGlobalTime * 17.82f + instancePivot.z * 1.9f);
    float combinedFlutter = flutterPhaseX * flutterPhaseZ * g_WindForceVelocity;

    float3 perpendicularWindAxis = cross(g_WindVectorDirection, float3(0.0f, 1.0f, 0.0f));
    float3 microFlutter = perpendicularWindAxis * (combinedFlutter * 0.65f);

    // PHASE 3: THE SYNCHRONOUS CATASTROPHIC CASCADE SYSTEM
    // Overrides localized wind variables when the engine calls for the seasonal drop
    float3 gravityFallDisplacement = float3(0.0f, 0.0f, 0.0f);
    
    if (g_GinkgoDropTimeline > 0.01f)
    {
        // Randomized procedural threshold calculation based on instance placement
        float leafReleaseThreshold = frac(instancePivot.x * 45.32f + instancePivot.z * 12.81f);
        
        if (g_GinkgoDropTimeline > leafReleaseThreshold)
        {
            float fallDuration = (g_GinkgoDropTimeline - leafReleaseThreshold);
            
            // Accelerate velocity downwards based on continuous gravity mechanics
            gravityFallDisplacement.y -= (fallDuration * fallDuration * 9.81f * 2.5f);
            
            // Add air-friction gliding drift to the falling fan profile
            gravityFallDisplacement.x += sin(g_AbsoluteGlobalTime * 6.0f + instancePivot.y) * (fallDuration * 4.0f);
            gravityFallDisplacement.z += cos(g_AbsoluteGlobalTime * 5.2f + instancePivot.x) * (fallDuration * 4.0f);
            
            // Neutralize parent tree macro sways once leaf detaches from bark system
            macroSway = float3(0.0f, 0.0f, 0.0f);
            microFlutter = float3(0.0f, 0.0f, 0.0f);
        }
    }

    // Outer scalloped fan tips take the full weight vector, base attached stem points stay locked
    float leafVertexWeight = vertexPos.y; 
    
    return macroSway + (microFlutter * leafVertexWeight) + gravityFallDisplacement;
}

#endif // GINKGO_BILOBA_WIND_PIPELINE_INCLUDED
