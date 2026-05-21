#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <iomanip>

// ============================================================================
// SYSTEM CONFIGURATION & ANATOMICAL CONSTANTS FOR MIDDLE FINGER
// ============================================================================
const float MATH_PI = 3.14159265f;

// Biomechanical Constraints specific to the Middle Finger (Central Axis)
const float MIDDLE_MCP_MIN_FLEX = -5.0f;    // Minimal hyperextension allowed by tendons
const float MIDDLE_MCP_MAX_FLEX = 90.0f;    // Full 90-degree knuckle drop
const float MIDDLE_MCP_MIN_SPLAY = -2.0f;   // Structural centerline - almost zero lateral drift
const float MIDDLE_MCP_MAX_SPLAY = 2.0f;    // Minimal deviation from hand line

const float MIDDLE_PIP_MIN_FLEX = 0.0f;     // Dead straight extension
const float MIDDLE_PIP_MAX_FLEX = 115.0f;   // Deepest flexion among all fingers for maximum leverage

const float MIDDLE_DIP_MIN_FLEX = 0.0f;     // Flat tip extension
const float MIDDLE_DIP_MAX_FLEX = 90.0f;    // Sharp claw angle locking capability

// ============================================================================
// INDEPENDENT MIDDLE FINGER CONTROLLER (Central Axis Matrix Node)
// ============================================================================
class MiddleFingerSimulation {
public:
    // Real-time calculated Dynamic Joint Angles
    float mcp_flexion;   // Base flexion (X-Axis)
    float mcp_splay;     // Central axis stability alignment (Y-Axis)
    float pip_flexion;   // Central power joint (X-Axis)
    float dip_flexion;   // Precision tip enclosure (X-Axis)

    // Independent Target Anchors for Multi-Axis Transitions
    float target_mcp_flex;
    float target_mcp_splay;
    float target_pip_flex;
    float target_dip_flex;

    // Muscle Dynamics (Heaviest damping factor due to maximum muscle mass)
    float muscle_inertia; 
    
    // Bone Proportions (Anatomical Apex: Longest and thickest finger structure)
    float mcp_length;
    float pip_length;
    float dip_length;
    float bone_thickness;

    MiddleFingerSimulation() {
        // Safe defaults - Neutral open resting hand stance
        mcp_flexion = 0.0f;
        mcp_splay = 0.0f;
        pip_flexion = 0.0f;
        dip_flexion = 0.0f;

        target_mcp_flex = 0.0f;
        target_mcp_splay = 0.0f;
        target_pip_flex = 0.0f;
        target_dip_flex = 0.0f;

        // Higher muscle inertia = solid, powerful, and weighted transitions
        muscle_inertia = 0.070f; 

        // Anatomical scaling matrices: The structural peak of the hand
        mcp_length = 1.48f;      // Apex length for proximal phalanx
        pip_length = 0.96f;      // Mid phalanx segment
        dip_length = 0.68f;      // Extended distal fingertip
        bone_thickness = 0.24f;  // Max structural diameter for gripping load distribution
    }

    float enforceAnatomicalLimits(float value, float min_limit, float max_limit) {
        if (value < min_limit) return min_limit;
        if (value > max_limit) return max_limit;
        return value;
    }

    // Mathematical Muscle Simulator Loop (Asymptotic Interpolation Engine)
    void updateKinematics() {
        mcp_flexion += (target_mcp_flex - mcp_flexion) * muscle_inertia;
        mcp_splay   += (target_mcp_splay - mcp_splay) * muscle_inertia;
        pip_flexion += (target_pip_flex - pip_flexion) * muscle_inertia;
        dip_flexion += (target_dip_flex - dip_flexion) * muscle_inertia;

        // Apply strict biomechanical boundaries
        mcp_flexion = enforceAnatomicalLimits(mcp_flexion, MIDDLE_MCP_MIN_FLEX, MIDDLE_MCP_MAX_FLEX);
        mcp_splay   = enforceAnatomicalLimits(mcp_splay, MIDDLE_MCP_MIN_SPLAY, MIDDLE_MCP_MAX_SPLAY);
        pip_flexion = enforceAnatomicalLimits(pip_flexion, MIDDLE_PIP_MIN_FLEX, MIDDLE_PIP_MAX_FLEX);
        dip_flexion = enforceAnatomicalLimits(dip_flexion, MIDDLE_DIP_MIN_FLEX, MIDDLE_DIP_MAX_FLEX);
    }

