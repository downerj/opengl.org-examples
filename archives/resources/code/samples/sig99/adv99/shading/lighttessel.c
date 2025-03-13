#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

int px, moving = 0;
GLuint drawMode = GL_FILL;
int tesselation = 1;
float theta = 0;

/* Display callback */
void 
cbDisplay(void)
{
    int i, j;

    static GLfloat position[4] = {0.0, 0.0, 0.5, 1.0};

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, drawMode);

    glLoadIdentity();
    glTranslatef(0.0, 0.0, -8.0);
    glRotatef(theta, 0.0, 1.0, 0.0);
    glLightfv(GL_LIGHT0, GL_POSITION, position);

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

    case '+':
    case 'i':
    case 'I':
	tesselation *= 2;
	break;
    case '-':
    case 'd':
    case 'D':
	if (tesselation > 1)
	    tesselation /= 2;
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
    GLfloat rgb[4] = {1.0, 0.5, 0.0, 1.0};

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0, 1.0, 1, 100.0);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, rgb);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.667f, 0.f);
}

int 
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(400, 400);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Tesselation effects on lighting");

    glutDisplayFunc(cbDisplay);
    glutKeyboardFunc(cbKeyboard);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);

    glutCreateMenu(cbMenu);
    glutAddMenuEntry("Increase tesselation ('+')", 'i');
    glutAddMenuEntry("Decrease tesselation ('-')", 'd');
    glutAddMenuEntry("Toggle wireframe mode ('w')", 'w');
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    init();
    glutMainLoop();
    return 0;
}
