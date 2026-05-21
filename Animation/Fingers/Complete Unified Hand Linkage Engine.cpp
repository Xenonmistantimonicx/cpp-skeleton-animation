#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <vector>

// ============================================================================
// GLOBAL CORE MATHS & STRUCTS
// ============================================================================
const float HAND_PI = 3.14159265f;

enum FingerType { THUMB = 0, INDEX = 1, MIDDLE = 2, RING = 3, PINKY = 4, TOTAL_DIGITS = 5 };

struct JointLimits {
    float mcp_flex_min, mcp_flex_max;
    float mcp_splay_min, mcp_splay_max;
    float pip_flex_min, pip_flex_max;
    float dip_flex_min, dip_flex_max;
};

// ============================================================================
// INTEGRATED FINGER NODE (Skeletal Segment Descriptor)
// ============================================================================
class FingerNode {
public:
    FingerType type;
    std::string name;
    
    // Runtime Kinematic State Vectors
    float mcp_flex, mcp_splay;
    float pip_flex, dip_flex;
    
    // Target Anchors (Commanded Inputs)
    float t_mcp_flex, t_mcp_splay;
    float t_pip_flex, t_dip_flex;
    
    // Biological Boundary Matrix
    JointLimits limits;
    
    // Structural Proportions
    float mcp_len, pip_len, dip_len, thickness;
    float muscle_responsiveness;

    FingerNode(FingerType f_type, std::string f_name, JointLimits f_limits, 
               float m_len, float p_len, float d_len, float thick, float resp)
        : type(f_type), name(f_name), limits(f_limits), 
          mcp_len(m_len), pip_len(p_len), dip_len(d_len), thickness(thick), muscle_responsiveness(resp) {
        
        mcp_flex = mcp_splay = pip_flex = dip_flex = 0.0f;
        t_mcp_flex = t_mcp_splay = t_pip_flex = t_dip_flex = 0.0f;
    }

    float clamp(float val, float min_lim, float max_lim) {
        if (val < min_lim) return min_lim;
        if (val > max_lim) return max_lim;
        return val;
    }

    void interpolateStep() {
        mcp_flex  += (t_mcp_flex - mcp_flex) * muscle_responsiveness;
        mcp_splay += (t_mcp_splay - mcp_splay) * muscle_responsiveness;
        pip_flex  += (t_pip_flex - pip_flex) * muscle_responsiveness;
        dip_flex  += (t_dip_flex - dip_flex) * muscle_responsiveness;

        // Strict limit enforcement
        mcp_flex  = clamp(mcp_flex, limits.mcp_flex_min, limits.mcp_flex_max);
        mcp_splay = clamp(mcp_splay, limits.mcp_splay_min, limits.mcp_splay_max);
        pip_flex  = clamp(pip_flex, limits.pip_flex_min, limits.pip_flex_max);
        dip_flex  = clamp(dip_flex, limits.dip_flex_min, limits.dip_flex_max);
    }
};

// ============================================================================
// MASTER HAND KINEMATICS ENGINE (The Central Linkage Hub)
// ============================================================================
class HumanHandKinematicsEngine {
private:
    std::vector<FingerNode*> digits;
    float palm_width;
    float palm_height;

public:
    HumanHandKinematicsEngine() {
        palm_width = 2.2f;
        palm_height = 2.4f;
        initializeSkeletalArchitecture();
    }

    ~HumanHandKinematicsEngine() {
        for (auto digit : digits) delete digit;
    }

    void initializeSkeletalArchitecture() {
        // 1. THUMB (Pollex) - Robust Saddle configuration
        digits.push_back(new FingerNode(THUMB, "Thumb", 
            {-5.0f, 60.0f, -10.0f, 45.0f, 0.0f, 60.0f, -10.0f, 80.0f}, // Limits
            1.10f, 0.0f, 0.85f, 0.30f, 0.065f)); // Note: Thumb has 2 main phalanges (pip_len mapped to 0 inside render)

        // 2. INDEX (Digiti Secundus) - Precision Node
        digits.push_back(new FingerNode(INDEX, "Index", 
            {-10.0f, 90.0f, -7.0f, 15.0f, 0.0f, 100.0f, 0.0f, 80.0f},
            1.38f, 0.90f, 0.64f, 0.22f, 0.095f));

        // 3. MIDDLE (Digiti Tertius) - Structural Axis Core
        digits.push_back(new FingerNode(MIDDLE, "Middle", 
            {-5.0f, 90.0f, -8.0f, 8.0f, 0.0f, 110.0f, 0.0f, 90.0f},
            1.50f, 0.98f, 0.70f, 0.24f, 0.080f));

        // 4. RING (Digiti Quartus) - Dependent Tendon Anchor
        digits.push_back(new FingerNode(RING, "Ring", 
            {-5.0f, 90.0f, -12.0f, 8.0f, 0.0f, 100.0f, 0.0f, 85.0f},
            1.36f, 0.92f, 0.66f, 0.21f, 0.075f));

        // 5. PINKY (Digiti Minimus) - Lateral Escape Node
        digits.push_back(new FingerNode(PINKY, "Pinky", 
            {-10.0f, 90.0f, -20.0f, 5.0f, 0.0f, 95.0f, 0.0f, 85.0f},
            1.12f, 0.72f, 0.58f, 0.18f, 0.090f));
    }

