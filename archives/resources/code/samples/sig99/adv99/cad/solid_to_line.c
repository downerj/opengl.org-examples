/*
 * solid_to_line.c
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
 *	and set various modes while the object moves.  Show different methods
 *      of generating wire-frame data for solid objects.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

/* Drawing State Flags */
#define DRAW_SOLID		0x0001
#define DRAW_WIREFRAME		0x0002
static int drawState = DRAW_SOLID;

/* Object transformation data */
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
    DRAW_F15,

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

    LIGHTS_ON,
    LIGHTS_OFF,

    Z_ON,
    Z_OFF,

    DOUBLE_BUFFER_ON,
    DOUBLE_BUFFER_OFF,

    QUIT
};


/* Menu entry numbers for submenus and toggle modes */
static int depthBufferMenuNum;
static int doubleBufferMenuNum;
static int lightingMenuNum;
static int menuIdent;
static int objectMenuIdent;
static int aaMenuIdent;
static int drawMenuIdent;

/* Current settings for each */
static int objectSetting = DRAW_SHUTTLE;
static int aaSetting = AA_BLEND_CONST;
static int lightSetting = LIGHTS_ON;
static int depthSetting = Z_OFF;
static int drawSetting = POLYGONS;
static int doubleBufferSetting = DOUBLE_BUFFER_ON;

