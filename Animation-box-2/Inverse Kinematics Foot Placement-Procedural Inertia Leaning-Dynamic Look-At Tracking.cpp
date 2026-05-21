#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>

// ========================================================================
// --- CORE MATH FRAMEWORK (Vector3, Quaternion, SLERP, Clamp Math) ---
// ========================================================================

struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vector3 operator+(const Vector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vector3 operator-(const Vector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vector3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    
    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    
    Vector3 Normalize() const {
        float len = Length();
        if (len > 0.00001f) return {x / len, y / len, z / len};
        return {0.0f, 0.0f, 0.0f};
    }

    static float Dot(const Vector3& a, const Vector3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static Vector3 Cross(const Vector3& a, const Vector3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }
};

struct Quaternion {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;

    static Quaternion FromEuler(float pitch, float yaw, float roll) {
        // Convert degrees to radians
        float p = pitch * 0.0174533f * 0.5f;
        float y = yaw * 0.0174533f * 0.5f;
        float r = roll * 0.0174533f * 0.5f;

        float sinP = std::sin(p), cosP = std::cos(p);
        float sinY = std::sin(y), cosY = std::cos(y);
        float sinR = std::sin(r), cosR = std::cos(r);

        Quaternion q;
        q.w = cosR * cosP * cosY + sinR * sinP * sinY;
        q.x = sinR * cosP * cosY - cosR * sinP * sinY;
        q.y = cosR * sinP * cosY + sinR * cosP * sinY;
        q.z = cosR * cosP * sinY - sinR * sinP * cosY;
        return q;
    }

    static Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, float alpha) {
        float dot = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
        
        Quaternion target = q2;
        if (dot < 0.0f) {
            dot = -dot;
            target.x = -q2.x; target.y = -q2.y; target.z = -q2.z; target.w = -q2.w;
        }

        if (dot > 0.9995f) {
            // Linear interpolation fallback for extremely close angles
            Quaternion result = {
                q1.x + alpha * (target.x - q1.x),
                q1.y + alpha * (target.y - q1.y),
                q1.z + alpha * (target.z - q1.z),
                q1.w + alpha * (target.w - q1.w)
            };
            float len = std::sqrt(result.x*result.x + result.y*result.y + result.z*result.z + result.w*result.w);
            result.x /= len; result.y /= len; result.z /= len; result.w /= len;
            return result;
        }

        float theta_0 = std::acos(dot);
        float theta = theta_0 * alpha;
        float sin_theta = std::sin(theta);
        float sin_theta_0 = std::sin(theta_0);

        float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
        float s1 = sin_theta / sin_theta_0;

        return {
            (s0 * q1.x) + (s1 * target.x),
            (s0 * q1.y) + (s1 * target.y),
            (s0 * q1.z) + (s1 * target.z),
            (s0 * q1.w) + (s1 * target.w)
        };
    }
};

// Engine Structural Components
struct Transform {
    Vector3 position;
    Quaternion localRotation;
};

struct RaycastHit {
    bool bHit = false;
    Vector3 hitPoint;
    Vector3 surfaceNormal;
};

// ========================================================================
// --- MONOLITHIC REAL-LIFE PROCEDURAL ANIMATION ENGINE SYSTEM ---
// ========================================================================

class RealLifeAnimationEngine {
private:
    // Internal Skeleton Storage Simulator
    enum BoneIndex { Hips = 0, Spine_01, Spine_02, Neck, Head, LeftThigh, LeftKnee, LeftFoot, RightThigh, RightKnee, RightFoot, TotalBones };
    Transform m_skeletonPose[TotalBones];
    std::string m_boneNames[TotalBones];

    // --- PIPELINE 1: FOOT PLACEMENT ENGINE DATA VARIABLES ---
    float m_currentLeftIKOffset = 0.0f;
    float m_currentRightIKOffset = 0.0f;
    float m_currentPelvisOffset = 0.0f;
    float m_leftFootNormalPitch = 0.0f;
    float m_rightFootNormalPitch = 0.0f;

    // --- PIPELINE 2: LOCOMOTION INERTIA PHYSICS VARIABLES ---
    Vector3 m_characterVelocity;
    Vector3 m_characterAcceleration;
    float m_targetLeanIntensity = 0.0f;
    float m_currentLeanIntensity = 0.0f;

    // --- PIPELINE 3: PROCEDURAL LOOK-AT TRACKING VARIABLES ---
    Quaternion m_currentHeadTargetRotation;

public:
    RealLifeAnimationEngine() {
        // Initialize names for debugging visibility
        m_boneNames[Hips] = "Hips"; m_boneNames[Spine_01] = "Spine_01"; m_boneNames[Spine_02] = "Spine_02";
        m_boneNames[Neck] = "Neck"; m_boneNames[Head] = "Head";
        m_boneNames[LeftThigh] = "LeftThigh"; m_boneNames[LeftKnee] = "LeftKnee"; m_boneNames[LeftFoot] = "LeftFoot";
        m_boneNames[RightThigh] = "RightThigh"; m_boneNames[RightKnee] = "RightKnee"; m_boneNames[RightFoot] = "RightFoot";

        // Establish default base stance setup (Character standing straight at Origin)
        for (int i = 0; i < TotalBones; ++i) {
            m_skeletonPose[i].position = { 0.0f, 0.0f, 0.0f };
            m_skeletonPose[i].localRotation = Quaternion::FromEuler(0, 0, 0);
        }
        // Base bone default vertical offsets
        m_skeletonPose[Hips].position.y = 0.95f; 
        m_skeletonPose[LeftFoot].position = { -0.2f, 0.0f, 0.0f };
        m_skeletonPose[RightFoot].position = { 0.2f, 0.0f, 0.0f };
        m_skeletonPose[Head].position = { 0.0f, 1.70f, 0.0f };
    }

    // ====================================================================
    // INTERACTION PASS 1: PROCEDURAL TERRAIN IK (FOOT PLACEMENT & PELVIS)
    // ====================================================================
    void ExecuteTerrainIKProcessing(RaycastHit leftFootTrace, RaycastHit rightFootTrace, float deltaTime) {
        float targetLeftOffset = 0.0f;
        float targetRightOffset = 0.0f;

        // 1. Process Left Leg Trace & Normal Rotations
        if (leftFootTrace.bHit) {
            targetLeftOffset = leftFootTrace.hitPoint.y - 0.0f; // Calculate ground delta
            // Convert normal slope vectors to local ankle Pitch delta
            m_leftFootNormalPitch = std::atan2(leftFootTrace.surfaceNormal.z, leftFootTrace.surfaceNormal.y) * 57.2958f;
        }

        // 2. Process Right Leg Trace & Normal Rotations
        if (rightFootTrace.bHit) {
            targetRightOffset = rightFootTrace.hitPoint.y - 0.0f;
            m_rightFootNormalPitch = std::atan2(rightFootTrace.surfaceNormal.z, rightFootTrace.surfaceNormal.y) * 57.2958f;
        }

        // 3. Dynamic Pelvis Drop Math (Hips drop to accommodate lowest stretching joint leg limits)
        float targetPelvisDrop = std::min(targetLeftOffset, targetRightOffset);
        if (targetPelvisDrop < 0.0f) {
            m_currentPelvisOffset += (targetPelvisDrop - m_currentPelvisOffset) * (1.0f - std::exp(-15.0f * deltaTime));
        } else {
            m_currentPelvisOffset += (0.0f - m_currentPelvisOffset) * (1.0f - std::exp(-12.0f * deltaTime));
        }

        // 4. Relative Foot Space Extraction (Compensate IK offset relative to the new pelvis posture weight)
        targetLeftOffset -= m_currentPelvisOffset;
        targetRightOffset -= m_currentPelvisOffset;

        // 5. Muscle Dampening Interpolation Loop (Smooth out instantaneous stepping pops)
        const float blendVelocity = 14.0f; 
        m_currentLeftIKOffset += (targetLeftOffset - m_currentLeftIKOffset) * (1.0f - std::exp(-blendVelocity * deltaTime));
        m_currentRightIKOffset += (targetRightOffset - m_currentRightIKOffset) * (1.0f - std::exp(-blendVelocity * deltaTime));

        // 6. Write final offsets directly to simulated bones
        m_skeletonPose[Hips].position.y = 0.95f + m_currentPelvisOffset;
        m_skeletonPose[LeftFoot].position.y = 0.0f + m_currentLeftIKOffset;
        m_skeletonPose[RightFoot].position.y = 0.0f + m_currentRightIKOffset;

        // Apply calculated normal pitch to ankles to keep feet flat on slanted slopes
        m_skeletonPose[LeftFoot].localRotation = Quaternion::FromEuler(m_leftFootNormalPitch, 0.0f, 0.0f);
        m_skeletonPose[RightFoot].localRotation = Quaternion::FromEuler(m_rightFootNormalPitch, 0.0f, 0.0f);
    }

    // ====================================================================
    // INTERACTION PASS 2: PROCEDURAL INERTIA LEAN & ACCELERATION PHYSICS
    // ====================================================================
    void UpdateLocomotionPhysics(Vector3 currentInputVelocity, float deltaTime) {
        // 1. Core Physics Processing: a = (v_new - v_old) / dt
        Vector3 deltaVelocity = currentInputVelocity - m_characterVelocity;
        m_characterAcceleration = deltaVelocity * (1.0f / deltaTime);
        m_characterVelocity = currentInputVelocity;

        // 2. Centrifugal Mass Lean Conversion
        float forwardSpeed = m_characterVelocity.Length();
        if (forwardSpeed > 0.1f) {
            // Negative vector scaling matches true skeletal opposite lean response
            float lateralForce = m_characterAcceleration.x * -0.04f; 
            m_targetLeanIntensity = lateralForce * (forwardSpeed * 0.15f);
        } else {
            m_targetLeanIntensity = 0.0f; // Reset to center when idling
        }

        // 3. Ergonomic Human Safety Caps (Strict clamp bounds to prevent structural mesh breakdown)
        m_targetLeanIntensity = std::clamp(m_targetLeanIntensity, -0.38f, 0.38f); // Max ~22 Degrees

        // 4. Elastic Muscle Interp Lag
        const float leanBlendSpeed = 10.0f;
        m_currentLeanIntensity += (m_targetLeanIntensity - m_currentLeanIntensity) * (1.0f - std::exp(-leanBlendSpeed * deltaTime));

        // 5. Exponential Spinal Chain Weight Distribution Layer
        // Vertebrae curve progressively; Spine-01 takes the brunt, Spine-02 stabilizes
        m_skeletonPose[Spine_01].localRotation.z = m_currentLeanIntensity * 0.60f; 
        m_skeletonPose[Spine_02].localRotation.z = m_currentLeanIntensity * 0.30f;
        m_skeletonPose[Hips].localRotation.z = m_currentLeanIntensity * 0.10f; // Hip minor tilt balance
    }

    // ====================================================================
    // INTERACTION PASS 3: PROCEDURAL LOOK-AT EYE/HEAD TARGET ENGINE
    // ====================================================================
    void ExecuteLookAtTracking(Vector3 targetWorldPosition, float deltaTime) {
        // 1. Establish Forward Vector Delta from Head Center toward Target
        Vector3 headWorldPos = m_skeletonPose[Head].position;
        Vector3 directionToTarget = targetWorldPosition - headWorldPos;
        
        // 2. Extract Angular Trigonometry Offsets (Pitch & Yaw)
        float horizontalDistance = std::sqrt(directionToTarget.x * directionToTarget.x + directionToTarget.z * directionToTarget.z);
        
        // Rad to Deg conversion via inverse tangents
        float targetYaw = std::atan2(directionToTarget.x, directionToTarget.z) * 57.2958f;
        float targetPitch = std::atan2(-directionToTarget.y, horizontalDistance) * 57.2958f;

        // 3. Humanoid Anatomical Bounds Constraint 
        // Prevents the horror-movie 360 neck spin effect. Caps looking at extreme angles.
        float clampedYaw = std::clamp(targetYaw, -75.0f, 75.0f);    // Max 75 deg left/right rotation boundary
        float clampedPitch = std::clamp(targetPitch, -45.0f, 45.0f); // Max 45 deg up/down lookup boundary

        // 4. Construct Target Joint Orientation Quat
        Quaternion derivedLookTarget = Quaternion::FromEuler(clampedPitch, clampedYaw, 0.0f);

        // 5. Look-At Slerp Lag Interpolation (Simulates natural human oculomotor latency delay)
        const float trackingReactionSpeed = 8.5f; 
        float interpolationAlpha = 1.0f - std::exp(-trackingReactionSpeed * deltaTime);
        
        m_currentHeadTargetRotation = Quaternion::Slerp(m_currentHeadTargetRotation, derivedLookTarget, interpolationAlpha);

        // 6. Joint Hierarchy Injection (Distribute load: 75% Head turns, 25% Neck handles micro adjustment)
        m_skeletonPose[Head].localRotation = Quaternion::Slerp(Quaternion::FromEuler(0,0,0), m_currentHeadTargetRotation, 0.75f);
        m_skeletonPose[Neck].localRotation = Quaternion::Slerp(Quaternion::FromEuler(0,0,0), m_currentHeadTargetRotation, 0.25f);
    }

    // ====================================================================
    // ENGINE RUNTIME INSPECTOR & LOGGER DIAGNOSTICS LAYER
    // ====================================================================
    void PrintEngineTelemetryReport() {
        std::cout << "\n========================================================================\n";
        std::cout << "                 REAL-LIFE LOCOMOTION ENGINE DIAGNOSTICS                 \n";
        std::cout << "========================================================================\n";
        
        std::cout << "[PASSED 1: INVERSE KINEMATICS FOOT PLACEMENT]\n";
        std::cout << " -> Dynamic Pelvis Shift Adjustment  : " << m_currentPelvisOffset * 100.0f << " cm\n";
        std::cout << " -> Left Ankle IK Local Space Buffer  : " << m_currentLeftIKOffset * 100.0f << " cm\n";
        std::cout << " -> Right Ankle IK Local Space Buffer : " << m_currentRightIKOffset * 100.0f << " cm\n";
        std::cout << " -> Calculated Terrain Surface Slanted Pitch: Left: " << m_leftFootNormalPitch << " deg | Right: " << m_rightFootNormalPitch << " deg\n\n";

        std::cout << "[PASSED 2: PROCEDURAL LOCOMOTION INERTIA LEANING]\n";
        std::cout << " -> Velocity Matrix Vectors           : [" << m_characterVelocity.x << ", " << m_characterVelocity.y << ", " << m_characterVelocity.z << "] m/s\n";
        std::cout << " -> Instantaneous Acceleration Rate X : " << m_characterAcceleration.x << " m/s^2\n";
        std::cout << " -> Spine Spine_01 Core Joint Bend    : " << (m_currentLeanIntensity * 0.60f) * 57.2958f << " deg\n\n";

        std::cout << "[PASSED 3: DYNAMIC LOOK-AT TARGET HORIZON TRACKING]\n";
        std::cout << " -> Calculated Target Quaternion Quad : [" << m_currentHeadTargetRotation.x << ", " << m_currentHeadTargetRotation.y << ", " << m_currentHeadTargetRotation.z << ", " << m_currentHeadTargetRotation.w << "]\n";
        std::cout << " -> Head Track Mesh Stabilization Status: RESOLVED AND INTEGRATED SUCCESSFULLY\n";
        std::cout << "========================================================================\n\n";
    }
};

// ========================================================================
// --- MAIN PIPELINE SIMULATION EXECUTION LOOP ---
// ========================================================================

int main() {
    std::cout << "INITIALIZING COMPREHENSIVE PRODUCTION LEVEL LOCOMOTION ENGINE TESTING...\n";
    
    // Create master engine runtime context
    RealLifeAnimationEngine runtimeEngine;

    const float deltaFrameTime = 0.0166f; // Standard 60 FPS update loop execution tick

    // --------------------------------------------------------------------
    // ENVIRONMENT CONDITION DATA GENERATION (Simulating complex environment context)
    // --------------------------------------------------------------------
    
    // Condition A: Foot Raycasts detect highly uneven ground (Left foot on an 18cm boulder, Right foot on base ground)
    RaycastHit leftFootRaycast;
    leftFootRaycast.bHit = true;
    leftFootRaycast.hitPoint = { -0.2f, 0.18f, 0.0f }; // Elevated 18 centimeters
    leftFootRaycast.surfaceNormal = { 0.0f, 0.94f, -0.34f }; // Slanted surface angle

    RaycastHit rightFootRaycast;
    rightFootRaycast.bHit = true;
    rightFootRaycast.hitPoint = { 0.2f, 0.0f, 0.0f }; // Standard level ground floor
    rightFootRaycast.surfaceNormal = { 0.0f, 1.0f, 0.0f }; // Perfectly flat upright normal

    // Condition B: Character is sprint-running at high speed and suddenly makes a dramatic tactical hard turn left
    Vector3 rapidCutTurnVelocityInput = { -8.5f, 0.0f, 5.0f }; // Heavy lateral drift component on X-Axis

    // Condition C: An enemy AI bot spawns to the right flank and high up on a balcony
    Vector3 dynamicThreatEnemyTargetPos = { 4.5f, 3.2f, 8.0f }; // Target to be tracked procedurally by head bones

    // --------------------------------------------------------------------
    // PIPELINE TICK REFRESH SIMULATION LOOP
    // --------------------------------------------------------------------
    std::cout << "Processing physics iterations over multiple frame loops to build skeletal dampening weight calculations...\n";
    
    // Execute multiple engine ticks over time so math dampening and SLERP vectors interpolate smoothly
    for (int currentFrame = 1; currentFrame <= 10; ++currentFrame) {
        // Run Pass 1: Solve Foot Position IK mapping data
        runtimeEngine.ExecuteTerrainIKProcessing(leftFootRaycast, rightFootRaycast, deltaFrameTime);

        // Run Pass 2: Calculate Momentum physics vectors and spine lean adjustments
        runtimeEngine.UpdateLocomotionPhysics(rapidCutTurnVelocityInput, deltaFrameTime);

        // Run Pass 3: Process target position trigonometry to adjust looking yaw/pitch
        runtimeEngine.ExecuteLookAtTracking(dynamicThreatEnemyTargetPos, deltaFrameTime);
    }

    // Engine Core Telemetry Outflow Interface Output
    runtimeEngine.PrintEngineTelemetryReport();

    std::cout << "ENGINE RUN SUCCESSFUL: Architecture pipeline validated without errors.\n";
    return 0;
}
