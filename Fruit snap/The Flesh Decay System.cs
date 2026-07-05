using UnityEngine;
using System.Collections;

public class AAA_CarcassDecayController : MonoBehaviour
{
    public enum DecayPhase { Fresh, Bloating, Collapsing, Skeletonized }
    
    [Header("Decay State Tracking")]
    public DecayPhase currentPhase = DecayPhase.Fresh;
    [Range(0f, 1f)] public float structuralDecayProgress = 0f;

    [Header("Timeline Settings")]
    [Tooltip("Total real-time seconds it takes for the meat to fully rot away.")]
    [SerializeField] private float totalDecayDuration = 120.0f;

    [Header("Environmental VFX Polish")]
    [SerializeField] private ParticleSystem flySwarmFX;
    [SerializeField] private ParticleSystem rotMethaneGasFX;

    // Rendering optimization fields
    private Material carcassMaterial;
    private bool isDecayActive = false;

    // Fast Shader IDs
    private static readonly int DecayStageID = Shader.PropertyToID("_DecayStage");

    void Start()
    {
        Renderer carcassRenderer = GetComponent<Renderer>();
        if (carcassRenderer != null)
        {
            // Instance the material so carcasses rot completely independently
            carcassMaterial = carcassRenderer.material;
            carcassMaterial.SetFloat(DecayStageID, 0f);
        }
    }

    /// <summary>
    /// Call this immediately when the animal or enemy NPC dies in the game world.
    /// </summary>
    public void InitiateCarcassDecay()
    {
        if (!isDecayActive)
        {
            isDecayActive = true;
            StartCoroutine(ProcessDecayTimeline());
        }
    }

    private IEnumerator ProcessDecayTimeline()
    {
        float elapsedTime = 0f;

        while (elapsedTime < totalDecayDuration)
        {
            elapsedTime += Time.deltaTime;
            structuralDecayProgress = elapsedTime / totalDecayDuration;

            // Send standard normalization factor (0 to 1) down to the HLSL pipeline
            if (carcassMaterial != null)
            {
                carcassMaterial.SetFloat(DecayStageID, structuralDecayProgress);
            }

            // Handle Contextual State Switching for Audio/VFX triggers
            UpdateDecayPhase(structuralDecayProgress);

            yield return null;
        }

        structuralDecayProgress = 1.0f;
        if (carcassMaterial != null) carcassMaterial.SetFloat(DecayStageID, 1.0f);
        currentPhase = DecayPhase.Skeletonized;
    }

    private void UpdateDecayPhase(float progress)
    {
        if (progress < 0.3f && currentPhase != DecayPhase.Bloating)
        {
            currentPhase = DecayPhase.Bloating;
            
            // Spawn dynamic methane gases leaking from the bloated tissues
            if (rotMethaneGasFX != null) rotMethaneGasFX.Play();
        }
        else if (progress >= 0.3f && progress < 0.8f && currentPhase != DecayPhase.Collapsing)
        {
            currentPhase = DecayPhase.Collapsing;

            // Spawn active persistent fly vectors buzzing around the rancid site
            if (flySwarmFX != null && !flySwarmFX.isPlaying)
            {
                ParticleSystem flies = Instantiate(flySwarmFX, transform.position + Vector3.up * 0.5f, Quaternion.identity);
                flies.transform.SetParent(this.transform);
            }
        }
    }

    private void OnDestroy()
    {
        // Prevent background memory leaks from instanced materials
        if (carcassMaterial != null)
        {
            Destroy(carcassMaterial);
        }
    }
}
