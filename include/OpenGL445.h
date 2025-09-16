#pragma once
#include <GL/glew.h>
#include <GL/freeglut.h>
/******************************************************
OpenGL445Setup-2025.h

This header file contains initialization function calls and set-ups
for basic 3D CS 445/545 Open GL (Mesa) programs that use the GLUT/freeglut.
The initializations involve defining a callback handler (my_reshape_function)
that sets viewing parameters for orthographic 3D display.

TSN 2025 version – for OpenGL 4.3 w/legacy compatibility

******************************************************/

/* reshape callback handler - defines viewing parameters (projection) */

void my_3d_projection(int width, int height)
{
    GLdouble width_bound, height_bound;
    width_bound = (GLdouble) width; height_bound = (GLdouble) height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, width_bound, 0.0, height_bound, 0.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

#define STRT_X_POS 25
#define STRT_Y_POS 25

/* initialization routine */

void my_setup(int width, int height, char *window_name_str)
{
    // Allow for current OpenGL4.3 but backwards compatibility to legacy GL 2.1
    glutInitContextVersion(4, 3);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    // To get double buffering, uncomment the following line
    // glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    // below code line does single buffering - if above line is uncommented,
    // the single buffering line that follows will have to be commented out
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(STRT_X_POS, STRT_Y_POS);
    glutCreateWindow(window_name_str);
    glewExperimental = GL_TRUE;
    glewInit();

    glutReshapeFunc(my_3d_projection);
}
