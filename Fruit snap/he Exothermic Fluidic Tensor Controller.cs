using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_CesiumFluidController : MonoBehaviour
{
    [System.Serializable]
    public struct HypergolicFluidNode
    {
        public Vector3 localPosition;
        public float nodeTemperatureC;       // Thermal state tracking ambient melting & combustion spikes
        public float superoxideCrustVolume;   // Volume accumulation of orange ash scale
        public float deliquescentMeltLevel;   // Dissolution into pooling cesium hydroxide fluid
        public bool isNodeLiquidated;
    }

    [Header("Hypergolic Grid Settings")]
    [Range(0f, 1f)] public float integratedPhaseProgress = 0f;
    [SerializeField] private int latticeMatrixResolution = 10;
    [SerializeField] private float atmosphericHumidity = 1.5f; // Moisture velocity scaling factor

    [Header("Dynamic Rheology Yield")]
    [Tooltip("The initial yield structural strength of cesium. It is as soft as wax when cold.")]
    [SerializeField] private float pristineTensileStrength = 15.0f; 
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem lilacCombustionFlameFX; // Radiant lilac-violet chemical fire plumes
    [SerializeField] private ParticleSystem causticSizzlingSpitFX;    // Violent, popping liquid splash particles

    private List<HypergolicFluidNode> rheologyMatrix = new List<HypergolicFluidNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isAssetCompletelyLiquidated = false;

    // Fast GPU Parameter Cache Hashes
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");
    private static readonly int FluidWaveTimeID = Shader.PropertyToID("_FluidWaveTime");

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

        // Generate the 3D internal metallurgical tracking layout
        InitializeRheologyLattice();
    }

    void Update()
    {
        // Drive vertex fluid waveforms on the GPU material instance
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(FluidWaveTimeID, Time.time);
        }

        if (isAssetCompletelyLiquidated) return;

        SimulateAlkaliCombustionPipeline();
    }

    private void InitializeRheologyLattice()
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

                // Exterior boundaries melt and ignite immediately; core captures delayed heat propagation
                float surfaceProximity = Vector3.Distance(Vector3.zero, localPoint) / bounds.extents.magnitude;

                HypergolicFluidNode node = new HypergolicFluidNode
                {
                    localPosition = localPoint,
                    nodeTemperatureC = Mathf.Lerp(22.0f, 32.0f, surfaceProximity), // Instantly crosses 28.44°C melting point on surface
                    superoxideCrustVolume = 0f,
                    deliquescentMeltLevel = 0f,
                    isNodeLiquidated = false
                };
                rheologyMatrix.Add(node);
            }
        }
    }

    private void SimulateAlkaliCombustionPipeline()
    {
        int liquidatedNodesCount = 0;
        float totalMeltVolume = 0f;

        for (int i = 0; i < rheologyMatrix.Count; i++)
        {
            HypergolicFluidNode node = rheologyMatrix[i];

            // Stage 1: Melting point breach occurs, followed instantly by hypergolic humidity ignition heat spikes
            node.nodeTemperatureC += Time.deltaTime * atmosphericHumidity * 55.0f;

            // Stage 2: Above 100°C, burning golden liquid builds the porous orange superoxide crust
            if (node.nodeTemperatureC >= 100.0f && node.deliquescentMeltLevel <= 15f)
            {
                node.superoxideCrustVolume += Time.deltaTime * 7.5f;
            }

            // Stage 3: Extreme hygroscopic deliquescence converts the crust into pooling caustic liquid
            if (node.superoxideCrustVolume >= 45.0f)
            {
                node.deliquescentMeltLevel += Time.deltaTime * atmosphericHumidity * 5.8f;
            }

            if (node.deliquescentMeltLevel >= 95.0f)
            {
                node.isNodeLiquidated = true;
            }

            if (node.isNodeLiquidated) liquidatedNodesCount++;
            totalMeltVolume += node.deliquescentMeltLevel;

            rheologyMatrix[i] = node; // Sync struct modifications back to array index heap
        }

        // Map unified progress directly to the PBR GPU shader properties
        integratedPhaseProgress = (float)liquidatedNodesCount / rheologyMatrix.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, integratedPhaseProgress);
        }

        // Mechanical Structural Rigidity Adjustments
        dynamicStructuralIntegrity = Mathf.Lerp(pristineTensileStrength, pristineTensileStrength * 0.001f, integratedPhaseProgress);

        if (rb != null)
        {
            // Cesium has a modest density profile for a heavy element (1.93 g/cm³); mass reduces as it bursts and turns to pooling fluid
            rb.mass = Mathf.Lerp(45.0f, 15.0f, integratedPhaseProgress);
            
            // Adjust linear and angular damping to match the sluggish slumping of an amorphous liquid blob
            rb.linearDamping = Mathf.Lerp(0.05f, 7.0f, integratedPhaseProgress);
        }

        // Flame FX Pipeline Control
        if (integratedPhaseProgress > 0.05f && integratedPhaseProgress < 0.65f && lilacCombustionFlameFX != null)
        {
            if (!lilacCombustionFlameFX.isPlaying) lilacCombustionFlameFX.Play();
        }
        else if (integratedPhaseProgress >= 0.65f && lilacCombustionFlameFX != null && lilacCombustionFlameFX.isPlaying)
        {
            lilacCombustionFlameFX.Stop();
        }

        // If deliquescent melting completely liquidates more than 65% of the tracking network,
        // the remaining solid framework experiences absolute structural failure.
        if (integratedPhaseProgress >= 0.65f)
        {
            ExecuteAbsoluteChemicalCollapse();
        }
    }

    /// <summary>
    /// Processes physical impacts (Striking the melting liquid alkali matrix with weapons, tools, or bullet kinematics)
    /// </summary>
    public void RegisterKineticDeformationStrike(Vector3 contactWorldPoint, float forceInputJoules)
    {
        if (isAssetCompletelyLiquidated) return;

        Vector3 localImpact = transform.InverseTransformPoint(contactWorldPoint);

        // Volatize sizzling caustic bits outward under impact vectors
        if (causticSizzlingSpitFX != null)
        {
            causticSizzlingSpitFX.transform.position = contactWorldPoint;
            causticSizzlingSpitFX.Emit((int)(forceInputJoules * 0.6f));
        }

        for (int i = 0; i < rheologyMatrix.Count; i++)
        {
            HypergolicFluidNode node = rheologyMatrix[i];
            float range = Vector3.Distance(localImpact, node.localPosition);

            if (range < 1.6f)
            {
                // Physical kinetic disruption splits the fragile superoxide crust, exposing raw cesium core pools to moisture
                node.nodeTemperatureC += forceInputJoules * 4.5f;
                rheologyMatrix[i] = node;
            }
        }
    }

    private void ExecuteAbsoluteChemicalCollapse()
    {
        isAssetCompletelyLiquidated = true;
        StopAllCoroutines();

        // Flatten the object collider into a liquid level puddle mesh component
        if (rb != null)
        {
            rb.isKinematic = true; 
            GetComponent<Collider>().enabled = false;
        }

        if (causticSizzlingSpitFX != null)
        {
            ParticleSystem explosionCloud = Instantiate(causticSizzlingSpitFX, transform.position, Quaternion.identity);
            var main = explosionCloud.main;
            main.startSizeMultiplier = 2.8f; // Blinding, violent splash cloud of popping caustic liquid slag droplets
            Destroy(explosionCloud.gameObject, 3.0f);
        }

        Debug.Log($"[CAUSTIC LIQUIDATION COMPLETE] Cesium metal core architecture completely dissolved flat into a hydroxide fluid pool at {integratedPhaseProgress * 100f}% decay.");
        Destroy(gameObject, 0.5f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
