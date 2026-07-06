using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_RubidiumDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct AlkaliReactionNode
    {
        public Vector3 localPosition;
        public float localizedExothermicHeat; // Internal reaction temperature (20°C up to 800°C ignition)
        public float superoxideCrustVolume;   // Volume of expanding dark brown burnt ash
        public float deliquescentMeltLevel;   // Progress of liquefaction into fluid rubidium hydroxide
        public bool isNodeCompletelyMelted;
    }

    [Header("Pyrophoric Environmental Settings")]
    [Range(0f, 1f)] public float integratedReactionProgress = 0f;
    [SerializeField] private int latticeMatrixResolution = 10;
    [SerializeField] private float airMoistureHumidity = 1.4f; // Ambient humidity velocity multiplier

    [Header("Mechanical Rigidity Analysis")]
    [Tooltip("Initial structural cohesion strength of solid pure rubidium. It can be easily cut with a knife.")]
    [SerializeField] private float pristineTensileStrength = 40.0f; // Soft structural baseline
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem violetCombustionFlameFX; // Vibrant purple-pink atomic flame plumes
    [SerializeField] private ParticleSystem boilingCausticSpitFX;    // Sizzling liquid splatter particles

    private List<AlkaliReactionNode> chemicalMatrix = new List<AlkaliReactionNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isStructurePermanentlyLiquidated = false;

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
        InitializeAlkaliLattice();
    }

    void Update()
    {
        if (isStructurePermanentlyLiquidated) return;

        SimulateExothermicCombustionPipeline();
    }

    private void InitializeAlkaliLattice()
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

                // Exterior nodes catch humidity instantly, building immediate thermal heat centers
                float surfaceProximity = Vector3.Distance(Vector3.zero, localPoint) / bounds.extents.magnitude;

                AlkaliReactionNode node = new AlkaliReactionNode
                {
                    localPosition = localPoint,
                    localizedExothermicHeat = Mathf.Lerp(25.0f, 200.0f, surfaceProximity),
                    superoxideCrustVolume = 0f,
                    deliquescentMeltLevel = 0f,
                    isNodeCompletelyMelted = false
                };
                chemicalMatrix.Add(node);
            }
        }
    }

    private void SimulateExothermicCombustionPipeline()
    {
        int meltedNodesCount = 0;
        float totalMeltVolume = 0f;

        for (int i = 0; i < chemicalMatrix.Count; i++)
        {
            AlkaliReactionNode node = chemicalMatrix[i];

            // Stage 1: Moisture triggers immediate runaway exothermic heat spikes
            node.localizedExothermicHeat += Time.deltaTime * airMoistureHumidity * 45.0f;

            // Stage 2: Above 150°C, the metal ignites into brown and yellow superoxide ash scales
            if (node.localizedExothermicHeat >= 150.0f && node.deliquescentMeltLevel <= 10f)
            {
                node.superoxideCrustVolume += Time.deltaTime * 6.8f;
            }

            // Stage 3: Extreme hygroscopic conversion pulls moisture, melting the ash into liquid hydroxide
            if (node.superoxideCrustVolume >= 50.0f)
            {
                node.deliquescentMeltLevel += Time.deltaTime * airMoistureHumidity * 5.2f;
            }

            if (node.deliquescentMeltLevel >= 90.0f)
            {
                node.isNodeCompletelyMelted = true;
            }

            if (node.isNodeCompletelyMelted) meltedNodesCount++;
            totalMeltVolume += node.deliquescentMeltLevel;

            chemicalMatrix[i] = node; // Sync struct updates back to memory block
        }

        // Map calculation progress straight to the PBR GPU material properties
        integratedReactionProgress = (float)meltedNodesCount / chemicalMatrix.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, integratedReactionProgress);
        }

        // Mechanical Structural Rigidity Adjustments
        dynamicStructuralIntegrity = Mathf.Lerp(pristineTensileStrength, pristineTensileStrength * 0.001f, integratedReactionProgress);

        if (rb != null)
        {
            // Pure rubidium has low density for a metal (1.53 g/cm³); it rapidly softens and behaves as an amorphous fluid blob
            rb.mass = Mathf.Lerp(30.0f, 12.0f, integratedReactionProgress);
            rb.linearDamping = Mathf.Lerp(0.05f, 6.0f, integratedReactionProgress); // Fluid resistance slumping
        }

        // FX Trigger Management
        if (integratedReactionProgress > 0.05f && integratedReactionProgress < 0.6f && violetCombustionFlameFX != null)
        {
            if (!violetCombustionFlameFX.isPlaying) violetCombustionFlameFX.Play();
        }
        else if (integratedReactionProgress >= 0.6f && violetCombustionFlameFX != null && violetCombustionFlameFX.isPlaying)
        {
            violetCombustionFlameFX.Stop();
        }

        // If deliquescent melting dissolves more than 65% of the tracking network,
        // the remaining solid architecture undergoes absolute structural failure.
        if (integratedReactionProgress >= 0.65f)
        {
            ExecuteAbsoluteChemicalCollapse();
        }
    }

    /// <summary>
    /// Processes physical impacts (Striking the burning alkali layer with weapon assets, tools, or bullet kinetics)
    /// </summary>
    public void RegisterKineticDeformationStrike(Vector3 contactWorldPoint, float forceInputJoules)
    {
        if (isStructurePermanentlyLiquidated) return;

        Vector3 localImpact = transform.InverseTransformPoint(contactWorldPoint);

        // Splat caustic boiling bits outward under pressure impact
        if (boilingCausticSpitFX != null)
        {
            boilingCausticSpitFX.transform.position = contactWorldPoint;
            boilingCausticSpitFX.Emit((int)(forceInputJoules * 0.5f));
        }

        for (int i = 0; i < chemicalMatrix.Count; i++)
        {
            AlkaliReactionNode node = chemicalMatrix[i];
            float range = Vector3.Distance(localImpact, node.localPosition);

            if (range < 1.5f)
            {
                // Physical disruption breaks up the insulating oxide crust, exposing raw rubidium to air and accelerating thermal runaway
                node.localizedExothermicHeat += forceInputJoules * 3.5f;
                chemicalMatrix[i] = node;
            }
        }
    }

    private void ExecuteAbsoluteChemicalCollapse()
    {
        isStructurePermanentlyLiquidated = true;
        StopAllCoroutines();

        // Convert the asset from a solid dynamic mesh entity into a flattened liquid debris puddle
        if (rb != null)
        {
            rb.isKinematic = true; // Melts directly flat into the level ground geometry floor maps
            GetComponent<Collider>().enabled = false;
        }

        if (boilingCausticSpitFX != null)
        {
            ParticleSystem splashCloud = Instantiate(boilingCausticSpitFX, transform.position, Quaternion.identity);
            var main = splashCloud.main;
            main.startSizeMultiplier = 2.5f;
            Destroy(splashCloud.gameObject, 3.0f);
        }

        Debug.Log($"[ALKALI LIQUEFACTION COMPLETE] Rubidium structural framework completely dissolved into a corrosive caustic liquid puddle at {integratedReactionProgress * 100f}% timeline progress.");
        Destroy(gameObject, 0.4f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
