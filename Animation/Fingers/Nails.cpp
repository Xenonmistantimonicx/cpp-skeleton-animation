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
// INTEGRATED FINGER SEGMENT NODEWITH NAIL GEOMETRY
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

    // Dynamic Nail Mesh Generation Engine (Dorsal Quad Strip Projection)
    void drawAdaptiveNailMesh() {
        glPushMatrix();
        
        // Material specs: Translucent, slightly glossy, soft pinkish-keratin look
        glColor4f(0.92f, 0.80f, 0.78f, 0.85f); 
        
        // Precision coordinate extraction based on local finger scaling factors
        float nailWidth  = thickness * 0.82f;
        float nailLength = dip_len * 0.45f;
        float nailHeight = thickness * 0.56f; // Positioning on top surface profile

        // Structural adjustments specific to the broad Thumb Nail architecture
        if (type == THUMB) {
            nailWidth *= 1.25f;
            nailLength *= 1.15f;
        }

        // Shift origin slightly backward from extreme tip to lock on the ungual fold
        glTranslatef(0.0f, dip_len * 0.20f, nailHeight);

        // Render curved Keratin plate using parametric QUAD STRIPS
        glBegin(GL_QUAD_STRIP);
        int segments = 8;
        for (int i = 0; i <= segments; ++i) {
            // Curvature calculation for smooth biological nail arching profile
            float t = (float)i / segments;
            float angle = -ENGINE_PI * 0.15f + t * (ENGINE_PI * 0.30f);
            
            float xOffset = sin(angle) * nailWidth;
            float zOffset = cos(angle) * (thickness * 0.12f) - (thickness * 0.10f);

            // Normal Vector generation for accurate lighting interaction across nail plate
            glNormal3f(sin(angle), 0.0f, cos(angle));
            
            // Base vertex (Proximal Matrix near cuticle)
            glVertex3f(xOffset, 0.0f, zOffset);
            // Extension vertex (Distal Free Edge of the nail)
            glVertex3f(xOffset, nailLength, zOffset);
        }
        glEnd();

        glPopMatrix();
    }
};

// ============================================================================
// WRIST JOINT CONTROLLER 
// ============================================================================
class WristJointController {
public:
    float pitch_flexion, yaw_deviation;
    float target_pitch, target_yaw;
    const float WRIST_PITCH_MIN = -70.0f, WRIST_PITCH_MAX = 80.0f;
    const float WRIST_YAW_MIN   = -15.0f, WRIST_YAW_MAX   = 35.0f;
    float inertia_damping;

    WristJointController() {
        pitch_flexion = yaw_deviation = target_pitch = target_yaw = 0.0f;
        inertia_damping = 0.050f;
    }

    void updateWristKinematics() {
        pitch_flexion += (target_pitch - pitch_flexion) * inertia_damping;
        yaw_deviation += (target_yaw - yaw_deviation) * inertia_damping;

        if (pitch_flexion < WRIST_PITCH_MIN) pitch_flexion = WRIST_PITCH_MIN;
        if (pitch_flexion > WRIST_PITCH_MAX) pitch_flexion = WRIST_PITCH_MAX;
        if (yaw_deviation < WRIST_YAW_MIN)   yaw_deviation = WRIST_YAW_MIN;
        if (yaw_deviation > WRIST_YAW_MAX)   yaw_deviation = WRIST_YAW_MAX;
    }
};

