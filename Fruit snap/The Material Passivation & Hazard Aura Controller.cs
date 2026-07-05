using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_PlutoniumMaterialController : MonoBehaviour
{
    [System.Serializable]
    public struct MaterialLatticeNode
    {
        public Vector3 positionWS;
        public float metalPristineHP;       // Remainder of unoxidized metal core (100 down to 0)
        public float looseOxideVolume;      // Accumulation level of flaking dioxide dust
        public float internalThermalLoad;    // Heat scaling driven by isotopic decay density
        public bool isSurfaceSpalled;
    }

    [Header("Material Exposure Metrics")]
    [Range(0f, 1f)] public float aggregatedDecayProgress = 0f;
    [SerializeField] private int gridMatrixResolution = 8;
    [SerializeField] private float airExposureVelocity = 1.4f;

    [Header("Dynamic Gameplay Hazard Aura")]
    public float currentHazardZoneRadius = 2.5f;
    [SerializeField] private SphereCollider gameplayHazardTrigger;
    public float dynamicGeigerClickRate = 2.0f;

    [Header("Mechanical Structural Cohesion")]
    [Tooltip("The initial physical compression limit of the metal asset before structural degradation.")]
    [SerializeField] private float metalYieldThreshold = 320.0f;
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem blackOxideDustFX; // Flaking dark oxide particulate clouds
    [SerializeField] private ParticleSystem thermalHeatHazeFX; // Distorted thermal convection particles

    private List<MaterialLatticeNode> materialLattice = new List<MaterialLatticeNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isAssetCompletelyPulverized = false;

    // Fast GPU Parameter String ID Hashes
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicStructuralIntegrity = metalYieldThreshold;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Initialize the 3D grid layout across bounding dimensions
        InitializeMaterialGrid();
    }

    void Update()
    {
        if (isAssetCompletelyPulverized) return;

        SimulateMaterialOxidationPipeline();
    }

    private void InitializeMaterialGrid()
    {
        Bounds bounds = GetComponent<Collider>() != null ? GetComponent<Collider>().bounds : new Bounds(transform.position, Vector3.one);
        Vector3 stepSpacing = bounds.size / (float)gridMatrixResolution;

        for (int x = 0; x < gridMatrixResolution; x++)
        {
            for (int y = 0; y < gridMatrixResolution; y++)
            {
                Vector3 localPoint = new Vector3(
                    (-bounds.extents.x) + (x * stepSpacing.x),
                    (-bounds.extents.y) + (y * stepSpacing.y),
                    (-bounds.extents.z) + (Random.value * bounds.size.z)
                );

                // Exterior edge nodes oxidize rapidly; inner core nodes build up massive thermal loads
                float centerDistance = Vector3.Distance(Vector3.zero, localPoint) / bounds.extents.magnitude;
                float coreMassBias = Mathf.Clamp01(1.0f - centerDistance);

                MaterialLatticeNode node = new MaterialLatticeNode
                {
                    positionWS = transform.TransformPoint(localPoint),
                    metalPristineHP = 100.0f,
                    looseOxideVolume = 0f,
                    internalThermalLoad = 35.0f + (coreMassBias * 85.0f), // Self-heating properties scale toward core
                    isSurfaceSpalled = false
                };
                materialLattice.Add(node);
            }
        }
    }

    private void SimulateMaterialOxidationPipeline()
    {
        int oxidizedNodesCount = 0;
        float totalRemainingMetal = 0f;

        for (int i = 0; i < materialLattice.Count; i++)
        {
            MaterialLatticeNode node = materialLattice[i];

            if (node.metalPristineHP > 0f)
            {
                // Stage 1: Ambient moisture and oxygen attack the metal skin, stripping specular reflection
                float reactionRate = Time.deltaTime * airExposureVelocity * 1.8f;
                node.metalPristineHP = Mathf.Max(0f, node.metalPristineHP - reactionRate);

                // Stage 2: Loose, powdery dark dioxide builds up over weathered surfaces
                if (node.metalPristineHP < 65f)
                {
                    node.looseOxideVolume += Time.deltaTime * airExposureVelocity * 4.0f;
                }

                // Stage 3: Surface flaking and spalling check driven by internal thermal expansion
                if (node.looseOxideVolume >= 75f)
                {
                    node.isSurfaceSpalled = true;
                }
            }

            if (node.isSurfaceSpalled) oxidizedNodesCount++;
            totalRemainingMetal += node.metalPristineHP;

            materialLattice[i] = node; // Commit node changes back to heap memory stack
        }

        // Synchronize computed progress values directly with the rendering context
        aggregatedDecayProgress = (float)oxidizedNodesCount / materialLattice.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, aggregatedDecayProgress);
        }

        // Gameplay Hazard Mechanic: Expanding Airborne Dust Toxicity Ring
        // As the outer crust converts to loose, crumbling oxide flakes, the tracking zone grows
        currentHazardZoneRadius = Mathf.Lerp(2.5f, 6.0f, aggregatedDecayProgress);
        dynamicGeigerClickRate = Mathf.Lerp(2.0f, 40.0f, aggregatedDecayProgress);
        
        if (gameplayHazardTrigger != null)
        {
            gameplayHazardTrigger.radius = currentHazardZoneRadius;
        }

        // Physical Properties Modification Loop
        float structuralHealthPct = totalRemainingMetal / (materialLattice.Count * 100f);
        dynamicStructuralIntegrity = Mathf.Lerp(metalYieldThreshold * 0.05f, metalYieldThreshold, structuralHealthPct);

        if (rb != null)
        {
            // Pure unoxidized metal is incredibly dense; mass decreases smoothly as it turns into an airy, porous dust shell
            rb.mass = Mathf.Lerp(80.0f, 45.0f, aggregatedDecayProgress);
        }

        // Active Heat Haze Simulation Link
        if (aggregatedDecayProgress > 0.15f && thermalHeatHazeFX != null && !thermalHeatHazeFX.isPlaying)
        {
            thermalHeatHazeFX.Play();
        }

        // If advanced oxidation splits apart more than 65% of the crystalline node boundaries,
        // the remaining core structure breaks down into brittle dust chunks under physics loads.
        if (aggregatedDecayProgress >= 0.65f)
        {
            ExecuteCatastrophicMaterialCollapse();
        }
    }

    /// <summary>
    /// Processes localized mechanical shocks (Tool hits, physics drops, heavy impact waves)
    /// </summary>
    public void RegisterKineticShockVector(Vector3 contactWorldPoint, float inputForceJoules)
    {
        if (isAssetCompletelyPulverized) return;

        for (int i = 0; i < materialLattice.Count; i++)
        {
            MaterialLatticeNode node = materialLattice[i];
            float interactionDistance = Vector3.Distance(contactWorldPoint, node.positionWS);

            if (interactionDistance < 1.2f)
            {
                // A loose, crumbling powdery oxide shell features zero elastic defense and breaks instantly
                float fractureBrittlenessMultiplier = 1.0f + (node.looseOxideVolume * 0.05f);
                node.metalPristineHP = Mathf.Max(0f, node.metalPristineHP - (inputForceJoules / (interactionDistance + 0.1f)) * fractureBrittlenessMultiplier);

                if (node.metalPristineHP <= 20f)
                {
                    node.isSurfaceSpalled = true;
                }
                materialLattice[i] = node;
            }
        }
    }

    private void ExecuteCatastrophicMaterialCollapse()
    {
        isAssetCompletelyPulverized = true;
        StopAllCoroutines();

        // Release the entity layout into full rigid gravity physics simulations
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply a dull, sluggish tumble force profile matching soft, crumbling oxide crust fragments
        rb.AddTorque(Random.onUnitSphere * 20f, ForceMode.Impulse);

        if (blackOxideDustFX != null)
        {
            ParticleSystem collapseCloud = Instantiate(blackOxideDustFX, transform.position, Quaternion.identity);
            var main = collapseCloud.main;
            main.startSizeMultiplier = 4.5f; // Enormous cloud of fine, dark velvety-black dioxide dust particles
            Destroy(collapseCloud.gameObject, 5.0f);
        }

        Debug.Log($"[OXIDE SPALLING COMPLETE] Actinide metal structural framework crumbled entirely into loose oxide flakes at {aggregatedDecayProgress * 100f}% corrosion index.");
        Destroy(gameObject, 0.2f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
