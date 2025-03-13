/*
 * frustum_view.c
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
 *	Module containing routines to handle the 'view' window.
 *	The 'view' window displays the view-frustum used in the 'model'
 *	window.
 */

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

#ifdef _WIN32
#include <float.h>
#define isnan _isnan
#else
#include <math.h>
#endif

#include "frustum.h"
#include "common.h"

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

/* Data view and model windows */
extern ViewContext *view;
extern ModelContext *model;

/* Stereo flags */
extern int stereoMode;
extern GLboolean hasStereo;

static int inMotionCallback = 0;	/* Prevents annoying GLUT warning message */

/*
 * viewInitWindow
 *
 *	Initializes the view window for use
 */

void
viewInitWindow(void)
{
static GLfloat ambient[3] = {0.1f, 0.1f, 0.1f};
static GLfloat diffuse[4] = {1.0f, 0.9f, 0.8f};
static GLfloat specular[3] = {1.0f, 1.0f, 1.0f};

    /* Initialize the model context data */
    view->scaleObj = 1.0;
    view->needToUpdateViewMat = 0;
    view->pointerMotion = 0;

    /* Set up windows to fit the available screen size */
    if (glutGet(GLUT_SCREEN_WIDTH) > 0)
	view->windowWidth = glutGet(GLUT_SCREEN_WIDTH) - 10;
    else
	view->windowWidth = 1270;
    if (glutGet(GLUT_SCREEN_HEIGHT) > 0)
	view->windowHeight = glutGet(GLUT_SCREEN_HEIGHT) * 4 / 9;
    else
	view->windowHeight = 485;
    view->minimumScale = MINIMUM_VIEW_SCALE;
    view->op = ROTATE;
    view->animateFlag = 1;

    glutInitWindowPosition(0, 0);
    glutInitWindowSize((GLint)view->windowWidth, (GLint)view->windowHeight);
    view->windowIdent = glutCreateWindow("View");

    /* Go do all of the projection computations */
    viewReshape((GLint)view->windowWidth, (GLint)view->windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glGetDoublev(GL_MODELVIEW_MATRIX, view->rotMat);	/* Initial rotation */

    /* Set the screen colors */
    glClearColor(0.0, 0.0, 0.0, 1.0);

    /* Set up for drawing solids */
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
    setLights(1);

    /* Specify all of the callback routines */
    glutDisplayFunc(display);
    glutReshapeFunc(viewReshape);
    glutKeyboardFunc(viewKeys);
    glutSpecialFunc(viewSpecialKeys);
    glutMouseFunc(viewMouse);
    glutMotionFunc(viewMotion);
    glutVisibilityFunc(viewVisibility);

    /* Create a menu */
    /* Objects submenu */
    model->objectMenuIdent = glutCreateMenu(modelMenu);
    glutAddMenuEntry("Shuttle", DRAW_SHUTTLE);
 	glutAddMenuEntry("X29", DRAW_X29);	
    glutAddMenuEntry("Porsche", DRAW_PORSCHE);

    model->menuIdent = glutCreateMenu(modelMenu);
    glutAddSubMenu("Objects", model->objectMenuIdent);

    glutAddMenuEntry("Enable Stereo Mode", STEREO_ON);
    model->stereoMenuNum = glutGet(GLUT_MENU_NUM_ITEMS);

    glutAddMenuEntry("Quit", QUIT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    /* Create the eyeball display list */
    view->eyeball = gluNewQuadric();
    gluQuadricDrawStyle(view->eyeball, GLU_FILL);
    gluQuadricNormals(view->eyeball, GLU_SMOOTH);
    view->eyeballListNum = glGenLists(1);
    glNewList(view->eyeballListNum, GL_COMPILE);
	setColor(ambient, diffuse, specular, 50.0, 0);
	gluSphere(view->eyeball, 0.2, 10, 10);
    glEndList();
} /* End of viewInitWindow */



/*
 * viewDisplay
 *
 *	The display loop code
 */

void
viewDisplay(void)
{
    if ((stereoMode == MONO) || model->stereoSetting == STEREO_OFF) {
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	/* Look at the model from the defined position */
	gluLookAt(CAMERA_BACK, 0.0, 0.0,
	    0.0, 0.0, 0.0,
	    0.0, 1.0, 0.0);

	viewSetView();
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	viewDrawImage();
    }
    else {

	/* Draw the left view */

	if (hasStereo) {
	    glDrawBuffer(GL_BACK_LEFT);
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	else {
	    /* Clear everything the first time */
	    stereoMode = LEFT;		/* Left is red */
	    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	    glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	/* Look at the model from the defined position */
	gluLookAt(CAMERA_BACK, 0.0, 1.25,
	    0.0, 0.0, 0.0,
	    0.0, 1.0, 0.0);

	viewSetView();
	viewDrawImage();

	/* Draw the right view */

	if (hasStereo) {
	    glDrawBuffer(GL_BACK_RIGHT);
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	else {
	    /* Just clear blue this time */
	    stereoMode = RIGHT;		/* Right is blue */
	    glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	/* Look at the model from the defined position */
	gluLookAt(CAMERA_BACK, 0.0, -1.25,
	    0.0, 0.0, 0.0,
	    0.0, 1.0, 0.0);

	viewSetView();
	viewDrawImage();

	/* Put the mask back to their original value */
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    glutSwapBuffers();

    view->pointerMotion = 0;			/* We just redrew */
} /* End of viewDisplay */



/*
 * viewDrawImage
 *
 */

void
viewDrawImage(void)
{
    register int i, j;
    register int nextIndex;		/* Next index that has a color */
    GLdouble newLeft, newRight, newBottom, newTop;
    GLdouble screenLeft, screenRight, screenBottom, screenTop;
    GLdouble zRatio;
/* Some default colors for models with no color */
static GLfloat ambient[3] = {0.1f, 0.05f, 0.2f};
static GLfloat diffuse[4] = {0.3f, 0.0f, 0.8f, 1.0f};
static GLfloat specular[3] = {1.0f, 1.0f, 1.0f};
/* Colors for drawing the frustum */
static GLfloat backColor[4] = {0.1f, 0.2f, 1.0f, 0.2f};
static GLfloat cornerColor[4] = {0.25f, 0.5f, 0.75f, 1.0f};
static GLfloat frontColor[4] = {0.2f, 1.0f, 0.1f, 0.2f};
static GLfloat screenColor[4] = {1.0f, 0.2f, 1.0f, 1.0f};
    GLboolean RBstereo;			/* Flag for red/blue stereo */

    RBstereo = !hasStereo && (stereoMode != MONO);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    /* Set up the view window's scale and rotation */
    glScaled(view->scaleObj, view->scaleObj, view->scaleObj);

    if (view->needToUpdateViewMat) {
	/* Build a proper cumulative rotation for automatic motion */
	glPushMatrix();
	glLoadIdentity();
	glRotated(view->rotX * 0.2, 0.0, 1.0, 0.0);
	glRotated(-view->rotY * 0.2, 0.0, 0.0, 1.0);
	glMultMatrixd(view->rotMat);
	glGetDoublev(GL_MODELVIEW_MATRIX, view->rotMat);
	glPopMatrix();
	view->needToUpdateViewMat = 0;
    }
    glMultMatrixd(view->rotMat);

    /* Move the origin so that it is in the middle of the system */
    /* The number '4.0' is a magic-fudge-factor */
    glTranslatef(0.0, 0.0, 4.0-(model->eyeballZ / 2.0));

    /* Prepare data for the back clipping plane and viewlines */
    zRatio = (model->backPlaneZ - model->eyeballZ) /
	(model->frontPlaneZ - model->eyeballZ);
    newLeft = model->left * zRatio;
    newRight = model->right * zRatio;
    newBottom = model->bottom * zRatio;
    newTop = model->top * zRatio;

    zRatio = (model->screenZ - model->eyeballZ) /
	(model->frontPlaneZ - model->eyeballZ);
    screenLeft = model->left * zRatio;
    screenRight = model->right * zRatio;
    screenBottom = model->bottom * zRatio;
    screenTop = model->top * zRatio;

    /* Set up the model's scale and rotation */
    glPushMatrix();
    if (model->needToUpdateViewMat) {
	/* Build a proper cumulative rotation for automatic motion */
	glPushMatrix();
	glLoadIdentity();
	glRotated(model->rotX * 0.2, 0.0, 1.0, 0.0);
	glRotated(-model->rotY * 0.2, 0.0, 0.0, 1.0);
	glMultMatrixd(model->rotMat);
	glGetDoublev(GL_MODELVIEW_MATRIX, model->rotMat);
	glPopMatrix();
	model->needToUpdateViewMat = 0;
    }
    glMultMatrixd(model->rotMat);

    glScaled(model->scaleObj, model->scaleObj, model->scaleObj);

    /* Correctly orient the object */
    glRotated(model->rotAngle, model->rotAxis[0],
	model->rotAxis[1], model->rotAxis[2]);

    /* Now draw the object */
    i = 0;
    j = 0;
    /* Set a default color, in case there isn't a model color */
    setColor(ambient, diffuse, specular, 50.0, 0);
    nextIndex = model->colorList[j].index;
    {
	if (model->haveNormals) {
	    do {
		if (i == nextIndex) {
		    setColor(&model->colorList[j].ra, &model->colorList[j].rd,
			&model->colorList[j].rs, model->colorList[j].spec,
			RBstereo);
		    j++;
		    nextIndex = model->colorList[j].index;
		}
		glBegin(GL_TRIANGLE_STRIP);
		    glNormal3fv(&model->vertexList[i].nx);
		    glVertex3fv(&model->vertexList[i++].x);
		    while (model->vertexList[i].draw) {
			glNormal3fv(&model->vertexList[i].nx);
			glVertex3fv(&model->vertexList[i++].x);
		    }
		glEnd();
	    } while (i < model->vertexCount);
	}
	else do {
	    if (i == nextIndex) {
		setColor(&model->colorList[j].ra, &model->colorList[j].rd,
		    &model->colorList[j].rs, model->colorList[j].spec,
		    RBstereo);
		j++;
		nextIndex = model->colorList[j].index;
	    }
	    glBegin(GL_TRIANGLE_STRIP);
		glVertex3fv(&model->vertexList[i++].x);
		while (model->vertexList[i].draw) {
		    glVertex3fv(&model->vertexList[i++].x);
		}
	    glEnd();
	} while (i < model->vertexCount);
    }
    glPopMatrix();

    /* Draw back clipping plane */
    setColor(backColor, backColor, backColor, 1.0, RBstereo);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glBegin(GL_QUADS);
	glVertex3d(newLeft * 2.0, newBottom * 2.0, model->backPlaneZ);
	glVertex3d(newRight * 2.0, newBottom * 2.0, model->backPlaneZ);
	glVertex3d(newRight * 2.0, newTop * 2.0, model->backPlaneZ);
	glVertex3d(newLeft * 2.0, newTop * 2.0, model->backPlaneZ);
    glEnd();

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);

    /* Put in the eyeball */
    glPushMatrix();
    glTranslatef(0.0, 0.0, model->eyeballZ);
    glCallList(view->eyeballListNum);
    glPopMatrix();

    glPushMatrix();
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);

    /* Put in front clipping plane */
    setColor(frontColor, frontColor, frontColor, 1.0, RBstereo);
    glDepthMask(GL_FALSE);
    glBegin(GL_QUADS);
	glVertex3d(model->left * 2.0, model->bottom * 2.0, model->frontPlaneZ);
	glVertex3d(model->right * 2.0, model->bottom * 2.0, model->frontPlaneZ);
	glVertex3d(model->right * 2.0, model->top * 2.0, model->frontPlaneZ);
	glVertex3d(model->left * 2.0, model->top * 2.0, model->frontPlaneZ);
    glEnd();

    /* Put in the viewlines */
    glEnable(GL_LINE_SMOOTH);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBegin(GL_LINES);
	setColor(cornerColor, cornerColor, cornerColor, 1.0, RBstereo);
	glVertex3d(0.0, 0.0, model->eyeballZ);
	glVertex3d(newLeft, newBottom, model->backPlaneZ);
	glVertex3d(0.0, 0.0, model->eyeballZ);
	glVertex3d(newLeft, newTop, model->backPlaneZ);
	glVertex3d(0.0, 0.0, model->eyeballZ);
	glVertex3d(newRight, newBottom, model->backPlaneZ);
	glVertex3d(0.0, 0.0, model->eyeballZ);
	glVertex3d(newRight, newTop, model->backPlaneZ);
    glEnd();
    glBegin(GL_LINE_STRIP);
	setColor(frontColor, frontColor, frontColor, 1.0, RBstereo);
	glVertex3d(model->left, model->bottom, model->frontPlaneZ);
	glVertex3d(model->left, model->top, model->frontPlaneZ);
	glVertex3d(model->right, model->top, model->frontPlaneZ);
	glVertex3d(model->right, model->bottom, model->frontPlaneZ);
	glVertex3d(model->left, model->bottom, model->frontPlaneZ);
    glEnd();
    glBegin(GL_LINE_STRIP);
	setColor(backColor, backColor, backColor, 1.0, RBstereo);
	glVertex3d(newLeft, newBottom, model->backPlaneZ);
	glVertex3d(newLeft, newTop, model->backPlaneZ);
	glVertex3d(newRight, newTop, model->backPlaneZ);
	glVertex3d(newRight, newBottom, model->backPlaneZ);
	glVertex3d(newLeft, newBottom, model->backPlaneZ);
    glEnd();
    glBegin(GL_LINE_STRIP);
	setColor(screenColor, screenColor, screenColor, 1.0, RBstereo);
	glVertex3d(screenLeft, screenBottom, model->screenZ);
	glVertex3d(screenLeft, screenTop, model->screenZ);
	glVertex3d(screenRight, screenTop, model->screenZ);
	glVertex3d(screenRight, screenBottom, model->screenZ);
	glVertex3d(screenLeft, screenBottom, model->screenZ);
    glEnd();
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    glEnable(GL_LIGHTING);
    glPopMatrix();

    glPopMatrix();
} /* End of viewDrawImage */



/*
 * viewMouse
 *
 *	Deal with the mouse.  The menu button (right) never
 *	gets this far.
 */
static int oldButton, oldState;		/* For motion routine */

void
viewMouse(int button,
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
    if (!inMotionCallback) {
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

		view->rotX = (GLdouble)(x - xOrigin) * 0.2;
		view->rotY = (GLdouble)(y - yOrigin) * 0.2;

		/* Build a proper cumulative rotation */
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glRotated(view->rotX, 0.0, 1.0, 0.0);
		glRotated(-view->rotY,  0.0, 0.0, 1.0);
		glMultMatrixd(view->rotMat);
		glGetDoublev(GL_MODELVIEW_MATRIX, view->rotMat);
		glPopMatrix();

		if (view->pointerMotion == 0) {
		    xhPtr = (xhPtr + 1) & 0x0f;
		    xhCount++;
		    view->pointerMotion = 1;	/* For > 1 move/redraw */
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
	    view->animateFlag = 0;	/* No automatic motion until release */
	    ld = 1;			/* Button is down */
	}
	else {
	    /* Only choice here is: button just came up */
	    getTime(newTime);
	    milliseconds = timeDiff(oldTime, newTime);

	    if (milliseconds > 500000) {	/* Over 1/2 second? */
		/* Stop the motion */

		view->animateFlag = 0;
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
		    view->rotX = (GLdouble)xSum * 0.1;
		    view->rotY = (GLdouble)ySum * 0.1;
		    view->animateFlag = 1;
		}
		else {
		    /* Insufficient history, stop! */
		    view->rotX = 0.0;
		    view->rotY = 0.0;
		    view->animateFlag = 0;
		}
	    }
	    ld = 0;
	}
	break;

      case GLUT_MIDDLE_BUTTON:
	if (state == GLUT_DOWN) {
	    if ((md) && (!isnan(y)))
		view->scaleObj *= (GLdouble)(y - yOrigin) * SCALE_SPEED + 1.0;

	    if (view->scaleObj < view->minimumScale)
		view->scaleObj = view->minimumScale;
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

} /* End of viewMouse */



/*
 * viewSpecialKeys
 *
 *	Deal with arrow keys and such
 */

/*ARGSUSED1*/
void
viewSpecialKeys(int key,
    int x,
    int y)
{
    switch (key) {

      case GLUT_KEY_LEFT:
	glTranslated(-0.05 * view->scaleObj, 0.0, 0.0);
	glutPostRedisplay();
	break;

      case GLUT_KEY_RIGHT:
	glTranslated(0.05 * view->scaleObj, 0.0, 0.0);
	glutPostRedisplay();
	break;

      case GLUT_KEY_UP:
	glTranslated(0.0, 0.05 * view->scaleObj, 0.0);
	glutPostRedisplay();
	break;

      case GLUT_KEY_DOWN:
	glTranslated(0.0, -0.05 * view->scaleObj, 0.0);
	glutPostRedisplay();
	break;

    } /* End of switch on special key */
} /* End of viewSpecialKeys */



/*
 * viewKeys
 *
 *	Routine to deal with the keyboard
 */

void
viewKeys(unsigned char key,
    int x,
    int y)
{
    switch (key) {

      case '+':
      case '=':
	view->scaleObj *= 1.05;
	break;

      case '-':
      case '_':
	view->scaleObj *= 0.95;
	break;

      default:
	/* Send the keypress to the modelkeys handler */
	modelKeys(key, x, y);

    } /* End of switch on key */

    glutPostRedisplay();
} /* End of viewKeys */



/*
 * viewMotion
 *
 *	No changes to mouse button, but it is moving with at least
 *	one button down.
 */

void
viewMotion(int x,
    int y)
{
	inMotionCallback = 1;
    viewMouse(oldButton, oldState, x, y);
	inMotionCallback = 0;
} /* End of viewMotion */



/*
 * viewVisibility
 *
 *	The visibility of the window has changed.
 */

void
viewVisibility(int state)
{
    if (state == GLUT_NOT_VISIBLE) {
	view->animateFlag = 0;
	if (model->animateFlag == 0)
	    glutIdleFunc(NULL);
    }
    else {
	view->animateFlag = 1;
	glutIdleFunc(animate);
    }
} /* End of viewVisibility */



/*
 * viewReshape
 *
 *	Change the size and shape of the window.  Use correct math to
 *	keep relative sizes now matter what the window is shaped like.
 */

void
viewReshape(int w,
    int h)
{
    view->windowWidth = (GLdouble)w;
    view->windowHeight = (GLdouble)h;

    /* Notify OpenGL as to where we want to draw this */
    glViewport(0, 0, (GLint)view->windowWidth, (GLint)view->windowHeight);
} /* End of viewReshape */




/*
 * viewSetView
 *
 *	Set up the proper view.
 */

void
viewSetView(void)
{
    GLdouble distanceAdjust;    /* Front plane is NOT at screen */

    /* Compute a correct projection matrix */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    /* Compute adjustment factor */
    /* Ratio: front plane to screen */
    distanceAdjust = FRONT_PLANE / CAMERA_BACK;

    /* Convert to measuring in inches, then adjust for center to edge */
    distanceAdjust /= PIXELS_PER_INCH / FRONT_PLANE;

    glFrustum(-view->windowWidth * distanceAdjust,
	view->windowWidth * distanceAdjust, -view->windowHeight *
	distanceAdjust, view->windowHeight * distanceAdjust,
	FRONT_PLANE, 1000.0);		/* Front clip at (2), back at 1000 */
} /* End of viewSetView */

/* End of frustum_view.c */
