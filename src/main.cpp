// Graphics Pgm 3 for Caleb Bowen
// File: main.cpp

#include <cstdio>
#include <cmath>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "OpenGL445Setup-2025.h"
/*
ARCHITECTURE STATEMENT
Main Idea:
  Render a simple toss at the tower scene using MODELVIEW transforms.
  Every frame, clear the canvas and redraw the scene from current state.
  Projectiles are positioned and oriented via glTranslatef and glScalef, never by
  drawing at absolute coordinates directly.

GLUT Callbacks:
  - display_func: draws the entire scene each frame (full redraw; no trails).
  - keyboard_func:
      * In READY: pressing M or E selects gravity and starts the simulation.
      * In RUNNING: H or J nudges the projectile left or right by 4 units and
        also launches it so gravity begins to act.
  - timer_func: advances animation at about 20 Hz (re-armed each tick). While
    RUNNING and after the first H or J press (launched == true), applies gravity
    to the projectile vertical motion using a constant-acceleration update.
  - cooldown_ready: one-shot timer that fires 1 second after a projectile
    ends; spawns the next projectile on the pad and resumes RUNNING.

Data and Global State:
  - g_state: { READY, RUNNING, FINISHED, NEXT_PROJECTILE } controls the main loop.
  - current_projectile: { DIAMONDS, TEAPOT } selects which shape to draw.
  - (cx, cy): projectile centroid in world units (feet). cy is initialized so
    the projectile bottom rests on the launch pad.
  - v: current vertical velocity (ft/s). Initialized to 0 for each projectile.
  - gravity: magnitude of gravitational acceleration (ft/s^2). Sign is applied
    inside physics; positive values here mean downward acceleration is -gravity.
  - dt: fixed time step (0.02 s).
  - launched: false until the user presses H or J; gravity updates only after
    launch.
  - projectileCount: remaining projectiles (game flow control).
  - tower_hit: latched flag to print You Win when the tower is hit.

Rendering:
  - Canvas: 600x600, black background.
  - View box and world: x in [0, 600], y in [0, 600], z in [0, -100].
  - All visible geometry lies in the z = -50 plane (Z_PLANE and PROJECTILE_Z).
  - All drawing uses wireframe line segments.
  - Color scheme:
      * Water line (aqua) at y = 7 across the width.
      * Tower (red) at the lower right; width 100, height 200.
      * Launch pad (white) of length 50 at y = 450 attached to left wall.
      * Projectiles (green): diamond = GLUT wire octahedron; teapot = GLUT teapot.

Transforms and Sizes:
  - Projectiles are centered at (cx, cy, -50) via glTranslatef.
  - Diamond is uniformly scaled with glScalef(PROJECTILE_RADIUS) so its
    overall size is 40 (radius = 20).
  - Teapot is drawn with glutWireTeapot(PROJECTILE_RADIUS) which is about
    40 units overall.

Animation and Physics:
  - Start in READY: on-screen message prompts to choose gravity:
      Press e for Earths gravity and m for Ganymedes
  - Gravity selection (one-time, cannot change later):
      * M or m -> Ganymede: gravity = -4.7 
      * E or e -> Earth:    gravity = -32  
  - After selecting gravity, state goes to RUNNING and first projectile appears
    on the pad; it does not fall until the first H or J keypress sets launched to true.
  - Vertical motion update each timer tick when RUNNING and launched:
      y += v * dt + 0.5 * (-gravity) * dt^2
      v += (-gravity) * dt

Controls:
  - H or h: move projectile left by 4 units and set launched to true.
  - J or j: move projectile right by 4 units and set launched to true.

End of Flight and Lifecycle:
  - A projectile ends its flight immediately when any one occurs:
      * Bottom tip reaches the water line (y = 7).
      * Right tip reaches the right screen edge (x = 600).
      * Tower collision check triggers.
  - After a non-winning end, state goes to NEXT_PROJECTILE and the projectile is
    hidden for 1 second. After cooldown_ready fires, a new projectile is
    spawned on the launch pad (cx = 25, cy = 450 + radius, v = 0, launched = false)
    and state returns to RUNNING.
  - When tower_hit is true, display_func prints You Win and stops play.
  - Program exits only via the window manager or default system exit.

NOTES:
  - The teapot Radius has been kind of weird for me to figure out
    The spout and the handle seem to not be included
    I thought about changing the radius but I decided to keep it 20 like the assignment says
    If desired I can easily change the hit box to include the spout and handle
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
constexpr GLfloat Z_PLANE = -50.0f;

// Projectile constants

constexpr int DIAMOND_COUNT = 0;
constexpr int TEAPOT_COUNT = 0;
constexpr GLfloat PROJECTILE_RADIUS = 20.0f;

constexpr GLfloat PROJECTILE_Z = -50.0f;

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
static GLfloat cy = 450.0f + PROJECTILE_RADIUS;

static ProjectileState current_projectile = DIAMONDS;

static GLfloat dt = 0.02f;
static GLfloat v = 0;

static GLfloat gravity = 32;

static bool tower_hit = false;

static bool launched = false;

static bool visible = true;


static GLint projectileCount = 3;
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

// Draw the wireframe teapot centered at (cx, cy, PROJECTILE_Z).
// Uses PROJECTILE_RADIUS as the teapot size.
static void drawTeapot(GLfloat cx, GLfloat cy)
{
    glLoadIdentity();
    glTranslatef(cx, cy, PROJECTILE_Z);
    
    glutWireTeapot(PROJECTILE_RADIUS);
    glLoadIdentity();
}

// Draw the wireframe octahedron (diamond) centered at (cx, cy, PROJECTILE_Z).
// Uniformly scales by PROJECTILE_RADIUS to achieve size 40.
static void drawDiamond(GLfloat cx, GLfloat cy)
{
    glLoadIdentity();
    glTranslatef(cx, cy, PROJECTILE_Z);
    glScalef(PROJECTILE_RADIUS, PROJECTILE_RADIUS, PROJECTILE_RADIUS);
    glutWireOctahedron();
    glLoadIdentity();
}

// Draw the white launch pad line at y = 450 on the left edge.
static void drawPlatform()
{
    glLoadIdentity();
    glTranslatef(0, 450, 0);
    line2(0, 0, 50, 0);
    glLoadIdentity();
}

// Draw the aqua water line across the screen at y = 7.
static void drawWater()
{
    glLoadIdentity();
    //glTranslatef(0, 450, 0.0f);
    line2(0, 7, 700, 7);
    glLoadIdentity();
}

// Draw the red tower wireframe (axis-aligned box) at the lower-right.
static void drawTower()
{
    glLoadIdentity();
    line2(440, 0, 540, 0);
    line2(440, 200, 540, 200);
    line2(440, 0, 440, 200);
    line2(540, 0, 540, 200);
    glLoadIdentity();
}


// Advance position & velocity by timestep dt (seconds). Returns new y.
inline float stepY(float& y, float& v, float dt) {
    y += v * dt + 0.5f * -gravity * dt * dt;  // new position
    v += -gravity * dt;                        // new velocity
    return y;
}


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

// Quick tower-hit test based on current projectile center and radius.
// Returns true if projectile overlaps the tower region
static bool towerHit(GLfloat cx, GLfloat cy){
    if((cy-20) <= 200 && cx >= 420 && cx <= 560){
        //g_state = FINISHED;
        return true;
    }
    return false;
}

// Returns true if the projectile’s bottom tip reaches the water line (y = 7).
static bool waterHit(GLfloat cx, GLfloat cy) {
    if (current_projectile == TEAPOT){
        cy-=3;
    }
    if((cy-20) <= 7){
        //g_state = FINISHED;
        return true;
    }
    return false;
}

// Returns true if the projectile’s right tip reaches the right screen edge (x = 600).
static bool rightHit(GLfloat cx, GLfloat cy) {
    if((cx+20) >= 600){
        
        //g_state = FINISHED;
        return true;
    }
    return false;
}


// Declared so signature is there
static void timer_func(int /*value*/);


