#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <iomanip>

// ============================================================================
// SYSTEM CONFIGURATION & ANATOMICAL CONSTANTS FOR RING FINGER
// ============================================================================
const float PI_VAL = 3.14159265f;

// Biological Constraints specific to the Ring Finger
const float RING_MCP_MIN_FLEX = -8.0f;     // Limited hyperextension
const float RING_MCP_MAX_FLEX = 90.0f;     // Full grip closure
const float RING_MCP_MIN_SPLAY = -5.0f;    // Splay towards Middle finger (Adduction)
const float RING_MCP_MAX_SPLAY = 10.0f;    // Splay towards Pinky finger (Abduction)

const float RING_PIP_MIN_FLEX = 0.0f;      // Extended straight position
const float RING_PIP_MAX_FLEX = 105.0f;    // Deep knuckle bend

const float RING_DIP_MIN_FLEX = 0.0f;      // Extended tip
const float RING_DIP_MAX_FLEX = 85.0f;     // Final claw closure angle

// ============================================================================
// INDEPENDENT RING FINGER CONTROLLER (Kinematic Matrix Node)
// ============================================================================
class RingFingerSimulation {
public:
    // Real-time calculated Dynamic Joint Angles
    float mcp_flexion;   // Base flexion (X-Axis)
    float mcp_splay;     // Lateral adjustment (Y-Axis)
    float pip_flexion;   // Middle joint (X-Axis)
    float dip_flexion;   // Tip joint closure (X-Axis)

    // Independent Target Anchors for Multi-Axis Transitions
    float target_mcp_flex;
    float target_mcp_splay;
    float target_pip_flex;
    float target_dip_flex;

    // Physics Tuning (Damping variables for heavy organic weight)
    float muscle_damping; 
    
    // Bone Proportions (Scaled up from Pinky, shorter than Middle)
    float mcp_length;
    float pip_length;
    float dip_length;
    float bone_thickness;

    RingFingerSimulation() {
        // Safe defaults - Neutral open resting hand stance
        mcp_flexion = 0.0f;
        mcp_splay = 0.0f;
        pip_flexion = 0.0f;
        dip_flexion = 0.0f;

        target_mcp_flex = 0.0f;
        target_mcp_splay = 0.0f;
        target_pip_flex = 0.0f;
        target_dip_flex = 0.0f;

        // Damping parameter: tuned differently than pinky to simulate muscle resistance
        muscle_damping = 0.075f; 

        // Anatomical scaling matrices relative to human anatomy
        mcp_length = 1.35f;      // Longer structural lever than Pinky
        pip_length = 0.88f;      // Mid phalanx segment
        dip_length = 0.62f;      // Fingertip phalanx
        bone_thickness = 0.21f;  // Thicker structural diameter than Pinky
    }

    float enforceLimits(float val, float min_l, float max_l) {
        if (val < min_l) return min_l;
        if (val > max_l) return max_l;
        return val;
    }

    // Mathematical Muscle Simulator Loop (Asymptotic Interpolation Engine)
    void updateKinematics() {
        mcp_flexion += (target_mcp_flex - mcp_flexion) * muscle_damping;
        mcp_splay   += (target_mcp_splay - mcp_splay) * muscle_damping;
        pip_flexion += (target_pip_flex - pip_flexion) * muscle_damping;
        dip_flexion += (target_dip_flex - dip_flexion) * muscle_damping;

        // Enforce strict biomechanical limits
        mcp_flexion = enforceLimits(mcp_flexion, RING_MCP_MIN_FLEX, RING_MCP_MAX_FLEX);
        mcp_splay   = enforceLimits(mcp_splay, RING_MCP_MIN_SPLAY, RING_MCP_MAX_SPLAY);
        pip_flexion = enforceLimits(pip_flexion, RING_PIP_MIN_FLEX, RING_PIP_MAX_FLEX);
        dip_flexion = enforceLimits(dip_flexion, RING_DIP_MIN_FLEX, RING_DIP_MAX_FLEX);
    }

