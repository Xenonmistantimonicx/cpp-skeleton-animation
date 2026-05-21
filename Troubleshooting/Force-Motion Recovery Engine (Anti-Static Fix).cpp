#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <chrono>
#include <string>

// ============================================================================
// GLOBAL ENGINE CONFIGURATIONS & MOVEMENT STATES
// ============================================================================
enum EngineMovementState {
    STATE_IDLE_BREATHING = 0,
    STATE_EXERCISE_FLEXION = 1,
    STATE_WAVING_BROADCAST = 2,
    STATE_STUCK_OVERRIDE = 3
};

// Global State Trackers
EngineMovementState currentMotionState = STATE_IDLE_BREATHING;
float globalDeltaTimeModifier = 1.0f;     // Speed controller for movement
bool isInternalTickerActive = true;       // Engine heartbeat flag
float masterTimeAccumulator = 0.0f;       // Clean continuous math driver

// Complex Hierarchical Angular Matrices (Default Rest Postures)
float rightShoulderX = -15.0f, rightShoulderY = 0.0f, rightShoulderZ = 0.0f;
float rightElbowX = 45.0f;
float leftShoulderX = -15.0f, leftShoulderY = 0.0f, leftShoulderZ = 0.0f;
float leftElbowX = 30.0f;
float globalFingerFlexAngle = 10.0f;

// Camera Space Matrices
float viewCamX = 0.0f, viewCamY = 1.0f, viewCamZ = -16.0f;
float sceneRotationX = 15.0f, sceneRotationY = 0.0f;
int mouseTrackerLastX, mouseTrackerLastY;
bool interactionMouseLatch = false;

// ============================================================================
// ANTI-FREEZE WATCHDOG ENGINE VARIABLES (The Core Safety Layers)
// ============================================================================
float historicalCheckAngleRight = -9999.0f;
float historicalCheckAngleLeft = -9999.0f;
unsigned long continuousStaticFrameStrikes = 0;
unsigned long automaticRecoveryTriggersCount = 0;
double lastWatchdogCheckTimestamp = 0.0;

std::chrono::high_resolution_clock::time_point engineBootClock = std::chrono::high_resolution_clock::now();

// High-Precision Absolute Clock Reader
double GetSystemHighPrecisionTime() {
    auto currentClockTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> calculatedElapsed = currentClockTime - engineBootClock;
    return calculatedElapsed.count();
}

// ============================================================================
// TRIPLE-LAYER MOTION INSPECTOR & AUTOMATIC UNFREEZER
// ============================================================================
void ExecuteAdvancedUnfreezeInspector() {
    double currentSystemTick = GetSystemHighPrecisionTime();
    
    // Check internal loop interval safety (Every 0.5 seconds matrix validation)
    if (currentSystemTick - lastWatchdogCheckTimestamp >= 0.5) {
        
        // Layer 1 Check: absolute difference check of continuous float values
        float rightDeltaDiff = std::fabs(rightShoulderX - historicalCheckAngleRight);
        float leftDeltaDiff = std::fabs(leftElbowX - historicalCheckAngleLeft);

        if (isInternalTickerActive && (rightDeltaDiff < 0.0001f && leftDeltaDiff < 0.0001f)) {
            continuousStaticFrameStrikes++;
            
            // Layer 2 Identification: Frame threshold exceeded implies dead state
            if (continuousStaticFrameStrikes >= 2) { 
                std::cout << "\n[!!! WATCHDOG CRITICAL ALERT !!!] Character Movement Stalled detected!" << std::endl;
                std::cout << "[DIAGNOSTIC] Previous state matrix was locked. Initiating Emergency Core Overload..." << std::endl;
                
                // Layer 3 Resolution: Force State Mutation & Mathematical Core Shift
                currentMotionState = STATE_STUCK_OVERRIDE;
                masterTimeAccumulator += 1.5707f; // Push sine wave phase by 90 degrees manually
                globalDeltaTimeModifier = 2.5f;   // Boost internal computation cycles
                
                // Artificial noise injection to snap the hardware thread out of local minimums
                rightShoulderX += 5.5f;
                leftElbowX -= 4.0f;
                
                automaticRecoveryTriggersCount++;
                continuousStaticFrameStrikes = 0;
                
                std::cout << "[RECOVERY] Phase-shift force applied. Overload Count: " << automaticRecoveryTriggersCount << std::endl;
            }
        } else {
            // System is verified alive, cooling down strike registers
            if(continuousStaticFrameStrikes > 0) continuousStaticFrameStrikes--;
        }

        // Cache coordinates for upcoming historical cycle inspection
        historicalCheckAngleRight = rightShoulderX;
        historicalCheckAngleLeft = leftElbowX;
        lastWatchdogCheckTimestamp = currentSystemTick;
    }
}