// Cooldown callback fired 1 second after a projectile ends.
// Makes the new projectile visible and resumes RUNNING with the timer.
static void cooldown_ready(int) {
    visible = true;
    g_state = RUNNING;  // after 1s, allow user to start the next shot
    glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);
    glutPostRedisplay();
}

// Timer callback driving the animation at ~20 ms intervals.
// Applies gravity after launch; handles hit/miss, cooldown scheduling, and redraw.
static void timer_func(int /*value*/)
{
    if (g_state != RUNNING) return;

    if (!launched) {
    glutPostRedisplay();
    glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0); // keep ticking, but no fall yet
    return;
}
    
    if(waterHit(cx, cy) == true){
        std::puts("Water");
        if(projectileCount <= 0){
            g_state = FINISHED;
            std::puts("Ran");
            glutPostRedisplay();
            return;
        }
        if(projectileCount > 0){
            g_state = NEXT_PROJECTILE;
            cx = 25.0f;
            cy = 450.0f + PROJECTILE_RADIUS;
            v = 0;
            launched = false;
            visible = false;
            glutTimerFunc(1000, cooldown_ready, 0);
        }
        if(projectileCount <= 1){
            current_projectile = TEAPOT;
        }
        projectileCount--;
        glutPostRedisplay();
        return;
    }

    if(towerHit(cx, cy)){
        std::puts("You win");
        g_state = FINISHED;
        tower_hit = true;
        /*if(projectileCount <= 0){
            g_state = FINISHED;
            glutPostRedisplay();
            return;
        }
        if(projectileCount > 0){
            g_state = NEXT_PROJECTILE;
            cx = 25.0f;
            cy = 450.0f;
            glutTimerFunc(1000, cooldown_ready, 0);
        }*/
        if(projectileCount <= 1){
            current_projectile = TEAPOT;
        }
        projectileCount--;
        glutPostRedisplay();
        return;
    }

    if(rightHit(cx, cy)){
        std::puts("Right wall");
        if(projectileCount <= 0){
            g_state = FINISHED;
            glutPostRedisplay();
            return;
        }
        if(projectileCount > 0){
            g_state = NEXT_PROJECTILE;
            cx = 25.0f;
            cy = 450.0f + PROJECTILE_RADIUS;
            v = 0;
            launched = false;
            visible = false;
            glutTimerFunc(1000, cooldown_ready, 0);
        }
        if(projectileCount <= 1){
            current_projectile = TEAPOT;
        }
        projectileCount--;
        glutPostRedisplay();
        return;
    }

    cy = stepY(cy, v, dt); // Apply acceleration
    glutPostRedisplay();
    glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);
}



