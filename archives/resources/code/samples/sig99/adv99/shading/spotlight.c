#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#ifdef _WIN32
#ifndef M_PI
#define M_PI 3.14159265
#endif
#endif

#define rad2deg(a) (180.0 * (a) / M_PI)

int px, py, moving = -1;
GLuint drawMode = GL_FILL, drawCone = 1, object = 0;
float theta = (float)M_PI / 3.f, phi = (float)M_PI / 3.f, angle = (float)M_PI / 12.f;

/* Draw a cone */
#define numSides 30
void 
drawLightCone(void)
{
    int i;
    float s = 2 * sin(angle);
    float c = 2 * cos(angle);

    /* The light */
    glEnable(GL_POINT_SMOOTH);
    glPointSize(6.0);
    glColor4f(1.0, 1.0, 0.0, 1.0);
    glBegin(GL_POINTS);
    glVertex3f(0.0, 0.0, 0.0);
    glEnd();
    glDisable(GL_POINT_SMOOTH);

    /* The cone */
    if (drawCone) {
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(1.0, 1.0, 0.5, 0.75);
	glVertex3f(0.0, 0.0, 0.0);

	glColor4f(1.0, 1.0, 1.0, 0.25);
	for (i = 0; i <= numSides; i++)
	    glVertex3f(s * cos(2 * i * M_PI / numSides),
		       s * sin(2 * i * M_PI / numSides), -c);
	glEnd();
	glCullFace(GL_BACK);
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(1.0, 1.0, 0.5, 0.75);
	glVertex3f(0.0, 0.0, 0.0);

	glColor4f(1.0, 1.0, 1.0, 0.25);
	for (i = 0; i <= numSides; i++)
	    glVertex3f(s * cos(2 * i * M_PI / numSides),
		       s * sin(2 * i * M_PI / numSides), -c);
	glEnd();
	glDisable(GL_CULL_FACE);
    }
}

/* Display callback */
void 
cbDisplay(void)
{
    static GLfloat position[4] = {1.0, 0.0, 1.0, 1.0};
    static GLfloat direction[4] = {-1.0, 0.0, -1.0, 1.0};

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, drawMode);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, direction);

    glPushMatrix();
    switch (object) {
    case 0:
	glutSolidTorus(0.25, 0.5, 256, 256);
	break;
    case 1:
	glutSolidSphere(0.5, 256, 256);
	break;
    case 2:
	glRotatef(90, 1.0, 0.0, 0.0);
	glutSolidTeapot(0.5);
	break;
    }
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.0, 0.0, 1.0);
    glRotatef(45.0, 0.0, 1.0, 0.0);
    glDisable(GL_LIGHTING);
    drawLightCone();
    glEnable(GL_LIGHTING);
    glPopMatrix();

    glutSwapBuffers();
}

/* Mouse button callback */
void 
cbMouse(int button, int state, int x, int y)
{
    if (moving == -1 && state == GLUT_DOWN) {
	moving = button;
	px = x;
	py = y;
    } else if (button == moving && state == GLUT_UP) {
	moving = -1;
    }
}

/* Mouse motion callback */
void 
cbMotion(int x, int y)
{
    switch (moving) {
    case GLUT_LEFT_BUTTON:
	theta -= 0.01 * (x - px);
	phi -= 0.01 * (y - py);
	break;
    case GLUT_MIDDLE_BUTTON:
	angle -= 0.003 * (y - py);
	glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, rad2deg(angle));
	break;
    case GLUT_RIGHT_BUTTON:
    default:
	return;
    }
    px = x;
    py = y;
    glLoadIdentity();
    gluLookAt(3.0 * sin(phi) * cos(theta), 3.0 * sin(phi) * sin(theta), 3.0 * cos(phi),
	      0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
    glutPostRedisplay();
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

    case '0':
    case '1':
    case '2':
	object = (key - '0');
	break;

    case 'c':
    case 'C':
	drawCone = !drawCone;
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

    /* Spotlight */
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, rad2deg(angle));
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, surface);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0, 1.0, 1, 10.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(3.0 * sin(phi) * cos(theta), 3.0 * sin(phi) * sin(theta), 3.0 * cos(phi),
	      0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.667f, 1.0f);
}

int 
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Spotlight illumination");

    glutDisplayFunc(cbDisplay);
    glutKeyboardFunc(cbKeyboard);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);

    glutCreateMenu(cbMenu);
    glutAddMenuEntry("Show torus ('0')", '0');
    glutAddMenuEntry("Show sphere ('1')", '1');
    glutAddMenuEntry("Show teapot ('2')", '2');
    glutAddMenuEntry("Toggle light cone ('c')", 'c');
    glutAddMenuEntry("Toggle wireframe mode ('w')", 'w');
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    init();
    glutMainLoop();
    return 0;
}
