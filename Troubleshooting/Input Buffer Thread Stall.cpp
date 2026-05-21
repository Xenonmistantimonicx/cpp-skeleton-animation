#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <chrono>

// ============================================================================
// STATE MACHINE & CORE ENGINE PARAMETERS
// ============================================================================
enum MotionEngineState {
    MOTION_STATE_IDLE = 0,
    MOTION_STATE_STRESS_TEST = 1,
    MOTION_STATE_UNFREEZE_OVERRIDE = 2
};

MotionEngineState currentEngineState = MOTION_STATE_IDLE;
float automaticTimeModifier = 0.0f;
float dynamicSpeedMultiplier = 1.0f;
bool isAutoMotionActive = true;

// Joint Matrices (Skeletal Vectors)
float shoulderJointAngle = -30.0f;
float elbowJointAngle = 45.0f;

// Vision Matrix Coordinates
float cameraYaw = 0.0f;
float cameraPitch = 0.0f;
float mouseSensitivity = 0.15f;
int lastMouseX = 400;
int lastMouseY = 300;
bool isMouseLookLocked = false;

// --- CRITICAL INPUT FAILSAFE REGISTERS ---
double lastRecordedInputTimestamp = 0.0; // Tracks when ANY key/mouse event happened
unsigned long totalFailsafeInjections = 0;

// Stuck checks for watchdog
float lastCheckedShoulderAngle = -999.0f;
float lastCheckedElbowAngle = -999.0f;
unsigned int motionStuckStrikes = 0;

std::chrono::high_resolution_clock::time_point engineInitClock = std::chrono::high_resolution_clock::now();

double GetAbsoluteEnginePrecisionTime() {
    auto currentClockTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedSecs = currentClockTime - engineInitClock;
    return elapsedSecs.count();
}

// ============================================================================
// KEYBOARD STALL INTERVENTION LAYER (The Ultimate Watchdog)
// ============================================================================
void ForceMotionDiagnosticCheck() {
    double currentSystemTick = GetAbsoluteEnginePrecisionTime();
    static double lastWatchdogVerificationTime = 0.0;

    if (currentSystemTick - lastWatchdogVerificationTime >= 0.3) {
        
        // --- FEATURE 1: KEYBOARD FREEZE DETECTION ---
        // Agar user ne pichle 8 seconds se koi key nahi dabayi (ya keys block ho gayi hain)
        // aur engine freeze lag raha hai, toh automatic bypass recovery trigger hogi.
        if (isAutoMotionActive && (currentSystemTick - lastRecordedInputTimestamp > 8.0)) {
            std::cout << "\n[⚠️ KEYBOARD STALL WARNING] No Input Thread Signals Detected for 8 Seconds!" << std::endl;
            std::cout << "[RECOVERY] Simulating Synthetic Input Pulse to Verify Pipeline Integrity..." << std::endl;
            
            // Re-map states using system ticks directly to bypass input hardware
            currentEngineState = MOTION_STATE_UNFREEZE_OVERRIDE;
            dynamicSpeedMultiplier = 2.5f;
            lastRecordedInputTimestamp = currentSystemTick; // Reset clock timer
            totalFailsafeInjections++;
        }

        // 2. Bone Alignment Check
        float shoulderDelta = std::fabs(shoulderJointAngle - lastCheckedShoulderAngle);
        float elbowDelta = std::fabs(elbowJointAngle - lastCheckedElbowAngle);

        if (isAutoMotionActive && (shoulderDelta < 0.0001f && elbowDelta < 0.0001f)) {
            motionStuckStrikes++;
            if (motionStuckStrikes >= 3) {
                std::cout << "[WATCHDOG] Frame Pipeline Locked! Forcing Transformation Shift..." << std::endl;
                currentEngineState = MOTION_STATE_UNFREEZE_OVERRIDE;
                automaticTimeModifier += 1.5708f;
                shoulderJointAngle += 10.0f;
                motionStuckStrikes = 0;
            }
        } else {
            if (motionStuckStrikes > 0) motionStuckStrikes--;
        }

        lastCheckedShoulderAngle = shoulderJointAngle;
        lastCheckedElbowAngle = elbowJointAngle;
        lastWatchdogVerificationTime = currentSystemTick;
    }
}

