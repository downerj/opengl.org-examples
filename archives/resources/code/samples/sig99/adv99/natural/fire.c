/*
 * This program demonstrates animation using multiple texture objects
 * It draws a billboard polygon, animated with a flame texture. The
 * flame is a series of flame textures, bound in sequence, a new
 * on each frame. To increase the realism, a red/yellow lightsource
 * is placed at the center of the flame polygon, and flickers by
 * changing intensity each frame.
 *
 * Your job is to modify the display function to texture the flame
 * polygon with the proper flame texture, and to adjust the flame
 * light and position so the light appears to flicker and the light
 * follows the flame if it moves.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "texture.h"
#include <GL/glut.h>

#ifdef _WIN32
/* Win32 math.h doesn't define float versions of the trig functions. */
#define sinf sin
#define cosf cos
#define atan2f atan2

/* nor does it define M_PI. */
#ifndef M_PI
#define M_PI 3.14159265
#endif

#define drand48() ((float)rand()/RAND_MAX)
#endif

#if !defined(GL_VERSION_1_1) && !defined(GL_VERSION_1_2)
#define glBindTexture glBindTextureEXT
#endif

static int billboard = 1;

int flames = 0; /* current flame image */
int flameCount = 32; /* total number of flame images */

static float transx = 1.0, transy, rotx, roty;
static int ox = -1, oy = -1;
static int mot = 0;

enum {NO_DLIST, FLAME_BASE, GROUND};


#define PAN	1
#define ROT	2

#define RAD(x) (((x)*M_PI)/180.)

void
pan(const int x, const int y) {
    transx +=  (x-ox)/500.;
    transy -= (y-oy)/500.;
    ox = x; oy = y;
    glutPostRedisplay();
}

void
rotate(const int x, const int y) {
    rotx += x-ox;
    if (rotx > 360.) rotx -= 360.;
    else if (rotx < -360.) rotx += 360.;
    roty += y-oy;
    if (roty > 360.) roty -= 360.;
    else if (roty < -360.) roty += 360.;
    ox = x; oy = y;
    glutPostRedisplay();
}

void
motion(int x, int y) {
    if (mot == PAN) pan(x, y);
    else if (mot == ROT) rotate(x,y);
}

void
mouse(int button, int state, int x, int y) {

    /* hack for 2 button mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;

    if(state == GLUT_DOWN) {
	switch(button) {
	case GLUT_LEFT_BUTTON:
	    mot = PAN;
	    motion(ox = x, oy = y);
	    break;
	case GLUT_RIGHT_BUTTON:
	    mot = ROT;
	    motion(ox = x, oy = y);
	    break;
	case GLUT_MIDDLE_BUTTON:
	    break;
	}
    } else if (state == GLUT_UP) {
	mot = 0;
    }
}

void help(void) {
    printf("'h'           - help\n");
    printf("left mouse    - pan\n");
    printf("right mouse   - rotate\n");
}

/* light to simulate flames effects */
GLfloat lightpos[] = {0.f, 0.f, 0.f, 1.f};
GLfloat firecolor[] = {1.f, 0.6f, 0.3f, 1.f};

enum {R, G, B, A};

GLfloat firelight[32 * 4]; /* a brightness for every frame */

void init(void)
{
    static unsigned *image;
    static int width, height, components;
    char fname[sizeof("../Data/Natural/flame/f00") + 1];
    GLUquadric *cylinder;
    int i, j, duration, d;
    GLfloat oscale, nscale, scale, start;

    /* save the flame sequence into a set of texture objects */
    /* linear ramp between brightnesses */
    d = duration = 1 + drand48() * 4;
    start = oscale  = drand48() * 3/4.f + .25f; /* 1/4 to 1 */
    nscale = drand48() * 3/4.f + .25f; /* 1/4 to 1 */
    for(i = 0; i < flameCount; i++)
    {

	(void)sprintf(fname, "../../data/flame/f%.2d",i);
	image = read_texture(fname, &width, &height, &components);

	glBindTexture(GL_TEXTURE_2D, i);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, components, width,
                 height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 image);

	free(image);
	
	
	if(!d)
	{
	    d = duration = 1 + drand48() * 4;
	    if(d >= flameCount - i) /* make wraparound cleaner */
	    {
		d = duration = flameCount - i;
		nscale = start;
	    }
	    oscale = nscale;
	    nscale = drand48() * 3/4.f + .25f; /* 1/4 to 1 */
	}
	scale = d/(GLfloat)duration * oscale
	        + (1 - d/(GLfloat)duration) * nscale;
	d -= 1;

	firelight[i * 4 + R] = firecolor[R] * scale;
	firelight[i * 4 + G] = firecolor[G] * scale;
	firelight[i * 4 + B] = firecolor[B] * scale;
	firelight[i * 4 + A] = 1.f;
    }
    glAlphaFunc(GL_GREATER, .9f);

    glEnable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);

    /* turn on lighting, enable light 0, and turn on colormaterial */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 1.f);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, firelight);

    /* shift the flame texture down a little on the flame polygon */
    glMatrixMode(GL_TEXTURE);
    glTranslatef(0.f, .15f, 0.f);

    glMatrixMode(GL_PROJECTION);
    gluPerspective(50., 1., .1, 10.);

    glMatrixMode(GL_MODELVIEW);
    glTranslatef(0., 0., -5.5);

    /* geometry for the base of the flame */
    glNewList(FLAME_BASE, GL_COMPILE);
    glColor3f(.2f, .2f, .2f);
    glPushMatrix();
    glTranslatef(0.f, -.9f, 0.f);
    glRotatef(90.f, 1.f, 0.f, 0.f);
    cylinder = gluNewQuadric();
    gluQuadricOrientation(cylinder, GLU_INSIDE);
    gluCylinder(cylinder, .5, .5, .1, 20, 20);
    gluDisk(cylinder, 0.f, .5, 20, 20);
    gluDeleteQuadric(cylinder);
    glPopMatrix();
    glEndList();


    /* geometry for the ground */
    glNewList(GROUND, GL_COMPILE);
    glColor4f(0.6f, 0.8f, 0.5f, 1.f);
    glBegin(GL_QUADS);
    glNormal3f(0.f, 1.f, 0.f);
    for(j = 0; j < 20; j++) /* z direction */
	for(i = 0; i < 20; i++) /* x direction */
	{
	    glVertex3f(-2.0 + i * 4.f/20, -1.0, -2.0 + j * 4.f/20);
	    glVertex3f(-2.0 + i * 4.f/20, -1.0, -2.0 + (j + 1) * 4.f/20);
	    glVertex3f(-2.0 + (i + 1) * 4.f/20, -1.0, -2.0 + (j + 1) * 4.f/20);
	    glVertex3f(-2.0 + (i + 1) * 4.f/20, -1.0, -2.0 + j * 4.f/20);
	}
    glEnd();
    glEndList();
}


