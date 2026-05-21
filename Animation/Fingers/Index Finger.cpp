#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <iomanip>

// ============================================================================
// SYSTEM CONFIGURATION & ANATOMICAL CONSTANTS FOR INDEX FINGER
// ============================================================================
const float INDEX_PI = 3.14159265f;

// Biological Constraints specific to the Index Finger (Precision Node)
const float INDEX_MCP_MIN_FLEX = -10.0f;   // Good hyperextension for pointing postures
const float INDEX_MCP_MAX_FLEX = 90.0f;    // Standard fist closure
const float INDEX_MCP_MIN_SPLAY = -7.0f;   // Pressing against Middle finger (Adduction)
const float INDEX_MCP_MAX_SPLAY = 15.0f;   // Splaying outwards towards the Thumb (Abduction)

const float INDEX_PIP_MIN_FLEX = 0.0f;     // Straight alignment
const float INDEX_PIP_MAX_FLEX = 100.0f;   // Sharp functional bend for holding tools

const float INDEX_DIP_MIN_FLEX = 0.0f;     // Extended fingertip
const float INDEX_DIP_MAX_FLEX = 80.0f;    // Precision tip compression (Pinch mechanic)

// ============================================================================
// INDEPENDENT INDEX FINGER CONTROLLER (Precision Kinematics Matrix Node)
// ============================================================================
class IndexFingerSimulation {
public:
    // Real-time calculated Dynamic Joint Angles
    float mcp_flexion;   // Base flexion (X-Axis)
    float mcp_splay;     // Lateral adjustment toward Thumb/Middle (Y-Axis)
    float pip_flexion;   // Middle joint (X-Axis)
    float dip_flexion;   // Tip joint closure (X-Axis)

    // Independent Target Anchors for Multi-Axis Transitions
    float target_mcp_flex;
    float target_mcp_splay;
    float target_pip_flex;
    float target_dip_flex;

    // Physics Tuning (Snappier response for high dexterity simulation)
    float muscle_responsiveness; 
    
    // Bone Proportions (Slightly shorter than Middle, longer than Ring)
    float mcp_length;
    float pip_length;
    float dip_length;
    float bone_thickness;

    IndexFingerSimulation() {
        // Safe defaults - Neutral open resting hand stance
        mcp_flexion = 0.0f;
        mcp_splay = 0.0f;
        pip_flexion = 0.0f;
        dip_flexion = 0.0f;

        target_mcp_flex = 0.0f;
        target_mcp_splay = 0.0f;
        target_pip_flex = 0.0f;
        target_dip_flex = 0.0f;

        // Snappier responsive tuning (0.095f) since Index finger moves faster in real-life
        muscle_responsiveness = 0.095f; 

        // Anatomical scaling matrices for Index finger
        mcp_length = 1.38f;      // Strong proximal lever
        pip_length = 0.90f;      // Mid phalanx segment
        dip_length = 0.64f;      // Precision tactile fingertip
        bone_thickness = 0.22f;  // Sleeker than Middle finger, structural for precision pinch
    }

    float applyAnatomicalLimits(float val, float min_limit, float max_limit) {
        if (val < min_limit) return min_limit;
        if (val > max_limit) return max_limit;
        return val;
    }

    // Mathematical Muscle Simulator Loop (Asymptotic Interpolation Engine)
    void updateKinematics() {
        mcp_flexion += (target_mcp_flex - mcp_flexion) * muscle_responsiveness;
        mcp_splay   += (target_mcp_splay - mcp_splay) * muscle_responsiveness;
        pip_flexion += (target_pip_flex - pip_flexion) * muscle_responsiveness;
        dip_flexion += (target_dip_flex - dip_flexion) * muscle_responsiveness;

        // Force strict biological boundary enforcement
        mcp_flexion = applyAnatomicalLimits(mcp_flexion, INDEX_MCP_MIN_FLEX, INDEX_MCP_MAX_FLEX);
        mcp_splay   = applyAnatomicalLimits(mcp_splay, INDEX_MCP_MIN_SPLAY, INDEX_MCP_MAX_SPLAY);
        pip_flexion = applyAnatomicalLimits(pip_flexion, INDEX_PIP_MIN_FLEX, INDEX_PIP_MAX_FLEX);
        dip_flexion = applyAnatomicalLimits(dip_flexion, INDEX_DIP_MIN_FLEX, INDEX_DIP_MAX_FLEX);
    }