// ============================================================================
// FEATURE 2: BACKUP MOUSE CLICK INPUT PATH (Bypasses Keyboard Completely)
// ============================================================================
void mouseClickEmergencyBypass(int button, int state, int x, int y) {
    // Register action timestamp to prove the input hardware channel is alive
    lastRecordedInputTimestamp = GetAbsoluteEnginePrecisionTime();

    if (state == GLUT_DOWN) {
        std::cout << "\n[MOUSE INPUT INTERCEPT] Click received via hardware fallback channel." << std::endl;
        
        if (button == GLUT_LEFT_BUTTON) {
            // Left click fixes a frozen character or swaps state instantly
            if (currentEngineState == MOTION_STATE_STRESS_TEST) {
                currentEngineState = MOTION_STATE_IDLE;
            } else {
                currentEngineState = MOTION_STATE_STRESS_TEST;
            }
            std::cout << "[FALLBACK REGISTERED] Swapped Motion Profile via Left-Click Interface." << std::endl;
        } 
        else if (button == GLUT_RIGHT_BUTTON) {
            // Right click instantly unlocks/resets camera freeze parameters
            cameraYaw = 0.0f;
            cameraPitch = 0.0f;
            isMouseLookLocked = false;
            std::cout << "[FALLBACK REGISTERED] Camera Look Vectors Reset via Right-Click Interface." << std::endl;
        }
    }
    glutPostRedisplay();
}

// ============================================================================
// PASSIVE MOUSE LOOK ENGINE
// ============================================================================
void passiveMouseVisionEngine(int x, int y) {
    if (isMouseLookLocked) return;

    float deltaX = (float)(x - lastMouseX);
    float deltaY = (float)(y - lastMouseY);
    lastMouseX = x;
    lastMouseY = y;

    // Refresh timestamp on mouse hover event
    if (std::fabs(deltaX) > 0.5f || std::fabs(deltaY) > 0.5f) {
        lastRecordedInputTimestamp = GetAbsoluteEnginePrecisionTime();
    }

    cameraYaw += deltaX * mouseSensitivity;
    cameraPitch += deltaY * mouseSensitivity;

    if (cameraPitch > 80.0f)  cameraPitch = 80.0f;
    if (cameraPitch < -80.0f) cameraPitch = -80.0f;

    if (x < 50 || x > 750 || y < 50 || y > 550) {
        lastMouseX = 400;
        lastMouseY = 300;
        glutWarpPointer(400, 300);
    }
    glutPostRedisplay();
}

// ============================================================================
// GRAPHICS RENDERING RETRIEVAL MATRIX
// ============================================================================
void DrawRenderBone(float length, float thickness, float r, float g, float b) {
    GLUquadric* quad = gluNewQuadric();
    if (!quad) return;

    glColor3f(r + 0.12f, g + 0.12f, b + 0.12f);
    glutSolidSphere(thickness * 1.35f, 16, 16);

    glColor3f(r, g, b);
    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(quad, thickness, thickness * 0.82f, length, 16, 16);
    glPopMatrix();

    gluDeleteQuadric(quad);
}

void displayMainViewport() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Transform Vision Matrix
    glTranslatef(0.0f, -0.5f, -12.0f);
    glRotatef(cameraPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraYaw,   0.0f, 1.0f, 0.0f);

    // Call continuous health scans
    ForceMotionDiagnosticCheck();

    // RENDER BIOMECHANICAL RIG
    glPushMatrix(); 
    glTranslatef(0.0f, 2.2f, 0.0f); 

    glRotatef(shoulderJointAngle, 1.0f, 0.0f, 0.0f);
    DrawRenderBone(3.8f, 0.38f, 0.85f, 0.35f, 0.35f); // Crimson Upper Arm

    glTranslatef(0.0f, -3.8f, 0.0f);
    glRotatef(elbowJointAngle, 1.0f, 0.0f, 0.0f);
    DrawRenderBone(3.2f, 0.28f, 0.35f, 0.65f, 0.85f); // Cyan Forearm

    glPopMatrix(); 

    glutSwapBuffers();
}

