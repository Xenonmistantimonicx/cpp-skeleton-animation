using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_TitaniumDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct StressLatticeNode
    {
        public Vector3 localPosition;
        public float localizedTemperatureC;  // Node thermal profile (20°C up to 900°C catastrophic fire)
        public float microFractureStrain;     // Hot-salt stress corrosion cracking load
        public float rutileOxideThickness;   // Thickness scale of protective ceramic layer
        public bool isCeramicNodeShattered;
    }

    [Header("Thermo-Chemical Environment")]
    [Range(0f, 1f)] public float compiledDecayProgress = 0f;
    [SerializeField] private int latticeArrayResolution = 10;
    [SerializeField] private float environmentalThermalLoad = 450.0f; // Ambient engine heat or friction load scaling

    [Header("High-Tier Structural Rigidity")]
    [Tooltip("The pristine ultimate tensile strength of the titanium alloy (e.g., Grade 5 Ti-6Al-4V).")]
    [SerializeField] private float alloyTensileCapacity = 950.0f; // Measured in MegaPascals (Simulated)
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem ceramicWhiteFlakeFX;  // Flaking bright white rutile scales
    [SerializeField] private ParticleSystem sparksIncandescenceFX; // Pyrophoric burning sparks

    private List<StressLatticeNode> structuralMatrix = new List<StressLatticeNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isObjectMechanicallyFailed = false;

    // Fast GPU Parameter Lookup Caching
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicStructuralIntegrity = alloyTensileCapacity;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Generate the 3D internal stress tensor lattice
        InitializeTitaniumStressMatrix();
    }

    void Update()
    {
        if (isObjectMechanicallyFailed) return;

        SimulateThermoChemicalDegradation();
    }

    private void InitializeTitaniumStressMatrix()
    {
        Bounds bounds = GetComponent<Collider>() != null ? GetComponent<Collider>().bounds : new Bounds(transform.position, Vector3.one);
        Vector3 nodeSpacing = bounds.size / (float)latticeArrayResolution;

        for (int x = 0; x < latticeArrayResolution; x++)
        {
            for (int y = 0; y < latticeArrayResolution; y++)
            {
                Vector3 localPoint = new Vector3(
                    (-bounds.extents.x) + (x * nodeSpacing.x),
                    (-bounds.extents.y) + (y * nodeSpacing.y),
                    (-bounds.extents.z) + (Random.value * bounds.size.z)
                );

                // High boundary exposure nodes catch frictional heat loops instantly
                float surfaceProximity = Vector3.Distance(Vector3.zero, localPoint) / bounds.extents.magnitude;

                StressLatticeNode node = new StressLatticeNode
                {
                    localPosition = localPoint,
                    localizedTemperatureC = Mathf.Lerp(25.0f, environmentalThermalLoad, surfaceProximity),
                    microFractureStrain = 0f,
                    rutileOxideThickness = 10.0f, // Starts perfectly passivated with passive shield
                    isCeramicNodeShattered = false
                };
                structuralMatrix.Add(node);
            }
        }
    }

    private void SimulateThermoChemicalDegradation()
    {
        int shatteredNodesCount = 0;
        float aggregateAlloyHealth = 0f;

        for (int i = 0; i < structuralMatrix.Count; i++)
        {
            StressLatticeNode node = structuralMatrix[i];

            // Stage 1: Thermal energy scales up, initiating early thin-film anodization coloration shifts
            if (node.localizedTemperatureC > 300.0f)
            {
                node.rutileOxideThickness += Time.deltaTime * (node.localizedTemperatureC * 0.01f);
            }

            // Stage 2: If temperatures pass critical thresholds, the oxide film buckles and hot-salt cracking sets in
            if (node.localizedTemperatureC >= 610.0f)
            {
                node.microFractureStrain += Time.deltaTime * (node.localizedTemperatureC * 0.05f);
            }

            // Stage 3: Ceramic lattice fracture barrier evaluation
            if (node.microFractureStrain >= 150.0f)
            {
                node.isCeramicNodeShattered = true;
            }

            if (node.isCeramicNodeShattered) shatteredNodesCount++;
            aggregateAlloyHealth += (200.0f - node.microFractureStrain);

            structuralMatrix[i] = node; // Commit lattice node revisions back to heap memory array
        }

        // Pass normalization metrics straight to the PBR GPU shader properties
        compiledDecayProgress = (float)shatteredNodesCount / structuralMatrix.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, compiledDecayProgress);
        }

        // Mechanical Structural Adjustments
        // Titanium retains structural properties until hot salt embrittlement cracks the matrix core.
        float normalizedHealthFactor = Mathf.Clamp01(aggregateAlloyHealth / (structuralMatrix.Count * 200.0f));
        dynamicStructuralIntegrity = Mathf.Lerp(alloyTensileCapacity * 0.12f, alloyTensileCapacity, normalizedHealthFactor);

        // Pyrophoric Spark FX Logic: Triggers molten burning combustion trails if structural limits breach under thermal loads
        if (compiledDecayProgress > 0.4f && sparksIncandescenceFX != null && !sparksIncandescenceFX.isPlaying)
        {
            sparksIncandescenceFX.Play();
        }

        // If over 55% of the internal stress lattice nodes suffer high-temperature embrittlement rupture,
        // the titanium asset experiences sudden catastrophic explosive structural sheer snapping.
        if (compiledDecayProgress >= 0.55f)
        {
            TriggerCatastrophicAlloyShear();
        }
    }

    /// <summary>
    /// Evaluates structural kinetic shocks (Ballistic shell impacts, mechanical overloads, structural tearing)
    /// </summary>
    public void InduceMechanicalStressStrike(Vector3 impactWorldPoint, float inputEnergyJoules)
    {
        if (isObjectMechanicallyFailed) return;

        Vector3 localStrike = transform.InverseTransformPoint(impactWorldPoint);

        for (int i = 0; i < structuralMatrix.Count; i++)
        {
            StressLatticeNode node = structuralMatrix[i];
            float rangeFactor = Vector3.Distance(localStrike, node.localPosition);

            if (rangeFactor < 1.3f)
            {
                // Embrittled nodes experiencing high thermal stress buckle and crack instantly under mechanical shocks
                float thermalEmbrittlementMultiplier = node.localizedTemperatureC > 500.0f ? 2.5f : 1.0f;
                node.microFractureStrain += (inputEnergyJoules / (rangeFactor + 0.1f)) * 0.05f * thermalEmbrittlementMultiplier;

                if (node.microFractureStrain >= 150.0f)
                {
                    node.isCeramicNodeShattered = true;
                }
                structuralMatrix[i] = node;
            }
        }
    }

    private void TriggerCatastrophicAlloyShear()
    {
        isObjectMechanicallyFailed = true;
        StopAllCoroutines();

        // Release the entity layout into active rigid physics assets
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply high-velocity sharp fragmentation snap torque impulses
        rb.AddForce(Vector3.down * 4f, ForceMode.VelocityChange);
        rb.AddTorque(Random.onUnitSphere * 65f, ForceMode.Impulse);

        if (ceramicWhiteFlakeFX != null)
        {
            ParticleSystem fractureCloud = Instantiate(ceramicWhiteFlakeFX, transform.position, Quaternion.identity);
            var main = fractureCloud.main;
            main.startSizeMultiplier = 4.2f; // Large release of chalky white rutile ceramic chunks and metal shards
            Destroy(fractureCloud.gameObject, 5.0f);
        }

        Debug.Log($"[THERMAL EMBRITTLEMENT FAILURE] Titanium crystal lattice fractured along sheer bands at {compiledDecayProgress * 100f}% structural decay index.");
        Destroy(gameObject, 0.15f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
