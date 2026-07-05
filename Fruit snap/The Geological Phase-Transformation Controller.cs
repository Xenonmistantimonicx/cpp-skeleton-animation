using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_TantaliteMaterialController : MonoBehaviour
{
    [System.Serializable]
    public struct MetamictLatticeNode
    {
        public Vector3 localPosition;
        public float alphaLatticeIntegrity;   // Internal structural order (100 down to 0 completely amorphous)
        public float pentoxideLeachingVolume; // Accumulation level of flaking tan oxide crust
        public float internalExpansionStrain; // Tensile stress driven by metamict volume swelling
        public bool isGrainFractured;
    }

    [Header("Geological Environmental Dynamics")]
    [Range(0f, 1f)] public float aggregatedDecayProgress = 0f;
    [SerializeField] private int gridMatrixResolution = 10;
    [SerializeField] private float hydrothermalFluidFlow = 1.3f; // Chemical leaching speed factor

    [Header("Mechanical Mineral Rigidity")]
    [Tooltip("The initial structural load strength of heavy crystalline tantalite before metamictization.")]
    [SerializeField] private float baseMineralTensileStrength = 480.0f; // Simulated MegaPascals
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem tanPentoxideDustFX;      // Powdery tan-grey clay particulate clouds
    [SerializeField] private ParticleSystem crystalCleavageShardFX; // Heavy, sharp pitch-black mineral shards

    private List<MetamictLatticeNode> tantaliteGrid = new List<MetamictLatticeNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isMineralPermanentlyPulverized = false;

    // Fast GPU Parameter Cache Hashes
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicStructuralIntegrity = baseMineralTensileStrength;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Generate the 3D internal metallurgical tracking layout
        InitializeTantaliteLattice();
    }

    void Update()
    {
        if (isMineralPermanentlyPulverized) return;

        SimulateMetamictCrystallineBreakdown();
    }

    private void InitializeTantaliteLattice()
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

                // Deep core layers sustain maximum radiolytic distortion; external boundaries touch fluids
                float coreProximityBias = Vector3.Distance(Vector3.zero, localPoint) / bounds.extents.magnitude;

                MetamictLatticeNode node = new MetamictLatticeNode
                {
                    localPosition = localPoint,
                    alphaLatticeIntegrity = 100.0f,
                    pentoxideLeachingVolume = 0f,
                    internalExpansionStrain = Mathf.Lerp(30.0f, 0f, coreProximityBias), // Core expands heavily from radio-traces
                    isGrainFractured = false
                };
                tantaliteGrid.Add(node);
            }
        }
    }

    private void SimulateMetamictCrystallineBreakdown()
    {
        int fracturedNodesCount = 0;
        float aggregateLatticeHealth = 0f;

        for (int i = 0; i < tantaliteGrid.Count; i++)
        {
            MetamictLatticeNode node = tantaliteGrid[i];

            if (node.alphaLatticeIntegrity > 0f)
            {
                // Stage 1: Continuous alpha self-irradiation amorphizes the orthorhombic grain boundaries
                float radiolyticDamage = Time.deltaTime * 1.2f;
                node.alphaLatticeIntegrity = Mathf.Max(0f, node.alphaLatticeIntegrity - radiolyticDamage);

                // Volumetric swelling internally maps to escalating tensile strain values
                node.internalExpansionStrain += radiolyticDamage * 1.4f;

                // Stage 2: Crystalline lattice disruption lets hydrothermal fluids leach out iron/manganese ions
                if (node.alphaLatticeIntegrity < 65f)
                {
                    node.pentoxideLeachingVolume += Time.deltaTime * hydrothermalFluidFlow * 4.8f;
                }

                // Stage 3: Strain capacity threshold check
                if (node.internalExpansionStrain >= 85f || node.pentoxideLeachingVolume >= 80f)
                {
                    node.isGrainFractured = true;
                }
            }

            if (node.isGrainFractured) fracturedNodesCount++;
            aggregateLatticeHealth += node.alphaLatticeIntegrity;

            tantaliteGrid[i] = node; // Sync struct parameters back to memory block array
        }

        // Map calculated progress variables directly down to the PBR GPU shader properties
        aggregatedDecayProgress = (float)fracturedNodesCount / tantaliteGrid.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, aggregatedDecayProgress);
        }

        // Mechanical Structural Rigidity Adjustments
        // Metamict mineral lattices grow highly brittle, completely shedding original fracture toughness.
        float normalizedHealthPct = aggregateLatticeHealth / (tantaliteGrid.Count * 100f);
        dynamicStructuralIntegrity = Mathf.Lerp(baseMineralTensileStrength * 0.02f, baseMineralTensileStrength, normalizedHealthPct);

        if (rb != null)
        {
            // Tantalite is extraordinarily heavy (Density ~ 8.0 g/cm³); mass decreases as layers fragment away into airy clay soil
            rb.mass = Mathf.Lerp(120.0f, 75.0f, aggregatedDecayProgress);
        }

        // If internal metamict swelling compromises more than 60% of the nodes,
        // the remaining component experiences a spontaneous catastrophic geometric cleavage failure.
        if (aggregatedDecayProgress >= 0.60f)
        {
            ExecuteCatastrophicCrystallineShatter();
        }
    }

    /// <summary>
    /// Processes sudden heavy kinetic overloads (Mining pickaxe strikes, drill waves, heavy explosions)
    /// </summary>
    public void RegisterKineticDeformationStrike(Vector3 contactWorldPoint, float forceInputJoules)
    {
        if (isMineralPermanentlyPulverized) return;

        Vector3 localImpact = transform.InverseTransformPoint(contactWorldPoint);

        for (int i = 0; i < tantaliteGrid.Count; i++)
        {
            MetamictLatticeNode node = tantaliteGrid[i];
            float localizedRange = Vector3.Distance(localImpact, node.localPosition);

            if (localizedRange < 1.3f)
            {
                // Amorphous, internally strained lattices feature zero ductility and fracture cleanly under shocks
                float brittlenessMultiplier = 1.0f + ((100f - node.alphaLatticeIntegrity) * 0.06f);
                node.internalExpansionStrain += (forceInputJoules / (localizedRange + 0.1f)) * brittlenessMultiplier;

                if (node.internalExpansionStrain >= 85f)
                {
                    node.isGrainFractured = true;
                }
                tantaliteGrid[i] = node;
            }
        }
    }

    private void ExecuteCatastrophicCrystallineShatter()
    {
        isMineralPermanentlyPulverized = true;
        StopAllCoroutines();

        // Release the entity components completely into dynamic gravity rigid bodies
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply a violent, crisp snapping rotation profile matching structural cleavage failure
        rb.AddForce(Vector3.down * 5f, ForceMode.VelocityChange);
        rb.AddTorque(Random.onUnitSphere * 65f, ForceMode.Impulse);

        // Spawn a dual-layered particle splash: heavy pitch-black shards mixed with chalky tan pentoxide dust clouds
        if (crystalCleavageShardFX != null)
        {
            ParticleSystem shards = Instantiate(crystalCleavageShardFX, transform.position, Quaternion.identity);
            var main = shards.main;
            main.startSizeMultiplier = 2.2f;
            Destroy(shards.gameObject, 4.0f);
        }
        if (tanPentoxideDustFX != null)
        {
            ParticleSystem powderCloud = Instantiate(tanPentoxideDustFX, transform.position, Quaternion.identity);
            Destroy(powderCloud.gameObject, 5.0f);
        }

        Debug.Log($"[METAMICT LATTICE EXPLOSION] Heavy tantalite crystal structure collapsed into sharp cleavage shards at {aggregatedDecayProgress * 100f}% structural strain.");
        Destroy(gameObject, 0.15f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
