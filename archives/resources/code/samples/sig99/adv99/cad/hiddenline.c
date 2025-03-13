/*
 * hiddenline.c
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
 *	and set various modes while the object moves. Show the different
 *	forms of edge highlight/hidden line.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>

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


#define DEPTH_RANGE_OFFSET 0.0001

static GLfloat pointSize = 1.0;
static GLfloat lineWidth = 1.0;

static GLdouble rotAngle = -90.0;
static GLdouble rotAxis[3] = { 1.0, 0.0, 0.0 };

static GLfloat edgeColor[3] = { 1.0, 1.0, 0.0 };
static GLfloat backgroundColor[3] = { 0.0, 0.0, 0.0 };

/* Menu definitions */
enum {
    DRAW_BEGIN = 1000,
    DRAW_SPHERE,
    DRAW_CUBE,
    DRAW_AIRBOAT,
    DRAW_CESSNA,
    DRAW_CROC,
    DRAW_FLAMINGO,
    DRAW_SHUTTLE,
    DRAW_PORSCHE,
    DRAW_END,

    AA_BEGIN,
    AA_BLEND_ARB,
    AA_OFF,
    AA_END,

    POLY_FILL,
    POLY_LINE,
    POLY_POINT,

    EDGE_NONE,
    EDGE_NO_OFFSET,
    EDGE_POLY_OFFSET,
    EDGE_DEPTH_RANGE,
    EDGE_STENCIL,
    EDGE_DEPTH_BUFFER,

    LINE_POLYGON,
    LINE_LOOP,
    LINE_INDEPENDENT,
    LINE_STRIPS,

    EDGE_HIGHLIGHT,
    HIDDEN_LINE,

    CULL_ON,
    CULL_OFF,

    LIGHTS_ON,
    LIGHTS_OFF,

    Z_ON,
    Z_OFF,

    DOUBLE_BUFFER_ON,
    DOUBLE_BUFFER_OFF,

    QUIT
};


/* Menu entry numbers for these "toggle" modes */
static int cullMenuNum;
static int edgeHiddenMenuNum;
static int aaMenuNum;
static int doubleBufferMenuNum;
static int menuIdent;
static int objectMenuIdent;
static int edgeMenuIdent;

/* Current settings for each */
static int objectSetting = DRAW_SHUTTLE;
static int aaSetting = AA_OFF;
static int edgeHiddenSetting = EDGE_HIGHLIGHT;
static int lightSetting = LIGHTS_ON;
static int doubleBufferSetting = DOUBLE_BUFFER_ON;
static int depthSetting = Z_OFF;
static int lineSetting = LINE_STRIPS;
static int edgeSetting = EDGE_NONE;

/* Function prototypes */
void display(void);
void drawImage(void);
void keys(unsigned char key, int x, int y);
void setMode(int value);
void menu(int value);
void objectMenu(int value);
void edgeMenu(int value);
void drawFPS(char *string);
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
    model->scaleObj = 10.0;		/* Start at 10 inches */
    model->needToUpdateViewMat = 0;
    model->pointerMotion = 0;
    model->windowWidth = 640;
    model->windowHeight = 480;
    model->minimumScale = MINIMUM_SCALE;
    model->triangleFlag = 0;		/* Display as polygons */

    /* Override defaults with command-line arguments */
    parseArgs (argc, argv);


    /* Initialize: */

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STENCIL);

    glutInitWindowSize((GLint)model->windowWidth, (GLint)model->windowHeight);
    glutInitWindowPosition(1, 1);
    glutCreateWindow("Hidden Line - Edge Highlighting");

    /* Go do all of the projection computations */
    reshapeCallback((GLint)model->windowWidth, (GLint)model->windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glGetDoublev(GL_MODELVIEW_MATRIX, model->rotMat);	/* Initial rotation */

    /* Set the screen model->colorList */
    glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], 1.0);

    /* Set up for drawing solids */
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_BLEND);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    setLights(1);

    /* Specify all of the callback routines */
    glutDisplayFunc(display);
    glutReshapeFunc(reshapeCallback);
    glutKeyboardFunc(keys);
    glutSpecialFunc(specialKeysCallback);
    glutMouseFunc(mouseCallback);
    glutMotionFunc(motionCallback);
    glutVisibilityFunc(visibilityCallback);

    /* Create a menu */

    /* First create the object submenus */
    objectMenuIdent = glutCreateMenu(objectMenu);
    glutAddMenuEntry("Sphere", DRAW_SPHERE);
    glutAddMenuEntry("Cube", DRAW_CUBE);
    glutAddMenuEntry("Air Boat", DRAW_AIRBOAT);
    glutAddMenuEntry("Cessna", DRAW_CESSNA);
