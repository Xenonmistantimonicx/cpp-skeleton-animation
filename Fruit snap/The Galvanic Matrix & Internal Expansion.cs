using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_PyrrhotiteDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct GalvanicLatticeNode
    {
        public Vector3 localPosition;
        public float pyrrhotiteDensity;     // Crystalline mass profile (100 down to 0)
        public float volumetricSwellingPsi; // Secondary mineral expansion pressure (0 to 300)
        public float internalAcidMolarity;  // H2SO4 concentration eating surrounding matrix
        public bool isPlateSheared;
    }

    [Header("Electro-Chemical Settings")]
    [Range(0f, 1f)] public float aggregatedDecayProgress = 0f;
    [SerializeField] private int latticeArrayResolution = 10;
    [SerializeField] private float moistureInfiltrationVelocity = 1.4f;

    [Header("Mechanical & Magnetic Properties")]
    [Tooltip("The ultimate tensile pressure limit before the host rock matrix explodes from internal swelling.")]
    [SerializeField] private float matrixTensileLimit = 350.0f;
    private float dynamicStructuralStability;
    public float dynamicMagneticIntensity = 1.0f; // Drops to 0 as pyrrhotite turns to rust

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem sulfurAcidVaporFX;  // Corrosive acidic fumes
    [SerializeField] private ParticleSystem rockShatterFlakesFX; // Spanning rock fracture flakes

    private List<GalvanicLatticeNode> internalLattice = new List<GalvanicLatticeNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isStructureCompletelyRuptured = false;

    // Fast Lookup GPU String Hashes
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicStructuralStability = matrixTensileLimit;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Generate the 3D internal stress tensor lattice
        InitializeMonoclinicLattice();
    }

    void Update()
    {
        if (isStructureCompletelyRuptured) return;

        SimulatePyrrhotiteGalvanicDisease();
    }

    private void InitializeMonoclinicLattice()
    {
        Bounds bounds = GetComponent<Collider>() != null ? GetComponent<Collider>().bounds : new Bounds(transform.position, Vector3.one);
        Vector3 stepSpacing = bounds.size / (float)latticeArrayResolution;

        for (int x = 0; x < latticeArrayResolution; x++)
        {
            for (int y = 0; y < latticeArrayResolution; y++)
            {
                Vector3 targetLocalPos = new Vector3(
                    (-bounds.extents.x) + (x * stepSpacing.x),
                    (-bounds.extents.y) + (y * stepSpacing.y),
                    (-bounds.extents.z) + (Random.value * bounds.size.z)
                );

                // External nodes react immediately; interior nodes build insane trapped pressure profiles
                float surfaceDistance = Vector3.Distance(Vector3.zero, targetLocalPos);
                float depthFactor = Mathf.Clamp01(surfaceDistance / bounds.extents.magnitude);

                GalvanicLatticeNode node = new GalvanicLatticeNode
                {
                    localPosition = targetLocalPos,
                    pyrrhotiteDensity = 100.0f,
                    volumetricSwellingPsi = 0f,
                    internalAcidMolarity = 0f,
                    isPlateSheared = false
                };
                internalLattice.Add(node);
            }
        }
    }

    private void SimulatePyrrhotiteGalvanicDisease()
    {
        int shearedNodesCount = 0;
        float remainingDensityPool = 0f;

        for (int i = 0; i < internalLattice.Count; i++)
        {
            GalvanicLatticeNode node = internalLattice[i];

            if (node.pyrrhotiteDensity > 0f)
            {
                // Stage 1: Fast Galvanic Oxidation fueled by iron-deficiency lattice instability
                float reactionRate = Time.deltaTime * moistureInfiltrationVelocity * 2.2f;
                node.pyrrhotiteDensity = Mathf.Max(0f, node.pyrrhotiteDensity - reactionRate);

                // Stage 2: Sulfuric Acid creation breaks down surrounding calcium matrix
                if (node.pyrrhotiteDensity < 90f)
                {
                    node.internalAcidMolarity += Time.deltaTime * 1.5f;
                }

                // Stage 3: Secondary Ettringite Mineral Buildup -> High Volumetric Expansion Pressure (Up to 300%)
                if (node.internalAcidMolarity > 20f)
                {
                    node.volumetricSwellingPsi += Time.deltaTime * node.internalAcidMolarity * 3.8f;
                }

                // Stage 4: Critical Intergranular Shearing Check
                if (node.volumetricSwellingPsi >= 120f)
                {
                    node.isPlateSheared = true;
                }
            }

            if (node.isPlateSheared) shearedNodesCount++;
            remainingDensityPool += node.pyrrhotiteDensity;

            internalLattice[i] = node; // Push structural modifications back to memory heap
        }

        // Synchronize computed calculations directly with the GPU graphics context
        aggregatedDecayProgress = (float)shearedNodesCount / internalLattice.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, aggregatedDecayProgress);
        }

        // Mechanical & Physical Properties Mutation
        float structuralHealthPct = remainingDensityPool / (internalLattice.Count * 100f);
        dynamicStructuralStability = Mathf.Lerp(matrixTensileLimit * 0.01f, matrixTensileLimit, structuralHealthPct);

        // AAA Feature: Magnetic Degradation
        // As pyrrhotite's iron sulfide lattice converts to non-magnetic rust components, 
        // the object loses its gameplay magnetic tracking properties completely.
        dynamicMagneticIntensity = Mathf.Max(0f, structuralHealthPct);

        // Play corrosive bubbling acid vapor emissions at mid-stages of structural distress
        if (aggregatedDecayProgress > 0.25f && sulfurAcidVaporFX != null && !sulfurAcidVaporFX.isPlaying)
        {
            sulfurAcidVaporFX.Play();
        }

        // If the internal volumetric swelling strain shears more than 50% of the internal grid nodes,
        // the entire object undergoes violent mechanical explosive decompression/spalling.
        if (aggregatedDecayProgress >= 0.50f)
        {
            ExecuteCatastrophicSpallingCollapse();
        }
    }

    /// <summary>
    /// Receives localized external energy strikes (Blunt pickaxe shocks, heavy physics drops, vibration notes)
    /// </summary>
    public void RegisterKineticShockWave(Vector3 impactWorldPoint, float forceMegaNewtons)
    {
        if (isStructureCompletelyRuptured) return;

        Vector3 localStrike = transform.InverseTransformPoint(impactWorldPoint);

        for (int i = 0; i < internalLattice.Count; i++)
        {
            GalvanicLatticeNode node = internalLattice[i];
            float localizedRange = Vector3.Distance(localStrike, node.localPosition);

            if (localizedRange < 1.4f)
            {
                // The trapped swelling pressure inside acts like a loaded spring; shock impacts trigger instant failure loops
                float internalPressureAmplifier = 1.0f + (node.volumetricSwellingPsi * 0.08f);
                node.volumetricSwellingPsi += (forceMegaNewtons / (localizedRange + 0.1f)) * internalPressureAmplifier;

                if (node.volumetricSwellingPsi >= 120f)
                {
                    node.isPlateSheared = true;
                }
                internalLattice[i] = node;
            }
        }
    }

    private void ExecuteCatastrophicSpallingCollapse()
    {
        isStructureCompletelyRuptured = true;
        StopAllCoroutines();

        // Drop the component into raw physics simulation fragments
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply high-pressure explosive disintegration vectors
        rb.AddForce(Vector3.up * 8f, ForceMode.Impulse);
        rb.AddTorque(Random.onUnitSphere * 80f, ForceMode.Impulse);

        if (rockShatterFlakesFX != null)
        {
            ParticleSystem explosionCloud = Instantiate(rockShatterFlakesFX, transform.position, Quaternion.identity);
            var main = explosionCloud.main;
            main.startSizeMultiplier = 5.0f; // Enormous burst of structural rock shards and gray acid powder
            Destroy(explosionCloud.gameObject, 6.0f);
        }

        Debug.Log($"[PYRRHOTITE SPALLING CRITICAL] Volumetric swelling breached tensile limits. Structural concrete lattice pulverized.");
        Destroy(gameObject, 0.2f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
