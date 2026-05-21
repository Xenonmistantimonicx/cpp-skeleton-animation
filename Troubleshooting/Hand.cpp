#include <GL/glut.h>
#include <cmath>
#include <iostream>

// Global Camera Controls (Troubleshooting ke waqt haath ko dhoondne ke liye)
float camX = 0.0f, camY = 1.5f, camZ = -15.0f;
float rotX = 10.0f, rotY = 0.0f;
int lastMouseX, lastMouseY;
bool isMousePressed = false;

// Joint angles control variables
float rightShoulderPitch = -20.0f, rightElbowFlex = 70.0f;
float leftShoulderPitch = -20.0f, leftElbowFlex = 45.0f;
float animationTimer = 0.0f;

// ============================================================================
// VISUAL DIAGNOSTIC TOOLS (Screen par haath na dikhne par ye madad karega)
// ============================================================================
void DrawDiagnosticGrid() {
    glDisable(GL_LIGHTING); // Grid ke liye light band karein taaki saf dikhe
    glLineWidth(1.0f);
    
    // Draw 3D Ground Grid (XZ Plane)
    glColor3f(0.2f, 0.25f, 0.3f);
    glBegin(GL_LINES);
    for(float i = -10.0f; i <= 10.0f; i += 1.0f) {
        glVertex3f(i, -5.0f, -10.0f); glVertex3f(i, -5.0f, 10.0f);
        glVertex3f(-10.0f, -5.0f, i); glVertex3f(10.0f, -5.0f, i);
    }
    glEnd();

    // Draw Central Axis Reference (Red = X, Green = Y, Blue = Z)
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(2.0f, 0.0f, 0.0f); // X
    glColor3f(0.0f, 1.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 2.0f, 0.0f); // Y
    glColor3f(0.0f, 0.0f, 1.0f); glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 2.0f); // Z
    glEnd();
    
    glEnable(GL_LIGHTING); // Light wapas chalu
}

// ============================================================================
// SAFE-MODE SINGLE BONE RENDERER (Pure Math, Ultra-Lightweight)
// ============================================================================
void RenderSafeBone(float length, float thickness) {
    GLUquadric* q = gluNewQuadric();
    gluQuadricDrawStyle(q, GLU_FILL);
    
    // Top Joint Sphere
    glutSolidSphere(thickness * 1.4f, 10, 10);
    
    // Bone Cylinder
    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(q, thickness, thickness * 0.8f, length, 10, 10);
    glPopMatrix();
    
    gluDeleteQuadric(q);
}

// ============================================================================
// UNIFIED ABSTRACT LIMB CHAIN (Dono Hands ke liye 100% Reusable Block)
// ============================================================================
void BuildAbstractArm(bool isLeft, float shoulderPitch, float elbowFlex) {
    glPushMatrix();

    // Step 1: Positioning & Flawless Mirror Integration
    if (isLeft) {
        glTranslatef(-4.5f, 0.0f, 0.0f); // Left shoulder point
        glScalef(-1.0f, 1.0f, 1.0f);     // Flip X-Axis (Symmetry Magic)
    } else {
        glTranslatef(4.5f, 0.0f, 0.0f);  // Right shoulder point
    }

    // Step 2: Shoulder Rotation & Humerus Render
    glRotatef(shoulderPitch, 1.0f, 0.0f, 0.0f);
    if(isLeft) glColor3f(0.75f, 0.8f, 0.85f); // Soft bone tone for left
    else glColor3f(0.85f, 0.88f, 0.92f);      // Slightly brighter for right
    
    RenderSafeBone(4.0f, 0.4f); // Render Humerus

    // Step 3: Move to Elbow Node & Flex
    glTranslatef(0.0f, -4.0f, 0.0f);
    glRotatef(-elbowFlex, 1.0f, 0.0f, 0.0f);
    
    // Render Forearm (Radius/Ulna abstract representation)
    glColor3f(0.65f, 0.68f, 0.72f);
    RenderSafeBone(3.2f, 0.3f);

    // Step 4: Move to Wrist & Draw Palm
    glTranslatef(0.0f, -3.2f, 0.0f);
    glColor3f(0.4f, 0.45f, 0.5f);
    glPushMatrix();
    glScalef(1.8f, 1.8f, 0.4f);
    glutSolidCube(1.0f); // Quick abstract palm representation
    glPopMatrix();

    glPopMatrix();
}

