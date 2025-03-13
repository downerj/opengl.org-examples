#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#ifdef WIN32
#ifndef M_PI
#define M_PI	3.14159265
#endif
#endif

int px, moving = 0;
GLuint drawMode = GL_FILL, drawCube = 0;
float theta = 0;

/* Display callback */
void 
cbDisplay(void)
{
    int i;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, drawMode);

    glLoadIdentity();
    glTranslatef(0.0, 0.0, -15.0);

    glTranslatef(-20.0, 0.0, 0.0);
    glPushMatrix();
    glRotatef(theta, 0.0, 1.0, 0.0);

    glBegin(GL_TRIANGLE_STRIP);
    for (i = 0; i <= 100; i++) {
	glColor3f((100 - i) / 100.0, i / 100.0, i / 100.0);
	glVertex2f(i / 2.5, 1.25);
	glVertex2f(i / 2.5, 0.25);
    }
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
    glColor3f(1.0, 0.0, 0.0);
    glVertex2f(0.0, -0.25);
    glVertex2f(0.0, -1.25);
    glColor3f(0.0, 1.0, 1.0);
    glVertex2f(40.0, -0.25);
    glVertex2f(40.0, -1.25);
    glEnd();
    glPopMatrix();

    if (drawCube) {
	glColor3f(1.0, 1.0, 1.0);
	glScalef(-40.0 * cos(M_PI * theta / 180.0), 2.25, 40.0 * sin(M_PI * theta / 180.0));
	glTranslatef(-0.5, 0.0, -0.5);
	glutWireCube(1);
    }
    glutSwapBuffers();
}

/* Mouse button callback */
/*ARGSUSED3*/
void 
cbMouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
	moving = 1;
	px = x;
    } else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
	moving = 0;
    }
}

/* Mouse motion callback */
/*ARGSUSED1*/
void 
cbMotion(int x, int y)
{
    if (moving) {
	theta -= 0.05 * (x - px);
	px = x;
	glutPostRedisplay();
    }
}

/* Keyboard callback */
/*ARGSUSED1*/
void 
cbKeyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 27:
    case 'q':
	exit(0);

    case 'c':
    case 'C':
	drawCube = !drawCube;
	break;
    case 'w':
    case 'W':
    case 't':
    case 'T':
	drawMode = (drawMode == GL_FILL) ? GL_LINE : GL_FILL;
	break;

    default:
	return;
    }
    glutPostRedisplay();
}

void 
cbMenu(int option)
{
    cbKeyboard((unsigned char) option, 0, 0);
}

void 
init(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0, 4.0, 1, 100.0);
    glMatrixMode(GL_MODELVIEW);

    glClearColor(0.0f, 0.0f, 0.667f, 1.0f);
    glEnable(GL_DEPTH_TEST);
}

int 
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(800, 200);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Top gradient is perspective correct, bottom is not");

    glutDisplayFunc(cbDisplay);
    glutKeyboardFunc(cbKeyboard);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);

    glutCreateMenu(cbMenu);
    glutAddMenuEntry("Toggle tesselation view ('t')", 't');
    glutAddMenuEntry("Toggle cube ('c')", 'c');
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    init();
    glutMainLoop();
    return 0;
}
