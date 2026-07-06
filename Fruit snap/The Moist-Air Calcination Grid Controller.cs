using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_EuropiumDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct CalcinationNode
    {
        public Vector3 localPosition;
        public float localizedAtmosphericExposure; // Exposure factor to ambient moist air
        public float hydroxideBloomVolume;       // Volume accumulation of white hydroxide scale
        public float trivalentOxideStrain;        // Internal mechanical shear stress from crystal transformation
        public bool isNodeCrumbled;
    }

    [Header("Thermo-Chemical Simulation")]
    [Range(0f, 1f)] public float combinedOxidationProgress = 0f;
    [SerializeField] private int materialMatrixResolution = 10;
    [SerializeField] private float airMoistureHumidity = 1.3f; // Atmospheric moisture scaling factor

    [Header("Mechanical Structural Yield")]
    [Tooltip("The initial structural yield capacity of the europium component. It is a soft, lead-like rare-earth metal.")]
    [SerializeField] private float pristineMetalYieldMPa = 160.0f;
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem whiteHydroxideScaleFX; // Brittle white chalky flakes
    [SerializeField] private ParticleSystem yellowOxideDustFX;     // Fine, powdery pastel-yellow dust trails

    private List<CalcinationNode> structuralMatrix = new List<CalcinationNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isStructurePermanentlyFailed = false;

    // Fast GPU Parameter Cache Hashes
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicStructuralIntegrity = pristineMetalYieldMPa;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Generate the 3D internal metallurgical tracking layout
        InitializeCalcinationLattice();
    }

    void Update()
    {
        if (isStructurePermanentlyFailed) return;

        SimulateLanthanideOxidationCycle();
    }

    private void InitializeCalcinationLattice()
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

                // High boundary exposure nodes catch moisture immediately
                float surfaceProximity = Vector3.Distance(Vector3.zero, localPoint) / bounds.extents.magnitude;

                CalcinationNode node = new CalcinationNode
                {
                    localPosition = localPoint,
                    localizedAtmosphericExposure = Mathf.Lerp(10.0f, 100.0f, surfaceProximity),
                    hydroxideBloomVolume = 0f,
                    trivalentOxideStrain = 0f,
                    isNodeCrumbled = false
                };
                structuralMatrix.Add(node);
            }
        }
    }

    private void SimulateLanthanideOxidationCycle()
    {
        int crumbledNodesCount = 0;
        float totalCrystallineStrain = 0f;

        for (int i = 0; i < structuralMatrix.Count; i++)
        {
            CalcinationNode node = structuralMatrix[i];

            // Stage 1: Moisture interaction triggers growth of the white hydroxide crust
            if (node.localizedAtmosphericExposure >= 30.0f && node.trivalentOxideStrain <= 12f)
            {
                node.hydroxideBloomVolume += Time.deltaTime * airMoistureHumidity * 4.8f;
            }

            // Stage 2: Hydroxide layers continuously oxidize further into the terminal trivalent powder
            if (node.hydroxideBloomVolume >= 45.0f)
            {
                // Volume mismatch generates high mechanical shear stress at the crystal face
                node.trivalentOxideStrain += Time.deltaTime * (airMoistureHumidity * 3.5f);
            }

            // Stage 3: High lattice strain causes local brittle crumbling failure
            if (node.trivalentOxideStrain >= 90.0f)
            {
                node.isNodeCrumbled = true;
            }

            if (node.isNodeCrumbled) crumbledNodesCount++;
            totalCrystallineStrain += node.trivalentOxideStrain;

            structuralMatrix[i] = node; // Sync struct updates back to core array memory
        }

        // Map calculation progress parameters directly down to the PBR GPU shader properties
        combinedOxidationProgress = (float)crumbledNodesCount / structuralMatrix.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, combinedOxidationProgress);
        }

        // Mechanical Structural Rigidity Adjustments
        // Europium completely drops its mechanical shear strength as it turns into loose powder layers.
        float normalizedStrainFactor = 1.0f - (totalCrystallineStrain / (structuralMatrix.Count * 100f));
        dynamicStructuralIntegrity = Mathf.Lerp(pristineMetalYieldMPa * 0.02f, pristineMetalYieldMPa, normalizedStrainFactor);

        if (rb != null)
        {
            // Europium has a low density for a rare-earth metal (5.26 g/cm³); mass drops as it sheds porous crust chunks
            rb.mass = Mathf.Lerp(52.0f, 30.0f, combinedOxidationProgress);
        }

        // Constant particle puffing when moving under advanced crumbling stress
        if (combinedOxidationProgress > 0.4f && rb != null && rb.linearVelocity.magnitude > 0.5f)
        {
            if (yellowOxideDustFX != null && !yellowOxideDustFX.isPlaying) yellowOxideDustFX.Play();
        }

        // If advanced crumbling disintegrates more than 58% of the core node lattice boundaries,
        // the remaining asset architecture collapses into fragmented powder debris.
        if (combinedOxidationProgress >= 0.58f)
        {
            ExecuteCatastrophicPowderCollapse();
        }
    }

    /// <summary>
    /// Processes physical impacts (Striking the decaying rare-earth asset with tools, weapons, or projectile kinetics)
    /// </summary>
    public void RegisterKineticDeformationImpact(Vector3 contactWorldPoint, float forceInputJoules)
    {
        if (isStructurePermanentlyFailed) return;

        Vector3 localStrike = transform.InverseTransformPoint(contactWorldPoint);

        for (int i = 0; i < structuralMatrix.Count; i++)
        {
            CalcinationNode node = structuralMatrix[i];
            float physicalRange = Vector3.Distance(localStrike, node.localPosition);

            if (physicalRange < 1.4f)
            {
                // Heavily calcined oxide layers feature near-zero shear resistance and crumble instantly under shock waves
                float strainBrittlenessMultiplier = 1.0f + (node.trivalentOxideStrain * 0.08f);
                node.trivalentOxideStrain += (forceInputJoules / (physicalRange + 0.1f)) * 0.6f * strainBrittlenessMultiplier;

                if (node.trivalentOxideStrain >= 90.0f)
                {
                    node.isNodeCrumbled = true;
                    // Shed out a messy cloud of white flakes and pastel-yellow dust from the impact point
                    if (whiteHydroxideScaleFX != null) whiteHydroxideScaleFX.Emit(5);
                    if (yellowOxideDustFX != null) yellowOxideDustFX.Emit(8);
                }
                
                structuralMatrix[i] = node;
            }
        }
    }

    private void ExecuteCatastrophicPowderCollapse()
    {
        isStructurePermanentlyFailed = true;
        StopAllCoroutines();

        // Release the entity layout into active gravity rigid body assets
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply a highly brittle, structural crumbling tumble profile
        rb.AddForce(Vector3.down * 6f, ForceMode.VelocityChange);
        rb.AddTorque(Random.onUnitSphere * 35f, ForceMode.Impulse);

        if (yellowOxideDustFX != null)
        {
            ParticleSystem fractureCloud = Instantiate(yellowOxideDustFX, transform.position, Quaternion.identity);
            var main = fractureCloud.main;
            main.startSizeMultiplier = 3.5f; // Large burst cloud of powdery yellow-pink ceramic debris chunks
            Destroy(fractureCloud.gameObject, 5.0f);
        }

        Debug.Log($"[LANTHANIDE MATRIX COLLAPSE] Europium crystal core completely crumbled due to atmospheric moisture conversion at {combinedOxidationProgress * 100f}% decay.");
        Destroy(gameObject, 0.15f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