    // Direct Target Interface mapping
    void setTargetPose(FingerType finger, float mcp_f, float mcp_s, float pip_f, float dip_f) {
        if(finger >= 0 && finger < TOTAL_DIGITS) {
            digits[finger]->t_mcp_flex = mcp_f;
            digits[finger]->t_mcp_splay = mcp_s;
            digits[finger]->t_pip_flex = pip_f;
            digits[finger]->t_dip_flex = dip_f;
        }
    }

    // ========================================================================
    // THE BIOMECHANICAL LINKAGE EQUATION SYSTEM (Cross-Talk Solver)
    // ========================================================================
    void resolveInterDigitalLinkage() {
        // --- 1. THE CONNEXUS INTERTENDINEUS EFFECT (Middle & Ring Dependency) ---
        // Ring finger cannot achieve clean full extension if Middle finger is deeply flexed.
        float middle_flexion_ratio = digits[MIDDLE]->mcp_flex / digits[MIDDLE]->limits.mcp_flex_max;
        if (middle_flexion_ratio > 0.4f) {
            // Force pull on Ring finger's structural minimum limits dynamically
            float dynamic_ring_mcp_min = (middle_flexion_ratio - 0.4f) * 35.0f; 
            if (digits[RING]->t_mcp_flex < dynamic_ring_mcp_min) {
                digits[RING]->t_mcp_flex = dynamic_ring_mcp_min; // Dragging Ring finger down
            }
        }
        
        // Inverse Tendon Coupling: Ring lifting drags Middle up slightly
        if (digits[RING]->mcp_flex < 15.0f && digits[MIDDLE]->t_mcp_flex > 45.0f) {
            digits[MIDDLE]->t_mcp_flex -= (15.0f - digits[RING]->mcp_flex) * 0.3f;
        }

        // --- 2. THE LATERAL SPLAY COLLISION SOLVER (Inter-Digital Spacing) ---
        // If Index splays hard left and Middle remains stationary, force boundary compensation
        // Map sequential lateral relationships across palm coordinates
        for (int i = INDEX; i < TOTAL_DIGITS - 1; ++i) {
            float current_splay = digits[i]->mcp_splay;
            float next_splay = digits[i+1]->mcp_splay;
            
            // Checking if fingers cross physical space boundaries (Spatial collision matrix)
            float distance_delta = current_splay - next_splay;
            float minimum_clearance = -12.0f; // Multi-axis spacing allowance
            
            if (distance_delta < minimum_clearance) {
                // Propagate kinetic force vector to shift the adjacent node automatically
                digits[i+1]->t_mcp_splay += (minimum_clearance - distance_delta) * 0.5f;
            }
        }

        // --- 3. DIGITAL DIP-PIP MATRIX COUPLING (Tenodesis Emulation) ---
        // For standard fingers, DIP flexion is mechanically coupled to PIP state via deep tendons
        for (int i = INDEX; i <= PINKY; ++i) {
            float natural_coupling_factor = 0.65f; 
            // If DIP is uncommanded, tie it directly to PIP posture
            if (digits[i]->t_dip_flex == 0.0f && digits[i]->pip_flex > 10.0f) {
                digits[i]->t_dip_flex = digits[i]->pip_flex * natural_coupling_factor;
            }
        }

        // Execute asynchronous muscle interpolation steps
        for (auto digit : digits) {
            digit->interpolateStep();
        }
    }

