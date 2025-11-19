// Graphics Pgm 5 for Caleb Bowen
// File: main.cpp

#include <cstdio>
#include <cmath>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "OpenGL445Setup-2025.h"
/*
Main Idea:
The program implements a small interactive 3D model builder using legacy OpenGL and GLUT.
The user selects either a cube or a sphere from two wireframe icons, and the chosen shape
appears at the center of the canvas at size 50. The shape remains still for one second
and then rotates at a constant rate of approximately 1.5 revolutions per second. All
placement and animation use MODELVIEW transforms (glTranslatef and glRotatef), never
absolute coordinates.

GLUT Callbacks:
display_func clears the canvas, draws the UI icons without lighting, then draws the
active shape with lighting enabled. mouse_func handles all left clicks: selecting icons,
changing size by plus or minus icons, or repositioning the shape by clicking elsewhere.
keyboard_func toggles orthographic and perspective viewing with P.timer_func runs at 24 Hz,
managing the still period and advancing the rotation angle once spinning begins.

Data and Global State:
current_shape (cube or sphere), current_state (ready or clicked), sx and sy for the
centroid, size in units (5 to 200), rotation_angle, spinning flag, still_frames counter
for the one second delay, and use_perspective for projection mode.

Rendering and Lighting:
Canvas is 800 by 800 with a black background. Icons are simple 2D wireframes drawn at z = -400.
The plus icon is green and the minus icon is yellow; all others are white. Shapes are solid
GLUT primitives rendered with fixed-function Phong/Blinn-Phong lighting.
A white point light is fixed at the world origin. Material is copper based on figma values
(198, 131, 70) with low ambient, strong diffuse, and moderate specular for a visible highlight.
Lighting is disabled during UI drawing and enabled only when rendering the 3D shape.

Viewing and Projection:
Default view is orthographic with x and y in [-400, 400] and z from 0 to -800.
The shape always lies in the plane z = -400. Pressing P switches to a perspective frustum
with the viewpoint at the origin looking down negative z, sized so the cross section at z = -400
matches the 800 by 800 view. Switching modes rebuilds the projection matrix and reloads MODELVIEW.

Transforms and Animation:
Shapes are positioned using glTranslatef(sx, sy, -400) and rotated about the vertical axis
using glRotatef(rotation_angle, 0, 1, 0). Rotation increments are uniform each frame to
maintain a constant 1.5 rps spin. The timer controls the still period and rotation updates,
and double buffering ensures smooth animation.
*/

// Canvas & Colors 
#define canvas_Width 800
#define canvas_Height 800
char canvas_Name[] = "CS 445 Shape Generator";


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
constexpr unsigned TIMER_PERIOD_MS = 1000 / 24; 

constexpr int FRAMES_PER_SECOND = 24;  
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
// Draws a 2D wireframe circle used for the UI icon. I at first just used a sphere but 
// was not sure if that was ok
static void draw2DCircle(GLfloat cx, GLfloat cy, GLfloat radius)
{
    const int SEGMENTS = 40; 

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < SEGMENTS; i++)
    {
        float theta = 2.0f * 3.1415926f * (float)i / (float)SEGMENTS;
        float x = radius * cosf(theta);
        float y = radius * sinf(theta);
        glVertex3f(cx + x, cy + y, -400.0f);
    }
    glEnd();
}
// Draws a 2D wireframe square centered at (x, y).
static void drawSquare(GLfloat x, GLfloat y ,GLfloat s){
  s=s/2;
  line2(x+s, y+s, x - s, y+s);
  line2(x+s, y-s, x - s, y-s);

  line2(x+s, y+s, x + s, y-s);
  line2(x-s, y+s, x - s, y-s);

}

// Renders the square selection icon in the top-left corner.
static void drawSquareIcon(){
  glColor3f(1.0f, 1.0f, 1.0f);
  drawSquare(-375, 375, 25);
  drawSquare(-375, 375, 15);
}
// Renders the sphere selection icon in the top-left corner.
static void drawCircleIcon(){
  glColor3f(1.0f, 1.0f, 1.0f);
  drawSquare(-345, 375, 25);
  draw2DCircle(-345, 375, 7.5f);
}
// Renders the green plus icon for increasing size.
static void drawPlusIcon(){
  glColor3f(1.0f, 1.0f, 1.0f);
  drawSquare(-375, -375, 25);

  glColor3f(0.0f, 1.0f, 0.0f);
  line2(-385, -375, -365, -375);
  line2(-375, -365, -375, -385);
}
// Renders the yellow minus icon for decreasing size.
static void drawMinusIcon(){
  glColor3f(1.0f, 1.0f, 1.0f);
  drawSquare(-345, -375, 25);
  glColor3f(1.0f, 1.0f, 0.0f);
  line2(-355, -375, -335, -375);
  
}

