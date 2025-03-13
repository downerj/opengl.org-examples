/*
 * lineaa.c
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
 *	Read in various wire-frame models and render them using different
 *	anti-aliasing techniques and depth buffering.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>

#include "modelcontext.h"
#include "callbacks.h"
#include "fileread.h"


/* The data-set for the model */
ModelContext *model;

/* Percent which intensity is scaled up or down by */
#define INTENSITY_SCALE 0.1f

/* Object transformation data */
static GLdouble rotAngle = -90.0;
static GLdouble rotAxis[3] = { 1.0, 0.0, 0.0 };

static float intensity = 1.0;              /* Default intensity */
static GLfloat defaultColorsFlag = 0;      /* Used if no colors are defined */
static GLfloat defaultColorRed = 0.5;
static GLfloat defaultColorGreen = 0.5;
static GLfloat defaultColorBlue = 0.5;


/* Menu definitions */
enum {
    DRAW_BEGIN = 1000,
    DRAW_F15,
    DRAW_ENTERPRISE,
    DRAW_BAY_AREA,
    DRAW_HAWAII,
    DRAW_END,

    AA_BEGIN,
    AA_BLEND_CONST,
    AA_BLEND_ARB,
    AA_OFF,
    AA_END,

    INTENSITY_FULL,
    INTENSITY_INCREASE,
    INTENSITY_DECREASE, 

    Z_ON,
    Z_OFF,

    BLEND_ON,
    BLEND_OFF,

    QUIT
};

/* Menu entries for submenus and "toggle" modes */
static int menuIdent;  
static int objectMenuIdent; 
static int aaMenuIdent;  
static int intensityMenuIdent;
static int depthMenuNum;
static int blendMenuNum;

/* Current settings for each */
static int objectSetting = DRAW_F15;
static int aaSetting = AA_OFF;
static int depthSetting = Z_OFF;

/* Function prototypes */
void display(void);
void keys(unsigned char key, int x, int y);
void menu(int value);
void setMode(int value);
void reshape(int w, int h);
void parseArgs(int argc, char *argv[]);
void displayHelp(void);


/*
 * main
 */