    // High fidelity OpenGL matrix projection renderer
    void render() {
        glPushMatrix();

        // --------------------------------------------------------------------
        // SEGMENT 1: Middle Metacarpophalangeal Node (The Hand Center Knuckle)
        // --------------------------------------------------------------------
        glRotatef(mcp_splay, 0.0f, 1.0f, 0.0f);     // Minimal central balancing
        glRotatef(mcp_flexion, 1.0f, 0.0f, 0.0f);   // Massive structural curl
        
        // Render Center MCP Joint Base Spherical Core
        glColor3f(0.83f, 0.58f, 0.48f); 
        glutSolidSphere(bone_thickness * 1.15f, 16, 16);

        // Render Proximal Phalanx (Longest bone segment)
        glColor3f(0.87f, 0.65f, 0.55f);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        GLUquadric* middleQuad = gluNewQuadric();
        gluCylinder(middleQuad, bone_thickness, bone_thickness * 0.92f, mcp_length, 16, 16);
        glPopMatrix();

        // Shift coordinates up to the PIP hinge boundary
        glTranslatef(0.0f, mcp_length, 0.0f);

        // --------------------------------------------------------------------
        // SEGMENT 2: Middle Proximal Interphalangeal Node (Power Hinge)
        // --------------------------------------------------------------------
        glRotatef(pip_flexion, 1.0f, 0.0f, 0.0f);
        
        // Render PIP Joint Spherical Core
        glColor3f(0.80f, 0.55f, 0.45f);
        glutSolidSphere(bone_thickness * 0.98f, 16, 16);

        // Render Intermediate Phalanx 
        glColor3f(0.87f, 0.65f, 0.55f);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(middleQuad, bone_thickness * 0.92f, bone_thickness * 0.82f, pip_length, 16, 16);
        glPopMatrix();

        // Shift rendering head to DIP node
        glTranslatef(0.0f, pip_length, 0.0f);

        // --------------------------------------------------------------------
        // SEGMENT 3: Middle Distal Interphalangeal Node (Apex Fingertip)
        // --------------------------------------------------------------------
        glRotatef(dip_flexion, 1.0f, 0.0f, 0.0f);
        
        // Render DIP Joint Spherical Core
        glColor3f(0.77f, 0.52f, 0.42f);
        glutSolidSphere(bone_thickness * 0.88f, 16, 16);

        // Render Distal Phalanx & Apex Tip Contour
        glColor3f(0.85f, 0.62f, 0.52f);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(middleQuad, bone_thickness * 0.82f, bone_thickness * 0.55f, dip_length * 0.75f, 16, 16);
        
        // Terminal Hemisphere Cap (Finger point apex)
        glTranslatef(0.0f, 0.0f, dip_length * 0.75f);
        glutSolidSphere(bone_thickness * 0.55f, 16, 16);
        glPopMatrix();

        gluDeleteQuadric(middleQuad);
        glPopMatrix(); // Clear local state completely
    }
};

// ============================================================================
// ENGINE WORKSPACE ENVIRONMENT
// ============================================================================
MiddleFingerSimulation middleSimulator;
float viewYaw = 15.0f;
float viewPitch = 25.0f;
int globalTimeStep = 0;
int loopPhase = 0; // 0: Resting, 1: High Stretched Extension, 2: Deep Flexion Grab, 3: High Torque Sine Pulse

void renderViewScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Viewport workspace positioning
    glTranslatef(0.0f, -1.5f, -8.5f);
    glRotatef(viewPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(viewYaw, 0.0f, 1.0f, 0.0f);

    // --- METACARPAL BASE ANCHOR HUB ---
    glColor3f(0.3f, 0.32f, 0.35f);
    glPushMatrix();
    glScalef(2.0f, 0.50f, 1.5f);
    glutWireCube(1.0f);
    glPopMatrix();

    // --- MATRIX DEPLOYMENT POINT FOR MIDDLE FINGER AXIS ---
    glPushMatrix();
    // Centered right in the exact geometric hub of the wrist assembly
    glTranslatef(0.15f, 0.25f, 0.0f);
    
    // Fire interpolation telemetry and rasterize
    middleSimulator.render();
    glPopMatrix();

    // Console Telemetry Data Stream
    std::cout << "\r[Telemetry | Middle Finger] MCP_FX: " << std::fixed << std::setw(6) << std::setprecision(2) << middleSimulator.mcp_flexion 
              << " | SPLAY: " << std::setw(6) << middleSimulator.mcp_splay
              << " | PIP_FX: " << std::setw(6) << middleSimulator.pip_flexion
              << " | DIP_FX: " << std::setw(6) << middleSimulator.dip_flexion << std::flush;

    glutSwapBuffers();
}

void mainSimulationTicker(int sysVal) {
    globalTimeStep++;

    // Staged Behavior State Machine
    if (globalTimeStep % 250 == 0) {
        loopPhase = (loopPhase + 1) % 4;
    }

    switch(loopPhase) {
        case 0: // Natural Resting State (Zero splay, slight functional arc)
            middleSimulator.target_mcp_flex  = 10.0f;
            middleSimulator.target_mcp_splay = 0.0f; // Dead center
            middleSimulator.target_pip_flex  = 15.0f;
            middleSimulator.target_dip_flex  = 10.0f;
            break;
        case 1: // Max Extension Stretches (Locks dead center, forces open palm structure)
            middleSimulator.target_mcp_flex  = -4.0f;
            middleSimulator.target_mcp_splay = 0.0f; // No drift
            middleSimulator.target_pip_flex  = 0.0f;
            middleSimulator.target_dip_flex  = 0.0f;
            break;
        case 2: // Maximum Core Gripping Flexion Sequence
            middleSimulator.target_mcp_flex  = 88.0f;
            middleSimulator.target_mcp_splay = 0.0f; 
            middleSimulator.target_pip_flex  = 112.0f; // Massive curl profile
            middleSimulator.target_dip_flex  = 85.0f;
            break;
        case 3: // Complex Object Grab-Release Cycles
            float sineOscillator = sin(globalTimeStep * 0.022f);
            middleSimulator.target_mcp_flex  = 55.0f + (sineOscillator * 15.0f);
            middleSimulator.target_mcp_splay = 0.0f;
            middleSimulator.target_pip_flex  = 85.0f + (sineOscillator * 20.0f); // High elasticity simulation
            middleSimulator.target_dip_flex  = 65.0f + (sineOscillator * 12.0f);
            break;
    }

    // Process kinematic engine equations
    middleSimulator.updateKinematics();

    glutPostRedisplay();
    glutTimerFunc(16, mainSimulationTicker, 0);
}

void resizeViewportHandler(int width, int height) {
    if (height == 0) height = 1;
    float aspectPerspective = (float)width / (float)height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, aspectPerspective, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void configureLightingPipeline() {
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    GLfloat ambientSpec[]  = { 0.20f, 0.20f, 0.22f, 1.0f };
    GLfloat diffuseSpec[]  = { 0.88f, 0.88f, 0.85f, 1.0f };
    GLfloat specularSpec[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat positionSpec[] = { 0.0f, 10.0f, 4.0f, 1.0f }; // Top-down structural light injection

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientSpec);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseSpec);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularSpec);
    glLightfv(GL_LIGHT0, GL_POSITION, positionSpec);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    GLfloat materialReflect[] = { 0.25f, 0.25f, 0.25f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialReflect);
    glMateriali(GL_FRONT, GL_SHININESS, 50);

    glClearColor(0.06f, 0.07f, 0.08f, 1.0f);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Advanced Isolated Middle Finger Central Axis Engine");

    configureLightingPipeline();

    glutDisplayFunc(renderViewScene);
    glutReshapeFunc(resizeViewportHandler);
    glutTimerFunc(16, mainSimulationTicker, 0);

    std::cout << "=========================================================" << std::endl;
    std::cout << "  MIDDLE FINGER AXIS INITIALIZED MECHANICS ONLINE        " << std::endl;
    std::cout << "=========================================================" << std::endl;

    glutMainLoop();
    return 0;
}
