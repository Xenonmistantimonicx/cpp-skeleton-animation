#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <chrono>

// ============================================================================
// MOVEMENT & VISION CONTROL STATES
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

// Joint Matrices (Skeletal Rig Vectors)
float shoulderJointAngle = -30.0f;
float elbowJointAngle = 45.0f;

// --- ADVANCED ADVANCED CAMERA & VISION CONSTANTS ---
float cameraYaw = 0.0f;    // Left / Right looking angle (Mouse X)
float cameraPitch = 0.0f;  // Up / Down looking angle (Mouse Y)
float mouseSensitivity = 0.15f;

// Mouse freeze protection registers
int lastMouseX = 400;
int lastMouseY = 300;
bool isMouseLookLocked = false; // Toggle view lock via 'M' key
unsigned int cameraStuckStrikes = 0;

// Stuck checks for rig tracking
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
// VISION & INTERVENTION DIAGNOSTIC CHECK (Camera Unfreeze Guard)
// ============================================================================
void ForceMotionDiagnosticCheck() {
    double currentSystemTick = GetAbsoluteEnginePrecisionTime();
    static double lastWatchdogVerificationTime = 0.0;

    if (currentSystemTick - lastWatchdogVerificationTime >= 0.3) {
        // 1. Joint Rig System Check
        float shoulderDelta = std::fabs(shoulderJointAngle - lastCheckedShoulderAngle);
        float elbowDelta = std::fabs(elbowJointAngle - lastCheckedElbowAngle);

        if (isAutoMotionActive && (shoulderDelta < 0.0001f && elbowDelta < 0.0001f)) {
            motionStuckStrikes++;
            if (motionStuckStrikes >= 3) {
                std::cout << "[WATCHDOG] Character Rig Stalled! Triggering Noise Burst..." << std::endl;
                currentEngineState = MOTION_STATE_UNFREEZE_OVERRIDE;
                automaticTimeModifier += 1.5708f;
                dynamicSpeedMultiplier = 3.0f;
                shoulderJointAngle += 7.5f;
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
// MOUSE VISION LOOK INTEGRATOR (Side me dekhne ka advance mechanics)
// ============================================================================
void passiveMouseVisionEngine(int x, int y) {
    if (isMouseLookLocked) return; // Agar 'M' dabakar lock kiya hai toh movement skip karo

    // Calculate mouse displacement deltas from the center/previous frame
    float deltaX = (float)(x - lastMouseX);
    float deltaY = (float)(y - lastMouseY);

    // Save history pointers
    lastMouseX = x;
    lastMouseY = y;

    // Apply sensitivity modifiers
    cameraYaw += deltaX * mouseSensitivity;
    cameraPitch += deltaY * mouseSensitivity;

    // Strict Pitch Constraints (Gardan ko 360 degree tootne se bachane ke liye clamp)
    if (cameraPitch > 80.0f)  cameraPitch = 80.0f;
    if (cameraPitch < -80.0f) cameraPitch = -80.0f;

    // Infinite loop correction for Yaw (Left/Right continuous rotation tracking)
    if (cameraYaw > 360.0f)  cameraYaw -= 360.0f;
    if (cameraYaw < -360.0f) cameraYaw += 360.0f;

    // Boundary Wrap Recovery: Window edges par cursor lock hone se bachane ka logic
    if (x < 50 || x > 750 || y < 50 || y > 550) {
        lastMouseX = 400;
        lastMouseY = 300;
        glutWarpPointer(400, 300); // Forcibly snap cursor back to center matrix
    }

    glutPostRedisplay();
}

// ============================================================================
// SKELETAL RIG GEOMETRY RENDERING
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

// ============================================================================
// CORE GRAPHICS DISPLAY & VISION VIEWPORT MATRIX
// ============================================================================
void displayMainViewport() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // ----------------========================================================
    // ADVANCED 3D VISION MATRIX ENGINE (Left/Right & Up/Down Look Orientation)
    // ----------------========================================================
    // Pehle base position apply karo (Camera translation offsets)
    glTranslatef(0.0f, -0.5f, -12.0f);

    // Dynamic rotations based on active mouse vectors
    glRotatef(cameraPitch, 1.0f, 0.0f, 0.0f); // X-Axis Control: Up/Down look angles
    glRotatef(cameraYaw,   0.0f, 1.0f, 0.0f); // Y-Axis Control: Left/Right side look angles

    // Pipeline Check Call
    ForceMotionDiagnosticCheck();

    // ---- CHARACTER RIG ASSEMBLY ----
    glPushMatrix(); 
    glTranslatef(0.0f, 2.2f, 0.0f); 

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
// MOTION ENGINE TICKER LOOP
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
                    std::cout << "[WATCHDOG] Character tracking recovered successfully." << std::endl;
                }
                break;
        }
    }

    glutPostRedisplay(); 
    glutTimerFunc(16, motionStateTimer, 0);
}

