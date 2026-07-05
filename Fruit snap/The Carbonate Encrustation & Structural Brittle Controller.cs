using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_GalenaDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct CrystalLatticeNode
    {
        public Vector3 localPosition;
        public float leadSulfideDensity;    // Core galena mass remaining (100 down to 0)
        public float cerussiteCrustVolume;  // White carbonate accumulation level (0 to 100)
        public float localizedRainExposure; // Top-facing areas crust faster due to water interaction
        public bool isCubeCornerShattered;
    }

    [Header("Chemical Weathering Metrics")]
    [Range(0f, 1f)] public float aggregatedDecayProgress = 0f;
    [SerializeField] private int gridArrayResolution = 10;
    [SerializeField] private float environmentalCarbonDioxideScale = 1.2f;

    [Header("Mechanical Structural Breakdown")]
    [Tooltip("The mechanical strength of the brittle crystal before it splits under impact.")]
    [SerializeField] private float pristineCleavageStrength = 400.0f;
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem cerussiteWhiteDustFX; // Chalky white carbonate flakes
    [SerializeField] private ParticleSystem heavyMineralShardsFX;  // Heavy metallic gray shards

    private List<CrystalLatticeNode> crystalMatrix = new List<CrystalLatticeNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isObjectCompletelyCrumbled = false;

    // GPU String Property Lookup Caching
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicStructuralIntegrity = pristineCleavageStrength;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Generate the 3D internal cubic structural grid
        InitializeCubicStructuralGrid();
    }

    void Update()
    {
        if (isObjectCompletelyCrumbled) return;

        SimulateSupergeneOxidationPipeline();
    }

    private void InitializeCubicStructuralGrid()
    {
        Bounds bounds = GetComponent<Collider>() != null ? GetComponent<Collider>().bounds : new Bounds(transform.position, Vector3.one);
        Vector3 stepSpacing = bounds.size / (float)gridArrayResolution;

        for (int x = 0; x < gridArrayResolution; x++)
        {
            for (int y = 0; y < gridArrayResolution; y++)
            {
                Vector3 localPos = new Vector3(
                    (-bounds.extents.x) + (x * stepSpacing.x),
                    (-bounds.extents.y) + (y * stepSpacing.y),
                    (-bounds.extents.z) + (Random.value * bounds.size.z)
                );

                // Top nodes catch rainwater directly, heavily accelerating the carbonate transformation
                float skyExposure = Mathf.Clamp01((localPos.y + bounds.extents.y) / bounds.size.y);

                CrystalLatticeNode node = new CrystalLatticeNode
                {
                    localPosition = localPos,
                    leadSulfideDensity = 100.0f,
                    cerussiteCrustVolume = 0f,
                    localizedRainExposure = skyExposure * environmentalCarbonDioxideScale,
                    isCubeCornerShattered = false
                };
                crystalMatrix.Add(node);
            }
        }
    }

    private void SimulateSupergeneOxidationPipeline()
    {
        int crustedNodesCount = 0;
        float totalRemainingMass = 0f;

        for (int i = 0; i < crystalMatrix.Count; i++)
        {
            CrystalLatticeNode node = crystalMatrix[i];

            if (node.leadSulfideDensity > 0f)
            {
                // Stage 1: Fast initial passivation into anglesite, stripping mirror luster
                float weatheringVelocity = Time.deltaTime * node.localizedRainExposure * 2.0f;
                node.leadSulfideDensity = Mathf.Max(0f, node.leadSulfideDensity - weatheringVelocity);

                // Stage 2: Carbon dioxide interaction builds up brittle white cerussite crusts
                if (node.leadSulfideDensity < 70f)
                {
                    node.cerussiteCrustVolume += Time.deltaTime * node.localizedRainExposure * 4.5f;
                }

                // Stage 3: Corner cleavage structural failure check
                if (node.cerussiteCrustVolume >= 65f)
                {
                    node.isCubeCornerShattered = true;
                }
            }

            if (node.isCubeCornerShattered) crustedNodesCount++;
            totalRemainingMass += node.leadSulfideDensity;

            crystalMatrix[i] = node; // Sync grid modifications back to the heap array
        }

        // Pass calculated progress data directly down to the HLSL vertex/pixel shaders
        aggregatedDecayProgress = (float)crustedNodesCount / crystalMatrix.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, aggregatedDecayProgress);
        }

        // Mechanical Structural Degradation
        // Cerussite is incredibly brittle and chalky compared to solid galena metal blocks.
        float normalizedDensityFactor = totalRemainingMass / (crystalMatrix.Count * 100f);
        dynamicStructuralIntegrity = Mathf.Lerp(pristineCleavageStrength * 0.08f, pristineCleavageStrength, normalizedDensityFactor);

        // Physics Realism Tuning: 
        // Real galena is extraordinarily heavy (Density ~ 7.6 g/cm³). As it rots into a porous carbonate shell, 
        // it loses substantial bulk density. We smoothly step down the physics mass profile.
        if (rb != null)
        {
            rb.mass = Mathf.Lerp(45.0f, 28.0f, aggregatedDecayProgress);
            rb.angularDrag = Mathf.Lerp(0.1f, 0.7f, aggregatedDecayProgress); // Rough crust resists rolling
        }

        // If the white carbonate shell compromises more than 60% of the crystal bonds, 
        // the entire cubic structure shatters into shards under its own remaining weight.
        if (aggregatedDecayProgress >= 0.60f)
        {
            ExecuteBrittleCubicCollapse();
        }
    }

    /// <summary>
    /// Processes sudden localized mechanical strikes (Mining picks, blunt hammer hits, bullet impacts)
    /// </summary>
    public void RegisterKineticCleavageShock(Vector3 impactWorldPoint, float rawForceGigaNewtons)
    {
        if (isObjectCompletelyCrumbled) return;

        Vector3 localStrikePoint = transform.InverseTransformPoint(impactWorldPoint);

        for (int i = 0; i < crystalMatrix.Count; i++)
        {
            CrystalLatticeNode node = crystalMatrix[i];
            float physicalRange = Vector3.Distance(localStrikePoint, node.localPosition);

            if (physicalRange < 1.2f)
            {
                // A crusted, weathered node loses its elastic defense buffer and fractures instantly
                float crustBrittlenessFactor = 1.0f + (node.cerussiteCrustVolume * 0.06f);
                node.leadSulfideDensity = Mathf.Max(0f, node.leadSulfideDensity - (rawForceGigaNewtons / (physicalRange + 0.1f)) * crustBrittlenessFactor);

                if (node.leadSulfideDensity <= 30f)
                {
                    node.isCubeCornerShattered = true;
                }
                crystalMatrix[i] = node;
            }
        }
    }

    private void ExecuteBrittleCubicCollapse()
    {
        isObjectCompletelyCrumbled = true;
        StopAllCoroutines();

        // Release the entity components completely into dynamic physics gravity assets
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply an abrupt, snapping torque rotation profile
        rb.AddTorque(Random.onUnitSphere * 50f, ForceMode.Impulse);

        // Spawn a dual-layered particle splash: heavy gray lead shards mixed with clouds of white carbonate dust
        if (heavyMineralShardsFX != null)
        {
            ParticleSystem shards = Instantiate(heavyMineralShardsFX, transform.position, Quaternion.identity);
            Destroy(shards.gameObject, 4.0f);
        }
        if (cerussiteWhiteDustFX != null)
        {
            ParticleSystem cloud = Instantiate(cerussiteWhiteDustFX, transform.position, Quaternion.identity);
            var main = cloud.main;
            main.startSizeMultiplier = 3.8f;
            Destroy(cloud.gameObject, 5.0f);
        }

        Debug.Log($"[CLEAVAGE SHATTER CRITICAL] Galena cubic crystal lattice disintegrated into white cerussite shell at {aggregatedDecayProgress * 100f}% decay state.");
        Destroy(gameObject, 0.15f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
