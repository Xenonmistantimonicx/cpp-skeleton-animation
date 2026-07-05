using UnityEngine;
using System.Collections;

[RequireComponent(typeof(Rigidbody))]
[RequireComponent(typeof(AudioSource))]
public class AAA_EnvironmentalStone : MonoBehaviour
{
    [Header("Physics & Motion Tuning")]
    [SerializeField] private float mass = 15.0f; // AAA stones have realistic weight
    [SerializeField] private float rollingBrakeFactor = 0.5f; 
    [SerializeField] private float minimumKickVelocity = 2.0f;

    [Header("Visual Effects (VFX)")]
    [SerializeField] private ParticleSystem impactDustPrefab;
    [SerializeField] private ParticleSystem rollingDebrisPrefab;
    [SerializeField] private GameObject impactDecalPrefab; // For leaving scuffs/cracks

    [Header("Audio Effects (SFX)")]
    [SerializeField] private AudioClip[] impactSounds;
    [SerializeField] private AudioClip rollingSoundLoop;
    
    // Internal States
    private Rigidbody rb;
    private AudioSource audioSource;
    private ParticleSystem activeRollingDebris;
    private bool isRolling = false;
    private float lastImpactTime;

    void Awake()
    {
        // Initialize AAA Physics Properties
        rb = GetComponent<Rigidbody>();
        audioSource = GetComponent<AudioSource>();
        
        rb.mass = mass;
        rb.collisionDetectionMode = CollisionDetectionMode.ContinuousDynamic; // Prevents clipping at high speeds
        rb.drag = 0.2f;          // Air resistance
        rb.angularDrag = 0.5f;   // Simulates non-perfectly-spherical friction
    }

    void Update()
    {
        HandleRollingEffects();
    }

    /// <summary>
    /// Triggered by a player kick, explosion, or melee strike.
    /// </summary>
    public void DisplaceStone(Vector3 forceDirection, float forceMagnitude, Vector3 hitPoint)
    {
        // Wake up physics if sleeping
        if (rb.IsSleeping()) rb.WakeUp();

        // Calculate realistic force application (Linear + Angular Torque for organic rolling)
        Vector3 appliedForce = forceDirection.normalized * forceMagnitude;
        rb.AddForceAtPosition(appliedForce, hitPoint, ForceMode.Impulse);

        // Procedural angular velocity twist based on hit location deviation from center of mass
        Vector3 torqueDir = Vector3.Cross(hitPoint - transform.position, forceDirection);
        rb.AddTorque(torqueDir * forceMagnitude * 0.3f, ForceMode.Impulse);

        // Immediate visual/audio response to the kick
        if (forceMagnitude > minimumKickVelocity)
        {
            SpawnImpactEffects(hitPoint, forceMagnitude);
        }
    }

    private void OnCollisionEnter(Collision collision)
    {
        // Prevent sound spamming on micro-collisions
        if (Time.time - lastImpactTime < 0.1f) return;

        float impactVelocity = collision.relativeVelocity.magnitude;

        if (impactVelocity > 1.5f)
        {
            lastImpactTime = Time.time;
            Vector3 contactPoint = collision.contacts[0].point;
            
            // AAA Feature: Dynamically scale audio pitch and volume by impact intensity
            float volume = Mathf.Clamp01(impactVelocity / 10f);
            audioSource.PlayOneShot(impactSounds[Random.Range(0, impactSounds.Length)], volume);

            // Spawn dynamic impact dust/decals
            SpawnImpactEffects(contactPoint, impactVelocity);
            
            // AAA Environmental Interaction: Flatten foliage/grass if colliding with nature layers
            if (collision.gameObject.CompareTag("Foliage"))
            {
                collision.gameObject.SendMessage("OnFlatten", transform.position, SendingOptions.DontRequireReceiver);
            }
        }
    }

    private void HandleRollingEffects()
    {
        float speed = rb.velocity.magnitude;
        float angularSpeed = rb.angularVelocity.magnitude;

        // Condition to check if stone is actively rolling across a surface
        if (speed > 0.5f && angularSpeed > 0.5f && IsGrounded())
        {
            if (!isRolling)
            {
                isRolling = true;
                
                // Play and loop rolling audio
                if (rollingSoundLoop != null)
                {
                    audioSource.clip = rollingSoundLoop;
                    audioSource.loop = true;
                    audioSource.Play();
                }

                // Instantiate dynamic rolling dust particles trailing the stone
                if (rollingDebrisPrefab != null && activeRollingDebris == null)
                {
                    activeRollingDebris = Instantiate(rollingDebrisPrefab, transform.position, Quaternion.identity);
                    activeRollingDebris.transform.SetParent(this.transform);
                }
            }

            // AAA Polish: Modulate audio pitch/volume and particle emission based on actual speed
            if (audioSource.loop)
            {
                audioSource.volume = Mathf.Clamp01(speed / 5f);
                audioSource.pitch = Mathf.Lerp(0.8f, 1.2f, speed / 5f);
            }
        }
        else
        {
            // Stop effects smoothly when coming to a rest
            if (isRolling)
            {
                isRolling = false;
                audioSource.loop = false;
                audioSource.Stop();
                if (activeRollingDebris != null) Destroy(activeRollingDebris.gameObject, 1.0f);
            }
        }
    }

    private void SpawnImpactEffects(Vector3 point, float intensity)
    {
        if (impactDustPrefab != null)
        {
            ParticleSystem dust = Instantiate(impactDustPrefab, point, Quaternion.LookRotation(Vector3.up));
            var main = dust.main;
            main.startSizeMultiplier = Mathf.Clamp(intensity * 0.2f, 0.5f, 2.0f); // Scale size with force
            Destroy(dust.gameObject, 2.0f);
        }

        if (impactDecalPrefab != null && intensity > 5.0f)
        {
            // Leaves a dynamic scuff mark/crack on the surface or stone
            GameObject decal = Instantiate(impactDecalPrefab, point, Quaternion.LookRotation(-Vector3.up));
            Destroy(decal, 15.0f); // Clean up memory over time
        }
    }

    private bool IsGrounded()
    {
        // Simple down-raycast to ensure rolling effects don't play while the stone is flying through the air
        return Physics.Raycast(transform.position, Vector3.down, (transform.localScale.y * 0.6f));
    }
}