int
main(int argc,
    char *argv[])
{
    /* Display help listing on startup */
    displayHelp();

    /* Allocate modelcontext */
    model = (ModelContext *)calloc(1, sizeof(ModelContext)); 

    if (model == NULL) {
        printf ("Not enough memory count be allocated for model data\n");
        exit (-1);
    }

    /* Go grab the data first */
    readLineData("f15");

    /* Initialize the model context data */
    model->scaleObj = 10.0;		/* Start at 10 inches */
    model->needToUpdateViewMat = 0;
    model->pointerMotion = 0;
    model->windowWidth = 640;
    model->windowHeight = 480;
    model->minimumScale = MINIMUM_SCALE;

    /* Override defaults with command-line arguments */
    parseArgs (argc, argv);

    /* Initialize: */
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize((GLint)model->windowWidth, (GLint)model->windowHeight);
    glutInitWindowPosition(1, 1);
    glutCreateWindow("Line Anti-Aliasing");

    /* Go do all of the projection computations */
    reshape((GLint)model->windowWidth, (GLint)model->windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glGetDoublev(GL_MODELVIEW_MATRIX, model->rotMat); /* Initial rotation */

    /* Look at the model from the defined position */
    gluLookAt(0.0, 0.0, EYE_BACK,
	0.0, 0.0, 0.0,
	0.0, 1.0, 0.0);

    /* Set the screen colors */
    glClearColor(0.0, 0.0, 0.0, 1.0);

    /* Specify all of the callback routines */
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keys);
    glutSpecialFunc(specialKeysCallback);
    glutMouseFunc(mouseCallback);
    glutMotionFunc(motionCallback);
    glutVisibilityFunc(visibilityCallback);

    /* Create a menu */

    /* First create the submenus */
    objectMenuIdent = glutCreateMenu(menu);
    glutAddMenuEntry("F15", DRAW_F15);
    glutAddMenuEntry("Enterprise", DRAW_ENTERPRISE);
    glutAddMenuEntry("Bay Area", DRAW_BAY_AREA);
    glutAddMenuEntry("Hawaii", DRAW_HAWAII);

    aaMenuIdent = glutCreateMenu(menu);
    glutAddMenuEntry("Line Antialiasing, Blend Constant", AA_BLEND_CONST);
    glutAddMenuEntry("Line Antialiasing, Blend Arbitrary", AA_BLEND_ARB);
    glutAddMenuEntry("Disable Line Antialiasing", AA_OFF);

    intensityMenuIdent = glutCreateMenu(menu);
    glutAddMenuEntry("Full Intensity", INTENSITY_FULL);
    glutAddMenuEntry("Increase Intensity", INTENSITY_INCREASE);
    glutAddMenuEntry("Decrease Intensity", INTENSITY_DECREASE);

    /* Now create the main menu */
    menuIdent = glutCreateMenu(menu);
    glutAddSubMenu("Objects", objectMenuIdent);
    glutAddSubMenu("Anti-Aliasing Method", aaMenuIdent);
    glutAddSubMenu("Intensity", intensityMenuIdent);

    glutAddMenuEntry("Enable Depth Buffer", Z_ON);
    depthMenuNum = glutGet(GLUT_MENU_NUM_ITEMS);

    glutAddMenuEntry("Disable Blending", BLEND_OFF);
    blendMenuNum = glutGet(GLUT_MENU_NUM_ITEMS);

    glutAddMenuEntry("Quit", QUIT);

    glutAttachMenu(GLUT_RIGHT_BUTTON);

    /* Set up for AA lines */
    setMode(aaSetting);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glutMainLoop();

    free (model);

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
    register int i, j;
    register int nextIndex;		/* Next index that has a color */

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

    /* Now draw the model */
    if (defaultColorsFlag)
	glColor3f(defaultColorRed * intensity, 
	    defaultColorGreen * intensity,
	    defaultColorBlue * intensity);
    i = 0;
    j = 0;
    nextIndex = model->colorList[j].index;
    do {
	glBegin(GL_LINE_STRIP);
	    /* Select a new color, if needed */
	    if (i == nextIndex) {
		glColor3f(model->colorList[j].rd * intensity, 
		    model->colorList[j].gd * intensity,
		    model->colorList[j].bd * intensity);
		j++;
		nextIndex = model->colorList[j].index;
	    }

	    /* Draw a linestrip until a 'move' is reached */
	    glVertex3fv(&model->vertexList[i++].x);
	    while (model->vertexList[i].draw) {
		/* Change color if required */
		if (i == nextIndex) {
		    glColor3f(model->colorList[j].rd * intensity,
			model->colorList[j].gd * intensity,
			model->colorList[j].bd * intensity);
		    j++;
		    nextIndex = model->colorList[j].index;
		}
		glVertex3fv(&model->vertexList[i++].x);
	    }
	glEnd();
    } while (i < model->vertexCount);

    glPopMatrix();

    glutSwapBuffers();

    model->pointerMotion = 0;			/* We just redrew */
} /* End of display */



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
	model->scaleObj *= 1.05;
	break;

      case '-':
	model->scaleObj *= 0.95;
	break;

      case '[':
      case '{':
	setMode(INTENSITY_DECREASE);
	break;

      case ']':
      case '}':
	setMode(INTENSITY_INCREASE);
	break;

      case 'i':
      case 'I':
	setMode(INTENSITY_FULL);
	break;

      case 'o':
	/* Next object */
	objectSetting++;
	if (objectSetting == DRAW_END)
	    objectSetting = DRAW_BEGIN + 1;
	menu(objectSetting);
	break;
      case 'O':
	/* Previous object */
	objectSetting--;
	if (objectSetting == DRAW_BEGIN)
	    objectSetting = DRAW_END - 1;
	menu(objectSetting);
	break;

      case 'a':
	/* Next antialiasing mode */
	aaSetting++;
	if (aaSetting == AA_END)
	    aaSetting = AA_BEGIN + 1;
	menu(aaSetting);
	break;
      case 'A':
	/* Previous antialiasing mode */
	aaSetting--;
	if (aaSetting == AA_BEGIN)
	    aaSetting = AA_END - 1;
	menu(aaSetting);
	break;

      case 'z':
      case 'Z':
      case 'd':
      case 'D':
	/* Toggle depth-buffering */
	if (depthSetting == Z_ON)
	    depthSetting = Z_OFF;
	else
	    depthSetting = Z_ON;
	menu(depthSetting);
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
 * menu
 *
 *	Menu processing code.  Sends most requests to a recursive-friendly
 *	setMode function for handling.
 */

void
menu(int value)
{
    switch (value) {
      case QUIT:
	exit(0);

      default:
	setMode (value);
	break;
    } /* End of switch on menu value */

    /* Make sure that we redraw */
    glutPostRedisplay();

} /* End of menu */




/*
 * setMode
 *
 *	Called by the menus to set varioius modes based on the menu values.
 *	May be called elsewhere (like keyboard handler).  May be called
 *	recursively to make sure that all of the right stuff is set.
 */

