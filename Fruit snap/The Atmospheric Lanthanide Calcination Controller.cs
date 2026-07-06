using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_PraseodymiumDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct IntergranularLatticeNode
    {
        public Vector3 localPosition;
        public float localizedAirExposure;    // Local moisture infiltration factor
        public float oxideVolumeSwell;        // Accumulation volume of the green crust
        public float intergranularShearStrain;// Internal interface stress from volume expansion (0 to 100 max)
        public bool isNodeSpalled;
    }

    [Header("Atmospheric Simulation Settings")]
    [Range(0f, 1f)] public float compiledSpallationProgress = 0f;
    [SerializeField] private int gridMatrixResolution = 10;
    [SerializeField] private float atmosphericHumidity = 1.4f; // Ambient moisture velocity multiplier

    [Header("Mechanical Mineral Rigidity")]
    [Tooltip("The initial structural load strength of pure praseodymium metal. It is soft and easily deformed.")]
    [SerializeField] private float pristineMetalYieldMPa = 150.0f; 
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem slateSubOxideFlakeFX; // Dark charcoal sub-metallic particles
    [SerializeField] private ParticleSystem greenOxideDustFX;     // Brittle, chalky pale-green debris flakes

    private List<IntergranularLatticeNode> calcinationMatrix = new List<IntergranularLatticeNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isAssetCompletelyPulverized = false;

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

        // Generate the 3D internal tracking layout
        InitializeCalcinationLattice();
    }

    void Update()
    {
        if (isAssetCompletelyPulverized) return;

        SimulateLanthanideCorrosionCycle();
    }

    private void InitializeCalcinationLattice()
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

                // External nodes interact with moisture immediately; core nodes track internal mass parameters
                float surfaceDistance = Vector3.Distance(Vector3.zero, localPoint) / bounds.extents.magnitude;

                IntergranularLatticeNode node = new IntergranularLatticeNode
                {
                    localPosition = localPoint,
                    localizedAirExposure = Mathf.Lerp(15.0f, 100.0f, surfaceDistance),
                    oxideVolumeSwell = 0f,
                    intergranularShearStrain = 0f,
                    isNodeSpalled = false
                };
                calcinationMatrix.Add(node);
            }
        }
    }

    private void SimulateLanthanideCorrosionCycle()
    {
        int spalledNodesCount = 0;
        float totalInterfaceStrain = 0f;

        for (int i = 0; i < calcinationMatrix.Count; i++)
        {
            IntergranularLatticeNode node = calcinationMatrix[i];

            // Stage 1: Moisture interaction triggers growth of the initial green oxide crust
            if (node.localizedAirExposure >= 25.0f && node.intergranularShearStrain <= 15f)
            {
                node.oxideVolumeSwell += Time.deltaTime * atmosphericHumidity * 4.6f;
            }

            // Stage 2: Continued oxygen exposure generates massive expansion mismatch strain
            if (node.oxideVolumeSwell >= 35.0f)
            {
                node.intergranularShearStrain += Time.deltaTime * (atmosphericHumidity * 5.4f);
            }

            // Stage 3: Strain limit breach triggers localized mechanical spallation flaking
            if (node.intergranularShearStrain >= 95.0f)
            {
                node.isNodeSpalled = true;
            }

            if (node.isNodeSpalled) spalledNodesCount++;
            totalInterfaceStrain += node.intergranularShearStrain;

            calcinationMatrix[i] = node; // Commit updated settings back to structural index heap
        }

        // Pass normalization factors directly down to the PBR GPU shader properties
        compiledSpallationProgress = (float)spalledNodesCount / calcinationMatrix.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, compiledSpallationProgress);
        }

        // Mechanical Structural Rigidity Adjustments
        // Praseodymium completely sheds its mechanical strength as it turns into loose, spalling powder layers.
        float normalizedStrainFactor = 1.0f - (totalInterfaceStrain / (calcinationMatrix.Count * 100f));
        dynamicStructuralIntegrity = Mathf.Lerp(pristineMetalYieldMPa * 0.01f, pristineMetalYieldMPa, normalizedStrainFactor);

        if (rb != null)
        {
            // Praseodymium has an elemental mass density of 6.77 g/cm³; mass drops as crumbling layers shed off
            rb.mass = Mathf.Lerp(67.0f, 35.0f, compiledSpallationProgress);
        }

        // Particle Management Loop: Sheds fine green scales if moved under advanced decay stress
        if (compiledSpallationProgress > 0.35f && rb != null && rb.linearVelocity.magnitude > 0.5f)
        {
            if (greenOxideDustFX != null && !greenOxideDustFX.isPlaying)
            {
                greenOxideDustFX.Play();
            }
        }

        // If advanced spallation disintegrates more than 60% of the lattice boundaries,
        // the remaining core experiences total catastrophic crumbling pulverization.
        if (compiledSpallationProgress >= 0.60f)
        {
            ExecuteCatastrophicLatticeFailure();
        }
    }

    /// <summary>
    /// Processes sudden heavy kinetic impacts (Pickaxe strikes, bullet impacts, mechanical stress testing)
    /// </summary>
    public void RegisterKineticDeformationImpact(Vector3 contactWorldPoint, float forceInputJoules)
    {
        if (isAssetCompletelyPulverized) return;

        Vector3 localStrike = transform.InverseTransformPoint(contactWorldPoint);

        for (int i = 0; i < calcinationMatrix.Count; i++)
        {
            IntergranularLatticeNode node = calcinationMatrix[i];
            float physicalRange = Vector3.Distance(localStrike, node.localPosition);

            if (physicalRange < 1.4f)
            {
                // Unstable oxide crusts feature near-zero shear resistance and flake off instantly under mechanical shocks
                float shearBrittlenessMultiplier = 1.0f + (node.intergranularShearStrain * 0.07f);
                node.intergranularShearStrain += (forceInputJoules / (physicalRange + 0.1f)) * 0.6f * shearBrittlenessMultiplier;

                if (node.intergranularShearStrain >= 95.0f)
                {
                    node.isNodeSpalled = true;
                    if (greenOxideDustFX != null) greenOxideDustFX.Emit(8);
                }
                
                calcinationMatrix[i] = node;
            }
        }
    }

    private void ExecuteCatastrophicLatticeFailure()
    {
        isAssetCompletelyPulverized = true;
        StopAllCoroutines();

        // Release the entity components completely into dynamic gravity rigid bodies
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply a brittle, sliding shear tumble impulse profile
        rb.AddForce(Vector3.down * 5f, ForceMode.VelocityChange);
        rb.AddTorque(Random.onUnitSphere * 40f, ForceMode.Impulse);

        if (greenOxideDustFX != null)
        {
            ParticleSystem fractureCloud = Instantiate(greenOxideDustFX, transform.position, Quaternion.identity);
            var main = fractureCloud.main;
            main.startSizeMultiplier = 3.0f; // Large burst cloud of peeling, crumbly green oxide chunks
            Destroy(fractureCloud.gameObject, 4.5f);
        }

        Debug.Log($"[LANTHANIDE MATRIX FAILURE] Praseodymium crystal core completely pulverized under internal oxide expansion at {compiledSpallationProgress * 100f}% decay progress.");
        Destroy(gameObject, 0.15f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