// ============================================================================
// HIGH-FIDELITY STRUCTURAL MODEL GENERATOR (Bilateral Anatomy)
// ============================================================================
void RenderAnatomicalBone(float boneLength, float baseRadius, float targetRadius, float colorR, float colorG, float colorB) {
    GLUquadric* structuralMeshPointer = gluNewQuadric();
    if (!structuralMeshPointer) return;

    // Joint Capsule Sphere
    glColor3f(colorR * 1.15f, colorG * 1.15f, colorB * 1.15f);
    glutSolidSphere(baseRadius * 1.35f, 16, 16);

    // Main Diaphysis Cylinder Shaft
    glColor3f(colorR, colorG, colorB);
    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // Rotate downward to preserve architectural hierarchy orientation
    gluCylinder(structuralMeshPointer, baseRadius, targetRadius, boneLength, 16, 16);
    glPopMatrix();

    gluDeleteQuadric(structuralMeshPointer);
}

void BuildSkeletalArmChain(bool mirrorLeftConfiguration) {
    glPushMatrix();

    // Bilateral Inversion & Spatial Offsets Setup
    if (mirrorLeftConfiguration) {
        glTranslatef(-4.2f, 1.5f, 0.0f);     // Position Left Shoulder
        glScalef(-1.0f, 1.0f, 1.0f);         // Symmetric Space Transformation Matrix
        
        // Multi-Axis Angular Rotation Apply
        glRotatef(leftShoulderX, 1.0f, 0.0f, 0.0f);
        glRotatef(leftShoulderY, 0.0f, 1.0f, 0.0f);
        glRotatef(leftShoulderZ, 0.0f, 0.0f, 1.0f);
    } else {
        glTranslatef(4.2f, 1.5f, 0.0f);      // Position Right Shoulder
        
        glRotatef(rightShoulderX, 1.0f, 0.0f, 0.0f);
        glRotatef(rightShoulderY, 0.0f, 1.0f, 0.0f);
        glRotatef(rightShoulderZ, 0.0f, 0.0f, 1.0f);
    }

    // 1. UPPER LIMB BLOCK (Humerus Cylinder Generation)
    RenderAnatomicalBone(4.2f, 0.42f, 0.35f, 0.85f, 0.85f, 0.90f);

    // 2. MIDDLE TRANSITION NODE (Elbow Joint Hub Transformation)
    glTranslatef(0.0f, -4.2f, 0.0f);
    if (mirrorLeftConfiguration) {
        glRotatef(-leftElbowX, 1.0f, 0.0f, 0.0f);
    } else {
        glRotatef(-rightElbowX, 1.0f, 0.0f, 0.0f);
    }
    
    // Forearm Segment Assembly (Radius & Ulna Architecture)
    RenderAnatomicalBone(3.5f, 0.32f, 0.26f, 0.72f, 0.75f, 0.80f);

    // 3. DISTAL EXTREMITY SYSTEM (Wrist & Complete Hand Generation)
    glTranslatef(0.0f, -3.5f, 0.0f);
    
    // Render Stylized Carpal/Palm Matrix Box
    glColor3f(0.50f, 0.55f, 0.60f);
    glPushMatrix();
    glScalef(1.8f, 1.6f, 0.45f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // 4. FIVE-DIGIT PHALANX PARSER (Complex Internal Loop Generation)
    float baseThick[5] = { 0.24f, 0.18f, 0.20f, 0.18f, 0.15f };
    float digitLen[5]  = { 0.95f, 1.25f, 1.35f, 1.22f, 1.00f };
    
    for (int index = 0; index < 5; index++) {
        glPushMatrix();
        
        // Mathematical distribution scaling across hand width plane
        float lateralPlacementX = -0.75f + (index * 0.38f);
        float structuralElevationY = -0.8f;
        
        if (index == 0) { // Thumb Anatomical Position Offset Override
            lateralPlacementX = -1.0f; 
            structuralElevationY = -0.3f;
        }

        glTranslatef(lateralPlacementX, structuralElevationY, 0.0f);
        
        // Apply Finger Clench Rotations Engine Formula
        if (index == 0) {
            glRotatef(-globalFingerFlexAngle * 0.6f, 0.0f, 1.0f, 1.0f); // Thumb multi-axis orientation
        } else {
            glRotatef(-globalFingerFlexAngle, 1.0f, 0.0f, 0.0f);        // Basic Finger curling roll
        }

        // Render Proximal Phalanx Segment
        RenderAnatomicalBone(digitLen[index] * 0.6f, baseThick[index], baseThick[index] * 0.8f, 0.45f, 0.48f, 0.52f);
        
        // Shift to Distal Tip joint node
        glTranslatef(0.0f, -digitLen[index] * 0.6f, 0.0f);
        glOriginalUnfreeze: glRotatef(-globalFingerFlexAngle * 0.4f, 1.0f, 0.0f, 0.0f);
        
        // Render Distal Phalanx Segment
        RenderAnatomicalBone(digitLen[index] * 0.4f, baseThick[index] * 0.8f, baseThick[index] * 0.6f, 0.40f, 0.42f, 0.46f);
        
        // Solid Keratin Layer Addition (Nail Polish Matrix)
        glTranslatef(0.0f, -digitLen[index] * 0.38f, baseThick[index] * 0.5f);
        glColor3f(0.92f, 0.78f, 0.75f);
        glPushMatrix();
        glScalef(baseThick[index] * 1.1f, digitLen[index] * 0.22f, 0.06f);
        glutSolidCube(1.0f);
        glPopMatrix();

        glPopMatrix(); // End single finger instance matrix block
    }

    glPopMatrix(); // End overall compound limb allocation matrix block
}

// ============================================================================
// CORE VIEWPORT MAIN DRAW PIPELINE
// ============================================================================
void drawViewportRenderPass() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Camera View Space Coordinates Application
    glTranslatef(viewCamX, viewCamY, viewCamZ);
    glRotatef(sceneRotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(sceneRotationY, 0.0f, 1.0f, 0.0f);

    // CALL INTERVENTIONAL PROTECTION: Movement Check Engine
    ExecuteAdvancedUnfreezeInspector();

    // Rendering Master Characters Joint Chones
    BuildSkeletalArmChain(false); // Execute Right Arm Rig Tree
    BuildSkeletalArmChain(true);  // Execute Left Arm Rig Tree

    glutSwapBuffers();
}

// ============================================================================
// STATE-DRIVEN MULTI-MODE MOTION ENGINE (The Unfreeze Core Logic Loop)
// ============================================================================
void hardwareSynchronizedUpdateLoop(int loopTickValue) {
    if (isInternalTickerActive) {
        
        // Dynamic time flow step injection based on manual scalar controls
        masterTimeAccumulator += 0.025f * globalDeltaTimeModifier;

        // MULTI-BRANCH STATE MACHINE: Complex calculations to verify variables NEVER collapse to static states
        switch (currentMotionState) {
            
            case STATE_IDLE_BREATHING:
                // Gentle continuous wave patterns to mimic idle human breath frequency
                rightShoulderX = -15.0f + std::sin(masterTimeAccumulator * 1.2f) * 6.0f;
                rightShoulderZ = std::cos(masterTimeAccumulator * 0.8f) * 3.0f;
                rightElbowX    = 40.0f + std::sin(masterTimeAccumulator * 1.5f) * 8.0f;
                
                leftShoulderX  = -15.0f + std::sin(masterTimeAccumulator * 1.1f) * 5.0f;
                leftElbowX     = 35.0f + std::cos(masterTimeAccumulator * 1.4f) * 7.0f;
                
                globalFingerFlexAngle = 12.0f + std::sin(masterTimeAccumulator * 2.0f) * 10.0f;
                break;

            case STATE_EXERCISE_FLEXION:
                // Extreme heavy angular transformation testing loops (Workout Routine)
                rightShoulderX = -40.0f + std::sin(masterTimeAccumulator * 2.5f) * 45.0f;
                rightElbowX    = 70.0f + std::cos(masterTimeAccumulator * 2.5f) * 55.0f;
                
                leftShoulderX  = -40.0f + std::cos(masterTimeAccumulator * 2.5f) * 45.0f;
                leftElbowX     = 70.0f + std::sin(masterTimeAccumulator * 2.5f) * 55.0f;
                
                globalFingerFlexAngle = 45.0f + std::sin(masterTimeAccumulator * 3.0f) * 35.0f;
                break;

            case STATE_WAVING_BROADCAST:
                // Asymmetric high frequency waving task assigned to Right hand
                rightShoulderZ = -50.0f + std::sin(masterTimeAccumulator * 4.0f) * 30.0f;
                rightShoulderX = -30.0f + std::cos(masterTimeAccumulator * 1.5f) * 10.0f;
                rightElbowX    = 85.0f + std::sin(masterTimeAccumulator * 4.5f) * 20.0f;
                
                // Left arm maintains minimal stability balancing loop
                leftShoulderX  = -10.0f + std::sin(masterTimeAccumulator * 0.5f) * 4.0f;
                leftElbowX     = 20.0f;
                
                globalFingerFlexAngle = 15.0f;
                break;

            case STATE_STUCK_OVERRIDE:
                // Emergency recovery spiral calculation path to disrupt CPU structural lockups
                std::cout << "[EMERGENCY WORKING] High Frequency Jerk Function executing..." << std::endl;
                
                rightShoulderX = -20.0f + std::sin(masterTimeAccumulator * 12.0f) * 40.0f;
                leftElbowX     = 50.0f + std::cos(masterTimeAccumulator * 12.0f) * 40.0f;
                
                // Gradually restore balance and slide back to base state after automatic unfreeze kick
                if (std::fmod(masterTimeAccumulator, 3.1415f) < 0.1f) {
                    currentMotionState = STATE_IDLE_BREATHING;
                    globalDeltaTimeModifier = 1.0f; // Restore normal engine cycle speed
                    std::cout << "[WATCHDOG] Unfreeze operation complete. Returning to Idle Chain." << std::endl;
                }
                break;
        }
    }

    // MANDATORY FORCE REDRAW COMMAND (Failsafe Core call)
    glutPostRedisplay();

    // Re-trigger the hardware synchronized loop to maintain smooth rendering cadence (16ms = ~60FPS)
    glutTimerFunc(16, hardwareSynchronizedUpdateLoop, 0);
}

// ============================================================================
// RUNTIME USER INTERACTION OVERRIDES (Keyboard Diagnostics Matrix)
// ============================================================================
void keyboardCoreInputInterface(unsigned char userPressedKey, int mouseCoordsX, int mouseCoordsY) {
    switch (userPressedKey) {
        
        case ' ': // SPACEBAR: Force pause/play the entire back-end clock loop
            isInternalTickerActive = !isInternalTickerActive;
            std::cout << "[INPUT] Heartbeat Engine Ticker set to: " << (isInternalTickerActive ? "ACTIVE" : "PAUSED (Frozen on Purpose)") << std::endl;
            break;

        case '1': // Shift state machine down to basic organic loop
            currentMotionState = STATE_IDLE_BREATHING;
            std::cout << "[STATE CHANGED] Loaded: STATE_IDLE_BREATHING" << std::endl;
            break;

        case '2': // Shift state machine down to heavy muscle flexion loop
            currentMotionState = STATE_EXERCISE_FLEXION;
            std::cout << "[STATE CHANGED] Loaded: STATE_EXERCISE_FLEXION" << std::endl;
            break;

        case '3': // Shift state machine down to rapid broadcast signaling wave
            currentMotionState = STATE_WAVING_BROADCAST;
            std::cout << "[STATE CHANGED] Loaded: STATE_WAVING_BROADCAST" << std::endl;
            break;

        case 'f': // EMERGENCY OVERRIDE FORCE PUSH: Manual system kick simulation button
            std::cout << "[INPUT] Simulating manual user-induced unfreeze override injection..." << std::endl;
            historicalCheckAngleRight = rightShoulderX; // Fake static loop parameters to trick the watchdog
            historicalCheckAngleLeft = leftElbowX;
            continuousStaticFrameStrikes = 5;            // Force-load critical strikes count
            break;

        case '+': // Speed up delta calculation ticks
            globalDeltaTimeModifier += 0.25f;
            std::cout << "[SPEED CALIBRATION] Global Delta Time Multiplier: " << globalDeltaTimeModifier << "x" << std::endl;
            break;

        case '-': // Slow down delta calculation ticks
            globalDeltaTimeModifier -= 0.25f;
            if (globalDeltaTimeModifier < 0.0f) globalDeltaTimeModifier = 0.0f;
            std::cout << "[SPEED CALIBRATION] Global Delta Time Multiplier: " << globalDeltaTimeModifier << "x" << std::endl;
            break;

        case 27: // Safe Window Close Escape Protocol
            exit(0);
            break;
    }
}

// ============================================================================
// INTERACTIVE MOUSE INTERFACE (Spatial Orbit View Mechanics)
// ============================================================================
void mouseActionPressHandler(int coreButton, int mouseState, int posX, int posY) {
    if (coreButton == GLUT_LEFT_BUTTON) {
        if (mouseState == GLUT_DOWN) {
            interactionMouseLatch = true;
            mouseTrackerLastX = posX; mouseTrackerLastY = posY;
        } else if (mouseState == GLUT_UP) {
            interactionMouseLatch = false;
        }
    }
}

void mouseActiveTrackingHandler(int currentX, int currentY) {
    if (interactionMouseLatch) {
        sceneRotationY += (currentX - mouseTrackerLastX) * 0.4f;
        sceneRotationX += (currentY - mouseTrackerLastY) * 0.4f;
        mouseTrackerLastX = currentX; mouseTrackerLastY = currentY;
    }
}

// ============================================================================
// SYSTEM CONTEXT ENVIRONMENT LOADERS
// ============================================================================
void buildSystemLightingEnvironment() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    GLfloat structuralLightPos[] = { 4.0f, 12.0f, 6.0f, 1.0f };
    GLfloat lightAmbiencyColor[] = { 0.25f, 0.25f, 0.28f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, structuralLightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbiencyColor);

    glClearColor(0.06f, 0.08f, 0.12f, 1.0f); // Professional Dark Steel Blueprint background Canvas
}

