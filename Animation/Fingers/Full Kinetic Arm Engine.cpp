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
// DYNAMIC UPPER ARM CONTROLLER (HUMERUS & SHOULDER MATRIX ENGINE)
// ============================================================================
class HumerusSegmentController {
public:
    float humerus_length;
    float humerus_thickness;

    // 3-DoF Shoulder Kinematics (Ball and Socket simulation)
    float shoulder_pitch; // Front-to-back swing (Flexion/Extension)
    float shoulder_roll;  // Side-to-side lift (Abduction/Adduction)
    float shoulder_yaw;   // Arm twisting (Internal/External Rotation)

    float target_pitch, target_roll, target_yaw;
    float interpolation_speed;

    HumerusSegmentController() {
        humerus_length = 4.2f;
        humerus_thickness = 0.45f;
        shoulder_pitch = shoulder_roll = shoulder_yaw = 0.0f;
        target_pitch = target_roll = target_yaw = 0.0f;
        interpolation_speed = 0.05f;
    }

    void updateHumerusKinematics() {
        shoulder_pitch += (target_pitch - shoulder_pitch) * interpolation_speed;
        shoulder_roll  += (target_roll - shoulder_roll) * interpolation_speed;
        shoulder_yaw   += (target_yaw - shoulder_yaw) * interpolation_speed;
    }

    void renderHumerusBone() {
        GLUquadric* q = gluNewQuadric();
        float head_radius = humerus_thickness * 1.5f;

        glPushMatrix();
        glColor3f(0.80f, 0.82f, 0.85f); // Solid Bone Tone

        // 1. Proximal Spherical Head (Fits into Glenoid Cavity of Shoulder)
        glutSolidSphere(head_radius, 14, 14);

        // 2. Bone Shaft Orientation (Directing downwards toward elbow)
        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        // Humerus is thick at the top head, slightly tapers, and flattens at distal condyles
        gluCylinder(q, humerus_thickness * 1.1f, humerus_thickness * 0.9f, humerus_length, 12, 12);
        glPopMatrix();

        // 3. Distal Epicondyles / Trochlea (The flanged spool structure at elbow interface)
        glTranslatef(0.0f, -humerus_length, 0.0f);
        glPushMatrix();
        glScalef(1.6f, 0.9f, 1.1f); // Flattened wide geometric profile for elbow hinge seating
        glutSolidSphere(humerus_thickness * 1.1f, 12, 10);
        glPopMatrix();

        glPopMatrix();
        gluDeleteQuadric(q);
    }
};

// ============================================================================
// FOREARM CONTROLLER (RADIUS & ULNA CROSSOVER)
// ============================================================================
class ForearmSegmentController {
public:
    float forearm_length;
    float forearm_thickness;
    float pronation_supination_angle; 
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
        pronation_supination_angle += (target_pro_sup - pronation_supination_angle) * interpolation_speed;
    }

    void renderBones() {
        GLUquadric* q = gluNewQuadric();
        float bone_radius = forearm_thickness * 0.4f;
        float separation = forearm_thickness * 0.8f;

        // --- THE ULNA ---
        glPushMatrix();
        glColor3f(0.72f, 0.75f, 0.78f);
        glTranslatef(-separation * 0.5f, 0.0f, 0.0f);
        glutSolidSphere(bone_radius * 1.4f, 10, 10); // Olecranon seating
        
        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(q, bone_radius, bone_radius * 0.8f, forearm_length, 12, 12);
        glPopMatrix();
        
        glPushMatrix(); glTranslatef(0.0f, -forearm_length, 0.0f); glutSolidSphere(bone_radius * 0.9f, 10, 10); glPopMatrix();
        glPopMatrix();

        // --- THE RADIUS ---
        glPushMatrix();
        glColor3f(0.85f, 0.88f, 0.90f);
        glTranslatef(separation * 0.5f, 0.0f, 0.0f);
        glutSolidSphere(bone_radius * 1.2f, 10, 10); 

        // Rotational pivot logic for crossover track
        glTranslatef(-separation * 0.5f, 0.0f, 0.0f); 
        glRotatef(-pronation_supination_angle, 0.0f, 1.0f, 0.0f); 
        glTranslatef(separation * 0.5f, 0.0f, 0.0f);

        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(q, bone_radius * 0.9f, bone_radius * 1.5f, forearm_length, 12, 12);
        glPopMatrix();

        glPushMatrix(); glTranslatef(0.0f, -forearm_length, 0.0f); glutSolidSphere(bone_radius * 1.5f, 10, 10); glPopMatrix();
        glPopMatrix();
        gluDeleteQuadric(q);
    }
};

