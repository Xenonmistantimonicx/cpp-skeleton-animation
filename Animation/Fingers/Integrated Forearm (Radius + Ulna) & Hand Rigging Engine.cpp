#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <vector>

const float ENGINE_PI = 3.14159265f;
enum FingerType { THUMB = 0, INDEX = 1, MIDDLE = 2, RING = 3, PINKY = 4, TOTAL_DIGITS = 5 };

struct JointLimits {
    float mcp_flex_min, mcp_flex_max;
    float mcp_splay_min, mcp_splay_max;
    float pip_flex_min, pip_flex_max;
    float dip_flex_min, dip_flex_max;
};

// ============================================================================
// DYNAMIC FOREARM CONTROLLER (PRO-SUPINATION ENGINE)
// ============================================================================
class ForearmSegmentController {
public:
    float forearm_length;
    float forearm_thickness;
    
    // Kinematic States
    float pronation_supination_angle; // 0 deg = Supination (Parallel), 90 deg = Pronation (Crossed)
    float target_pro_sup;
    float interpolation_speed;

    ForearmSegmentController() {
        forearm_length = 3.5f;
        forearm_thickness = 0.35f;
        pronation_supination_angle = 0.0f;
        target_pro_sup = 0.0f;
        interpolation_speed = 0.05f;
    }

    void updateForearmKinematics() {
        // Smoothly interpolate towards target rotation
        pronation_supination_angle += (target_pro_sup - pronation_supination_angle) * interpolation_speed;
    }

    // Renders the dual-bone system with mechanical precision
    void renderBones() {
        GLUquadric* q = gluNewQuadric();
        float bone_radius = forearm_thickness * 0.4f;
        float separation = forearm_thickness * 0.8f;

        // --------------------------------------------------------------------
        // 1. THE ULNA (The Anchor Bone)
        // --------------------------------------------------------------------
        // Biomechanically, Ulna wrist ke rotation ke sath spin nahi karti.
        // Yeh elbow se lekar pinky finger ki side tak straight rehti hai.
        glPushMatrix();
        glColor3f(0.72f, 0.75f, 0.78f); // Distinct bone color
        
        // Shift slightly to the medial (pinky) side
        glTranslatef(-separation * 0.5f, 0.0f, 0.0f);
        
        // Olécranon process (Elbow joint head)
        glutSolidSphere(bone_radius * 1.4f, 10, 10);
        
        // Ulna Shaft
        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // Point downwards towards wrist
        gluCylinder(q, bone_radius, bone_radius * 0.8f, forearm_length, 12, 12);
        glPopMatrix();
        
        // Distal Head of Ulna (Near wrist)
        glPushMatrix();
        glTranslatef(0.0f, -forearm_length, 0.0f);
        glutSolidSphere(bone_radius * 0.9f, 10, 10);
        glPopMatrix();
        glPopMatrix();

        // --------------------------------------------------------------------
        // 2. THE RADIUS (The Rotating Bone)
        // --------------------------------------------------------------------
        // Radius elbow par pivot karti hai aur wrist ke sath pura rotate hoti hai.
        glPushMatrix();
        glColor3f(0.85f, 0.88f, 0.90f); // Slightly brighter bone color
        
        // Proximal head at elbow (Lateral/Thumb side)
        glTranslatef(separation * 0.5f, 0.0f, 0.0f);
        glutSolidSphere(bone_radius * 1.2f, 10, 10); // Radial head

        // Apply Pronation/Supination Rotation Matrix around the Ulna pivot point
        // Rotation axis origin is at the elbow, swinging the wrist end
        glTranslatef(-separation * 0.5f, 0.0f, 0.0f); 
        glRotatef(-pronation_supination_angle, 0.0f, 1.0f, 0.0f); // Rotates around Y-axis
        glTranslatef(separation * 0.5f, 0.0f, 0.0f);

        // Radius Shaft (Starts straight, but visual crossover happens via matrix transformation)
        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        // Radius ends thicker near the thumb wrist interface
        gluCylinder(q, bone_radius * 0.9f, bone_radius * 1.5f, forearm_length, 12, 12);
        glPopMatrix();

        // Distal Styloid Process of Radius (The thick wrist base)
        glPushMatrix();
        glTranslatef(0.0f, -forearm_length, 0.0f);
        glutSolidSphere(bone_radius * 1.5f, 10, 10);
        glPopMatrix();

        glPopMatrix();
        gluDeleteQuadric(q);
    }
};

