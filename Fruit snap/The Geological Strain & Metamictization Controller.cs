using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_ColumbiteMaterialController : MonoBehaviour
{
    [System.Serializable]
    public struct CrystalLatticeNode
    {
        public Vector3 localPosition;
        public float radiolyticMetamictization; // Internal crystalline lattice disruption (0 to 100 max)
        public float hydrothermalLeachingScale;  // Volume of iron/manganese cations extracted out
        public float mechanicalShearLoad;       // Stored compression forces from earth movements/mining
        public bool isNodeShattered;
    }

    [Header("Geological Environment")]
    [Range(0f, 1f)] public float compiledDecayProgress = 0f;
    [SerializeField] private int gridMatrixResolution = 9;
    [SerializeField] private float fluidAcidityIntensity = 1.5f; // Hydrothermal fluid chemical load scaling

    [Header("Mechanical Mineral Rigidity")]
    [Tooltip("The initial Mohs-to-MPa equivalent structural yield capacity of raw columbite crystals.")]
    [SerializeField] private float pristineMineralCohesionStrength = 420.0f; 
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem earthyOchreDustFX;      // Fine brown-yellow clay particulate clouds
    [SerializeField] private ParticleSystem conchoidalBlackShardFX; // Sharp, curved dark sub-metallic mineral fragments

    private List<CrystalLatticeNode> internalLattice = new List<CrystalLatticeNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isMineralPermanentlyPulverized = false;

    // Fast GPU Parameter Cache Hashes
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicStructuralIntegrity = pristineMineralCohesionStrength;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Generate the 3D internal metallurgical/geological tracking layout
        InitializeColumbiteGrid();
    }

    void Update()
    {
        if (isMineralPermanentlyPulverized) return;

        SimulateGeologicalBreakdown();
    }

    private void InitializeColumbiteGrid()
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

                // External boundary nodes interact with fluid pathways; core layers hold metamict structural decay
                float coreMassBias = Vector3.Distance(Vector3.zero, localPoint) / bounds.extents.magnitude;

                CrystalLatticeNode node = new CrystalLatticeNode
                {
                    localPosition = localPoint,
                    radiolyticMetamictization = Mathf.Lerp(15.0f, 65.0f, 1.0f - coreMassBias), // Internal trace elements bombard core lines
                    hydrothermalLeachingScale = 0f,
                    mechanicalShearLoad = 0f,
                    isNodeShattered = false
                };
                internalLattice.Add(node);
            }
        }
    }

    private void SimulateGeologicalBreakdown()
    {
        int shatteredNodesCount = 0;
        float totalLatticeDisruption = 0f;

        for (int i = 0; i < internalLattice.Count; i++)
        {
            CrystalLatticeNode node = internalLattice[i];

            // Stage 1: Chronic internal trace alpha self-bombardment expands crystallographic structural tension
            node.radiolyticMetamictization += Time.deltaTime * 0.8f;

            // Stage 2: Hydrothermal fluids leach mineral skins, turning crisp surfaces into earthy clay
            if (node.radiolyticMetamictization >= 45.0f)
            {
                node.hydrothermalLeachingScale += Time.deltaTime * fluidAcidityIntensity * 3.2f;
            }

            // Stage 3: Metamict amorphization makes the mineral matrix highly unstable, triggering localized shearing
            if (node.hydrothermalLeachingScale >= 75.0f)
            {
                node.mechanicalShearLoad += Time.deltaTime * 4.0f;
            }

            // Final fracture breakdown limit verification
            if (node.mechanicalShearLoad >= 85.0f || node.radiolyticMetamictization >= 95.0f)
            {
                node.isNodeShattered = true;
            }

            if (node.isNodeShattered) shatteredNodesCount++;
            totalLatticeDisruption += node.radiolyticMetamictization;

            internalLattice[i] = node; // Sync updated structural settings back to memory stack loop
        }

        // Pass calculated progress parameters directly down to the PBR GPU shader properties
        compiledDecayProgress = (float)shatteredNodesCount / internalLattice.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, compiledDecayProgress);
        }

        // Physics Structural Integrity Alteration
        // Metamict mineral structures lose crystalline cohesion completely, collapsing into brittle glass shards.
        float normalizedHealthPct = 1.0f - (totalLatticeDisruption / (internalLattice.Count * 100f));
        dynamicStructuralIntegrity = Mathf.Lerp(pristineMineralCohesionStrength * 0.04f, pristineMineralCohesionStrength, normalizedHealthPct);

        if (rb != null)
        {
            // Columbite is very dense (Density ~ 5.2 to 6.5 g/cm³); mass drops down slightly as components crumble into loose soil
            rb.mass = Mathf.Lerp(90.0f, 60.0f, compiledDecayProgress);
        }

        // If advanced metamict cracking disrupts more than 55% of the tracking node lattice boundaries,
        // the remaining mineral component experiences immediate catastrophic explosive brittle collapse.
        if (compiledDecayProgress >= 0.55f)
        {
            ExecuteCatastrophicMineralShatter();
        }
    }

    /// <summary>
    /// Processes intense external kinetic shocks (Mining pickaxe strikes, heavy impact drills, explosive shockwaves)
    /// </summary>
    public void RegisterKineticDeformationStrike(Vector3 contactWorldPoint, float inputEnergyJoules)
    {
        if (isMineralPermanentlyPulverized) return;

        Vector3 localImpact = transform.InverseTransformPoint(contactWorldPoint);

        for (int i = 0; i < internalLattice.Count; i++)
        {
            CrystalLatticeNode node = internalLattice[i];
            float interactionDistance = Vector3.Distance(localImpact, node.localPosition);

            if (interactionDistance < 1.4f)
            {
                // Amorphous metamict crystals are incredibly fragile under sharp impacts, fracturing instantly
                float crystalVulnerabilityMultiplier = 1.0f + (node.radiolyticMetamictization * 0.06f);
                node.mechanicalShearLoad += (inputEnergyJoules / (interactionDistance + 0.1f)) * 0.5f * crystalVulnerabilityMultiplier;

                if (node.mechanicalShearLoad >= 85.0f)
                {
                    node.isNodeShattered = true;
                    // Burst out crisp conchoidal fragments directly from the tool contact point
                    if (conchoidalBlackShardFX != null) conchoidalBlackShardFX.Emit(5);
                }
                internalLattice[i] = node;
            }
        }
    }

    private void ExecuteCatastrophicMineralShatter()
    {
        isMineralPermanentlyPulverized = true;
        StopAllCoroutines();

        // Release the entity layout into active gravity rigid physics loops
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply a violent, high-velocity fragmentation snap torque impulse profile representing crystalline failure
        rb.AddForce(Vector3.down * 4f, ForceMode.VelocityChange);
        rb.AddTorque(Random.onUnitSphere * 70f, ForceMode.Impulse);

        // Spawn a dual-layered particle splash: sharp conchoidal black shards mixed with dusty earthy ochre soil
        if (conchoidalBlackShardFX != null)
        {
            ParticleSystem shards = Instantiate(conchoidalBlackShardFX, transform.position, Quaternion.identity);
            var mainMod = shards.main;
            mainMod.startSizeMultiplier = 2.5f;
            Destroy(shards.gameObject, 4.0f);
        }
        if (earthyOchreDustFX != null)
        {
            ParticleSystem dustCloud = Instantiate(earthyOchreDustFX, transform.position, Quaternion.identity);
            Destroy(dustCloud.gameObject, 5.0f);
        }

        Debug.Log($"[METAMICT CRYSTAL COLLAPSE] Columbite mineral node experienced complete intergranular structural shattering at {compiledDecayProgress * 100f}% decay index.");
        Destroy(gameObject, 0.15f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