// ============================================================================
// INTERNAL PROCEDURAL TICKER LOOP
// ============================================================================
void motionStateTimer(int value) {
    if (isAutoMotionActive) {
        automaticTimeModifier += 0.025f * dynamicSpeedMultiplier;

        switch (currentEngineState) {
            case MOTION_STATE_IDLE:
                shoulderJointAngle = -30.0f + std::sin(automaticTimeModifier * 1.1f) * 20.0f;
                elbowJointAngle = 45.0f + std::cos(automaticTimeModifier * 1.3f) * 25.0f;
                break;

            case MOTION_STATE_STRESS_TEST:
                shoulderJointAngle = -45.0f + std::sin(automaticTimeModifier * 2.8f) * 50.0f;
                elbowJointAngle = 60.0f + std::cos(automaticTimeModifier * 3.2f) * 45.0f;
                break;

            case MOTION_STATE_UNFREEZE_OVERRIDE:
                shoulderJointAngle = -30.0f + std::sin(automaticTimeModifier * 15.0f) * 35.0f;
                elbowJointAngle = 45.0f + std::cos(automaticTimeModifier * 15.0f) * 35.0f;
                
                if (std::fmod(automaticTimeModifier, 3.14159f) < 0.08f) {
                    currentEngineState = MOTION_STATE_IDLE;
                    dynamicSpeedMultiplier = 1.0f;
                    std::cout << "[WATCHDOG] Active motion state stabilized." << std::endl;
                }
                break;
        }
    }

    glutPostRedisplay(); 
    glutTimerFunc(16, motionStateTimer, 0);
}

// ============================================================================
// STANDARD KEYBOARD ROUTINE (Primary Input Line)
// ============================================================================
void keyboardInputOverride(unsigned char key, int x, int y) {
    // Log the keystroke timing to inform watchdog that the driver thread is active
    lastRecordedInputTimestamp = GetAbsoluteEnginePrecisionTime();

    switch (key) {
        case ' ':
            isAutoMotionActive = !isAutoMotionActive;
            break;
        case '1':
            currentEngineState = MOTION_STATE_IDLE;
            break;
        case '2':
            currentEngineState = MOTION_STATE_STRESS_TEST;
            break;
        case 27:
            exit(0);
    }
    glutPostRedisplay();
}

// ============================================================================
// INITIALIZATION LAYERS
// ============================================================================
void setupGraphicsContext() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    GLfloat lightDirection[] = { 2.0f, 6.0f, 3.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightDirection);

    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
    
    glutSetCursor(GLUT_CURSOR_NONE); 
    glutWarpPointer(400, 300);

    // Initialize clock markers on execution start
    lastRecordedInputTimestamp = GetAbsoluteEnginePrecisionTime();
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
    glutInitWindowSize(800, 600);
    glutCreateWindow("Anti-Freeze Multi-Channel Input Master Rig Engine");

    setupGraphicsContext();

    // 1. PRIMARY CONTROLS (Keyboard Driver)
    glutKeyboardFunc(keyboardInputOverride);

    // 2. VISION & MOTION CONTROLS (Mouse Movement Driver)
    glutPassiveMotionFunc(passiveMouseVisionEngine);

    // 3. SECONDARY RECOVERY CONTROLS (Alternative Mouse Click Driver)
    glutMouseFunc(mouseClickEmergencyBypass);

    glutDisplayFunc(displayMainViewport);
    glutReshapeFunc(standardReshape);
    
    glutTimerFunc(16, motionStateTimer, 0);

    std::cout << "==========================================================================" << std::endl;
    std::cout << "    DUAL-CHANNEL HARDWARE INTERVENTION ENGINE ONLINE                      " << std::endl;
    std::cout << "==========================================================================" << std::endl;
    std::cout << "  [Anti-Keyboard Lockout Features]:                                       " << std::endl;
    std::cout << "   - IF KEYBOARD FREEZES -> Click MOUSE LEFT BUTTON to toggle animations.  " << std::endl;
    std::cout << "   - IF KEYBOARD FREEZES -> Click MOUSE RIGHT BUTTON to reset camera view. " << std::endl;
    std::cout << "   - HEARTBEAT PULSE     -> Auto-recovers matrix if 0 input for 8 seconds. " << std::endl;
    std::cout << "==========================================================================" << std::endl;

    glutMainLoop();
    return 0;
}
