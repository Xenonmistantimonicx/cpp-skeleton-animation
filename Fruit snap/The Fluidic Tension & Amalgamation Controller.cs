using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_MercuryFluidController : MonoBehaviour
{
    [System.Serializable]
    public struct FluidCohesionNode
    {
        public Vector3 localPosition;
        public float fluidBeadingTension;    // Local surface tension curve value (High down to zero)
        public float amalgamAlloyGrowth;    // Volume accumulation of stiff silver-grey mush skin
        public float calcinedOxideVolume;    // Local transformation progress into bright red powder
        public bool isNodeSolidified;
    }

    [Header("Fluid Cohesion Dynamics")]
    [Range(0f, 1f)] public float integratedPhaseProgress = 0f;
    [SerializeField] private int fluidLatticeResolution = 10;
    [SerializeField] private float thermalExposureRate = 1.6f; // Ambient thermal/reaction velocity multiplier

    [Header("Mechanical Rheology Yield")]
    [Tooltip("The dynamic fluid surface tension cohesion rating before oxide crystallization solidifies the asset.")]
    [SerializeField] private float nativeSurfaceTensionmNM = 485.5f; 
    private float dynamicViscosityCohesion;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem toxicMercuryVaporFX;    // Invisible/faint iridescent heavy thermal haze fumes
    [SerializeField] private ParticleSystem redOxidePowderFX;      // Fine, bright red-orange crystalline dust trails

    private List<FluidCohesionNode> liquidLattice = new List<FluidCohesionNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isPoolCompletelySolidified = false;

    // Fast GPU Parameter Cache Hashes
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");
    private static readonly int FluidWaveTimeID = Shader.PropertyToID("_FluidWaveTime");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicViscosityCohesion = nativeSurfaceTensionmNM;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Generate the 3D internal metallurgical fluid tracking layout
        InitializeFluidLattice();
    }

    void Update()
    {
        // Drive fluid vertex waves inside the GPU material instance pipeline
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(FluidWaveTimeID, Time.time);
        }

        if (isPoolCompletelySolidified) return;

        SimulateMercuryPhaseTransformation();
    }

    private void InitializeFluidLattice()
    {
        Bounds bounds = GetComponent<Collider>() != null ? GetComponent<Collider>().bounds : new Bounds(transform.position, Vector3.one);
        Vector3 stepSpacing = bounds.size / (float)fluidLatticeResolution;

        for (int x = 0; x < fluidLatticeResolution; x++)
        {
            for (int y = 0; y < fluidLatticeResolution; y++)
            {
                Vector3 localPoint = new Vector3(
                    (-bounds.extents.x) + (x * stepSpacing.x),
                    (-bounds.extents.y) + (y * stepSpacing.y),
                    (-bounds.extents.z) + (Random.value * bounds.size.z)
                );

                // Exterior boundaries alter quickly into amalgam/oxide skins; core holds heavy fluid volume
                float surfaceProximityBias = Vector3.Distance(Vector3.zero, localPoint) / bounds.extents.magnitude;

                FluidCohesionNode node = new FluidCohesionNode
                {
                    localPosition = localPoint,
                    fluidBeadingTension = nativeSurfaceTensionmNM,
                    amalgamAlloyGrowth = 0f,
                    calcinedOxideVolume = 0f,
                    isNodeSolidified = false
                };
                liquidLattice.Add(node);
            }
        }
    }

    private void SimulateMercuryPhaseTransformation()
    {
        int solidifiedNodesCount = 0;
        float totalOxideVolume = 0f;

        for (int i = 0; i < liquidLattice.Count; i++)
        {
            FluidCohesionNode node = liquidLattice[i];

            // Stage 1: Amalgamation attacks outer fluid boundaries, dropping liquid tension properties
            node.amalgamAlloyGrowth += Time.deltaTime * thermalExposureRate * 2.8f;
            node.fluidBeadingTension = Mathf.Max(0f, nativeSurfaceTensionmNM - (node.amalgamAlloyGrowth * 3.5f));

            // Stage 2: Under sustained thermal oxidation, amalgam converts completely into solid red mercuric oxide
            if (node.amalgamAlloyGrowth >= 60.0f)
            {
                node.calcinedOxideVolume += Time.deltaTime * thermalExposureRate * 4.2f;
            }

            // Stage 3: Core phase verification converting liquid node to solid crystal node
            if (node.calcinedOxideVolume >= 90.0f)
            {
                node.isNodeSolidified = true;
            }

            if (node.isNodeSolidified) solidifiedNodesCount++;
            totalOxideVolume += node.calcinedOxideVolume;

            liquidLattice[i] = node; // Commit updated node values back to index stack loop
        }

        // Map calculated progress timelines directly straight down to the PBR GPU material properties
        integratedPhaseProgress = (float)solidifiedNodesCount / liquidLattice.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, integratedPhaseProgress);
        }

        // Rheology Viscosity Adjustments
        // As liquid mirror beads solidify into dry, crumbly oxide shells, cohesive fluid motion ceases.
        float normalizedOxidePct = totalOxideVolume / (liquidLattice.Count * 100f);
        dynamicViscosityCohesion = Mathf.Lerp(nativeSurfaceTensionmNM, nativeSurfaceTensionmNM * 0.01f, normalizedOxidePct);

        if (rb != null)
        {
            // Mercury is incredibly dense (Density ~ 13.53 g/cm³); mass reduces as calcination splits components into porous dry crusts
            rb.mass = Mathf.Lerp(140.0f, 90.0f, integratedPhaseProgress);
            
            // Simulates liquid fluid drag freezing into static, stiff rigid behaviors
            rb.linearDamping = Mathf.Lerp(0.05f, 4.0f, integratedPhaseProgress);
            rb.angularDamping = Mathf.Lerp(0.05f, 5.0f, integratedPhaseProgress);
        }

        // Vapor Release System
        if (integratedPhaseProgress > 0.1f && toxicMercuryVaporFX != null && !toxicMercuryVaporFX.isPlaying)
        {
            toxicMercuryVaporFX.Play();
        }

        // If advanced crystallization completely solidifies more than 70% of the internal tracking nodes,
        // the remaining component behaves entirely as a brittle, dry solid ceramic shell asset.
        if (integratedPhaseProgress >= 0.70f)
        {
            ExecuteCompleteCrystallineSolidification();
        }
    }

    /// <summary>
    /// Processes intense localized kinetic disruptions (Blunt tool impacts, slashing strikes, drop shocks)
    /// </summary>
    public void RegisterKineticDeformationStrike(Vector3 contactWorldPoint, float forceInputJoules)
    {
        if (isPoolCompletelySolidified) return;

        Vector3 localImpact = transform.InverseTransformPoint(contactWorldPoint);

        for (int i = 0; i < liquidLattice.Count; i++)
        {
            FluidCohesionNode node = liquidLattice[i];
            float interactionRange = Vector3.Distance(localImpact, node.localPosition);

            if (interactionRange < 1.4f)
            {
                // If pristine fluid, kinetic impacts shatter the droplet into smaller high-tension satellite spheres.
                // If calcined, it cracks open the brittle red oxide shell layer.
                if (node.calcinedOxideVolume > 40.0f)
                {
                    node.calcinedOxideVolume = Mathf.Min(100f, node.calcinedOxideVolume + (forceInputJoules * 0.5f));
                    if (redOxidePowderFX != null) redOxidePowderFX.Emit(4);
                }
                else
                {
                    // High-tension ripple propagation vector mapping
                    node.amalgamAlloyGrowth = Mathf.Max(0f, node.amalgamAlloyGrowth - 15.0f); // Impact disrupts fragile surface crusts
                }

                liquidLattice[i] = node;
            }
        }
    }

    private void ExecuteCompleteCrystallineSolidification()
    {
        isPoolCompletelySolidified = true;
        StopAllCoroutines();

        // Release the asset physics loops out of rolling liquid behavior and into crisp rigid impact mechanics
        if (rb != null)
        {
            rb.isKinematic = false;
            rb.useGravity = true;
            rb.linearDamping = 0.1f; // Resets fluid drag values back to crisp solid dynamics
            rb.angularDamping = 0.05f;
        }

        if (redOxidePowderFX != null)
        {
            ParticleSystem explosionCloud = Instantiate(redOxidePowderFX, transform.position, Quaternion.identity);
            var mainMod = explosionCloud.main;
            mainMod.startSizeMultiplier = 3.2f; // Large burst cloud of fine, bright red-orange mercuric oxide dust chunks
            Destroy(explosionCloud.gameObject, 4.0f);
        }

        Debug.Log($"[PHASE PHASE COMPLETED] Fluid pool completely solidified into rigid, brittle crystalline mercuric oxide at {integratedPhaseProgress * 100f}% oxidation index.");
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