    // High fidelity OpenGL matrix projection renderer
    void render() {
        glPushMatrix();

        // --------------------------------------------------------------------
        // SEGMENT 1: Index Metacarpophalangeal Node (Base Knuckle)
        // --------------------------------------------------------------------
        glRotatef(mcp_splay, 0.0f, 1.0f, 0.0f);     // Lateral abduction/adduction
        glRotatef(mcp_flexion, 1.0f, 0.0f, 0.0f);   // Main core bending
        
        // Render Index MCP Joint Base Spherical Core
        glColor3f(0.84f, 0.59f, 0.49f); 
        glutSolidSphere(bone_thickness * 1.15f, 16, 16);

        // Render Proximal Phalanx
        glColor3f(0.88f, 0.66f, 0.56f);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        GLUquadric* indexQuad = gluNewQuadric();
        gluCylinder(indexQuad, bone_thickness, bone_thickness * 0.92f, mcp_length, 16, 16);
        glPopMatrix();

        // Step coordinate engine up to PIP threshold
        glTranslatef(0.0f, mcp_length, 0.0f);

        // --------------------------------------------------------------------
        // SEGMENT 2: Index Proximal Interphalangeal Node (Tactile Hinge)
        // --------------------------------------------------------------------
        glRotatef(pip_flexion, 1.0f, 0.0f, 0.0f);
        
        // Render PIP Joint Spherical Core
        glColor3f(0.81f, 0.56f, 0.46f);
        glutSolidSphere(bone_thickness * 0.98f, 16, 16);

        // Render Intermediate Phalanx
        glColor3f(0.88f, 0.66f, 0.56f);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(indexQuad, bone_thickness * 0.92f, bone_thickness * 0.82f, pip_length, 16, 16);
        glPopMatrix();

        // Shift matrix pipe head to DIP node
        glTranslatef(0.0f, pip_length, 0.0f);

        // --------------------------------------------------------------------
        // SEGMENT 3: Index Distal Interphalangeal Node (Precision Tip)
        // --------------------------------------------------------------------
        glRotatef(dip_flexion, 1.0f, 0.0f, 0.0f);
        
        // Render DIP Joint Spherical Core
        glColor3f(0.78f, 0.53f, 0.43f);
        glutSolidSphere(bone_thickness * 0.88f, 16, 16);

        // Render Distal Phalanx & Precision Tip Contour
        glColor3f(0.86f, 0.63f, 0.53f);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(indexQuad, bone_thickness * 0.82f, bone_thickness * 0.55f, dip_length * 0.72f, 16, 16);
        
        // Terminal Hemisphere Cap (Precision Point)
        glTranslatef(0.0f, 0.0f, dip_length * 0.72f);
        glutSolidSphere(bone_thickness * 0.55f, 16, 16);
        glPopMatrix();

        gluDeleteQuadric(indexQuad);
        glPopMatrix(); // Flush transformations back to parent hub anchor
    }
};

// ============================================================================
// ENGINE WORKSPACE ENVIRONMENT
// ============================================================================
IndexFingerSimulation indexSimulator;
float camYaw = -20.0f;
float camPitch = 20.0f;
int localTickCounter = 0;
int gesturePhase = 0; // 0: Neutral Relaxed, 1: Pointing Stance, 2: Deep Flexion Curl, 3: Rapid Precision Pinch Loop

