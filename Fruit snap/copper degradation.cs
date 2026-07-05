using UnityEngine;
using System.Collections;

public class AAA_CopperOxidationController : MonoBehaviour
{
    [Header("Oxidation Timeline")]
    [Tooltip("Time in seconds to completely turn green under normal exposure.")]
    [SerializeField] private float weatheringDuration = 180.0f;
    [Range(0f, 1f)] public float currentOxidation = 0f;

    [Header("Weather Conditions")]
    public bool isExposedToRain = true;
    [Tooltip("Accelerates rot multiplier near oceans or wet areas.")]
    [SerializeField] private float environmentalHumidityMultiplier = 1.0f;

    private Material copperMaterial;
    private bool isWeathering = false;

    private static readonly int OxidationProgressID = Shader.PropertyToID("_OxidationProgress");

    void Start()
    {
        Renderer renderer = GetComponent<Renderer>();
        if (renderer != null)
        {
            copperMaterial = renderer.material;
            copperMaterial.SetFloat(OxidationProgressID, currentOxidation);
        }

        StartWeatheringSequence();
    }

    public void StartWeatheringSequence()
    {
        if (!isWeathering)
        {
            isWeathering = true;
            StartCoroutine(ExecuteOxidationTimeline());
        }
    }

    private IEnumerator ExecuteOxidationTimeline()
    {
        float elapsed = currentOxidation * weatheringDuration;

        while (elapsed < weatheringDuration)
        {
            // Dynamically alter oxidation speed depending on weather conditions
            float dynamicSpeed = Time.deltaTime * environmentalHumidityMultiplier;
            if (isExposedToRain) dynamicSpeed *= 3.0f; // Acid rain/moisture speeds up decay heavily

            elapsed += dynamicSpeed;
            currentOxidation = Mathf.Clamp01(elapsed / weatheringDuration);

            if (copperMaterial != null)
            {
                copperMaterial.SetFloat(OxidationProgressID, currentOxidation);
            }

            yield return null;
        }
    }

    /// <summary>
    /// Call this if an enemy attacks with acid weapons, or chemical explosions occur nearby.
    /// </summary>
    public void InjectCorrosiveChemicals(float flashOxidationAmount)
    {
        currentOxidation = Mathf.Clamp01(currentOxidation + flashOxidationAmount);
        if (copperMaterial != null)
        {
            copperMaterial.SetFloat(OxidationProgressID, currentOxidation);
        }
    }

    private void OnDestroy()
    {
        if (copperMaterial != null) Destroy(copperMaterial);
    }
}