void matrixViewportReshapeEvent(int windowW, int windowH) {
    if (windowH == 0) windowH = 1;
    glViewport(0, 0, windowW, windowH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)windowW / (float)windowH, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1100, 650);
    glutCreateWindow("Mega Modular C++ Humanoid Rig & Anti-Freeze Movement Engine");

    buildSystemLightingEnvironment();

    // Framework Runtime Event Mapping Callbacks
    glutDisplayFunc(drawViewportRenderPass);
    glutReshapeFunc(matrixViewportReshapeEvent);
    glutKeyboardFunc(keyboardCoreInputInterface);
    glutMouseFunc(mouseActionPressHandler);
    glutMotionFunc(mouseActiveTrackingHandler);

    // Bootstrap hardware rendering timer tick
    glutTimerFunc(16, hardwareSynchronizedUpdateLoop, 0);

    std::cout << "==========================================================================" << std::endl;
    std::cout << "  HYPER-RESPONSIVE MOVEMENT UNFREEZE ENGINE INITIALIZED SUCCESSFUL        " << std::endl;
    std::cout << "==========================================================================" << std::endl;
    std::cout << "  [Runtime Control Dashboard Matrix]:                                     " << std::endl;
    std::cout << "   - Press '1' : Trigger Realistic Breathing Motion Loop                   " << std::endl;
    std::cout << "   - Press '2' : Engage Extreme Bilateral Exercise Loop                    " << std::endl;
    std::cout << "   - Press '3' : Launch Rapid Multi-Joint Waving Routine                  " << std::endl;
    std::cout << "   - Press 'SPACE' : Manually Pause Heartbeat Loop (Simulate Freeze)      " << std::endl;
    std::cout << "   - Press 'F' : Force-Inject Watchdog Recovery Overload Sequence          " << std::endl;
    std::cout << "   - Press '+/-' : Dynamic Acceleration/Deceleration of Time Clock Cycles " << std::endl;
    std::cout << "   - Mouse Drag Left Click : Rotate $360^\\circ$ View Space Coordinate Systems" << std::endl;
    std::cout << "==========================================================================" << std::endl;

    glutMainLoop();
    return 0;
}
