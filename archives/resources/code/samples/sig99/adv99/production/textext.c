#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include <GL/glut.h>
#include "texture.h"
#include "textmap.h"

static float scale = .01f;
static char *string = "OpenGL rules";
static float transx, transy, rotx, roty;
static int ox = -1, oy = -1;
static int active;
enum {OBJ_ANGLE, OBJ_TRANSLATE, OBJ_PICK};

void display(void);

void motion(int x, int y) {
    switch(active) {
    case OBJ_ANGLE:
	rotx += x-ox;
	if (rotx > 360.) rotx -= 360.;
	else if (rotx < -360.) rotx += 360.;
	roty += y-oy;
	if (roty > 360.) roty -= 360.;
	else if (roty < -360.) roty += 360.;
	ox = x; oy = y;
	break;
    case OBJ_TRANSLATE:
	transx +=  (x-ox)/500.;
	transy -= (y-oy)/500.;
	ox = x; oy = y;
	break;
    }
    glutPostRedisplay();
}

void
mouse(int button, int state, int x, int y) {

    /* hack for 2 button mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;

    if(state == GLUT_DOWN)
	switch(button) {
	case GLUT_LEFT_BUTTON: /* move the light */
	    active = OBJ_ANGLE;
	    ox = x; oy = y;
	    break;
	case GLUT_RIGHT_BUTTON: /* move the polygon */
	    active = OBJ_PICK;
	    break;
	case GLUT_MIDDLE_BUTTON:
	    active = OBJ_TRANSLATE;
	    ox = x; oy = y;
	    break;
	}
}

void up(void) { scale += .0025f; }
void down(void) { scale -= .0025f; }

void help(void) {
    printf("Usage: textext [string]\n");
    printf("'h'            - help\n");
    printf("'UP'           - scale up\n");
    printf("'DOWN'         - scale down\n");
    printf("left mouse     - pan\n");
    printf("middle mouse   - rotate\n");
}

void myinit(void) {
    texfntinit("../../data/Times-Italic.bw");
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90.,1.,.1,10.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.,0.,-1.5);
}

void display(void) {
    float width = texstrwidth(string);
    glClear(GL_COLOR_BUFFER_BIT);
    glPushMatrix();
    glTranslatef(transx, transy, 0.f);
    glRotatef(rotx, 0., 1., 0.);
    glRotatef(roty, 1., 0., 0.);
    glScalef(scale,scale,0.);
    glTranslatef(-width*5, 0.f, 0.f);
    texfntstroke(string, 0.f, 0.f);
    glPopMatrix();
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
}

/*ARGSUSED1*/
void
special(int key, int x, int y) {
    switch(key) {
    case GLUT_KEY_UP:	up(); break;
    case GLUT_KEY_DOWN:	down(); break;
    }
    glutPostRedisplay();
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case '\033': exit(EXIT_SUCCESS); break;
    default: help(); break;
    }
    glutPostRedisplay();
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(512, 512);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    (void)glutCreateWindow("textext");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);

    if (argc > 1) string = argv[1];
    myinit();

    glutMainLoop();
    return 0;
}