    // ========================================================================
    // UNIFIED GRAPHICS RENDER PIPELINE
    // ========================================================================
    void renderHand() {
        glPushMatrix();

        // 1. RENDER PALM COMPARTMENT RIGID HULL
        glColor3f(0.35f, 0.38f, 0.42f);
        glPushMatrix();
        glTranslatef(0.0f, palm_height * 0.5f, 0.0f);
        glScalef(palm_width, palm_height, 0.45f);
        glutWireCube(1.0f); // Spatial reference bound box
        glPopMatrix();

        // 2. ITERATE AND POSITION ALL LOGICAL DIGITS ACROSS THE CARPAL ARCH
        for (int i = 0; i < TOTAL_DIGITS; ++i) {
            FingerNode* f = digits[i];
            glPushMatrix();

            // Calculate precise anatomical root transformation matrix on carpal shelf
            float lateral_offset = 0.0f;
            float elevation_offset = palm_height;
            float forward_depth = 0.0f;

            switch(f->type) {
                case THUMB:
                    lateral_offset = -palm_width * 0.52f;
                    elevation_offset = palm_height * 0.25f;
                    forward_depth = 0.15f;
                    break;
                case INDEX:  lateral_offset = -palm_width * 0.38f; break;
                case MIDDLE: lateral_offset = -palm_width * 0.02f; break;
                case RING:   lateral_offset =  palm_width * 0.34f; break;
                case PINKY:  lateral_offset =  palm_width * 0.68f; elevation_offset *= 0.92f; break;
            }

            glTranslatef(lateral_offset, elevation_offset, forward_depth);

            // Execute specific geometric rendering rules per branch archetype
            if (f->type == THUMB) {
                // Apply the unique rotated trapezium plane transformation matrix
                glRotatef(45.0f, 0.0f, 1.0f, 0.0f);   // Outward Abduction configuration
                glRotatef(-20.0f, 0.0f, 0.0f, 1.0f);  // Downward pitch offset
                glRotatef(f->mcp_splay, 0.0f, 0.0f, 1.0f); // CMC Opposition Sweep
                glRotatef(f->mcp_flex, 1.0f, 0.0f, 0.0f);  // MCP Flexion
            } else {
                // Standard finger joint chain routing
                glRotatef(f->mcp_splay, 0.0f, 1.0f, 0.0f); // Y-Axis Adduction/Abduction
                glRotatef(f->mcp_flex, 1.0f, 0.0f, 0.0f);  // X-Axis Core Flexion
            }

            // --- SEGMENT 1: PROXIMAL COMPARTMENT ---
            glColor3f(0.85f, 0.62f, 0.52f);
            glutSolidSphere(f->thickness * 1.1f, 12, 12);
            
            glPushMatrix();
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            GLUquadric* q = gluNewQuadric();
            gluCylinder(q, f->thickness, f->thickness * 0.9f, f->mcp_len, 12, 12);
            glPopMatrix();

            glTranslatef(0.0f, f->mcp_len, 0.0f);

            // --- SEGMENT 2: INTERMEDIATE COMPARTMENT (Skipped for Thumb structure) ---
            if (f->type != THUMB) {
                glRotatef(f->pip_flex, 1.0f, 0.0f, 0.0f);
                glColor3f(0.82f, 0.58f, 0.48f);
                glutSolidSphere(f->thickness * 0.95f, 12, 12);

                glPushMatrix();
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gluCylinder(q, f->thickness * 0.9f, f->thickness * 0.8f, f->pip_len, 12, 12);
                glPopMatrix();

                glTranslatef(0.0f, f->pip_len, 0.0f);
            }

            // --- SEGMENT 3: DISTAL COMPARTMENT (Precision Tip) ---
            glRotatef(f->dip_flex, 1.0f, 0.0f, 0.0f);
            glColor3f(0.79f, 0.54f, 0.44f);
            glutSolidSphere(f->thickness * 0.82f, 12, 12);

            glPushMatrix();
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            gluCylinder(q, f->thickness * 0.8f, f->thickness * 0.55f, f->dip_len * 0.75f, 12, 12);
            glTranslatef(0.0f, 0.0f, f->dip_len * 0.75f);
            glutSolidSphere(f->thickness * 0.55f, 12, 12); // Tactile pad termination
            glPopMatrix();

            gluDeleteQuadric(q);
            glPopMatrix(); // Pop back to carpal base anchor
        }
        glPopMatrix(); // Return global coordinate space
    }

    void debugConsoleTelemetry() {
        std::cout << "\r[Global Matrix Engine Mode] "
                  << "MID_FX: " << std::setw(4) << (int)digits[MIDDLE]->mcp_flex << " | "
                  << "RNG_FX: " << std::setw(4) << (int)digits[RING]->mcp_flex << " | "
                  << "THM_OPP: " << std::setw(4) << (int)digits[THUMB]->mcp_splay << std::flush;
    }
};

