/*
 * inaccuracies.c
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
 *	Simple program to show inaccuracies in mathematically correct models.
 */

#include <stdio.h>
#include <stdlib.h>

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

#include <GL/glut.h>


/* Constants & initial values for frustum calculations */
#define FRONT_PLANE 2.0	
#define BACK_PLANE 100.0
#define EYE_BACK 36.0
#define PIXELS_PER_INCH 100.0

#define INITIAL_SCALE_VALUE 1.0

#define HALF_PLANE_WIDTH 12.0	/* 1/2 the plane width */
#define HALF_PLANE_HEIGHT 12.0	/* 1/2 the plane height */
#define PLANE_COLUMNS 36.0	/* Number of rows and columns in the plane */
#define PLANE_ROWS 1.0	
#define DISTANCE_BETWEEN_PLANES 0.05
#define T_POINT_PERCENT 0.6	/* normalized offset for the vertex which */
				/* forms the T intersection, 0.5 is halfway */

/* Translation and scaling speeds related values */
#define SCALE_SPEED 0.003

/* Menu definitions */
enum {
    PLANES_EQUAL,
    PLANES_FAR,

    KEYS_ON,
    KEYS_OFF,

    QUIT
};

static int mouseX, mouseY;

/* Menu identifiers and settings */
static int planesMenuNum;
static int keyTrianglesMenuNum;
static int planesSetting = PLANES_FAR;
static int keyTrianglesSetting = KEYS_OFF;

static int zoomFlag = 0;
static float zoomFactor = 8.0;

static GLdouble rotX, rotY;
static GLdouble scaleObj;
static GLdouble rotMat[16];
static GLdouble windowWidth, windowHeight;
static int needToUpdateViewMat;
static int pointerMotion;		/* Flag, multiple mouse moves/redraw */


static int inMotionCallback = 0;	/* Prevents annoying GLUT warning message */

/* Function prototypes */
void display(void);
void displayMain(void);
void displayZoom(void);
void animate(void);
void menu(int value);
void mouse(int button, int state, int x, int y);
void specialKeys(int key, int x, int y);
void keys(unsigned char key, int x, int y);
void motion(int x, int y);
void visibility(int state);
void reshape(int w, int h);
void setView (void);
void setLights(int lights);
void displayHelp(void);


/*
 * main
 */

int
main(int argc, char *argv[])
{
    /* Display help listing on startup */
    displayHelp();

    /* Initialize: */
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE); 

    scaleObj = INITIAL_SCALE_VALUE;
    needToUpdateViewMat = 0;
    pointerMotion = 0;
    windowWidth = 800;
    windowHeight = 800;

    glutInitWindowPosition(0, 0);
    glutInitWindowSize((GLint)windowWidth, (GLint)windowHeight);
    glutCreateWindow("Inaccuracies");

    /* Go do all of the projection computations */
    reshape((GLint)windowWidth, (GLint)windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glGetDoublev(GL_MODELVIEW_MATRIX, rotMat);	/* Initial rotation */

    /* Set the screen colors */
    glClearColor(0.0, 0.0, 0.0, 1.0);

    /* Set up for drawing solids */
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);

    /* Specify all of the callback routines */
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keys);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutVisibilityFunc(visibility);

    /* Create a menu */
    glutCreateMenu(menu);
    glutAddMenuEntry("Set Plane Positions Equal", PLANES_EQUAL);
    planesMenuNum = glutGet(GLUT_MENU_NUM_ITEMS);

    glutAddMenuEntry("Draw Key Triangles", KEYS_ON);
    keyTrianglesMenuNum = glutGet(GLUT_MENU_NUM_ITEMS);

    glutAddMenuEntry("Quit", QUIT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
 
    glutMainLoop();

    return 0;

} /* End of main */



/*
 * displayMain
 *
 *	The display loop code
 */

