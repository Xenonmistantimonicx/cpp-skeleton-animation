using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_UraniniteDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct RadiolyticLatticeNode
    {
        public Vector3 localPosition;
        public float metamictizationIndex;   // Structural radiation damage (0 to 100 max crystalline chaos)
        public float oxygenInfiltrationMols; // Depth mapping of groundwater penetration
        public float secondaryUranylVolume;  // Secondary neon oxide accumulation level
        public bool isLatticeAtomized;
    }

    [Header("Radio-Chemical Settings")]
    [Range(0f, 1f)] public float compiledDecayProgress = 0f;
    [SerializeField] private int latticeResolution = 10;
    [SerializeField] private float hydrationLeachingVelocity = 1.3f;

    [Header("Radioactive Aura Mechanics")]
    [Tooltip("The dynamic trigger radius for the gameplay radiation damage zone.")]
    public float currentRadiationHazardRadius = 2.0f;
    public float baseGeigerClickFrequency = 1.0f;

    [Header("Mechanical Structural Cohesion")]
    [Tooltip("The structural strength of the mineral before radiolysis crumbles it into powder.")]
    [SerializeField] private float baseLatticeCohesion = 300.0f;
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem neonUranylDustFX;   // High-visibility yellow-green mineral flakes
    [SerializeField] private SphereCollider radiationTriggerZone; // Dynamic gameplay trigger map

    private List<RadiolyticLatticeNode> radiolyticMatrix = new List<RadiolyticLatticeNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isStructureCompletelyPulverized = false;

    // Fast GPU Parameter Hash Mapping
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicStructuralIntegrity = baseLatticeCohesion;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Generate the 3D internal radiolytic tracking layout
        InitializeIsotopicMatrix();
    }

    void Update()
    {
        if (isStructureCompletelyPulverized) return;

        SimulateRadiolyticWeatheringPipeline();
    }

    private void InitializeIsotopicMatrix()
    {
        Bounds bounds = GetComponent<Collider>() != null ? GetComponent<Collider>().bounds : new Bounds(transform.position, Vector3.one);
        Vector3 spacing = bounds.size / (float)latticeResolution;

        for (int x = 0; x < latticeResolution; x++)
        {
            for (int y = 0; y < latticeResolution; y++)
            {
                Vector3 targetLocalPos = new Vector3(
                    (-bounds.extents.x) + (x * spacing.x),
                    (-bounds.extents.y) + (y * spacing.y),
                    (-bounds.extents.z) + (Random.value * bounds.size.z)
                );

                // Exterior nodes capture oxygenated water immediately; core nodes take heavy radiolytic damage
                float edgeDistance = Vector3.Distance(Vector3.zero, targetLocalPos);
                float infiltrationFactor = Mathf.Clamp01(1.0f - (edgeDistance / bounds.extents.magnitude));

                RadiolyticLatticeNode node = new RadiolyticLatticeNode
                {
                    localPosition = targetLocalPos,
                    metamictizationIndex = 0f, // Starts perfectly crystalline
                    oxygenInfiltrationMols = infiltrationFactor * hydrationLeachingVelocity,
                    secondaryUranylVolume = 0f,
                    isLatticeAtomized = false
                };
                radiolyticMatrix.Add(node);
            }
        }
    }

    private void SimulateRadiolyticWeatheringPipeline()
    {
        int atomizedNodesCount = 0;
        float aggregateLatticeDamage = 0f;

        for (int i = 0; i < radiolyticMatrix.Count; i++)
        {
            RadiolyticLatticeNode node = radiolyticMatrix[i];

            if (node.metamictizationIndex < 100f)
            {
                // Stage 1: Constant alpha particle emission breaks down the internal crystal structure
                node.metamictizationIndex += Time.deltaTime * 3.5f;

                // Stage 2: Once structural defects multiply, oxygenated water leaks in and converts U4+ to U6+
                if (node.metamictizationIndex >= 35f)
                {
                    node.secondaryUranylVolume += Time.deltaTime * node.oxygenInfiltrationMols * 5.0f;
                }

                // Stage 3: Complete atomic lattice failure check
                if (node.secondaryUranylVolume >= 75f)
                {
                    node.isLatticeAtomized = true;
                }
            }

            if (node.isLatticeAtomized) atomizedNodesCount++;
            aggregateLatticeDamage += node.metamictizationIndex;

            radiolyticMatrix[i] = node; // Sync matrix data changes back to heap memory arrays
        }

        // Synchronize calculated progress factors directly with the GPU graphics rendering context
        compiledDecayProgress = (float)atomizedNodesCount / radiolyticMatrix.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, compiledDecayProgress);
        }

        // AAA Gameplay Feature: Dynamic Radiation Hazard Expansion
        // As uranium oxidizes into soluble secondary salts, it spreads easily into surrounding pockets.
        // We expand the hazard radius and spike the Geiger click click loop frequency dynamically.
        currentRadiationHazardRadius = Mathf.Lerp(2.0f, 6.5f, compiledDecayProgress);
        baseGeigerClickFrequency = Mathf.Lerp(1.0f, 25.0f, compiledDecayProgress);
        
        if (radiationTriggerZone != null)
        {
            radiationTriggerZone.radius = currentRadiationHazardRadius;
        }

        // Mechanical Physics Degradation
        // Metamict mineral structures lose atomic order, becoming soft, highly fragile glasses.
        float normalizedStabilityFactor = 1.0f - (aggregateLatticeDamage / (radiolyticMatrix.Count * 100f));
        dynamicStructuralIntegrity = Mathf.Lerp(baseLatticeCohesion * 0.02f, baseLatticeCohesion, normalizedStabilityFactor);

        if (rb != null)
        {
            rb.mass = Mathf.Lerp(65.0f, 40.0f, compiledDecayProgress); // Uraninite is hyper-dense (Density ~ 10.0 g/cm³); drops as it turns porous
        }

        // If the radiolytic bloom destroys more than 55% of the crystal grid order, the rock collapses into dust
        if (compiledDecayProgress >= 0.55f)
        {
            ExecuteRadiolyticStructureCollapse();
        }
    }

    /// <summary>
    /// Processes sudden localized mechanical strikes (Mining tool swings, kinetic blasts, projectile impacts)
    /// </summary>
    public void RegisterKineticShockVector(Vector3 contactWorldPoint, float forceJoules)
    {
        if (isStructureCompletelyPulverized) return;

        Vector3 localImpact = transform.InverseTransformPoint(contactWorldPoint);

        for (int i = 0; i < radiolyticMatrix.Count; i++)
        {
            RadiolyticLatticeNode node = radiolyticMatrix[i];
            float physicalRange = Vector3.Distance(localImpact, node.localPosition);

            if (physicalRange < 1.4f)
            {
                // Glassy, metamictized crystal nodes have zero elasticity and crumble instantly under mechanical load
                float structuralVulnerability = 1.0f + (node.metamictizationIndex * 0.05f);
                node.secondaryUranylVolume += (forceJoules / (physicalRange + 0.1f)) * structuralVulnerability;

                if (node.secondaryUranylVolume >= 75f)
                {
                    node.isLatticeAtomized = true;
                }
                radiolyticMatrix[i] = node;
            }
        }
    }

    private void ExecuteRadiolyticStructureCollapse()
    {
        isStructureCompletelyPulverized = true;
        StopAllCoroutines();

        // Sever the component and turn it into full physics debris objects
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply an erratic, high-velocity fracturing twist simulation profile
        rb.AddForce(Vector3.down * 5f, ForceMode.VelocityChange);
        rb.AddTorque(Random.onUnitSphere * 45f, ForceMode.Impulse);

        if (neonUranylDustFX != null)
        {
            ParticleSystem collapseBurst = Instantiate(neonUranylDustFX, transform.position, Quaternion.identity);
            var main = collapseBurst.main;
            main.startSizeMultiplier = 4.5f; // Massive blast cloud of toxic yellow-green oxide particulate dust
            Destroy(collapseBurst.gameObject, 6.0f);
        }

        Debug.Log($"[METAMICT DISINTEGRATION] Uraninite core crystal structure shattered completely from alpha auto-radiolysis at {compiledDecayProgress * 100f}% decay state.");
        Destroy(gameObject, 0.15f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