// ============================================================================
// DIGITAL NODES AND NAIL KINEMATICS
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
        glColor4f(0.92f, 0.80f, 0.78f, 0.85f); 
        float nailWidth  = thickness * 0.82f;
        float nailLength = dip_len * 0.45f;
        float nailHeight = thickness * 0.56f;
        if (type == THUMB) { nailWidth *= 1.25f; nailLength *= 1.15f; }
        glTranslatef(0.0f, dip_len * 0.20f, nailHeight);
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= 8; ++i) {
            float t = (float)i / 8.0f;
            float angle = -ENGINE_PI * 0.15f + t * (ENGINE_PI * 0.30f);
            float xOffset = sin(angle) * nailWidth;
            float zOffset = cos(angle) * (thickness * 0.12f) - (thickness * 0.10f);
            glNormal3f(sin(angle), 0.0f, cos(angle));
            glVertex3f(xOffset, 0.0f, zOffset); glVertex3f(xOffset, nailLength, zOffset);
        }
        glEnd(); glPopMatrix();
    }
};

class WristJointController {
public:
    float pitch_flexion, yaw_deviation;
    float target_pitch, target_yaw;
    WristJointController() { pitch_flexion = yaw_deviation = target_pitch = target_yaw = 0.0f; }
    void updateWristKinematics() {
        pitch_flexion += (target_pitch - pitch_flexion) * 0.05f;
        yaw_deviation += (target_yaw - yaw_deviation) * 0.05f;
    }
};

// ============================================================================
// CONVERGENT HIERARCHICAL KINEMATICS ENGINE
// ============================================================================
class FullArmKinematicsEngine {
public:
    HumerusSegmentController humerus;
    ForearmSegmentController forearm;
    WristJointController wrist;
    std::vector<FingerNode*> digits;
    float palm_w, palm_h;

    // Dynamic Tracking Variable for Elbow Hinge Flexion
    float elbow_flexion_angle;
    float target_elbow_flexion;

    FullArmKinematicsEngine() {
        palm_w = 2.2f; palm_h = 2.4f;
        elbow_flexion_angle = target_elbow_flexion = 0.0f;
        initializeSkeletalFramework();
    }

    ~FullArmKinematicsEngine() { for (auto d : digits) delete d; }

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
        // Compute structural updates down the cascade chain
        humerus.updateHumerusKinematics();
        
        // Elbow Hinge local solver
        elbow_flexion_angle += (target_elbow_flexion - elbow_flexion_angle) * 0.05f;
        if (elbow_flexion_angle < 0.0f)   elbow_flexion_angle = 0.0f;
        if (elbow_flexion_angle > 145.0f) elbow_flexion_angle = 145.0f; // Biological limit

        forearm.updateForearmKinematics();
        wrist.updateWristKinematics();