// ============================================================================
// INTEGRATED FINGER SEGMENT NODE WITH NAIL GEOMETRY
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

    void drawAdaptiveNailMesh() {
        glPushMatrix();
        glColor4f(0.92f, 0.80f, 0.78f, 0.85f); // Keratin look
        
        float nailWidth  = thickness * 0.82f;
        float nailLength = dip_len * 0.45f;
        float nailHeight = thickness * 0.56f;

        if (type == THUMB) {
            nailWidth *= 1.25f;
            nailLength *= 1.15f;
        }

        glTranslatef(0.0f, dip_len * 0.20f, nailHeight);

        glBegin(GL_QUAD_STRIP);
        int segments = 8;
        for (int i = 0; i <= segments; ++i) {
            float t = (float)i / segments;
            float angle = -ENGINE_PI * 0.15f + t * (ENGINE_PI * 0.30f);
            float xOffset = sin(angle) * nailWidth;
            float zOffset = cos(angle) * (thickness * 0.12f) - (thickness * 0.10f);

            glNormal3f(sin(angle), 0.0f, cos(angle));
            glVertex3f(xOffset, 0.0f, zOffset);
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
// SYSTEM MASTER CORE ENGINE (Convergence Hub with Dual Forearm Bones)
// ============================================================================
class HumanHandKinematicsEngine {
public:
    std::vector<FingerNode*> digits;
    WristJointController wrist;
    ForearmSegmentController forearm;
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
        forearm.updateForearmKinematics();
        wrist.updateWristKinematics();

        // Tenodesis & Biomechanical cross-talk loops
        float mid_ratio = digits[MIDDLE]->mcp_flex / digits[MIDDLE]->limits.mcp_flex_max;
        if (mid_ratio > 0.4f) {
            float drag_ring = (mid_ratio - 0.4f) * 35.0f;
            if (digits[RING]->t_mcp_flex < drag_ring) digits[RING]->t_mcp_flex = drag_ring;
        }
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

        // 1. RENDER FOREARM SEGMENT (Radius + Ulna System)
        // System origin sits at the Elbow Joint
        forearm.renderBones();

        // 2. RADIOCARPAL WRIST INTERFACE (Positioned at the base of Radius/Ulna)
        glTranslatef(0.0f, -forearm.forearm_length, 0.0f);
        
        // The hand rotates with the Radius during Pronation/Supination, 
        // so we inject the forearm rotation directly into the hand's root matrix tracker!
        glRotatef(-forearm.pronation_supination_angle, 0.0f, 1.0f, 0.0f);

        // Wrist Joint flexions
        glRotatef(wrist.pitch_flexion, 1.0f, 0.0f, 0.0f); 
        glRotatef(wrist.yaw_deviation, 0.0f, 1.0f, 0.0f); 
        
        glColor3f(0.55f, 0.35f, 0.30f); glutSolidSphere(0.65f, 16, 16);

        // 3. PALM ASSEMBLY
        glColor3f(0.35f, 0.38f, 0.42f);
        glPushMatrix(); 
        glTranslatef(0.0f, -palm_h * 0.5f, 0.0f); // Building downwards
        glScalef(palm_w, palm_h, 0.45f); 
        glutWireCube(1.0f); 
        glPopMatrix();

        // 4. DIGITAL SPLINE TRAVERSAL (Fingers + Nails)
        for (int i = 0; i < TOTAL_DIGITS; ++i) {
            FingerNode* f = digits[i];
            glPushMatrix();

            float lat_off = 0.0f, elev_off = -palm_h, fwd_dep = 0.0f;
            switch(f->type) {
                case THUMB:  lat_off = -palm_w * 0.52f; elev_off = -palm_h * 0.35f; fwd_dep = 0.15f; break;
                case INDEX:  lat_off = -palm_w * 0.38f; break;
                case MIDDLE: lat_off = -palm_w * 0.02f; break;
                case RING:   lat_off =  palm_w * 0.34f; break;
                case PINKY:  lat_off =  palm_w * 0.68f; elev_off *= 0.92f; break;
            }

            glTranslatef(lat_off, elev_off, fwd_dep);

            // Finger joints transformation matrix
            if (f->type == THUMB) {
                glRotatef(-45.0f, 0.0f, 1.0f, 0.0f);   
                glRotatef(f->mcp_splay, 0.0f, 0.0f, 1.0f); 
                glRotatef(-f->mcp_flex, 1.0f, 0.0f, 0.0f);  
            } else {
                glRotatef(f->mcp_splay, 0.0f, 1.0f, 0.0f); 
                glRotatef(-f->mcp_flex, 1.0f, 0.0f, 0.0f);  
            }

            // --- PROXIMAL PHALANX ---
            glColor3f(0.85f, 0.62f, 0.52f); glutSolidSphere(f->thickness * 1.1f, 12, 12);
            glPushMatrix(); glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            GLUquadric* q = gluNewQuadric(); gluCylinder(q, f->thickness, f->thickness * 0.9f, f->mcp_len, 12, 12);
            glPopMatrix();
            glTranslatef(0.0f, -f->mcp_len, 0.0f);

            // --- INTERMEDIATE PHALANX ---
            if (f->type != THUMB) {
                glRotatef(-f->pip_flex, 1.0f, 0.0f, 0.0f);
                glColor3f(0.82f, 0.58f, 0.48f); glutSolidSphere(f->thickness * 0.95f, 12, 12);
                glPushMatrix(); glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
                gluCylinder(q, f->thickness * 0.9f, f->thickness * 0.8f, f->pip_len, 12, 12); glPopMatrix();
                glTranslatef(0.0f, -f->pip_len, 0.0f);
            }

            // --- DISTAL PHALANX & EMBEDDED NAILS ---
            glRotatef(-f->dip_flex, 1.0f, 0.0f, 0.0f);
            glColor3f(0.79f, 0.54f, 0.44f); glutSolidSphere(f->thickness * 0.82f, 12, 12);
            
            glPushMatrix(); 
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            gluCylinder(q, f->thickness * 0.8f, f->thickness * 0.55f, f->dip_len * 0.75f, 12, 12);
            glTranslatef(0.0f, 0.0f, f->dip_len * 0.75f); glutSolidSphere(f->thickness * 0.55f, 12, 12);
            glPopMatrix();

            // Render nails flipped contextually to match anatomical layout
            glPushMatrix(); glRotatef(180.0f, 0.0f, 0.0f, 1.0f); f->drawAdaptiveNailMesh(); glPopMatrix();

            gluDeleteQuadric(q);
            glPopMatrix(); 
        }
        glPopMatrix(); 
    }
};

// ============================================================================
// PIPELINE MAIN EXECUTION LOGIC
// ============================================================================
HumanHandKinematicsEngine CoreEngine;
float viewYaw = 30.0f; 
float viewPitch = 25.0f;
int systemClockTicks = 0;
int stanceMachineState = 0;

void renderSceneViewport() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Push back slightly to fit the entire forearm + hand on viewport
    glTranslatef(0.0f, 1.8f, -11.0f);
    glRotatef(viewPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(viewYaw, 0.0f, 1.0f, 0.0f);

    CoreEngine.renderSystem();
    glutSwapBuffers();
}

void mainUpdateTick(int t_id) {
    systemClockTicks++;

    if (systemClockTicks % 300 == 0) {
        stanceMachineState = (stanceMachineState + 1) % 3;
    }

    switch(stanceMachineState) {
        case 0: // Full Supination State (Bones completely parallel)
            CoreEngine.forearm.target_pro_sup = 0.0f; 
            CoreEngine.wrist.target_pitch = 0.0f;
            break;

        case 1: // Progressive Pronation Cycle (Radius swings over Ulna)
            CoreEngine.forearm.target_pro_sup = 85.0f; // Cross-over execution
            CoreEngine.wrist.target_pitch = -15.0f;
            break;

        case 2: // Interactive Mechanics: Fast twisting wrist rotation
            CoreEngine.forearm.target_pro_sup = 45.0f + sin(systemClockTicks * 0.08f) * 40.0f;
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
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLfloat ambient[]  = { 0.25f, 0.25f, 0.28f, 1.0f };
    GLfloat diffuse[]  = { 0.85f, 0.85f, 0.85f, 1.0f };
    GLfloat position[] = { 5.0f, 5.0f, 8.0f, 1.0f }; 
    
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient); glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse); glLightfv(GL_LIGHT0, GL_POSITION, position);
    glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Advanced Forearm Radioulnar Pronation-Supination Mechanical Simulation Engine");

    configureAdvancedLighting();
    glutDisplayFunc(renderSceneViewport); glutReshapeFunc(viewportReshape);
    glutTimerFunc(16, mainUpdateTick, 0);

    std::cout << "====================================================================" << std::endl;
    std::cout << " RADIUS & ULNA DUAL-BONE ROTATION ENGINE INTEGRATED SUCCESSFULLY   " << std::endl;
    std::cout << "====================================================================" << std::endl;

    glutMainLoop();
    return 0;
}
