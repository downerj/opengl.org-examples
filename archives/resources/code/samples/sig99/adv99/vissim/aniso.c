
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <GL/glut.h>
#include "../util/texture.h"

#ifdef WIN32
#ifndef M_PI
#define M_PI 3.14159265
#endif
#endif

/*
 * TEST PROGRAM 
 */

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if((error = glGetError()) != GL_NO_ERROR)                      \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

#define RAD(x) (((x)*M_PI)/180.)

enum {
    RESET,			/* reset to square texture map */
    TALLER,			/* make texture map taller */
    WIDER,			/* make texture map wider */
    EXIT			/* exit program */
};

/*
 * maximum number of mipmaps in aniso array 
 */
int maxSratios = 5;
int maxTratios = 5;
int texSindex = 0;		/* index into a particular texture */
int texTindex = 0;

int divisions = 8;
int grid = 1;			/* turn grid on if true */
int matrix = 0;			/* use non-uniform scaled texture matrix if true */
int three = 0;			/* use 3/4 ratio texture */

GLfloat viewangle = 0.f;	/* 0 -> 90 */
GLfloat viewXscale = 1.f;	/* minify -> 1 -> magnify */
GLfloat viewYscale = 1.f;	/* minify -> 1 -> magnify */

enum { X, Y, Z };
enum { CHANGE_ASPECT, CHANGE_SCALE };
int active;
int dblbuf = GL_TRUE;

int winwid = 512;
int winht = 512;

void
motion(int x, int y)
{

    switch (active) {
    case CHANGE_ASPECT:
	{
	    viewangle = (winht - y) * 135.f / winht - 45.f;
	    if (viewangle < -45.f)
		viewangle = -45.f;
	    if (viewangle > 90.f)
		viewangle = 90.f;
	    glutPostRedisplay();
	    break;
	}
    case CHANGE_SCALE:
	/*
	 * center of screen is 1:1 scaling varying by factor of 4 
	 */
	viewXscale = (x - winwid / 2) * 4.f / winwid;
	viewYscale = (winht / 2 - y) * 4.f / winht;
	glutPostRedisplay();
	break;
    }
}

void
mouse(int button, int state, int x, int y)
{
    /*
     * hack for 2 button mouse 
     */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;

    if (state == GLUT_DOWN)
	switch (button) {
	case GLUT_LEFT_BUTTON:	/* rotate texture to change aspect */
	    active = CHANGE_ASPECT;
	    motion(x, y);
	    break;
	case GLUT_MIDDLE_BUTTON:	/* scale texture to change magnification */
	    active = CHANGE_SCALE;
	    motion(x, y);
	    break;
	case GLUT_RIGHT_BUTTON:
	    /* menu */
	    break;
	}
}

void
reshape(int wid, int ht)
{
    winwid = wid;
    winht = ht;
    glViewport(0, 0, wid, ht);
}


/*
 * Called when window needs to be redrawn 
 */
void 
redraw(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    glPushMatrix();		/* start with view transform (from glulookat) */

    glTranslatef(0.f, -100.f, 100.f);
    glScalef(viewXscale, viewYscale, 1.f);	/* scale texture */
    glRotatef(viewangle, 1.f, 0.f, 0.f);	/* rotate texture */
    glTranslatef(0.f, 100.f, -100.f);

    glBegin(GL_QUADS);
    /*
     * floor 
     */
    glNormal3f(0.f, 1.f, 0.f);
    glTexCoord2i(0, 0);
    glVertex3f(-100.f, -100.f, 100.f);
    glTexCoord2i(1, 0);
    glVertex3f(100.f, -100.f, 100.f);
    glTexCoord2i(1, 1);
    glVertex3f(100.f, -100.f, -100.f);
    glTexCoord2i(0, 1);
    glVertex3f(-100.f, -100.f, -100.f);
    glEnd();

    glPopMatrix();

    CHECK_ERROR("redraw(): draw floor");

    if (grid) {
	int i;			/* to draw grid */
	GLfloat delta;		/* grid spacing */

	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
	glColor3f(1.f, 0.f, 0.f);

	delta = 200 / divisions;
	for (i = 0; i <= divisions; i++) {
	    /* draw horizontal lines */
	    glVertex3f(-100.f, -100 + delta * i, 100.f);
	    glVertex3f(100.f, -100 + delta * i, 100.f);

	    /* draw vertical lines */
	    glVertex3f(-100 + delta * i, -100.f, 100.f);
	    glVertex3f(-100 + delta * i, 100.f, 100.f);
	}
	glColor3f(1.f, 1.f, 1.f);
	glEnd();
	glEnable(GL_TEXTURE_2D);
    }
    /*
     * turn the color and depth buffer back on, set the stencil function 
     * to pass if the stencil values are set, then draw the reflection.
     */

    if (dblbuf)
	glutSwapBuffers();
    else
	glFlush();

    CHECK_ERROR("end of redraw()");
}


