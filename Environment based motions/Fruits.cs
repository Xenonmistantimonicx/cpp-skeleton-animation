using UnityEngine;
using System.Collections;

[RequireComponent(typeof(Rigidbody))]
[RequireComponent(typeof(Collider))]
[RequireComponent(typeof(AudioSource))]
public class AAA_FallingFruit : MonoBehaviour
{
    public enum FruitState { Attached, Wobbling, Falling, Grounded }
    [Header("Fruit State")]
    public FruitState currentState = FruitState.Attached;

    [Header("Detachment Settings")]
    [SerializeField] private float StructuralIntegrity = 100f;
    [SerializeField] private float windSusceptibility = 1.5f;
    [SerializeField] private float detachmentThreshold = 0f;

    [Header("Juice & Impact VFX")]
    [SerializeField] private ParticleSystem splatJuiceFX;
    [SerializeField] private GameObject dynamicSplatDecal;
    [SerializeField] private float crushVelocityThreshold = 8.0f;

    [Header("Audio")]
    [SerializeField] private AudioClip[] rustleSounds;
    [SerializeField] private AudioClip[] splatSounds;

    private Rigidbody rb;
    private Collider fruitCollider;
    private AudioSource audioSource;
    private Transform branchAttachmentPoint;
    private Vector3 localAttachmentOffset;
    private float currentWobbleTime = 0f;
    private Vector3 originalLocalScale;

    void Awake()
    {
        rb = GetComponent<Rigidbody>();
        fruitCollider = GetComponent<Collider>();
        audioSource = GetComponent<AudioSource>();
        originalLocalScale = transform.localScale;

        // Initialize as static foliage until woken up or triggered
        rb.isKinematic = true;
        fruitCollider.isTrigger = true; 
    }

    /// <summary>
    /// Binds the fruit dynamically to a moving or wind-animated tree branch mesh.
    /// </summary>
    public void InitializeAttachment(Transform branch)
    {
        branchAttachmentPoint = branch;
        localAttachmentOffset = branch.InverseTransformPoint(transform.position);
        currentState = FruitState.Attached;
    }

    void Update()
    {
        if (currentState == FruitState.Attached)
        {
            SimulateBranchAttachment();
        }
        else if (currentState == FruitState.Wobbling)
        {
            SimulateWobbleAndSnap();
        }
    }

    private void SimulateBranchAttachment()
    {
        if (branchAttachmentPoint == null) return;
        
        // Sync position flawlessly with the animated/procedural wind tree branches
        transform.position = branchAttachmentPoint.TransformPoint(localAttachmentOffset);
    }

    /// <summary>
    /// Called when an explosion force, player melee strike, or high storm wind hits the tree.
    /// </summary>
    public void ApplyEnvironmentalStress(float damageForce)
    {
        if (currentState != FruitState.Attached) return;

        StructuralIntegrity -= damageForce;
        
        if (StructuralIntegrity <= detachmentThreshold)
        {
            StartCoroutine(InitiateDetachmentSequence());
        }
    }

    private IEnumerator InitiateDetachmentSequence()
    {
        currentState = FruitState.Wobbling;
        
        // Dynamic Audio feedback for structural failure (stem snapping)
        if (rustleSounds.Length > 0)
            audioSource.PlayOneShot(rustleSounds[Random.Range(0, rustleSounds.Length)], 0.6f);

        yield return null;
    }

    private void SimulateWobbleAndSnap()
    {
        currentWobbleTime += Time.deltaTime * 25f;
        
        // Procedural high-frequency oscillation calculation mimicking stem snap physics
        float wobbleOffset = Mathf.Sin(currentWobbleTime) * 0.15f * windSusceptibility;
        transform.position += new Vector3(wobbleOffset, 0, wobbleOffset);

        // Degrade structural integrity to zero rapidly during wobble phase
        StructuralIntegrity -= Time.deltaTime * 150f;

        if (StructuralIntegrity <= -20f)
        {
            ExecutePhysicalDrop();
        }
    }

    private void ExecutePhysicalDrop()
    {
        currentState = FruitState.Falling;
        branchAttachmentPoint = null;

        // Convert to physical entity affected by gravity
        rb.isKinematic = false;
        fruitCollider.isTrigger = false;
        rb.collisionDetectionMode = CollisionDetectionMode.Continuous;

        // Inherit dynamic physics velocity from wind/tree shakes if applicable
        rb.AddForce(new Vector3(Random.Range(-1.5f, 1.5f), -0.5f, Random.Range(-1.5f, 1.5f)), ForceMode.Impulse);
        rb.AddTorque(Random.onUnitSphere * 10f, ForceMode.Impulse);
        
        // Register with optimization manager to prevent cluttering
        AAA_FruitManager.Instance.TrackActiveFruit(this);
    }

    private void OnCollisionEnter(Collision collision)
    {
        if (currentState != FruitState.Falling) return;

        float impactVelocity = collision.relativeVelocity.magnitude;

        // AAA Squish Logic: If impact is massive, destroy fruit and leave a dynamic splat decal
        if (impactVelocity >= crushVelocityThreshold)
        {
            TriggerFruitSplat(collision.contacts[0].point, collision.contacts[0].normal);
        }
        else if (impactVelocity > 1.5f)
        {
            // Normal bounce sound play
            currentState = FruitState.Grounded;
            if (splatSounds.Length > 0)
                audioSource.PlayOneShot(splatSounds[0], impactVelocity / crushVelocityThreshold);
        }
    }

    private void TriggerFruitSplat(Vector3 contactPoint, Vector3 surfaceNormal)
    {
        currentState = FruitState.Grounded;

        // Audio
        if (splatSounds.Length > 0)
            AudioSource.PlayClipAtPoint(splatSounds[Random.Range(0, splatSounds.Length)], contactPoint, 1.0f);

        // VFX Particles
        if (splatJuiceFX != null)
        {
            ParticleSystem fx = Instantiate(splatJuiceFX, contactPoint, Quaternion.LookRotation(surfaceNormal));
            Destroy(fx.gameObject, 3f);
        }

        // Environment Decal (Stains the ground permanently/semi-permanently)
        if (dynamicSplatDecal != null)
        {
            GameObject decal = Instantiate(dynamicSplatDecal, contactPoint + (surfaceNormal * 0.01f), Quaternion.LookRotation(-surfaceNormal));
            Destroy(decal, 30f); // Clean up decal after 30 seconds
        }

        // Scaled squash deformation effect before full memory cleanup
        StartCoroutine(SquashDeformationAnimation());
    }

    private IEnumerator SquashDeformationAnimation()
    {
        rb.isKinematic = true;
        fruitCollider.enabled = false;

        // Flatten the scale along local Y axis rapidly to simulate flattening
        Vector3 squashedScale = new Vector3(originalLocalScale.x * 1.4f, originalLocalScale.y * 0.15f, originalLocalScale.z * 1.4f);
        float elapsed = 0;
        
        while (elapsed < 0.15f)
        {
            transform.localScale = Vector3.Lerp(originalLocalScale, squashedScale, elapsed / 0.15f);
            elapsed += Time.deltaTime;
            yield return null;
        }

        // Clean up from scene
        AAA_FruitManager.Instance.UnregisterFruit(this);
        Destroy(gameObject);
    }
}
