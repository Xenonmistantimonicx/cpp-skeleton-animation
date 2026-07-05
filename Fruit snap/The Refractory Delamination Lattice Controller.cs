using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_TantalumDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct DelaminationNode
    {
        public Vector3 localPosition;
        public float localizedThermalStress; // Node temperature state (20°C up to 900°C furnace/exhaust simulation)
        public float subOxidePassivation;    // Volume of dark slate-blue passivating film
        public float pentoxideShearStrain;   // Internal interface stress from crystal expansion (0 to 100 max)
        public bool isLayerDelaminated;
    }

    [Header("Thermo-Chemical Profile")]
    [Range(0f, 1f)] public float compiledDecayProgress = 0f;
    [SerializeField] private int latticeMatrixResolution = 10;
    [SerializeField] private float environmentalThermalLoad = 750.0f; // Target operational thermal injection

    [Header("Mechanical Structural Yield")]
    [Tooltip("The initial structural yield capacity of the refractory tantalum metal core before high-temperature calcination.")]
    [SerializeField] private float pristineAlloyYieldMPa = 240.0f;
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem slateSubOxideFlakeFX;  // Dark slate sub-metallic particles
    [SerializeField] private ParticleSystem whitePentoxideScaleFX; // Brittle white ceramic wafer shards

    private List<DelaminationNode> structuralMatrix = new List<DelaminationNode>();
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
        InitializeDelaminationLattice();
    }

    void Update()
    {
        if (isStructurePermanentlyFailed) return;

        SimulatePentoxideDelaminationCycle();
    }

    private void InitializeDelaminationLattice()
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

                // External nodes catch flame, plasma, or electrical arcing; core stores heat lines
                float surfaceDistance = Vector3.Distance(Vector3.zero, localPoint) / bounds.extents.magnitude;

                DelaminationNode node = new DelaminationNode
                {
                    localPosition = localPoint,
                    localizedThermalStress = Mathf.Lerp(25.0f, environmentalThermalLoad, surfaceDistance),
                    subOxidePassivation = 0f,
                    pentoxideShearStrain = 0f,
                    isLayerDelaminated = false
                };
                structuralMatrix.Add(node);
            }
        }
    }

    private void SimulatePentoxideDelaminationCycle()
    {
        int delaminatedNodesCount = 0;
        float totalInterfaceStrain = 0f;

        for (int i = 0; i < structuralMatrix.Count; i++)
        {
            DelaminationNode node = structuralMatrix[i];

            // Stage 1: Heat moves past 500°C, generating the passive slate sub-oxide layer
            if (node.localizedThermalStress >= 500.0f && node.pentoxideShearStrain <= 15f)
            {
                node.subOxidePassivation += Time.deltaTime * 5.8f;
            }

            // Stage 2: Above 700°C, intense oxygen calcination converts the structure into white pentoxide crystals
            if (node.subOxidePassivation >= 40.0f)
            {
                // Volume expansion mismatch generates massive physical shear stress along the oxide boundary
                node.pentoxideShearStrain += Time.deltaTime * (node.localizedThermalStress * 0.025f);
            }

            // Stage 3: Shear limit breach triggers mechanical delamination flaking failure
            if (node.pentoxideShearStrain >= 95.0f)
            {
                node.isLayerDelaminated = true;
            }

            if (node.isLayerDelaminated) delaminatedNodesCount++;
            totalInterfaceStrain += node.pentoxideShearStrain;

            structuralMatrix[i] = node; // Commit struct state variations back to array index
        }

        // Pass normalization factors directly down to the PBR GPU shader properties
        compiledDecayProgress = (float)delaminatedNodesCount / structuralMatrix.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, compiledDecayProgress);
        }

        // Mechanical Physics Integrity Updates
        // Tantalum stands incredibly firm until the expanding white pentoxide shell peels the structural thickness away.
        float normalizedStrainFactor = 1.0f - (totalInterfaceStrain / (structuralMatrix.Count * 100f));
        dynamicStructuralIntegrity = Mathf.Lerp(pristineAlloyYieldMPa * 0.08f, pristineAlloyYieldMPa, normalizedStrainFactor);

        if (rb != null)
        {
            // Tantalum is exceptionally heavy (Density ~ 16.69 g/cm³); mass decreases as the metal sheds dense layers into light ceramic ash
            rb.mass = Mathf.Lerp(85.0f, 42.0f, compiledDecayProgress);
        }

        // Particle Management Loop: Sheds flat white shards if moved under internal pressure stress
        if (compiledDecayProgress > 0.35f && rb != null && rb.velocity.magnitude > 0.8f)
        {
            if (whitePentoxideScaleFX != null && !whitePentoxideScaleFX.isPlaying)
            {
                whitePentoxideScaleFX.Play();
            }
        }

        // If delamination destroys more than 65% of the core node lattice boundaries,
        // the remaining framework completely shears and undergoes catastrophic crumbling.
        if (compiledDecayProgress >= 0.65f)
        {
            ExecuteCatastrophicLatticeFailure();
        }
    }

    /// <summary>
    /// Processes sudden external impact overloads (Tool blows, physical drops, kinetic shocks)
    /// </summary>
    public void RegisterKineticDeformationImpact(Vector3 contactWorldPoint, float forceInputJoules)
    {
        if (isStructurePermanentlyFailed) return;

        Vector3 localStrike = transform.InverseTransformPoint(contactWorldPoint);

        for (int i = 0; i < structuralMatrix.Count; i++)
        {
            DelaminationNode node = structuralMatrix[i];
            float physicalRange = Vector3.Distance(localStrike, node.localPosition);

            if (physicalRange < 1.3f)
            {
                // Highly strained oxide layers are mechanically unstable and peel off instantly under shock vectors
                float shearBrittlenessMultiplier = 1.0f + (node.pentoxideShearStrain * 0.05f);
                node.pentoxideShearStrain += (forceInputJoules / (physicalRange + 0.1f)) * 0.4f * shearBrittlenessMultiplier;

                if (node.pentoxideShearStrain >= 95.0f)
                {
                    node.isLayerDelaminated = true;
                    if (whitePentoxideScaleFX != null) whitePentoxideScaleFX.Emit(6);
                }
                
                structuralMatrix[i] = node;
            }
        }
    }

    private void ExecuteCatastrophicLatticeFailure()
    {
        isStructurePermanentlyFailed = true;
        StopAllCoroutines();

        // Convert the asset into active gravity rigid body assets
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply a brittle, sliding shear tumble impulse profile matching layer separation
        rb.AddForce(Vector3.down * 5f, ForceMode.VelocityChange);
        rb.AddTorque(Random.onUnitSphere * 40f, ForceMode.Impulse);

        if (whitePentoxideScaleFX != null)
        {
            ParticleSystem fractureCloud = Instantiate(whitePentoxideScaleFX, transform.position, Quaternion.identity);
            var main = fractureCloud.main;
            main.startSizeMultiplier = 3.5f; // Large burst cloud of peeling wafer-like white ceramic oxide chunks
            Destroy(fractureCloud.gameObject, 5.0f);
        }

        Debug.Log($"[REFRACTORY LAYER DELAMINATION] Tantalum core framework sheared apart due to localized oxide volume pressure at {compiledDecayProgress * 100f}% total decay.");
        Destroy(gameObject, 0.2f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
