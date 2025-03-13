/*
 * shadows.c
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
 *	Read in various Wavefront .OBJ models.  Allow them to be rotated
 *	and set various draw modes while the object moves.  Cast shadows
 *      using several different methods.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>
#ifdef _WIN32

#include <float.h>
#ifndef M_PI
#define M_PI	3.1415926535897932384626433832795f
#define isnan _isnan
#endif
#endif

#include "modelcontext.h"
#include "callbacks.h"
#include "common.h"
#include "fileread.h"

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

/* The data-set for the model */
ModelContext *model;

/* Drawing State Flags */
#define DRAW_SOLID		0x0001
#define DRAW_WIREFRAME		0x0002
static int drawState = DRAW_SOLID;

/* Shadow constants (heights above the floor to place model and shadow) */
#define FLOOR_DIST_FROM_MODEL 0.0
#define SHADOW_ABOVE_FLOOR 0.001

/* Light definitions */
static GLfloat light1[4] = {1.0f, 1.0f, 0.9f, 1.0f};
static GLfloat pos1[4] = { 5.0f,  7.0f, 1.0f, 0.0f};
static GLfloat ambient[3] = {0.2f, 0.2f, 0.2f};
static GLfloat diffuse[4] = {0.8f, 0.8f, 0.8f, 1.0f};
static GLfloat specular[3] = {1.0f, 1.0f, 1.0f};

/* Transform setup data */
static int viewHeight = -10.0;
static GLdouble rotAngle = -90.0;
static GLdouble rotAxis[3] = { 1.0, 0.0, 0.0 };


/* Menu definitions */
enum {
    DRAW_BEGIN = 1000,
    DRAW_AIRBOAT,
    DRAW_CESSNA,
    DRAW_CROC,
    DRAW_FLAMINGO,
    DRAW_SHUTTLE,
    DRAW_PORSCHE,
    DRAW_END,

    AA_BEGIN,
    AA_BLEND_CONST,
    AA_BLEND_ARB,
    AA_OFF,
    AA_END,

    POLYGONS,
    LINE_POLYGON,
    LINE_LOOP,
    LINE_INDEPENDENT,
    LINE_STRIPS,

    SHADOWS_COPLANAR,
    SHADOWS_OFFSET,
    SHADOWS_STENCIL,
    SHADOWS_OFF,

    LIGHTS_ON,
    LIGHTS_OFF,

    Z_ON,
    Z_OFF,

    DOUBLE_BUFFER_ON,
    DOUBLE_BUFFER_OFF,

    QUIT
};


/* Menu entry numbers for these "toggle" modes */
static int depthBufferMenuNum;
static int doubleBufferMenuNum;
static int lightingMenuNum;

/* Identifiers for various menus */
static int menuIdent;
static int objectMenuIdent;
static int shadowsMenuIdent;

/* Current settings for each menu selection */
static int objectSetting = DRAW_SHUTTLE;
static int aaSetting = AA_BLEND_CONST;
static int lightSetting = LIGHTS_ON;
static int depthSetting = Z_OFF;
static int drawSetting = POLYGONS;
static int doubleBufferSetting = DOUBLE_BUFFER_ON;
static int shadowsSetting = SHADOWS_OFF;

static int inMotionCallback = 0;	/* Prevents annoying GLUT warning message */

/* Function prototypes */
void display(void);
void setView(void);
void drawImage(void);
void drawFPS(char *string);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void keys(unsigned char key, int x, int y);
void menu(int value);
void objectMenu(int value);
void aaMenu(int value);
void polyMenu(int value);
void drawMenu(int value);
void colorMenu(int value);
void setState(int newState);
void setMode(int value);
void setAlternateLights(int lights);
void parseArgs(int argc, char *argv[]);
void displayHelp(void);



/*
 * main
 */

