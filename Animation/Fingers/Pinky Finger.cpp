#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <iomanip>

// ============================================================================
// SYSTEM CONFIGURATION & CONSTANTS
// ============================================================================
const float PI = 3.14159265358979323846f;
const float DEGREES_TO_RADIANS = PI / 180.0f;

// Biological Constraints for Pinky Finger (Anatomical safety limits in degrees)
const float PINKY_MCP_MIN_FLEX = -10.0f;   // Hyperextension limit
const float PINKY_MCP_MAX_FLEX = 90.0f;    // Full fist closure
const float PINKY_MCP_MIN_SPLAY = -15.0f;  // Moving away from Ring finger (Abduction)
const float PINKY_MCP_MAX_SPLAY = 5.0f;    // Pressing against Ring finger (Adduction)

const float PINKY_PIP_MIN_FLEX = 0.0f;     // Straight finger
const float PINKY_PIP_MAX_FLEX = 110.0f;   // Deep curl (PIP folds more than MCP)

const float PINKY_DIP_MIN_FLEX = 0.0f;     // Straight fingertip
const float PINKY_DIP_MAX_FLEX = 80.0f;    // Tip enclosure

// ============================================================================
// INDEPENDENT PINKY JOINT CONTROLLER (Mathematical State Machine)
// ============================================================================
class PinkyFingerSimulation {
public:
    // Joint angles (Current rendering state)
    float mcp_flexion;  // Main knuckle bend (X-axis)
    float mcp_splay;    // Side-to-side spread (Z-axis)
    float pip_flexion;  // Middle joint bend (X-axis)
    float dip_flexion;  // Tip joint bend (X-axis)

    // Target states for interpolation (Smooth transition anchors)
    float target_mcp_flex;
    float target_mcp_splay;
    float target_pip_flex;
    float target_dip_flex;

    // Kinematic Physics Parameters
    float joint_smoothing; // Physics interpolation factor (Damping constant)
    
    // Bone Dimension Specs (Anatomically scaled relative to a standard hand)
    float mcp_length;
    float pip_length;
    float dip_length;
    float bone_thickness;

    PinkyFingerSimulation() {
        // Initializing finger in an open, relaxed state
        mcp_flexion = 0.0f;
        mcp_splay = 0.0f;
        pip_flexion = 0.0f;
        dip_flexion = 0.0f;

        target_mcp_flex = 0.0f;
        target_mcp_splay = 0.0f;
        target_pip_flex = 0.0f;
        target_dip_flex = 0.0f;

        joint_smoothing = 0.08f; // Lower = smoother/heavier feel, Higher = snappy response

        // Physical proportions of a typical pinky finger (in units)
        mcp_length = 1.15f;
        pip_length = 0.75f;
        dip_length = 0.55f;
        bone_thickness = 0.16f;
    }

    // Mathematical clamping to prevent unnatural bone breaking mutations
    float clampValue(float value, float min_val, float max_val) {
        if (value < min_val) return min_val;
        if (value > max_val) return max_val;
        return value;
    }

    // Frame-by-frame physics loop update (Asymptotic smoothing function)
    void updatePhysics() {
        mcp_flexion += (target_mcp_flex - mcp_flexion) * joint_smoothing;
        mcp_splay   += (target_mcp_splay - mcp_splay) * joint_smoothing;
        pip_flexion += (target_pip_flex - pip_flexion) * joint_smoothing;
        dip_flexion += (target_dip_flex - dip_flexion) * joint_smoothing;

        // Apply strict anatomical boundaries
        mcp_flexion = clampValue(mcp_flexion, PINKY_MCP_MIN_FLEX, PINKY_MCP_MAX_FLEX);
        mcp_splay   = clampValue(mcp_splay, PINKY_MCP_MIN_SPLAY, PINKY_MCP_MAX_SPLAY);
        pip_flexion = clampValue(pip_flexion, PINKY_PIP_MIN_FLEX, PINKY_PIP_MAX_FLEX);
        dip_flexion = clampValue(dip_flexion, PINKY_DIP_MIN_FLEX, PINKY_DIP_MAX_FLEX);
    }

