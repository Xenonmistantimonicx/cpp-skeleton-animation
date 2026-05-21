#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <vector>

// ============================================================================
// GLOBAL SYSTEM MATHEMATICS & ANATOMICAL CONFIGURATIONS
// ============================================================================
const float ENGINE_PI = 3.14159265f;
enum FingerType { THUMB = 0, INDEX = 1, MIDDLE = 2, RING = 3, PINKY = 4, TOTAL_DIGITS = 5 };

struct JointLimits {
    float mcp_flex_min, mcp_flex_max;
    float mcp_splay_min, mcp_splay_max;
    float pip_flex_min, pip_flex_max;
    float dip_flex_min, dip_flex_max;
};

// ============================================================================
// INTEGRATED FINGER SEGMENT NODE
// ============================================================================
class FingerNode {
public:
    FingerType type;
    float mcp_flex, mcp_splay, pip_flex, dip_flex;
    float t_mcp_flex, t_mcp_splay, t_pip_flex, t_dip_flex;
    JointLimits limits;
    float mcp_len, pip_len, dip_len, thickness, responsiveness;

    FingerNode(FingerType f_type, JointLimits f_limits, float m_len, float p_len, float d_len, float thick, float resp)
        : type(f_type), limits(f_limits), mcp_len(m_len), pip_len(p_len), dip_len(d_len), thickness(thick), responsiveness(resp) {
        mcp_flex = mcp_splay = pip_flex = dip_flex = 0.0f;
        t_mcp_flex = t_mcp_splay = t_pip_flex = t_dip_flex = 0.0f;
    }

    float clamp(float val, float min_l, float max_l) {
        return (val < min_l) ? min_l : ((val > max_l) ? max_l : val);
    }

    void stepKinematics() {
        mcp_flex  += (t_mcp_flex - mcp_flex) * responsiveness;
        mcp_splay += (t_mcp_splay - mcp_splay) * responsiveness;
        pip_flex  += (t_pip_flex - pip_flex) * responsiveness;
        dip_flex  += (t_dip_flex - dip_flex) * responsiveness;

        mcp_flex  = clamp(mcp_flex, limits.mcp_flex_min, limits.mcp_flex_max);
        mcp_splay = clamp(mcp_splay, limits.mcp_splay_min, limits.mcp_splay_max);
        pip_flex  = clamp(pip_flex, limits.pip_flex_min, limits.pip_flex_max);
        dip_flex  = clamp(dip_flex, limits.dip_flex_min, limits.dip_flex_max);
    }
};

// ============================================================================
// WRIST JOINT CONTROLLER (The Biomechanical Base Matrix Node)
// ============================================================================
class WristJointController {
public:
    // Real-time Radiocarpal Angles
    float pitch_flexion;   // Up/Down bending (X-Axis)
    float yaw_deviation;   // Radial (Thumb) / Ulnar (Pinky) movement (Y-Axis)
    
    // Target Anchors
    float target_pitch;
    float target_yaw;

    // Wrist Structural Constants
    const float WRIST_PITCH_MIN = -70.0f;  // Max Extension (Peeche bending)
    const float WRIST_PITCH_MAX = 80.0f;   // Max Flexion (Aage folding)
    const float WRIST_YAW_MIN   = -15.0f;  // Radial Deviation (Thumb side tilt)
    const float WRIST_YAW_MAX   = 35.0f;   // Ulnar Deviation (Pinky side tilt)
    
    float inertia_damping; // Heavy inertia for forearm to hand link

    WristJointController() {
        pitch_flexion = yaw_deviation = 0.0f;
        target_pitch = target_yaw = 0.0f;
        inertia_damping = 0.050f; // Stable mass simulation
    }

    void updateWristKinematics() {
        pitch_flexion += (target_pitch - pitch_flexion) * inertia_damping;
        yaw_deviation += (target_yaw - yaw_deviation) * inertia_damping;

        // Boundary Locking
        if (pitch_flexion < WRIST_PITCH_MIN) pitch_flexion = WRIST_PITCH_MIN;
        if (pitch_flexion > WRIST_PITCH_MAX) pitch_flexion = WRIST_PITCH_MAX;
        if (yaw_deviation < WRIST_YAW_MIN)   yaw_deviation = WRIST_YAW_MIN;
        if (yaw_deviation > WRIST_YAW_MAX)   yaw_deviation = WRIST_YAW_MAX;
    }
};