/*  glutAddMenuEntry("Crocodile", DRAW_CROC);	*/
    glutAddMenuEntry("Flamingo", DRAW_FLAMINGO);
    glutAddMenuEntry("Space Shuttle", DRAW_SHUTTLE);
    glutAddMenuEntry("Porsche", DRAW_PORSCHE);

    edgeMenuIdent = glutCreateMenu(edgeMenu);
    glutAddMenuEntry("No edges", EDGE_NONE);
    glutAddMenuEntry("No offset", EDGE_NO_OFFSET);
    glutAddMenuEntry("Depth Range", EDGE_DEPTH_RANGE);
    glutAddMenuEntry("Polygon Offset", EDGE_POLY_OFFSET);
    glutAddMenuEntry("Depth Buffer", EDGE_DEPTH_BUFFER);
    glutAddMenuEntry("Stencil", EDGE_STENCIL);

    /* Now create the main menu */
    menuIdent = glutCreateMenu(menu);
    glutAddSubMenu("Objects", objectMenuIdent);
    glutAddSubMenu("Edge Method", edgeMenuIdent);

    glutAddMenuEntry("Hidden Line Mode", HIDDEN_LINE);
    edgeHiddenMenuNum = glutGet(GLUT_MENU_NUM_ITEMS);

    glutAddMenuEntry("Enable Line Antialiasing", AA_BLEND_ARB);
    aaMenuNum = glutGet(GLUT_MENU_NUM_ITEMS);

    glutAddMenuEntry("Disable Face Culling", CULL_OFF);
    cullMenuNum = glutGet(GLUT_MENU_NUM_ITEMS);

    glutAddMenuEntry("Single Buffering", DOUBLE_BUFFER_OFF);
    doubleBufferMenuNum = glutGet(GLUT_MENU_NUM_ITEMS);

    glutAddMenuEntry("Quit", QUIT);

    glutAttachMenu(GLUT_RIGHT_BUTTON);

    readObjData("shuttle.obj");

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
    int aaMode;				/* To save aaSetting */
    int indexSave;			/* To save color index value */
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
    gluLookAt(0.0, 0.0, EYE_BACK,
	0.0, 0.0, 0.0,
	0.0, 1.0, 0.0);

    setView();
    if (doubleBufferSetting == DOUBLE_BUFFER_ON)
	glDrawBuffer(GL_BACK);
    else
	glDrawBuffer(GL_FRONT);
    clearBits = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
    if (edgeSetting == EDGE_STENCIL)
	clearBits |= GL_STENCIL_BUFFER_BIT;

    glDepthMask(GL_TRUE);
    glClear(clearBits);

    /* Display frame rate, no wide lines allowed */
    glLineWidth(1.0);
    drawFPS(perf);
    glLineWidth(lineWidth);

    /* Draw the solid object */
    indexSave = model->colorList[0].index;
    if ((edgeHiddenSetting == HIDDEN_LINE) ||
	    (edgeSetting == EDGE_STENCIL)) {
	setMode(LIGHTS_OFF);
	model->colorList[0].index = -1;		/* No first color */
	glColor3fv(backgroundColor);
    }
    else {
	setMode(LIGHTS_ON);
    }
    setMode(Z_ON);
    aaMode = aaSetting;
    setMode(AA_OFF);
    aaSetting = aaMode;
    lineSetting = LINE_POLYGON;
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    /* prepare to draw wireframe version, if enabled */
    switch (edgeSetting) {

      case EDGE_NONE:
	glDepthRange(0.0, 1.0);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.0, 0.0);
	break;

      case EDGE_NO_OFFSET:
	glDepthRange(0.0, 1.0);
	glDisable(GL_POLYGON_OFFSET_FILL);
	break;

      case EDGE_DEPTH_RANGE:
	glDepthRange(0.0 + DEPTH_RANGE_OFFSET, 1.0);
	glDisable(GL_POLYGON_OFFSET_FILL);
	break;

      case EDGE_POLY_OFFSET:
	glDepthRange(0.0, 1.0);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0, 1.0);	/* Values tuned for jaggy lines */
	break;

      case EDGE_STENCIL:
	glEnable(GL_DEPTH_TEST);
	break;

      case EDGE_DEPTH_BUFFER:
	/* This does hidden line only, no edge highlighting */
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDepthRange(0.0 + DEPTH_RANGE_OFFSET, 1.0);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	break;
    } /* End of switch on hidden line method */

    drawImage();
    model->colorList[0].index = indexSave;	/* Restore first color */

    /* Draw the wireframe version */
    if ((edgeSetting != EDGE_NONE) && (edgeSetting != EDGE_STENCIL)) {

	indexSave = model->colorList[0].index;
	model->colorList[0].index = -1;		/* No first color */

	glColor3fv(edgeColor);
	setMode(LIGHTS_OFF);		/* Set needed settings */
	setMode(Z_OFF);
	setMode(aaSetting);
	lineSetting = LINE_STRIPS;

	switch (edgeSetting) {

	  case EDGE_NO_OFFSET:
	    break;

	  case EDGE_DEPTH_RANGE:
	    glDepthRange(0.0, 1.0 - DEPTH_RANGE_OFFSET);
	    break;

	  case EDGE_POLY_OFFSET:
	    glPolygonOffset(0.0, 0.0);
	    break;

	  case EDGE_STENCIL:
	    break;

	  case EDGE_DEPTH_BUFFER:
	    glDepthRange(0.0, 1.0 - DEPTH_RANGE_OFFSET);
	    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	    break;
	}
	drawImage();

	model->colorList[0].index = indexSave;	/* Restore first color */
    } /* End of if drawing edges */

    glDisable(GL_STENCIL_TEST);

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
 * drawImage
 *
 *	Draw the full image here.
 */

