#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <chrono>
#include <string>

// ============================================================================
// STATE MACHINE & MATRIX PARAMETERS
// ============================================================================
enum MotionEngineState {
    MOTION_STATE_IDLE = 0,
    MOTION_STATE_STRESS_TEST = 1,
    MOTION_STATE_UNFREEZE_OVERRIDE = 2
};

enum QuestStatus {
    QUEST_NOT_STARTED = 0,
    QUEST_ACTIVE = 1,
    QUEST_COMPLETED_AWAITING_REWARD = 2,
    QUEST_FULLY_RESOLVED = 3
};

// --- CORE SYSTEM REGISTERS ---
MotionEngineState currentEngineState = MOTION_STATE_IDLE;
float automaticTimeModifier = 0.0f;
float dynamicSpeedMultiplier = 1.0f;
bool isAutoMotionActive = true;

// Joint Rig & Camera Coordinates
float shoulderJointAngle = -30.0f; float elbowJointAngle = 45.0f;
float cameraYaw = 0.0f; float cameraPitch = 0.0f; float mouseSensitivity = 0.15f;
int lastMouseX = 400; int lastMouseY = 300; bool isMouseLookLocked = false;
double lastRecordedInputTimestamp = 0.0;

// Environmental Sky Matrices
float environmentTimeClock = 0.0f;
float skyColorR = 0.08f; float skyColorG = 0.09f; float skyColorB = 0.12f;

// --- ADVANCED ADVANCED QUEST TRANSACTION REGISTERS ---
QuestStatus currentQuestState = QUEST_NOT_STARTED;
std::string questName = "Operation: Overlord Bone Rig Recovery";
unsigned int playerGoldInventory = 0;
bool isRewardDroppedVisualSignalActive = false;
float rewardVisualPulseTimer = 0.0f;

// Transaction Watchdog Profilers (Reward Protection Layer)
double questCompletionTimestamp = 0.0;
unsigned int rewardVerificationStrikes = 0;
unsigned long totalForcedRewardInjections = 0;

// Regular Rig Watchdog Registers
float lastCheckedShoulderAngle = -999.0f; float lastCheckedElbowAngle = -999.0f;
unsigned int motionStuckStrikes = 0;
float lastCheckedSkyColorR = -1.0f; unsigned int environmentStuckStrikes = 0;

std::chrono::high_resolution_clock::time_point engineInitClock = std::chrono::high_resolution_clock::now();

double GetAbsoluteEnginePrecisionTime() {
    auto currentClockTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedSecs = currentClockTime - engineInitClock;
    return elapsedSecs.count();
}

// ============================================================================
// QUEST LEDGER TRANSACTION VERIFIER (The Failsafe Reward Unfreeze Guard)
// ============================================================================
void ForceMotionDiagnosticCheck() {
    double currentSystemTick = GetAbsoluteEnginePrecisionTime();
    static double lastWatchdogVerificationTime = 0.0;

    if (currentSystemTick - lastWatchdogVerificationTime >= 0.3) {
        
        // --- FEATURE: REWARD DROP FAULT IDENTIFIER ---
        // Agar quest state 'AWAITING_REWARD' par fansi hui hai aur completion ke baad 
        // 4 seconds se zyada beet chuke hain bina transaction resolve hue (State fully resolved nahi hui),
        // toh iska matlab reward logic memory me jam ho chuka hai. Watchdog ise forcibly override karega!
        if (currentQuestState == QUEST_COMPLETED_AWAITING_REWARD) {
            double timeElapsedSinceQuestComplete = currentSystemTick - questCompletionTimestamp;
            
            if (timeElapsedSinceQuestComplete > 4.0) { // 4 Seconds Deadlock Window
                rewardVerificationStrikes++;
                
                if (rewardVerificationStrikes >= 2) { // Double verification check pass
                    std::cout << "\n[❌ QUEST TRANSACTION TRANSACTION FAULT DETECTED!]" << std::endl;
                    std::cout << "[DIAGNOSTIC] Quest: '" << questName << "' is stuck in Awaiting State." << std::endl;
                    std::cout << "[RECOVERY] Running Memory Override... Forcefully dropping 500 Gold into Inventory!" << std::endl;
                    
                    // Forcefully complete the transaction in memory ledger
                    playerGoldInventory += 500;
                    currentQuestState = QUEST_FULLY_RESOLVED;
                    
                    // Visual notification trigger (Cyan bone turns bright gold for a moment)
                    isRewardDroppedVisualSignalActive = true;
                    rewardVisualPulseTimer = 0.0f;
                    
                    totalForcedRewardInjections++;
                    rewardVerificationStrikes = 0;
                    
                    std::cout << "[RECOVERY STATUS] Ledger Restructured. Current Inventory Gold: " << playerGoldInventory << " | Corrections: " << totalForcedRewardInjections << std::endl;
                }
            }
        } else {
            rewardVerificationStrikes = 0;
        }

        // --- OTHER CORE RIG WATCHDOG CHECKS ---
        if (isAutoMotionActive && (std::fabs(skyColorR - lastCheckedSkyColorR) < 0.00001f)) {
            environmentStuckStrikes++;
            if (environmentStuckStrikes >= 4) { environmentTimeClock += 0.5f; environmentStuckStrikes = 0; }
        } else { environmentStuckStrikes = 0; }
        lastCheckedSkyColorR = skyColorR;

        if (isAutoMotionActive && (std::fabs(shoulderJointAngle - lastCheckedShoulderAngle) < 0.0001f)) {
            motionStuckStrikes++;
            if (motionStuckStrikes >= 3) { currentEngineState = MOTION_STATE_UNFREEZE_OVERRIDE; automaticTimeModifier += 1.5f; shoulderJointAngle += 10.0f; motionStuckStrikes = 0; }
        } else { motionStuckStrikes = 0; }
        lastCheckedShoulderAngle = shoulderJointAngle; lastCheckedElbowAngle = elbowJointAngle;

        lastWatchdogVerificationTime = currentSystemTick;
    }
}

