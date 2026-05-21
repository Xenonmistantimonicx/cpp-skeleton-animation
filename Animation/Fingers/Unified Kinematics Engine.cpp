#include <GL/glut.h>
#include <cmath>
#include <iostream>
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
// SINGLE HUMERUS CORE (Used by both hands)
// ============================================================================
class HumerusSegmentController {
public:
    float humerus_length, humerus_thickness;
    float shoulder_pitch, shoulder_roll, shoulder_yaw;
    float target_pitch, target_roll, target_yaw;
    float interpolation_speed;

    HumerusSegmentController() {
        humerus_length = 4.2f; humerus_thickness = 0.45f;
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
        glColor3f(0.82f, 0.84f, 0.88f);

        // 1. Shoulder Ball Joint
        glutSolidSphere(head_radius, 14, 14);

        // 2. Bone Shaft
        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(q, humerus_thickness * 1.1f, humerus_thickness * 0.9f, humerus_length, 12, 12);
        glPopMatrix();

        // 3. Elbow Interface
        glTranslatef(0.0f, -humerus_length, 0.0f);
        glPushMatrix();
        glScalef(1.6f, 0.9f, 1.1f);
        glutSolidSphere(humerus_thickness * 1.1f, 12, 10);
        glPopMatrix();

        glPopMatrix();
        gluDeleteQuadric(q);
    }
};

// ============================================================================
// SINGLE FOREARM CORE (Used by both hands)
// ============================================================================
class ForearmSegmentController {
public:
    float forearm_length, forearm_thickness;
    float pronation_supination_angle, target_pro_sup, interpolation_speed;

    ForearmSegmentController() {
        forearm_length = 3.5f; forearm_thickness = 0.35f;
        pronation_supination_angle = target_pro_sup = 0.0f;
        interpolation_speed = 0.05f;
    }

    void updateForearmKinematics() {
        pronation_supination_angle += (target_pro_sup - pronation_supination_angle) * interpolation_speed;
    }

    void renderBones() {
        GLUquadric* q = gluNewQuadric();
        float bone_radius = forearm_thickness * 0.4f;
        float separation = forearm_thickness * 0.8f;

        // Ulna Bone
        glPushMatrix();
        glColor3f(0.72f, 0.75f, 0.78f);
        glTranslatef(-separation * 0.5f, 0.0f, 0.0f);
        glutSolidSphere(bone_radius * 1.4f, 10, 10);
        glPushMatrix(); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(q, bone_radius, bone_radius * 0.8f, forearm_length, 12, 12); glPopMatrix();
        glPushMatrix(); glTranslatef(0.0f, -forearm_length, 0.0f); glutSolidSphere(bone_radius * 0.9f, 10, 10); glPopMatrix();
        glPopMatrix();

        // Radius Bone (Rotational Tracking)
        glPushMatrix();
        glColor3f(0.85f, 0.88f, 0.90f);
        glTranslatef(separation * 0.5f, 0.0f, 0.0f);
        glutSolidSphere(bone_radius * 1.2f, 10, 10);
        glTranslatef(-separation * 0.5f, 0.0f, 0.0f);
        glRotatef(-pronation_supination_angle, 0.0f, 1.0f, 0.0f);
        glTranslatef(separation * 0.5f, 0.0f, 0.0f);
        glPushMatrix(); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(q, bone_radius * 0.9f, bone_radius * 1.5f, forearm_length, 12, 12); glPopMatrix();
        glPushMatrix(); glTranslatef(0.0f, -forearm_length, 0.0f); glutSolidSphere(bone_radius * 1.5f, 10, 10); glPopMatrix();
        glPopMatrix();
        gluDeleteQuadric(q);
    }
};

// ============================================================================
// FINGER NODES & NAILS
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
        mcp_flex = mcp_splay = pip_flex = dip_flex = t_mcp_flex = t_mcp_splay = t_pip_flex = t_dip_flex = 0.0f;
    }

    void stepKinematics() {
        mcp_flex  += (t_mcp_flex - mcp_flex) * responsiveness;
        mcp_splay += (t_mcp_splay - mcp_splay) * responsiveness;
        pip_flex  += (t_pip_flex - pip_flex) * responsiveness;
        dip_flex  += (t_dip_flex - dip_flex) * responsiveness;
    }

    void drawAdaptiveNailMesh() {
        glPushMatrix();
        glColor4f(0.94f, 0.82f, 0.80f, 0.85f);
        float nailWidth = thickness * 0.82f, nailLength = dip_len * 0.45f, nailHeight = thickness * 0.56f;
        if (type == THUMB) { nailWidth *= 1.25f; nailLength *= 1.15f; }
        glTranslatef(0.0f, dip_len * 0.20f, nailHeight);
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= 6; ++i) {
            float t = (float)i / 6.0f;
            float angle = -ENGINE_PI * 0.15f + t * (ENGINE_PI * 0.30f);
            glNormal3f(sin(angle), 0.0f, cos(angle));
            glVertex3f(sin(angle) * nailWidth, 0.0f, cos(angle) * (thickness * 0.12f));
            glVertex3f(sin(angle) * nailWidth, nailLength, cos(angle) * (thickness * 0.12f));
        }
        glEnd(); glPopMatrix();
    }
};

