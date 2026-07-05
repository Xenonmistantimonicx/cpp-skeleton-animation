using UnityEngine;
using System.Collections;

public class AAA_SilverTarnishController : MonoBehaviour
{
    [Header("Tarnish Progression Settings")]
    [Range(0f, 1f)] public float tarnishProgress = 0f;
    [SerializeField] private float standardTarnishDuration = 240.0f; // Seconds to turn completely black

    [Header("Environmental Chemistry")]
    [Tooltip("Is this silver item near sulfur sources (swamps, volcanic biomes, sewer levels, or rotten eggs)?")]
    public bool isInHighSulfurEnvironment = false;
    [Tooltip("Does handling the item keep edges polished via friction?")]
    public bool isActivelyHandled = false;

    private Material silverMaterial;
    private bool isTarnishing = false;

    private static readonly int TarnishProgressID = Shader.PropertyToID("_TarnishProgress");

    void Start()
    {
        Renderer rend = GetComponent<Renderer>();
        if (rend != null)
        {
            // Instance material to track tarnish uniquely per item
            silverMaterial = rend.material;
            silverMaterial.SetFloat(TarnishProgressID, tarnishProgress);
        }

        StartTarnishingSequence();
    }

    public void StartTarnishingSequence()
    {
        if (!isTarnishing)
        {
            isTarnishing = true;
            StartCoroutine(ExecuteTarnishLifecycle());
        }
    }

    private IEnumerator ExecuteTarnishLifecycle()
    {
        float elapsed = tarnishProgress * standardTarnishDuration;

        while (elapsed < standardTarnishDuration)
        {
            // Environmental modifiers
            float chemicalMultiplier = 1.0f;
            if (isInHighSulfurEnvironment) chemicalMultiplier *= 6.0f; // Swamp/volcanic gases accelerate decay instantly
            if (isActivelyHandled) chemicalMultiplier *= 0.3f;        // Player handling rub-polishes high-contact points

            elapsed += Time.deltaTime * chemicalMultiplier;
            tarnishProgress = Mathf.Clamp01(elapsed / standardTarnishDuration);

            if (silverMaterial != null)
            {
                silverMaterial.SetFloat(TarnishProgressID, tarnishProgress);
            }

            yield return null;
        }
    }

    /// <summary>
    /// AAA Polish Mechanic: Allows players to use a cleaning item, rag, or polish tool 
    /// to restore the weapon, coin, or goblet back to its pristine mirror-shiny state.
    /// </summary>
    public void ApplyPolishingAction(float polishEfficiency)
    {
        tarnishProgress = Mathf.Clamp01(tarnishProgress - polishEfficiency);
        if (silverMaterial != null)
        {
            silverMaterial.SetFloat(TarnishProgressID, tarnishProgress);
        }
    }

    private void OnDestroy()
    {
        if (silverMaterial != null) Destroy(silverMaterial);
    }
}
