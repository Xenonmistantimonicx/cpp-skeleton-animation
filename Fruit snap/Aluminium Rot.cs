using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_AluminiumDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct CorrosionNode
    {
        public Vector3 positionWS;
        public float localizedSalinity;      // Chloride ion density accelerating galvanic breakdown
        public float passivationShieldHp;   // Aluminum Oxide thickness (0.0 to 100.0)
        public float intergranularStress;    // Internal structural strain
        public bool isStructuralPointRuptured;
    }

    [Header("Core Chemical Dynamics")]
    [Range(0f, 1f)] public float globalCorrosionProgress = 0f;
    [SerializeField] private int materialMatrixResolution = 16;
    [SerializeField] private float environmentalSalinityModifier = 1.5f; // Sea air/coastal maps scale this high

    [Header("Mechanical Structural Engineering")]
    [Tooltip("Pristine yield strength profile of the alloy (e.g., T6-6061 Aluminum).")]
    [SerializeField] private float alloyYieldStrength = 276.0f; // Measured in MegaPascals (Simulated)
    private float dynamicStructuralIntegrity;

    [Header("Volumetric VFX Generation")]
    [SerializeField] private ParticleSystem aluminumHydroxideDustFX; // Flaky white dust emission

    private List<CorrosionNode> materialMatrix = new List<CorrosionNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isObjectMechanicallyFailed = false;

    // GPU Parameter Lookup Hashes
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicStructuralIntegrity = alloyYieldStrength;

        // Initialize unique instanced PBR material context
        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Generate full physical structural nodes tracking point maps across bounding box dimensions
        InitializeStructuralMatrix();
    }

    void Update()
    {
        if (isObjectMechanicallyFailed) return;

        SimulateGalvanicCorrosionMatrix();
    }

    private void InitializeStructuralMatrix()
    {
        Bounds bounds = GetComponent<Collider>() != null ? GetComponent<Collider>().bounds : new Bounds(transform.position, Vector3.one);
        Vector3 stepSize = bounds.size / (float)materialMatrixResolution;

        for (int x = 0; x < materialMatrixResolution; x++)
        {
            for (int y = 0; y < materialMatrixResolution; y++)
            {
                Vector3 localPoint = new Vector3(
                    (-bounds.extents.x) + (x * stepSize.x),
                    (-bounds.extents.y) + (y * stepSize.y),
                    0f // Simplified slicing array tracking profiles across plane matrices
                );

                CorrosionNode node = new CorrosionNode
                {
                    positionWS = transform.TransformPoint(localPoint),
                    localizedSalinity = Random.Range(0.1f, 0.5f) * environmentalSalinityModifier,
                    passivationShieldHp = 100.0f, // Starts perfectly shielded with Al2O3
                    intergranularStress = 0f,
                    isStructuralPointRuptured = false
                };
                materialMatrix.Add(node);
            }
        }
    }

    private void SimulateGalvanicCorrosionMatrix()
    {
        int rupturedNodes = 0;
        float totalSystemStrength = 0f;

        // Run deep structural parsing across entire alloy matrix arrays
        for (int i = 0; i < materialMatrix.Count; i++)
        {
            CorrosionNode node = materialMatrix[i];

            if (!node.isStructuralPointRuptured)
            {
                // Stage 1: Environmental chloride ions attack and dissolve the oxide passivation shield
                float shieldChemicalDepletion = node.localizedSalinity * Time.deltaTime * 2.0f;
                node.passivationShieldHp = Mathf.Max(0f, node.passivationShieldHp - shieldChemicalDepletion);

                // Stage 2: Once passivation hits 0%, active intergranular moisture pitting begins
                if (node.passivationShieldHp <= 0f)
                {
                    node.intergranularStress += Time.deltaTime * node.localizedSalinity * 5.0f;
                }

                // Stage 3: Stress point breakdown calculation
                if (node.intergranularStress >= 50f)
                {
                    node.isStructuralPointRuptured = true;
                }
            }

            if (node.isStructuralPointRuptured) rupturedNodes++;
            totalSystemStrength += (100f - node.intergranularStress);

            materialMatrix[i] = node; // Commit changes back to memory pipeline
        }

        // Process unified values to update shaders
        globalCorrosionProgress = (float)rupturedNodes / materialMatrix.Count;
        
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, globalCorrosionProgress);
        }

        // Dynamic Mechanical Modifications
        // As exfoliation eats the crystalline bonds, the physical rigidity (Yield Strength) drops.
        dynamicStructuralIntegrity = Mathf.Lerp(alloyYieldStrength, alloyYieldStrength * 0.08f, globalCorrosionProgress);

        // Physics Alteration: Aluminum becomes highly porous and light. We drop its mass as density decays.
        if (rb != null)
        {
            rb.mass = Mathf.Lerp(27.0f, 18.0f, globalCorrosionProgress); // Structural density loss
        }

        // Emit chalky hydroxide flaking particles during movement stress if heavily corroded
        if (globalCorrosionProgress > 0.45f && rb != null && rb.velocity.magnitude > 1.2f)
        {
            if (aluminumHydroxideDustFX != null && !aluminumHydroxideDustFX.isPlaying)
            {
                aluminumHydroxideDustFX.Play();
            }
        }

        // If over 65% of the alloy nodes suffer internal crystal fracturing, the entire structural matrix snaps
        if (globalCorrosionProgress >= 0.65f)
        {
            TriggerCatastrophicAlloyFailure();
        }
    }

    /// <summary>
    /// Higher Precision Structural Damage Input (e.g., Flak Explosions, Stress loads, Mechanical Strain)
    /// </summary>
    public void InduceMechanicalStressVector(Vector3 globalImpactPoint, float kineticEnergyJoules)
    {
        if (isObjectMechanicallyFailed) return;

        for (int i = 0; i < materialMatrix.Count; i++)
        {
            CorrosionNode node = materialMatrix[i];
            float physicalRange = Vector3.Distance(globalImpactPoint, node.positionWS);

            if (physicalRange < 2.0f) // 2-meter structural strain radius
            {
                // Stress is amplified heavily if the passivation layer is already gone
                float vulnerabilityFactor = node.passivationShieldHp <= 0f ? 3.0f : 1.0f;
                node.intergranularStress += (kineticEnergyJoules / (physicalRange + 0.1f)) * 0.01f * vulnerabilityFactor;

                if (node.intergranularStress >= 50f)
                {
                    node.isStructuralPointRuptured = true;
                }
                materialMatrix[i] = node;
            }
        }
    }

    private void TriggerCatastrophicAlloyFailure()
    {
        isObjectMechanicallyFailed = true;
        StopAllCoroutines();

        // Convert component architecture into a broken physical scrap asset
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply brittle sheer force collapse impulse vector
        rb.AddForce(Vector3.down * 10f, ForceMode.VelocityChange);
        rb.AddTorque(Random.onUnitSphere * 25f, ForceMode.Impulse);

        if (aluminumHydroxideDustFX != null)
        {
            ParticleSystem burst = Instantiate(aluminumHydroxideDustFX, transform.position, Quaternion.identity);
            var main = burst.main;
            main.startSizeMultiplier = 3.5f; // Massive chalky cloud release
            Destroy(burst.gameObject, 5.0f);
        }

        Debug.Log($"[METALLURGICAL STRUCTURAL COLLAPSE] Aluminum crystalline lattice sheared at entropy factor {globalCorrosionProgress * 100f}%. Object split.");
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
