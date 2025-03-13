/*
 * frustum_model.c
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
 *	Module containing routines to handle the 'model' window of the
 *	frustum demo.  The 'model' window's view frustum is graphically
 *      represented in the 'view' window.
 */

#include <stdio.h>
#include <string.h>
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


extern void displayHelp(void);

/* Data view and model windows */
extern ViewContext *view;
extern ModelContext *model;

/* Stereo flags */
extern int stereoMode;
extern GLboolean hasStereo;

MaterialColor *readMaterials(void);
static int inMotionCallback = 0;	/* Prevents annoying GLUT warning message */

/*
 * modelInitWindow
 *
 *	Initializes the model window for use
 */

void
modelInitWindow(void)
{
static GLfloat yellow[3] = { 1.0, 1.0, 0.0 };
static GLfloat black[3] = { 0.0, 0.0, 0.0 };

    /* Initialize the model context data */
    model->scaleObj = 5.0;		/* Start at 10 inches */
    model->needToUpdateViewMat = 0;
    model->pointerMotion = 0;
    if (glutGet(GLUT_SCREEN_WIDTH) > 0)
        model->windowWidth = glutGet(GLUT_SCREEN_WIDTH) / 2;
    else
        model->windowWidth = 640;
    model->windowHeight = glutGet(GLUT_SCREEN_HEIGHT) - view->windowHeight- 60;
    model->minimumScale = MINIMUM_MODEL_SCALE;
    model->rotAxis[0] = 1.0;
    model->rotAxis[1] = 0.0;
    model->rotAxis[2] = 0.0;
    model->rotAngle = -90.0;
    model->op = ROTATE;
    model->animateFlag = 1;
    model->eyeballZ = EYE_BACK;
    model->frontPlaneDist = FRONT_PLANE;
    model->frontPlaneZ = EYE_BACK - FRONT_PLANE;
    model->backPlaneDist = BACK_PLANE;
    model->backPlaneZ = EYE_BACK - BACK_PLANE;
    model->screenZ = 0.0;
    model->halfEyeDist = 1.25;
    model->objectSetting = DRAW_SHUTTLE;
    model->stereoSetting = STEREO_OFF;

    glutInitWindowPosition(0, (GLint)view->windowHeight + 30);
    glutInitWindowSize((GLint)model->windowWidth, (GLint)model->windowHeight);
    model->windowIdent = glutCreateWindow("Model");

    /* Go do all of the projection computations */
    modelReshape((GLint)model->windowWidth, (GLint)model->windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glGetDoublev(GL_MODELVIEW_MATRIX, model->rotMat);	/* Initial rotation */

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

    /* Set it up to draw bright yellow on back surfaces */
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glMaterialfv(GL_BACK, GL_EMISSION, yellow);
    glMaterialfv(GL_BACK, GL_AMBIENT, black);
    glMaterialfv(GL_BACK, GL_DIFFUSE, black);
    glMaterialfv(GL_BACK, GL_SPECULAR, black);

    /* Specify all of the callback routines */
    glutDisplayFunc(display);
    glutReshapeFunc(modelReshape);
    glutKeyboardFunc(modelKeys);
    glutSpecialFunc(modelSpecialKeys);
    glutMouseFunc(modelMouse);
    glutMotionFunc(modelMotion);
    glutVisibilityFunc(modelVisibility);

    /* Menu created in frustum_view (which is init'ed first) */
    glutSetMenu(model->menuIdent);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
} /* End of modelInitWindow */



/*
 * modelDisplay
 *
 *	The display loop code
 */

void
modelDisplay(void)
{
    if ((stereoMode == MONO) || model->stereoSetting == STEREO_OFF) {
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	/* Look at the model from the defined position */
	gluLookAt(0.0, 0.0, model->eyeballZ,
	    0.0, 0.0, model->screenZ,
	    0.0, 1.0, 0.0);

	modelSetView();
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	modelDrawImage();
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
	gluLookAt(-model->halfEyeDist, 0.0, model->eyeballZ,
	    0.0, 0.0, model->screenZ,
	    0.0, 1.0, 0.0);

	modelSetView();
	modelDrawImage();

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
	gluLookAt(model->halfEyeDist, 0.0, model->eyeballZ,
	    0.0, 0.0, model->screenZ,
	    0.0, 1.0, 0.0);

	modelSetView();
	modelDrawImage();

	/* Put the mask back to their original value */
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    glutSwapBuffers();

    model->pointerMotion = 0;			/* We just redrew */
} /* End of modelDisplay */



/*
 * modelDrawImage
 *
 */

void
modelDrawImage(void)
{
    register int i, j;
    register int nextIndex;		/* Next index that has a color */
    /* Some default colors for models with no color */
    static GLfloat ambient[3] = {0.1f, 0.05f, 0.2f};
    static GLfloat diffuse[4] = {0.3f, 0.0f, 0.8f, 1.0f};
    static GLfloat specular[3] = {1.0f, 1.0f, 1.0f};
    GLboolean RBstereo;			/* Flag for red/blue stereo */

    RBstereo = !hasStereo && (stereoMode != MONO);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    /* Scale and rotate the model */
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

} /* End of modelDrawImage */



/*
 * modelMouse
 *
 *	Deal with the mouse.  The menu button (right) never
 *	gets this far.
 */
static int oldButton, oldState;		/* For motion routine */

void
modelMouse(int button,
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
	    model->animateFlag = 0;	/* No automatic motion until release */
	    ld = 1;			/* Button is down */
	}
	else {
	    /* Only choice here is: button just came up */
	    getTime(newTime);
	    milliseconds = timeDiff(oldTime, newTime);

	    if (milliseconds > 500000) {	/* Over 1/2 second? */
		/* Stop the motion */

		model->animateFlag = 0;
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
		    model->animateFlag = 1;
		}
		else {
		    /* Insufficient history, stop! */
		    model->rotX = 0.0;
		    model->rotY = 0.0;
		    model->animateFlag = 0;
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

} /* End of modelMouse */



/*
 * readObjData
 *
 *	Read a Wavefront .OBJ file and store as triangles.
 *	This version is far from complete and can't deal with all
 *	.OBJ files by a long ways.  This is NOT robust code!
 */

void
readObjData(char *FileName)
{
    FILE *dataFile;			/* The file we're reading */
    int count;				/* Count of number read on input */
    char inputLine[512];		/* Place to store one line of input */
    int ilLen;				/* Length of input line */
    GLfloat x, y, z;			/* A vertex coordinate */
    int facetVertex[100];		/* Index array for facets */
    int facetNormal[100];		/* For normals, if there are any */
    int fvCount;			/* How many vertices in the facet */
    int linePos, lp;			/* Position on input line */
    int i, j;				/* Iterative counters*/
    char materialName[128];		/* Name of material in .mtl file */
    MaterialColor *materials;		/* Pointer to material description */
    MaterialColor *matPtr;		/* Moving ptr to material description */

    dataFile = fileOpen(FileName, "r");
    if (dataFile == NULL) {
	perror(FileName);
	exit(1);
    }
    materials = NULL;
    model->haveNormals = 0;
    model->ovCount = 1;			/* Vertices start at 1, not 0 */
    model->onCount = 1;
    model->vertexCount = 0;
    model->colorCount = 0;
    model->colorList[0].index = -1;

    for(;;) {
	if (fgets(inputLine, 500, dataFile) == NULL)
	    break;			/* End of file */

	/* Get length of line, no trailing spaces */
	ilLen = strlen(inputLine);
	while ((ilLen > 0) && ((inputLine[ilLen - 1] == ' ') ||
		(inputLine[ilLen - 1] == '\n')))
	    ilLen--;

	if (inputLine[0] == 'v') {
	    /* Read one vertex and store it in the vertex array */

	    if (inputLine[1] == ' ') {
		/* A vertex */
		count = sscanf(inputLine, "v %f %f %f", &x, &y, &z);
		if (count != 3)
		    continue;
		model->objVertexList[model->ovCount].x = x;
		model->objVertexList[model->ovCount].y = y;
		model->objVertexList[model->ovCount++].z = z;
	    }

	    else if (inputLine[1] == 'n') {
		/* A normal */
		count = sscanf(inputLine, "vn %f %f %f", &x, &y, &z);
		if (count != 3)
		    continue;
		model->objVertexList[model->onCount].nx = x;
		model->objVertexList[model->onCount].ny = y;
		model->objVertexList[model->onCount++].nz = z;
		model->haveNormals = 1;
	    }
	}

	else if (inputLine[0] == 'f') {
	    /* Read one facet, get its vertex coordinates and add to list */
	    fvCount = 0;
	    linePos = 2;
	    while (linePos < ilLen) {
		/* Get the next number */
		sscanf(&inputLine[linePos], "%d%n", &facetVertex[fvCount], &lp);
		if (inputLine[linePos + lp] == '/') {
		    /* We have normals to, assume "//" for now */
		    linePos += lp + 2;
		    sscanf(&inputLine[linePos], "%d%n",
			&facetNormal[fvCount], &lp);
		}
		fvCount++;
		linePos += lp + 1;
	    }
	    if (fvCount < 3)
		continue;		/* Two vertex polygon?  Not! */
	    facetNormal[fvCount] = facetNormal[0];	/* Wrap at end */

	    /* Convert vertex numbers to XYZ, store in vertex data array */
	    for (i = 0; i < fvCount; i++) {
		j = i;
		if (j > 0) {
		    if ((i & 1) == 1)
			j = (i + 1) >> 1;
		    else
			j = fvCount - (i >> 1);
		}
		/* 0 is move, non-zero is draw */
		model->vertexList[model->vertexCount].draw = i;
		model->vertexList[model->vertexCount].x =
		    model->objVertexList[facetVertex[j]].x;
		model->vertexList[model->vertexCount].y =
		    model->objVertexList[facetVertex[j]].y;
		model->vertexList[model->vertexCount].z =
		    model->objVertexList[facetVertex[j]].z;
		if (model->haveNormals) {
		    model->vertexList[model->vertexCount].nx =
			model->objVertexList[facetNormal[j]].nx;
		    model->vertexList[model->vertexCount].ny =
			model->objVertexList[facetNormal[j]].ny;
		    model->vertexList[model->vertexCount].nz =
			model->objVertexList[facetNormal[j]].nz;
		}
		model->vertexCount++;
	    }
	}

	else if (inputLine[0] == 'l') {
	    /* A line, do this some day */
	}
	else if (inputLine[0] == 'p') {
	    /* A point, do this some day */
	}
	else if (inputLine[0] == 's') {
	    /* A smoothing value */
	}
	else if (inputLine[0] == 'u') {
	    /* Probably usemtl, need to add that too */
	    if (strncmp("usemtl", inputLine, 6) != 0)
		continue;		/* "Who knows" what this is */
	    if (materials == NULL)
		materials = readMaterials();

	    sscanf(&inputLine[7], "%s", materialName);
	    matPtr = materials;
	    while ((strcmp(materialName, matPtr->name) != 0) &&
		    (matPtr->name[0] != 0))
		matPtr++;

	    /* We have a pointer to the right material */
	    model->colorList[model->colorCount].index = model->vertexCount;
	    model->colorList[model->colorCount].ra = matPtr->ra;
	    model->colorList[model->colorCount].ga = matPtr->ga;
	    model->colorList[model->colorCount].ba = matPtr->ba;
	    model->colorList[model->colorCount].rd = matPtr->rd;
	    model->colorList[model->colorCount].gd = matPtr->gd;
	    model->colorList[model->colorCount].bd = matPtr->bd;
	    model->colorList[model->colorCount].ad = matPtr->ad;
	    model->colorList[model->colorCount].rs = matPtr->rs;
	    model->colorList[model->colorCount].gs = matPtr->gs;
	    model->colorList[model->colorCount].bs = matPtr->bs;
	    model->colorList[model->colorCount].spec = matPtr->spec;
	    model->colorCount++;
	}
	/* Else, a line we can ignore */
    }

    /* Mark last one as a move */
    model->vertexList[model->vertexCount].draw = 0;

    fclose(dataFile);
    free(materials);
} /* End of readObjData */



/*
 * readMaterials
 *
 *	Read the contents of the materials file
 */

MaterialColor *
readMaterials(void)
{
    FILE *mtlFile;			/* The material file we're reading */
    char inputLine[256];		/* Read a line here */
    MaterialColor *matPtr;		/* Pointer to allocated space */
    int i;				/* Index into array */
    int count;				/* How many values read in */
    GLfloat r, g, b;			/* To read colors into */
    GLfloat spec;			/* To read specular value into */

    mtlFile = fileOpen("materials.mtl", "r");
    if (mtlFile == NULL)
	exit(1);

    matPtr = (MaterialColor *)calloc(1, 100 * sizeof(MaterialColor));

    i = -1;
    for(;;) {
	if (fgets(inputLine, 250, mtlFile) == NULL)
	    break;			/* End of file */

	if (strncmp("newmtl", inputLine, 6) == 0) {
	    i++;
	    sscanf(&inputLine[7], "%s", matPtr[i].name);
	}
	else if (strncmp("Ka", inputLine, 2) == 0) {
	    count = sscanf(inputLine, "Ka %f %f %f", &r, &g, &b);
	    if (count != 3)
		continue;
	    matPtr[i].ra = r;
	    matPtr[i].ga = g;
	    matPtr[i].ba = b;
	}
	else if (strncmp("Kd", inputLine, 2) == 0) {
	    count = sscanf(inputLine, "Kd %f %f %f", &r, &g, &b);
	    if (count != 3)
		continue;
	    matPtr[i].rd = r;
	    matPtr[i].gd = g;
	    matPtr[i].bd = b;
	    matPtr[i].ad = 1.0;
	}
	else if (strncmp("Ks", inputLine, 2) == 0) {
	    count = sscanf(inputLine, "Ks %f %f %f", &r, &g, &b);
	    if (count != 3)
		continue;
	    matPtr[i].rs = r;
	    matPtr[i].gs = g;
	    matPtr[i].bs = b;
	}
	else if (strncmp("Ns", inputLine, 2) == 0) {
	    count = sscanf(inputLine, "Ns %f", &spec);
	    if (count != 1)
		continue;
	    matPtr[i].spec = spec;
	}
    }
    i++;

    matPtr[i].name[0] = 0;		/* After last valid entry */
    matPtr[i].ra = 0.0f;
    matPtr[i].ga = 0.1f;
    matPtr[i].ba = 0.0f;
    matPtr[i].rd = 0.2f;
    matPtr[i].gd = 1.0f;
    matPtr[i].bd = 0.0f;
    matPtr[i].ad = 1.0f;
    matPtr[i].rs = 1.0f;
    matPtr[i].gs = 0.8f;
    matPtr[i].bs = 0.0f;
    matPtr[i].spec = 25.0f;

    return matPtr;
} /* End of readMaterials */



/*
 * fileOpen
 *
 *	Attempts to fopen the file in many locations, dictated by an internal
 *	paths array.  fileOpen returns the first successful opened file in
 *	the path or NULL if the file was not found.
 *
 * Assumes strlen(path + filename + NULL) < 256
 */

FILE *
fileOpen(const char *filename,
    const char *mode)
{
    static char *paths[] = {
	"./",
	"../../data/",
	NULL
	};
    char fullFilename[256];
    int index;
    FILE *retVal;

    for (index=0; paths[index] != NULL; index++) {
	strcpy (fullFilename, paths[index]);
	strcat (fullFilename, filename);
	retVal = fopen(fullFilename, mode);
        if (retVal != NULL)
	    break;
    }

    return retVal;
} /* End of fileOpen */



/*
 * modelSpecialKeys
 *
 *	Deal with arrow keys and such
 */

/*ARGSUSED1*/
void
modelSpecialKeys(int key,
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
} /* End of modelSpecialKeys */



/*
 * modelKeys
 *
 *      Routine to deal with the keyboard
 */

/*ARGSUSED1*/
void
modelKeys(unsigned char key,
    int x,
    int y)
{
    switch (key) {

   /* Scale object */

      case '+':
      case '=':
        model->scaleObj *= 1.05;
        break;

      case '-':
      case '_':
        model->scaleObj *= 0.95;
        break;

   /* Move back clipping plane in/out */

      case '{':
      case '[':
	model->backPlaneDist *= 1.0f - TRANSLATE_VALUE;
	if (model->backPlaneDist < model->frontPlaneDist)
	    model->backPlaneDist = model->frontPlaneDist;
	model->backPlaneZ = model->eyeballZ - model->backPlaneDist;
	break;
      case '}':
      case ']':
	model->backPlaneDist *= 1.0f + TRANSLATE_VALUE;
	model->backPlaneZ = model->eyeballZ - model->backPlaneDist;
	break;

   /* Move front clipping plane in/out */

      case ':':
      case ';':
	model->frontPlaneDist *=  1.0f - TRANSLATE_VALUE;
	model->frontPlaneZ = model->eyeballZ - model->frontPlaneDist;
	break;
      case '\'':
      case '\"':
	model->frontPlaneDist *= 1.0f + TRANSLATE_VALUE;
	if (model->backPlaneDist < model->frontPlaneDist)
	    model->frontPlaneDist = model->backPlaneDist;
	model->frontPlaneZ = model->eyeballZ - model->frontPlaneDist;
	break;

    /* Move eye location in/out */

      case '>':
      case '.':
	model->eyeballZ *= 1.0f - TRANSLATE_VALUE;
	if (model->eyeballZ < model->frontPlaneZ)
	    model->eyeballZ /= 1.0f - TRANSLATE_VALUE;
	model->frontPlaneDist = model->eyeballZ - model->frontPlaneZ;
	model->backPlaneDist = model->eyeballZ - model->backPlaneZ;
	break;
      case '<':
      case ',':
	model->eyeballZ *= 1.0f + TRANSLATE_VALUE;
	model->frontPlaneDist = model->eyeballZ - model->frontPlaneZ;
	model->backPlaneDist = model->eyeballZ - model->backPlaneZ;
	break;

    /* Move screen location in/out */

      case '9':
      case '(':
	model->screenZ += SCREEN_SHIFT_VALUE;
	break;
      case '0':
      case ')':
	model->screenZ -= SCREEN_SHIFT_VALUE;
	break;

    /* Decrease/Increase Eye distance */

      case 'o':
      case 'O':
        model->halfEyeDist *= 1.0f - TRANSLATE_VALUE;
        break;
      case 'p':
      case 'P':
        model->halfEyeDist *= 1.0f + TRANSLATE_VALUE;
        break;

    /* Toggle Stereo mode */

      case 't':
      case 'T':
	if (model->stereoSetting == STEREO_ON)
	    model->stereoSetting = STEREO_OFF;
	else
	    model->stereoSetting = STEREO_ON;
	modelMenu(model->stereoSetting);
	break;

      case 'f':
      case 'F':
        displayInfo();
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
} /* End of modelKeys */



/*
 * modelMenu
 *
 *	Menu processing code.  Sends most requests to a recursive-friendly
 *	setMode function for handling.
 */

void
modelMenu(int value)
{
    switch (value) {
      case DRAW_SHUTTLE:
	model->objectSetting = value;
	readObjData("shuttle.obj");
	break;

      case DRAW_X29:
	model->objectSetting = value;
	readObjData("x29.obj");
	break;

      case DRAW_PORSCHE:
	model->objectSetting = value;
	readObjData("porsche.obj");
	break;

    /* Stereo mode */

      case STEREO_OFF:
	stereoMode = MONO;
	model->stereoSetting = STEREO_OFF;
	glutSetMenu (model->menuIdent);
	glutChangeToMenuEntry(model->stereoMenuNum,
            "Enable Stereo Mode", STEREO_ON);
	break;

      case STEREO_ON:
	stereoMode = LEFT;
	model->stereoSetting = STEREO_ON;
	glutSetMenu (model->menuIdent);
	glutChangeToMenuEntry(model->stereoMenuNum,
            "Disable Stereo Mode", STEREO_OFF);
	break;

      case QUIT:
	exit(0);

      default:
	break;
    } /* End of switch on menu value */

    /* Make sure that we redraw */
    glutPostRedisplay();

} /* End of modelMenu */



/*
 * modelMotion
 *
 *	No changes to mouse button, but it is moving with at least
 *	one button down.
 */

void
modelMotion(int x,
    int y)
{
	inMotionCallback = 1;
    modelMouse(oldButton, oldState, x, y);
	inMotionCallback = 0;
} /* End of modelMotion */




/*
 * modelVisibility
 *
 *      The visibility of the window has changed.
 */

void
modelVisibility(int state)
{
    if (state == GLUT_NOT_VISIBLE) {
	model->animateFlag = 0;
        if (view->animateFlag == 0)
            glutIdleFunc(NULL);
    }
    else {
	model->animateFlag = 1;
        glutIdleFunc(animate);
    }
} /* End of modelVisibility */



/*
 * modelReshape
 *
 *	Change the size and shape of the window.  Use correct math to
 *	keep relative sizes now matter what the window is shaped like.
 */

void
modelReshape(int w,
    int h)
{
    model->windowWidth = (GLdouble)w;
    model->windowHeight = (GLdouble)h;

    /* Notify OpenGL as to where we want to draw this */
    glViewport(0, 0, (GLint)model->windowWidth, (GLint)model->windowHeight);
} /* End of modelReshape */



/*
 * modelSetView
 *
 *	Set up the proper view.
 */

void
modelSetView(void)
{
    /* Compute a correct projection matrix */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    /* Compute adjustment factor */
    /* Ratio: front plane to screen */
    model->distanceAdjust = model->frontPlaneDist / model->eyeballZ
	/ PIXELS_PER_INCH;

    /* Prepare near-plane corner points for use here and in viewDisplay */
    model->left = -model->windowWidth * model->distanceAdjust;
    model->right = model->windowWidth * model->distanceAdjust;
    model->bottom = -model->windowHeight * model->distanceAdjust;
    model->top = model->windowHeight * model->distanceAdjust;

    glFrustum(model->left, model->right, model->bottom, model->top,
	model->frontPlaneDist, model->backPlaneDist);
} /* End of modelSetView */


/* End of frustum_model.c */
