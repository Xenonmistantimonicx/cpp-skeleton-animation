using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_TungstenDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct ThermalStrainNode
    {
        public Vector3 localPosition;
        public float localizedThermalStress; // Node thermal profile (20°C up to 1500°C calcination envelope)
        public float recrystallizationIndex; // Scale of grain embrittlement (0 to 100 max crystalline brittleness)
        public float trioxideCrustVolume;    // Localized volume profile of powdery oxide yellowing
        public bool isGrainFractured;
    }

    [Header("Refractory Environment Metrics")]
    [Range(0f, 1f)] public float aggregatedDecayProgress = 0f;
    [SerializeField] private int latticeMatrixResolution = 10;
    [SerializeField] private float operatingThermalLoad = 900f; // Ambient thermal exposure multiplier

    [Header("Mechanical Rigidity Analysis")]
    [Tooltip("The initial structural load strength of the dense refractory alloy before embrittlement.")]
    [SerializeField] private float pristineTensileStrength = 600.0f; // Simulated MegaPascals
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem trioxideYellowDustFX; // Fine yellow-green oxide powder particles
    [SerializeField] private ParticleSystem crystalCleavageShardFX; // Sharp, jagged gray metal shards

    private List<ThermalStrainNode> strainLattice = new List<ThermalStrainNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isStructurePermanentlyShattered = false;

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

        // Generate the 3D internal metallurgical tracking layout
        InitializeRefractoryLattice();
    }

    void Update()
    {
        if (isStructurePermanentlyShattered) return;

        SimulateHighTemperatureCalcination();
    }

    private void InitializeRefractoryLattice()
    {
        Bounds bounds = GetComponent<Collider>() != null ? GetComponent<Collider>().bounds : new Bounds(transform.position, Vector3.one);
        Vector3 stepSpacing = bounds.size / (float)latticeMatrixResolution;

        for (int x = 0; x < latticeMatrixResolution; x++)
        {
            for (int y = 0; y < latticeMatrixResolution; y++)
            {
                Vector3 localPoint = new Vector3(
                    (-bounds.extents.x) + (x * stepSpacing.x),
                    (-bounds.extents.y) + (y * stepSpacing.y),
                    (-bounds.extents.z) + (Random.value * bounds.size.z)
                );

                // External nodes catch direct flame/plasma loops; inner core accumulates extreme thermal stress
                float distanceToSurface = Vector3.Distance(Vector3.zero, localPoint) / bounds.extents.magnitude;

                ThermalStrainNode node = new ThermalStrainNode
                {
                    localPosition = localPoint,
                    localizedThermalStress = Mathf.Lerp(25.0f, operatingThermalLoad, distanceToSurface),
                    recrystallizationIndex = 0f,
                    trioxideCrustVolume = 0f,
                    isGrainFractured = false
                };
                strainLattice.Add(node);
            }
        }
    }

    private void SimulateHighTemperatureCalcination()
    {
        int fracturedNodesCount = 0;
        float aggregateLatticeHealth = 0f;

        for (int i = 0; i < strainLattice.Count; i++)
        {
            ThermalStrainNode node = strainLattice[i];

            // Stage 1: Intense operating heat triggers internal recrystallization grain growth
            if (node.localizedThermalStress > 400.0f)
            {
                node.recrystallizationIndex += Time.deltaTime * (node.localizedThermalStress * 0.015f);
            }

            // Stage 2: Above 700°C, atmospheric oxygen calcines the embrittled grain paths into loose trioxide
            if (node.localizedThermalStress >= 700.0f && node.recrystallizationIndex >= 40f)
            {
                node.trioxideCrustVolume += Time.deltaTime * 5.2f;
            }

            // Stage 3: Complete micro-structural grain boundary cleavage check
            if (node.trioxideCrustVolume >= 80f)
            {
                node.isGrainFractured = true;
            }

            if (node.isGrainFractured) fracturedNodesCount++;
            aggregateLatticeHealth += (150.0f - node.recrystallizationIndex);

            strainLattice[i] = node; // Sync matrix parameters back to global memory heap
        }

        // Map calculated progress variables directly down to the PBR GPU shader properties
        aggregatedDecayProgress = (float)fracturedNodesCount / strainLattice.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, aggregatedDecayProgress);
        }

        // Mechanical Structural Rigidity Adjustments
        // Recrystallized tungsten experiences a severe drop in fracture toughness at room temperature.
        float normalizedHealthFactor = Mathf.Clamp01(aggregateLatticeHealth / (strainLattice.Count * 150.0f));
        dynamicStructuralIntegrity = Mathf.Lerp(pristineTensileStrength * 0.02f, pristineTensileStrength, normalizedHealthFactor);

        if (rb != null)
        {
            // Pure tungsten is exceptionally dense (Density ~ 19.3 g/cm³); mass reduces as it turns to airy trioxide powder
            rb.mass = Mathf.Lerp(95.0f, 50.0f, aggregatedDecayProgress);
        }

        // If calcined yellow powder compromises more than 60% of the crystalline network,
        // the remaining core structure experiences immediate brittle cleavage fragmentation under physics loads.
        if (aggregatedDecayProgress >= 0.60f)
        {
            ExecuteCatastrophicCleavageShatter();
        }
    }

    /// <summary>
    /// Processes localized mechanical strikes (Blunt tools, dynamic ballistic kinetic impacts, explosive pressure)
    /// </summary>
    public void RegisterKineticDeformationStrike(Vector3 contactWorldPoint, float forceInputJoules)
    {
        if (isStructurePermanentlyShattered) return;

        Vector3 localImpact = transform.InverseTransformPoint(contactWorldPoint);

        for (int i = 0; i < strainLattice.Count; i++)
        {
            ThermalStrainNode node = strainLattice[i];
            float interactionDistance = Vector3.Distance(localImpact, node.localPosition);

            if (interactionDistance < 1.3f)
            {
                // Embrittled grain nodes have high stiffness but zero ductility, splitting instantly under mechanical loads
                float embrittlementMultiplier = 1.0f + (node.recrystallizationIndex * 0.06f);
                node.trioxideCrustVolume += (forceInputJoules / (interactionDistance + 0.1f)) * 0.04f * embrittlementMultiplier;

                if (node.trioxideCrustVolume >= 80f)
                {
                    node.isGrainFractured = true;
                }
                strainLattice[i] = node;
            }
        }
    }

    private void ExecuteCatastrophicCleavageShatter()
    {
        isStructurePermanentlyShattered = true;
        StopAllCoroutines();

        // Release the entity components completely into dynamic gravity rigid body assets
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply a violent, high-velocity snapping torque rotation profile representing rigid snapping
        rb.AddForce(Vector3.down * 6f, ForceMode.VelocityChange);
        rb.AddTorque(Random.onUnitSphere * 55f, ForceMode.Impulse);

        // Spawn a dual-layered particle splash: sharp metallic shards mixed with bright yellow-green trioxide dust clouds
        if (crystalCleavageShardFX != null)
        {
            ParticleSystem shards = Instantiate(crystalCleavageShardFX, transform.position, Quaternion.identity);
            Destroy(shards.gameObject, 4.0f);
        }
        if (trioxideYellowDustFX != null)
        {
            ParticleSystem powderCloud = Instantiate(trioxideYellowDustFX, transform.position, Quaternion.identity);
            var main = powderCloud.main;
            main.startSizeMultiplier = 4.2f;
            Destroy(powderCloud.gameObject, 5.0f);
        }

        Debug.Log($"[BRITTLE METALLURGICAL CLEAVAGE] Tungsten lattice experienced catastrophic grain boundary snapping at {aggregatedDecayProgress * 100f}% calcination.");
        Destroy(gameObject, 0.15f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