    // High fidelity rendering using OpenGL Matrix stack
    void render() {
        glPushMatrix();

        // --------------------------------------------------------------------
        // VISUAL SEGMENT 1: Metacarpophalangeal (MCP) Joint & Phalanx
        // --------------------------------------------------------------------
        // Apply transformation matrices sequentially (Translate -> Splay -> Flex)
        glRotatef(mcp_splay, 0.0f, 1.0f, 0.0f);     // Side-to-side spread
        glRotatef(mcp_flexion, 1.0f, 0.0f, 0.0f);   // Main knuckle bending
        
        // Draw MCP Joint Sphere
        glColor3f(0.80f, 0.55f, 0.45f); // Joint skin tone highlighting
        glutSolidSphere(bone_thickness * 1.1f, 16, 16);

        // Draw Proximal Bone Cylinder
        glColor3f(0.85f, 0.62f, 0.52f);
        glPushMatrix();
        // Align cylinder to point upwards along Y-axis
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        GLUquadric* quad = gluNewQuadric();
        gluCylinder(quad, bone_thickness, bone_thickness * 0.9f, mcp_length, 16, 16);
        glPopMatrix();

        // Advance local coordinate system to the end of proximal bone
        glTranslatef(0.0f, mcp_length, 0.0f);

        // --------------------------------------------------------------------
        // VISUAL SEGMENT 2: Proximal Interphalangeal (PIP) Joint & Phalanx
        // --------------------------------------------------------------------
        glRotatef(pip_flexion, 1.0f, 0.0f, 0.0f);   // Interphalangeal flexion
        
        // Draw PIP Joint Sphere
        glColor3f(0.78f, 0.53f, 0.43f);
        glutSolidSphere(bone_thickness * 0.95f, 16, 16);

        // Draw Intermediate Bone Cylinder
        glColor3f(0.85f, 0.62f, 0.52f);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(quad, bone_thickness * 0.9f, bone_thickness * 0.8f, pip_length, 16, 16);
        glPopMatrix();

        // Advance matrix pipeline to the next node
        glTranslatef(0.0f, pip_length, 0.0f);

        // --------------------------------------------------------------------
        // VISUAL SEGMENT 3: Distal Interphalangeal (DIP) Joint & Fingertip
        // --------------------------------------------------------------------
        glRotatef(dip_flexion, 1.0f, 0.0f, 0.0f);   // Fingertip flexion
        
        // Draw DIP Joint Sphere
        glColor3f(0.75f, 0.50f, 0.40f);
        glutSolidSphere(bone_thickness * 0.85f, 16, 16);

        // Draw Distal Bone Cylinder & Cap (Fingertip contour)
        glColor3f(0.83f, 0.60f, 0.50f);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(quad, bone_thickness * 0.8f, bone_thickness * 0.5f, dip_length * 0.7f, 16, 16);
        // Cap off the tip with a smooth hemisphere
        glTranslatef(0.0f, 0.0f, dip_length * 0.7f);
        glutSolidSphere(bone_thickness * 0.5f, 16, 16);
        glPopMatrix();

        gluDeleteQuadric(quad);
        glPopMatrix(); // Restore base transform pipeline
    }
};

// ============================================================================
// GLOBAL APPLICATION CONTEXT & TIMELINE COORDINATOR
// ============================================================================
PinkyFingerSimulation pinkySimulator;
float globalCameraYaw = 45.0f;
float globalCameraPitch = 15.0f;
int animationTimeTrack = 0;
int currentMotionState = 0; // 0: Idle, 1: Spreading, 2: Flexing, 3: Object Grasping Simulation

