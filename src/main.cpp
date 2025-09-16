
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "OpenGL445.h"

#include <cstdio>
#include <cmath>

// CS 445/545 OpenGL Program skeleton w/line drawing done in
// display callback display_func() : skeleton-line.cpp
// Author: T. Newman

void display_func()
{
    // this is a display callback to draw a line.
    // learner’s note: display callbacks are automatically invoked after GLUT finds that
    // a window needs to be
    // displayed or redisplayed

    glClearColor(1.0, 1.0, 1.0,1.0);  
    glClear(GL_COLOR_BUFFER_BIT);

    // draw a short green line
    glColor3f(0.0, 1.0, 0.0);
    glBegin(GL_LINES);
        glVertex2i(25, 25);
        glVertex2i(50, 75);
    glEnd();
    glFlush(); // may need to uncomment this line!
}

// can customize the below 3 items to make canvas of one’s own size and labeling

#define canvas_Width 100
#define canvas_Height 100
char canvas_Name[] = "NAME OF CANVAS";

// #define canvas_Name "NAME OF CANVAS"

int main(int argc, char ** argv)
{
    glutInit(&argc, argv);
    my_setup(canvas_Width, canvas_Height, canvas_Name);

    glutDisplayFunc(display_func);

    glutMainLoop();
    return 0;
}