int
main(int argc, char *argv[])
{
    /* Display help listing on startup */
    displayHelp();

    /* Allocate modelcontext */
    model = (ModelContext *)calloc(1, sizeof(ModelContext));

    if (model == NULL) {
	printf ("Not enough memory count be allocated for model data\n");
	exit (-1);
    }

    /* Initialize the model context data */
    model->scaleObj = 5.0;		/* Start at 5 inches */
    model->needToUpdateViewMat = 0;
    model->pointerMotion = 0;
    model->windowWidth = 640;
    model->windowHeight = 480;
    model->minimumScale = MINIMUM_SCALE;
    model->triangleFlag = 0;		/* Draw as polygons */

    /* Override defaults with command-line arguments */
    parseArgs (argc, argv);

    /* Initialize: */
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);

    glutInitWindowSize((GLint)model->windowWidth, (GLint)model->windowHeight);
    glutInitWindowPosition(1, 1);
    glutCreateWindow("Shadows");

    /* Go do all of the projection computations */
    reshapeCallback((GLint)model->windowWidth, (GLint)model->windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glGetDoublev(GL_MODELVIEW_MATRIX, model->rotMat);	/* Initial rotation */

    /* Set the screen colors */
    glClearColor(0.2f, 0.4f, 0.6f, 1.0f);

    /* Set up for drawing solids */
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_BLEND);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    setAlternateLights(1);

    /* Specify all of the callback routines */
    glutDisplayFunc(display);
    glutReshapeFunc(reshapeCallback);
    glutKeyboardFunc(keys);
    glutSpecialFunc(specialKeysCallback);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutVisibilityFunc(visibilityCallback);

    /* Create a menu */

    /* First create the object submenus */
    objectMenuIdent = glutCreateMenu(objectMenu);
    glutAddMenuEntry("Air Boat", DRAW_AIRBOAT);
    glutAddMenuEntry("Cessna", DRAW_CESSNA);
/*    glutAddMenuEntry("Crocodile", DRAW_CROC);	*/
    glutAddMenuEntry("Flamingo", DRAW_FLAMINGO);
    glutAddMenuEntry("Space Shuttle", DRAW_SHUTTLE);
    glutAddMenuEntry("Porsche", DRAW_PORSCHE);
/*
    aaMenuIdent = glutCreateMenu(aaMenu);
    glutAddMenuEntry("Blend Constant", AA_BLEND_CONST);
    glutAddMenuEntry("Blend Arbitrary", AA_BLEND_ARB);
    glutAddMenuEntry("Disable Line Antialiasing", AA_OFF);

    drawMenuIdent = glutCreateMenu(drawMenu);
    glutAddMenuEntry("Polygons", POLYGONS);
    glutAddMenuEntry("Line Polygon", LINE_POLYGON);
    glutAddMenuEntry("Line Loop", LINE_LOOP);
    glutAddMenuEntry("Line Independent", LINE_INDEPENDENT);
    glutAddMenuEntry("Line Strips", LINE_STRIPS);
*/
    shadowsMenuIdent = glutCreateMenu(menu);
    glutAddMenuEntry("Coplanar Shadows", SHADOWS_COPLANAR);
    glutAddMenuEntry("Offset Shadows", SHADOWS_OFFSET);
    glutAddMenuEntry("Stencil Shadows", SHADOWS_STENCIL);
    glutAddMenuEntry("No Shadows", SHADOWS_OFF);

    /* Now create the main menu */
    menuIdent = glutCreateMenu(menu);
    glutAddSubMenu("Objects", objectMenuIdent);
/*    glutAddSubMenu("Line Antialiasing", aaMenuIdent);
    glutAddSubMenu("Drawing Method", drawMenuIdent);*/
    glutAddSubMenu("Shadows", shadowsMenuIdent);

    glutAddMenuEntry("Disable Depth Buffer", Z_OFF);
    depthBufferMenuNum = glutGet(GLUT_MENU_NUM_ITEMS);

    glutAddMenuEntry("Single Buffering", DOUBLE_BUFFER_OFF);
    doubleBufferMenuNum = glutGet(GLUT_MENU_NUM_ITEMS);

    glutAddMenuEntry("Disable Lighting", LIGHTS_OFF);
    lightingMenuNum = glutGet(GLUT_MENU_NUM_ITEMS);

    glutAddMenuEntry("Quit", QUIT);

    glutAttachMenu(GLUT_RIGHT_BUTTON);

    readObjData("porsche.obj");

    glutMainLoop();

    return 0;

} /* End of main */



/*
 * display
 *
 *	The display loop code
 */