// ============================================================================
// DYNAMIC QUEST TRIGGER EVENT CONTROLLER
// ============================================================================
void ExecuteQuestMilestoneSignal() {
    double activeClock = GetAbsoluteEnginePrecisionTime();

    if (currentQuestState == QUEST_NOT_STARTED) {
        currentQuestState = QUEST_ACTIVE;
        std::cout << "\n[QUEST LOG] Quest Initialized: [" << questName << "] - Target: Flex Rig 3 Times." << std::endl;
    } 
    else if (currentQuestState == QUEST_ACTIVE) {
        // Quest objectives completed, moving to verification phase
        currentQuestState = QUEST_COMPLETED_AWAITING_REWARD;
        questCompletionTimestamp = activeClock; // Stamp the exact completion time
        std::cout << "\n[QUEST LOG] Objectives Met! Status set to: AWAITING_REWARD." << std::endl;
        std::cout << "[SYSTEM] Simulated Glitch Induced: Standard Reward Thread is now intentionally paused." << std::endl;
    }
    else if (currentQuestState == QUEST_FULLY_RESOLVED) {
        // Reset system to play again
        currentQuestState = QUEST_NOT_STARTED;
        std::cout << "\n[QUEST LOG] Resetting pipeline ledger for development simulation loop." << std::endl;
    }
}

// ============================================================================
// ENVIRONMENTAL LIGHTING & GRAPHICS LAYERS
// ============================================================================
void AdvancedEnvironmentLightingPipeline() {
    float skySineFactor = std::sin(environmentTimeClock);
    skyColorR = 0.08f + (skySineFactor + 1.0f) * 0.15f; 
    skyColorG = 0.09f + (skySineFactor + 1.0f) * 0.25f;
    skyColorB = 0.12f + (skySineFactor + 1.0f) * 0.35f;
    glClearColor(skyColorR, skyColorG, skyColorB, 1.0f);

    GLfloat structuralLight0Pos[] = { 2.0f + std::cos(environmentTimeClock) * 10.0f, 4.0f + skySineFactor * 10.0f, 3.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, structuralLight0Pos);
}

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
    AdvancedEnvironmentLightingPipeline();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Camera Transformations
    glTranslatef(0.0f, -0.5f, -12.0f);
    glRotatef(cameraPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraYaw,   0.0f, 1.0f, 0.0f);

    ForceMotionDiagnosticCheck();

    // CHARACTER RENDER MATRIX BLOCK
    glPushMatrix(); 
    glTranslatef(0.0f, 2.2f, 0.0f); 

    glRotatef(shoulderJointAngle, 1.0f, 0.0f, 0.0f);
    DrawRenderBone(3.8f, 0.38f, 0.85f, 0.35f, 0.35f); // Upper Arm (Crimson)

    glTranslatef(0.0f, -3.8f, 0.0f);
    glRotatef(elbowJointAngle, 1.0f, 0.0f, 0.0f);
    
    // --- FEATURE: REWARD FLASH SIGNAL MODIFIER ---
    // Agar watchdog reward inject karta hai, toh forearm kuch seconds ke liye Gold color me flash karega
    if (isRewardDroppedVisualSignalActive && rewardVisualPulseTimer < 2.0f) {
        DrawRenderBone(3.2f, 0.28f, 1.0f, 0.84f, 0.0f); // Bright Gold Reward Flash Color!
    } else {
        DrawRenderBone(3.2f, 0.28f, 0.35f, 0.65f, 0.85f); // Standard Cyan Forearm
    }

    glPopMatrix(); 

    glutSwapBuffers();
}

