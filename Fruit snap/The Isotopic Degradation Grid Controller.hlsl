using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_TechnetiumDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct RadiolyticLatticeNode
    {
        public Vector3 localPosition;
        public float isotopicDecayHeat;     // Internal node temperature driven by self-irradiation
        public float heptoxideCrystalLevel; // Conversion volume into volatile pink-purple salt
        public float pertechneticAcidWeep;  // Liquefaction decay via deliquescence absorption
        public bool isNodeVaporized;
    }

    [Header("Radiolytic Grid Controls")]
    [Range(0f, 1f)] public float integratedIsotopicDecay = 0f;
    [SerializeField] private int latticeGridResolution = 10;
    [SerializeField] private float environmentalMoisture = 1.25f; // Relative humidity degradation multiplier

    [Header("Mechanical Core Rheology")]
    [Tooltip("The initial structural yield capacity of pure technetium metal. It handles load stresses similarly to rhenium or platinum.")]
    [SerializeField] private float pristineTensileStrengthMPa = 320.0f;
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem volatilePurpleGasVaporFX; // Toxic, pale-pinkish purple heptoxide sublime smoke plumes
    [SerializeField] private ParticleSystem acidCausticDripFX;         // Corrosive radioactive liquid droplets

    private List<RadiolyticLatticeNode> radiolyticMatrix = new List<RadiolyticLatticeNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isAssetCompletelySublimed = false;

    // Fast GPU Parameter Cache Hashes
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");
    private static readonly int RadioactiveFlickerTimeID = Shader.PropertyToID("_RadioactiveFlickerTime");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicStructuralIntegrity = pristineTensileStrengthMPa;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Generate the 3D internal metallurgical lattice layout
        InitializeRadiolyticLattice();
    }

    void Update()
    {
        // Drive high-frequency radiolytic energy flickers within the PBR shader instance
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(RadioactiveFlickerTimeID, Time.time + Random.Range(-0.02f, 0.02f));
        }

        if (isAssetCompletelySublimed) return;

        SimulateRadiolyticDecayCycle();
    }

    private void InitializeRadiolyticLattice()
    {
        Bounds bounds = GetComponent<Collider>() != null ? GetComponent<Collider>().bounds : new Bounds(transform.position, Vector3.one);
        Vector3 nodeSpacing = bounds.size / (float)latticeGridResolution;

        for (int x = 0; x < latticeGridResolution; x++)
        {
            for (int y = 0; y < latticeGridResolution; y++)
            {
                Vector3 localPoint = new Vector3(
                    (-bounds.extents.x) + (x * nodeSpacing.x),
                    (-bounds.extents.y) + (y * nodeSpacing.y),
                    (-bounds.extents.z) + (Random.value * bounds.size.z)
                );

                // Exterior nodes accumulate atmosphere and radiation spikes rapidly
                float surfaceProximity = Vector3.Distance(Vector3.zero, localPoint) / bounds.extents.magnitude;

                RadiolyticLatticeNode node = new RadiolyticLatticeNode
                {
                    localPosition = localPoint,
                    isotopicDecayHeat = Mathf.Lerp(25.0f, 65.0f, surfaceProximity), // Instantly manifests self-irradiation heating on exterior boundaries
                    heptoxideCrystalLevel = 0f,
                    pertechneticAcidWeep = 0f,
                    isNodeVaporized = false
                };
                radiolyticMatrix.Add(node);
            }
        }
    }

    private void SimulateRadiolyticDecayCycle()
    {
        int vaporizedNodesCount = 0;
        float totalAcidVolume = 0f;

        for (int i = 0; i < radiolyticMatrix.Count; i++)
        {
            RadiolyticLatticeNode node = radiolyticMatrix[i];

            // Stage 1: Isotopic self-heating compounds with atmospheric oxygen exposure
            node.isotopicDecayHeat += Time.deltaTime * 3.2f;

            // Stage 2: Above 50°C, the metal converts into the volatile heptoxide pink crystal skin
            if (node.isotopicDecayHeat >= 50.0f && node.pertechneticAcidWeep <= 15f)
            {
                node.heptoxideCrystalLevel += Time.deltaTime * 4.8f;
            }

            // Stage 3: Hygroscopic threshold reached; crystals turn into pertechnetic acid trails while vaporizing into gas
            if (node.heptoxideCrystalLevel >= 45.0f)
            {
                node.pertechneticAcidWeep += Time.deltaTime * environmentalMoisture * 5.4f;
            }

            // Stage 4: Absolute structural liquidation and sublimation conversion
            if (node.pertechneticAcidWeep >= 95.0f)
            {
                node.isNodeVaporized = true;
            }

            if (node.isNodeVaporized) vaporizedNodesCount++;
            totalAcidVolume += node.pertechneticAcidWeep;

            radiolyticMatrix[i] = node; // Commit updated struct metrics back to index layout heap
        }

        // Map computed radiolytic timelines directly down to the PBR GPU material parameters
        integratedIsotopicDecay = (float)vaporizedNodesCount / radiolyticMatrix.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, integratedIsotopicDecay);
        }

        // Mechanical Structural Load Remapping
        dynamicStructuralIntegrity = Mathf.Lerp(pristineTensileStrengthMPa, pristineTensileStrengthMPa * 0.005f, integratedIsotopicDecay);

        if (rb != null)
        {
            // Pure technetium is heavy (Density 11.5 g/cm³); mass drops aggressively as heptoxide sublimes straight into gas lines
            rb.mass = Mathf.Lerp(115.0f, 45.0f, integratedIsotopicDecay);
            rb.linearDamping = Mathf.Lerp(0.05f, 3.5f, integratedIsotopicDecay); // Simulates pitted drag deformation resistance
        }

        // Volatile Heptoxide Smoke Emission Management Loop
        if (integratedIsotopicDecay > 0.15f && integratedIsotopicDecay < 0.75f && volatilePurpleGasVaporFX != null)
        {
            if (!volatilePurpleGasVaporFX.isPlaying) volatilePurpleGasVaporFX.Play();
            
            var emissionMod = volatilePurpleGasVaporFX.emission;
            emissionMod.rateOverTimeMultiplier = Mathf.Lerp(10f, 80f, integratedIsotopicDecay);
        }
        else if (integratedIsotopicDecay >= 0.75f && volatilePurpleGasVaporFX != null && volatilePurpleGasVaporFX.isPlaying)
        {
            volatilePurpleGasVaporFX.Stop();
        }

        // If radioactive weeping liquidates more than 65% of the tracking network nodes,
        // the remaining structural shell asset experiences total material dissolution collapse.
        if (integratedIsotopicDecay >= 0.65f)
        {
            ExecuteAbsoluteRadiolyticMeltdown();
        }
    }

    /// <summary>
    /// Processes physical impacts (Shocks from blunt force mining, heavy projectile strikes, or shattering kinetic loads)
    /// </summary>
    public void RegisterKineticDeformationStrike(Vector3 contactWorldPoint, float forceInputJoules)
    {
        if (isAssetCompletelySublimed) return;

        Vector3 localImpact = transform.InverseTransformPoint(contactWorldPoint);

        // Splat radioactive caustic drops from the impact vector if wet paths are established
        if (integratedIsotopicDecay > 0.4f && acidCausticDripFX != null)
        {
            acidCausticDripFX.transform.position = contactWorldPoint;
            acidCausticDripFX.Emit((int)(forceInputJoules * 0.4f));
        }

        for (int i = 0; i < radiolyticMatrix.Count; i++)
        {
            RadiolyticLatticeNode node = radiolyticMatrix[i];
            float interactionRange = Vector3.Distance(localImpact, node.localPosition);

            if (interactionRange < 1.3f)
            {
                // Blunt shock waves fracture the brittle, decaying dioxide shell, venting pressurized isotopic gases
                node.isotopicDecayHeat += forceInputJoules * 2.5f;
                radiolyticMatrix[i] = node;
            }
        }
    }

    private void ExecuteAbsoluteRadiolyticMeltdown()
    {
        isAssetCompletelySublimed = true;
        StopAllCoroutines();

        // Dissolve the physical entity map completely out of the navigation mesh loops
        if (rb != null)
        {
            rb.isKinematic = true; 
            GetComponent<Collider>().enabled = false;
        }

        if (volatilePurpleGasVaporFX != null)
        {
            ParticleSystem explosionCloud = Instantiate(volatilePurpleGasVaporFX, transform.position, Quaternion.identity);
            var mainMod = explosionCloud.main;
            mainMod.startSizeMultiplier = 3.5f; // Blinding flash plume of toxic pink-purple sublimed heptoxide vapor
            Destroy(explosionCloud.gameObject, 5.0f);
        }

        Debug.Log($"[RADIOLYTIC TRANSFORMATION COMPLETE] Technetium metal framework completely sublimed and dissolved flat at {integratedIsotopicDecay * 100f}% radioactive decay path.");
        Destroy(gameObject, 0.2f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