void displayCallback() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Scene Navigation matrix positioning
    glTranslatef(0.0f, -1.0f, -7.0f);
    glRotatef(globalCameraPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(globalCameraYaw, 0.0f, 1.0f, 0.0f);

    // --- REFERENCE PALM BASE CORE ---
    // Drawing a rigid anchor point where the pinky attaches to the wrist mechanics
    glColor3f(0.4f, 0.45f, 0.5f); // Structural technical color
    glPushMatrix();
    glScalef(1.5f, 0.4f, 1.2f);
    glutWireCube(1.0f);
    glPopMatrix();

    // --- PIPELINE ISOLATION FOR PINKY DEPLOYMENT ---
    glPushMatrix();
    // Offset the pinky to the left side edge of the theoretical hand coordinate layout
    glTranslatef(-0.65f, 0.2f, 0.0f);
    
    // Fire the granular hardware renderer engine for pinky finger
    pinkySimulator.render();
    glPopMatrix();

    // Engine Telemetry UI Overlay printing on console stream
    std::cout << "\r[Telemetry] MCP_FX: " << std::fixed << std::setw(6) << std::setprecision(2) << pinkySimulator.mcp_flexion 
              << " | SPLAY: " << std::setw(6) << pinkySimulator.mcp_splay
              << " | PIP_FX: " << std::setw(6) << pinkySimulator.pip_flexion
              << " | DIP_FX: " << std::setw(6) << pinkySimulator.dip_flexion << std::flush;

    glutSwapBuffers();
}

void simulationTicker(int val) {
    animationTimeTrack++;

    // Automating complex movement loops every few frames to show realistic dexterity
    if (animationTimeTrack % 250 == 0) {
        currentMotionState = (currentMotionState + 1) % 4;
    }

    switch(currentMotionState) {
        case 0: // Natural Relaxed State
            pinkySimulator.target_mcp_flex  = 10.0f;
            pinkySimulator.target_mcp_splay = 0.0f;
            pinkySimulator.target_pip_flex  = 15.0f;
            pinkySimulator.target_dip_flex  = 10.0f;
            break;
        case 1: // Max Extension and Lateral Splay (Uncurling and spreading outward)
            pinkySimulator.target_mcp_flex  = -5.0f;
            pinkySimulator.target_mcp_splay = -12.0f; // Splay out
            pinkySimulator.target_pip_flex  = 0.0f;
            pinkySimulator.target_dip_flex  = 0.0f;
            break;
        case 2: // Hyper flexion Sequence (Tight full isolated fold)
            pinkySimulator.target_mcp_flex  = 80.0f;
            pinkySimulator.target_mcp_splay = 4.0f; // Pulling back inward
            pinkySimulator.target_pip_flex  = 95.0f;
            pinkySimulator.target_dip_flex  = 75.0f;
            break;
        case 3: // Complex Object Grasping (Organic multi-axis lock curvature)
            // Realistically, DIP and PIP lock onto shapes ahead of the main knuckle
            float sineModulator = sin(animationTimeTrack * 0.03f);
            pinkySimulator.target_mcp_flex  = 45.0f + (sineModulator * 15.0f);
            pinkySimulator.target_mcp_splay = -2.0f;
            pinkySimulator.target_pip_flex  = 75.0f + (sineModulator * 20.0f);
            pinkySimulator.target_dip_flex  = 55.0f + (sineModulator * 15.0f);
            break;
    }

    // Execute internal physics computation
    pinkySimulator.updatePhysics();

    glutPostRedisplay();
    glutTimerFunc(16, simulationTicker, 0); // Steady 60Hz tick injection
}

void windowReshape(int width, int height) {
    if (height == 0) height = 1;
    float aspect = (float)width / (float)height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, aspect, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void initGraphicsEngine() {
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH); // Enables high performance smooth shading maps

    // Hardware Lighting Pipeline configuration
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    GLfloat lightAmbient[]  = { 0.25f, 0.25f, 0.25f, 1.0f };
    GLfloat lightDiffuse[]  = { 0.85f, 0.85f, 0.85f, 1.0f };
    GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat lightPosition[] = { 5.0f, 8.0f, 6.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    // Material properties for subtle bone/skin specular sheen
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    GLfloat materialSpecular[] = { 0.15f, 0.15f, 0.15f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    glMateriali(GL_FRONT, GL_SHININESS, 32);

    glClearColor(0.08f, 0.09f, 0.1f, 1.0f);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Advanced Isolated Pinky Kinematics Engine");

    initGraphicsEngine();

    glutDisplayFunc(displayCallback);
    glutReshapeFunc(windowReshape);
    glutTimerFunc(16, simulationTicker, 0);

    std::cout << "=========================================================" << std::endl;
    std::cout << "  PINKY CORE KINEMATICS INITIALIZED ENGINE ONLINE        " << std::endl;
    std::cout << "=========================================================" << std::endl;

    glutMainLoop();
    return 0;
}
