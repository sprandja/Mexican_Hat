#include <GL/freeglut.h> // Uses FreeGLUT (ensure NuGet package is installed)
#include <math.h>
#include <stdio.h>
#include "lu6_a.h"


#define GRID_SIZE 30 // Density of the control point grid


GLUnurbsObj* theNurb;
float rotateY = 0.0f;
float rotateX = 20.0f;

// Control Points Array: u, v, x/y/z
GLfloat ctlpoints[GRID_SIZE][GRID_SIZE][3];

// Knot vectors (needed for NURBS mathematics)
GLfloat knots[GRID_SIZE + 4]; // Order 4 (Cubic) needs #points + order knots

void init_surface_data() {
    // 1. Generate Control Points
    for (int u = 0; u < GRID_SIZE; u++) {
        for (int v = 0; v < GRID_SIZE; v++) {
            // Map grid index to coordinates -2.0 to 2.0
            float x = 4.0f * ((float)u / (GRID_SIZE - 1)) - 2.0f;
            float z = 4.0f * ((float)v / (GRID_SIZE - 1)) - 2.0f;

            // Calculate radius from center
            float r = sqrt(x * x + z * z);

            // --- FORMULA --- //

            // 1. The Crown
            // 1.6f is the height. 0.65f is the width of the crown.
            float crown = 1.6f * exp(-pow(r / 0.65f, 4));

            // 2. The Rim
            float rim = 0.25f * pow(r / 1.8f, 12);

            // Combine
            float y = crown + rim;

            // small negative numbers to 0.0
            if (y < 0.001f) y = 0.0f;

            ctlpoints[u][v][0] = x;
            ctlpoints[u][v][1] = y;
            ctlpoints[u][v][2] = z;
        }
    }

    // 2. Generate Knot Vector
    int numKnots = GRID_SIZE + 4;
    for (int i = 0; i < numKnots; i++) {
        if (i < 4) knots[i] = 0.0f;
        else if (i >= numKnots - 4) knots[i] = 1.0f;
        else knots[i] = (float)(i - 3) / (float)(GRID_SIZE - 3);
    }
}

// --- Initialization of OpenGL ---
// --- Initialization of OpenGL ---
void init() {
    // 1. Background: White
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Two-Sided Lighting
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    // Ambient Light
    GLfloat global_ambient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);

    // --- Material Settings (Orange/Gold) --- CHANGE MATERIAL COLOR
    GLfloat mat_diffuse[] = { 1.0f, 0.6f, 0.0f, 1.0f };
    GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat mat_shininess[] = { 100.0f };

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse); // Apply to FRONT AND BACK
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);

    // --- Light Source Settings ---
    GLfloat light_position[] = { 0.0f, 20.0f, 20.0f, 1.0f };
    GLfloat white_light[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
    glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Initialize NURBS Object
    theNurb = gluNewNurbsRenderer();
    gluNurbsProperty(theNurb, GLU_SAMPLING_TOLERANCE, 25.0);
    gluNurbsProperty(theNurb, GLU_DISPLAY_MODE, GLU_FILL);

    // Pre-calculate the geometry
    init_surface_data();
}

// --- Display Function ---
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // CHANGE CAMERA POSITION
    gluLookAt(0.0, 5.0, 10.0,  // Eye
        0.0, 0.0, 0.0,   // Center
        0.0, 1.0, 0.0);  // Up

    // Rotate the object
    glPushMatrix();
    glRotatef(rotateX, 1.0, 0.0, 0.0);
    glRotatef(rotateY, 0.0, 1.0, 0.0);

    // --- DRAW NURBS SURFACE ---
    gluBeginSurface(theNurb);

    // Draw the main surface
    gluNurbsSurface(theNurb,
        GRID_SIZE + 4, knots, // U knots
        GRID_SIZE + 4, knots, // V knots
        GRID_SIZE * 3,        // U stride (distance between U points in memory)
        3,                    // V stride (distance between V points in memory)
        &ctlpoints[0][0][0],  // Control points array
        4, 4,                 // Order (Cubic)
        GL_MAP2_VERTEX_3);    // Type

    // --- TRIMMING (Making it round) ---
    // NURBS are naturally square. We cut a circle to make the sombrero round.
    gluBeginTrim(theNurb);
    GLfloat curvePt[41][2]; // 40 segments for a circle
    for (int i = 0; i <= 40; i++) {
        float angle = 2.0f * 3.14159f * (float)i / 40.0f;
        // Circle centered at 0.5, 0.5 with radius 0.45 (in UV space 0..1)
        curvePt[i][0] = 0.5f + 0.45f * cos(angle);
        curvePt[i][1] = 0.5f + 0.45f * sin(angle);
    }
    // gluPwlCurve defines a Piecewise Linear trimming curve [cite: 195]
    gluPwlCurve(theNurb, 41, &curvePt[0][0], 2, GLU_MAP1_TRIM_2);
    gluEndTrim(theNurb);

    gluEndSurface(theNurb);

    glPopMatrix();
    glutSwapBuffers();
}

// --- Window Resize Handle ---
void reshape(int w, int h) {
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (GLdouble)w / (GLdouble)h, 3.0, 20.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// --- Animation ---
void timer(int value) {
    rotateY += 1.0f;
    if (rotateY > 360) rotateY -= 360;
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); // ~60 FPS
}

// --- Main Entry Point ---
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Sombrero NURBS Surface");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}