void
display(void)
{
	static timer oldTime, newTime;
    int microseconds;			/* How long one frame takes */
    float framesPerSecond;		/* How many frames we're getting */
static int timeSinceUpdate;		/* How long since fps update */
static int frames = -1;
static char perf[128];			/* String to display */
    GLbitfield clearBits;		/* Which buffers to clear */

    /* First time through, set the oldTime value */
    if (frames == -1)
    {
        getTime(oldTime);
        frames = 0;
    }

    /* Figure out how long since the last frame */
    getTime(newTime);
    microseconds = timeDiff(oldTime, newTime);
    oldTime = newTime;			/* Remember current time for next */
    timeSinceUpdate += microseconds;
    frames++;

    /* Display the performance numbers on the corner of the screen */
    if ((timeSinceUpdate >= 1000000) || (timeSinceUpdate < 0)) {
	/* Not more than 1 per second */
	framesPerSecond = (float)(1000000 * frames) / (float)timeSinceUpdate;
	sprintf(perf, "%.2ffps", framesPerSecond);
	timeSinceUpdate = 0;
	frames = 0;
    }

    /* Draw the picture */

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Look at the model from the defined position */
    gluLookAt(0.0, EYE_BACK, viewHeight,
	0.0, 0.0, 0.0,
	0.0, 0.0, -1.0);

    setView();
    if (doubleBufferSetting == DOUBLE_BUFFER_ON)
	glDrawBuffer(GL_BACK);
    else
	glDrawBuffer(GL_FRONT);

    clearBits = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
    if (shadowsSetting == SHADOWS_STENCIL)
	clearBits |= GL_STENCIL_BUFFER_BIT;
    glClear(clearBits);

    drawFPS(perf);
    drawImage();

    /* Finish off this frame */
    if (doubleBufferSetting == DOUBLE_BUFFER_ON) {
	glutSwapBuffers();
    }
    else {
	glFlush();
	glFinish();
    }

    if ((model->rotX == 0.0) && (model->rotY == 0.0))
	glutIdleFunc(NULL);

    model->pointerMotion = 0;			/* We just redrew */
} /* End of display */



/*
 * drawFPS
 *
 *	Draw the text to indicate the frame per second (passed in)
 */

void
drawFPS(char *perf)
{
    int i;

    if (lightSetting == LIGHTS_ON)
	glDisable(GL_LIGHTING);
    glColor3f(1.0, 1.0, 1.0);		/* Make characters visible */

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    gluLookAt(0.0, 0.0, EYE_BACK,
	0.0, 0.0, 0.0,
	0.0, 1.0, 0.0);
    glTranslatef(model->windowWidth * -0.0191,
	model->windowHeight * -0.0182, 0.0);
    glScalef(0.006f, 0.006f, 0.006f);

    for (i = 0; perf[i] != 0; i++)
	glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, perf[i]);
    glPopMatrix();

    if (lightSetting == LIGHTS_ON)
	glEnable(GL_LIGHTING);		/* Restore the lights */
} /* End of drawFPS */



/*
 * drawImage
 *
 *	Draw the full image here including shadows and floor.
 */

