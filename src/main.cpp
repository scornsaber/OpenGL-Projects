// Graphics Pgm 1 for Caleb Bowen
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

// Scene Geometry 
constexpr GLfloat CAR_BODY_W = 222.0f;
constexpr GLfloat CAR_BODY_H = 48.0f;
constexpr GLfloat CAR_CABIN_W = 102.0f;
constexpr GLfloat CAR_CABIN_H = 48.0f;
constexpr GLfloat WHEEL_SIZE = 30.0f;
constexpr GLfloat CAR_LEFT_MARGIN = 50.0f;
constexpr GLfloat WHEEL_FRONT_EDGE_TO_BODY_RIGHT = 18.0f;
constexpr GLfloat WHEEL_BACK_EDGE_TO_BODY_LEFT = 18.0f;
constexpr GLfloat WHEEL_BOTTOM_Y = 0.0f;
constexpr GLfloat Z_PLANE = -10.0f;

// Meteor geometry
constexpr GLfloat METEOR_SIDE = 24.0f;
static const GLfloat METEOR_HALF_DIAG = METEOR_SIDE / std::sqrt(2.0f); // In current C++ standard, constexpr cannot call std::sqrt apparently
constexpr GLfloat N_LEFT_TIP_FROM_RIGHT = 90.0f;

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
static GLfloat g_mx = 0.0f;
static GLfloat g_my = 0.0f;

//  Utility: draw a line 
static inline void line2(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    glBegin(GL_LINES);
    glVertex3f(x1, y1, Z_PLANE);
    glVertex3f(x2, y2, Z_PLANE);
    glEnd();
}

//  Draw: meteor 
static void drawMeteor(GLfloat cx, GLfloat cy)
{
    const GLfloat d = METEOR_HALF_DIAG;
    const GLfloat leftX = cx - d;
    const GLfloat rightX = cx + d;
    const GLfloat topY = cy + d;
    const GLfloat botY = cy - d;

    line2(leftX, cy, cx, topY);
    line2(cx, topY, rightX, cy);
    line2(rightX, cy, cx, botY);
    line2(cx, botY, leftX, cy);
}

//  Draw: axis-aligned box 
static void drawBox(GLfloat x, GLfloat y, GLfloat w, GLfloat h)
{
    const GLfloat x2 = x + w;
    const GLfloat y2 = y + h;
    line2(x, y, x2, y);
    line2(x2, y, x2, y2);
    line2(x2, y2, x, y2);
    line2(x, y2, x, y);
}

//  Draw: car 
static void drawCar()
{
    const GLfloat wheelY = WHEEL_BOTTOM_Y;
    const GLfloat wheelTopY = wheelY + WHEEL_SIZE;

    const GLfloat bodyX = CAR_LEFT_MARGIN;
    const GLfloat bodyY = wheelTopY;
    const GLfloat bodyRight = bodyX + CAR_BODY_W;

    const GLfloat backWheelRight = bodyX + WHEEL_BACK_EDGE_TO_BODY_LEFT;
    const GLfloat backWheelLeft = backWheelRight /*- WHEEL_SIZE*/;

    const GLfloat frontWheelFront = bodyRight - WHEEL_FRONT_EDGE_TO_BODY_RIGHT;
    const GLfloat frontWheelLeft = frontWheelFront - WHEEL_SIZE;

    drawBox(backWheelLeft, wheelY, WHEEL_SIZE, WHEEL_SIZE);
    drawBox(frontWheelLeft, wheelY, WHEEL_SIZE, WHEEL_SIZE); 

    drawBox(bodyX, bodyY, CAR_BODY_W, CAR_BODY_H);

    const GLfloat bodyCenterX = bodyX + CAR_BODY_W * 0.5f;
    const GLfloat cabinX = bodyCenterX - (CAR_CABIN_W * 0.5f);
    const GLfloat cabinY = bodyY + CAR_BODY_H;
    drawBox(cabinX, cabinY, CAR_CABIN_W, CAR_CABIN_H);
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

    drawCar();
    drawMeteor(g_mx, g_my);

    glFlush(); // single buffering
}

//  Timer callback 
static void timer_func(int /*value*/)
{
    if (g_state != RUNNING) return;

    g_my -= STEP_PER_TICK;

    if (meteorAtBottom(g_my) <= 0.0f) { //Break for indirect recursion(if it is technically indirect recursion)
        const GLfloat bottomNow = meteorAtBottom(g_my);
        g_my -= bottomNow;
        g_state = FINISHED;
        std::puts("Animation FINISHED");
        glutPostRedisplay();
        return;
    }

    glutPostRedisplay();
    glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);
}

//  Keyboard callback 
static void keyboard_func(unsigned char /*key*/, int /*x*/, int /*y*/)
{
    if (g_state == STOPPED) {
        g_state = RUNNING;
        std::puts("Key pressed -> animation RUNNING");
        glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);
    }
}

// Init meteor position 
static void init_meteor_start()
{
    const GLfloat leftTipX = (GLfloat)canvas_Width - N_LEFT_TIP_FROM_RIGHT;
    g_mx = leftTipX + METEOR_HALF_DIAG;
    g_my = (GLfloat)canvas_Height - METEOR_HALF_DIAG;
}

//  Main 
int main(int argc, char **argv)
{
    std::printf("Any Key Click Will Start\n"); // Because printf is better than cout (Im not oppinionated lol)

    glutInit(&argc, argv);
    my_setup(canvas_Width, canvas_Height, canvas_Name);

    init_meteor_start();

    glutDisplayFunc(display_func);
    glutKeyboardFunc(keyboard_func);
    glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);

    glutMainLoop();
    return 0;
}
