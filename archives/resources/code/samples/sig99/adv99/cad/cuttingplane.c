#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#ifdef _WIN32
#ifndef M_PI
#define M_PI 3.14159265f
#endif
#endif

/*
 * Globals 
 */
int px, py, moving = -1, model = 0, drawTransparent = 1;
float theta = M_PI, phi = M_PI / 3.f, distance = 2.0f;
GLdouble clip0[4] = {1.0, 0.0, 0.0, 0.0};
GLdouble clip1[4] = {-1.0, 0.0, 0.0, 0.0};

/*
 * The model 
 */
void 
drawModel(void)
{
    glPushMatrix();
    switch (model) {
    case 0:
	glutSolidTorus(0.1, 0.2, 32, 32);
	break;

    case 1:
	glutSolidSphere(0.2, 32, 32);
	break;

    case 2:
	glRotatef(45.0, 0.0, 1.0, 0.0);
	glRotatef(45.0, 1.0, 0.0, 0.0);
	glutSolidCube(0.4);
	break;
    }
    glPopMatrix();
}

/*
 * Draw the gradent background to make the transparency stand out 
 */
void 
drawGradientBackground(void)
{
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0f, 1.0f, 0.0f, 1.0f);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.8f, 0.4f);
    glVertex2f(0.0f, 0.0f);
    glColor3f(0.4f, 0.6f, 0.6f);
    glVertex2f(1.0f, 0.0f);
    glColor3f(0.4f, 0.4f, 0.8f);
    glVertex2f(1.0f, 1.0f);
    glColor3f(0.4f, 0.6f, 0.6f);
    glVertex2f(0.0f, 1.0f);
    glEnd();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

}

/*
 * Display callback 
 */
void 
cbDisplay(void)
{
    glClear(GL_DEPTH_BUFFER_BIT);

    /*
     * Draw the background 
     */
    drawGradientBackground();

    /*
     * Draw the solid part of the object 
     */
    glEnable(GL_CLIP_PLANE0);
    drawModel();
    glDisable(GL_CLIP_PLANE0);

    if (drawTransparent) {
	/*
	 * Draw the transparent part of the object 
	 */
	glEnable(GL_BLEND);
	glEnable(GL_CLIP_PLANE1);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	drawModel();
	glCullFace(GL_BACK);
	drawModel();
	glDisable(GL_CULL_FACE);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_BLEND);
    }
    /*
     * Draw the outline of the plane 
     */
    glDisable(GL_LIGHTING);
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-clip0[3], -0.5, -0.5);
    glVertex3f(-clip0[3], 0.5, -0.5);
    glVertex3f(-clip0[3], 0.5, 0.5);
    glVertex3f(-clip0[3], -0.5, 0.5);
    glEnd();
    glEnable(GL_LIGHTING);

    glutSwapBuffers();
}

/*
 * Mouse button callback 
 */
void 
cbMouse(int button, int state, int x, int y)
{
    /*
     * hack for 2 button mouse 
     */
    if ((button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT) ||
	(moving == GLUT_MIDDLE_BUTTON && button == GLUT_LEFT_BUTTON && state == GLUT_UP))
	button = GLUT_MIDDLE_BUTTON;

    if (moving == -1 && state == GLUT_DOWN) {
	moving = button;
	px = x;
	py = y;
    } else if (button == moving && state == GLUT_UP) {
	moving = -1;
    }
}

/*
 * Mouse motion callback 
 */
void 
cbMotion(int x, int y)
{
    switch (moving) {
    case GLUT_LEFT_BUTTON:
	theta -= 0.01 * (x - px);
	phi -= 0.01 * (y - py);
	break;
    case GLUT_MIDDLE_BUTTON:
	clip0[3] += 0.01 * (x - px);
	clip1[3] -= 0.01 * (x - px);
	break;
    }
    px = x;
    py = y;

    glLoadIdentity();
    gluLookAt(distance * sin(phi) * cos(theta), distance * sin(phi) * sin(theta), distance * cos(phi),
	      0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    glClipPlane(GL_CLIP_PLANE0, clip0);
    glClipPlane(GL_CLIP_PLANE1, clip1);

    glutPostRedisplay();
}

/*
 * Keyboard callback 
 */
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
	model = 2;
	break;
    case 's':
    case 'S':
	model = 1;
	break;
    case 't':
    case 'T':
	model = 0;
	break;

    case ' ':
	drawTransparent = !drawTransparent;
	glutChangeToMenuEntry(4, drawTransparent ? "Don't draw clipped region (<space>)" : "Draw clipped region transparent (<space>)", ' ');
	break;

    default:
	return;
    }

    glutPostRedisplay();
}

/*
 * Menu callback 
 */
void 
cbMenu(int option)
{
    cbKeyboard((unsigned char) option, 0, 0);
}

void 
init(void)
{
    GLfloat position[4] = {2.0f, 2.0f, 2.0f, 1.0f};
    GLfloat red[4] = {1.0f, 0.0f, 0.0f, 0.3f};
    GLfloat white[4] = {1.0f, 1.0f, 1.0f, 0.3f};

    /*
     * Matrices 
     */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0, 1.0, 0.5, 5.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(distance * sin(phi) * cos(theta), distance * sin(phi) * sin(theta), distance * cos(phi),
	      0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
    glClipPlane(GL_CLIP_PLANE0, clip0);
    glClipPlane(GL_CLIP_PLANE1, clip1);

    /*
     * Lighting 
     */
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
    glMaterialfv(GL_FRONT, GL_SPECULAR, white);
    glMaterialf(GL_FRONT, GL_SHININESS, 25.0);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    /*
     * Misc 
     */
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.4f, 1.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(100, 100);
    glutCreateWindow(argv[0]);

    glutDisplayFunc(cbDisplay);
    glutKeyboardFunc(cbKeyboard);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);

    glutCreateMenu(cbMenu);
    glutAddMenuEntry("Show torus ('t')", 't');
    glutAddMenuEntry("Show sphere ('s')", 's');
    glutAddMenuEntry("Show cube ('c')", 'c');
    glutAddMenuEntry("Don't draw clipped region (<space>)", ' ');
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    init();
    glutMainLoop();
    return 0;
}