void
drawImage(void)
{
    register int i, j;
    register int nextIndex;		/* Next index that has a color */
    register int currColor;
    GLdouble planeDim;                  /* Dim. of floor plane */
    GLfloat ytrans;                     /* Scratch variable */

    GLfloat lightpos[3];                /* Rotated light position */

/* Floor material colors */
static const GLfloat floorAmbient[] = { 0.1f, 0.1f, 0.1f };
static const GLfloat floorDiffuse[] = { 0.3f, 0.3f, 0.4f, 1.0f };
static const GLfloat floorSpecular[] = { 0.2f, 0.2f, 0.3f };
static const GLfloat floorShininess = 0.3f;

/* An array of blackness to use in material calls */
static GLfloat black4f[] = { 0.0f, 0.0f, 0.0f, 0.0f };

/*
 * The shadows are generated using the following matrix equation
 * (which both flattens Y and skews X & Z based on the direction of the
 *  light source):
 *
 * [xs 0 zs 1] = [xp yp zp 1] [ 1         0      0       0 ]
 *                            [ -xl/yl    0      -zl/yl  0 ]
 *                            [ 0         0      1       0 ]
 *                            [ 0         0      0       1 ]
 * where:
 *   xs, zs = shadow coords (ys = 0 since we're squashing it down)
 *   xp, yp, zp = model space coords
 *   xl, yl, zl = light position
 */

static float shadowMat[] = { 1.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0 };


    /* Calculate a nice size for the floor, based on the model size */
    if (model->boundBoxRight > model->boundBoxFar)
	planeDim = model->boundBoxRight * 2.0;
    else
	planeDim = model->boundBoxFar * 2.0;

    /* Begin drawing */

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glScaled(model->scaleObj, model->scaleObj, model->scaleObj);
    if (model->needToUpdateViewMat) {
	/* Build a proper cumulative rotation for automatic motion */
	glPushMatrix();
	glLoadIdentity();

	glRotated(model->rotX * 0.2, 0.0, 0.0, -1.0);
	glMultMatrixd(model->rotMat);
	glGetDoublev(GL_MODELVIEW_MATRIX, model->rotMat);
	glPopMatrix();
	model->needToUpdateViewMat = 0;
    }
    glMultMatrixd(model->rotMat);

    /*-
     * Rotate the light source coords, so that we get shadows which change
     * as the object rotates.
     */
    lightpos[0] = (pos1[0] * model->rotMat[0]) + (pos1[2] * model->rotMat[1]);
    lightpos[1] = pos1[1];
    lightpos[2] = (pos1[0] * model->rotMat[4]) + (pos1[2] * model->rotMat[5]);

    /* Fill in the last two spots in the shadow matrix with current data */
    shadowMat[4] = -lightpos[0]/lightpos[1];
    shadowMat[6] = -lightpos[2]/lightpos[1];

    /* Correctly orient the object */
    glRotated(rotAngle, rotAxis[0], rotAxis[1], rotAxis[2]);

    /* Now draw the object */
    i = 0;
    j = 0;
    nextIndex = model->colorList[j].index;
    if (drawSetting < LINE_INDEPENDENT) {
	/* Draw solid object as polygons */
	if (model->haveNormals) {
	    do {
		if (i == nextIndex) {
		    setColor(&model->colorList[j].ra, &model->colorList[j].rd,
			&model->colorList[j].rs, model->colorList[j].spec, 0);
		    j++;
		    nextIndex = model->colorList[j].index;
		}
		if (drawSetting <= LINE_POLYGON)
		    glBegin(GL_POLYGON);
		else
		    glBegin(GL_LINE_LOOP);
		    glNormal3fv(&model->vertexList[i].nx);
		    glVertex3fv(&model->vertexList[i++].x);
		    while (model->vertexList[i].draw) {
			glNormal3fv(&model->vertexList[i].nx);
			glVertex3fv(&model->vertexList[i++].x);
		    }
		glEnd();
	    } while (i < model->vertexCount);
	} /* End of have normals */
	else do {
	    if (i == nextIndex) {
		setColor(&model->colorList[j].ra, &model->colorList[j].rd,
		    &model->colorList[j].rs, model->colorList[j].spec, 0);
		j++;
		nextIndex = model->colorList[j].index;
	    }
	    if (drawSetting <= LINE_POLYGON)
		glBegin(GL_POLYGON);
	    else
		glBegin(GL_LINE_LOOP);
		glVertex3fv(&model->vertexList[i++].x);
		while (model->vertexList[i].draw) {
		    glVertex3fv(&model->vertexList[i++].x);
		}
	    glEnd();
	} while (i < model->vertexCount);
    }
    else {
	/* Draw solid object as lines */
	currColor = -1;

	if (drawSetting == LINE_INDEPENDENT) {
	    glBegin(GL_LINES);	/* One call overall */
	    do {
		if (model->lineList[i].colorIndex != currColor) {
		    currColor = model->lineList[i].colorIndex;
		    setColor(&model->colorList[currColor].ra,
			&model->colorList[currColor].rd,
			&model->colorList[currColor].rs,
			model->colorList[currColor].spec, 0);
		}
		glVertex3fv(&model->lineList[i++].x);
		glVertex3fv(&model->lineList[i++].x);
	    } while (i < model->lineCount);
	    glEnd();
	}

	if (drawSetting == LINE_STRIPS)
	do {
	    glBegin(GL_LINE_STRIP);
		if (model->lineStripList[i].colorIndex != currColor) {
		    currColor = model->lineStripList[i].colorIndex;
		    setColor(&model->colorList[currColor].ra,
			&model->colorList[currColor].rd,
			&model->colorList[currColor].rs,
			model->colorList[currColor].spec, 0);
		}
		glVertex3fv(&model->lineStripList[i++].x);
		while (model->lineStripList[i].draw) {
		    glVertex3fv(&model->lineStripList[i++].x);
		}

	    glEnd();
	} while (i < model->lineStripCount);
    } /* End of solid object as lines */

    /* Draw shadows */

    if (shadowsSetting != SHADOWS_OFF && lightSetting == LIGHTS_ON) {
	glPushMatrix();

	/* Redraw the model, flattened */

	if (shadowsSetting == SHADOWS_OFFSET)
	    ytrans = model->boundBoxBottom - FLOOR_DIST_FROM_MODEL +
		SHADOW_ABOVE_FLOOR;
	else
	    ytrans = model->boundBoxBottom-FLOOR_DIST_FROM_MODEL;

	/* Position the shadow so that it is 'attached' to the model at
	 * the model's base */
	glTranslatef( -(lightpos[0] * model->boundBoxTop) / lightpos[1],
	     ytrans, -(lightpos[2] * model->boundBoxTop) / lightpos[1]);
	glMultMatrixf(shadowMat);

	/* Draw the shadow (redraw the model in black, flattened and skewed) */
	if (shadowsSetting == SHADOWS_STENCIL) {
	    glEnable(GL_STENCIL_TEST);
	    glStencilMask(1);
	    glStencilFunc(GL_NEVER, 1, 1);
	    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	}

	i = 0;
	setColor(floorAmbient, black4f, black4f, 0.0, 0);
	do {
	    if (drawSetting <= LINE_POLYGON)
		glBegin(GL_POLYGON);
	    else
		glBegin(GL_LINE_LOOP);

		glNormal3d(0.0, 1.0, 0.0);
		glVertex3fv(&model->vertexList[i++].x);
		while (model->vertexList[i].draw) {
		    glVertex3fv(&model->vertexList[i++].x);
		}
	    glEnd();
	} while (i < model->vertexCount);

	glPopMatrix();
	glPushMatrix();

	if (shadowsSetting == SHADOWS_STENCIL) {
	    glStencilFunc(GL_EQUAL, 0, 1);
	    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	}

	/* Draw floor to project the shadow on */
	setColor(floorAmbient, floorDiffuse, floorSpecular, floorShininess, 0);
	glBegin(GL_QUADS);
	    glNormal3d(0.0, 1.0, 0.0);
	    glVertex3d(-planeDim, model->boundBoxBottom
		 - FLOOR_DIST_FROM_MODEL, -planeDim);
	    glVertex3d(planeDim, model->boundBoxBottom
		 - FLOOR_DIST_FROM_MODEL, -planeDim);
	    glVertex3d(planeDim, model->boundBoxBottom
		 - FLOOR_DIST_FROM_MODEL, planeDim);
	    glVertex3d(-planeDim, model->boundBoxBottom
		 - FLOOR_DIST_FROM_MODEL, planeDim);
	glEnd();

	/* If stencil shadow, still need to draw in the shadow (the first
	 * pass was just to create the shadow mask in the stencil buffer)
	 */
	if (shadowsSetting == SHADOWS_STENCIL) {
	    glStencilFunc(GL_EQUAL, 1, 1);
	    glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);

	    /* Draw the flagged bits in the stencil buffer as the shadow */
	    setColor(floorAmbient, black4f, black4f, 0.0, 0);
	    glBegin(GL_QUADS);
		glNormal3d(0.0, 1.0, 0.0);
		glVertex3d(-planeDim, model->boundBoxBottom
		     - FLOOR_DIST_FROM_MODEL, -planeDim);
		glVertex3d(planeDim, model->boundBoxBottom
		     - FLOOR_DIST_FROM_MODEL, -planeDim);
		glVertex3d(planeDim, model->boundBoxBottom
		     - FLOOR_DIST_FROM_MODEL, planeDim);
		glVertex3d(-planeDim, model->boundBoxBottom
		     - FLOOR_DIST_FROM_MODEL, planeDim);
	    glEnd();

	    glDisable(GL_STENCIL_TEST);
	}
	if (drawSetting != POLYGONS)
	    setState(DRAW_WIREFRAME);
	glPopMatrix();
    }
    else {
	/* Draw the floor sans any shadows */
	setColor(floorAmbient, floorDiffuse, floorSpecular, floorShininess, 0);
	glBegin(GL_QUADS);
	    glNormal3d(0.0, 1.0, 0.0);
	    glVertex3d(-planeDim, model->boundBoxBottom
		 - FLOOR_DIST_FROM_MODEL, -planeDim);
	    glVertex3d(planeDim, model->boundBoxBottom
		 - FLOOR_DIST_FROM_MODEL, -planeDim);
	    glVertex3d(planeDim, model->boundBoxBottom
		 - FLOOR_DIST_FROM_MODEL, planeDim);
	    glVertex3d(-planeDim, model->boundBoxBottom
		 - FLOOR_DIST_FROM_MODEL, planeDim);
	glEnd();
    }
   glPopMatrix();
} /* End of drawImage */



