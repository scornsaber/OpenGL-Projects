// Graphics Pgm 4 for Caleb Bowen
// File: main.cpp

#include <cstdio>
#include <cmath>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "OpenGL445Setup-2025.h"
/*
Extra credit for doubling rotation rate completed. Input 'd' to double 'u' to undo
Main Idea:
Render an animated aquarium scene in orthographic projection where a large orange fish 
continuously swims between the tank walls, rotating 180 each time it reaches an edge, 
and a smaller yellow fish and green teapot serve as static decorations. All motion and 
transformations are handled using glTranslatef, glScalef, and glRotatef rather than absolute world coordinates.

GLUT Callbacks:
display_func: Clears the canvas each frame, updates position and rotation states,
 and redraws the entire tank. Draws the large fish, small fish, and teapot using current global state.
timer_func: Drives animation at fixed intervals,
 re-arming itself each tick to maintain continuous motion and trigger redisplay.
keyboard_func: Handles quit control (Q / q) to terminate the simulation cleanly.
  Input 'd' to double rotation rate 'u' to undo.

Data and Global State:
current_direction: {Left, Right} — determines swim direction.
current_state: {Swimming, Rotating, Done} — manages behavior phase.
fx, fy, fz: position of the large fish torso center (in z = −400 plane).
rotate_accum: cumulative rotation angle (0 <--> 180) for smooth turning animation.
Color constants use PANTONE references for realism: Spun Sugar (background water), 
Tangerine Tango (large fish), Spicy Mustard (small fish), Hunter Green (teapot).

Rendering and Geometry:
drawFishBody scales the GLUT wireframe octahedron to represent the fish torso 
(150x50x25 units for large fish, 50x20x10 for small).
drawFishTail draws a triangular wireframe tail attached at the body’s tip.
drawLargeFish / drawSmallFish compose the full fish model by translating to world 
position and drawing the body and tail in correct orientation.
drawTeapot positions a 50-unit Hunter Green wireframe teapot at (75, -400, -400).
The view volume spans x,y  [-400, 400] and z  [-900, -100], camera at origin facing -z.

Animation Logic:
During each frame, Turn() checks if the large fish’s nose approaches a wall (|x| > 396 - half-width).
 If so, current_state switches to Rotating. The fish then rotates +=5 per frame about its vertical (y)
  axis through its center until rotate_accum reaches its target (0 or 180). Once complete, the state 
  returns to Swimming, and linear translation (+=5 units per frame) resumes in the opposite direction. 
  The loop continues indefinitely until user exit.

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
constexpr unsigned TIMER_PERIOD_MS = 50; // 20 fps

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
  Done = 1,
  Rotating = 2
};


static Direction current_direction = Left;
static State current_state = Swimming;

static float rotate_accum = 180.0f;

static GLfloat fx = 0.0f;
static GLfloat fy = 0.0f;
static GLfloat change_x = 5.0f;
static GLfloat change_deg = 5.0f;
static constexpr GLfloat fz = -400.0f;
//  Utility: draw a line 
static inline void line2(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    glBegin(GL_LINES);
    glVertex3f(x1, y1, -400);
    glVertex3f(x2, y2, -400);
    glEnd();
}
// Draws a green wireframe teapot centered at (cx, cy, -400).
static void drawTeapot(GLfloat cx, GLfloat cy)
{
    glLoadIdentity();
    glTranslatef(cx, cy, -400.0f);
    
    glutWireTeapot(50);
    glLoadIdentity();
}

// Draws the octahedral fish body, scaling to specified size (Big or Small).
static void drawFishBody(Size s)
{   
    GLfloat w = (s == Big) ? 75.0f: 25.0f;
    GLfloat h = (s == Big) ? 25.0f: 10.0f;
    GLfloat d = (s == Big) ? 12.5f: 5.0f;
    //glLoadIdentity();
    //glTranslatef(x, y, z);
    glScalef(w, h, d);
    glutWireOctahedron();
    glScalef(1/w, 1/h, 1/d);
    //glLoadIdentity();
}
// Draws a triangular wireframe tail attached to the fish body.
static void drawFishTail(GLfloat x, GLfloat y, Size s, Direction di)
{
    const GLfloat w  = (s == Small) ? 25.0f : 75.0f;  // half body width
    const GLfloat th = (s == Small) ? 3.5f  : 10.0f;  // tail half-height
    const GLfloat tx = (s == Small) ? 7.0f  : 20.0f;  // tail length

    
    glBegin(GL_LINE_LOOP);
    glVertex3f(-w,      0.0f,  0.0f);
    glVertex3f(-w - tx, -th,   0.0f);
    glVertex3f(-w - tx,  th,   0.0f);
    glEnd();
    
}
// Draws the small stationary fish using its body and tail at a fixed position.
static void drawSmallFish(GLfloat x, GLfloat y, GLfloat z, Direction d) {
  glPushMatrix();
  glTranslatef(x, y, z);
  glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
  drawFishBody(Small);
  //glLoadIdentity();
  drawFishTail(x, y, Small, d);
  glPopMatrix();

}
// Draws the large moving fish, applying translation and rotation for animation.
static void drawLargeFish(GLfloat x, GLfloat y, GLfloat z, Direction d, GLfloat rot) {
  glPushMatrix();
  glTranslatef(x, y, z);
  glRotatef(rot, 0.0f, 1.0f, 0.0f);
  drawFishBody(Big);
  //glLoadIdentity();
  //glRotatef(rot, 0.0f, 1.0f, 0.0f);
  drawFishTail(x, y, Big, d);
  glPopMatrix();
}


// Detects wall proximity and triggers a turning state when the fish nears an edge.
static void Turn(){
    if(current_state != Swimming) return;
    if(fx+75 >= 396.0f || fx-75 <= -396.0f){
      if(current_direction == Right){
        current_direction = Left;
      }
      else{
        current_direction = Right;
      }
      current_state = Rotating;
    }
}



// Timer callback that updates the display periodically for animation.
static void timer_func(int /*value*/)
{
    if(current_state == Done){
      return;
    }
    glutPostRedisplay();
    glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);
}