void
drawImage(void)
{
    register int i, j;
    register int nextIndex;		/* Next index that has a color */

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glScaled(model->scaleObj, model->scaleObj, model->scaleObj);
    if (model->needToUpdateViewMat) {
	/* Build a proper cumulative rotation for automatic motion */
	glPushMatrix();
	glLoadIdentity();
	glRotated(model->rotX * 0.2, 0.0, 1.0, 0.0);
	glRotated(model->rotY * 0.2, 1.0, 0.0, 0.0);
	glMultMatrixd(model->rotMat);
	glGetDoublev(GL_MODELVIEW_MATRIX, model->rotMat);
	glPopMatrix();
	model->needToUpdateViewMat = 0;
    }
    glMultMatrixd(model->rotMat);
    /* Correctly orient the object */
    glRotated(rotAngle, rotAxis[0], rotAxis[1], rotAxis[2]);

    /* Now draw the object */
    i = 0;
    j = 0;
    nextIndex = model->colorList[j].index;
    if (edgeSetting == EDGE_STENCIL) {
	/* Note: doesn't work with antialiasing */
	glEnable(GL_STENCIL_TEST);
	glStencilMask(1);
	do {
	    glStencilFunc(GL_ALWAYS, 1, 1);
	    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	    j = i;			/* Remember starting i */
	    glBegin(GL_LINE_LOOP);
		glVertex3fv(&model->vertexList[i++].x);
		while (model->vertexList[i].draw) {
		    glVertex3fv(&model->vertexList[i++].x);
		}
	    glEnd();

	    glColor3fv(backgroundColor);
	    glStencilFunc(GL_EQUAL, 0, 1);
	    glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
	    i = j;			/* Go back to original i */
	    glBegin(GL_POLYGON);
		glVertex3fv(&model->vertexList[i++].x);
		while (model->vertexList[i].draw) {
		    glVertex3fv(&model->vertexList[i++].x);
		}
	    glEnd();

	    glStencilFunc(GL_ALWAYS, 0, 1);
	    /*glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);*/
	    glColor3fv(edgeColor);
	    i = j;			/* Go back to original i */
	    glBegin(GL_LINE_LOOP);
		glVertex3fv(&model->vertexList[i++].x);
		while (model->vertexList[i].draw) {
		    glVertex3fv(&model->vertexList[i++].x);
		}
	    glEnd();
	} while (i < model->vertexCount);
    } else if (lineSetting < LINE_INDEPENDENT) {
	if (model->haveNormals) {
	    do {
		if (i == nextIndex) {
		    setColor(&model->colorList[j].ra, &model->colorList[j].rd,
			&model->colorList[j].rs, model->colorList[j].spec, 0);
		    j++;
		    nextIndex = model->colorList[j].index;
		}
		if (lineSetting == LINE_POLYGON)
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
	    if (lineSetting == LINE_POLYGON)
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
	if (model->colorList[0].index >= 0)
	    setColor(&model->colorList[0].ra, &model->colorList[0].rd,
		&model->colorList[0].rs, model->colorList[0].spec, 0);
	if (lineSetting == LINE_INDEPENDENT) {
	    glBegin(GL_LINES);	/* One call overall */
	    do {
		glVertex3fv(&model->lineList[i++].x);
		glVertex3fv(&model->lineList[i++].x);
	    } while (i < model->lineCount);
	    glEnd();
	}

	if (lineSetting == LINE_STRIPS) do {
	    glBegin(GL_LINE_STRIP);
		glVertex3fv(&model->lineStripList[i++].x);
		while (model->lineStripList[i].draw)
		    glVertex3fv(&model->lineStripList[i++].x);
	    glEnd();
	} while (i < model->lineStripCount);
    } /* End of solid object as lines */

    glPopMatrix();
} /* End of drawImage */



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

      case '+':
	pointSize *= 1.5f;
	if (pointSize >= 11.4f)		/* Should check hardware */
	    pointSize = 11.4f;
	glPointSize(pointSize);
	break;

      case '-':
	pointSize /= 1.5;
	glPointSize(pointSize);
	break;

      case 'o':
	/* Next object */
	objectSetting++;
	if (objectSetting == DRAW_END)
	    objectSetting = DRAW_BEGIN + 1;
	setMode(objectSetting);
	break;
      case 'O':
	/* Previous object */
	objectSetting--;
	if (objectSetting == DRAW_BEGIN)
	    objectSetting = DRAW_END - 1;
	menu(objectSetting);
	break;

      case 'A':
      case 'a':
	/* Switch antialiasing modes */
	if (aaSetting == AA_BLEND_ARB)
	    aaSetting = AA_OFF;
	else
	    aaSetting = AA_BLEND_ARB;
	menu(aaSetting);
	break;

      case 'd':
      case 'D':
	if (doubleBufferSetting == DOUBLE_BUFFER_ON)
	    doubleBufferSetting = DOUBLE_BUFFER_OFF;
	else
	    doubleBufferSetting = DOUBLE_BUFFER_ON;
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
	menu(lightSetting);
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
    setMode(LIGHTS_ON);
    setMode(Z_ON);			/* Get other needed settings */
    setMode(AA_OFF);
    setMode(value);

    /* Make sure that we redraw */
    glutPostRedisplay();

} /* End of objectMenu */



/*
 * edgeMenu
 *
 *	Menu processing code.
 *	Deal with edge drawing style.
 */

void
edgeMenu(int value)
{

    lineSetting = LINE_STRIPS;
    edgeSetting = value;
    switch (value) {

      case EDGE_NONE:
	break;

      case EDGE_NO_OFFSET:
	break;

      case EDGE_POLY_OFFSET:
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	break;

      case EDGE_DEPTH_RANGE:
	break;

      case EDGE_STENCIL:
	break;

      case EDGE_DEPTH_BUFFER:
	break;
    } /* End of switch on menu value */

    /* Make sure that we redraw */
    glutPostRedisplay();

} /* End of edgeMenu */



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
      case AA_BLEND_ARB:
	setMode(value);
	glutSetMenu(menuIdent);
	glutChangeToMenuEntry(aaMenuNum, "Disable Line Antialiasing", AA_OFF);
	break;

      case AA_OFF:
	setMode(value);
	glutSetMenu(menuIdent);
	glutChangeToMenuEntry(aaMenuNum, "Enable Line Antialiasing",
	    AA_BLEND_ARB);
	break;

      case QUIT:
	exit(0);

      default:
	setMode(value);
	break;
    } /* End of switch on menu value */

    /* Make sure that we redraw */
    glutPostRedisplay();

} /* End of menu */



