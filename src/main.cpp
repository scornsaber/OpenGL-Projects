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




//  Animation Tuning 
constexpr unsigned TIMER_PERIOD_MS = 50; // 20 fps






//  Utility: draw a line 
static inline void line2(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    glBegin(GL_LINES);
    glVertex3f(x1, y1, -400);
    glVertex3f(x2, y2, -400);
    glEnd();
}


// Draws the octahedral fish body, scaling to specified size (Big or Small).



// Timer callback that updates the display periodically for animation.
static void timer_func(int /*value*/)
{
    //if(current_state == Done){
    //  return;
    //}
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

    
    
    glutSwapBuffers(); // double buffer
}



// Handles keyboard input (currently only Q/q to quit the simulation).
static void keyboard_func(unsigned char key, int /*x*/, int /*y*/)
{
    if(key == 'p' || key == 'P'){
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
