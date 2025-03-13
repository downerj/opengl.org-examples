/*
 * callbacks.c
 *
 *	Written by Scott R. Nelson and Greg Taylor of Sun Microsystems, Inc.
 *
 * Copyright (c) 1998, Sun Microsystems, Inc.
 *		All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Sun Microsystems, Inc. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * 	Generic callbacks module for demo programs.
 */

#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <float.h>
#define isnan _isnan
#else
#include <math.h>
#endif
#include <GL/glut.h>

#include "modelcontext.h"
#include "callbacks.h"

/* For platform-independent timing */
#ifdef _WIN32
    #include <windows.h>
    typedef int timer;
    #define getTime(a)      (a = GetTickCount())
    #define timeDiff(a, b)  ((b - a) * 1000)
#else
    #include <sys/time.h>
    typedef struct timeval timer;
    #define getTime(a)      gettimeofday(&a, NULL)
    #define timeDiff(a, b)  (((b.tv_sec - a.tv_sec) * 1000000) + b.tv_usec - a.tv_usec)
#endif

extern ModelContext *model;

/*
 * mouseCallback
 *
 *	Deal with the mouse.  The menu button (right) never
 *	gets this far.
 */
static int oldButton, oldState;		/* For motion routine */
static int inMotionCallback = 0;	/* Prevents annoying GLUT warning message */
void
mouseCallback(int button, 
    int state, 
    int x, 
    int y)
{
    static timer oldTime, newTime;
    int milliseconds;
    static int xHist[16], yHist[16];
    static int xhPtr = 0;
    static int xhCount = 0;
    static int xOrigin, yOrigin;
    static int ld = 0, md = 0;			/* Left/middle down */
    int i;
    int histCount;
    int xSum, ySum;

    /* hack for 2 button mouse */
	if (!inMotionCallback)
	{
		if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
		button = GLUT_MIDDLE_BUTTON;
	}

	/* Save settings so "motion" can call this */
	oldButton = button;
	oldState = state;

    switch (button) {

      case GLUT_LEFT_BUTTON:
	if (state == GLUT_DOWN) {
	    if (ld == 0) {
		/* Button just came down */

		/* Clear the history log */
		for (i = 0; i < 16; i++) {
		    xHist[i] = 0;
		    yHist[i] = 0;
		}
		xhPtr = 0;
		xhCount = 0;
	    }
	    else {
		/* Button was down before, this is a drag operation */

		model->rotX = (GLdouble)(x - xOrigin) * 0.5;
		model->rotY = (GLdouble)(y - yOrigin) * 0.5;

		/* Build a proper cumulative rotation */
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glRotated(model->rotX, 0.0, 1.0, 0.0);
		glRotated(model->rotY, 1.0, 0.0, 0.0);
		glMultMatrixd(model->rotMat);
		glGetDoublev(GL_MODELVIEW_MATRIX, model->rotMat);
		glPopMatrix();

		if (model->pointerMotion == 0) {
		    xhPtr = (xhPtr + 1) & 0x0f;
		    xhCount++;
		    model->pointerMotion = 1;	/* For > 1 move/redraw */
		    xHist[xhPtr] = 0;
		    yHist[xhPtr] = 0;
		}
		xHist[xhPtr] += x - xOrigin;
		yHist[xhPtr] += y - yOrigin;

		glutPostRedisplay();	/* Force redraw */
	    }

	    getTime(oldTime); /* Remember time */
	    xOrigin = x;		/* Remember position */
	    yOrigin = y;
	    glutIdleFunc(NULL);		/* No automatic motion until release */
	    ld = 1;			/* Button is down */
	}
	else {
	    /* Only choice here is: button just came up */
	    getTime(newTime);
	    milliseconds = timeDiff(oldTime, newTime);

	    if (milliseconds > 500000) {	/* Over 1/2 second? */
		/* Stop the motion */

		glutIdleFunc(NULL);
		xhPtr = 0;
		xhCount = 0;
	    }
	    else {
		/* Get an average */

		if (xhCount >= 4) {
		    xhPtr += 16;	/* Handle wrap around */
		    xSum = 0;
		    ySum = 0;
		    histCount = xhCount;
		    if (histCount > 6)
			histCount = 6;	/* Only look at the last few */
		    for (i = 1; i < histCount; i++) {
			xSum += xHist[(xhPtr - i) & 0x0f];
			ySum += yHist[(xhPtr - i) & 0x0f];
		    }
		    model->rotX = (GLdouble)xSum * 0.1;
		    model->rotY = (GLdouble)ySum * 0.1;
		    glutIdleFunc(animateCallback);
		}
		else {
		    /* Insufficient history, stop! */
		    model->rotX = 0.0;
		    model->rotY = 0.0;
		    glutIdleFunc(NULL);
		}
	    }
	    ld = 0;
	}
	break;

      case GLUT_MIDDLE_BUTTON:
	if (state == GLUT_DOWN) {
	    if ((md) && (!isnan(y)))
		model->scaleObj *= (GLdouble)(y - yOrigin) * SCALE_SPEED + 1.0;

	    if (model->scaleObj < model->minimumScale)
		model->scaleObj = model->minimumScale;
	    xOrigin = x;		/* Remember position */
	    yOrigin = y;
	    md = 1;
	}
	else
	    md = 0;
	glutPostRedisplay();
	break;

      /* Right button is caught by the menu handler */

    } /* End of switch on button */

} /* End of mouseCallback */



