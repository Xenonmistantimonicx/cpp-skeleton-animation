using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_MolybdenumDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct ThermalPhaseNode
    {
        public Vector3 positionWS;
        public float localizedTemperatureC;  // Node heat profile (25°C to 900°C sublimation point)
        public float needleGrowthVolume;     // Accumulation level of crystalline trioxide strands
        public float slagLiquefactionLevel;  // Progress of melting phase transformation
        public bool isMassEvaporated;
    }

    [Header("Thermo-Chemical Profiles")]
    [Range(0f, 1f)] public float integratedDecayProgress = 0f;
    [SerializeField] private int latticeMatrixResolution = 10;
    [SerializeField] private float environmentalHeatLoad = 750.0f; // Ambient thermal exposure scaling

    [Header("Mechanical Structural Yield")]
    [Tooltip("The initial structural yield capacity of the refractory molybdenum component.")]
    [SerializeField] private float pristineTensileStrength = 550.0f; // Simulated MegaPascals
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem trioxideVaporSmokeFX; // Dense, pale smoky trioxide gas clouds
    [SerializeField] private ParticleSystem moltenSlagDripFX;     // Dripping liquid slag droplets

    private List<ThermalPhaseNode> thermalMatrix = new List<RadiolyticNode> colorShiftArray = new List<ThermalPhaseNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isStructureCompletelyLiquidated = false;

    // Fast GPU Parameter Cache Hashes
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicStructuralIntegrity = pristineTensileStrength;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Procedurally populate the 3D internal metallurgical tracking layout
        InitializeThermalLattice();
    }

    void Update()
    {
        if (isStructureCompletelyLiquidated) return;

        SimulateSublimationPipeline();
    }

    private void InitializeThermalLattice()
    {
        Bounds bounds = GetComponent<Collider>() != null ? GetComponent<Collider>().bounds : new Bounds(transform.position, Vector3.one);
        Vector3 nodeSpacing = bounds.size / (float)latticeMatrixResolution;

        for (int x = 0; x < latticeMatrixResolution; x++)
        {
            for (int y = 0; y < latticeMatrixResolution; y++)
            {
                Vector3 localPoint = new Vector3(
                    (-bounds.extents.x) + (x * nodeSpacing.x),
                    (-bounds.extents.y) + (y * nodeSpacing.y),
                    (-bounds.extents.z) + (Random.value * bounds.size.z)
                );

                // External nodes interact with engine plumes or heat loops instantly; core traps residual heat
                float surfaceDistance = Vector3.Distance(Vector3.zero, localPoint) / bounds.extents.magnitude;

                ThermalPhaseNode node = new ThermalPhaseNode
                {
                    positionWS = transform.TransformPoint(localPoint),
                    localizedTemperatureC = Mathf.Lerp(25.0f, environmentalHeatLoad, surfaceDistance),
                    needleGrowthVolume = 0f,
                    slagLiquefactionLevel = 0f,
                    isMassEvaporated = false
                };
                thermalMatrix.Add(node);
            }
        }
    }

    private void SimulateSublimationPipeline()
    {
        int evaporatedNodesCount = 0;
        float totalCrystallineVolume = 0f;
        float totalSlagVolume = 0f;

        for (int i = 0; i < thermalMatrix.Count; i++)
        {
            ThermalPhaseNode node = thermalMatrix[i];

            // Stage 1: Heat crosses 600°C, growing the massive yellow needle-like MoO3 crust
            if (node.localizedTemperatureC >= 600.0f && node.slagLiquefactionLevel <= 0f)
            {
                node.needleGrowthVolume += Time.deltaTime * 4.5f;
            }

            // Stage 2: Above 795°C, the needles undergo liquefaction into a molten green glass slag
            if (node.localizedTemperatureC >= 795.0f)
            {
                if (node.needleGrowthVolume > 30f)
                {
                    node.slagLiquefactionLevel += Time.deltaTime * 3.8f;
                }
                
                // Stage 3: Liquid components evaporate directly into gas (Sublimation)
                if (node.slagLiquefactionLevel >= 85f)
                {
                    node.isMassEvaporated = true;
                }
            }

            if (node.isMassEvaporated) evaporatedNodesCount++;
            totalCrystallineVolume += node.needleGrowthVolume;
            totalSlagVolume += node.slagLiquefactionLevel;

            thermalMatrix[i] = node; // Commit node modifications back to heap stack array
        }

        // Map unified progress metrics straight to the PBR GPU shader properties
        float normalizedNeedles = totalCrystallineVolume / (thermalMatrix.Count * 100f);
        float normalizedSlag = totalSlagVolume / (thermalMatrix.Count * 100f);
        
        // Master curve driving transitions from pristine (0) to needle clusters (0.5) to vaporous melt (1.0)
        integratedDecayProgress = (normalizedNeedles * 0.5f) + (normalizedSlag * 0.5f);
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, integratedDecayProgress);
        }

        // Mechanical Structural Engineering Adjustments
        // Solid molybdenum stands firm until liquefaction and sublimation evaporate the structural framing core away.
        dynamicStructuralIntegrity = Mathf.Lerp(pristineTensileStrength, pristineTensileStrength * 0.01f, normalizedSlag);

        // Physics Tuning: Molybdenum is exceptionally dense (Density ~ 10.2 g/cm³).
        // As sublimation converts the core mass straight into escaping airborne gas, physics mass drops drastically.
        if (rb != null)
        {
            rb.mass = Mathf.Lerp(75.0f, 20.0f, normalizedSlag);
        }

        // Vapor Release FX Management
        if (normalizedSlag > 0.2f && trioxideVaporSmokeFX != null)
        {
            if (!trioxideVaporSmokeFX.isPlaying) trioxideVaporSmokeFX.Play();
            var mainModule = trioxideVaporSmokeFX.main;
            mainModule.startSizeMultiplier = Mathf.Lerp(1.0f, 3.5f, normalizedSlag); // Smoke columns grow denser
        }

        // If over 70% of the internal thermal nodes suffer sublimation evaporation failure, 
        // the remaining component experiences a structural structural failure.
        if (integratedDecayProgress >= 0.75f)
        {
            TriggerCatastrophicSublimationCollapse();
        }
    }

    private void TriggerCatastrophicSublimationCollapse()
    {
        isStructureCompletelyLiquidated = true;
        StopAllCoroutines();

        // Convert the asset into dynamic physics gravity entities
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply a sluggish structural buckling tumble velocity change profile
        rb.AddForce(Vector3.down * 4f, ForceMode.VelocityChange);
        rb.AddTorque(Random.onUnitSphere * 15f, ForceMode.Impulse);

        if (moltenSlagDripFX != null)
        {
            ParticleSystem dripSplash = Instantiate(moltenSlagDripFX, transform.position, Quaternion.identity);
            Destroy(dripSplash.gameObject, 4.0f);
        }

        Debug.Log($"[SUBLIMATION CRITICAL FAILURE] Molybdenum metal matrix structural mass evaporated away into vapor at {integratedDecayProgress * 100f}% total timeline progress.");
        Destroy(gameObject, 0.3f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