        for (auto d : digits) d->stepKinematics();
    }

    // ========================================================================
    // ROOT MATRIX TRAVERSAL CASCADE (The Complete Arm Linkage Renderer)
    // ========================================================================
    void renderSystem() {
        glPushMatrix();

        // --------------------------------------------------------------------
        // LEVEL 1: SHOULDER CORE NODE (THE ROOT GENERATOR)
        // --------------------------------------------------------------------
        // Apply 3-DoF Ball-and-Socket rotations right at the shoulder point
        glRotatef(humerus.shoulder_pitch, 1.0f, 0.0f, 0.0f);
        glRotatef(humerus.shoulder_roll,  0.0f, 0.0f, 1.0f);
        glRotatef(humerus.shoulder_yaw,   0.0f, 1.0f, 0.0f);

        // Render Humerus Bone geometry
        humerus.renderHumerusBone();

        // --------------------------------------------------------------------
        // LEVEL 2: ELBOW ROTATION NODE (CASCADE LINKAGE TO FOREARM)
        // --------------------------------------------------------------------
        // Translate to the absolute tip end of the Humerus Shaft
        glTranslatef(0.0f, -humerus.humerus_length, 0.0f);
        
        // Hinge bending around local X axis
        glRotatef(-elbow_flexion_angle, 1.0f, 0.0f, 0.0f);

        // Render Radius and Ulna System
        forearm.renderBones();

        // --------------------------------------------------------------------
        // LEVEL 3: RADIOCARPAL WRIST NODE
        // --------------------------------------------------------------------
        glTranslatef(0.0f, -forearm.forearm_length, 0.0f);
        glRotatef(-forearm.pronation_supination_angle, 0.0f, 1.0f, 0.0f);

        glRotatef(wrist.pitch_flexion, 1.0f, 0.0f, 0.0f); 
        glRotatef(wrist.yaw_deviation, 0.0f, 1.0f, 0.0f); 
        
        glColor3f(0.55f, 0.35f, 0.30f); glutSolidSphere(0.65f, 14, 14);

        // --- PALM ASSEMBLY ---
        glColor3f(0.35f, 0.38f, 0.42f);
        glPushMatrix(); glTranslatef(0.0f, -palm_h * 0.5f, 0.0f); glScalef(palm_w, palm_h, 0.45f); glutWireCube(1.0f); glPopMatrix();

        // --------------------------------------------------------------------
        // LEVEL 4: DIGITAL SPLINE TERMINALS (Fingers, Joints, and Nails Mesh)
        // --------------------------------------------------------------------
        for (int i = 0; i < TOTAL_DIGITS; ++i) {
            FingerNode* f = digits[i];
            glPushMatrix();

            float lat_off = 0.0f, elev_off = -palm_h;
            switch(f->type) {
                case THUMB:  lat_off = -palm_w * 0.52f; elev_off = -palm_h * 0.35f; break;
                case INDEX:  lat_off = -palm_w * 0.38f; break;
                case MIDDLE: lat_off = -palm_w * 0.02f; break;
                case RING:   lat_off =  palm_w * 0.34f; break;
                case PINKY:  lat_off =  palm_w * 0.68f; elev_off *= 0.92f; break;
            }

            glTranslatef(lat_off, elev_off, 0.0f);

            if (f->type == THUMB) {
                glRotatef(-45.0f, 0.0f, 1.0f, 0.0f);   
                glRotatef(f->mcp_splay, 0.0f, 0.0f, 1.0f); glRotatef(-f->mcp_flex, 1.0f, 0.0f, 0.0f);  
            } else {
                glRotatef(f->mcp_splay, 0.0f, 1.0f, 0.0f); glRotatef(-f->mcp_flex, 1.0f, 0.0f, 0.0f);  
            }

            // --- PROXIMAL PHALANX ---
            glColor3f(0.85f, 0.62f, 0.52f); glutSolidSphere(f->thickness * 1.1f, 10, 10);
            glPushMatrix(); glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            GLUquadric* f_q = gluNewQuadric(); gluCylinder(f_q, f->thickness, f->thickness * 0.9f, f->mcp_len, 10, 10); glPopMatrix();
            glTranslatef(0.0f, -f->mcp_len, 0.0f);

            // --- INTERMEDIATE PHALANX ---
            if (f->type != THUMB) {
                glRotatef(-f->pip_flex, 1.0f, 0.0f, 0.0f);
                glColor3f(0.82f, 0.58f, 0.48f); glutSolidSphere(f->thickness * 0.95f, 10, 10);
                glPushMatrix(); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(f_q, f->thickness * 0.9f, f->thickness * 0.8f, f->pip_len, 10, 10); glPopMatrix();
                glTranslatef(0.0f, -f->pip_len, 0.0f);
            }

            // --- DISTAL PHALANX & EMBDEDDED NAILS ---
            glRotatef(-f->dip_flex, 1.0f, 0.0f, 0.0f);
            glColor3f(0.79f, 0.54f, 0.44f); glutSolidSphere(f->thickness * 0.82f, 10, 10);
            glPushMatrix(); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(f_q, f->thickness * 0.8f, f->thickness * 0.55f, f->dip_len * 0.75f, 10, 10); glPopMatrix();

            glPushMatrix(); glRotatef(180.0f, 0.0f, 0.0f, 1.0f); f->drawAdaptiveNailMesh(); glPopMatrix();

            gluDeleteQuadric(f_q);
            glPopMatrix(); 
        }
        glPopMatrix(); 
    }
};

