using UnityEngine;
using System.Collections;

public class AAA_BrassCorrosionController : MonoBehaviour
{
    [Header("Chemical Decay Timeline")]
    [Range(0f, 1f)] public float corrosionProgress = 0f;
    [SerializeField] private float timeToFullyCorrode = 150.0f; // Seconds under harsh environment

    [Header("Environmental Conditions")]
    [Tooltip("Is the object exposed to sweat, acid, sea salt, or carbon dioxide?")]
    public bool isExposedToAcids = true;

    [Header("Structural Integrity Loss")]
    [SerializeField] private float baseStructuralHealth = 500f;
    private float currentStructuralHealth;
    
    private Material brassMaterial;
    private Rigidbody rb;
    private bool isRuptured = false;

    // Shader Variable String Cache
    private static readonly int CorrosionProgressID = Shader.PropertyToID("_CorrosionProgress");

    void Start()
    {
        rb = GetComponent<Rigidbody>();
        currentStructuralHealth = baseStructuralHealth;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            // Create a unique instance of the material so items decay independently
            brassMaterial = rend.material;
            brassMaterial.SetFloat(CorrosionProgressID, corrosionProgress);
        }

        StartCoroutine(ExecuteBrassDecayLifecycle());
    }

    private IEnumerator ExecuteBrassDecayLifecycle()
    {
        float elapsed = corrosionProgress * timeToFullyCorrode;

        while (elapsed < timeToFullyCorrode && !isRuptured)
        {
            // Acid rain, handling by sweaty hands, or saltwater speeds up the chemical reaction heavily
            float environmentFactor = isExposedToAcids ? 2.5f : 1.0f;
            elapsed += Time.deltaTime * environmentFactor;
            
            corrosionProgress = Mathf.Clamp01(elapsed / timeToFullyCorrode);

            if (brassMaterial != null)
            {
                brassMaterial.SetFloat(CorrosionProgressID, corrosionProgress);
            }

            // AAA Mechanical Feature: Dezincification Damage
            // When brass loses its zinc content, it turns into porous copper.
            // It looks solid but can easily shatter or snap like clay under physical impact.
            currentStructuralHealth = Mathf.Lerp(baseStructuralHealth, baseStructuralHealth * 0.15f, corrosionProgress);

            // Dynamically lower the mass as zinc atoms literally dissolve out of the object matrix
            if (rb != null)
            {
                rb.mass = Mathf.Lerp(25.0f, 16.0f, corrosionProgress); // Structural weight reduction
            }

            yield return null;
        }
    }

    /// <summary>
    /// Called when the item takes physical impact forces (player kick, weapon blast, object drop).
    /// </summary>
    private void OnCollisionEnter(Collision collision)
    {
        if (isRuptured) return;

        float hitForce = collision.relativeVelocity.magnitude;

        // If the impact is greater than the weakened zinc-leached alloy threshold, break it
        if (hitForce >= currentStructuralHealth * 0.05f) 
        {
            TriggerAlloyFracture(collision.contacts[0].point);
        }
    }

    private void TriggerAlloyFracture(Vector3 impactPoint)
    {
        isRuptured = true;
        StopAllCoroutines();

        // Handle structural snapping or crumbling
        Debug.Log($"[MATERIAL CRISIS] Porous brass fractured due to high dezincification at {corrosionProgress * 100f}% decay.");
        
        // AAA Asset Workflow: Swap with a broken/crushed version or break the mesh into pieces here
        Destroy(gameObject);
    }

    private void OnDestroy()
    {
        if (brassMaterial != null) Destroy(brassMaterial);
    }
}
