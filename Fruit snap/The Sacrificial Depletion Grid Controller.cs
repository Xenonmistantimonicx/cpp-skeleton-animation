using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class AAA_GalvanizedSteelController : MonoBehaviour
{
    [System.Serializable]
    public struct GalvanicNode
    {
        public Vector3 positionWS;
        public float sacrificialZincHP;      // Protective zinc shield coating percentage (100 down to 0)
        public float ironCoreOxidation;     // Underlying steel rust level (0 to 100 max rot)
        public float localizedSalinityScale; // Localized electrolyte density (pooling salt water/acid rain)
        public bool isShieldBreached;
    }

    [Header("Electro-Chemical Timeline Metrics")]
    [Range(0f, 1f)] public float combinedCorrosionProgress = 0f;
    [SerializeField] private int materialMatrixResolution = 12;
    [SerializeField] private float environmentAcidFactor = 1.2f;

    [Header("Mechanical Structural Breakdown")]
    [Tooltip("The tensile yield capacity of the inner carbon steel structural core.")]
    [SerializeField] private float steelStructuralYieldStrength = 400.0f; // Simulated MegaPascals
    private float dynamicStructuralIntegrity;

    [Header("AAA Volumetric FX Links")]
    [SerializeField] private ParticleSystem whiteRustPowderFX; // Dry zinc oxide flaking dust
    [SerializeField] private ParticleSystem redRustFlakeFX;    // Brittle orange iron rust chunks

    private List<GalvanicNode> galvanicMatrix = new List<GalvanicNode>();
    private Material instancedMaterial;
    private Rigidbody rb;
    private bool isStructurePermanentlyFailed = false;

    // Fast GPU Parameter Cache Hashes
    private static readonly int GlobalDecayFactorID = Shader.PropertyToID("_GlobalDecayFactor");

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        dynamicStructuralIntegrity = steelStructuralYieldStrength;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            instancedMaterial = rend.material;
            instancedMaterial.SetFloat(GlobalDecayFactorID, 0f);
        }

        // Procedurally populate the 3D sacrificial galvanic grid across bounding coordinates
        InitializeGalvanicMatrix();
    }

    void Update()
    {
        if (isStructurePermanPermanentlyFailed) return;

        SimulateSacrificialGalvanicCycle();
    }

    private void InitializeGalvanicMatrix()
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

                // Underside or recessed crevice zones retain stagnant saltwater pools, accelerating galvanic failure
                float poolingBias = Mathf.Lerp(1.6f, 0.4f, (localPoint.y + bounds.extents.y) / bounds.size.y);

                GalvanicNode node = new GalvanicNode
                {
                    positionWS = transform.TransformPoint(localPoint),
                    sacrificialZincHP = 100.0f, // Starts fully galvanized
                    ironCoreOxidation = 0f,
                    localizedSalinityScale = poolingBias * environmentAcidFactor,
                    isShieldBreached = false
                };
                galvanicMatrix.Add(node);
            }
        }
    }

    private void SimulateSacrificialGalvanicCycle()
    {
        int rupturedIronNodes = 0;
        float totalZincShieldIntegrity = 0f;
        float totalIronCoreRot = 0f;

        for (int i = 0; i < galvanicMatrix.Count; i++)
        {
            GalvanicNode node = galvanicMatrix[i];

            // Stage 1: Environmental electrolytes attack and dissolve the sacrificial zinc shield first
            if (node.sacrificialZincHP > 0f)
            {
                float zincDepletion = Time.deltaTime * node.localizedSalinityScale * 2.5f;
                node.sacrificialZincHP = Mathf.Max(0f, node.sacrificialZincHP - zincDepletion);
            }

            // Stage 2: Once the zinc coating drops to 0%, the node shield breaches and the underlying iron structural steel rusts aggressively
            if (node.sacrificialZincHP <= 0f)
            {
                node.isShieldBreached = true;
                float ironOxidationSpeed = Time.deltaTime * node.localizedSalinityScale * 4.2f;
                node.ironCoreOxidation = Mathf.Min(100f, node.ironCoreOxidation + ironOxidationSpeed);
            }

            if (node.ironCoreOxidation >= 50f) rupturedIronNodes++;
            totalZincShieldIntegrity += node.sacrificialZincHP;
            totalIronCoreRot += node.ironCoreOxidation;

            galvanicMatrix[i] = node; // Sync struct updates back to stack memory array
        }

        // Map unified node calculation metrics directly down to the PBR GPU shader properties
        float totalZincNormalized = totalZincShieldIntegrity / (galvanicMatrix.Count * 100f);
        float totalIronNormalized = totalIronCoreRot / (galvanicMatrix.Count * 100f);
        
        // Master progress balance blend driver tracking transitions from pristine (0) to white rust (0.4) to deep red iron collapse (1.0)
        combinedCorrosionProgress = (1.0f - totalZincNormalized) * 0.4f + (totalIronNormalized * 0.6f);
        
        if (instancedMaterial != null)
        {
            instancedMaterial.SetFloat(GlobalDecayFactorID, combinedCorrosionProgress);
        }

        // Mechanical Structural Engineering Changes
        // Zinc loss doesn't ruin load integrity, but red rust eats carbon steel, dropping yield boundaries completely.
        dynamicStructuralIntegrity = Mathf.Lerp(steelStructuralYieldStrength, steelStructuralYieldStrength * 0.05f, totalIronNormalized);

        // Particle Loop Management: Emits flaky red rust chunks if heavily oxidized under friction or motion stress
        if (totalIronNormalized > 0.35f && rb != null && rb.velocity.magnitude > 1.0f)
        {
            if (redRustFlakeFX != null && !redRustFlakeFX.isPlaying)
            {
                redRustFlakeFX.Play();
            }
        }

        // If over 60% of the internal iron nodes suffer deep core oxidation structural failure, 
        // the structural framing completely shears and breaks apart.
        if (totalIronNormalized >= 0.60f)
            TriggerCatastrophicSteelFailure();
    }

    /// <summary>
    /// Processes localized heavy kinetic damage forces (Axe blows, projectile impacts, crushing weights)
    /// </summary>
    public void RegisterKineticDeformationImpact(Vector3 contactWorldPoint, float forceInputJoules)
    {
        if (isStructurePermanentlyFailed) return;

        for (int i = 0; i < galvanicMatrix.Count; i++)
        {
            GalvanicNode node = galvanicMatrix[i];
            float physicalRange = Vector3.Distance(contactWorldPoint, node.positionWS);

            if (physicalRange < 1.5f)
            {
                float rangeFalloff = forceInputJoules / (physicalRange + 0.1f);
                
                // Kinetic distortion cracks open the protective zinc layer instantly if struck hard
                if (node.sacrificialZincHP > 0f)
                {
                    node.sacrificialZincHP = Mathf.Max(0f, node.sacrificialZincHP - rangeFalloff * 0.5f);
                    if (whiteRustPowderFX != null) whiteRustPowderFX.Emit(5);
                }
                else // If zinc shield was already gone, structural iron crushing damage is heavily amplified
                {
                    node.ironCoreOxidation = Mathf.Min(100f, node.ironCoreOxidation + rangeFalloff * 1.5f);
                }
                
                galvanicMatrix[i] = node;
            }
        }
    }

    private void TriggerCatastrophicSteelFailure()
    {
        isStructurePermanentlyFailed = true;
        StopAllCoroutines();

        // Convert the asset into active gravity simulation entities
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply a brittle sheer buckling tumble impulse profile
        rb.AddForce(Vector3.down * 12f, ForceMode.VelocityChange);
        rb.AddTorque(Random.onUnitSphere * 35f, ForceMode.Impulse);

        if (redRustFlakeFX != null)
        {
            ParticleSystem collapseBurst = Instantiate(redRustFlakeFX, transform.position, Quaternion.identity);
            var main = collapseBurst.main;
            main.startSizeMultiplier = 4.0f; // Massive spray of brittle, oxidized orange iron rust scaling fragments
            Destroy(collapseBurst.gameObject, 5.0f);
        }

        Debug.Log($"[SACRIFICIAL SHIELD CRUSHED] Galvanized steel structural matrix snapped under load due to iron oxidation at {combinedCorrosionProgress * 100f}% total decay.");
        Destroy(gameObject, 0.2f);
    }

    private void OnDestroy()
    {
        if (instancedMaterial != null) Destroy(instancedMaterial);
    }
}
