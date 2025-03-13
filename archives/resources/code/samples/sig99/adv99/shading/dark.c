#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

int px, moving = 0;
GLuint drawMode = GL_FILL;
int tesselation = 128, point = 1;
float theta = 0;

/* Display callback */
void 
cbDisplay(void)
{
    static GLfloat position0[4] = {0.0, 0.0, 1.0, 0.0};
    static GLfloat position1[4] = {-1.0, 0.0, 1.0, 1.0};
    static GLfloat position2[4] = {1.0, 0.0, 1.0, 1.0};
    static GLfloat direction[4] = {0.0, 0.0, -1.0, 1.0};
    int i, j;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, drawMode);

    glLoadIdentity();
    glTranslatef(0.0, 0.0, -10.0);
    glRotatef(theta, 1.0, 0.0, 0.0);

    /* Set the light positions after the viewing transform */
    glLightfv(GL_LIGHT0, GL_POSITION, position0);
    glLightfv(GL_LIGHT1, GL_POSITION, position1);
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, direction);
    glLightfv(GL_LIGHT2, GL_POSITION, position2);
    glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, direction);

    glNormal3f(0.0, 0.0, 1.0);
    glScalef(5.0, 5.0, 0.0);
    glTranslatef(-0.5, -0.5, 0.0);
    for (i = 0; i < tesselation; i++) {
	glBegin(GL_TRIANGLE_STRIP);
	for (j = 0; j <= tesselation; j++) {
	    glVertex2f(j / (float) tesselation, i / (float) tesselation);
	    glVertex2f(j / (float) tesselation, (i + 1) / (float) tesselation);
	}
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

    case 'l':
    case 'L':
	point = !point;
	glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, point ? 180.0 : 60.0);
	glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, point ? 180.0 : 60.0);
	glutChangeToMenuEntry(1, point ? "Change to spotlights ('l')"
			      : "Change to point lights ('l')", 'l');
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
    GLfloat surface[4] = {0.3f, 1.0f, 0.3f, 1.0f};
    GLfloat ambient[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat zero[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat positive[4] = {0.7f, 0.7f, 0.7f, 1.0f};
    GLfloat negative[4] = {-0.7f, -0.7f, -0.7f, 1.0f};

    /* Ambient */
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, zero);
    glLightfv(GL_LIGHT0, GL_SPECULAR, zero);
    glEnable(GL_LIGHT0);

    /* Light */
    glLightfv(GL_LIGHT1, GL_AMBIENT, zero);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, positive);
    glEnable(GL_LIGHT1);

    /* "Dark" */
    glLightfv(GL_LIGHT2, GL_AMBIENT, zero);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, negative);
    glEnable(GL_LIGHT2);

    glEnable(GL_LIGHTING);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, surface);

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
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Light and Dark");

    glutDisplayFunc(cbDisplay);
    glutKeyboardFunc(cbKeyboard);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);

    glutCreateMenu(cbMenu);
    glutAddMenuEntry("Change to spotlights ('l')", 'l');
    glutAddMenuEntry("Toggle wireframe mode ('w')", 'w');
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    init();
    glutMainLoop();
    return 0;
}