// ============================================================================
// SYSTEM MASTER CORE ENGINE (Wrist-Digit-Nail Convergence Hub)
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

    void resolveGlobalLinkageEquations() {
        wrist.updateWristKinematics();

        // THE TENODESIS EFFECT SOLVER
        float tenodesis_factor = 0.0f;
        if (wrist.pitch_flexion < 0.0f) { 
            tenodesis_factor = (wrist.pitch_flexion / wrist.WRIST_PITCH_MIN) * 45.0f; 
            for (int i = INDEX; i <= PINKY; ++i) {
                digits[i]->t_mcp_flex += (tenodesis_factor - digits[i]->t_mcp_flex) * 0.1f;
                digits[i]->t_pip_flex += ((tenodesis_factor * 1.1f) - digits[i]->t_pip_flex) * 0.1f;
            }
        }

        // BIOMECHANICAL CROSS-TALK LOOP
        float mid_ratio = digits[MIDDLE]->mcp_flex / digits[MIDDLE]->limits.mcp_flex_max;
        if (mid_ratio > 0.4f) {
            float drag_ring = (mid_ratio - 0.4f) * 35.0f;
            if (digits[RING]->t_mcp_flex < drag_ring) digits[RING]->t_mcp_flex = drag_ring;
        }

        // INTER-PHALANGEAL LINKAGE COUPLING
        for (int i = INDEX; i <= PINKY; ++i) {
            if (digits[i]->t_dip_flex == 0.0f && digits[i]->pip_flex > 10.0f) {
                digits[i]->t_dip_flex = digits[i]->pip_flex * 0.65f;
            }
        }

        for (auto d : digits) d->stepKinematics();
    }

    // ========================================================================
    // ENGINE VISUALIZATION PIPELINE (Matrix Traversal Cascade)
    // ========================================================================
    void renderSystem() {
        glPushMatrix();

        // 1. Forearm Static Bone Blueprint
        glColor3f(0.2f, 0.22f, 0.25f);
        glPushMatrix(); glTranslatef(0.0f, -2.0f, -0.2f); glScalef(1.2f, 2.5f, 0.8f); glutWireCube(1.0f); glPopMatrix();

        // 2. Radiocarpal Matrix Interface
        glTranslatef(0.0f, -0.6f, 0.0f); 
        glRotatef(wrist.pitch_flexion, 1.0f, 0.0f, 0.0f); 
        glRotatef(wrist.yaw_deviation, 0.0f, 1.0f, 0.0f); 
        
        glColor3f(0.55f, 0.35f, 0.30f); glutSolidSphere(0.75f, 16, 16);

        // 3. Palm Assembly
        glColor3f(0.35f, 0.38f, 0.42f);
        glPushMatrix(); glTranslatef(0.0f, palm_h * 0.5f, 0.0f); glScalef(palm_w, palm_h, 0.45f); glutWireCube(1.0f); glPopMatrix();

        // 4. Digital Spline Traversal Engine (Attacking Bones + Nails)
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

            // --- PROXIMAL PHALANX ---
            glColor3f(0.85f, 0.62f, 0.52f); glutSolidSphere(f->thickness * 1.1f, 12, 12);
            glPushMatrix(); glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            GLUquadric* q = gluNewQuadric(); gluCylinder(q, f->thickness, f->thickness * 0.9f, f->mcp_len, 12, 12);
            glPopMatrix();
            glTranslatef(0.0f, f->mcp_len, 0.0f);

            // --- INTERMEDIATE PHALANX (Thumb Skips) ---
            if (f->type != THUMB) {
                glRotatef(f->pip_flex, 1.0f, 0.0f, 0.0f);
                glColor3f(0.82f, 0.58f, 0.48f); glutSolidSphere(f->thickness * 0.95f, 12, 12);
                glPushMatrix(); glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                gluCylinder(q, f->thickness * 0.9f, f->thickness * 0.8f, f->pip_len, 12, 12); glPopMatrix();
                glTranslatef(0.0f, f->pip_len, 0.0f);
            }

            // --- DISTAL PHALANX & EMBEDDED UNGUAL PLATES ---
            glRotatef(f->dip_flex, 1.0f, 0.0f, 0.0f);
            glColor3f(0.79f, 0.54f, 0.44f); glutSolidSphere(f->thickness * 0.82f, 12, 12);
            
            // Render actual fingertip core bone structure
            glPushMatrix(); 
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            gluCylinder(q, f->thickness * 0.8f, f->thickness * 0.55f, f->dip_len * 0.75f, 12, 12);
            glTranslatef(0.0f, 0.0f, f->dip_len * 0.75f); glutSolidSphere(f->thickness * 0.55f, 12, 12);
            glPopMatrix();

            // TRIGGER DYNAMIC LOCAL NAIL GENERATOR
            // Because this executes inside the local DIP coordinate matrix stack, the nail tracks fingertips perfectly!
            f->drawAdaptiveNailMesh();

            gluDeleteQuadric(q);
            glPopMatrix(); 
        }
        glPopMatrix(); 
    }

    void printTelemetry() {
        std::cout << "\r[System Matrix Render Engine Status] | Nails Operational: TRUE" 
                  << " | Wrist Pitch: " << std::setw(3) << (int)wrist.pitch_flexion 
                  << " | Index DIP Flex: " << std::setw(3) << (int)digits[INDEX]->dip_flex << std::flush;
    }
};

