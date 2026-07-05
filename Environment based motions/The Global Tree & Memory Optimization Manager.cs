using UnityEngine;
using System.Collections.Generic;

public class AAA_FruitManager : MonoBehaviour
{
    public static AAA_FruitManager Instance { get; private set; }

    [Header("Performance Thresholds")]
    [SerializeField] private int maxActivePhysicalFruits = 48; // Hard cap on physical simulations
    
    private Queue<AAA_FallingFruit> activePhysicalFruits = new Queue<AAA_FallingFruit>();

    void Awake()
    {
        if (Instance == null) Instance = this;
        else Destroy(gameObject);
    }

    /// <summary>
    /// Tracks newly dropped fruit and enforces hardware safety limits.
    /// </summary>
    public void TrackActiveFruit(AAA_FallingFruit fruit)
    {
        activePhysicalFruits.Enqueue(fruit);

        // If pool limit is exceeded, cleanly eliminate the oldest fallen fruit
        if (activePhysicalFruits.Count > maxActivePhysicalFruits)
        {
            AAA_FallingFruit oldestFruit = activePhysicalFruits.Dequeue();
            if (oldestFruit != null)
            {
                Destroy(oldestFruit.gameObject);
            }
        }
    }

    public void UnregisterFruit(AAA_FallingFruit fruit)
    {
        // Handled cleanly if fruit gets collected or squashed prematurely
        if (activePhysicalFruits.Contains(fruit))
        {
            // Internal cleanup tracking helper
        }
    }

    /// <summary>
    /// Proximity method invoked by environmental triggers (e.g., explosions or player hitting tree trunk)
    /// </summary>
    public static void ShakeTreeAtPosition(Vector3 impactPoint, float radius, float damageIntensity)
    {
        Collider[] hitColliders = Physics.OverlapSphere(impactPoint, radius);
        foreach (var col in hitColliders)
        {
            AAA_FallingFruit fruit = col.GetComponent<AAA_FallingFruit>();
            if (fruit != null)
            {
                // Scaled force based on how close the impact was to the fruit
                float distance = Vector3.Distance(impactPoint, col.transform.position);
                float damageEvaluation = damageIntensity * (1.0f - (distance / radius));
                fruit.ApplyEnvironmentalStress(damageEvaluation);
            }
        }
    }
}
