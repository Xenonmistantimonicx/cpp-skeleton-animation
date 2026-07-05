using UnityEngine;
using System.Collections;

public class AAA_TreeRotController : MonoBehaviour
{
    [Header("Decay Timeline")]
    [SerializeField] private float totalDecayDuration = 300f; // Long lifecycle decay
    [Range(0f, 1f)] public float rotProgress = 0f;

    [Header("Dynamic Physics Structural Integrity")]
    [Tooltip("The mechanical force required to snap this tree when healthy.")]
    [SerializeField] private float healthyMaxHealth = 500f;
    private float currentStructuralIntegrity;

    [Header("VFX Prefabs")]
    [SerializeField] private ParticleSystem woodDustCloudFX; // Spawns falling wood rot spores/particles

    private Material treeMaterial;
    private Rigidbody rb;
    private bool isSnapped = false;
    
    private static readonly int DecayProgressID = Shader.PropertyToID("_DecayProgress");

    void Start()
    {
        currentStructuralIntegrity = healthyMaxHealth;
        
        Renderer treeRenderer = GetComponent<Renderer>();
        if (treeRenderer != null)
        {
            treeMaterial = treeRenderer.material;
            treeMaterial.SetFloat(DecayProgressID, 0f);
        }

        // Start background decaying process
        StartCoroutine(ExecuteTreeDecayTimeline());
    }

    private IEnumerator ExecuteTreeDecayTimeline()
    {
        float elapsed = 0f;
        while (elapsed < totalDecayDuration && !isSnapped)
        {
            elapsed += Time.deltaTime;
            rotProgress = elapsed / totalDecayDuration;

            if (treeMaterial != null)
            {
                treeMaterial.SetFloat(DecayProgressID, rotProgress);
            }

            // AAA Feature: The weaker/rotted the wood gets, the less force it takes to break it down
            // At 100% rot, its structural strength is reduced to 5% of original baseline
            currentStructuralIntegrity = Mathf.Lerp(healthyMaxHealth, healthyMaxHealth * 0.05f, rotProgress);

            // Periodically drop subtle fungal spores or wood dust particles
            if (rotProgress > 0.4f && Random.value < 0.02f && woodDustCloudFX != null)
            {
                woodDustCloudFX.Play();
            }

            yield return null;
        }
    }

    /// <summary>
    /// Invoked when hit by outside environmental impacts (e.g., player kick, axe strike, rock impacts).
    /// </summary>
    public void TakeImpactDamage(float damageForce, Vector3 hitPoint, Vector3 forceDirection)
    {
        if (isSnapped) return;

        // Apply hit damage to remaining integrity pool
        currentStructuralIntegrity -= damageForce;

        if (currentStructuralIntegrity <= 0)
        {
            TriggerStructuralSnap(hitPoint, forceDirection);
        }
    }

    private void TriggerStructuralSnap(Vector3 hitPoint, Vector3 fallDirection)
    {
        isSnapped = true;
        StopAllCoroutines();

        // Convert static foliage to a dynamic falling rigid body physics asset
        rb = gameObject.GetComponent<Rigidbody>();
        if (rb == null) rb = gameObject.AddComponent<Rigidbody>();

        rb.mass = 250f; // Heavy weight profile
        rb.isKinematic = false;
        rb.useGravity = true;

        // Apply brittle rotational break force away from point of structural failure
        rb.AddForceAtPosition(fallDirection.normalized * 50f, hitPoint, ForceMode.Impulse);

        // Spawn a massive burst of powdery decayed wood dust upon snapping
        if (woodDustCloudFX != null)
        {
            ParticleSystem explosion = Instantiate(woodDustCloudFX, hitPoint, Quaternion.LookRotation(Vector3.up));
            Destroy(explosion.gameObject, 4f);
        }

        Debug.Log($"{gameObject.name} snapped cleanly due to heavy inner wood decay.");
    }
}