// ============================================================================
// SYSTEM KEYBOARD INTERFACE
// ============================================================================
void keyboardInputOverride(unsigned char key, int x, int y) {
    switch (key) {
        case ' ':
            isAutoMotionActive = !isAutoMotionActive;
            std::cout << "[STATUS] Auto Motion Heartbeat: " << (isAutoMotionActive ? "RUNNING" : "PAUSED") << std::endl;
            break;

        case '1':
            currentEngineState = MOTION_STATE_IDLE;
            break;

        case '2':
            currentEngineState = MOTION_STATE_STRESS_TEST;
            break;

        case 'm': // Vision Lock Toggle Check
        case 'M':
            isMouseLookLocked = !isMouseLookLocked;
            std::cout << "[VISION STATUS] Mouse Look Pipeline Override: " << (isMouseLookLocked ? "LOCKED (Freeze View)" : "UNLOCKED (Free look active)") << std::endl;
            break;

        case 'r': // Reset view system parameters instantly
        case 'R':
            cameraYaw = 0.0f;
            cameraPitch = 0.0f;
            std::cout << "[VISION CONTROL] View Matrices Zeroed Out to Default Default Front Position." << std::endl;
            break;

        case 27:
            exit(0);
    }
    glutPostRedisplay();
}

// ============================================================================
// SYSTEM UTILITIES INITIALIZER
// ============================================================================
void setupGraphicsContext() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    GLfloat lightDirection[] = { 2.0f, 6.0f, 3.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightDirection);

    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
    
    // Hide native windows cursor and center it inside window coordinates
    glutSetCursor(GLUT_CURSOR_NONE); 
    glutWarpPointer(400, 300);
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
    glutCreateWindow("Advanced Hyper-Responsive Vision & Motion Rig Engine");

    setupGraphicsContext();

    glutDisplayFunc(displayMainViewport);
    glutReshapeFunc(standardReshape);
    glutKeyboardFunc(keyboardInputOverride);
    
    // --- BIND MOUSE LOOK PASSIVE PIPELINE CALLBACK ---
    glutPassiveMotionFunc(passiveMouseVisionEngine);

    glutTimerFunc(16, motionStateTimer, 0);

    std::cout << "==========================================================================" << std::endl;
    std::cout << "    VISION LOOK INTERACTIVE MATRIX & UNFREEZE SUBSYSTEM                  " << std::endl;
    std::cout << "==========================================================================" << std::endl;
    std::cout << "  [Interactive Operations Manual]:                                        " << std::endl;
    std::cout << "   - MOVE MOUSE    : Look freely around the room (Left/Right/Up/Down)      " << std::endl;
    std::cout << "   - Press 'M'     : Toggle Mouse Vision Engine Lock (Freeze Looking)     " << std::endl;
    std::cout << "   - Press 'R'     : Hard Reset Camera Orientation Matrix back to Front   " << std::endl;
    std::cout << "   - Press 'SPACE' : Freeze/Unfreeze Character Joint Motions              " << std::endl;
    std::cout << "==========================================================================" << std::endl;

    glutMainLoop();
    return 0;
}
