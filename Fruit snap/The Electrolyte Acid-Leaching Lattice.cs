using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_ChalcopyriteDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct MineralLatticeNode
    {
        public Vector3 localPosition;
        public float copperIonDensity;        // Copper concentration remaining (100 down to 0)
        public float thinFilmThicknessNm;     // Secondary sulfide film layer thickness for iridescence
        public float limonitePorosityFactor;  // Structural honeycomb degradation state
        public bool isPeacockPhaseActive;
    }

    [Header("Chemical Exposure Metrics")]
    [Range(0f, 1f)] public float aggregatedDecayProgress = 0f;
    [SerializeField] private int matrixArrayResolution = 12;
    [SerializeField] private float rainAcidMolarityFactor = 1.3f; // Higher values accelerate leaching loops

    [Header("Mechanical Structural Engineering")]
    [Tooltip("The compressive failure limit of the crystal matrix before it crumbles into powdery ochre.")]
    [SerializeField] private float baseCohesionStrength = 250.0f;
    private float dynamicStructuralIntegrity;

    [Header("Dynamic Particle Generation")]
    [SerializeField] private ParticleSystem limoniteOchreDustFX;   // Crumbly yellow-brown flakes
    [SerializeField] private ParticleSystem copperSulfateDripFX;   // Neon-blue/cyan acidic liquid drips

    private List<MineralLatticeNode> oreLattice = new List<MineralLatticeNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isOreCompletelyPulverized = false;

    // Fast GPU Parameter Cache Mapping
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicStructuralIntegrity = baseCohesionStrength;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Generate the 3D grid framework tracking mineral concentrations
        InitializeOreMatrix();
    }

    void Update()
    {
        if (isOreCompletelyPulverized) return;

        SimulateAcidMineDrainageWeathering();
    }

    private void InitializeOreMatrix()
    {
        Bounds bounds = GetComponent<Collider>() != null ? GetComponent<Collider>().bounds : new Bounds(transform.position, Vector3.one);
        Vector3 spacing = bounds.size / (float)matrixArrayResolution;

        for (int x = 0; x < matrixArrayResolution; x++)
        {
            for (int y = 0; y < matrixArrayResolution; y++)
            {
                Vector3 localPos = new Vector3(
                    (-bounds.extents.x) + (x * spacing.x),
                    (-bounds.extents.y) + (y * spacing.y),
                    (-bounds.extents.z) + (Random.value * bounds.size.z)
                );

                // High-elevation nodes leach copper downward; low-elevation nodes capture liquid run-off
                float heightBias = (localPos.y + bounds.extents.y) / bounds.size.y;

                MineralLatticeNode node = new MineralLatticeNode
                {
                    localPosition = localPos,
                    copperIonDensity = 100.0f,
                    thinFilmThicknessNm = 0f,
                    limonitePorosityFactor = 0f,
                    isPeacockPhaseActive = false
                };
                oreLattice.Add(node);
            }
        }
    }

    private void SimulateAcidMineDrainageWeathering()
    {
        int oxidizedNodesCount = 0;
        float aggregateCopperReserve = 0f;

        for (int i = 0; i < oreLattice.Count; i++)
        {
            MineralLatticeNode node = oreLattice[i];

            if (node.copperIonDensity > 0f)
            {
                // Stage 1: Surface oxidation leaches copper out and converts it to thin bornite/covellite films
                float leachingVelocity = Time.deltaTime * rainAcidMolarityFactor * 1.8f;
                node.copperIonDensity = Mathf.Max(0f, node.copperIonDensity - leachingVelocity);

                // Stage 2: Film growth handles active iridescence spectrum processing
                if (node.copperIonDensity < 95f && node.copperIonDensity > 45f)
                {
                    node.thinFilmThicknessNm += Time.deltaTime * 12f;
                    node.isPeacockPhaseActive = true;
                }
                else
                {
                    node.isPeacockPhaseActive = false;
                }

                // Stage 3: Late stage conversion to porous, structural iron oxide (Limonite honeycomb)
                if (node.copperIonDensity <= 45f)
                {
                    node.limonitePorosityFactor += Time.deltaTime * 4.2f;
                }
            }

            if (node.copperIonDensity <= 45f) oxidizedNodesCount++;
            aggregateCopperReserve += node.copperIonDensity;

            oreLattice[i] = node; // Commit node data updates back to memory structure
        }

        // Send normalized transformation metrics down to the GPU rendering context
        aggregatedDecayProgress = (float)oxidizedNodesCount / oreLattice.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, aggregatedDecayProgress);
        }

        // Mechanical & Structural Modification Calculations
        float remainingMetalPct = aggregateCopperReserve / (oreLattice.Count * 100f);
        dynamicStructuralIntegrity = Mathf.Lerp(baseCohesionStrength * 0.05f, baseCohesionStrength, remainingMetalPct);

        // Acid Drip FX Logic: Spawns highly aesthetic blue copper sulfate drops during the active leaching phase
        if (remainingMetalPct < 0.85f && remainingMetalPct > 0.4f)
        {
            if (copperSulfateDripFX != null && !copperSulfateDripFX.isPlaying)
            {
                copperSulfateDripFX.Play();
            }
        }
        else if (remainingMetalPct <= 0.4f && copperSulfateDripFX != null && copperSulfateDripFX.isPlaying)
        {
            copperSulfateDripFX.Stop(); // Drips dry up as copper completely empties from the ore
        }

        // If more than 60% of the ore matrix nodes collapse into highly porous limonite, 
        // the crystal architecture disintegrates into crumbling ochre dirt chunks.
        if (aggregatedDecayProgress >= 0.60f)
        {
            ExecuteLimoniteMatrixCollapse();
        }
    }

    /// <summary>
    /// Processes physical impact shocks (Mining picks, dynamic explosives, weapon impacts)
    /// </summary>
    public void RegisterKineticDeformationForce(Vector3 contactWorldPoint, float forceInputNewtons)
    {
        if (isOreCompletelyPulverized) return;

        Vector3 localImpact = transform.InverseTransformPoint(contactWorldPoint);

        for (int i = 0; i < oreLattice.Count; i++)
        {
            MineralLatticeNode node = oreLattice[i];
            float interactionDistance = Vector3.Distance(localImpact, node.localPosition);

            if (interactionDistance < 1.3f)
            {
                // Heavily porous limonite honeycombs pulverize immediately if struck
                float porosityVulnerability = 1.0f + (node.limonitePorosityFactor * 0.08f);
                node.copperIonDensity = Mathf.Max(0f, node.copperIonDensity - (forceInputNewtons / (interactionDistance + 0.1f)) * porosityVulnerability);
                
                if (node.copperIonDensity <= 45f)
                {
                    node.limonitePorosityFactor += 15f;
                }
                oreLattice[i] = node;
            }
        }
    }

    private void ExecuteLimoniteMatrixCollapse()
    {
        isOreCompletelyPulverized = true;
        StopAllCoroutines();

        // Release the entity directly into active physics simulation debris
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply a grinding, sluggish tumble force profile matching a soft, crumbly ore chunk
        rb.AddTorque(Random.onUnitSphere * 25f, ForceMode.Impulse);

        if (limoniteOchreDustFX != null)
        {
            ParticleSystem collapseBurst = Instantiate(limoniteOchreDustFX, transform.position, Quaternion.identity);
            var main = collapseBurst.main;
            main.startSizeMultiplier = 4.2f; // Large release of earthy, yellow-brown ochre dust particles
            Destroy(collapseBurst.gameObject, 5.0f);
        }

        Debug.Log($"[LEACHING TOTAL FAILURE] Crystalline chalcopyrite matrix completely dissolved into porous limonite dust at {aggregatedDecayProgress * 100f}% decay state.");
        Destroy(gameObject, 0.2f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
