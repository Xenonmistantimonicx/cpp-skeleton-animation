using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_ThoriumDecayController : MonoBehaviour
{
    [System.Serializable]
    public struct RadiolyticNode
    {
        public Vector3 localPosition;
        public float radiolyticDislocationHP; // Internal atomic order integrity (100 down to 0)
        public float oxygenExposureScale;     // Surface depth mapping for structural oxide growth
        public float oxideCrustVolume;        // Localized volume profile of carbonate salts
        public bool isNodeHoneycombed;
    }

    [Header("Radio-Chemical Environment")]
    [Range(0f, 1f)] public float combinedDecayProgress = 0f;
    [SerializeField] private int materialMatrixResolution = 10;
    [SerializeField] private float environmentalMoistureFactor = 1.2f;

    [Header("Dynamic Geiger Radiation Aura")]
    public float currentHazardRadius = 1.5f;
    [SerializeField] private SphereCollider geigerTriggerZone;

    [Header("Mechanical Structural Yield")]
    [Tooltip("The pristine elastic shear threshold of the thorium metal core.")]
    [SerializeField] private float baseAlloyCohesionStrength = 280.0f; // Simulated MegaPascals
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem grayCarbonateDustFX; // Powdery gray-white oxide flakes
    [SerializeField] private ParticleSystem heavyAshShedFX;        // Flaky charcoal oxide chunks

    private List<RadiolyticNode> thoriumLattice = new List<RadiolyticNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isObjectPermanentlyCrumbled = false;

    // Fast GPU Parameter Cache Hashes
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicStructuralIntegrity = baseAlloyCohesionStrength;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Procedurally populates the 3D internal actinide grid tracking coordinates
        InitializeThoriumMatrix();
    }

    void Update()
    {
        if (isObjectPermanentlyCrumbled) return;

        SimulateIsotopicRadiolyticDecay();
    }

    private void InitializeThoriumMatrix()
    {
        Bounds bounds = GetComponent<Collider>() != null ? GetComponent<Collider>().bounds : new Bounds(transform.position, Vector3.one);
        Vector3 stepSpacing = bounds.size / (float)materialMatrixResolution;

        for (int x = 0; x < materialMatrixResolution; x++)
        {
            for (int y = 0; y < materialMatrixResolution; y++)
            {
                Vector3 localPoint = new Vector3(
                    (-bounds.extents.x) + (x * stepSpacing.x),
                    (-bounds.extents.y) + (y * stepSpacing.y),
                    (-bounds.extents.z) + (Random.value * bounds.size.z)
                );

                // External boundary nodes encounter air instantly; core nodes experience constant alpha dislocation stress
                float coreDistance = Vector3.Distance(Vector3.zero, localPoint) / bounds.extents.magnitude;

                RadiolyticNode node = new RadiolyticNode
                {
                    localPosition = localPoint,
                    radiolyticDislocationHP = 100.0f,
                    oxygenExposureScale = Mathf.Clamp01(coreDistance) * environmentalMoistureFactor,
                    oxideCrustVolume = 0f,
                    isNodeHoneycombed = false
                };
                thoriumLattice.Add(node);
            }
        }
    }

    private void SimulateIsotopicRadiolyticDecay()
    {
        int honeycombedNodesCount = 0;
        float aggregateLatticeHealth = 0f;

        for (int i = 0; i < thoriumLattice.Count; i++)
        {
            RadiolyticNode node = thoriumLattice[i];

            if (node.radiolyticDislocationHP > 0f)
            {
                // Stage 1: Continuous alpha decay disrupts the crystalline structure (metamictization)
                float radiolyticDamage = Time.deltaTime * 2.2f;
                node.radiolyticDislocationHP = Mathf.Max(0f, node.radiolyticDislocationHP - radiolyticDamage);

                // Stage 2: Structural defects allow atmospheric oxygen to seep deeper, growing oxide/carbonate volumes
                if (node.radiolyticDislocationHP < 75f)
                {
                    node.oxideCrustVolume += Time.deltaTime * node.oxygenExposureScale * 4.8f;
                }

                // Stage 3: Micro-structural shearing checkpoint
                if (node.oxideCrustVolume >= 70f)
                {
                    node.isNodeHoneycombed = true;
                }
            }

            if (node.isNodeHoneycombed) honeycombedNodesCount++;
            aggregateLatticeHealth += node.radiolyticDislocationHP;

            thoriumLattice[i] = node; // Commit structural matrix updates back to memory heap array
        }

        // Map calculated progress factors directly down to the PBR GPU shader properties
        combinedDecayProgress = (float)honeycombedNodesCount / thoriumLattice.Count;
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, combinedDecayProgress);
        }

        // Gameplay Feature: Dynamic Radio-Hazard Scaling
        // As thorium breaks down into dusty, highly breathable oxides, the radiation zone expands
        currentHazardRadius = Mathf.Lerp(1.5f, 4.0f, combinedDecayProgress);
        if (geigerTriggerZone != null)
        {
            geigerTriggerZone.radius = currentHazardRadius;
        }

        // Mechanical Physics Integrity Updates
        // Pure thorium is dense and soft; alpha-honeycombed oxides lose cohesion completely.
        float normalizedHealthPct = aggregateLatticeHealth / (thoriumLattice.Count * 100f);
        dynamicStructuralIntegrity = Mathf.Lerp(baseAlloyCohesionStrength * 0.04f, baseAlloyCohesionStrength, normalizedHealthPct);

        if (rb != null)
        {
            // Thorium is heavy (Density ~ 11.7 g/cm³); mass reduces smoothly as it turns into porous ash shells
            rb.mass = Mathf.Lerp(55.0f, 32.0f, combinedDecayProgress);
        }

        // Periodic Ash Shedding FX
        if (combinedDecayProgress > 0.25f && heavyAshShedFX != null && rb != null && rb.velocity.magnitude > 0.5f)
        {
            if (!heavyAshShedFX.isPlaying) heavyAshShedFX.Play();
        }

        // If the radiolytic honeycombing compromises more than 60% of the nodes, 
        // the asset collapses under its own weight into crumbly ash chunks.
        if (combinedDecayProgress >= 0.60f)
        {
            ExecuteCatastrophicLatticeCollapse();
        }
    }

    /// <summary>
    /// Processes abrupt external impact shocks (Pickaxe strikes, heavy blunt forces, explosions)
    /// </summary>
    public void RegisterKineticDeformationStrike(Vector3 contactWorldPoint, float inputForceGiganewtons)
    {
        if (isObjectPermanentlyCrumbled) return;

        Vector3 localImpact = transform.InverseTransformPoint(contactWorldPoint);

        for (int i = 0; i < thoriumLattice.Count; i++)
        {
            RadiolyticNode node = thoriumLattice[i];
            float localizedRange = Vector3.Distance(localImpact, node.localPosition);

            if (localizedRange < 1.4f)
            {
                // Honeycombed, radiolytically damaged nodes have zero elasticity and shatter instantly
                float dislocationVulnerability = 1.0f + ((100f - node.radiolyticDislocationHP) * 0.05f);
                node.oxideCrustVolume += (inputForceGiganewtons / (localizedRange + 0.1f)) * dislocationVulnerability;

                if (node.oxideCrustVolume >= 70f)
                {
                    node.isNodeHoneycombed = true;
                }
                thoriumLattice[i] = node;
            }
        }
    }

    private void ExecuteCatastrophicLatticeCollapse()
    {
        isObjectPermanentlyCrumbled = true;
        StopAllCoroutines();

        // Release the entity completely into active gravity physics objects
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply a brittle, heavy crumbling rotation profile
        rb.AddTorque(Random.onUnitSphere * 30f, ForceMode.Impulse);

        if (grayCarbonateDustFX != null)
        {
            ParticleSystem collapseBurst = Instantiate(grayCarbonateDustFX, transform.position, Quaternion.identity);
            var main = collapseBurst.main;
            main.startSizeMultiplier = 4.0f; // Large burst cloud of powdery white-gray oxide dust particles
            Destroy(collapseBurst.gameObject, 5.0f);
        }

        Debug.Log($"[RADIOLYTIC FAILURE] Thorium actinide framework crumbled completely into porous carbonate dust at {combinedDecayProgress * 100f}% structural decay.");
        Destroy(gameObject, 0.15f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
