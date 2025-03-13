#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

int px, moving = 0;
GLuint drawMode = GL_FILL, direction = 0;
float theta = 0;

/* Display callback */
void 
cbDisplay(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, drawMode);

    glLoadIdentity();
    glTranslatef(0.0, 0.0, -5.0);
    glRotatef(theta, 0.0, 1.0, 0.0);

    if (direction) {
	glBegin(GL_TRIANGLE_STRIP);
	glColor3f(1.0, 0.0, 0.0);
	glVertex2f(1.0, -1.0);
	glColor3f(0.0, 1.0, 0.0);
	glVertex2f(-1.0, -1.0);
	glColor3f(1.0, 1.0, 0.0);
	glVertex2f(1.0, 1.0);
	glColor3f(0.0, 0.0, 1.0);
	glVertex2f(-1.0, 1.0);
	glEnd();
    } else {
	glBegin(GL_TRIANGLE_STRIP);
	glColor3f(1.0, 1.0, 0.0);
	glVertex2f(1.0, 1.0);
	glColor3f(1.0, 0.0, 0.0);
	glVertex2f(1.0, -1.0);
	glColor3f(0.0, 0.0, 1.0);
	glVertex2f(-1.0, 1.0);
	glColor3f(0.0, 1.0, 0.0);
	glVertex2f(-1.0, -1.0);
	glEnd();
    }


    glutSwapBuffers();
}

/* Mouse button callback */
/*ARGSUSED2*/
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
	theta -= 0.1 * (x - px);
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

    case 't':
    case 'T':
	direction = !direction;
	break;
    case 'w':
    case 'W':
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
    gluPerspective(40.0, 1.0, 1, 100.0);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.667f, 1.0f);
}

int 
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(400, 400);
    glutInitWindowPosition(100, 100);
    glutCreateWindow(argv[0]);

    glutDisplayFunc(cbDisplay);
    glutKeyboardFunc(cbKeyboard);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);

    glutCreateMenu(cbMenu);
    glutAddMenuEntry("Change triangle split direction ('t')", 't');
    glutAddMenuEntry("Toggle wireframe view ('w')", 'w');
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    init();
    glutMainLoop();
    return 0;
}