void
displayMain(void)
{
    GLdouble incCol, incRow;
    GLdouble i, j;
    GLdouble backPlaneZ;
    int drawnKeys = 1;

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

   /* Look at the model from the defined position */
    gluLookAt(0.0, 0.0, EYE_BACK,
	0.0, 0.0, 0.0,
	0.0, 1.0, 0.0);

    setView();

    glDrawBuffer(GL_BACK);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);

    glPushMatrix();

     /* Set up the view window's scale and rotation */
    glScaled(scaleObj, scaleObj, scaleObj);

    if (needToUpdateViewMat) {
	/* Build a proper cumulative rotation for automatic motion */
	glPushMatrix();
	glLoadIdentity();
	glRotated(rotX * 0.2, 0.0, 1.0, 0.0);
	glRotated(rotY * 0.2, 1.0, 0.0, 0.0);
	glMultMatrixd(rotMat);
	glGetDoublev(GL_MODELVIEW_MATRIX, rotMat);
	glPopMatrix();
	needToUpdateViewMat = 0;
    }
    glMultMatrixd(rotMat);

    /* Put in the planes */
    glColor3f(0.05f, 0.02f, 0.10f);
    glDisable(GL_LIGHTING);
    incCol = HALF_PLANE_WIDTH * 2.0 / PLANE_COLUMNS;
    incRow = HALF_PLANE_HEIGHT * 2.0 / PLANE_ROWS;

    drawnKeys = 0;
    for (j=HALF_PLANE_HEIGHT; j>-HALF_PLANE_HEIGHT; 
	    j-=incRow) {
	for (i=-HALF_PLANE_WIDTH; i<HALF_PLANE_WIDTH; 
		i+=incCol) {
	    if (!drawnKeys && keyTrianglesSetting == KEYS_ON &&
		    i > 0.0) {
		/* Draw the 'key' triangles strip */
		glBegin(GL_TRIANGLES);
		    glColor3f(0.9f, 0.1f, 0.5f);
		    glVertex3f(i, j, 0.0f);
		    glVertex3f(i+incCol, j, 0.0f);
		    glVertex3f(i, j - incRow * T_POINT_PERCENT, 0.0f);

		    glColor3f(0.5f, 0.8f, 0.0f);
		    glVertex3f(i, j - incRow * T_POINT_PERCENT, 0.0f);
		    glVertex3f(i+incCol, j, 0.0f);
		    glVertex3f(i+incCol, j - incRow, 0.0f);

		    glColor3f(0.4f, 0.2f, 0.95f);
		    glVertex3f(i, j - incRow, 0.0f);
		    glVertex3f(i, j - incRow * T_POINT_PERCENT, 0.0f);
		    glVertex3f(i+incCol, j - incRow, 0.0f);

		    glColor3f(0.05f, 0.02f, 0.10f);
		    drawnKeys = 1;
		glEnd();
	    }
	    else {
	    	glBegin(GL_TRIANGLE_FAN);
		    glVertex3f(i, j - incRow * T_POINT_PERCENT, 0.0);
		    glVertex3f(i, j, 0.0);
		    glVertex3f(i+incCol, j, 0.0);
		    glVertex3f(i+incCol, j - incRow, 0.0);
		    glVertex3f(i, j - incRow, 0.0);
	    	glEnd();
	    }
	}
    }

    /* Draw the back plane, 90% size of front plane */
    if (planesSetting == PLANES_FAR)
    	backPlaneZ = -DISTANCE_BETWEEN_PLANES;
    else
	backPlaneZ = 0.0;

    glColor3f(1.0, 1.0, 0.0);

    /* Use 'inc' vars to hold the shrunken down plane dims */
    incRow = HALF_PLANE_HEIGHT * 0.9;
    incCol = HALF_PLANE_WIDTH * 0.9;
    glBegin(GL_QUADS);
	glVertex3d(-incCol, -incRow, backPlaneZ);
	glVertex3d(-incCol, incRow, backPlaneZ);
	glVertex3d(incCol, incRow, backPlaneZ);
	glVertex3d(incCol, -incRow, backPlaneZ);
    glEnd();

    glPopMatrix();

    pointerMotion = 0;			/* We just redrew */
} /* End of displayMain */



/*
 * displayZoom
 *
 *	Display callback for zoom 'window', which is currently not an 
 *	official window or sub-window, but rather just 'faked' by writing
 *	the pixels to a corner of the main window.  This should be moved
 *      into its own window for better behaviour.
 */ 

