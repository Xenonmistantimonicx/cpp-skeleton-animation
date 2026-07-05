using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_PyriteDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct CrystalLatticeNode
    {
        public Vector3 localPosition;
        public float pyriteIntegrity;      // Crystalline health level (100 down to 0)
        public float internalAcidPressure;  // Accumulating chemical expansion stress (0 to 100)
        public float moistureInfiltration; // Depth factor of moisture penetration
        public bool isLatticeFractured;
    }

    [Header("Chemical Evolution Settings")]
    [Range(0f, 1f)] public float compiledDecayProgress = 0f;
    [SerializeField] private int latticeMatrixResolution = 10;
    [SerializeField] private float environmentalHumidityScale = 1.2f;

    [Header("Structural Stress Engineering")]
    [Tooltip("The brittle cleavage strength of the mineral before it forcefully shatters.")]
    [SerializeField] private float crystalTensileLimit = 200.0f;
    private float dynamicStructuralStability;

    [Header("Dynamic FX Generation")]
    [SerializeField] private ParticleSystem sulfateDustFX;     // Powdery yellow-gray decay flakes
    [SerializeField] private ParticleSystem acidSmokeFX;       // White acidic moisture vapor smoke

    private List<CrystalLatticeNode> crystalLattice = new List<CrystalLatticeNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isMeshCompletelyPulverized = false;

    // Shader Variable Hash Cache
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicStructuralStability = crystalTensileLimit;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Generate the full internal spatial lattice tracing matrix
        InitializeCubicLattice();
    }

    void Update()
    {
        if (isMeshCompletelyPulverized) return;

        SimulatePyriteDiseaseLattice();
    }

    private void InitializeCubicLattice()
    {
        Bounds bounds = GetComponent<Collider>() != null ? GetComponent<Collider>().bounds : new Bounds(transform.position, Vector3.one);
        Vector3 nodeSpacing = bounds.size / (float)latticeMatrixResolution;

        for (int x = 0; x < latticeMatrixResolution; x++)
        {
            for (int y = 0; y < latticeMatrixResolution; y++)
            {
                Vector3 targetLocalPos = new Vector3(
                    (-bounds.extents.x) + (x * nodeSpacing.x),
                    (-bounds.extents.y) + (y * nodeSpacing.y),
                    (-bounds.extents.z) + (Random.value * bounds.size.z)
                );

                // Deep core nodes have slow moisture infiltration but high pressure retention
                float distanceFromSurface = Vector3.Distance(Vector3.zero, targetLocalPos);
                float calculatedInfiltration = Mathf.Clamp01(1.0f - (distanceFromSurface / bounds.extents.magnitude));

                CrystalLatticeNode node = new CrystalLatticeNode
                {
                    localPosition = targetLocalPos,
                    pyriteIntegrity = 100.0f,
                    internalAcidPressure = 0f,
                    moistureInfiltration = calculatedInfiltration * environmentalHumidityScale,
                    isLatticeFractured = false
                };
                crystalLattice.Add(node);
            }
        }
    }

    private void SimulatePyriteDiseaseLattice()
    {
        int rupturedNodes = 0;
        float aggregateStructurePool = 0f;

        for (int i = 0; i < crystalLattice.Count; i++)
        {
            CrystalLatticeNode node = crystalLattice[i];

            if (node.pyriteIntegrity > 0f)
            {
                // Stage 1: Moisture fuels the oxidation reaction, degrading pure pyrite crystal matrices
                float decayStep = Time.deltaTime * node.moistureInfiltration * 1.5f;
                node.pyriteIntegrity = Mathf.Max(0f, node.pyriteIntegrity - decayStep);

                // Stage 2: Volumetric Expansion. Oxides/sulfates take up more space, scaling internal stress pressure
                if (node.pyriteIntegrity < 80f)
                {
                    node.internalAcidPressure += Time.deltaTime * node.moistureInfiltration * 4.5f;
                }

                // Stage 3: Lattice Rupture Check
                if (node.internalAcidPressure >= 60f)
                {
                    node.isLatticeFractured = true;
                }
            }

            if (node.isLatticeFractured) rupturedNodes++;
            aggregateStructurePool += node.pyriteIntegrity;

            crystalLattice[i] = node; // Update structure stack in memory
        }

        // Pass normalization factors down to the GPU shader parameters
        compiledDecayProgress = (float)rupturedNodes / crystalLattice.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, compiledDecayProgress);
        }

        // Mechanical Physics Translation
        float normalizedHealthFactor = aggregateStructurePool / (crystalLattice.Count * 100f);
        dynamicStructuralStability = Mathf.Lerp(crystalTensileLimit * 0.02f, crystalTensileLimit, normalizedHealthFactor);

        // Acid Vapor FX logic: Spawns faint chemical vapor trails as the crystals actively dissolve
        if (compiledDecayProgress > 0.3f && acidSmokeFX != null && !acidSmokeFX.isPlaying)
        {
            acidSmokeFX.Play();
        }

        // If the internal expansion pressure causes over 55% of the crystalline lattice nodes to shear, 
        // the crystal structure experiences explosive structural disintegration.
        if (compiledDecayProgress >= 0.55f)
        {
            TriggerExplosiveCrystalFracture();
        }
    }

    /// <summary>
    /// Captures physical structural strikes (Pickaxe impacts, sonic waves, blunt weapon forces)
    /// </summary>
    public void RegisterKineticShockwave(Vector3 contactWorldPoint, float forceGiganewtons)
    {
        if (isMeshCompletelyPulverized) return;

        Vector3 localImpact = transform.InverseTransformPoint(contactWorldPoint);

        for (int i = 0; i < crystalLattice.Count; i++)
        {
            CrystalLatticeNode node = crystalLattice[i];
            float localizedRange = Vector3.Distance(localImpact, node.localPosition);

            if (localizedRange < 1.2f)
            {
                // Internal stress load is violently forced upward if nodes are already under pressure
                float stressAmplifier = 1.0f + (node.internalAcidPressure * 0.05f);
                node.internalAcidPressure += (forceGiganewtons / (localizedRange + 0.1f)) * stressAmplifier;

                if (node.internalAcidPressure >= 60f)
                {
                    node.isLatticeFractured = true;
                }
                crystalLattice[i] = node;
            }
        }
    }

    private void TriggerExplosiveCrystalFracture()
    {
        isMeshCompletelyPulverized = true;
        StopAllCoroutines();

        // Sever the asset and let physical debris take over
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply a sharp, jagged torque vector simulating a structural pop
        rb.AddTorque(Random.onUnitSphere * 60f, ForceMode.Impulse);

        if (sulfateDustFX != null)
        {
            ParticleSystem fractureCloud = Instantiate(sulfateDustFX, transform.position, Quaternion.identity);
            var main = fractureCloud.main;
            main.startSizeMultiplier = 4.5f; // Large release of toxic gray-yellow iron sulfate dust
            Destroy(fractureCloud.gameObject, 5.0f);
        }

        Debug.Log($"[PYRITE DISEASE CRITICAL] Internal expansion pressure breached crystal yield limit at {compiledDecayProgress * 100f}% structural failure. Rock disintegrated.");
        Destroy(gameObject, 0.15f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
