// Graphics Pgm 2 for Caleb Bowen
// File: meteor_car_445.cpp

#include <cstdio>
#include <cmath>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "OpenGL445Setup-2025.h"
/*
ARCHITECTURE STATEMENT
Main Idea:
  Instead of relying on drawing each object at desired position each frame,
  I use glTranslatef to move the origin to the desired position and draw
  each object at the origin. This made it much more enjoyable to code.
GLUT callbacks:
    - display_func redraws whole scene,
    - keyboard_func flips from READY to RUNNING,
    - timer_func advances meteor position and readys itself again.
Data:
  Global state stores meteor center (x,y), animation flags, constants for car dimensions.
  Callbacks read and update global state as needed.
  Displaylist for meteor.
Render:
  Each frame clears to brand black (44,42,41), then draws car and meteor.
  Drawing is done by translating to the desired position and drawing each object at the origin.
  Car is drawn with 3 boxes and 2 wheels.
  Meteor is drawn with a displaylist of a scaled wire octahedron.
  All objects lie at z = -10 except for meteor at -100.
Animation:
  Timer period = 20 ms meteor steps 4 units down each tick and car goes 4 units forward each time j/J is pressed.
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
static GLuint g_meteorList = 0;
constexpr GLfloat METEOR_SIDE = 24.0f;
static const GLfloat METEOR_HALF_DIAG = METEOR_SIDE / std::sqrt(2.0f); // In current C++ standard, constexpr cannot call std::sqrt apparently
constexpr GLfloat N_LEFT_TIP_FROM_RIGHT = 100.0f;
constexpr GLfloat METEOR_RADIUS = METEOR_SIDE;;

// Car start position
constexpr GLfloat CAR_START_X = 161.0f;
constexpr GLfloat CAR_START_Y = 54.0f;

constexpr GLfloat LEFFT_WHEEL_X_OFFSET = -78.0f;
constexpr GLfloat RIGHT_WHEEL_X_OFFSET = 78.0f;
constexpr GLfloat WHEEL_Y_OFFSET = -39.0f;

constexpr GLfloat CAR_TOP_Y_OFFSET = 48.0f;

//  Animation Tuning 
constexpr unsigned TIMER_PERIOD_MS = 20; // ~50 FPS
constexpr GLfloat STEP_PER_TICK = 4.0f;  // units per tick

//  Global State 
enum AnimState { 
    READY = 0, 
    RUNNING = 1, 
    FINISHED = 2
};

static AnimState g_state = READY;
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
    glLoadIdentity();
    glTranslatef(cx, cy, -100.0f);
    glCallList(g_meteorList);
    glLoadIdentity();
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

    //right wheel
    glLoadIdentity(); // I went a little overboard with the loadidentity calls to isolate a bug I was tracking down
    glTranslatef(cx+RIGHT_WHEEL_X_OFFSET, cy+ WHEEL_Y_OFFSET, 0.0f);
    drawBox(WHEEL_SIZE, WHEEL_SIZE);

    //left wheel
    glLoadIdentity();
    glTranslatef(cx+LEFFT_WHEEL_X_OFFSET, cy+ WHEEL_Y_OFFSET, 0.0f);
    drawBox(WHEEL_SIZE, WHEEL_SIZE); 

    // top box
    glLoadIdentity();
    glTranslatef(cx, cy+CAR_TOP_Y_OFFSET, 0.0f);
    drawBox(CAR_CABIN_W, CAR_CABIN_H);

    //body box
    glLoadIdentity();
    glTranslatef(cx, cy, 0.0f);
    drawBox(CAR_BODY_W, CAR_BODY_H);

    glLoadIdentity();
    
}

//  Meteor bounds helpers 
static inline GLfloat meteorAtBottom(GLfloat cy) { return cy - METEOR_SIDE; }

//  Utility: draw a bitmap string centered at (cx,cy) on Z_PLANE
static void drawBitmapStringCenter(const char* s, void* font, GLfloat cx, GLfloat cy)
{
    int w = 0; 
    for (const char* p = s; *p; ++p){
        w += glutBitmapWidth(font, *p);
    }
    glRasterPos3f(cx - w*0.5f, cy, Z_PLANE);
    for (const char* p = s; *p; ++p) {
        glutBitmapCharacter(font, *p);
    }
}

//  Timer callback 
// Mecahnism: if in RUNNING state, move meteor down by STEP_PER_TICK
// If meteor bottom <= 0, set state to FINISHED and return
// Otherwise, post redisplay and reset timer
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

// Display callback 
//  Render the whole scene
//  Clear to brand black, draw car and meteor
//  If in READY state, draw "Any Key Click Will Start" message
//  Single buffering

static void display_func()
{
    glClearColor(BRAND_R, BRAND_G, BRAND_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (g_state == READY) {
        glColor3f(1.0f, 1.0f, 1.0f);
        drawBitmapStringCenter("Any Key Click Will Start", GLUT_BITMAP_HELVETICA_18, canvas_Width*0.5f, canvas_Height*0.5f);
        glFlush();
        return;
    }

    glLoadIdentity();
    glColor3f(1.0f, 1.0f, 1.0f);

    drawCar(cx, cy);

    glColor3f(PUMPKIN_R, PUMPKIN_G, PUMPKIN_B);

    drawMeteor(mx, my);

    

    glFlush(); // single buffering
}



//  Keyboard callback 
static void keyboard_func(unsigned char key, int /*x*/, int /*y*/)
{
    if (g_state == READY) {
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
    my = (GLfloat)canvas_Height - METEOR_SIDE;
    const GLfloat N = N_LEFT_TIP_FROM_RIGHT; // 100
    mx = (GLfloat)canvas_Width - N + METEOR_SIDE;
}

// Init car position 
static void init_car_start()
{
    cx = CAR_START_X;
    cy = CAR_START_Y;
}



//  Main 
int main(int argc, char **argv)
{

    glutInit(&argc, argv);
    my_setup(canvas_Width, canvas_Height, canvas_Name);

    // Create meteor display list using a scaled wire octahedron
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
    //glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);

    glutMainLoop();
    return 0;
}