/*
 * mouse
 *
 *	Deal with the mouse.  The menu button (right) never
 *	gets this far.
 */
static int oldButton, oldState;		/* For motion routine */

void
mouse(int button,
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

		glRotated(model->rotX, 0.0, 0.0, -1.0);
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

        getTime(oldTime);   /* Remember time */
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

} /* End of mouse */



/*
 * motion
 *
 *	No changes to mouse button, but it is moving with at least
 *	one button down.
 */

void
motion(int x,
    int y)
{
    inMotionCallback = 1;
    mouse(oldButton, oldState, x, y);
    inMotionCallback = 0;
} /* End of motion */



/*
 * keys
 *
 *	Routine to deal with the keyboard
 */

/*ARGSUSED1*/
void
keys(unsigned char key, int x, int y)
{
    switch (key) {

      case 'o':
	/* Next object */
	objectSetting++;
	if (objectSetting == DRAW_END)
	    objectSetting = DRAW_BEGIN + 1;
	objectMenu(objectSetting);
	break;
      case 'O':
	/* Previous object */
	objectSetting--;
	if (objectSetting == DRAW_BEGIN)
	    objectSetting = DRAW_END - 1;
	objectMenu(objectSetting);
	break;

      case 'a':
	/* Next antialiasing mode */
	aaSetting++;
	if (aaSetting == AA_END)
	    aaSetting = AA_BEGIN + 1;
	aaMenu(aaSetting);
	break;
      case 'A':
	/* Previous antialiasing mode */
	aaSetting--;
	if (aaSetting == AA_BEGIN)
	    aaSetting = AA_END - 1;
	aaMenu(aaSetting);
	break;

      case 'd':
      case 'D':
	if (doubleBufferSetting == DOUBLE_BUFFER_ON)
	    doubleBufferSetting = DOUBLE_BUFFER_OFF;
	else
	    doubleBufferSetting = DOUBLE_BUFFER_OFF;
	menu(doubleBufferSetting);
	break;

      case 'z':
      case 'Z':
	/* Toggle depth-buffering */
	if (depthSetting == Z_ON)
	    depthSetting = Z_OFF;
	else
	    depthSetting = Z_ON;
	menu(depthSetting);
	break;

      case 's':
	model->rotX /= 1.05;
	model->rotY /= 1.05;
	break;
      case 'S':
	model->rotX *= 1.05;
	model->rotY *= 1.05;
	break;

      case 'l':
      case 'L':
	if (lightSetting == LIGHTS_ON)
	    lightSetting = LIGHTS_OFF;
	else
	    lightSetting = LIGHTS_ON;
	setMode(lightSetting);
	break;

      case '?':
      case 'h':
      case 'H':
	displayHelp();
	break;

      case 27:
      case 'q':
      case 'Q':
	exit(0);

    } /* End of switch on key */

    glutPostRedisplay();
} /* End of keys */