// Main rendering function that updates fish position, handles rotation, and redraws the scene.
static void display_func()
{
    if(current_state == Done){
      return;
    }
    glClearColor(BRAND_R, BRAND_G, BRAND_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    
    glLoadIdentity();

    // I had to adjust teapot position to make it be on the screen correctly
    glColor3f(GREEN_R, GREEN_G, GREEN_B);
    drawTeapot(75.0f, -360.0f);

    glColor3f(ORANGE_R, ORANGE_G, ORANGE_B);
    Turn();
    if (current_state == Rotating) {
      const float target = (current_direction == Right) ? 0.0f : 180.0f;

      if (fabsf(rotate_accum - target) <= 0.01f) {
          rotate_accum = target;
          current_state = Swimming;
          fx += (current_direction == Right) ? change_x : -change_x;  // nudge off wall
      } else {
          rotate_accum += (rotate_accum < target) ? change_deg : -change_deg; 
      }
    } else {
      fx += (current_direction == Right) ? change_x : -change_x;
    }
    drawLargeFish(fx, fy, fz, current_direction, rotate_accum);
    glColor3f(SPICY_R, SPICY_G, SPICY_B);
    drawSmallFish(-325.0f, -350.0f, -400.0f, Left);
    
    glutSwapBuffers(); // double buffer
}



// Handles keyboard input (currently only Q/q to quit the simulation).
static void keyboard_func(unsigned char key, int /*x*/, int /*y*/)
{
    if(key == 'q' || key == 'Q'){
        current_state = Done;
        return;
    }
    if(key == 'd' || key == 'D'){
        change_deg = 10.0f;
        return;
    }
    if(key == 'u' || key == 'U'){
        change_deg = 5.0f;
        return;
    }
    glutPostRedisplay();
    
}


// Initializes GLUT, registers callbacks, and starts the main animation loop.
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
