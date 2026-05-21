#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <chrono>

// ============================================================================
// FORWARD MOTION CONTROL STATES & MULTI-STATE MACHINE
// ============================================================================
enum MotionEngineState {
    MOTION_STATE_IDLE = 0,
    MOTION_STATE_STRESS_TEST = 1,
    MOTION_STATE_UNFREEZE_OVERRIDE = 2
};

// --- CORE SYSTEM CONTROLLERS ---
MotionEngineState currentEngineState = MOTION_STATE_IDLE;
float automaticTimeModifier = 0.0f;
float dynamicSpeedMultiplier = 1.0f;    // Controls calculation velocity
bool isAutoMotionActive = true;         // Toggle automatically via 'Space'

// Joint Matrices (Strictly Managed Structural Anchors)
float shoulderJointAngle = -30.0f;
float elbowJointAngle = 45.0f;

// Advanced Tracking Matrices to Detect Deadlocks
float lastCheckedShoulderAngle = -999.0f;
float lastCheckedElbowAngle = -999.0f;
unsigned int motionStuckStrikes = 0;
unsigned long totalRecoveryInterventions = 0;
double lastWatchdogVerificationTime = 0.0;

// High-Precision Hardware Clock Base
std::chrono::high_resolution_clock::time_point engineInitClock = std::chrono::high_resolution_clock::now();

double GetAbsoluteEnginePrecisionTime() {
    auto currentClockTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedSecs = currentClockTime - engineInitClock;
    return elapsedSecs.count();
}

// ============================================================================
// TRIPLE-LAYER FORCE INTEGRATOR (Character Unfreeze Logic)
// ============================================================================
void ForceMotionDiagnosticCheck() {
    double currentSystemTick = GetAbsoluteEnginePrecisionTime();

    // Temporal Guard: Check consistency every 0.3 seconds to optimize performance
    if (currentSystemTick - lastWatchdogVerificationTime >= 0.3) {
        
        // Layer 1: Validate coordinate differentiation down to 4 decimal places
        float shoulderDelta = std::fabs(shoulderJointAngle - lastCheckedShoulderAngle);
        float elbowDelta = std::fabs(elbowJointAngle - lastCheckedElbowAngle);

        if (isAutoMotionActive && (shoulderDelta < 0.0001f && elbowDelta < 0.0001f)) {
            motionStuckStrikes++;
            
            // Layer 2: Core Jam Identification after consecutive strike validations
            if (motionStuckStrikes >= 3) { 
                std::cout << "\n[!!! CRITICAL RECOVERY ACTIVATED !!!] Character Matrix Stalled!" << std::endl;
                std::cout << "[DIAGNOSTIC] Current Mode: " << currentEngineState << " | Forcing Pipeline Realignment..." << std::endl;
                
                // Layer 3: System Disruption. Change State & Inject High-Frequency Vectors
                currentEngineState = MOTION_STATE_UNFREEZE_OVERRIDE;
                automaticTimeModifier += 1.5708f; // Phase shift the core sine wave by 90 degrees instantly
                dynamicSpeedMultiplier = 3.0f;    // Overclock delta updates to shake the thread
                
                // Mathematical kick to immediately force rotation updates
                shoulderJointAngle += 7.5f;
                elbowJointAngle -= 5.0f;
                
                totalRecoveryInterventions++;
                motionStuckStrikes = 0;
                
                std::cout << "[RECOVERY STATUS] Noise injection complete. Total Interventions: " << totalRecoveryInterventions << std::endl;
            }
        } else {
            // System is running fluidly, decay strikes down gracefully
            if (motionStuckStrikes > 0) motionStuckStrikes--;
        }

        // Cache coordinates for historical trace profiling on next frame pass
        lastCheckedShoulderAngle = shoulderJointAngle;
        lastCheckedElbowAngle = elbowJointAngle;
        lastWatchdogVerificationTime = currentSystemTick;
    }
}

// ============================================================================
// SOLID GEOMETRY BLOCK (Skeletal Rig Architecture)
// ============================================================================
void DrawRenderBone(float length, float thickness, float r, float g, float b) {
    GLUquadric* quad = gluNewQuadric();
    if (!quad) return; // Fail-safe structural fallback

    // Joint Hub Node
    glColor3f(r + 0.12f, g + 0.12f, b + 0.12f);
    glutSolidSphere(thickness * 1.35f, 16, 16);

    // Main Structural Bone Shaft
    glColor3f(r, g, b);
    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // Map downward into system space
    gluCylinder(quad, thickness, thickness * 0.82f, length, 16, 16);
    glPopMatrix();

    gluDeleteQuadric(quad); // Structural memory protection
}