void 
displayZoom(void)
{
    glPushMatrix();
    glDisable(GL_DEPTH_TEST);

    /* This is the first working version of the zoom, so it's a bit klunky...
     *
     * The values -16 and 8 are 'magic' and depend on 800x800 view window 
     * If the time arises, it would be good to eliminate this dependency
     * and preferably get the zoom window as its own window.
     * '100' is 200*0.5 and '200' is the dim of the zoom window.
     */ 
    glRasterPos2i(-16, 8);
    glPixelZoom(zoomFactor, zoomFactor);
    glCopyPixels(mouseX-(100.0f/zoomFactor), mouseY-(100.0f/zoomFactor),
	200.0f/zoomFactor, 200.0f/zoomFactor, GL_COLOR); 
    glPixelZoom(1.0, 1.0);

    glEnable(GL_DEPTH_TEST);   
    glPopMatrix();
}



/* 
 * display
 *
 *	The display callback
 */

void 
display(void)
{
    displayMain();
    if (zoomFlag) {
	displayZoom();
    }
    glutSwapBuffers();
}



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
    static int ld = 0; /* Left/middle down */
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

		rotX = (GLdouble)(x - xOrigin) * 0.2;
		rotY = (GLdouble)(y - yOrigin) * 0.2;

		/* Build a proper cumulative rotation */
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glRotated(rotX, 0.0, 1.0, 0.0);
		glRotated(rotY,  1.0, 0.0, 0.0);
		glMultMatrixd(rotMat);
		glGetDoublev(GL_MODELVIEW_MATRIX, rotMat);
		glPopMatrix();

		if (pointerMotion == 0) {
		    xhPtr = (xhPtr + 1) & 0x0f;
		    xhCount++;
		    pointerMotion = 1;	/* For > 1 move/redraw */
		    xHist[xhPtr] = 0;
		    yHist[xhPtr] = 0;
		}
		xHist[xhPtr] += x - xOrigin;
		yHist[xhPtr] += y - yOrigin;

		glutPostRedisplay();	/* Force redraw */
	    }

	    getTime(oldTime);
	    xOrigin = x;		/* Remember position */
	    yOrigin = y;
	    glutIdleFunc(NULL);	/* No automatic motion until release */
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
		    rotX = (GLdouble)xSum * 0.1;
		    rotY = (GLdouble)ySum * 0.1;
		    glutIdleFunc(animate);
		}
		else {
		    /* Insufficient history, stop! */
		    rotX = 0.0;
		    rotY = 0.0;
		    glutIdleFunc(NULL);
		}
	    }
	    ld = 0;
	}
	break;

      case GLUT_MIDDLE_BUTTON:
	if (state == GLUT_DOWN) {
	    zoomFlag = 1;
	    mouseX = x;
	    mouseY = windowHeight - y;
	}
	else {
	    zoomFlag = 0;
	}
	glutPostRedisplay();
	break;

      /* Right button is caught by the menu handler */

    } /* End of switch on button */

} /* End of mouse */



/*
 * specialKeys
 *
 *	Deal with arrow keys and such
 */

/*ARGSUSED1*/
void
specialKeys(int key,
    int x,
    int y)
{
    switch (key) {

      case GLUT_KEY_LEFT:
	glTranslated(-0.05 * scaleObj, 0.0, 0.0);
	glutPostRedisplay();
	break;

      case GLUT_KEY_RIGHT:
	glTranslated(0.05 * scaleObj, 0.0, 0.0);
	glutPostRedisplay();
	break;

      case GLUT_KEY_UP:
	glTranslated(0.0, 0.05 * scaleObj, 0.0);
	glutPostRedisplay();
	break;

      case GLUT_KEY_DOWN:
	glTranslated(0.0, -0.05 * scaleObj, 0.0);
	glutPostRedisplay();
	break;

    } /* End of switch on special key */
} /* End of specialKeys */



/*
 * keys
 *
 *      Routine to deal with the keyboard
 */