// ============================================================================
// SYSTEM TIME CLOCK TICKER INTERFACE
// ============================================================================
void motionStateTimer(int value) {
    if (isAutoMotionActive) {
        automaticTimeModifier += 0.025f * dynamicSpeedMultiplier;
        environmentTimeClock += 0.005f * dynamicSpeedMultiplier;

        // Process color signal timers smoothly across delta clock passes
        if (isRewardDroppedVisualSignalActive) {
            rewardVisualPulseTimer += 0.016f; // Standard frame step index acceleration
            if (rewardVisualPulseTimer >= 2.0f) isRewardDroppedVisualSignalActive = false;
        }

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
                    currentEngineState = MOTION_STATE_IDLE; dynamicSpeedMultiplier = 1.0f;
                }
                break;
        }
    }

    glutPostRedisplay(); 
    glutTimerFunc(16, motionStateTimer, 0);
}

// ============================================================================
// INPUT OVERRIDES & INTERACTION CONTROLS
// ============================================================================
void mouseClickEmergencyBypass(int button, int state, int x, int y) {
    lastRecordedInputTimestamp = GetAbsoluteEnginePrecisionTime();
    
    // MOUSE FALLBACK FOR THE CRITICAL QUEST GLITCH:
    // Keyboard kaam na kare toh mouse ke Left Click se aap quest events fire kar sakte hain!
    if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON) {
        ExecuteQuestMilestoneSignal();
    }
    glutPostRedisplay();
}

void passiveMouseVisionEngine(int x, int y) {
    float deltaX = (float)(x - lastMouseX); float deltaY = (float)(y - lastMouseY);
    lastMouseX = x; lastMouseY = y;
    if (std::fabs(deltaX) > 0.5f || std::fabs(deltaY) > 0.5f) lastRecordedInputTimestamp = GetAbsoluteEnginePrecisionTime();

    cameraYaw += deltaX * mouseSensitivity; cameraPitch += deltaY * mouseSensitivity;
    if (cameraPitch > 80.0f) cameraPitch = 80.0f; if (cameraPitch < -80.0f) cameraPitch = -80.0f;

    if (x < 50 || x > 750 || y < 50 || y > 550) { lastMouseX = 400; lastMouseY = 300; glutWarpPointer(400, 300); }
    glutPostRedisplay();
}

void keyboardInputOverride(unsigned char key, int x, int y) {
    lastRecordedInputTimestamp = GetAbsoluteEnginePrecisionTime();

    switch (key) {
        case ' ': isAutoMotionActive = !isAutoMotionActive; break;
        
        case 'q': // Keyboard Quest Trigger key
        case 'Q':
            ExecuteQuestMilestoneSignal();
            break;

        case 'i': // Inventory Debug Check
        case 'I':
            std::cout << "\n[INVENTORY LOG] Current Balance: " << playerGoldInventory << " Gold | Quest State: " << currentQuestState << std::endl;
            break;

        case 27: exit(0);
    }
    glutPostRedisplay();
}

void setupGraphicsContext() {
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_COLOR_MATERIAL);
    glutSetCursor(GLUT_CURSOR_NONE); glutWarpPointer(400, 300);
    lastRecordedInputTimestamp = GetAbsoluteEnginePrecisionTime();
}

void standardReshape(int w, int h) {
    if (h == 0) h = 1; glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(45.0f, (float)w / (float)h, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Advanced Quest Transaction Failsafe & Reward Unfreeze Engine");

    setupGraphicsContext();

    glutKeyboardFunc(keyboardInputOverride);
    glutPassiveMotionFunc(passiveMouseVisionEngine);
    glutMouseFunc(mouseClickEmergencyBypass);
    glutDisplayFunc(displayMainViewport);
    glutReshapeFunc(standardReshape);
    
    glutTimerFunc(16, motionStateTimer, 0);

    std::cout << "==========================================================================" << std::endl;
    std::cout << "    QUEST TRANSACTION LEDGER & REWARD SYSTEM ONLINE                        " << std::endl;
    std::cout << "==========================================================================" << std::endl;
    std::cout << "  [Quest Simulation Controls]:                                            " << std::endl;
    std::cout << "   - Press 'Q' (or Left Click) : 1st press starts Quest.                  " << std::endl;
    std::cout << "                               : 2nd press Completes it & Locks Reward.  " << std::endl;
    std::cout << "   - AUTOMATIC FAILSAFE        : If reward hangs for 4 secs, Watchdog     " << std::endl;
    std::cout << "                                 injects Gold & Flashes the arm GOLD!    " << std::endl;
    std::cout << "   - Press 'I'                 : Check Live Wallet Balance & Quest State. " << std::endl;
    std::cout << "==========================================================================" << std::endl;

    glutMainLoop();
    return 0;
}
