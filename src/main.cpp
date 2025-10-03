// Graphics Pgm 2 for Caleb Bowen
// File: meteor_car_445.cpp

#include <cstdio>
#include <cmath>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "OpenGL445.h"

/*
ARCHITECTURE STATEMENT
Main Idea:
  To have it based on vertex primitives only, but have to actually type vertex as few times as possible.
  This is the reason for the line2 utility function, and the drawBox function.

GLUT callbacks:
    - display_func redraws whole scene,
    - keyboard_func flips from STOPPED to RUNNING,
    - timer_func advances meteor position and readys itself again.

Data:
  Global state stores meteor center (x,y), animation flags, constants for car dimensions.

Render:
  Each frame clears to brand black (44,42,41), then draws car and meteor.
  All objects lie at z = -10.

Animation:
  Timer period = 20 ms meteor steps 4 units down each tick.
  Total fall time roughly 3 s.
*/

// Canvas & Colors 
#define canvas_Width 600
#define canvas_Height 600
char canvas_Name[] = "CS 445 Meteor & Car"; // creative name I know

// Brand black (44,42,41) normalized
constexpr GLfloat BRAND_R = 44.0f / 255.0f;
constexpr GLfloat BRAND_G = 42.0f / 255.0f;
constexpr GLfloat BRAND_B = 41.0f / 255.0f;

// Pumpkin orange (255,117,24) normalized
constexpr GLfloat PUMPKIN_R = 255.0f / 255.0f;
constexpr GLfloat PUMPKIN_G = 117.0f / 255.0f;
constexpr GLfloat PUMPKIN_B = 24.0f / 255.0f;

// Scene Geometry 
constexpr GLfloat CAR_BODY_W = 222.0f;
constexpr GLfloat CAR_BODY_H = 48.0f;
constexpr GLfloat CAR_CABIN_W = 102.0f;
constexpr GLfloat CAR_CABIN_H = 48.0f;
constexpr GLfloat WHEEL_SIZE = 30.0f;
constexpr GLfloat Z_PLANE = -10.0f;

// Meteor geometry
static int g_meteorList = 0;
constexpr GLfloat METEOR_SIDE = 24.0f;
static const GLfloat METEOR_HALF_DIAG = METEOR_SIDE / std::sqrt(2.0f); // In current C++ standard, constexpr cannot call std::sqrt apparently
constexpr GLfloat N_LEFT_TIP_FROM_RIGHT = 90.0f;
constexpr GLfloat METEOR_RADIUS = METEOR_SIDE;;

//  Animation Tuning 
constexpr unsigned TIMER_PERIOD_MS = 20; // ~50 FPS
constexpr GLfloat STEP_PER_TICK = 4.0f;  // units per tick



//  Global State 
enum AnimState { 
    STOPPED = 0, 
    RUNNING = 1, 
    FINISHED = 2
};

static AnimState g_state = STOPPED;
static GLfloat mx = 0.0f;
static GLfloat my = 0.0f;

static GLfloat cx = 0.0f;
static GLfloat cy = 0.0f;
//static GLfloat carOffsetX = 0.0f;

//  Utility: draw a line 
static inline void line2(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    glBegin(GL_LINES);
    glVertex3f(x1, y1, Z_PLANE);
    glVertex3f(x2, y2, Z_PLANE);
    glEnd();
}

// Displaylist for meteor


//  Draw: meteor 
static void drawMeteor(GLfloat cx, GLfloat cy)
{
    glTranslatef(cx, cy, -100.0f);
    glCallList(g_meteorList);
}

//  Draw: axis-aligned box a centered at orgin
static void drawBox(GLfloat length, GLfloat height)
{
    line2(-length/2, -height/2, length/2, -height/2);
    line2(-length/2, -height/2, -length/2, height/2);
    line2(-length/2, height/2, length/2, height/2);
    line2(length/2, height/2, length/2, -height/2);
}

//  Draw: car based on center (cx,cy) of the car body
static void drawCar(GLfloat cx, GLfloat cy)
{

    //left wheel
    glLoadIdentity(); 
    glTranslatef(cx+88, cy-39, 0.0f);
    drawBox(WHEEL_SIZE, WHEEL_SIZE);

    //right wheel
    glLoadIdentity();
    glTranslatef(cx-88, cy-39, 0.0f);
    drawBox(WHEEL_SIZE, WHEEL_SIZE); 

    // top box
    glLoadIdentity();
    glTranslatef(cx, cy+48, 0.0f);
    drawBox(CAR_CABIN_W, CAR_CABIN_H);

    //body box
    glLoadIdentity();
    glTranslatef(cx, cy, 0.0f);
    drawBox(CAR_BODY_W, CAR_BODY_H);

    glLoadIdentity();
    
}

//  Meteor bounds helpers 
static inline GLfloat meteorAtBottom(GLfloat cy) { return cy - METEOR_HALF_DIAG; }

//  Display callback 
static void display_func()
{
    glClearColor(BRAND_R, BRAND_G, BRAND_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glLoadIdentity();
    glColor3f(1.0f, 1.0f, 1.0f);

    drawCar(cx, cy);

    glColor3f(PUMPKIN_R, PUMPKIN_G, PUMPKIN_B);

    drawMeteor(mx, my);

    glFlush(); // single buffering
}

//  Timer callback 
static void timer_func(int /*value*/)
{
    if (g_state != RUNNING) return;

    my -= STEP_PER_TICK;

    if (meteorAtBottom(my) <= 0.0f) { //Break for indirect recursion(if it is technically indirect recursion)
        const GLfloat bottomNow = meteorAtBottom(my);
        my -= bottomNow;
        g_state = FINISHED;
        std::puts("Animation FINISHED");
        glutPostRedisplay();
        return;
    }

    glutPostRedisplay();
    glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);
}

//  Keyboard callback 
static void keyboard_func(unsigned char key, int /*x*/, int /*y*/)
{
    if (g_state == STOPPED) {
        g_state = RUNNING;
        std::puts("Key pressed -> animation RUNNING");
        if (key == 'j' || key == 'J') {
        //    cx += STEP_PER_TICK;
        }
        glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);
        return;
    }

    if (g_state == RUNNING) {
        if (key == 'j' || key == 'J') {
            cx += STEP_PER_TICK; // move car right by 4 units per press
            glutPostRedisplay();
        }
    }
}

// Init meteor position 
static void init_meteor_start()
{
    const GLfloat leftTipX = (GLfloat)canvas_Width - N_LEFT_TIP_FROM_RIGHT;
    mx = leftTipX + METEOR_HALF_DIAG;
    my = (GLfloat)canvas_Height - METEOR_HALF_DIAG;
}

// Init car position 
static void init_car_start()
{
    cx = 161.0f;
    cy = 54.0f;
}

//  Main 
int main(int argc, char **argv)
{
    std::printf("Any Key Click Will Start\n"); // Because printf is better than cout (Im not oppinionated lol)

    glutInit(&argc, argv);
    my_setup(canvas_Width, canvas_Height, canvas_Name);

    // Create meteor display list
    g_meteorList = glGenLists(1);
    glNewList(g_meteorList, GL_COMPILE);
        glPushMatrix();
        glScalef(METEOR_RADIUS, METEOR_RADIUS, METEOR_RADIUS);
        glutWireOctahedron();   
        glPopMatrix();
    glEndList();

    init_meteor_start();

    init_car_start();

    glutDisplayFunc(display_func);
    glutKeyboardFunc(keyboard_func);
    glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);

    glutMainLoop();
    return 0;
}