// ============================================================================
// PIPELINE WORKSPACE INITIALIZATION
// ============================================================================
HumanHandKinematicsEngine CoreEngine;
float viewYaw = -45.0f; // Tilted angle to clearly see fingernails on dorsal view
float viewPitch = 30.0f;
int systemClockTicks = 0;
int stanceMachineState = 0;

void renderSceneViewport() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glTranslatef(0.0f, -0.2f, -9.0f);
    glRotatef(viewPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(viewYaw, 0.0f, 1.0f, 0.0f);

    CoreEngine.renderSystem();
    CoreEngine.printTelemetry();

    glutSwapBuffers();
}

void mainUpdateTick(int t_id) {
    systemClockTicks++;

    if (systemClockTicks % 350 == 0) {
        stanceMachineState = (stanceMachineState + 1) % 4;
    }

    switch(stanceMachineState) {
        case 0: // Flat Calibrated Plane (Nails facing upwards towards camera view)
            CoreEngine.wrist.target_pitch = 0.0f; CoreEngine.wrist.target_yaw = 0.0f;
            for(int i = THUMB; i < TOTAL_DIGITS; ++i) CoreEngine.setTargetFingerPose((FingerType)i, 0.0f, 0.0f, 0.0f, 0.0f);
            break;

        case 1: // Extended Claw Posture (Locking PIP/DIP joints to display Nail tracing matrix profiles)
            CoreEngine.wrist.target_pitch = -25.0f; // Moderate back bend
            CoreEngine.setTargetFingerPose(INDEX,  30.0f, 0.0f, 65.0f, 50.0f);
            CoreEngine.setTargetFingerPose(MIDDLE, 30.0f, 0.0f, 70.0f, 55.0f);
            CoreEngine.setTargetFingerPose(RING,   30.0f, 0.0f, 65.0f, 50.0f);
            CoreEngine.setTargetFingerPose(PINKY,  35.0f, 0.0f, 60.0f, 45.0f);
            break;

        case 2: // Severe Tenodesis Curl (Passive fingers drop during deep wrist extension)
            CoreEngine.wrist.target_pitch = -65.0f; 
            break;

        case 3: // High Speed Typing Simulation (Oscillating fingertips dynamically)
            CoreEngine.wrist.target_pitch = 10.0f; // Slight downward relax
            float speedFactor = systemClockTicks * 0.15f;
            CoreEngine.setTargetFingerPose(INDEX,  15.0f + sin(speedFactor)*15.0f, 0.0f, 30.0f, 0.0f);
            CoreEngine.setTargetFingerPose(MIDDLE, 20.0f + cos(speedFactor)*20.0f, 0.0f, 35.0f, 0.0f);
            break;
    }

    CoreEngine.resolveGlobalLinkageEquations();
    glutPostRedisplay();
    glutTimerFunc(16, mainUpdateTick, 0);
}

void viewportReshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(45.0f, (float)w / (float)h, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void configureAdvancedLighting() {
    glEnable(GL_DEPTH_TEST); glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
    
    // Enabling alpha blending for translucent organic fingernail plate representation
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLfloat ambient[]  = { 0.26f, 0.26f, 0.29f, 1.0f };
    GLfloat diffuse[]  = { 0.88f, 0.88f, 0.88f, 1.0f };
    GLfloat position[] = { 4.0f, 8.0f, 7.0f, 1.0f }; // Light casting down to emphasize nail ridges
    
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient); glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse); glLightfv(GL_LIGHT0, GL_POSITION, position);
    glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Complete Human Hand, Wrist & Ungual Keratin Nail Matrix Architecture Engine");

    configureAdvancedLighting();
    glutDisplayFunc(renderSceneViewport); glutReshapeFunc(viewportReshape);
    glutTimerFunc(16, mainUpdateTick, 0);

    std::cout << "====================================================================" << std::endl;
    std::cout << "   UNGUAL NAIL MATRIX GENERATOR RIGGED AND SYNCHRONIZED COMPLETED  " << std::endl;
    std::cout << "====================================================================" << std::endl;

    glutMainLoop();
    return 0;
}
