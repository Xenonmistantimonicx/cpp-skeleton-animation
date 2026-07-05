using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_ZincDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct GalvanicLatticeNode
    {
        public Vector3 localPosition;
        public float zincMassIntegrity;       // Percentage of active pure zinc layer remaining (0 to 100)
        public float moistureRetentionFactor; // Stagnant water pooling multiplier (accelerates white rust)
        public float carbonateShieldHp;      // Protective patina layer health
        public bool isChalkyCrustFormed;
    }

    [Header("Core Chemical Attributes")]
    [Range(0f, 1f)] public float compiledDecayProgress = 0f;
    [SerializeField] private int latticeArrayResolution = 12;
    [SerializeField] private float environmentalHumidityIndex = 1.0f; // Ambient air dampness scale

    [Header("Mechanical Structural Breakdown")]
    [Tooltip("The tensile capacity of the underlying substrate before the zinc matrix crumbles.")]
    [SerializeField] private float materialBrittleThreshold = 150f;
    private float currentAlloyCohesionPool;

    [Header("Volumetric Ash Particles")]
    [SerializeField] private ParticleSystem whiteRustPowderFX; // Chalky white zinc dust loops

    private List<GalvanicLatticeNode> electrochemicalMatrix = new List<GalvanicLatticeNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isStructurePulverized = false;

    // Shader Variable Hash Optimizations
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        currentAlloyCohesionPool = materialBrittleThreshold;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Procedurally construct the 3D grid layout tracking electro-chemical vectors
        GenerateElectrochemicalLattice();
    }

    void Update()
    {
        if (isStructurePulverized) return;

        SimulateSacrificialOxidationMatrix();
    }

    private void GenerateElectrochemicalLattice()
    {
        Bounds bounds = GetComponent<Collider>() != null ? GetComponent<Collider>().bounds : new Bounds(transform.position, Vector3.one);
        Vector3 nodeSpacing = bounds.size / (float)latticeArrayResolution;

        for (int x = 0; x < latticeArrayResolution; x++)
        {
            for (int y = 0; y < latticeArrayResolution; y++)
            {
                Vector3 targetLocalPos = new Vector3(
                    (-bounds.extents.x) + (x * nodeSpacing.x),
                    (-bounds.extents.y) + (y * nodeSpacing.y),
                    Random.Range(-bounds.extents.z, bounds.extents.z)
                );

                // Nodes near the bottom or inside crevices have naturally higher moisture retention (stagnant pooling)
                float calculatedMoisturePool = Mathf.Lerp(1.5f, 0.5f, (targetLocalPos.y + bounds.extents.y) / bounds.size.y);

                GalvanicLatticeNode node = new GalvanicLatticeNode
                {
                    localPosition = targetLocalPos,
                    zincMassIntegrity = 100.0f,
                    carbonateShieldHp = 30.0f, // Starts ready to passivate
                    moistureRetentionFactor = calculatedMoisturePool * environmentalHumidityIndex,
                    isChalkyCrustFormed = false
                };
                electrochemicalMatrix.Add(node);
            }
        }
    }

    private void SimulateSacrificialOxidationMatrix()
    {
        int totalCrustedNodes = 0;
        float aggregateMassIntegrity = 0f;

        for (int i = 0; i < electrochemicalMatrix.Count; i++)
        {
            GalvanicLatticeNode node = electrochemicalMatrix[i];

            if (node.zincMassIntegrity > 0f)
            {
                // Stage 1: Attempt to build the stable carbonate patina protective shield
                if (node.carbonateShieldHp > 0f)
                {
                    // High humidity without ventilation wears away the carbonate shield capability
                    node.carbonateShieldHp -= Time.deltaTime * node.moistureRetentionFactor * 0.5f;
                }

                // Stage 2: Accelerated White Rust activation if the shield drops while moisture is active
                float corrosionRate = Time.deltaTime * node.moistureRetentionFactor * 2.0f;
                if (node.carbonateShieldHp <= 0f)
                {
                    corrosionRate *= 3.5f; // Exponentially fast white hydroxide accumulation
                    node.isChalkyCrustFormed = true;
                }

                node.zincMassIntegrity = Mathf.Max(0f, node.zincMassIntegrity - corrosionRate);
            }

            if (node.isChalkyCrustFormed) totalCrustedNodes++;
            aggregateMassIntegrity += node.zincMassIntegrity;

            electrochemicalMatrix[i] = node; // Sync array changes back
        }

        // Global value updates for GPU pipeline variables
        compiledDecayProgress = (float)totalCrustedNodes / electrochemicalMatrix.Count;
        
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, compiledDecayProgress);
        }

        // Mechanical Physics Translation
        // Crusted zinc is porous and chalky. It breaks away from its substrate, reducing physical density.
        float normalizedMassFactor = aggregateMassIntegrity / (electrochemicalMatrix.Count * 100f);
        currentAlloyCohesionPool = Mathf.Lerp(materialBrittleThreshold * 0.1f, materialBrittleThreshold, normalizedMassFactor);

        if (rb != null)
        {
            rb.mass = Mathf.Lerp(15.0f, 22.0f, normalizedMassFactor); // Weight stripping as zinc salts dissolve
            rb.drag = Mathf.Lerp(0.6f, 0.1f, normalizedMassFactor);   // Corrosion scaling dynamic friction
        }

        // Trigger dry crumbling powder bursts if subjected to friction velocities
        if (compiledDecayProgress > 0.5f && rb != null && rb.velocity.magnitude > 0.8f)
        {
            if (whiteRustPowderFX != null && !whiteRustPowderFX.isPlaying)
            {
                whiteRustPowderFX.Play();
            }
        }

        // If the sacrificial zinc shell depletes entirely, the node matrix fractures
        if (compiledDecayProgress >= 0.75f)
        {
            PulverizeZincLattice();
        }
    }

    /// <summary>
    /// Captures physical structural impacts (Axe slices, shell fragmentation, tool scrubbing)
    /// </summary>
    public void RegisterKineticDeformation(Vector3 collisionWorldPoint, float energyJoules)
    {
        if (isStructurePulverized) return;

        Vector3 localImpact = transform.InverseTransformPoint(collisionWorldPoint);

        for (int i = 0; i < electrochemicalMatrix.Count; i++)
        {
            GalvanicLatticeNode node = electrochemicalMatrix[i];
            float range = Vector3.Distance(localImpact, node.localPosition);

            if (range < 1.5f)
            {
                // Crumble crusted nodes instantly if mechanically jarred
                float damageScale = energyJoules / (range + 0.1f);
                node.zincMassIntegrity = Mathf.Max(0f, node.zincMassIntegrity - (damageScale * 0.25f));
                
                if (node.zincMassIntegrity <= 10f)
                {
                    node.isChalkyCrustFormed = true;
                }
                electrochemicalMatrix[i] = node;
            }
        }
    }

    private void PulverizeZincLattice()
    {
        isStructurePulverized = true;
        StopAllCoroutines();

        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply a brittle crunch tumble simulation profile
        rb.AddTorque(Random.onUnitSphere * 40f, ForceMode.Impulse);

        if (whiteRustPowderFX != null)
        {
            ParticleSystem collapseCloud = Instantiate(whiteRustPowderFX, transform.position, Quaternion.identity);
            var main = collapseCloud.main;
            main.startSizeMultiplier = 4.0f; // Explosive release of powdery white carbonate dust
            Destroy(collapseCloud.gameObject, 6.0f);
        }

        Debug.Log($"[SACRIFICIAL CORE VOID] Galvanic coating completely ruptured at {compiledDecayProgress * 100f}% corrosion profile. Material matrix dissolved.");
        Destroy(gameObject, 0.1f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