    // High fidelity OpenGL matrix projection renderer
    void render() {
        glPushMatrix();

        // --------------------------------------------------------------------
        // SEGMENT 1: Ring Metacarpophalangeal Node (Base Knuckle)
        // --------------------------------------------------------------------
        glRotatef(mcp_splay, 0.0f, 1.0f, 0.0f);     // Side-to-side alignment
        glRotatef(mcp_flexion, 1.0f, 0.0f, 0.0f);   // Main grip contraction
        
        // Render MCP Joint Base Spherical Core
        glColor3f(0.82f, 0.57f, 0.47f); 
        glutSolidSphere(bone_thickness * 1.15f, 16, 16);

        // Render Proximal Phalanx (Main lower bone bone)
        glColor3f(0.86f, 0.64f, 0.54f);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        GLUquadric* ringQuad = gluNewQuadric();
        gluCylinder(ringQuad, bone_thickness, bone_thickness * 0.92f, mcp_length, 16, 16);
        glPopMatrix();

        // Step up local matrix cache to PIP threshold
        glTranslatef(0.0f, mcp_length, 0.0f);

        // --------------------------------------------------------------------
        // SEGMENT 2: Ring Proximal Interphalangeal Node (Middle Joint)
        // --------------------------------------------------------------------
        glRotatef(pip_flexion, 1.0f, 0.0f, 0.0f);
        
        // Render PIP Joint Spherical Core
        glColor3f(0.79f, 0.54f, 0.44f);
        glutSolidSphere(bone_thickness * 0.98f, 16, 16);

        // Render Intermediate Phalanx (Middle bone)
        glColor3f(0.86f, 0.64f, 0.54f);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(ringQuad, bone_thickness * 0.92f, bone_thickness * 0.82f, pip_length, 16, 16);
        glPopMatrix();

        // Shift rendering head to DIP node
        glTranslatef(0.0f, pip_length, 0.0f);

        // --------------------------------------------------------------------
        // SEGMENT 3: Ring Distal Interphalangeal Node (Fingertip)
        // --------------------------------------------------------------------
        glRotatef(dip_flexion, 1.0f, 0.0f, 0.0f);
        
        // Render DIP Joint Spherical Core
        glColor3f(0.76f, 0.51f, 0.41f);
        glutSolidSphere(bone_thickness * 0.88f, 16, 16);

        // Render Distal Phalanx & Organic Tip Contour 
        glColor3f(0.84f, 0.61f, 0.51f);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(ringQuad, bone_thickness * 0.82f, bone_thickness * 0.55f, dip_length * 0.72f, 16, 16);
        
        // Terminal Hemisphere Cap
        glTranslatef(0.0f, 0.0f, dip_length * 0.72f);
        glutSolidSphere(bone_thickness * 0.55f, 16, 16);
        glPopMatrix();

        gluDeleteQuadric(ringQuad);
        glPopMatrix(); // Wipe transform state clean for parent stack reset
    }
};

// ============================================================================
// ENGINE WORKSPACE ENVIRONMENT
// ============================================================================
RingFingerSimulation ringSimulator;
float cameraYawAxis = 30.0f;
float cameraPitchAxis = 20.0f;
int simulationFrameCount = 0;
int behaviorPhase = 0; // 0: Relaxed, 1: Dynamic Extension, 2: Heavy Fist Pull, 3: Cybernetic Grasp

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Establish viewport coordinate positioning
    glTranslatef(0.0f, -1.2f, -8.0f);
    glRotatef(cameraPitchAxis, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraYawAxis, 0.0f, 1.0f, 0.0f);

    // --- ANCHOR HAND METACARPAL BASE ---
    glColor3f(0.35f, 0.38f, 0.42f);
    glPushMatrix();
    glScalef(1.8f, 0.45f, 1.4f);
    glutWireCube(1.0f);
    glPopMatrix();

    // --- ISOLATED MATRIX ROUTING FOR THE RING FINGER NODE ---
    glPushMatrix();
    // Positioned on the internal left sector, directly adjacent to where the middle finger will sit
    glTranslatef(-0.25f, 0.22f, 0.0f);
    
    // Process calculation arrays and rasterize pixels
    ringSimulator.render();
    glPopMatrix();

    // Data Streaming Metrics
    std::cout << "\r[Telemetry | Ring Finger] MCP_FX: " << std::fixed << std::setw(6) << std::setprecision(2) << ringSimulator.mcp_flexion 
              << " | SPLAY: " << std::setw(6) << ringSimulator.mcp_splay
              << " | PIP_FX: " << std::setw(6) << ringSimulator.pip_flexion
              << " | DIP_FX: " << std::setw(6) << ringSimulator.dip_flexion << std::flush;

    glutSwapBuffers();
}