/*ARGSUSED1*/
void 
keys(unsigned char key, 
    int x, 
    int y)
{
    switch (key) {

      case '+':
      case '=':
        scaleObj *= 1.05;
        break;

      case '-':
      case '_':
        scaleObj *= 0.95;
        break;
 
      case 'p':
      case 'P':
	if (planesSetting == PLANES_EQUAL)
	    planesSetting = PLANES_FAR;
	else
	    planesSetting = PLANES_EQUAL;
	menu(planesSetting);
	break;

      case 'k':
      case 'K':
	if (keyTrianglesSetting == KEYS_ON)
	    keyTrianglesSetting = KEYS_OFF;
	else
	    keyTrianglesSetting = KEYS_ON;
	menu(keyTrianglesSetting);
	break;

      case '?':
      case 'h':
      case 'H':
        displayHelp();
        break;

      case '[':
      case '{':
	zoomFactor -= 1.0;
	if (zoomFactor < 1.0)
	    zoomFactor = 1.0;
	break;

      case ']':
      case '}':
	zoomFactor += 1.0;
	break;

      case 27:
      case 'q':
      case 'Q':
        exit(0);

    } /* End of switch on key */

    glutPostRedisplay();
} /* End of keys */




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
 * animate
 *
 *	The animation function.
 */

void
animate(void)
{
    needToUpdateViewMat = 1;
    glutPostRedisplay();
} /* End of animate */



/*
 * visibility
 *
 *      The visibility of the window has changed.
 */

void
visibility(int state)
{
    if (state == GLUT_VISIBLE)
	glutIdleFunc(animate);
    else if (state == GLUT_NOT_VISIBLE)
	glutIdleFunc(NULL);
} /* End of visibility */


 
/*
 * reshape
 *
 *	Change the size and shape of the window.  Use correct math to
 *	keep relative sizes now matter what the window is shaped like.
 */

void
reshape(int w,
    int h)
{
    windowWidth = (GLdouble)w;
    windowHeight = (GLdouble)h;

    /* Notify OpenGL as to where we want to draw this */
    glViewport(0, 0, (GLint)windowWidth, (GLint)windowHeight);
} /* End of reshape */




/*
 * setView
 *
 *	Set up the proper view.
 */

void
setView(void)
{
    GLdouble distanceAdjust;    /* Front plane is NOT at screen */

    /* Compute a correct projection matrix */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    /* Compute adjustment factor */
    /* Ratio: front plane to screen */
    distanceAdjust = FRONT_PLANE / EYE_BACK; 

    /* Convert to measuring in inches, then adjust for center to edge */
    distanceAdjust /= PIXELS_PER_INCH / FRONT_PLANE;

    glFrustum(-windowWidth * distanceAdjust, 
	windowWidth * distanceAdjust, -windowHeight * 
	distanceAdjust, windowHeight * distanceAdjust,
	FRONT_PLANE, BACK_PLANE);	/* Front clip at (2), back at 100 */
} /* End of setView */



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
      case PLANES_EQUAL:
	planesSetting = value;
	glutChangeToMenuEntry(planesMenuNum, 
	    "Set Plane Positions Apart", PLANES_FAR);
	break;

      case PLANES_FAR:
	planesSetting = value;
	glutChangeToMenuEntry(planesMenuNum, 
	    "Set Plane Positions Equal", PLANES_EQUAL);
	break;

      case KEYS_OFF:
	keyTrianglesSetting = value;
	glutChangeToMenuEntry(keyTrianglesMenuNum, 
	    "Draw Key Triangles", KEYS_ON);
	break;

      case KEYS_ON:
	keyTrianglesSetting = value;
	glutChangeToMenuEntry(keyTrianglesMenuNum, 
	    "Hide Key Triangles", KEYS_OFF);
	break;

      case QUIT:
	exit(0);

      default:
	break;
    } /* End of switch on menu value */

    /* Make sure that we redraw */
    glutPostRedisplay();
} /* End of menu */



/*
 * displayHelp
 *
 *	Tells the user what they can do.
 */
void
displayHelp(void)
{
    printf("------------\n");
    printf("Inaccuracies\n");
    printf("\n");
    printf("Mouse: Left   = motion\n");
    printf("       Middle = zoom\n");
    printf("       Right  = menu\n");
    printf("Keys: +, -    = scale\n");
    printf("      p, P    = toggle plane distance (equal/apart)\n");
    printf("      k, K    = toggle drawing of 'key' triangles\n");
    printf("      [, ]    = adjust zoom value\n\n");
} /* End of displayHelp */


/* End of inaccuracies.c */
