// Graphics Pgm 5 for Caleb Bowen
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
char canvas_Name[] = "CS 445 Shape Generator";

// Brand black (44,42,41) normalized
constexpr GLfloat BRAND_R = 44.0f / 255.0f;
constexpr GLfloat BRAND_G = 42.0f / 255.0f;
constexpr GLfloat BRAND_B = 41.0f / 255.0f;

// State enums

enum state{
  ready,
  clicked
};

enum shape{
  square,
  sphere
};

static shape current_shape = square;

static state current_state = ready;

static float rotation_angle = 0.0f;     // degrees
static bool  spinning = false;   // currently rotating or not

static GLfloat sx = 0.0f;
static GLfloat sy = 0.0f;

static bool use_perspective = false;  // start in orthographic


static int size = 50;
//  Animation Tuning 
constexpr unsigned TIMER_PERIOD_MS = 1000 / 24; // 24 ish fps

constexpr int FRAMES_PER_SECOND = 24;  // with 50 ms → 20
static int still_frames = 0;

constexpr float ANGLE_STEP = 540.0f / FRAMES_PER_SECOND;


//  Utility: draw a line 
static inline void line2(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    glBegin(GL_LINES);
    glVertex3f(x1, y1, -400);
    glVertex3f(x2, y2, -400);
    glEnd();
}

static void draw2DCircle(GLfloat cx, GLfloat cy, GLfloat radius)
{
    const int SEGMENTS = 40;  // Smooth enough for a small UI icon

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < SEGMENTS; i++)
    {
        float theta = 2.0f * 3.1415926f * (float)i / (float)SEGMENTS;
        float x = radius * cosf(theta);
        float y = radius * sinf(theta);
        glVertex3f(cx + x, cy + y, -400.0f);  // same z-plane as your UI
    }
    glEnd();
}

static void drawSquare(GLfloat x, GLfloat y ,GLfloat s){
  s=s/2;
  line2(x+s, y+s, x - s, y+s);
  line2(x+s, y-s, x - s, y-s);

  line2(x+s, y+s, x + s, y-s);
  line2(x-s, y+s, x - s, y-s);

}


static void drawSquareIcon(){
  drawSquare(-375, 375, 25);
  drawSquare(-375, 375, 15);
}

static void drawCircleIcon(){
  drawSquare(-345, 375, 25);
  draw2DCircle(-345, 375, 7.5f);
}

static void drawPlusIcon(){
  drawSquare(-375, -375, 25);
  line2(-385, -375, -365, -375);
  line2(-375, -365, -375, -385);
}

static void drawMinusIcon(){
  drawSquare(-345, -375, 25);
  line2(-355, -375, -335, -375);
  
}
static void drawCube(){
  glPushMatrix();
  glTranslatef(sx, sy, -400.0f);
  glRotatef(rotation_angle, 0.0f, 1.0f, 0.0f);
  glutSolidCube(size); 
  glPopMatrix();
}

static void drawSolidSphere(){
  glPushMatrix();
  glTranslatef(sx, sy, -400.0f);
  glRotatef(rotation_angle, 0.0f, 1.0f, 0.0f);
  glutSolidSphere(size / 2.0f, 30, 20);
  glPopMatrix();
}



bool insideIcon(float mx, float my, float x, float y, float size)
{
    return (mx >= x && mx <= x + size &&
            my >= y && my <= y + size);
}

void mouse_func(int button, int state, int mx, int my)
{
    // Only handle left-button press
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
        return;

    
    // Convert GLUT (origin top-left) OpenGL (origin bottom-left)
    int my_flipped = canvas_Height - my;

    float wx = mx - canvas_Width  / 2.0f;
    float wy = my_flipped - canvas_Height / 2.0f;

    // square
    if(insideIcon(wx, wy, -375 - 12.5f, 375 - 12.5f, 25)) { 
      current_shape = square;
      current_state = clicked;

      sx = 0.0;
      sy = 0.0;
      size = 50.0f;

       // reset rotation 1 second still, then spin
      rotation_angle = 0.0f;
      spinning       = false;
      still_frames   = FRAMES_PER_SECOND;
      return;
    }
    // sphere
    if(insideIcon(wx, wy, -345 - 12.5f, 375 - 12.5f, 25)) {
      current_shape = sphere;
      current_state = clicked;

      sx = 0.0;
      sy = 0.0;
      size = 50.0f;

      // reset rotation 1 second still, then spin
      rotation_angle = 0.0f;
      spinning       = false;
      still_frames   = FRAMES_PER_SECOND;
      return;
    }
    //plus
    if(insideIcon(wx, wy, -375 - 12.5f, -375 - 12.5f, 25)) {
      size+=5;
      if(size>200){size = 200;}
      return;
    }
    //minus
    if(insideIcon(wx, wy, -345 - 12.5f, -375 - 12.5f, 25)) {
      size-=5;
      if(size<5){size = 5;}
      return;
    }
    sx = wx;
    sy = wy;
    glutPostRedisplay();
}


// Timer callback that updates the display periodically for animation.
static void timer_func(int /*value*/)
{
    if (current_state == clicked) {
        if (still_frames > 0) {
            // still phase do nothing but count down
            --still_frames;
            if (still_frames == 0) {
                spinning = true;  // start rotating after 1 second
            }
        } else if (spinning) {
            // rotating phase
            rotation_angle += ANGLE_STEP;
            if (rotation_angle >= 360.0f) {
                rotation_angle -= 360.0f;
            }
        }
    }

    glutPostRedisplay();
    glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);
}



// Main rendering function that updates fish position, handles rotation, and redraws the scene.
static void display_func()
{
    /*if(current_state == Done){
      return;
    }*/
    glClearColor(BRAND_R, BRAND_G, BRAND_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawSquareIcon();
    drawCircleIcon();
    drawPlusIcon();
    drawMinusIcon();

    if(current_shape == square && current_state == clicked){
      
      drawCube();
      
    }
    else if(current_shape == sphere && current_state == clicked){
      
      drawSolidSphere();
      
    }
    
    glutSwapBuffers(); // double buffer
}

static void setProjection()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (!use_perspective) {
        // Orthographic view matching my_3d_projection:
        glOrtho(-400.0, 400.0,
                -400.0, 400.0,
                100.0, 900.0);
    } else {
        // At z=-400, edges are scaled by (400/100)=4 => x,y in [-400,400].
        glFrustum(-100.0, 100.0,
                  -100.0, 100.0,   
                  100.0, 900.0);   
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}



// Handles keyboard input (currently only Q/q to quit the simulation).
static void keyboard_func(unsigned char key, int /*x*/, int /*y*/)
{
    if (key == 'p' || key == 'P') {
        use_perspective = !use_perspective;  // toggle mode
        setProjection();
        glutPostRedisplay();
        return;
    }

    if (key == 'q' || key == 'Q') {
        std::exit(0);
    }

    glutPostRedisplay();
    
}


// Initializes GLUT, registers callbacks, and starts the main animation loop.
int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    my_setup(canvas_Width, canvas_Height, canvas_Name);
    setProjection();
    
    glutDisplayFunc(display_func);
    glutKeyboardFunc(keyboard_func);
    glutMouseFunc(mouse_func);
    glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);

    glutMainLoop();
    return 0;
}