// Draws the lit, rotating solid cube at its world position.
static void drawCube(){
  glPushMatrix();
  glTranslatef(sx, sy, -400.0f);
  glRotatef(rotation_angle, 0.0f, 1.0f, 0.0f);
  glutSolidCube(size); 
  glPopMatrix();
}

// Draws the lit, rotating solid sphere at its world position.
static void drawSolidSphere(){
  glPushMatrix();
  glTranslatef(sx, sy, -400.0f);
  glRotatef(rotation_angle, 0.0f, 1.0f, 0.0f);
  glutSolidSphere(size / 2.0f, 30, 20);
  glPopMatrix();
}


// Returns true if a mouse click lies within an icon box.
bool insideIcon(float mx, float my, float x, float y, float size)
{
    return (mx >= x && mx <= x + size &&
            my >= y && my <= y + size);
}
// Handles all left-button clicks for selecting and moving shapes.
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


// Advances frame timing, stillness countdown, and rotation updates.
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
// Initializes fixed-function Phong lighting and copper material.
static void initLighting()
{
    // Enable depth testing so nearer fragments overwrite farther ones
    glEnable(GL_DEPTH_TEST);

    // Enable fixed-function lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);

    // White point light at the origin
    GLfloat light_pos[]     = { 0.0f, 0.0f, 0.0f, 1.0f };  
    GLfloat light_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    GLfloat light_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat light_spec[]    = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_spec);

    // Copper material (ambient small, strong diffuse, moderate specular)
   
    GLfloat copper_R = 198.0f / 255.0f;   //  0.776
    GLfloat copper_G = 131.0f / 255.0f;   //  0.514
    GLfloat copper_B =  70.0f / 255.0f;   //  0.274

    // Ambient darker version of copper
    GLfloat mat_ambient[] = {
        copper_R * 0.3f,
        copper_G * 0.3f,
        copper_B * 0.3f,
        1.0f
    };

    // Diffuse
    GLfloat mat_diffuse[] = {
        copper_R,
        copper_G,
        copper_B,
        1.0f
    };

    // Specular bright, slightly goldish highlight
    GLfloat mat_specular[] = {
        copper_R * 0.9f + 0.1f,
        copper_G * 0.9f + 0.1f,
        copper_B * 0.9f + 0.1f,
        1.0f
    };

    GLfloat mat_shine[] = { 45.0f };

    glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shine);
}


// Clears the canvas, draws UI, and renders the active shape.
static void display_func()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // 2D UI no lighting
    glDisable(GL_LIGHTING);

    drawSquareIcon();
    drawCircleIcon();
    drawPlusIcon();
    drawMinusIcon();

    // 3D shapes lighting ON 
    if (current_state == clicked) {
        glEnable(GL_LIGHTING);  // uses light & material from initLighting()

        if (current_shape == square) {
            drawCube();
        } else if (current_shape == sphere) {
            drawSolidSphere();
        }
    }

    glutSwapBuffers();
}
 // Builds either orthographic or perspective projection matrices.
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


// Handles P for projection toggle
static void keyboard_func(unsigned char key, int /*x*/, int /*y*/)
{
    if (key == 'p' || key == 'P') {
        use_perspective = !use_perspective;  // toggle mode
        setProjection();
        glutPostRedisplay();
        return;
    }
    glutPostRedisplay();
    
}


// Initializes GLUT, registers callbacks, and starts the main animation loop.
int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    my_setup(canvas_Width, canvas_Height, canvas_Name);
    setProjection();
    initLighting();
    
    glutDisplayFunc(display_func);
    glutKeyboardFunc(keyboard_func);
    glutMouseFunc(mouse_func);
    glutTimerFunc(TIMER_PERIOD_MS, timer_func, 0);

    glutMainLoop();
    return 0;
}
