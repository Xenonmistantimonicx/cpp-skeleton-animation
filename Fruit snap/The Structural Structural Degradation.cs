using UnityEngine;
using System.Collections;

[RequireComponent(typeof(Rigidbody))]
public class AAA_IronRustController : MonoBehaviour
{
    [Header("Oxidation Progression")]
    [Range(0f, 1f)] public float rustProgress = 0f;
    [SerializeField] private float timeToFullyRust = 120.0f; // Seconds under standard moisture

    [Header("Mechanical Structural Degradation")]
    [SerializeField] private float pristineBreakThreshold = 1000f; // Force required to shatter it when new
    private float currentBreakThreshold;

    [Header("Environmental Flaking VFX")]
    [SerializeField] private ParticleSystem rustFlakesFX; // Spawns crumbling brown dust flakes

    private Material ironMaterial;
    private Rigidbody rb;
    private bool isShattered = false;

    private static readonly int RustProgressID = Shader.PropertyToID("_DynamicRustProgress");

    void Start()
    {
        rb = GetComponent<Rigidbody>();
        currentBreakThreshold = pristineBreakThreshold;

        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            ironMaterial = rend.material;
            ironMaterial.SetFloat(RustProgressID, rustProgress);
        }

        StartCoroutine(ExecuteRustingLifecycle());
    }

    private IEnumerator ExecuteRustingLifecycle()
    {
        float elapsed = rustProgress * timeToFullyRust;

        while (elapsed < timeToFullyRust && !isShattered)
        {
            elapsed += Time.deltaTime;
            rustProgress = Mathf.Clamp01(elapsed / timeToFullyRust);

            if (ironMaterial != null)
            {
                ironMaterial.SetFloat(RustProgressID, rustProgress);
            }

            // AAA Mechanical Feature: Degradation Physics
            // As iron becomes brittle, the force required to break or snap it drops drastically.
            currentBreakThreshold = Mathf.Lerp(pristineBreakThreshold, pristineBreakThreshold * 0.1f, rustProgress);

            // Dynamically alter physical friction profiles over time
            // Rust is highly abrasive compared to slick, polished iron metal sheets
            if (rb != null)
            {
                rb.angularDrag = Mathf.Lerp(0.05f, 0.8f, rustProgress); // Makes it harder to roll/spin
            }

            // Periodically emit subtle crumbling flakes if the object is moving/vibrating
            if (rustProgress > 0.5f && rb != null && rb.velocity.magnitude > 0.5f)
            {
                if (rustFlakesFX != null && !rustFlakesFX.isPlaying) rustFlakesFX.Play();
            }

            yield return null;
        }
    }

    /// <summary>
    /// Triggered by dynamic physical collisions or weapons fire hitting this iron object.
    /// </summary>
    private void OnCollisionEnter(Collision collision)
    {
        if (isShattered) return;

        float impactForce = collision.relativeVelocity.magnitude * (rb != null ? rb.mass : 1.0f);

        // If the impact exceeds the current rust-compromised stress threshold, shatter the asset
        if (impactForce >= currentBreakThreshold)
        {
            ShatterBrittleIron(collision.contacts[0].point);
        }
    }

    private void ShatterBrittleIron(Vector3 point)
    {
        isShattered = true;
        StopAllCoroutines();

        // Spawn massive explosion of dry, rusted iron particulate dust clouds
        if (rustFlakesFX != null)
        {
            ParticleSystem cloud = Instantiate(rustFlakesFX, point, Quaternion.identity);
            var main = cloud.main;
            main.startSizeMultiplier = 2.5f; // Scale up impact cloud size
            Destroy(cloud.gameObject, 4.0f);
        }

        // AAA System Integration: 
        // Swap this object out for broken geometry chunks, or cleanly disintegrate the asset mesh
        Debug.Log($"[CRITICAL STRUCTURAL FAILURE] Iron component structural lattice split at {rustProgress * 100f}% corrosion.");
        Destroy(gameObject);
    }

    private void OnDestroy()
    {
        if (ironMaterial != null) Destroy(ironMaterial);
    }
}