// ============================================================================
// SYSTEM MASTER CORE ENGINE (Wrist-Digit Convergence Architecture)
// ============================================================================
class HumanHandKinematicsEngine {
public:
    std::vector<FingerNode*> digits;
    WristJointController wrist;
    float palm_w, palm_h;

    HumanHandKinematicsEngine() {
        palm_w = 2.2f; palm_h = 2.4f;
        initializeSkeletalFramework();
    }

    ~HumanHandKinematicsEngine() {
        for (auto d : digits) delete d;
    }

    void initializeSkeletalFramework() {
        digits.push_back(new FingerNode(THUMB,  {-5.0f, 60.0f, -10.0f, 45.0f, 0.0f, 60.0f, -10.0f, 80.0f}, 1.10f, 0.0f,  0.85f, 0.30f, 0.065f));
        digits.push_back(new FingerNode(INDEX,  {-10.0f, 90.0f, -7.0f, 15.0f,  0.0f, 100.0f, 0.0f, 80.0f}, 1.38f, 0.90f, 0.64f, 0.22f, 0.095f));
        digits.push_back(new FingerNode(MIDDLE, {-5.0f, 90.0f,  -8.0f, 8.0f,   0.0f, 110.0f, 0.0f, 90.0f}, 1.50f, 0.98f, 0.70f, 0.24f, 0.080f));
        digits.push_back(new FingerNode(RING,   {-5.0f, 90.0f,  -12.0f, 8.0f,  0.0f, 100.0f, 0.0f, 85.0f}, 1.36f, 0.92f, 0.66f, 0.21f, 0.075f));
        digits.push_back(new FingerNode(PINKY,  {-10.0f, 90.0f, -20.0f, 5.0f,  0.0f, 95.0f,  0.0f, 85.0f}, 1.12f, 0.72f, 0.58f, 0.18f, 0.090f));
    }

    void setTargetFingerPose(FingerType f, float mf, float ms, float pf, float df) {
        if(f >= 0 && f < TOTAL_DIGITS) {
            digits[f]->t_mcp_flex = mf; digits[f]->t_mcp_splay = ms;
            digits[f]->t_pip_flex = pf; digits[f]->t_dip_flex = df;
        }
    }

    // ========================================================================
    // GLOBAL COUPLING NETWORK ARCHITECTURE (The Cross-Talk Engine)
    // ========================================================================
    void resolveGlobalLinkageEquations() {
        // Step 1: Update Base wrist transformation matrix states
        wrist.updateWristKinematics();

        // Step 2: THE TENODESIS EFFECT MATRIX PIPELINE
        // Extension (Negative Pitch) bends fingers. Flexion (Positive Pitch) opens them.
        float tenodesis_factor = 0.0f;
        if (wrist.pitch_flexion < 0.0f) { // Wrist Extension
            // Normalize pitch state to drive automated passive closure vectors
            tenodesis_factor = (wrist.pitch_flexion / wrist.WRIST_PITCH_MIN) * 45.0f; 
            for (int i = INDEX; i <= PINKY; ++i) {
                digits[i]->t_mcp_flex += (tenodesis_factor - digits[i]->t_mcp_flex) * 0.1f;
                digits[i]->t_pip_flex += ((tenodesis_factor * 1.1f) - digits[i]->t_pip_flex) * 0.1f;
            }
        }

        // Step 3: INTER-DIGITAL MULTI-AXIS CROSS TALK (Connexus Intertendineus)
        float mid_ratio = digits[MIDDLE]->mcp_flex / digits[MIDDLE]->limits.mcp_flex_max;
        if (mid_ratio > 0.4f) {
            float drag_ring = (mid_ratio - 0.4f) * 35.0f;
            if (digits[RING]->t_mcp_flex < drag_ring) digits[RING]->t_mcp_flex = drag_ring;
        }

        // Step 4: DIGITAL COUPLING (PIP to DIP tendon lock)
        for (int i = INDEX; i <= PINKY; ++i) {
            if (digits[i]->t_dip_flex == 0.0f && digits[i]->pip_flex > 10.0f) {
                digits[i]->t_dip_flex = digits[i]->pip_flex * 0.65f;
            }
        }

        // Execute processing pipeline steps for all sub-nodes
        for (auto d : digits) d->stepKinematics();
    }

