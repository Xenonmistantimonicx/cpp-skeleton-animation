#ifndef DRAGONS_BLOOD_TREE_SHADING_INCLUDED
#define DRAGONS_BLOOD_TREE_SHADING_INCLUDED

cbuffer SocotraAtmosphereParameters : register(b4)
{
    float3 g_SiroccoWindVector;
    float  g_WindVelocitySpeed;
    float  g_DeltaRunningTime;
    float  g_DynamicInjurySecretion; // Material uniform mapped to tree health system (0.0 -> Normal, 1.0 -> Heavy Bleeding)
    float4 g_DynamicImpactCoordinate; // World position collision tracker mapping physical damage areas
};

struct VertexInputSwordLeaf
{
    float3 BasePosition   : POSITION;
    float3 GeometricNormal : NORMAL;
    float2 TexCoord       : TEXCOORD0;
    float3 LeafInstanceAnchor : TEXCOORD1; 
    float  StructuralTier  : BLENDINDICES0;
};

struct VertexOutputSwordLeaf
{
    float4 SVPosition     : SV_POSITION;
    float3 WorldPosition  : TEXCOORD0;
    float4 ResinVibrancy  : COLOR0;
};

// Compiles highly specialized Cinnabar Red resin color maps dynamically based on impact parameters
float4 CalculateCinnabarFluidProfile(float3 worldPosition, float bleedWeight)
{
    float3 charcoalBarkTone = float3(0.24f, 0.22f, 0.21f);
    float3 brightResinBlood = float3(0.78f, 0.03f, 0.03f);

    // Compute proximity fields against dynamic engine collision spheres
    float distanceToInjury = distance(worldPosition, g_DynamicImpactCoordinate.xyz);
    float localStressTrigger = saturate(1.0f - (distanceToInjury * 0.15f)) * g_DynamicInjurySecretion;

    float dynamicMixAlpha = saturate(bleedWeight * localStressTrigger);
    return float4(lerp(charcoalBarkTone, brightResinBlood, dynamicMixAlpha), 1.0f);
}

float3 ComputeAdvancedDragonsBloodDisplacement(float3 vertexPos, float3 instanceAnchor, float tier)
{
    // STEP 1: Micro-Vibrational Shield Shimmer (High Frequency, Tiny Linear Deviation)
    // Rigid sword leaves do not bend; they transfer energy via fast structural trembling
    float structuralVibrFreq = g_DeltaRunningTime * (22.50f + (tier * 1.5f));
    float shakeComponent = sin(structuralVibrFreq + instanceAnchor.x * 3.5f) * cos(structuralVibrFreq * 0.85f);

    // High altitude canopy perimeter multiplier dampening logic
    float canopyHeightDampener = saturate(tier / 6.0f); 
    float3 leafTrembleOffset = g_SiroccoWindVector * (shakeComponent * g_WindVelocitySpeed * 0.12f * canopyHeightDampener);

    // STEP 2: METABOLIC CINNABAR RESIN OOZING SYSTEM
    // Computes downward vertical fluid flow vector shifts on wounded vertex groups
    float3 dynamicFluidDrip = float3(0.0f, 0.0f, 0.0f);

    if (g_DynamicInjurySecretion > 0.02f)
    {
        float distanceCheck = distance(instanceAnchor, g_DynamicImpactCoordinate.xyz);
        if (distanceCheck < 4.0f) // Localized damage containment field radius
        {
            // Compute thick viscous descent logic using smooth step modulo mathematics
            float fluidDescentVelocity = g_DeltaRunningTime * 0.45f;
            float continuousOozeCycle = frac(fluidDescentVelocity + (instanceAnchor.z * 0.5f));
            
            // Deflect vertex normal channels down along gravity axis to simulate dripping fluid bags
            dynamicFluidDrip.y -= (continuousOozeCycle * 0.85f * g_DynamicInjurySecretion);
            dynamicFluidDrip.xz += (vertexPos.xz * 0.15f * sin(g_DeltaRunningTime * 4.0f)); // Viscous swelling animation
        }
    }

    // Leaf tip point index scales displacement limits, base collar remains anchored to branch mesh
    float vertexDilationFactor = saturate(vertexPos.y * 1.5f);

    return (leafTrembleOffset * vertexDilationFactor) + dynamicFluidDrip;
}

#endif // DRAGONS_BLOOD_TREE_SHADING_INCLUDED