// ============================================================================
// CORE VIEWPORT MAIN ENGINE
// ============================================================================
void renderScene() {
    // Background Clear Color (Thoda dark blueprint vibe taaki 3D objects uth kar dikhein)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Interactive Camera Matrix System
    glTranslatef(camX, camY, camZ);
    glRotatef(rotX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);

    // 1. Diagnostic Environment Load karo
    DrawDiagnosticGrid();

    // 2. Render Both Arms via Unified Controller
    BuildAbstractArm(false, rightShoulderPitch, rightElbowFlex); // Right Arm
    BuildAbstractArm(true, leftShoulderPitch, leftElbowFlex);   // Left Arm

    glutSwapBuffers();
}

// ============================================================================
// EMERGENCY RUNTIME ANIMATION TICKER
// ============================================================================
void updateTicker(int value) {
    animationTimer += 0.05f;

    // Right Arm: Smooth ongoing organic wave calculation
    rightShoulderPitch = -20.0f + sin(animationTimer * 2.0f) * 15.0f;
    rightElbowFlex = 60.0f + cos(animationTimer * 2.0f) * 20.0f;

    // Left Arm: Dynamic complementary motion patterns
    leftElbowFlex = 50.0f + sin(animationTimer * 1.5f) * 25.0f;

    glutPostRedisplay();
    glutTimerFunc(16, updateTicker, 0); // 16ms = Stable 60 FPS Engine target
}

// ============================================================================
// INTERACTIVE TROUBLESHOOTING CONTROLS (Mouse se camera ghumane ke liye)
// ============================================================================
void mouseButtonCallback(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            isMousePressed = true;
            lastMouseX = x; lastMouseY = y;
        } else if (state == GLUT_UP) {
            isMousePressed = false;
        }
    }
}

void mouseMotionCallback(int x, int y) {
    if (isMousePressed) {
        rotY += (x - lastMouseX) * 0.5f;
        rotX += (y - lastMouseY) * 0.5f;
        lastMouseX = x; lastMouseY = y;
        glutPostRedisplay();
    }
}

void handleKeyboardInput(unsigned char key, int x, int y) {
    // Zoom in/out functions for debugging perspective spacing
    if (key == 'w' || key == 'W') camZ += 0.5f;
    if (key == 's' || key == 'S') camZ -= 0.5f;
    if (key == 27) exit(0); // Escape key se safe exit
    glutPostRedisplay();
}

void reshapeViewport(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(45.0f, (float)w / (float)h, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void setupDebugLighting() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    
    GLfloat lightPos[] = { 5.0f, 10.0f, 5.0f, 1.0f };
    GLfloat ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

    glClearColor(0.08f, 0.1f, 0.13f, 1.0f); // Fallback Slate Grey background
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1024, 600);
    glutCreateWindow("C++ OpenGL Master Diagnostic & Failback Engine");

    setupDebugLighting();
    
    // Callback Assignments
    glutDisplayFunc(renderScene);
    glutReshapeFunc(reshapeViewport);
    glutTimerFunc(16, updateTicker, 0);
    
    // Hooking Troubleshooting Handlers
    glutMouseFunc(buttonCallback);
    glutMotionFunc(motionCallback);
    glutKeyboardFunc(handleKeyboardInput);

    std::cout << "========================================================" << std::endl;
    std::cout << "    CRASH-PROOF DIAGNOSTIC ENG RETAINED AND ACTIVE      " << std::endl;
    std::cout << "    Controls: Mouse Drag to Rotate | W/S to Zoom        " << std::endl;
    std::cout << "========================================================" << std::endl;

    glutMainLoop();
    return 0;
}
