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
constexpr GLfloat Z_PLANE = -100.0f;

// Meteor geometry

// Car start position

// Projectile constants
constexpr GLfloat GRAVITY = 32.0f; // ft/s^2
constexpr int DIAMOND_COUNT = 0;
constexpr int TEAPOT_COUNT = 0;
constexpr GLfloat PROJECTILE_RADIUS = 20.0f;

//  Animation Tuning 
constexpr unsigned TIMER_PERIOD_MS = 20; // ~50 FPS
constexpr GLfloat STEP_PER_TICK = 4.0f;  // units per tick

//  Global State 
enum AnimState { 
    READY = 0, 
    RUNNING = 1, 
    FINISHED = 2,
    NEXT_PROJECTILE =3
};


enum ProjectileState { 
    DIAMONDS = 0, 
    TEAPOT = 1
};

static AnimState g_state = READY;


static GLfloat cx = 25.0f;
static GLfloat cy = 455.0f;

static ProjectileState current_projectile = DIAMONDS;


static GLint projectileCount = 5;
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

// Projectiles

static void drawTeapot(GLfloat cx, GLfloat cy)
{
    //projectileCount--;
    glLoadIdentity();
    glTranslatef(cx, cy, -100.0f);
    
    glutWireTeapot(PROJECTILE_RADIUS);
    glLoadIdentity();
}

static void drawDiamond(GLfloat cx, GLfloat cy)
{
    //projectileCount--;
    glLoadIdentity();
    glTranslatef(cx, cy, -100.0f);
    glScalef(PROJECTILE_RADIUS, PROJECTILE_RADIUS, 0);
    glutWireOctahedron();
    glLoadIdentity();
}

static void drawPlatform()
{
    glLoadIdentity();
    glTranslatef(0, 450, 0.0f);
    line2(0, 0, 50, 0);
    glLoadIdentity();
}

static void drawWater()
{
    glLoadIdentity();
    //glTranslatef(0, 450, 0.0f);
    line2(0, 7, 700, 7);
    glLoadIdentity();
}

static void drawTower()
{
    glLoadIdentity();
    line2(440, 0, 540, 0);
    line2(440, 200, 540, 200);
    line2(440, 0, 440, 200);
    line2(540, 0, 540, 200);
    glLoadIdentity();
}


//static inline GLfloat meteorAtBottom(GLfloat cy) { return cy - METEOR_SIDE; }

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

static bool towerHit(GLfloat cx, GLfloat cy){
    if((cy-20) <= 200 && cx >= 420 && cx <= 560){ // pick up here
        g_state = FINISHED;
        return true;
    }
    return false;
}

static bool waterHit(GLfloat cx, GLfloat cy) {
    if((cy-20) <= 7){
        g_state = FINISHED;
        return true;
    }
    return false;
}

static bool rightHit(GLfloat cx, GLfloat cy) {
    if((cx+20) >= 600){
        
        g_state = FINISHED;
        return true;
    }
    return false;
}


//  Timer callback 
// Mecahnism: if in RUNNING state, move meteor down by STEP_PER_TICK
// If meteor bottom <= 0, set state to FINISHED and return
// Otherwise, post redisplay and reset timer
static void timer_func(int /*value*/)
{
    if (g_state != RUNNING && g_state != NEXT_PROJECTILE) return;
    
    if(waterHit(cx, cy) == true){
        std::puts("Animation FINISHED");
        if(projectileCount > 0){
            g_state = NEXT_PROJECTILE;
            cx = 25.0f;
            cy = 450.0f;
        }
        if(projectileCount <= 3){
            current_projectile = TEAPOT;
        }
        projectileCount--;
        glutPostRedisplay();
        return;
    }

    if(towerHit(cx, cy)){
        std::puts("You win");
        if(projectileCount > 0){
            g_state = NEXT_PROJECTILE;
            cx = 25.0f;
            cy = 450.0f;
        }
        if(projectileCount <= 3){
            current_projectile = TEAPOT;
        }
        projectileCount--;
        glutPostRedisplay();
        return;
    }

    if(rightHit(cx, cy)){
        std::puts("Animation FINISHED");
        if(projectileCount > 0){
            g_state = NEXT_PROJECTILE;
            cx = 25.0f;
            cy = 450.0f;
        }
        if(projectileCount <= 3){
            current_projectile = TEAPOT;
        }
        projectileCount--;
        glutPostRedisplay();
        return;
    }

    cy = cy - 32 * 0.02;
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

    if (g_state == NEXT_PROJECTILE) {
        glColor3f(1.0f, 1.0f, 1.0f);
        drawBitmapStringCenter("Press m or e to shoot", GLUT_BITMAP_HELVETICA_18, canvas_Width*0.5f, canvas_Height*0.5f);
        glFlush();
        return;
    }

    glLoadIdentity();
    glColor3f(1.0f, 1.0f, 1.0f);

    //drawCar(cx, cy);

    glColor3f(PUMPKIN_R, PUMPKIN_G, PUMPKIN_B);

    drawWater();

    drawPlatform();

    drawTower();

    if(current_projectile == DIAMONDS){
        drawDiamond(cx, cy);
    }
    else{
        drawTeapot(cx, cy);
    }

    


    

    //drawMeteor(mx, my);

    

    glFlush(); // single buffering
}



//  Keyboard callback 
static void keyboard_func(unsigned char key, int /*x*/, int /*y*/)
{
    if (g_state == READY || g_state == NEXT_PROJECTILE) {
        g_state = RUNNING;
        std::puts("Key pressed -> animation RUNNING");
        //if (key == 'm' || key == 'M' || key == 'e' || key == 'E') {
        //    cx += STEP_PER_TICK;
        //}
        glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);
        return;
    }

    if ((key == 'm' || key == 'M') && g_state != FINISHED) {
        cx += STEP_PER_TICK;
    }

    if ((key == 'e' || key == 'E') && g_state != FINISHED) {
        cx -= STEP_PER_TICK;
    }
    glutPostRedisplay();
    
}




//  Main 
int main(int argc, char **argv)
{

    glutInit(&argc, argv);
    my_setup(canvas_Width, canvas_Height, canvas_Name);

    // Create meteor display list using a scaled wire octahedron
    
    glutDisplayFunc(display_func);
    glutKeyboardFunc(keyboard_func);
    //glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);

    glutMainLoop();
    return 0;
}