// ============================================================================
// CORE GRAPHICS DISPLAY MATRIX
// ============================================================================
void displayMainViewport() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Establish stable camera framing matrices
    glTranslatef(0.0f, 1.5f, -11.0f);

    // RUNTIME PROTECTION LAYER: Continuous loop scanning call
    ForceMotionDiagnosticCheck();

    // ---- CHARACTER RIG ASSEMBLY ----
    glPushMatrix(); 
    glTranslatef(0.0f, 2.2f, 0.0f); // Global pivot root translate

    // 1. Upper Segment (Shoulder Transform Branch)
    glRotatef(shoulderJointAngle, 1.0f, 0.0f, 0.0f);
    DrawRenderBone(3.8f, 0.38f, 0.85f, 0.35f, 0.35f); // Crimson Humerus

    // 2. Lower Segment (Elbow Transform Branch)
    glTranslatef(0.0f, -3.8f, 0.0f);
    glRotatef(elbowJointAngle, 1.0f, 0.0f, 0.0f);
    DrawRenderBone(3.2f, 0.28f, 0.35f, 0.65f, 0.85f); // Cyan Forearm

    glPopMatrix(); 
    // ---------------------------------

    glutSwapBuffers();
}

// ============================================================================
// ADVANCED STATE-DRIVEN TICKER ENGINE (Multi-Trajectory Motion Systems)
// ============================================================================
void motionStateTimer(int value) {
    if (isAutoMotionActive) {
        // High-precision clock delta mapping safely handled via mathematical step size
        automaticTimeModifier += 0.025f * dynamicSpeedMultiplier;

        // STATE PARSING: Branch variables across dynamic functional limits
        switch (currentEngineState) {
            
            case MOTION_STATE_IDLE:
                // Smooth lifelike breathing oscillation patterns
                shoulderJointAngle = -30.0f + std::sin(automaticTimeModifier * 1.1f) * 20.0f;
                elbowJointAngle = 45.0f + std::cos(automaticTimeModifier * 1.3f) * 25.0f;
                break;

            case MOTION_STATE_STRESS_TEST:
                // Fast-paced mechanical stress routines to push limits
                shoulderJointAngle = -45.0f + std::sin(automaticTimeModifier * 2.8f) * 50.0f;
                elbowJointAngle = 60.0f + std::cos(automaticTimeModifier * 3.2f) * 45.0f;
                break;

            case MOTION_STATE_UNFREEZE_OVERRIDE:
                // Rapid disruptor cycle to break external loop blockages
                shoulderJointAngle = -30.0f + std::sin(automaticTimeModifier * 15.0f) * 35.0f;
                elbowJointAngle = 45.0f + std::cos(automaticTimeModifier * 15.0f) * 35.0f;
                
                // Graceful loop stabilization mechanism
                if (std::fmod(automaticTimeModifier, 3.14159f) < 0.08f) {
                    currentEngineState = MOTION_STATE_IDLE;
                    dynamicSpeedMultiplier = 1.0f; // Return to standard tick speeds
                    std::cout << "[WATCHDOG] Unfreeze loop successful. Returning to regular Idle stream." << std::endl;
                }
                break;
        }
    }

    // MANDATORY PIPELINE SIGNAL COMMAND (Failsafe Call)
    glutPostRedisplay(); 

    // Re-schedule execution thread frame boundaries (60 FPS Targeting)
    glutTimerFunc(16, motionStateTimer, 0);
}