/*
 * objectMenu
 *
 *	Sub-menu processing code for object selection.
 */

void
objectMenu(int value)
{
    glutSetMenu(menuIdent);		/* Prepare to access other menu */

    switch (value) {

      case DRAW_AIRBOAT:
	setState(DRAW_SOLID);
	readObjData("airboat.obj");
	objectSetting = value;
	break;

      case DRAW_CESSNA:
	setState(DRAW_SOLID);
	readObjData("cessna.obj");
	objectSetting = value;
	break;

      case DRAW_CROC:
	setState(DRAW_SOLID);
	readObjData("croc.obj");
	objectSetting = value;
	break;

      case DRAW_FLAMINGO:
	setState(DRAW_SOLID);
	readObjData("flamingo.obj");
	objectSetting = value;
	break;

      case DRAW_SHUTTLE:
	setState(DRAW_SOLID);
	readObjData("shuttle.obj");
	objectSetting = value;
	break;

      case DRAW_PORSCHE:
	setState(DRAW_SOLID);
	readObjData("porsche.obj");
	objectSetting = value;
	break;
    } /* End of switch on menu value */

    /* Make sure that we redraw */
    glutPostRedisplay();
} /* End of objectMenu */



/*
 * aaMenu
 *
 *	Menu processing code.
 *	Current version loads an object and positions it.
 */

