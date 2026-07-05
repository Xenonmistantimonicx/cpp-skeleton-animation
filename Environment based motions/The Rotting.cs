using UnityEngine;
using System.Collections;

public class AAA_FruitRotController : MonoBehaviour
{
    [Header("Rotting Timeline")]
    [Tooltip("Time in seconds before the fruit starts rotting after touching the ground.")]
    [SerializeField] private float delayBeforeRot = 10.0f;
    [Tooltip("Duration of the rotting visual transition from fresh to spoiled.")]
    [SerializeField] private float rotDuration = 30.0f;

    [Header("Visual Settings")]
    [ColorUsage(false, false)]
    [SerializeField] private Color rotColor = new Color(0.22f, 0.15f, 0.09f); // Dark organic brown

    // Material Interaction Cache
    private Material fruitMaterial;
    private float currentRotProgress = 0.0f;
    private bool isGrounded = false;

    // Shader Property IDs (Cached for high performance optimization)
    private static readonly int RotProgressID = Shader.PropertyToID("_RotProgress");
    private static readonly int RotColorID = Shader.PropertyToID("_RotColor");

    void Start()
    {
        // Duplicate the material instance so this specific fruit rots independently of others
        Renderer fruitRenderer = GetComponent<Renderer>();
        if (fruitRenderer != null)
        {
            fruitMaterial = fruitRenderer.material;
            // Seed the initial target color directly to the GPU
            fruitMaterial.SetColor(RotColorID, rotColor);
            fruitMaterial.SetFloat(RotProgressID, 0.0f);
        }
    }

    // Call this method via your collision script or character pickup events when fruit hits dirt
    public void StartRottingSequence()
    {
        if (!isGrounded)
        {
            isGrounded = true;
            StartCoroutine(ExecuteRotTimeline());
        }
    }

    private IEnumerator ExecuteRotTimeline()
    {
        // Wait on the ground pristine for a configured duration
        yield return new WaitForSeconds(delayBeforeRot);

        float elapsed = 0.0f;

        while (elapsed < rotDuration)
        {
            elapsed += Time.deltaTime;
            
            // Calculate progress mathematically along a non-linear ease curve
            // Rotting accelerates as time progresses
            float linearProgress = elapsed / rotDuration;
            currentRotProgress = Mathf.Pow(linearProgress, 1.5f); 

            // Pass the float conversion directly down to the HLSL pipeline
            if (fruitMaterial != null)
            {
                fruitMaterial.SetFloat(RotProgressID, currentRotProgress);
            }

            yield return null;
        }

        // Lock at max rot values
        currentRotProgress = 1.0f;
        if (fruitMaterial != null) fruitMaterial.SetFloat(RotProgressID, 1.0f);
        
        OnRotComplete();
    }

    private void OnRotComplete()
    {
        // Handle post-rot logic here (e.g., spawn flies, lower food value metrics, or dissolve object)
        Debug.Log($"{gameObject.name} is completely rotten.");
    }

    private void OnDestroy()
    {
        // AAA Memory Management: Prevent permanent runtime material memory leaks
        if (fruitMaterial != null)
        {
            Destroy(fruitMaterial);
        }
    }

    // Simple physics hook implementation example
    private void OnCollisionEnter(Collision collision)
    {
        if (collision.gameObject.CompareTag("Ground"))
        {
            StartRottingSequence();
        }
    }
}
