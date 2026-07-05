using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_ZirconiumDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct CeramicStrainNode
    {
        public Vector3 localPosition;
        public float localizedThermalLoad;   // Node temperature state (20°C up to 1000°C oxidation sweep)
        public float blackOxideThickness;    // Protective sub-stoichiometric black layer volume
        public float whiteMonoclinicStrain;  // Internal compressive stress from phase expansion (0 to 100 max)
        public bool isNodeSpalled;
    }

    [Header("Thermo-Chemical Simulation")]
    [Range(0f, 1f)] public float combinedOxidationProgress = 0f;
    [SerializeField] private int materialMatrixResolution = 10;
    [SerializeField] private float thermalExposureVelocity = 650.0f; // Operational heat factor injection

    [Header("Mechanical Structural Yield")]
    [Tooltip("The initial physical yield threshold of the zirconium component before ceramic embrittlement.")]
    [SerializeField] private float pristineAlloyYieldMPa = 380.0f;
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem blackOxideScaleFX;   // Sharp, brittle black flake shards
    [SerializeField] private ParticleSystem whiteZirconiaDustFX; // Chalky, powdery white oxide clouds

    private List<CeramicStrainNode> structuralMatrix = new List<CeramicStrainNode>();
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
        InitializeCeramicStrainLattice();
    }

    void Update()
    {
        if (isStructurePermanentlyFailed) return;

        SimulateZirconiaPhaseTransformation();
    }

    private void InitializeCeramicStrainLattice()
    {
        Bounds bounds = GetComponent<Collider>() != null ? GetComponent<Collider>().bounds : new Bounds(transform.position, Vector3.one);
        Vector3 nodeSpacing = bounds.size / (float)materialMatrixResolution;

        for (int x = 0; x < materialMatrixResolution; x++)
        {
            for (int y = 0; y < materialMatrixResolution; y++)
            {
                Vector3 localPoint = new Vector3(
                    (-bounds.extents.x) + (x * nodeSpacing.x),
                    (-bounds.extents.y) + (y * nodeSpacing.y),
                    (-bounds.extents.z) + (Random.value * bounds.size.z)
                );

                // High boundary exposure nodes catch frictional/steam heat instantly
                float surfaceProximity = Vector3.Distance(Vector3.zero, localPoint) / bounds.extents.magnitude;

                CeramicStrainNode node = new CeramicStrainNode
                {
                    localPosition = localPoint,
                    localizedThermalLoad = Mathf.Lerp(25.0f, thermalExposureVelocity, surfaceProximity),
                    blackOxideThickness = 0f,
                    whiteMonoclinicStrain = 0f,
                    isNodeSpalled = false
                };
                structuralMatrix.Add(node);
            }
        }
    }

    private void SimulateZirconiaPhaseTransformation()
    {
        int spalledNodesCount = 0;
        float totalCrystallineStrain = 0f;

        for (int i = 0; i < structuralMatrix.Count; i++)
        {
            CeramicStrainNode node = structuralMatrix[i];

            // Stage 1: Temperatures rise above 400°C, growing the dense jet-black oxide layer
            if (node.localizedThermalLoad >= 400.0f && node.whiteMonoclinicStrain <= 10f)
            {
                node.blackOxideThickness += Time.deltaTime * 6.5f;
            }

            // Stage 2: Continued high-heat and oxygen absorption convert the black sheath into monoclinic white zirconia
            if (node.blackOxideThickness >= 50.0f)
            {
                // Volumetric expansion directly increments internal mechanical strain values
                node.whiteMonoclinicStrain += Time.deltaTime * (node.localizedThermalLoad * 0.02f);
            }

            // Stage 3: High Pilling-Bedworth strain threshold forces local ceramic spalling failure
            if (node.whiteMonoclinicStrain >= 90.0f)
            {
                node.isNodeSpalled = true;
            }

            if (node.isNodeSpalled) spalledNodesCount++;
            totalCrystallineStrain += node.whiteMonoclinicStrain;

            structuralMatrix[i] = node; // Sync struct updates back to stack memory array
        }

        // Map unified calculation metrics directly down to the PBR GPU shader properties
        combinedOxidationProgress = (float)spalledNodesCount / structuralMatrix.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, combinedOxidationProgress);
        }

        // Mechanical Structural Adjustments
        // Zirconium remains extraordinarily robust until the white oxide transformation cracks the metal core apart.
        float normalizedStrainFactor = 1.0f - (totalCrystallineStrain / (structuralMatrix.Count * 100f));
        dynamicStructuralIntegrity = Mathf.Lerp(pristineAlloyYieldMPa * 0.05f, pristineAlloyYieldMPa, normalizedStrainFactor);

        if (rb != null)
        {
            // Structural density drops slightly as the metal turns into porous, blistered ceramic nodules
            rb.mass = Mathf.Lerp(65.0f, 48.0f, combinedOxidationProgress);
        }

        // If over 60% of the internal grid nodes suffer extreme compressive phase fracture,
        // the remaining alloy framework completely shears and shatters apart.
        if (combinedOxidationProgress >= 0.60f)
        {
            ExecuteCatastrophicCeramicFailure();
        }
    }

    /// <summary>
    /// Processes localized heavy kinetic damage forces (Axe blows, projectile impacts, crushing weights)
    /// </summary>
    public void RegisterKineticDeformationImpact(Vector3 contactWorldPoint, float forceInputJoules)
    {
        if (isStructurePermanentlyFailed) return;

        Vector3 localStrike = transform.InverseTransformPoint(contactWorldPoint);

        for (int i = 0; i < structuralMatrix.Count; i++)
        {
            CeramicStrainNode node = structuralMatrix[i];
            float physicalRange = Vector3.Distance(localStrike, node.localPosition);

            if (physicalRange < 1.4f)
            {
                // Heavily strained, brittle ceramic nodes are completely unstable under kinetic shock vectors
                float strainVulnerabilityMultiplier = 1.0f + (node.whiteMonoclinicStrain * 0.08f);
                node.whiteMonoclinicStrain += (forceInputJoules / (physicalRange + 0.1f)) * 0.5f * strainVulnerabilityMultiplier;

                if (node.whiteMonoclinicStrain >= 90.0f)
                {
                    node.isNodeSpalled = true;
                    // Burst out a mixture of black and white oxide particles from the impact zone
                    if (blackOxideScaleFX != null) blackOxideScaleFX.Emit(4);
                    if (whiteZirconiaDustFX != null) whiteZirconiaDustFX.Emit(8);
                }
                
                structuralMatrix[i] = node;
            }
        }
    }

    private void ExecuteCatastrophicCeramicFailure()
    {
        isStructurePermanentlyFailed = true;
        StopAllCoroutines();

        // Convert the asset into active gravity simulation entities
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply a brittle, sharp fragmentation twisting tumble impulse profile
        rb.AddForce(Vector3.down * 8f, ForceMode.VelocityChange);
        rb.AddTorque(Random.onUnitSphere * 45f, ForceMode.Impulse);

        if (whiteZirconiaDustFX != null)
        {
            ParticleSystem fractureCloud = Instantiate(whiteZirconiaDustFX, transform.position, Quaternion.identity);
            var main = fractureCloud.main;
            main.startSizeMultiplier = 3.8f; // Blinding explosion of chalky white ceramic dust chunks
            Destroy(fractureCloud.gameObject, 5.0f);
        }

        Debug.Log($"[PILLING-BEDWORTH BURST] Zirconium crystal framework experienced structural failure due to white zirconia phase expansion at {combinedOxidationProgress * 100f}% total decay.");
        Destroy(gameObject, 0.15f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