/*
 * setMode
 *
 *	Called by the menu routines to set various modes based on
 *	the menu values.  May be called elsewhere (like keyboard
 *	handler) as well.  May be called recursively to make sure
 *	that all of the right stuff is set.
 */

void
setMode(int value)
{

    switch (value) {

/* Objects */

      case DRAW_SPHERE:
	buildSphere(50);
	objectSetting = value;
	break;

      case DRAW_CUBE:
	buildCube();
	objectSetting = value;
	break;

      case DRAW_AIRBOAT:
	readObjData("airboat.obj");
	objectSetting = value;
	break;

      case DRAW_CESSNA:
	readObjData("cessna.obj");
	objectSetting = value;
	break;

      case DRAW_CROC:
	readObjData("croc.obj");
	objectSetting = value;
	break;

      case DRAW_FLAMINGO:
	readObjData("flamingo.obj");
	objectSetting = value;
	break;

      case DRAW_SHUTTLE:
	readObjData("shuttle.obj");
	objectSetting = value;
	break;

      case DRAW_PORSCHE:
	readObjData("porsche.obj");
	objectSetting = value;
	break;

/* Line antialiasing */

      case AA_BLEND_ARB:
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	aaSetting = value;
	break;

      case AA_OFF:
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POINT_SMOOTH);
	glDisable(GL_BLEND);
	aaSetting = value;
	break;

/* Edge highlight/hidden line mode */

      case EDGE_HIGHLIGHT:
	edgeHiddenSetting = value;
	glutSetMenu(menuIdent);
	glutChangeToMenuEntry(edgeHiddenMenuNum, "Edge Highlight Mode",
	    HIDDEN_LINE);
	break;

      case HIDDEN_LINE:
	edgeHiddenSetting = value;
	glutSetMenu(menuIdent);
	glutChangeToMenuEntry(edgeHiddenMenuNum, "Edge Highlight Mode",
	    EDGE_HIGHLIGHT);
	break;

/* Polygon fill mode */

      case POLY_FILL:
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	break;

      case POLY_LINE:
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	break;

      case POLY_POINT:
	glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	break;

/* Cull mode */

      case CULL_ON:
	glEnable(GL_CULL_FACE);
	glutSetMenu(menuIdent);
	glutChangeToMenuEntry(cullMenuNum, "Disable Face Culling", CULL_OFF);
	break;

      case CULL_OFF:
	glDisable(GL_CULL_FACE);
	glutSetMenu(menuIdent);
	glutChangeToMenuEntry(cullMenuNum, "Enable Face Culling", CULL_ON);
	break;

/* Light mode */

      case LIGHTS_ON:
	setLights(1);
	lightSetting = value;
	break;

      case LIGHTS_OFF:
	setLights(0);
	lightSetting = value;
	break;

/* Double buffer mode */

      case DOUBLE_BUFFER_OFF:
	doubleBufferSetting = value;
	glutSetMenu(menuIdent);
	glutChangeToMenuEntry(doubleBufferMenuNum, "Double Buffering",
	    DOUBLE_BUFFER_ON);
	break;

      case DOUBLE_BUFFER_ON:
	doubleBufferSetting = value;
	glutChangeToMenuEntry(doubleBufferMenuNum, "Single Buffering",
	    DOUBLE_BUFFER_OFF);
	break;

/* Depth buffer mode */

      case Z_ON:
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	depthSetting = value;
	break;

      case Z_OFF:
/*	glDisable(GL_DEPTH_TEST);	Could be left enabled... */
	glDepthMask(GL_FALSE);
	depthSetting = value;
	break;

    } /* End of switch on mode value */

} /* End of setMode */



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
    /* Constant offset values hacked until they looked good: */
    glTranslatef(model->windowWidth * -0.0191, model->windowHeight * -0.0182,
	0.0);
    glScalef(0.006f, 0.006f, 0.006f);

    for (i = 0; perf[i] != 0; i++)
	glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, perf[i]);
    glPopMatrix();

    if (lightSetting == LIGHTS_ON)
	glEnable(GL_LIGHTING);		/* Restore the lights */
} /* End of drawFPS */



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
    printf("-------------------------------\n");
    printf("Hidden Line - edge highlighting\n\n");
    printf("Mouse: Left   = motion\n");
    printf("       Middle = scale\n");
    printf("       Right  = menu\n");
    printf("Keys: Arrows  = translate\n");
    printf("      +, -    = increase/decrease point size\n");
    printf("      o, O    = next, previous object\n");
    printf("      a, A    = switch line antialiasing mode\n");
    printf("      d, D    = toggle double buffering\n");
    printf("      z, Z    = toggle depth-buffering\n");
    printf("      s, S    = decrease, increase rotation speed\n");
    printf("      h, H, ? = print this message\n");
    printf("      q, Q, esc = quit\n");
} /* End of displayHelp */

/* End of hiddenline.c */