void executionTimer(int systemValue) {
    simulationFrameCount++;

    // Staged Behavior State Machine
    if (simulationFrameCount % 250 == 0) {
        behaviorPhase = (behaviorPhase + 1) % 4;
    }

    switch(behaviorPhase) {
        case 0: // Natural Resting State (Slight organic curl)
            ringSimulator.target_mcp_flex  = 12.0f;
            ringSimulator.target_mcp_splay = 0.0f;
            ringSimulator.target_pip_flex  = 18.0f;
            ringSimulator.target_dip_flex  = 12.0f;
            break;
        case 1: // Full Extension Stretches (Splaying outward towards Pinky)
            ringSimulator.target_mcp_flex  = -2.0f;
            ringSimulator.target_mcp_splay = 7.0f;  // Splayed left towards the pinky boundary
            ringSimulator.target_pip_flex  = 0.0f;
            ringSimulator.target_dip_flex  = 0.0f;
            break;
        case 2: // Hyper Angular Fist Crushing Movement
            ringSimulator.target_mcp_flex  = 85.0f;
            ringSimulator.target_mcp_splay = -2.0f; // Squeezed tight inward
            ringSimulator.target_pip_flex  = 100.0f;
            ringSimulator.target_dip_flex  = 80.0f;
            break;
        case 3: // Grasp Interlocking Sine Simulation
            float complexWave = sin(simulationFrameCount * 0.025f);
            ringSimulator.target_mcp_flex  = 50.0f + (complexWave * 12.0f);
            ringSimulator.target_mcp_splay = 2.0f;
            ringSimulator.target_pip_flex  = 80.0f + (complexWave * 15.0f);
            ringSimulator.target_dip_flex  = 60.0f + (complexWave * 10.0f);
            break;
    }

    // Step the physics calculus matrices
    ringSimulator.updateKinematics();

    glutPostRedisplay();
    glutTimerFunc(16, executionTimer, 0);
}

void resizeEngineWindow(int w, int h) {
    if (h == 0) h = 1;
    float ratio = (float)w / (float)h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, ratio, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void initializeRenderPipeline() {
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    GLfloat ambientLight[]  = { 0.22f, 0.22f, 0.22f, 1.0f };
    GLfloat diffuseLight[]  = { 0.82f, 0.82f, 0.82f, 1.0f };
    GLfloat specularLight[] = { 0.95f, 0.95f, 0.95f, 1.0f };
    GLfloat lightPosition[] = { -4.0f, 7.0f, 5.0f, 1.0f }; // Opposite casting angle for dynamic contrast

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    GLfloat materialSpecs[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecs);
    glMateriali(GL_FRONT, GL_SHININESS, 45);

    glClearColor(0.07f, 0.08f, 0.09f, 1.0f);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Advanced Isolated Ring Finger Kinematics Engine");

    initializeRenderPipeline();

    glutDisplayFunc(renderScene);
    glutReshapeFunc(resizeEngineWindow);
    glutTimerFunc(16, executionTimer, 0);

    std::cout << "=========================================================" << std::endl;
    std::cout << "  RING FINGER SYSTEM ARCHITECTURE MATRIX DEPLOYED        " << std::endl;
    std::cout << "=========================================================" << std::endl;

    glutMainLoop();
    return 0;
}