    // ========================================================================
    // MASTER GRAPHICS RENDER LOOP
    // ========================================================================
    void renderSystem() {
        glPushMatrix();

        // --------------------------------------------------------------------
        // FOREARM ANCHOR (Static Reference Segment)
        // --------------------------------------------------------------------
        glColor3f(0.2f, 0.22f, 0.25f);
        glPushMatrix();
        glTranslatef(0.0f, -2.0f, -0.2f);
        glScalef(1.2f, 2.5f, 0.8f);
        glutWireCube(1.0f);
        glPopMatrix();

        // --------------------------------------------------------------------
        // WRIST PILE MATRIX INTERFACE (Radiocarpal Entry Hub)
        // --------------------------------------------------------------------
        glTranslatef(0.0f, -0.6f, 0.0f); // Move up to Wrist pivot location
        glRotatef(wrist.pitch_flexion, 1.0f, 0.0f, 0.0f); // Pitch Axis Rotation
        glRotatef(wrist.yaw_deviation, 0.0f, 1.0f, 0.0f); // Yaw Axis Rotation
        
        // Render Wrist Articulation Sphere Core
        glColor3f(0.55f, 0.35f, 0.30f);
        glutSolidSphere(0.75f, 16, 16);

        // --------------------------------------------------------------------
        // RENDER PALM COMPARTMENT (Dynamic Root Moving with Wrist)
        // --------------------------------------------------------------------
        glColor3f(0.35f, 0.38f, 0.42f);
        glPushMatrix();
        glTranslatef(0.0f, palm_h * 0.5f, 0.0f);
        glScalef(palm_w, palm_h, 0.45f);
        glutWireCube(1.0f);
        glPopMatrix();

        // --------------------------------------------------------------------
        // ITERATE AND ATTACH ALL FINGERS ON THE MOVING CARPAL SHELF
        // --------------------------------------------------------------------
        for (int i = 0; i < TOTAL_DIGITS; ++i) {
            FingerNode* f = digits[i];
            glPushMatrix();

            float lat_off = 0.0f, elev_off = palm_h, fwd_dep = 0.0f;
            switch(f->type) {
                case THUMB:  lat_off = -palm_w * 0.52f; elev_off = palm_h * 0.25f; fwd_dep = 0.15f; break;
                case INDEX:  lat_off = -palm_w * 0.38f; break;
                case MIDDLE: lat_off = -palm_w * 0.02f; break;
                case RING:   lat_off =  palm_w * 0.34f; break;
                case PINKY:  lat_off =  palm_w * 0.68f; elev_off *= 0.92f; break;
            }

            glTranslatef(lat_off, elev_off, fwd_dep);

            if (f->type == THUMB) {
                glRotatef(45.0f, 0.0f, 1.0f, 0.0f);   
                glRotatef(-20.0f, 0.0f, 0.0f, 1.0f);  
                glRotatef(f->mcp_splay, 0.0f, 0.0f, 1.0f); 
                glRotatef(f->mcp_flex, 1.0f, 0.0f, 0.0f);  
            } else {
                glRotatef(f->mcp_splay, 0.0f, 1.0f, 0.0f); 
                glRotatef(f->mcp_flex, 1.0f, 0.0f, 0.0f);  
            }

            // Proximal Segment
            glColor3f(0.85f, 0.62f, 0.52f); glutSolidSphere(f->thickness * 1.1f, 12, 12);
            glPushMatrix(); glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            GLUquadric* q = gluNewQuadric(); gluCylinder(q, f->thickness, f->thickness * 0.9f, f->mcp_len, 12, 12);
            glPopMatrix();
            glTranslatef(0.0f, f->mcp_len, 0.0f);

            // Intermediate Segment (Thumb skips this)
            if (f->type != THUMB) {
                glRotatef(f->pip_flex, 1.0f, 0.0f, 0.0f);
                glColor3f(0.82f, 0.58f, 0.48f); glutSolidSphere(f->thickness * 0.95f, 12, 12);
                glPushMatrix(); glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gluCylinder(q, f->thickness * 0.9f, f->thickness * 0.8f, f->pip_len, 12, 12); glPopMatrix();
                glTranslatef(0.0f, f->pip_len, 0.0f);
            }

            // Distal Segment
            glRotatef(f->dip_flex, 1.0f, 0.0f, 0.0f);
            glColor3f(0.79f, 0.54f, 0.44f); glutSolidSphere(f->thickness * 0.82f, 12, 12);
            glPushMatrix(); glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            gluCylinder(q, f->thickness * 0.8f, f->thickness * 0.55f, f->dip_len * 0.75f, 12, 12);
            glTranslatef(0.0f, 0.0f, f->dip_len * 0.75f); glutSolidSphere(f->thickness * 0.55f, 12, 12);
            glPopMatrix();

            gluDeleteQuadric(q);
            glPopMatrix(); 
        }
        glPopMatrix(); 
    }