void
aaMenu(int value)
{
    aaSetting = value;
    if (drawState & DRAW_WIREFRAME) {
	setState(DRAW_WIREFRAME);
    }

    /* Make sure that we redraw */
    glutPostRedisplay();

} /* End of aaMenu */



/*
 * drawMenu
 *
 *	Menu processing code.
 *	Deal with drawing style.
 */

void
drawMenu(int value)
{
    switch (value) {

      case POLYGONS:
	setState(DRAW_SOLID);
	drawSetting = value;
	break;

      case LINE_POLYGON:
	setState(DRAW_WIREFRAME);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	drawSetting = value;
	break;

      case LINE_LOOP:
	setState(DRAW_WIREFRAME);
	drawSetting = value;
	break;

      case LINE_INDEPENDENT:
	setState(DRAW_WIREFRAME);
	drawSetting = value;
	break;

      case LINE_STRIPS:
	setState(DRAW_WIREFRAME);
	drawSetting = value;
	break;
    } /* End of switch on menu value */

    /* Make sure that we redraw */
    glutPostRedisplay();

} /* End of drawMenu */



/*
 * menu
 *
 *	Menu processing code.
 *	Current version loads an object and positions it.
 */

void
menu(int value)
{
    switch (value) {
      case QUIT:
	exit(0);

      default:
	setMode(value);
    } /* End of switch on menu value */

    /* Make sure that we redraw */
    glutPostRedisplay();

} /* End of menu */



/*
 * setState
 *
 *	Sets the GL machine in a given state (DRAW_SOLID or DRAW_WIREFRAME).
 */

void
setState(int newState)
{
    drawState = newState;

    if (newState == DRAW_SOLID) {
	menu(LIGHTS_ON);
	menu(Z_ON);
	menu(AA_OFF);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POINT_SMOOTH);
	glDisable(GL_BLEND);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    else if (newState == DRAW_WIREFRAME) {
	menu(LIGHTS_OFF);
	glEnable(GL_POINT_SMOOTH);
	if (aaSetting == AA_BLEND_CONST) {
	    if (shadowsSetting == SHADOWS_COPLANAR)
		menu(Z_ON);
	    else
		menu(Z_OFF);
	    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	    glEnable(GL_BLEND);
	    glEnable(GL_LINE_SMOOTH);
	}
	else if (aaSetting == AA_BLEND_ARB) {
	    menu(Z_ON);
	    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	    glEnable(GL_BLEND);
	    glEnable(GL_LINE_SMOOTH);
	}
	else /* if (aaSetting == AA_OFF) */ {
	    menu(Z_ON);
	    glDisable(GL_BLEND);
	    glDisable(GL_LINE_SMOOTH);
	}
    }
} /* End of setState */



/*
 * setMode
 *
 *	Recursive-friendly menu handling sub-function
 */