/*ARGSUSED1*/
void 
key(unsigned char key, int x, int y)
{
    static GLint oldtex = 0;	/* save previous texture id */

    switch (key) {
    case 'r':
    case 'R':
	glBindTexture(GL_TEXTURE_2D, 0);
	fprintf(stderr, "Reset Aspect Ratio to 1:1, bound texture 0\n");
	texSindex = 0;
	texTindex = 0;

	if (three) {		/* dropped out of 3/4 ratio texture; clean up state */
	    three = 0;
	    if (matrix) {	/* turn off matrix too */
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		matrix = 0;
	    }
	}
	glutPostRedisplay();
	break;

    case 't':
    case 'T':
	/*
	 * increment towards Height 
	 */
	if (texSindex)
	    texSindex--;
	else
	    texTindex++;
	if (texTindex >= maxTratios)
	    texTindex = maxTratios - 1;

	glBindTexture(GL_TEXTURE_2D, texSindex + maxSratios * texTindex);
	fprintf(stderr, "Change Aspect Ratio to %d:%d, bound texture %d\n",
		1 << texSindex, 1 << texTindex, texSindex + maxSratios * texTindex);

	if (three) {		/* dropped out of 3/4 ratio texture; clean up state */
	    three = 0;
	    if (matrix) {	/* turn off matrix too */
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		matrix = 0;
	    }
	}
	glutPostRedisplay();
	break;

    case 'w':
    case 'W':
	/*
	 * decrement towards Width 
	 */
	if (texTindex)
	    texTindex--;
	else
	    texSindex++;
	if (texSindex >= maxSratios)
	    texSindex = maxSratios - 1;

	glBindTexture(GL_TEXTURE_2D, texSindex + maxSratios * texTindex);
	fprintf(stderr, "Change Aspect Ratio to %d:%d; bound texture %d\n",
		1 << texSindex, 1 << texTindex, texSindex + maxSratios * texTindex);

	if (three) {		/* dropped out of 3/4 ratio texture; clean up state */
	    three = 0;
	    if (matrix) {	/* turn off matrix too */
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		matrix = 0;
	    }
	}
	glutPostRedisplay();
	break;

    case 'g':
    case 'G':
	if (grid)
	    fprintf(stderr, "Toggle Grid: off\n");
	else
	    fprintf(stderr, "Toggle Grid: on\n");
	grid = !grid;
	glutPostRedisplay();
	break;
    case '3':			/* toggle special 3/4 image */
	three = !three;		/* toggle texture */
	if (three) {		/* turning on */
	    /*
	     * save previous one 
	     */
	    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldtex);
	    glBindTexture(GL_TEXTURE_2D, maxSratios * maxTratios);
	    fprintf(stderr, "Loaded Special 3/4 texture image\n");

	    if (!matrix) {	/* set matrix too */
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glScalef(1.f, .75f, 1.f);
		glMatrixMode(GL_MODELVIEW);
		matrix = 1;
	    }
	} else {		/* turning off */
	    /*
	     * restore old binding 
	     */
	    fprintf(stderr, "Restoring texture %d\n", (int)oldtex);
	    glBindTexture(GL_TEXTURE_2D, oldtex);

	    if (matrix) {	/* turn off matrix too */
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		matrix = 0;
	    }
	}
	glutPostRedisplay();
	break;
    case 'm':
    case 'M':
	matrix = !matrix;	/* toggle */
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	if (matrix) {
	    glScalef(1.f, .75f, 1.f);
	    fprintf(stderr, "Toggle texture transform matrix, on\n");
	} else
	    fprintf(stderr, "Toggle texture transform matrix, off\n");
	glMatrixMode(GL_MODELVIEW);
	glutPostRedisplay();
	break;
    case 'i':
    case 'I':
	viewangle = 0.f;
	viewXscale = 1.f;
	viewYscale = 1.f;
	fprintf(stderr, "Reinitialize Geometry\n");
	glutPostRedisplay();
	break;
    case '\033':
	exit(0);
    default:
	fprintf(stderr, "Keyboard commands:\n\n"
		"t, T - Make Texture Taller\n"
		"w, W - Make Texture Wider\n"
		"r, R - Reset square Aspect Ratio\n"
		"i, I - Reinitialize Geometry Transforms\n"
		"3    - Toggle 3/4 height texture & tex matrix\n"
		"m, M - Toggle texture matrix with 3/4 scale \n"
		"g, G - Toggle grid\n"
	      "<left mouse button>; tilt polygon (up towards vertical)\n"
		"<middle mouse button>; scale polygon in x and y\n"
		"<escape key> - exit\n");

	break;
    }

}

void
menu(int choice)
{
    switch (choice) {
    case RESET:
	key('r', 0, 0);
	break;
    case TALLER:
	key('t', 0, 0);
	break;
    case WIDER:
	key('w', 0, 0);
	break;
    case EXIT:
	exit(0);
    }
}

/*
 * make checkerboard texture texwid X texht with divisions X divisions
 * squares
 */
/*
 * TODO make divisions work 
 */