// ============================================================================
// SYSTEM WORKSPACE REALTIME RENDERING ENGINE
// ============================================================================
FullArmKinematicsEngine CoreEngine;
float viewYaw = 40.0f; float viewPitch = 15.0f;
int systemClockTicks = 0; int stanceMachineState = 0;

void renderSceneViewport() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Pull back further to center the full long arm rig inside the viewport window
    glTranslatef(0.0f, 2.5f, -14.0f);
    glRotatef(viewPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(viewYaw, 0.0f, 1.0f, 0.0f);

    CoreEngine.renderSystem();
    glutSwapBuffers();
}

void mainUpdateTick(int t_id) {
    systemClockTicks++;

    if (systemClockTicks % 350 == 0) {
        stanceMachineState = (stanceMachineState + 1) % 4;
    }

    switch(stanceMachineState) {
        case 0: // Rest Position (Arm down, everything straight)
            CoreEngine.humerus.target_pitch = 0.0f; CoreEngine.humerus.target_roll = 0.0f; CoreEngine.humerus.target_yaw = 0.0f;
            CoreEngine.target_elbow_flexion = 0.0f; CoreEngine.forearm.target_pro_sup = 0.0f;
            break;

        case 1: // Lifting & Greeting Pose (Shoulder rolls out, elbow bends $90^\circ$)
            CoreEngine.humerus.target_pitch = -20.0f; // Swing forward slightly
            CoreEngine.humerus.target_roll = -35.0f;  // Abduct outwards
            CoreEngine.target_elbow_flexion = 90.0f;  // L-shape bend
            CoreEngine.forearm.target_pro_sup = 0.0f;
            break;

        case 2: // Complex Combined Cycle (Twisting Humerus while Pronating Forearm)
            CoreEngine.humerus.target_yaw = 45.0f;     // Internal bone rotation
            CoreEngine.forearm.target_pro_sup = 85.0f; // Turn palm down
            CoreEngine.target_elbow_flexion = 45.0f;
            break;

        case 3: // Wave Simulation (Shoulder swings back and forth dynamically)
            CoreEngine.humerus.target_roll = -30.0f + sin(systemClockTicks * 0.1f) * 15.0f;
            CoreEngine.target_elbow_flexion = 80.0f;
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

    GLfloat ambient[]  = { 0.22f, 0.22f, 0.25f, 1.0f };
    GLfloat diffuse[]  = { 0.85f, 0.85f, 0.85f, 1.0f };
    GLfloat position[] = { 6.0f, 6.0f, 10.0f, 1.0f }; 
    
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient); glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse); glLightfv(GL_LIGHT0, GL_POSITION, position);
    glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Complete Human Arm Kinematics Engine - Humerus Integration Rig");

    configureAdvancedLighting();
    glutDisplayFunc(renderSceneViewport); glutReshapeFunc(viewportReshape);
    glutTimerFunc(16, mainUpdateTick, 0);

    std::cout << "====================================================================" << std::endl;
    std::cout << "   HUMERUS AND GLENOHUMERAL JOINT ROOT SYSTEM SYNCHRONIZED READY    " << std::endl;
    std::cout << "====================================================================" << std::endl;

    glutMainLoop();
    return 0;
}