    void consoleTelemetry() {
        std::cout << "\r[Wrist Engine Hub] "
                  << "W_PITCH: " << std::setw(5) << (int)wrist.pitch_flexion << " | "
                  << "W_YAW: " << std::setw(5) << (int)wrist.yaw_deviation   << " | "
                  << "MID_MCP: " << std::setw(4) << (int)digits[MIDDLE]->mcp_flex << std::flush;
    }
};

// ============================================================================
// SYSTEM RUNTIME EXECUTION WORKSPACE
// ============================================================================
HumanHandKinematicsEngine HandCoreEngine;
float camYaw = -25.0f;
float camPitch = 20.0f;
int globalClockTicks = 0;
int loopState = 0;

void displayScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // System Viewer Matrix Setup
    glTranslatef(0.0f, -0.5f, -9.0f);
    glRotatef(camPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(camYaw, 0.0f, 1.0f, 0.0f);

    HandCoreEngine.renderSystem();
    HandCoreEngine.consoleTelemetry();

    glutSwapBuffers();
}

void simulationTick(int timerID) {
    globalClockTicks++;

    if (globalClockTicks % 350 == 0) {
        loopState = (loopState + 1) % 4;
    }

    switch(loopState) {
        case 0: // Neutral Calibration Stance
            HandCoreEngine.wrist.target_pitch = 0.0f;
            HandCoreEngine.wrist.target_yaw = 0.0f;
            for(int i = THUMB; i < TOTAL_DIGITS; ++i) HandCoreEngine.setTargetFingerPose((FingerType)i, 0.0f, 0.0f, 0.0f, 0.0f);
            break;

        case 1: // THE TENODESIS DEEP DEMO: Max Wrist Extension (Peeche Modna)
            // Notice: Wrist values set target to -60.0f, look at the automated fingers curl on screen!
            HandCoreEngine.wrist.target_pitch = -60.0f; 
            HandCoreEngine.wrist.target_yaw = 0.0f;
            break;

        case 2: // Wrist Flexion (Forward Bend) + Ulnar Tilted Wave
            HandCoreEngine.wrist.target_pitch = 45.0f;
            HandCoreEngine.wrist.target_yaw = 25.0f; // Tilting towards pinky side
            break;

        case 3: // Dynamic Complex Wave (Simulating real-time casting motion)
            float waveSine = sin(globalClockTicks * 0.05f);
            HandCoreEngine.wrist.target_pitch = waveSine * 30.0f;
            HandCoreEngine.wrist.target_yaw = cos(globalClockTicks * 0.05f) * 15.0f;
            break;
    }

    // Process systemic dependencies and run core matrix integrations
    HandCoreEngine.resolveGlobalLinkageEquations();

    glutPostRedisplay();
    glutTimerFunc(16, simulationTick, 0);
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(45.0f, (float)w / (float)h, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void setupLighting() {
    glEnable(GL_DEPTH_TEST); glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
    GLfloat ambient[]  = { 0.24f, 0.24f, 0.27f, 1.0f };
    GLfloat diffuse[]  = { 0.85f, 0.85f, 0.85f, 1.0f };
    GLfloat position[] = { 5.0f, 7.0f, 6.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient); glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse); glLightfv(GL_LIGHT0, GL_POSITION, position);
    glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glClearColor(0.05f, 0.06f, 0.08f, 1.0f);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Complete Human Hand & Wrist Radiocarpal Matrix Kinematics System");

    setupLighting();
    glutDisplayFunc(displayScene); glutReshapeFunc(reshape);
    glutTimerFunc(16, simulationTick, 0);

    std::cout << "====================================================================" << std::endl;
    std::cout << "   WRIST RADIOCARPAL SUBSYSTEM LINKED INTO CORE STRUCTURAL MATRIX   " << std::endl;
    std::cout << "====================================================================" << std::endl;

    glutMainLoop();
    return 0;
}