// Display callback 
//  Render the whole scene
// GLUT display callback: clears and redraws the entire scene each frame.
// Shows start text (READY) or You Win (tower_hit), otherwise draws world and projectile.

static void display_func()
{
    glClearColor(BRAND_R, BRAND_G, BRAND_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (g_state == READY) {
        glColor3f(1.0f, 1.0f, 1.0f);
        drawBitmapStringCenter("Press M or E to Start", GLUT_BITMAP_HELVETICA_18, canvas_Width*0.5f, canvas_Height*0.5f);
        glFlush();
        return;
    }

    if (g_state == FINISHED && tower_hit == true) {
        glColor3f(1.0f, 1.0f, 1.0f);
        drawBitmapStringCenter("You Win!", GLUT_BITMAP_HELVETICA_18, canvas_Width*0.5f, canvas_Height*0.5f);
        glFlush();
        return;
    }


    glLoadIdentity();
    

    //drawCar(cx, cy);

    glColor3f(0.0f, 1.0f, 1.0f);


    drawWater();

    glColor3f(1.0f, 0.0f, 0.0f);
    drawTower();

    glColor3f(1.0f, 1.0f, 1.0f);
    drawPlatform();

    glColor3f(0.0f, 1.0f, 0.0f);
    if(current_projectile == DIAMONDS && visible == true){
        
        drawDiamond(cx, cy);
    }
    else if(visible == true){
        drawTeapot(cx, cy-5);
    }
   

    glFlush(); // single buffering
}



// GLUT keyboard callback: selects gravity in READY, nudges left/right in RUNNING.
// First H/J press sets launched = true to begin gravity. 
static void keyboard_func(unsigned char key, int /*x*/, int /*y*/)
{
    
    if (g_state == READY && (key == 'm' || key == 'M' || key == 'e' || key == 'E')) {
        g_state = RUNNING;
        std::puts("Key pressed -> animation RUNNING");
        if(key == 'M' || key == 'm'){
            gravity = 4.7;
        }
        else{
            gravity = 32;
        }
        glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);
        return;
    }

    /*if((key == 'x' || key == 'X') && g_state == NEXT_PROJECTILE){
        g_state = RUNNING;
        std::puts("Key pressed -> Next Projectile");
        glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);
        return;
    }*/

    if ((key == 'j' || key == 'J') && g_state == RUNNING) {
        cx += STEP_PER_TICK;
        launched = true;
    }

    if ((key == 'h' || key == 'H') && g_state == RUNNING) {
        cx -= STEP_PER_TICK;
        launched = true;
    }
    glutPostRedisplay();
    
}




//  Program entry: initializes GLUT/GL, registers callbacks, and enters main loop.
int main(int argc, char **argv)
{

    glutInit(&argc, argv);
    my_setup(canvas_Width, canvas_Height, canvas_Name);

    
    glutDisplayFunc(display_func);
    glutKeyboardFunc(keyboard_func);
    //glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);

    glutMainLoop();
    return 0;
}