// ============================================================================
// UNIFIED KINEMATICS ENGINE CLASS (CAN RENDER BOTH LEFT AND RIGHT)
// ============================================================================
class FullArmKinematicsEngine {
public:
    HumerusSegmentController humerus;
    ForearmSegmentController forearm;
    float elbow_flexion_angle, target_elbow_flexion;
    std::vector<FingerNode*> digits;
    float palm_w, palm_h;

    FullArmKinematicsEngine() {
        palm_w = 2.2f; palm_h = 2.4f; elbow_flexion_angle = target_elbow_flexion = 0.0f;
        initializeSkeletalFramework();
    }
    ~FullArmKinematicsEngine() { for (auto d : digits) delete d; }

    void initializeSkeletalFramework() {
        digits.push_back(new FingerNode(THUMB,  {-5.0f, 60.0f, -10.0f, 45.0f, 0.0f, 60.0f, -10.0f, 80.0f}, 1.10f, 0.0f,  0.85f, 0.30f, 0.07f));
        digits.push_back(new FingerNode(INDEX,  {-10.0f, 90.0f, -7.0f, 15.0f,  0.0f, 100.0f, 0.0f, 80.0f}, 1.38f, 0.90f, 0.64f, 0.22f, 0.09f));
        digits.push_back(new FingerNode(MIDDLE, {-5.0f, 90.0f,  -8.0f, 8.0f,   0.0f, 110.0f, 0.0f, 90.0f}, 1.50f, 0.98f, 0.70f, 0.24f, 0.08f));
        digits.push_back(new FingerNode(RING,   {-5.0f, 90.0f,  -12.0f, 8.0f,  0.0f, 100.0f, 0.0f, 85.0f}, 1.36f, 0.92f, 0.66f, 0.21f, 0.07f));
        digits.push_back(new FingerNode(PINKY,  {-10.0f, 90.0f, -20.0f, 5.0f,  0.0f, 95.0f,  0.0f, 85.0f}, 1.12f, 0.72f, 0.58f, 0.18f, 0.09f));
    }

    void resolveEquations() {
        humerus.updateHumerusKinematics();
        elbow_flexion_angle += (target_elbow_flexion - elbow_flexion_angle) * 0.05f;
        forearm.updateForearmKinematics();
        for (auto d : digits) d->stepKinematics();
    }

    // ========================================================================
    // THE BI-LATERAL RENDERER SYSTEM (The Core Answer to Your Question)
    // ========================================================================
    void renderHandSystem(bool isLeftHand) {
        glPushMatrix();

        // 1. POSITIONING & SYMMETRY MATRIX INVERSION
        if (isLeftHand) {
            glTranslatef(-5.0f, 0.0f, 0.0f);  // Left Shoulder Position
            glScalef(-1.0f, 1.0f, 1.0f);      // FLIP THE X-AXIS! (Mirrors geometry & local math)
        } else {
            glTranslatef(5.0f, 0.0f, 0.0f);   // Right Shoulder Position
        }

        // 2. SHOULDER CORE NODE
        glRotatef(humerus.shoulder_pitch, 1.0f, 0.0f, 0.0f);
        glRotatef(humerus.shoulder_roll,  0.0f, 0.0f, 1.0f);
        glRotatef(humerus.shoulder_yaw,   0.0f, 1.0f, 0.0f);

        humerus.renderHumerusBone();

        // 3. ELBOW CORE HINGE
        glTranslatef(0.0f, -humerus.humerus_length, 0.0f);
        glRotatef(-elbow_flexion_angle, 1.0f, 0.0f, 0.0f);
        forearm.renderBones();

        // 4. WRIST CORE
        glTranslatef(0.0f, -forearm.forearm_length, 0.0f);
        glRotatef(-forearm.pronation_supination_angle, 0.0f, 1.0f, 0.0f);

        // Palm Mesh
        glColor3f(0.32f, 0.35f, 0.38f);
        glPushMatrix(); glTranslatef(0.0f, -palm_h * 0.5f, 0.0f); glScalef(palm_w, palm_h, 0.45f); glutWireCube(1.0f); glPopMatrix();

        // 5. DIGITAL SPLINE TERMINALS (Dono hands ke liye automatically absolute mirrored)
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

            // Proximal
            glPushMatrix(); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); GLUquadric* f_q = gluNewQuadric(); gluCylinder(f_q, f->thickness, f->thickness * 0.9f, f->mcp_len, 10, 10); glPopMatrix();
            glTranslatef(0.0f, -f->mcp_len, 0.0f);