/*ARGSUSED1*/
unsigned *
make_texture(int texwid, int texht, int divisions)
{
    unsigned *tex, *ptr;
    int i, j;
    tex = (unsigned *) malloc(texwid * texht * sizeof(GLuint));
    for (ptr = tex, j = 0; j < texht; j++)
	for (i = 0; i < texwid; i++)
	    if (((i >> 7) & 0x1) ^ ((j >> 7) & 0x1))
		*ptr++ = 0xffffffff;	/* R, G, B, A all = 255 */
	    else
		*ptr++ = 0x000000ff;	/* R, G, B = 0, A = 255 */
    return tex;
}


/*
 * create and bind mipmaps of various ratios; start with wid X ht image 
 */
/*
 * TextureId of mipmap = s * maxSratios * t 
 */

void 
make_mipmaps(int wid, int ht)
{
    unsigned *floortex;
    unsigned *tex, *tptr;
    GLsizei texwid, texht;
    int i, j;

    /*
     * load pattern for current 2d texture 
     */
    floortex = make_texture(wid, ht, divisions);

    /*
     * build default 1:1 texture, texture id 0 
     */
    glTexParameteri(GL_TEXTURE_2D,
		    GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, wid, ht, GL_RGBA,
		      GL_UNSIGNED_BYTE, floortex);

    /*
     * scratchpad for making images 
     */
    tex = (unsigned *) malloc(sizeof(GLuint) * wid * ht);

    /*
     * increasing i; wider. increasing j, taller 
     */
    for (j = 0; j < maxSratios; j++)
	for (i = 0; i < maxTratios; i++) {
	    /*
	     * already did the first one && only do aniso ratios
	     */
	    if ((i == 0 && j == 0) || (j && i))
		continue;

	    fprintf(stderr,
		    "Making Ansiotropic Texture %d\n",
		    i + maxSratios * j);

	    glBindTexture(GL_TEXTURE_2D, i + maxSratios * j);
	    texwid = wid / (1 << j);
	    texht = ht / (1 << i);

	    glTexParameteri(GL_TEXTURE_2D,
			 GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	    gluScaleImage(GL_RGBA, wid, ht, GL_UNSIGNED_BYTE, floortex,
			  texwid, texht, GL_UNSIGNED_BYTE, tex);

	    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, texwid, texht, GL_RGBA,
			      GL_UNSIGNED_BYTE, tex);

	}
    /*
     * make a special 3:4 ratio mipmap 
     */

    /*
     * choose a number that is never used 
     */
    glBindTexture(GL_TEXTURE_2D, maxSratios * maxTratios);

    /*
     * clear tex image to black 
     */
    for (tptr = tex, i = 0; i < wid * ht; i++)
	*tptr++ = 0x0;

    glTexParameteri(GL_TEXTURE_2D,
		    GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    /*
     * scale the image height by 3/4 
     */
    gluScaleImage(GL_RGBA, wid, ht, GL_UNSIGNED_BYTE, floortex,
		  wid, ht * 3 / 4, GL_UNSIGNED_BYTE, tex);

    /*
     * load in full image, but only bottom 3/4ths have checkerboard 
     */
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, wid, ht, GL_RGBA,
		      GL_UNSIGNED_BYTE, tex);


    glBindTexture(GL_TEXTURE_2D, 0);	/* reset to default texture */

    CHECK_ERROR("end of make_mipmaps");

}

int
main(int argc, char *argv[])
{
    glutInitWindowSize(winwid, winht);
    glutInit(&argc, argv);
    if (argc > 1) {
	char *args = argv[1];
	int done = GL_FALSE;
	while (!done) {
	    switch (*args) {
	    case 's':		/* single buffer */
		printf("Single Buffered\n");
		dblbuf = GL_FALSE;
		break;
	    case '-':		/* do nothing */
		break;
	    case 0:
		done = GL_TRUE;
		break;
	    }
	    args++;
	}
    }
    if (dblbuf)
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    else
	glutInitDisplayMode(GLUT_RGBA);

    (void) glutCreateWindow("anisotropic texturing");

    glutDisplayFunc(redraw);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);

    (void) glutCreateMenu(menu);
    glutAddMenuEntry("Reset Aspect Ratio to 1:1 ('r', 'R')", RESET);
    glutAddMenuEntry("Taller Aspect Ratio ('t', 'T')", TALLER);
    glutAddMenuEntry("Wider Aspect Ratio ('w', 'W')", WIDER);
    glutAddMenuEntry("Exit (escape key)", EXIT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    /*
     * draw a perspective scene 
     */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-100., 100., -100., 100., 100., 300.);
    glMatrixMode(GL_MODELVIEW);
    /*
     * ** viewer exactly near plane distance away. Tiles should form floor,
     * ** starting from bottom edge of screen
     */
    gluLookAt(0., 0., 200.,
	      0., 0., 100.,
	      0., 1., -.1);


    /*
     * turn on features 
     */
    glEnable(GL_TEXTURE_2D);
    glDepthFunc(GL_LEQUAL);
    glClearColor(.3f, .3f, .3f, 1.f);	/* to see the edge of the texture better */

    /*
     * create a set of mipmaps 
     */
    make_mipmaps(winwid * 2, winht * 2);

    CHECK_ERROR("end of main");

    glutMainLoop();

    return 0;
}