void drawSceneViewport() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Map viewport system space location
    glTranslatef(0.0f, -1.3f, -8.0f);
    glRotatef(camPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(camYaw, 0.0f, 1.0f, 0.0f);

    // --- CARPAL BASE CONNECTOR ROOT ---
    glColor3f(0.32f, 0.34f, 0.38f);
    glPushMatrix();
    glScalef(1.9f, 0.48f, 1.4f);
    glutWireCube(1.0f);
    glPopMatrix();

    // --- MATRIX LAYOUT ROUTING FOR THE INDEX LINE ---
    glPushMatrix();
    // Positioned on the internal right sector, directly balancing the central Middle finger track
    glTranslatef(0.55f, 0.23f, 0.0f);
    
    // Execute matrix solver and dump pixels to raster buffer
    indexSimulator.render();
    glPopMatrix();

    // High fidelity telemetry console feed
    std::cout << "\r[Telemetry | Index Finger] MCP_FX: " << std::fixed << std::setw(6) << std::setprecision(2) << indexSimulator.mcp_flexion 
              << " | SPLAY: " << std::setw(6) << indexSimulator.mcp_splay
              << " | PIP_FX: " << std::setw(6) << indexSimulator.pip_flexion
              << " | DIP_FX: " << std::setw(6) << indexSimulator.dip_flexion << std::flush;

    glutSwapBuffers();
}

void mainUpdateTick(int timerVal) {
    localTickCounter++;

    // Staged Gesture Automation Setup
    if (localTickCounter % 250 == 0) {
        gesturePhase = (gesturePhase + 1) % 4;
    }

    switch(gesturePhase) {
        case 0: // Natural Relaxed Resting Posture
            indexSimulator.target_mcp_flex  = 8.0f;
            indexSimulator.target_mcp_splay = 0.0f;
            indexSimulator.target_pip_flex  = 12.0f;
            indexSimulator.target_dip_flex  = 8.0f;
            break;
        case 1: // Classical Pointing Gesture (Hyperextended lock while others curl)
            indexSimulator.target_mcp_flex  = -8.0f; // Hyperextended flat lock
            indexSimulator.target_mcp_splay = 12.0f; // Splayed wide outward
            indexSimulator.target_pip_flex  = 0.0f;  // Perfectly straight
            indexSimulator.target_dip_flex  = 0.0f;
            break;
        case 2: // Full Fist Dynamic Grip Closure
            indexSimulator.target_mcp_flex  = 85.0f;
            indexSimulator.target_mcp_splay = -4.0f; // Squeezed tight inward against Middle
            indexSimulator.target_pip_flex  = 98.0f;
            indexSimulator.target_dip_flex  = 75.0f;
            break;
        case 3: // Rapid Precision Trigger/Pinch Sinusoidal Tracking
            float pinchSine = sin(localTickCounter * 0.045f); // Faster oscillation velocity
            indexSimulator.target_mcp_flex  = 40.0f + (pinchSine * 20.0f);
            indexSimulator.target_mcp_splay = 2.0f;
            indexSimulator.target_pip_flex  = 70.0f + (pinchSine * 25.0f);
            indexSimulator.target_dip_flex  = 50.0f + (pinchSine * 20.0f);
            break;
    }

    // Step internal algorithmic equations
    indexSimulator.updateKinematics();

    glutPostRedisplay();
    glutTimerFunc(16, mainUpdateTick, 0);
}

void viewportReshapeCallback(int width, int height) {
    if (height == 0) height = 1;
    float aspect = (float)width / (float)height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, aspect, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void initializePipelineSpecs() {
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    GLfloat ambientSpec[]  = { 0.22f, 0.22f, 0.25f, 1.0f };
    GLfloat diffuseSpec[]  = { 0.85f, 0.85f, 0.88f, 1.0f };
    GLfloat specularSpec[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat positionSpec[] = { 6.0f, 8.0f, 4.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientSpec);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseSpec);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularSpec);
    glLightfv(GL_LIGHT0, GL_POSITION, positionSpec);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    GLfloat materialReflect[] = { 0.22f, 0.22f, 0.22f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialReflect);
    glMateriali(GL_FRONT, GL_SHININESS, 60); // High precision specularity definition

    glClearColor(0.05f, 0.06f, 0.07f, 1.0f);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Advanced Isolated Index Finger Precision Kinematics Engine");

    initializePipelineSpecs();

    glutDisplayFunc(drawSceneViewport);
    glutReshapeFunc(viewportReshapeCallback);
    glutTimerFunc(16, mainUpdateTick, 0);

    std::cout << "=========================================================" << std::endl;
    std::cout << "  INDEX PRECISION DRIVER SUBSYSTEM COMPILATION COMPLETE  " << std::endl;
    std::cout << "=========================================================" << std::endl;

    glutMainLoop();
    return 0;
}