// ============================================================================
// RECOVERY INPUT INTERFACE (Keyboard Override Controls)
// ============================================================================
void keyboardInputOverride(unsigned char key, int x, int y) {
    switch (key) {
        
        case ' ': // Toggle continuous engine updating loops on/off (Simulate freeze)
            isAutoMotionActive = !isAutoMotionActive;
            std::cout << "[ENGINE KEYBOARD LOG] Automatic Heartbeat set to: " << (isAutoMotionActive ? "ENABLED" : "PAUSED (Simulating Freeze State)") << std::endl;
            break;

        case '1': // Switch engine trajectory matrix to Organic Ambient Mode
            currentEngineState = MOTION_STATE_IDLE;
            std::cout << "[STATE TRANSITION] Loaded Configuration: MOTION_STATE_IDLE" << std::endl;
            break;

        case '2': // Switch engine trajectory matrix to Performance Stress Mode
            currentEngineState = MOTION_STATE_STRESS_TEST;
            std::cout << "[STATE TRANSITION] Loaded Configuration: MOTION_STATE_STRESS_TEST" << std::endl;
            break;

        case 'f': // Inject artificial mathematical error conditions to force system watchdog check
        case 'F':
            std::cout << "[SYSTEM CONTROL] Forcing artificial deadlock strikes to test structural recovery..." << std::endl;
            lastCheckedShoulderAngle = shoulderJointAngle; // Mimic absolute identical frames
            lastCheckedElbowAngle = elbowJointAngle;
            motionStuckStrikes = 4;                       // Force-load critical bounds
            break;

        case '+': // Accelerate speed matrix values
            dynamicSpeedMultiplier += 0.25f;
            std::cout << "[SPEED DIAGNOSTIC] Current Core Step Multiplier: " << dynamicSpeedMultiplier << "x" << std::endl;
            break;

        case '-': // Decelerate speed matrix values
            dynamicSpeedMultiplier -= 0.25f;
            if (dynamicSpeedMultiplier < 0.0f) dynamicSpeedMultiplier = 0.0f;
            std::cout << "[SPEED DIAGNOSTIC] Current Core Step Multiplier: " << dynamicSpeedMultiplier << "x" << std::endl;
            break;

        // Manual calibration checks continue to function under manual mode
        default:
            if (!isAutoMotionActive) {
                if (key == 'o' || key == 'O') shoulderJointAngle += 4.0f;
                if (key == 'p' || key == 'P') shoulderJointAngle -= 4.0f;
                if (key == 'k' || key == 'K') elbowJointAngle += 4.0f;
                if (key == 'l' || key == 'L') elbowJointAngle -= 4.0f;
                std::cout << "[MANUAL OVERRIDE ADJUST] Shoulder Angle: " << shoulderJointAngle << " | Elbow Angle: " << elbowJointAngle << std::endl;
            }
            break;
    }

    if (key == 27) exit(0); // Esc key safe close
    glutPostRedisplay();
}

// ============================================================================
// SYSTEM UTILITIES
// ============================================================================
void setupGraphicsContext() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    GLfloat lightDirection[] = { 2.0f, 6.0f, 3.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightDirection);

    glClearColor(0.08f, 0.09f, 0.12f, 1.0f); // Clean Industrial Design Backdrop
}

void standardReshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(45.0f, (float)w / (float)h, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(900, 650);
    glutCreateWindow("Advanced Hyper-Responsive Anti-Freeze Movement Engine");

    setupGraphicsContext();

    // Default System Callbacks
    glutDisplayFunc(displayMainViewport);
    glutReshapeFunc(standardReshape);
    glutKeyboardFunc(keyboardInputOverride);

    // Initial Ticker Bootup
    glutTimerFunc(16, motionStateTimer, 0);

    std::cout << "==========================================================================" << std::endl;
    std::cout << "    HYPER-RESPONSIVE MOVEMENT UNFREEZE ENGINE ACTIVE                      " << std::endl;
    std::cout << "==========================================================================" << std::endl;
    std::cout << "  [Movement Diagnostic Input Interface]:                                  " << std::endl;
    std::cout << "   - Press 'SPACE' : Toggle Auto-Motion (Freeze simulation)               " << std::endl;
    std::cout << "   - Press '1'     : Organic Idle Motion Profile                          " << std::endl;
    std::cout << "   - Press '2'     : High-Frequency Stress Test Profile                   " << std::endl;
    std::cout << "   - Press 'F'     : Force-Inject Watchdog Unfreeze Sequence              " << std::endl;
    std::cout << "   - Press '+/-'   : Fine-Tune Delta Time Update Multipliers              " << std::endl;
    std::cout << "   - Manual Keys   : O/P for Shoulder | K/L for Elbow (When Paused)       " << std::endl;
    std::cout << "==========================================================================" << std::endl;

    glutMainLoop();
    return 0;
}