/* Function prototypes */
void display(void);
void setView(void);
void drawImage(void);
void drawFPS(char *string);
void keys(unsigned char key, int x, int y);
void menu(int value);
void objectMenu(int value);
void aaMenu(int value);
void polyMenu(int value);
void drawMenu(int value);
void colorMenu(int value);
void setMaterial(int n);
void setState(int newState);
void setMode(int value);
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
    model->triangleFlag = 0;		/* Draw as polygons */

    /* Override defaults with command-line arguments */
    parseArgs (argc, argv);

    /* Initialize: */

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);

    glutInitWindowSize((GLint)model->windowWidth, (GLint)model->windowHeight);
    glutInitWindowPosition(1, 1);
    glutCreateWindow("Solid to Line");

    /* Go do all of the projection computations */
    reshapeCallback((GLint)model->windowWidth, (GLint)model->windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glGetDoublev(GL_MODELVIEW_MATRIX, model->rotMat);	/* Initial rotation */

    /* Set the screen model->colorList */
    glClearColor(0.0, 0.0, 0.0, 1.0);

    /* Set up for drawing solids */
    glDisable(GL_CULL_FACE);
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
    glutAddMenuEntry("Air Boat", DRAW_AIRBOAT);
    glutAddMenuEntry("Cessna", DRAW_CESSNA);
/*  glutAddMenuEntry("Crocodile", DRAW_CROC); */
    glutAddMenuEntry("Flamingo", DRAW_FLAMINGO);
    glutAddMenuEntry("Space Shuttle", DRAW_SHUTTLE);
    glutAddMenuEntry("Porsche", DRAW_PORSCHE);
    glutAddMenuEntry("F15", DRAW_F15);

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

    /* Now create the main menu */
    menuIdent = glutCreateMenu(menu);
    glutAddSubMenu("Objects", objectMenuIdent);
    glutAddSubMenu("Line Antialiasing", aaMenuIdent);
    glutAddSubMenu("Drawing Method", drawMenuIdent);

    glutAddMenuEntry("Disable Depth Buffer", Z_OFF);
    depthBufferMenuNum = glutGet(GLUT_MENU_NUM_ITEMS);

    glutAddMenuEntry("Single Buffering", DOUBLE_BUFFER_OFF);
    doubleBufferMenuNum = glutGet(GLUT_MENU_NUM_ITEMS);

    glutAddMenuEntry("Disable Lighting", LIGHTS_OFF);
    lightingMenuNum = glutGet(GLUT_MENU_NUM_ITEMS);

    glutAddMenuEntry("Quit", QUIT);

    glutAttachMenu(GLUT_RIGHT_BUTTON);

    readObjData("../../data/shuttle.obj");

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
    int          microseconds;			/* How long one frame takes */
    float        framesPerSecond;		/* How many frames we're getting */
	static int   timeSinceUpdate;		/* How long since fps update */
	static int   frames = -1;
	static char  perf[128];			/* String to display */

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

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
 *	Draw the full image here.  Allows stereo and such.
 */

void
drawImage(void)
{
    register int i, j;
    register int nextIndex;		/* Next index that has a color */
    register int currColor;


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
    if ((objectSetting >= DRAW_BEGIN) && (objectSetting <= DRAW_END) &&
	    (drawSetting < LINE_INDEPENDENT)) {
	/* Solid object as polygons */
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
	/* Wireframe object or solid object as wireframe */
	if (objectSetting > DRAW_END) {
	    do {
		glBegin(GL_LINE_STRIP);
		    if (i == nextIndex) {
			setColor(&model->colorList[j].ra,
			    &model->colorList[j].rd, &model->colorList[j].rs,
			    model->colorList[j].spec, 0);
			j++;
			nextIndex = model->colorList[j].index;
		    }
		    glVertex3fv(&model->vertexList[i++].x);
		    while (model->vertexList[i].draw) {
			if (i == nextIndex) {
			    setColor(&model->colorList[j].ra,
				&model->colorList[j].rd,
				&model->colorList[j].rs,
				model->colorList[j].spec, 0);
			    j++;
			    nextIndex = model->colorList[j].index;
			}
			glVertex3fv(&model->vertexList[i++].x);
		    }
		glEnd();
	    } while (i < model->vertexCount);
	} /* End of line object */
	else {
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
    }
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
	readObjData("../../data/airboat.obj");
	objectSetting = value;
	break;

      case DRAW_CESSNA:
	setState(DRAW_SOLID);
	readObjData("../../data/cessna.obj");
	objectSetting = value;
	break;

      case DRAW_CROC:
	setState(DRAW_SOLID);
	readObjData("../../data/croc.obj");
	objectSetting = value;
	break;

      case DRAW_FLAMINGO:
	setState(DRAW_SOLID);
	readObjData("../../data/flamingo.obj");
	objectSetting = value;
	break;

      case DRAW_SHUTTLE:
	setState(DRAW_SOLID);
	readObjData("../../data/shuttle.obj");
	objectSetting = value;
	break;

      case DRAW_PORSCHE:
	setState(DRAW_SOLID);
	readObjData("../../data/porsche.obj");
	objectSetting = value;
	break;

      case DRAW_F15:
	setState(DRAW_WIREFRAME);
	readLineData("../../data/f15");
	rotAngle = -90.0;
	rotAxis[0] = 1.0;
	rotAxis[1] = 0.0;
	rotAxis[2] = 0.0;
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
	setLights(1);
	glutSetMenu(menuIdent);
	glutChangeToMenuEntry(lightingMenuNum, "Disable Lighting", LIGHTS_OFF);
	break;

      case LIGHTS_OFF:
	lightSetting = value;
	setLights(0);
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
    }
} /* End of setMode */



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
    printf("-------------\n");
    printf("Solid to Line\n\n");
	printf("Mouse: Left   = motion\n");
	printf("       Middle = scale\n");
	printf("       Right  = menu\n");
	printf("Keys: Arrows  = translate\n");
	printf("      +, -    = scale up, down\n");
	printf("      o, O    = next, previous object\n");
	printf("      a, A    = switch line antialiasing mode\n");
	printf("      d, D    = togle double buffering\n");
	printf("      z, Z    = toggle depth-buffering\n");
	printf("      s, S    = Decrease, increase speed\n");
	printf("      h, H, ? = print this message\n");
	printf("      q, Q, esc = quit\n");
} /* End of displayHelp */

/* End of solid_to_line.c */