void
buildRot(float theta, float x, float y, float z, float m[16])
{

    float ct = cosf(RAD(theta)), st = sinf(RAD(theta));

    /* clear matrix to indentity */

    m[ 0] = 1; m[ 4] = 0; m[ 8] = 0; m[12] = 0;
    m[ 1] = 0; m[ 5] = 1; m[ 9] = 0; m[13] = 0;
    m[ 2] = 0; m[ 6] = 0; m[10] = 1; m[14] = 0;
    m[ 3] = 0; m[ 7] = 0; m[11] = 0; m[15] = 1;

    /*
     * Now build R, the rotation matrix (which contains S)
     *
     * R = uu' + cos(theta) * (I - uu') + sin(theta) * S
     *
     * u' = (x, y, z)
     *
     * S =  0  -z   y    
     *	    z   0  -x
     *	   -y   x   0
     */

     m[ 0] = x * x + ct * (1 - x * x) + st *  0;
     m[ 4] = x * y + ct * (0 - x * y) + st * -z;
     m[ 8] = x * z + ct * (0 - x * z) + st *  y;

     m[ 1] = y * x + ct * (0 - y * x) + st *  z;
     m[ 5] = y * y + ct * (1 - y * y) + st *  0;
     m[ 9] = y * z + ct * (0 - y * z) + st * -x;

     m[ 2] = z * x + ct * (0 - z * x) + st * -y;
     m[ 6] = z * y + ct * (0 - z * y) + st *  x;
     m[10] = z * z + ct * (1 - z * z) + st *  0;
}

static void
calcMatrix(void) {
    float mat[16];

    glGetFloatv(GL_MODELVIEW_MATRIX, mat);
    buildRot(-180*atan2f(mat[8], mat[10])/M_PI, 0, 1, 0, mat);
    glMultMatrixf(mat);
}



void
display(void) 
{

    /*
     * save the viewing transform by saving a copy of the
     * modelview matrix at the right place, then undo the
     * rotation by calling calcMatrix() at the right time to
     * billboard the tree. Billboarding should only happen
     * if the billboard variable is not zero.
     */

    float mat[16];
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    gluLookAt(-sinf(RAD(rotx))*5.5,
	      transy,
	      cosf(RAD(rotx))*5.5,
	      0. ,0. ,0., 
	      0. ,1. ,0.);

    /* save the viewing tranform */
    glGetFloatv(GL_MODELVIEW_MATRIX, mat);

    /* change the brightness of the flame light */
    glLightfv(GL_LIGHT0, GL_DIFFUSE, &firelight[flames * 4]);

    /* ground */
    glDisable(GL_TEXTURE_2D);
    glCallList(GROUND);

    glPushMatrix();

    glTranslatef(0.f, 0.f, -transx);

    /* base of flame */
    glCallList(FLAME_BASE);

    /* undo the rotation in the viewing transform */
    if (billboard) 
	calcMatrix();

    /* flame polygon */
    
    /* 
     * position the flame light in the center of the flame polygon
     * turn off lighting, turn on texturing, bind the proper flame
     * texture, and draw the flame polygon.
     */

    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, flames);
    glColor4f(1.f, 1.f, 1.f, 1.f);
    glBegin(GL_POLYGON);
	glTexCoord2f(0.0, 0.0);
	glVertex2f(-1.0, -1.0);
	glTexCoord2f(1.0, 0.0);
	glVertex2f(1.0, -1.0);
	glTexCoord2f(1.0, 1.0);
	glVertex2f(1.0, 1.0);
	glTexCoord2f(0.0, 1.0);
	glVertex2f(-1.0, 1.0);
    glEnd();
    glPopMatrix();

    /* turn lighting back on */
    glEnable(GL_LIGHTING);

    glutSwapBuffers();
}

/*ARGSUSED*/
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case '\033':
	exit(EXIT_SUCCESS); 
	break;
    default:
	help();
	break;
    }
    glutPostRedisplay();
}

void
anim(void)
{
    flames += 1;
    if(flames >= flameCount)
	flames = 0;

    glutPostRedisplay();
}

int
main(int argc, char** argv)
{
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
    (void)glutCreateWindow("fire");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutIdleFunc(anim);
    glutMotionFunc(motion);

    key('?', 0, 0); /* to print help message */

    glutMainLoop();
    return 0;
}


