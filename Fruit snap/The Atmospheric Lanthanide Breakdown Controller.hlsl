using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_LanthanumDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct HydrationNode
    {
        public Vector3 localPosition;
        public float localizedMoistureExposure; // Local humidity infiltration factor
        public float carbonateVolumeSwell;      // Accumulation volume of white scaling crust
        public float boundaryShearStrain;       // Internal interface stress from volume expansion (0 to 100 max)
        public bool isNodeExfoliated;
    }

    [Header("Atmospheric Simulation")]
    [Range(0f, 1f)] public float aggregatedDecayProgress = 0f;
    [SerializeField] private int gridMatrixResolution = 10;
    [SerializeField] private float airMoistureHumidity = 1.4f; // Ambient humidity velocity multiplier

    [Header("Mechanical Structural Yield")]
    [Tooltip("The initial structural load capacity of the soft lanthanum core. It can be cut with a basic knife.")]
    [SerializeField] private float pristineAlloyYieldMPa = 130.0f;
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem indigoSubOxideFlakeFX;  // Dark indigo sub-metallic particles
    [SerializeField] private ParticleSystem whiteCarbonateScaleFX; // Brittle white wafer shards

    private List<HydrationNode> hydrationMatrix = new List<HydrationNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isStructurePermanentlyFailed = false;

    // Fast GPU Parameter Cache Hashes
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicStructuralIntegrity = pristineAlloyYieldMPa;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Generate the 3D internal metallurgical tracking layout
        InitializeHydrationLattice();
    }

    void Update()
    {
        if (isStructurePermanentlyFailed) return;

        SimulateLanthanideHydrationCycle();
    }

    private void InitializeHydrationLattice()
    {
        Bounds bounds = GetComponent<Collider>() != null ? GetComponent<Collider>().bounds : new Bounds(transform.position, Vector3.one);
        Vector3 nodeSpacing = bounds.size / (float)gridMatrixResolution;

        for (int x = 0; x < gridMatrixResolution; x++)
        {
            for (int y = 0; y < gridMatrixResolution; y++)
            {
                Vector3 localPoint = new Vector3(
                    (-bounds.extents.x) + (x * nodeSpacing.x),
                    (-bounds.extents.y) + (y * nodeSpacing.y),
                    (-bounds.extents.z) + (Random.value * bounds.size.z)
                );

                // External nodes catch humidity lines immediately; core holds delayed structural data
                float surfaceDistance = Vector3.Distance(Vector3.zero, localPoint) / bounds.extents.magnitude;

                HydrationNode node = new HydrationNode
                {
                    localPosition = localPoint,
                    localizedMoistureExposure = Mathf.Lerp(15.0f, 100.0f, surfaceDistance),
                    carbonateVolumeSwell = 0f,
                    boundaryShearStrain = 0f,
                    isNodeExfoliated = false
                };
                hydrationMatrix.Add(node);
            }
        }
    }

    private void SimulateLanthanideHydrationCycle()
    {
        int exfoliatedNodesCount = 0;
        float totalInterfaceStrain = 0f;

        for (int i = 0; i < hydrationMatrix.Count; i++)
        {
            HydrationNode node = hydrationMatrix[i];

            // Stage 1: Air exposure builds the initial dark indigo sub-oxide layer
            if (node.localizedMoistureExposure >= 25.0f && node.boundaryShearStrain <= 15f)
            {
                node.carbonateVolumeSwell += Time.deltaTime * airMoistureHumidity * 4.2f;
            }

            // Stage 2: Hydroxide-to-carbonate conversion creates heavy expansion mismatch strain
            if (node.carbonateVolumeSwell >= 35.0f)
            {
                node.boundaryShearStrain += Time.deltaTime * (airMoistureHumidity * 5.5f);
            }

            // Stage 3: Strain limit breach triggers mechanical flaking exfoliation
            if (node.boundaryShearStrain >= 95.0f)
            {
                node.isNodeExfoliated = true;
            }

            if (node.isNodeExfoliated) exfoliatedNodesCount++;
            totalInterfaceStrain += node.boundaryShearStrain;

            hydrationMatrix[i] = node; // Sync updated state changes back to index memory
        }

        // Pass normalization factors directly down to the PBR GPU shader properties
        aggregatedDecayProgress = (float)exfoliatedNodesCount / hydrationMatrix.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, aggregatedDecayProgress);
        }

        // Mechanical Physics Integrity Updates
        // Lanthanum turns into loose chalky ash layers, destroying its structural coherence.
        float normalizedStrainFactor = 1.0f - (totalInterfaceStrain / (hydrationMatrix.Count * 100f));
        dynamicStructuralIntegrity = Mathf.Lerp(pristineAlloyYieldMPa * 0.01f, pristineAlloyYieldMPa, normalizedStrainFactor);

        if (rb != null)
        {
            // Lanthanum has a typical rare-earth density (6.16 g/cm³); mass drops as it sheds porous crust scales
            rb.mass = Mathf.Lerp(61.0f, 32.0f, aggregatedDecayProgress);
        }

        // Particle Management Loop: Sheds flat white shards if moved under advanced decay stress
        if (aggregatedDecayProgress > 0.35f && rb != null && rb.linearVelocity.magnitude > 0.6f)
        {
            if (whiteCarbonateScaleFX != null && !whiteCarbonateScaleFX.isPlaying)
            {
                whiteCarbonateScaleFX.Play();
            }
        }

        // If advanced disintegration breaks down more than 62% of the lattice boundaries,
        // the remaining core experiences total catastrophic crumbling pulverization.
        if (aggregatedDecayProgress >= 0.62f)
        {
            ExecuteCatastrophicLatticeFailure();
        }
    }

    /// <summary>
    /// Processes physical impact shocks (Weapon strikes, weapon shots, structural crushes)
    /// </summary>
    public void RegisterKineticDeformationImpact(Vector3 contactWorldPoint, float forceInputJoules)
    {
        if (isStructurePermanentlyFailed) return;

        Vector3 localStrike = transform.InverseTransformPoint(contactWorldPoint);

        for (int i = 0; i < hydrationMatrix.Count; i++)
        {
            HydrationNode node = hydrationMatrix[i];
            float physicalRange = Vector3.Distance(localStrike, node.localPosition);

            if (physicalRange < 1.4f)
            {
                // Unstable carbonate layers feature near-zero shear resistance and flake off instantly under mechanical shocks
                float shearBrittlenessMultiplier = 1.0f + (node.boundaryShearStrain * 0.06f);
                node.boundaryShearStrain += (forceInputJoules / (physicalRange + 0.1f)) * 0.5f * shearBrittlenessMultiplier;

                if (node.boundaryShearStrain >= 95.0f)
                {
                    node.isNodeExfoliated = true;
                    if (whiteCarbonateScaleFX != null) whiteCarbonateScaleFX.Emit(8);
                }
                
                hydrationMatrix[i] = node;
            }
        }
    }

    private void ExecuteCatastrophicLatticeFailure()
    {
        isStructurePermanentlyFailed = true;
        StopAllCoroutines();

        // Convert the asset into active gravity rigid body components
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply a highly brittle, sliding shear tumble impulse profile
        rb.AddForce(Vector3.down * 5.5f, ForceMode.VelocityChange);
        rb.AddTorque(Random.onUnitSphere * 30f, ForceMode.Impulse);

        if (whiteCarbonateScaleFX != null)
        {
            ParticleSystem fractureCloud = Instantiate(whiteCarbonateScaleFX, transform.position, Quaternion.identity);
            var main = fractureCloud.main;
            main.startSizeMultiplier = 3.2f; // Large burst cloud of peeling wafer-like white carbonate chunks
            Destroy(fractureCloud.gameObject, 4.5f);
        }

        Debug.Log($"[LANTHANIDE PULVERIZATION] Lanthanum matrix crumbled under internal carbonate expansion stress at {aggregatedDecayProgress * 100f}% decay progress.");
        Destroy(gameObject, 0.15f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