/*
 * specialKeysCallback
 *
 *	Deal with arrow keys and such
 */

/*ARGSUSED1*/
void
specialKeysCallback(int key,
    int x,
    int y)
{
    switch (key) {

      case GLUT_KEY_LEFT:
	glTranslated(-0.05 * model->scaleObj, 0.0, 0.0);
	glutPostRedisplay();
	break;

      case GLUT_KEY_RIGHT:
	glTranslated(0.05 * model->scaleObj, 0.0, 0.0);
	glutPostRedisplay();
	break;

      case GLUT_KEY_UP:
	glTranslated(0.0, 0.05 * model->scaleObj, 0.0);
	glutPostRedisplay();
	break;

      case GLUT_KEY_DOWN:
	glTranslated(0.0, -0.05 * model->scaleObj, 0.0);
	glutPostRedisplay();
	break;

    } /* End of switch on special key */
} /* End of specialKeysCallback */



/*
 * motionCallback
 *
 *	No changes to mouse button, but it is moving with at least
 *	one button down.
 */

void
motionCallback(int x, 
    int y)
{
	inMotionCallback = 1;
    mouseCallback(oldButton, oldState, x, y);
	inMotionCallback = 0;
} /* End of motionCallback */



/*
 * animateCallback
 *
 *	The animation function.
 */

void
animateCallback(void)
{
    model->needToUpdateViewMat = 1;
    glutPostRedisplay();
} /* End of animateCallback */



/*
 * visibilityCallback
 *
 *      The visibility of the window has changed.
 */

void
visibilityCallback(int state)
{
    if (state == GLUT_VISIBLE)
	glutIdleFunc(animateCallback);
    else if (state == GLUT_NOT_VISIBLE)
	glutIdleFunc(NULL);
} /* End of visibilityCallback */



/*
 * reshapeCallback
 *
 *	Change the size and shape of the window.  Use correct math to
 *	keep relative sizes now matter what the window is shaped like.
 */

void
reshapeCallback(int w,
    int h)
{
    model->windowWidth = (GLdouble)w;
    model->windowHeight = (GLdouble)h;

    /* Notify OpenGL as to where we want to draw this */
    glViewport(0, 0, (GLint)model->windowWidth, (GLint)model->windowHeight);
} /* End of reshapeCallback */

/* End of callbacks.c */
