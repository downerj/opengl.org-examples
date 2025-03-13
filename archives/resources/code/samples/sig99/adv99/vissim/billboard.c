/*
 * This program demonstrates billboarding geometry, which makes the
 * orientation of the polygon view independent.
 *
 * The code for undoing the rotation component of the viewing tranform
 * is included in the function calcMatrix(). Your job is to find the
 * proper place to save the viewing transform and undo the rotation
 * in the display() function.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "../util/texture.h"
#include <GL/glut.h>

#ifdef WIN32
/* Win32 math.h doesn't define float versions of the trig functions. */
#define sinf sin
#define cosf cos
#define atan2f atan2

/* nor does it define M_PI. */
#define M_PI 3.14159265
#endif


static int billboard = 1, visible = 1, polygon = 1;
static float transx = 1.0, transy, rotx, roty;
static int ox = -1, oy = -1;
static int mot = 0;
static int fin;

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
    printf("'b'           - toggle billboard mode\n");
    printf("'f'           - toggle using a fin tree\n");
    printf("'p'           - toggle making polygon visible\n");
    printf("'v'           - toggle blending (make pgon and texture visible)\n");
    printf("left mouse    - pan\n");
    printf("right mouse   - rotate\n");
}

void init(void)
{
    static unsigned *image;
    static int width, height, components;
    image = read_texture("../../data/tree1.rgba",
			 &width, &height, &components);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, components, width,
                 height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 image);

    free(image);

    glAlphaFunc(GL_GREATER, .9f);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.f, 1.f, .1f, 10.f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0., 0., -5.5);

    glClearColor(.25f, .25f, .25f, .25f);
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



void display(void) {

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

    /* floor */
    glDisable(GL_TEXTURE_2D);
    glColor4f(0.2f, 0.8f, 0.2f, 1.f);
    glBegin(GL_POLYGON);
	glVertex3f(-2.0f, -1.0f, -2.0f);
	glVertex3f( 2.0f, -1.0f, -2.0f);
	glVertex3f( 2.0f, -1.0f,  2.0f);
	glVertex3f(-2.0f, -1.0f,  2.0f);
    glEnd();

    glPushMatrix();

    glTranslatef(0.f, 0.f, -transx);

    /* undo the rotation in the viewing transform */
    if (billboard && !fin) 
	calcMatrix();

    /* tree */
    if(!polygon)
	glEnable(GL_TEXTURE_2D);
    if(visible)
	glEnable(GL_ALPHA_TEST);
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
    if (fin) {
	glRotatef(90.f, 0.f, 1.f, 0.f);
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
    }
    glPopMatrix();
    if(visible)
	glDisable(GL_ALPHA_TEST);
    glutSwapBuffers();
}

/*ARGSUSED*/
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case 'B': 
    case 'b': 
	billboard ^= 1;
	break;
    case 'P': /* show polygon only */
    case 'p': 
	polygon ^= 1;
	break;
    case 'V': /* make texture and polygon visible (turn off blending) */
    case 'v': 
	visible ^= 1;
	break;
    case 'F':
    case 'f':
	fin ^= 1;
	break;
    case '\033': exit(EXIT_SUCCESS); break;
    default:
	help();
	break;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
    (void)glutCreateWindow("billboard");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    key('?', 0, 0); /* to print help message */

    glutMainLoop();
    return 0;
}
