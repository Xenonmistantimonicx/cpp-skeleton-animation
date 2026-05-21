#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <iomanip>

// ============================================================================
// SYSTEM CONFIGURATION & ANATOMICAL CONSTANTS FOR THUMB (POLLEX)
// ============================================================================
const float THUMB_PI = 3.14159265f;

// Biomechanical Constraints specific to the human Thumb (Opposition Space)
const float THUMB_CMC_MIN_ROT  = -10.0f;   // Base opening/Extension
const float THUMB_CMC_MAX_ROT  = 45.0f;    // Base sweeping inward across palm (Opposition)
const float THUMB_MCP_MIN_FLEX = -5.0f;    // Minimal knuckle hyperextension
const float THUMB_MCP_MAX_FLEX = 60.0f;    // Knuckle drop threshold
const float THUMB_DIP_MIN_FLEX = -10.0f;   // Tip flex extension
const float THUMB_DIP_MAX_FLEX = 80.0f;    // Final locking terminal angle (Interphalangeal)

// ============================================================================
// INDEPENDENT THUMB CONTROLLER (Multi-Axis Saddle & Opposition Node)
// ============================================================================
class ThumbSimulation {
public:
    // Real-time calculated Complex Joint Angles
    float cmc_opposition; // Base sweeping rotation (Y/Z Diagonal Axis twist)
    float mcp_flexion;    // Metacarpophalangeal intermediate bend (X-Axis)
    float dip_flexion;    // Distal Interphalangeal terminal lock (X-Axis)

    // Independent Target Anchors for Multi-Axis Non-Linear Transitions
    float target_cmc_opp;
    float target_mcp_flex;
    float target_dip_flex;

    // Muscle Dynamics (High torque, powerful low-frequency damping filter)
    float muscle_torque_damping; 
    
    // Bone Proportions (Shorter but significantly thicker structural diameter)
    float proximal_length; // First visible bone segment
    float distal_length;   // Fingertip phalanx segment
    float bone_thickness;

    ThumbSimulation() {
        // Safe defaults - Neutral open resting hand stance
        cmc_opposition = 0.0f;
        mcp_flexion = 0.0f;
        dip_flexion = 0.0f;

        target_cmc_opp = 0.0f;
        target_mcp_flex = 0.0f;
        target_dip_flex = 0.0f;

        // Heavy, stable damping factor (0.065f) to mimic the massive thenar muscle group mass
        muscle_torque_damping = 0.065f; 

        // Anatomical scaling matrices: Thickest structural component
        proximal_length = 1.10f; // Shorter lever but robust
        distal_length = 0.85f;   // Broad terminal gripping tip
        bone_thickness = 0.30f;  // Maximum thickness among all digits for pinch resistance
    }

    float enforceAnatomicalLimits(float val, float min_limit, float max_limit) {
        if (val < min_limit) return min_limit;
        if (val > max_limit) return max_limit;
        return val;
    }

    // Mathematical Muscle Simulator Loop (Asymptotic Interpolation Engine)
    void updateKinematics() {
        cmc_opposition += (target_cmc_opp - cmc_opposition) * muscle_torque_damping;
        mcp_flexion    += (target_mcp_flex - mcp_flexion) * muscle_torque_damping;
        dip_flexion    += (target_dip_flex - dip_flexion) * muscle_torque_damping;

        // Force strict biological boundary enforcement
        cmc_opposition = enforceAnatomicalLimits(cmc_opposition, THUMB_CMC_MIN_ROT, THUMB_CMC_MAX_ROT);
        mcp_flexion    = enforceAnatomicalLimits(mcp_flexion, THUMB_MCP_MIN_FLEX, THUMB_MCP_MAX_FLEX);
        dip_flexion    = enforceAnatomicalLimits(dip_flexion, THUMB_DIP_MIN_FLEX, THUMB_DIP_MAX_FLEX);
    }

    // High fidelity OpenGL matrix projection renderer
    void render() {
        glPushMatrix();

        // --------------------------------------------------------------------
        // INITIAL BIOMECHANICAL SETUP: The 90-Degree Trapezium Angular Offset
        // --------------------------------------------------------------------
        // Human thumb sits on a naturally rotated plane relative to the palm plane
        glRotatef(-40.0f, 0.0f, 1.0f, 0.0f); // Rotate outward into spatial abduction plane
        glRotatef(25.0f,  0.0f, 0.0f, 1.0f); // Tilt downward relative to index base

        // --------------------------------------------------------------------
        // SEGMENT 1: Carpometacarpal Node (Saddle Joint - Sweeps across palm)
        // --------------------------------------------------------------------
        glRotatef(cmc_opposition, 0.0f, 0.0f, 1.0f); // Sweep rotation matrix
        
        // Render CMC Joint Base Spherical Core
        glColor3f(0.85f, 0.60f, 0.50f); 
        glutSolidSphere(bone_thickness * 1.20f, 16, 16);

        // Render Proximal Phalanx
        glColor3f(0.89f, 0.67f, 0.57f);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        GLUquadric* thumbQuad = gluNewQuadric();
        gluCylinder(thumbQuad, bone_thickness, bone_thickness * 0.88f, proximal_length, 16, 16);
        glPopMatrix();

        // Step coordinate engine forward to MCP hinge threshold
        glTranslatef(0.0f, proximal_length, 0.0f);

        // --------------------------------------------------------------------
        // SEGMENT 2: Metacarpophalangeal Node (Knuckle Hinge)
        // --------------------------------------------------------------------
        glRotatef(mcp_flexion, 1.0f, 0.0f, 0.0f); // Hinge flexion action
        
        // Render MCP Joint Spherical Core
        glColor3f(0.81f, 0.56f, 0.46f);
        glutSolidSphere(bone_thickness * 1.02f, 16, 16);

        // Render Distal Phalanx (Final bone segment)
        glColor3f(0.89f, 0.67f, 0.57f);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(thumbQuad, bone_thickness * 0.88f, bone_thickness * 0.65f, distal_length * 0.70f, 16, 16);
        glPopMatrix();

        // Shift matrix head to Interphalangeal (DIP equivalent) terminal boundary
        glTranslatef(0.0f, distal_length * 0.70f, 0.0f);

        // --------------------------------------------------------------------
        // SEGMENT 3: Interphalangeal Node (Tip Lock / Opposition Pinch)
        // --------------------------------------------------------------------
        glRotatef(dip_flexion, 1.0f, 0.0f, 0.0f);
        
        // Render Terminal Hemisphere Cap (Broad structural grasping pad)
        glColor3f(0.86f, 0.63f, 0.53f);
        glPushMatrix();
        glutSolidSphere(bone_thickness * 0.65f, 16, 16);
        glPopMatrix();

        gluDeleteQuadric(thumbQuad);
        glPopMatrix(); // Wipe transform state completely for structural reset
    }
};