void
setMode(int value)
{
    switch (value) {

    /* Objects */
      case DRAW_F15:
	readLineData("f15");
	rotAngle = -90.0;
	rotAxis[0] = 1.0;
	rotAxis[1] = 0.0;
	rotAxis[2] = 0.0;
	defaultColorsFlag = 0;
	objectSetting = value;
	break;

      case DRAW_ENTERPRISE:
	readLineData("enterprise");
	rotAngle = 180.0f;
	rotAxis[0] = 0.0f;
	rotAxis[1] = 1.0f;
	rotAxis[2] = 0.0f;
	defaultColorsFlag = 1;
	defaultColorRed = 0.1f;
	defaultColorGreen = 0.4f;
	defaultColorBlue = 1.0f;
	objectSetting = value;
	break;

      case DRAW_BAY_AREA:
	readLineData("bayarea");
	rotAngle = 0.0f;
	rotAxis[0] = 0.0f;
	rotAxis[1] = 1.0f;
	rotAxis[2] = 0.0f;
	defaultColorsFlag = 1;
	defaultColorRed = 0.6f;
	defaultColorGreen = 0.6f;
	defaultColorBlue = 0.1f;
	objectSetting = value;
	break;

      case DRAW_HAWAII:
	readLineData("hawaii");
	rotAngle = 0.0f;
	rotAxis[0] = 0.0f;
	rotAxis[1] = 1.0f;
	rotAxis[2] = 0.0f;
	defaultColorsFlag = 1;
	defaultColorRed = 0.3f;
	defaultColorGreen = 0.8f;
	defaultColorBlue = 0.1f;
	objectSetting = value;
	break;

    /* Anti-aliasing Modes */

      case AA_BLEND_CONST:
	glEnable(GL_LINE_SMOOTH);
	setMode(BLEND_ON);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	setMode(Z_OFF);
	aaSetting = value;
	break;

      case AA_BLEND_ARB:
	glEnable(GL_LINE_SMOOTH);
	setMode(BLEND_ON);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	setMode(Z_OFF);
	aaSetting = value;
	break;

      case AA_OFF:
	glDisable(GL_LINE_SMOOTH);
	setMode(BLEND_OFF);
	setMode(Z_ON);
	aaSetting = value;
	break;

    /* Intensity Settings */
      case INTENSITY_FULL:
	intensity = 1.0f;
	break;

      case INTENSITY_INCREASE:
	intensity *= 1.f + INTENSITY_SCALE;
	if (intensity > 1.0f)
	    intensity = 1.0f;
	break;

      case INTENSITY_DECREASE:
	intensity *= 1.f - INTENSITY_SCALE;
	break;

    /* Depth Buffer mode */

      case Z_ON:
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	depthSetting = value;
	glutSetMenu (menuIdent);
	glutChangeToMenuEntry(depthMenuNum, "Disable Depth Buffer", Z_OFF);
	break;

      case Z_OFF:
	glDisable(GL_DEPTH_TEST);		/* Could be left enabled... */
	glDepthMask(GL_FALSE);
	depthSetting = value;
	glutSetMenu (menuIdent);
	glutChangeToMenuEntry(depthMenuNum, "Enable Depth Buffer", Z_ON);
	break;

    /* Blend mode */

      case BLEND_ON:
	glEnable(GL_BLEND);
	glutSetMenu (menuIdent);
	glutChangeToMenuEntry(blendMenuNum, "Disable Blending", BLEND_OFF);
	break;

      case BLEND_OFF:
	glDisable(GL_BLEND);
	glutSetMenu (menuIdent);
	glutChangeToMenuEntry(blendMenuNum, "Enable Blending", BLEND_ON);
	break;
    } /* End of switch on mode value */

} /* End of objectMenu */




/*
 * reshape
 *
 *	Change the size and shape of the window.  Use correct math to
 *	keep relative sizes now matter what the window is shaped like.
 */

void
reshape(int w, int h)
{
    GLdouble distanceAdjust;		/* Front plane is NOT at screen */

    model->windowWidth = (GLdouble)w;
    model->windowHeight = (GLdouble)h;

    /* Notify OpenGL as to where we want to draw this */
    glViewport(0, 0, (GLint)model->windowWidth, (GLint)model->windowHeight);

    /* Compute a correct projection matrix */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    /* Compute adjustment factor */
    distanceAdjust = 2.0 / EYE_BACK;	/* Ratio: front plane to screen */
    /* Convert to measuring in inches, then adjust for center to edge */
    distanceAdjust /= PIXELS_PER_INCH / 2.0;

    /* Front clip at 2, back at 1000 */
    glFrustum(-model->windowWidth * distanceAdjust, 
	model->windowWidth * distanceAdjust,
	-model->windowHeight * distanceAdjust, 
	model->windowHeight * distanceAdjust, 2.0, 1000.0);

    /* Don't forget to switch back to the "standard" matrix mode */
    glMatrixMode(GL_MODELVIEW);

} /* End of reshape */



/*
 * parseArgs
 *
 *	Set defaults based on the command-line arguments.
 */

void parseArgs(int argc,
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
}



/*
 * displayHelp
 *
 *	Tells the user what they can do.
 */
void
displayHelp(void)
{
    printf("------------------\n");
    printf("Line Anti-Aliasing\n\n");
    printf("Mouse: Left   = motion\n");
    printf("       Middle = scale\n");
    printf("       Right  = menu\n\n");
    printf("Keys: Arrows  = translate\n");
    printf("      +, -    = scale up, down\n");
    printf("      [, ]    = decrease, increase intensity\n");
    printf("      i, I    = full intensity\n");
    printf("      o, O    = next, previous object\n");
    printf("      a, A    = switch line antialiasing mode\n");
    printf("      d, D    = toggle depth-buffering (also z, Z)\n");
    printf("      h, H, ? = print this message\n");
    printf("      q, Q, esc = quit\n\n");
} /* End of displayHelp */

/* End of lineaa.c */
