#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>

int px, moving = 0;
int useAmbient = 0, useDiffuse = 0, useSpecular = 0, useEmissive = 0;
GLuint shading = GL_SMOOTH;
float theta = 0;

GLfloat brown[4] = {0.8f, 0.4f, 0.0f, 1.0f};
GLfloat red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
GLfloat violet[4] = {1.0f, 0.0f, 0.5f, 1.0f};
GLfloat gray[4] = {0.7f, 0.7f, 0.7f, 1.0f};
GLfloat white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
GLfloat black[4] = {0.0f, 0.0f, 0.0f, 1.0f};

GLfloat position[4] = {10.0f, 10.0f, 2.f, 1.0f};

void 
setTitle(void)
{
    static char title[128];

    if (!(useAmbient || useDiffuse || useSpecular || useEmissive))
	strcpy(title, "Lighting ( None )");
    else {
	strcpy(title, "Lighting ( ");
	if (useAmbient)
	    strcat(title, "Ambient ");
	if (useDiffuse)
	    strcat(title, "Diffuse ");
	if (useSpecular)
	    strcat(title, "Specular ");
	if (useEmissive)
	    strcat(title, "Emissive ");
	strcat(title, ")");
    }

    glutSetWindowTitle(title);
}

void 
sphereColor(GLfloat * rgb)
{
    glMaterialfv(GL_FRONT, GL_AMBIENT, useAmbient ? rgb : black);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, useDiffuse ? rgb : black);
    glMaterialfv(GL_FRONT, GL_SPECULAR, useSpecular ? white : black);
    glMaterialfv(GL_FRONT, GL_EMISSION, useEmissive ? rgb : black);
    glMaterialf(GL_FRONT, GL_SHININESS, 64.0);
}

/* Display callback */
void 
cbDisplay(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    gluLookAt(7 * cos(theta), 7 * sin(theta), 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    glTranslatef(0.25, 0.25, 0.25);
    sphereColor(brown);
    glutSolidSphere(1.0, 64, 64);

    glTranslatef(0.75, -1.25, -0.75);
    sphereColor(red);
    glutSolidSphere(0.667, 32, 32);

    glTranslatef(-0.25, 2.5, 1.25);
    sphereColor(violet);
    glutSolidSphere(0.5, 32, 32);

    glTranslatef(0.5, 0.0, -1.5);
    sphereColor(gray);
    glutSolidSphere(0.667, 32, 32);

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
	theta -= 0.03 * (x - px);
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

    case 'a':
    case 'A':
	useAmbient = !useAmbient;
	break;
    case 'd':
    case 'D':
	useDiffuse = !useDiffuse;
	break;
    case 'e':
    case 'E':
	useEmissive = !useEmissive;
	break;
    case 's':
    case 'S':
	useSpecular = !useSpecular;
	break;
    case 'm':
	shading = (shading == GL_FLAT) ? GL_SMOOTH : GL_FLAT;
	glShadeModel(shading);
	break;
    default:
	return;
    }

    setTitle();
    glutPostRedisplay();
}

/* Menu callback */
void 
cbMenu(int option)
{
    cbKeyboard((char) option, 0, 0);
}

void 
init(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0, 1.0, 0.1, 10.0);
    glMatrixMode(GL_MODELVIEW);

    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.667f, 0.0f);
}

int 
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("");
    setTitle();

    glutDisplayFunc(cbDisplay);
    glutKeyboardFunc(cbKeyboard);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);

    glutCreateMenu(cbMenu);
    glutAddMenuEntry("Toggle ambient ('a')", 'a');
    glutAddMenuEntry("Toggle diffuse ('d')", 'd');
    glutAddMenuEntry("Toggle specular ('s')", 's');
    glutAddMenuEntry("Toggle emissive ('e')", 'e');
    glutAddMenuEntry("Toggle flat/smooth shading model ('m')", 'm');
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    init();
    glutMainLoop();
    return 0;
}