void
setMode(int value)
{
    switch (value) {

      case LIGHTS_ON:
	lightSetting = value;
	setAlternateLights(1);
	glutSetMenu(menuIdent);
	glutChangeToMenuEntry(lightingMenuNum, "Disable Lighting", LIGHTS_OFF);
	break;

      case LIGHTS_OFF:
	lightSetting = value;
	setAlternateLights(0);
	glutSetMenu(menuIdent);
	glutChangeToMenuEntry(lightingMenuNum, "Enable Lighting", LIGHTS_ON);
	break;

      case DOUBLE_BUFFER_OFF:
	doubleBufferSetting = value;
	glutSetMenu(menuIdent);
	glutChangeToMenuEntry(doubleBufferMenuNum, "Double Buffering",
	    DOUBLE_BUFFER_ON);
	break;

      case DOUBLE_BUFFER_ON:
	doubleBufferSetting = value;
	glutSetMenu(menuIdent);
	glutChangeToMenuEntry(doubleBufferMenuNum, "Single Buffering",
	    DOUBLE_BUFFER_OFF);
	break;

      case Z_ON:
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	depthSetting = value;
	glutSetMenu(menuIdent);
	glutChangeToMenuEntry(depthBufferMenuNum, "Disable Depth Buffer",
	    Z_OFF);
	break;

      case Z_OFF:
	glDisable(GL_DEPTH_TEST);	/* Could be left enabled... */
	glDepthMask(GL_FALSE);
	depthSetting = value;
	glutSetMenu(menuIdent);
	glutChangeToMenuEntry(depthBufferMenuNum, "Enable Depth Buffer", Z_ON);
	break;

      case SHADOWS_COPLANAR:
	shadowsSetting = value;
	break;

      case SHADOWS_OFFSET:
	shadowsSetting = value;
	break;

      case SHADOWS_STENCIL:
	shadowsSetting = value;
	break;

      case SHADOWS_OFF:
	shadowsSetting = value;
	break;
    }
} /* End of setMode */



/*
 * setAlternateLights
 *
 *	Turn on the lights mode as dictated by the parameter:
 *	0 - no lights (disable lighting)
 *	1 - alternate lights (as opposed to those in setLights in common.c)
 */
void
setAlternateLights(int lights)
{
    if (lights == 0) {
	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT0);
	return;
    }
    else  {
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glEnable(GL_LIGHTING);

	/* We scale without changing the normal, choose the fastest method */
/*	if (glutExtensionSupported("GL_EXT_rescale_normal"))
	    glEnable(GL_RESCALE_NORMAL_EXT);
	else */
	    glEnable(GL_NORMALIZE);

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light1);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light1);
	glLightfv(GL_LIGHT0, GL_POSITION, pos1);

	/* Set some material properties, in case the object has none */
	setColor(ambient, diffuse, specular, 50.0, 0);
	glPopMatrix();
    }
} /* End of setAlternateLights */



/*
 * parseArgs
 *
 *	Set defaults based on the command-line arguments.
 */

void
parseArgs(int argc,
    char *argv[])
{
    int argCount;

    for (argCount=1; argCount<argc; argCount++) {

	/* 'Big' mode flag */
	if (strcmp(argv[argCount], "-b") == 0) {
	    model->windowWidth=1200;
	    model->windowHeight=975;
	}

	/* Set minimum scale limit */
	if (strcmp(argv[argCount], "-s") == 0) {
	   if (++argCount < argc)
	       model->minimumScale=(GLdouble)atof (argv[argCount]);
	   else
	       fprintf(stderr, "Error, not enough args to set min. scale.\n");
	}
    }
} /* End of parseArgs */



/*
 * displayHelp
 *
 *	Tells the user what they can do.
 */
void
displayHelp(void)
{
    printf("-------\n");
    printf("Shadows\n\n");
    printf("Mouse: Left   = motion\n");
    printf("       Middle = scale\n");
    printf("       Right  = menu\n");
    printf("Keys: Arrows  = translate\n");
    printf("      l, L    = toggle lights\n");
    printf("      o, O    = next, previous object\n");
    printf("      a, A    = switch line antialiasing mode\n");
    printf("      d, D    = toggle double buffering\n");
    printf("      z, Z    = toggle depth-buffering\n");
    printf("      s, S    = decrease, increase speed\n");
    printf("      h, H, ? = print this message\n");
    printf("      q, Q, esc = quit\n");
} /* End of displayHelp */

/* End of shadows.c */
