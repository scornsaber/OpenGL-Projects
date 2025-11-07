// Graphics Pgm 4 for Caleb Bowen
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
#define canvas_Width 800
#define canvas_Height 800
char canvas_Name[] = "CS 445 Fish in a Tank";
// PANTONE Spun Sugar normalized
constexpr GLfloat BRAND_R = 180.0f / 255.0f;
constexpr GLfloat BRAND_G = 220.0f / 255.0f;
constexpr GLfloat BRAND_B = 234.0f / 255.0f;

//  PANTONE Tangerine Tango
constexpr GLfloat ORANGE_R = 221.0f / 255.0f;
constexpr GLfloat ORANGE_G = 65.0f / 255.0f;
constexpr GLfloat ORANGE_B = 36.0f / 255.0f;

// PANTONE Spicy Mustard.
constexpr GLfloat SPICY_R = 216.0f / 255.0f;
constexpr GLfloat SPICY_G = 174.0f / 255.0f;
constexpr GLfloat SPICY_B = 72.0f / 255.0f;

//  PANTONE Hunter Green.
constexpr GLfloat GREEN_R = 53.0f / 255.0f;
constexpr GLfloat GREEN_G = 94.0f / 255.0f;
constexpr GLfloat GREEN_B = 59.0f / 255.0f;



//  Animation Tuning 
constexpr unsigned TIMER_PERIOD_MS = 20; // ~50 FPS
constexpr GLfloat STEP_PER_TICK = 4.0f;  // units per tick

enum Direction {
  Right = 0,
  Left = 1
};

enum Size {
  Big = 0,
  Small = 1
};

enum State {
  Swimming = 0,
  Done = 1
};


static Direction current_direction = Right;
static State current_state = Swimming;

static GLfloat fx = -5.0f;
static GLfloat fy = 0.0f;
static constexpr GLfloat fz = -400.0f;
//  Utility: draw a line 
static inline void line2(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    glBegin(GL_LINES);
    glVertex3f(x1, y1, -400);
    glVertex3f(x2, y2, -400);
    glEnd();
}

static void drawTeapot(GLfloat cx, GLfloat cy)
{
    glLoadIdentity();
    glTranslatef(cx, cy, -400.0f);
    
    glutWireTeapot(50);
    glLoadIdentity();
}

// Draw the wireframe octahedron (diamond) centered at (cx, cy, PROJECTILE_Z).
// Uniformly scales by PROJECTILE_RADIUS to achieve size 40.
static void drawFishBody(GLfloat x, GLfloat y, GLfloat z, Size s)
{   
    GLfloat w = 75.0f;
    GLfloat h = 25.0f;
    GLfloat d = 12.5f;
    if(s == Small){
        w = 25.0f;
        h = 10.0f;
        d = 5.0f;
    }
    glLoadIdentity();
    glTranslatef(x, y, z);
    glScalef(w, h, d);
    glutWireOctahedron();
    glLoadIdentity();
}

static void drawFishTail(GLfloat x, GLfloat y, Size s, Direction di)
{
    GLfloat x2;
    if(s == Small)
    {
        if(di == Right){
          x-= 25;
          x2 = x-7;
        }
        else{
          x+= 25;
          x2 = x+7;
        }
        glLoadIdentity();
        glBegin(GL_LINE_LOOP);
        glVertex3f(x,y,-400.0f);
        glVertex3f(x2,y-3.5,-400.0f);
        glVertex3f(x2,y+3.5,-400.0f);
        glEnd();
    }
    else
    {
        if(di == Right){
          x-= 75;
          x2 = x-20;
        }
        else{
          x+= 75;
          x2 = x+20;
        }
        glLoadIdentity();
        glBegin(GL_LINE_LOOP);
        glVertex3f(x,y,-400.0f);
        glVertex3f(x2,y-10,-400.0f);
        glVertex3f(x2,y+10,-400.0f);
        glEnd();
    }
    
}

static void drawSmallFish(GLfloat x, GLfloat y, GLfloat z, Direction d) {
  glPushMatrix();
  drawFishBody(x,y,z,Small);
  drawFishTail(x, y, Small, d);
  glPopMatrix();

}

static void drawLargeFish(GLfloat x, GLfloat y, GLfloat z, Direction d) {
  glPushMatrix();
  drawFishBody(x,y,z, Big);
  drawFishTail(x, y, Big, d);
  glPopMatrix();
}


//  Utility: draw a bitmap string centered at (cx,cy) on Z_PLANE
static void drawBitmapStringCenter(const char* s, void* font, GLfloat cx, GLfloat cy)
{
    int w = 0; 
    for (const char* p = s; *p; ++p){
        w += glutBitmapWidth(font, *p);
    }
    glRasterPos3f(cx - w*0.5f, cy, -400.0f);
    for (const char* p = s; *p; ++p) {
        glutBitmapCharacter(font, *p);
    }
}


static void Turn(){
    if(fx+75 >= 396.0f || fx-75 <= -396.0f){
      if(current_direction == Right){
        current_direction = Left;
      }
      else{
        current_direction = Right;
      }
    }
}




// Timer callback driving the animation at ~20 ms intervals.

static void timer_func(int /*value*/)
{
    if(current_state == Done){
      return;
    }
    glutPostRedisplay();
    glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);
}




static void display_func()
{
    if(current_state == Done){
      return;
    }
    glClearColor(BRAND_R, BRAND_G, BRAND_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    
    glLoadIdentity();

    glColor3f(GREEN_R, GREEN_G, GREEN_B);
    drawTeapot(75.0f, -360.0f);

    glColor3f(ORANGE_R, ORANGE_G, ORANGE_B);
    Turn();
    if(current_direction == Right){
      fx+=5;
    }
    else{
      fx-=5;
    }
    drawLargeFish(fx, fy, fz, current_direction);
    glColor3f(SPICY_R, SPICY_G, SPICY_B);
    drawSmallFish(-325.0f, -350.0f, -400.0f, Right);
    
    glutSwapBuffers(); // double buffer
}




static void keyboard_func(unsigned char key, int /*x*/, int /*y*/)
{
    if(key == 'q' || key == 'Q'){
        current_state = Done;
        return;
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
    glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);

    glutMainLoop();
    return 0;
}
