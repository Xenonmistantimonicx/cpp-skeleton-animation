#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <chrono>

// ============================================================================
// SYSTEM STATES & MATRIX PARAMETERS
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

// Joint Rig Coordinates
float shoulderJointAngle = -30.0f;
float elbowJointAngle = 45.0f;

// Camera Space Coordinates
float cameraYaw = 0.0f;
float cameraPitch = 0.0f;
float mouseSensitivity = 0.15f;
int lastMouseX = 400;
int lastMouseY = 300;
bool isMouseLookLocked = false;

// Input Fallback Registers
double lastRecordedInputTimestamp = 0.0;

// --- ADVANCED ADVANCED DAY/NIGHT ENGINE VARIABLES ---
float environmentTimeClock = 0.0f;    // Master driver for sky and sun rotations
float skyColorR = 0.08f;             // Live structural ambient tracking registers
float skyColorG = 0.09f;
float skyColorB = 0.12f;
float sunPlacementPositionX = 2.0f;  // Live 3D position vector for Light0
float sunPlacementPositionY = 6.0f;

// Environ-Watchdog Memory Elements
float lastCheckedSkyColorR = -1.0f;
unsigned int environmentStuckStrikes = 0;
unsigned long totalEnvironRecoveries = 0;

// Rigid Watchdog Registers
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
// ENVIRON & RIG DIAGNOSTIC INSPECTOR (The Double-Watchdog System)
// ============================================================================
void ForceMotionDiagnosticCheck() {
    double currentSystemTick = GetAbsoluteEnginePrecisionTime();
    static double lastWatchdogVerificationTime = 0.0;

    if (currentSystemTick - lastWatchdogVerificationTime >= 0.3) {
        
        // --- FEATURE 1: DAY/NIGHT CYCLE LOCKUP PROTECTION ---
        // Agar dynamic time system actively run ho raha hai par sky matrix coordinates
        // 4 decimal places tak stagnant pade hain, toh cycle break system trigger hoga.
        float skyColorDelta = std::fabs(skyColorR - lastCheckedSkyColorR);
        
        if (isAutoMotionActive && skyColorDelta < 0.00001f) {
            environmentStuckStrikes++;
            if (environmentStuckStrikes >= 4) { // Approximately 1.2 seconds of sky freeze
                std::cout << "\n[☀️ SKY MATRIX FAULT] Day/Night Cycle Freeze Detected!" << std::endl;
                std::cout << "[DIAGNOSTIC] Forcing Sky-Phase Rotation Matrix Disruption..." << std::endl;
                
                // Forcibly jump the sky system timeline clock index by adding phase noise
                environmentTimeClock += 0.5235f; // Push by 30 degrees phase angle instantly
                environmentTimeClock += (float)std::sin(currentSystemTick) * 0.1f;
                
                totalEnvironRecoveries++;
                environmentStuckStrikes = 0;
            }
        } else {
            if (environmentStuckStrikes > 0) environmentStuckStrikes--;
        }
        
        // Save current color index into profile cache
        lastCheckedSkyColorR = skyColorR;

        // --- FEATURE 2: INPUT THREAD FAILSAFE ---
        if (isAutoMotionActive && (currentSystemTick - lastRecordedInputTimestamp > 8.0)) {
            currentEngineState = MOTION_STATE_UNFREEZE_OVERRIDE;
            dynamicSpeedMultiplier = 2.5f;
            lastRecordedInputTimestamp = currentSystemTick;
        }

        // --- FEATURE 3: CHARACTER SKELETAL SYSTEM GUARD ---
        float shoulderDelta = std::fabs(shoulderJointAngle - lastCheckedShoulderAngle);
        float elbowDelta = std::fabs(elbowJointAngle - lastCheckedElbowAngle);

        if (isAutoMotionActive && (shoulderDelta < 0.0001f && elbowDelta < 0.0001f)) {
            motionStuckStrikes++;
            if (motionStuckStrikes >= 3) {
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
// DYNAMIC SKY ENVIRONMENT MATHEMATICS (Real-time Day/Night Processing)
// ============================================================================
void AdvancedEnvironmentLightingPipeline() {
    // 1. Calculate interpolations using continuous trigonometric vectors
    float skySineFactor = std::sin(environmentTimeClock);
    float skyCosineFactor = std::cos(environmentTimeClock);

    // Normalize color channels dynamically based on solar altitude curves
    // Day Phase: Bright blue steel sky | Night Phase: Dark industrial cosmic slate
    skyColorR = 0.08f + (skySineFactor + 1.0f) * 0.15f; 
    skyColorG = 0.09f + (skySineFactor + 1.0f) * 0.25f;
    skyColorB = 0.12f + (skySineFactor + 1.0f) * 0.35f;

    // Apply color buffers down onto hardware memory matrix safely
    glClearColor(skyColorR, skyColorG, skyColorB, 1.0f);

    // 2. Compute solar orbits for physical standard directional illumination
    sunPlacementPositionX = 2.0f + skyCosineFactor * 10.0f;
    sunPlacementPositionY = 4.0f + skySineFactor * 10.0f; // Sun sets below horizon on negative values

    GLfloat structuralLight0Pos[] = { sunPlacementPositionX, sunPlacementPositionY, 3.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, structuralLight0Pos);

    // Change ambient reflection states depending on sun visibility parameters
    GLfloat ambientBrightnessModifier = 0.15f + (skySineFactor + 1.0f) * 0.25f;
    GLfloat lightAmbiencyColor[] = { ambientBrightnessModifier, ambientBrightnessModifier, ambientBrightnessModifier + 0.05f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbiencyColor);
}

// ============================================================================
// CORE GRAPHICS VIEWPORT INTERACTION PASS
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
    // Process updated color transformations BEFORE clearing device color bit registers
    AdvancedEnvironmentLightingPipeline();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Camera space allocations
    glTranslatef(0.0f, -0.5f, -12.0f);
    glRotatef(cameraPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraYaw,   0.0f, 1.0f, 0.0f);

    // Health Engine Call
    ForceMotionDiagnosticCheck();

    // RENDER BIOMECHANICAL CHARACTER RIG
    glPushMatrix(); 
    glTranslatef(0.0f, 2.2f, 0.0f); 

    glRotatef(shoulderJointAngle, 1.0f, 0.0f, 0.0f);
    DrawRenderBone(3.8f, 0.38f, 0.85f, 0.35f, 0.35f); 

    glTranslatef(0.0f, -3.8f, 0.0f);
    glRotatef(elbowJointAngle, 1.0f, 0.0f, 0.0f);
    DrawRenderBone(3.2f, 0.28f, 0.35f, 0.65f, 0.85f); 

    glPopMatrix(); 

    glutSwapBuffers();
}

// ============================================================================
// SYSTEM TIME CLOCK TICKER INTERFACE
// ============================================================================
void motionStateTimer(int value) {
    if (isAutoMotionActive) {
        // Ticker driving character bone angles
        automaticTimeModifier += 0.025f * dynamicSpeedMultiplier;
        
        // --- DYNAMIC ADVANCED TIME-OF-DAY TICK STEP ---
        // Character se thoda dheere ghumega aasman taaki naturally look kare (0.005f step sizing)
        environmentTimeClock += 0.005f * dynamicSpeedMultiplier;
        if (environmentTimeClock > 6.28318f) environmentTimeClock = 0.0f; // Reset circle bounding limit at 2pi

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
                }
                break;
        }
    }

    glutPostRedisplay(); 
    glutTimerFunc(16, motionStateTimer, 0);
}

// ============================================================================
// RECOVERY INPUTS & UTILITIES
// ============================================================================
void mouseClickEmergencyBypass(int button, int state, int x, int y) {
    lastRecordedInputTimestamp = GetAbsoluteEnginePrecisionTime();

    if (state == GLUT_DOWN) {
        if (button == GLUT_LEFT_BUTTON) {
            if (currentEngineState == MOTION_STATE_STRESS_TEST) currentEngineState = MOTION_STATE_IDLE;
            else currentEngineState = MOTION_STATE_STRESS_TEST;
        } 
        else if (button == GLUT_RIGHT_BUTTON) {
            // BACKUP RECOVERY KEY FOR SUN SYSTEM FREEZE:
            // Agar day-night freeze ho jaye, right-click karte hi hardware clock bypass phase shift inject karegi
            environmentTimeClock += 1.0471f; // Fast forward sun timeline by 60 degrees manually
            std::cout << "[FALLBACK ENGAGED] Forced Manual Jump in Solar Orbital Path Matrix." << std::endl;
        }
    }
    glutPostRedisplay();
}

void passiveMouseVisionEngine(int x, int y) {
    if (isMouseLookLocked) return;

    float deltaX = (float)(x - lastMouseX);
    float deltaY = (float)(y - lastMouseY);
    lastMouseX = x; lastMouseY = y;

    if (std::fabs(deltaX) > 0.5f || std::fabs(deltaY) > 0.5f) {
        lastRecordedInputTimestamp = GetAbsoluteEnginePrecisionTime();
    }

    cameraYaw += deltaX * mouseSensitivity;
    cameraPitch += deltaY * mouseSensitivity;

    if (cameraPitch > 80.0f)  cameraPitch = 80.0f;
    if (cameraPitch < -80.0f) cameraPitch = -80.0f;

    if (x < 50 || x > 750 || y < 50 || y > 550) {
        lastMouseX = 400; lastMouseY = 300;
        glutWarpPointer(400, 300);
    }
    glutPostRedisplay();
}

void keyboardInputOverride(unsigned char key, int x, int y) {
    lastRecordedInputTimestamp = GetAbsoluteEnginePrecisionTime();

    switch (key) {
        case ' ': isAutoMotionActive = !isAutoMotionActive; break;
        case '1': currentEngineState = MOTION_STATE_IDLE; break;
        case '2': currentEngineState = MOTION_STATE_STRESS_TEST; break;
        
        case 'e': // Force Manual Environment Tick Key
        case 'E':
            environmentTimeClock += 0.2f;
            std::cout << "[ENV RESCUE LOG] Sky Matrix Step Advanced manually to radians: " << environmentTimeClock << std::endl;
            break;
            
        case 27: exit(0);
    }
    glutPostRedisplay();
}

void setupGraphicsContext() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    glutSetCursor(GLUT_CURSOR_NONE); 
    glutWarpPointer(400, 300);
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
    glutCreateWindow("Advanced Environment Cycle & Failsafe Watchdog Master Engine");

    setupGraphicsContext();

    glutKeyboardFunc(keyboardInputOverride);
    glutPassiveMotionFunc(passiveMouseVisionEngine);
    glutMouseFunc(mouseClickEmergencyBypass);
    glutDisplayFunc(displayMainViewport);
    glutReshapeFunc(standardReshape);
    
    glutTimerFunc(16, motionStateTimer, 0);

    std::cout << "==========================================================================" << std::endl;
    std::cout << "    ATMOSPHERIC ROTATION MATRIX & ANTI-STUCK WATCHDOG SYSTEM              " << std::endl;
    std::cout << "==========================================================================" << std::endl;
    std::cout << "  [System Operations Interface]:                                          " << std::endl;
    std::cout << "   - AUTO ATMOSPHERE     -> Background cycles smoothly from Day to Night. " << std::endl;
    std::cout << "   - WATCHDOG ALERT      -> Auto-snaps sky vectors if colors lock up.     " << std::endl;
    std::cout << "   - Press 'E' / Key     : Manual structural environment cycle tick step. " << std::endl;
    std::cout << "   - MOUSE RIGHT CLICK   : Emergency fallback bypass for solar freeze.    " << std::endl;
    std::cout << "==========================================================================" << std::endl;

    glutMainLoop();
    return 0;
}