            // Intermediate
            if (f->type != THUMB) {
                glRotatef(-f->pip_flex, 1.0f, 0.0f, 0.0f);
                glPushMatrix(); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(f_q, f->thickness * 0.9f, f->thickness * 0.8f, f->pip_len, 10, 10); glPopMatrix();
                glTranslatef(0.0f, -f->pip_len, 0.0f);
            }

            // Distal & Nails
            glRotatef(-f->dip_flex, 1.0f, 0.0f, 0.0f);
            glPushMatrix(); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(f_q, f->thickness * 0.8f, f->thickness * 0.55f, f->dip_len * 0.75f, 10, 10); glPopMatrix();
            glPushMatrix(); glRotatef(180.0f, 0.0f, 0.0f, 1.0f); f->drawAdaptiveNailMesh(); glPopMatrix();

            gluDeleteQuadric(f_q);
            glPopMatrix();
        }
        glPopMatrix();
    }
};

// ============================================================================
// DUAL INSTANTIATION ENGINE SPACE
// ============================================================================
FullArmKinematicsEngine RightArmInstance;
FullArmKinematicsEngine LeftArmInstance;

float viewYaw = 0.0f, viewPitch = 10.0f;
int globalTimelineClock = 0;

void renderSceneViewport() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Pull camera back to see both hands comfortably
    glTranslatef(0.0f, 1.5f, -18.0f);
    glRotatef(viewPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(viewYaw, 0.0f, 1.0f, 0.0f);

    // Call same code for Right Hand (isLeftHand = false)
    RightArmInstance.renderHandSystem(false);

    // Call exact same code for Left Hand (isLeftHand = true)
    LeftArmInstance.renderHandSystem(true);

    glutSwapBuffers();
}

void mainUpdateTick(int t_id) {
    globalTimelineClock++;

    // ----------------=======================================================
    // TESTING ANIMATION CHANNELS: Dono Hands same code reuse kar rahe hain!
    // ----------------=======================================================
    
    // 1. RIGHT ARM MOVEMENT (Wave Animation)
    RightArmInstance.humerus.target_pitch = -20.0f;
    RightArmInstance.humerus.target_roll = -40.0f;
    RightArmInstance.target_elbow_flexion = 90.0f;
    // Right Hand dynamically waves left-and-right using a sine wave
    RightArmInstance.humerus.target_yaw = sin(globalTimelineClock * 0.08f) * 35.0f; 

    // 2. LEFT ARM MOVEMENT (Fist Clench & Lift Animation)
    LeftArmInstance.humerus.target_pitch = -30.0f;
    LeftArmInstance.humerus.target_roll = -25.0f;
    // Left hand slowly bends elbow up and down
    LeftArmInstance.target_elbow_flexion = 60.0f + sin(globalTimelineClock * 0.04f) * 30.0f; 

    // Dynamic Finger Clenching for Left Hand (Making a moving fist)
    float clenchWave = (sin(globalTimelineClock * 0.05f) + 1.0f) * 0.5f; // Normalizes between 0 and 1
    for (int i = 0; i < TOTAL_DIGITS; ++i) {
        if(i == THUMB) {
            LeftArmInstance.digits[i]->t_mcp_flex = clenchWave * 40.0f;
        } else {
            LeftArmInstance.digits[i]->t_mcp_flex = clenchWave * 85.0f;
            LeftArmInstance.digits[i]->t_pip_flex = clenchWave * 90.0f;
            LeftArmInstance.digits[i]->t_dip_flex = clenchWave * 80.0f;
        }
    }

    // Resolve structural kinematics equations on both instances
    RightArmInstance.resolveEquations();
    LeftArmInstance.resolveEquations();

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
    GLfloat ambient[] = { 0.25f, 0.25f, 0.28f, 1.0f }, diffuse[] = { 0.85f, 0.85f, 0.85f, 1.0f }, position[] = { 0.0f, 10.0f, 10.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient); glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse); glLightfv(GL_LIGHT0, GL_POSITION, position);
    glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Unified Dual-Arm Bilateral Kinematics Engine");
    configureAdvancedLighting();
    glutDisplayFunc(renderSceneViewport); glutReshapeFunc(viewportReshape);
    glutTimerFunc(16, mainUpdateTick, 0);
    glutMainLoop();
    return 0;
}