// ============================================================================
// SYSTEM RUNTIME WORKING SPACE
// ============================================================================
HumanHandKinematicsEngine HandCoreEngine;
float viewYaw = 0.0f;
float viewPitch = 15.0f;
int globalClockTicks = 0;
int proceduralSequenceState = 0;

void displayPipeline() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Setup viewport pipeline matrix
    glTranslatef(0.0f, -1.0f, -8.5f);
    glRotatef(viewPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(viewYaw, 0.0f, 1.0f, 0.0f);

    // Call unified core simulation engine
    HandCoreEngine.renderHand();
    HandCoreEngine.debugConsoleTelemetry();

    glutSwapBuffers();
}

void simulationMasterTick(int timerID) {
    globalClockTicks++;

    // Staged Gesture Routine State Machine (Testing Cross-Talk Equations)
    if (globalClockTicks % 300 == 0) {
        proceduralSequenceState = (proceduralSequenceState + 1) % 4;
    }

    switch(proceduralSequenceState) {
        case 0: // Stance 0: Resetting hand to neutral flat open plane
            for(int i = THUMB; i < TOTAL_DIGITS; ++i) {
                HandCoreEngine.setTargetPose((FingerType)i, 0.0f, 0.0f, 0.0f, 0.0f);
            }
            break;

        case 1: // Stance 1: THE TENDON FORCE DEMO (Bending Middle finger deep)
            // NOTICE: Code triggers ONLY Middle flexion, but linkage engine will drag Ring finger down!
            HandCoreEngine.setTargetPose(MIDDLE, 85.0f, 0.0f, 90.0f, 0.0f);
            break;

        case 2: // Stance 2: COMBINED HIGH-DEX GRIP (Fist closure with dynamic thumb opposition tracking)
            HandCoreEngine.setTargetPose(THUMB,  35.0f, 42.0f, 0.0f, 45.0f); // Thumb sweeps over
            HandCoreEngine.setTargetPose(INDEX,  75.0f, -5.0f, 80.0f, 0.0f);
            HandCoreEngine.setTargetPose(MIDDLE, 80.0f, 0.0f,  85.0f, 0.0f);
            HandCoreEngine.setTargetPose(RING,   80.0f, 2.0f,  85.0f, 0.0f);
            HandCoreEngine.setTargetPose(PINKY,  85.0f, 5.0f,  90.0f, 0.0f);
            break;

        case 3: // Stance 3: THE INTER-DIGITAL SPLAY EVENT (Index forces lateral collision propagation)
            // Notice: Splaying Index deeply pushes adjacent finger parameters via solver
            HandCoreEngine.setTargetPose(INDEX,  10.0f, -25.0f, 20.0f, 0.0f); // Hard Abduction
            HandCoreEngine.setTargetPose(MIDDLE, 0.0f,  -5.0f,  0.0f,  0.0f);
            HandCoreEngine.setTargetPose(RING,   0.0f,  4.0f,   0.0f,  0.0f);
            HandCoreEngine.setTargetPose(PINKY,  5.0f,  22.0f,  10.0f, 0.0f); // Pinky flares out
            break;
    }

    // Solve inter-dependent equations and step physics timers
    HandCoreEngine.resolveInterDigitalLinkage();

    glutPostRedisplay();
    glutTimerFunc(16, simulationMasterTick, 0);
}

void reshapeViewport(int w, int h) {
    if (h == 0) h = 1;
    float aspect = (float)w / (float)h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, aspect, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void configureGlobalLighting() {
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    GLfloat ambient[]  = { 0.25f, 0.25f, 0.28f, 1.0f };
    GLfloat diffuse[]  = { 0.85f, 0.85f, 0.85f, 1.0f };
    GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat position[] = { 4.0f, 6.0f, 5.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Unified Human Hand Inter-Digital Kinematic Linkage Engine");

    configureGlobalLighting();

    glutDisplayFunc(displayPipeline);
    glutReshapeFunc(reshapeViewport);
    glutTimerFunc(16, simulationMasterTick, 0);

    std::cout << "====================================================================" << std::endl;
    std::cout << "     CENTRAL HAND ANATOMY COUPLING RESOLVER INITIALIZED RUNNING    " << std::endl;
    std::cout << "====================================================================" << std::endl;

    glutMainLoop();
    return 0;
}