// ============================================================================
// ENGINE WORKSPACE ENVIRONMENT
// ============================================================================
ThumbSimulation thumbSimulator;
float cameraYaw = -35.0f;
float cameraPitch = 25.0f;
int loopTickCounter = 0;
int behavioralState = 0; // 0: Relaxed, 1: Full Abduction (Wide Open), 2: Deep Opposition Pinch Lock

void drawScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Setup global workspace center configuration
    glTranslatef(0.0f, -0.8f, -7.5f);
    glRotatef(cameraPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraYaw, 0.0f, 1.0f, 0.0f);

    // --- CARPAL BASE RIGID ANCHOR ---
    glColor3f(0.28f, 0.30f, 0.33f);
    glPushMatrix();
    glScalef(1.8f, 0.50f, 1.5f);
    glutWireCube(1.0f);
    glPopMatrix();

    // --- MATRIX DEPLOYMENT POINT FOR THE THUMB ROOT ---
    glPushMatrix();
    // Positioned offset lower on the right side lateral edge of the palm root architecture
    glTranslatef(0.75f, -0.1f, 0.2f);
    
    // Execute matrix solver and rasterize pixels
    thumbSimulator.render();
    glPopMatrix();

    // High fidelity telemetry console feed
    std::cout << "\r[Telemetry | Thumb Pollex] CMC_OPP: " << std::fixed << std::setw(6) << std::setprecision(2) << thumbSimulator.cmc_opposition 
              << " | MCP_FX: " << std::setw(6) << thumbSimulator.mcp_flexion
              << " | IP_FX: " << std::setw(6) << thumbSimulator.dip_flexion << std::flush;

    glutSwapBuffers();
}

void simulationStep(int val) {
    loopTickCounter++;

    // Staged Gesture State Machine Loops
    if (loopTickCounter % 250 == 0) {
        behavioralState = (behavioralState + 1) % 3;
    }

    switch(behavioralState) {
        case 0: // Natural Resting Stance
            thumbSimulator.target_cmc_opp  = 5.0f;
            thumbSimulator.target_mcp_flex = 10.0f;
            thumbSimulator.target_dip_flex = 8.0f;
            break;
        case 1: // Max Extension / Abduction (Hand completely stretched open)
            thumbSimulator.target_cmc_opp  = -8.0f;  // Pulled wide away from palm
            thumbSimulator.target_mcp_flex = -2.0f;  // Flat extension
            thumbSimulator.target_dip_flex = -5.0f;  // Slight tip stretch
            break;
        case 2: // Complete Opposition Pinch Closure (Simulating grasping a tool/cylindrical object)
            // The CMC sweeps hard across the Z-axis, forcing the thumb to wrap front-facing
            thumbSimulator.target_cmc_opp  = 40.0f;  // Max sweeping angle lock
            thumbSimulator.target_mcp_flex = 45.0f;  // Solid base grip clamping force
            thumbSimulator.target_dip_flex = 55.0f;  // Final tip closure rotation
            break;
    }

    // Step internal algorithmic physics equations
    thumbSimulator.updateKinematics();

    glutPostRedisplay();
    glutTimerFunc(16, simulationStep, 0);
}

void resizeViewport(int w, int h) {
    if (h == 0) h = 1;
    float aspect = (float)w / (float)h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, aspect, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void configureLightingSpecs() {
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    GLfloat ambientSpec[]  = { 0.24f, 0.24f, 0.26f, 1.0f };
    GLfloat diffuseSpec[]  = { 0.86f, 0.86f, 0.84f, 1.0f };
    GLfloat specularSpec[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat positionSpec[] = { 5.0f, 5.0f, 6.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientSpec);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseSpec);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularSpec);
    glLightfv(GL_LIGHT0, GL_POSITION, positionSpec);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    GLfloat materialReflect[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialReflect);
    glMateriali(GL_FRONT, GL_SHININESS, 40);

    glClearColor(0.04f, 0.05f, 0.06f, 1.0f);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Advanced Isolated Thumb Opposition Matrix Engine");

    configureLightingSpecs();

    glutDisplayFunc(drawScene);
    glutReshapeFunc(resizeViewport);
    glutTimerFunc(16, simulationStep, 0);

    std::cout << "=========================================================" << std::endl;
    std::cout << "  THUMB SADDLE INTERPOLATION PIPELINE COMPILATION DONE   " << std::endl;
    std::cout << "=========================================================" << std::endl;

    glutMainLoop();
    return 0;
}
