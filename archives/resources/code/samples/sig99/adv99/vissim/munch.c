#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#ifdef _WIN32
#ifndef M_PI
#define M_PI 3.14159265f
#endif
#endif

#define texWidth 256
#define texHeight 256
#define tSlices  64
#define zSlices  1

int px, py, moving = -1;
GLuint drawMode = GL_FILL;
int tesselation = 1;
float theta = M_PI, phi = M_PI / 3.f, distance = 5.0f;
GLubyte texture[texWidth][texHeight];
GLuint texName;

void 
drawCylinder(void)
{
    int t, z;

    glPushMatrix();
    glTranslatef(0.0, 0.0, -1.0);
    glScalef(1.0, 1.0, 2.0 / zSlices);

    glEnable(GL_TEXTURE_2D);
    for (t = 0; t < tSlices; t++) {
	glBegin(GL_TRIANGLE_STRIP);
	for (z = 0; z <= zSlices; z++) {
	    glTexCoord2i(t, z);
	    glVertex3f(cos(t * 2 * M_PI / tSlices), sin(t * 2 * M_PI / tSlices), z);
	    glTexCoord2i(t + 1, z);
	    glVertex3f(cos((t + 1) * 2 * M_PI / tSlices), sin((t + 1) * 2 * M_PI / tSlices), z);
	}
	glEnd();
    }
    glDisable(GL_TEXTURE_2D);

    glColor3f(0.5, 0.0, 0.0);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0, 0.0);
    for (t = 0; t <= tSlices; t++) {
	glVertex2f(cos(t * 2 * M_PI / tSlices), sin(t * 2 * M_PI / tSlices));
    }
    glEnd();
    glTranslatef(0.0, 0.0, zSlices);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0, 0.0);
    for (t = 0; t <= tSlices; t++) {
	glVertex2f(cos(t * 2 * M_PI / tSlices), sin(t * 2 * M_PI / tSlices));
    }
    glEnd();
    glPopMatrix();
}

/*
 * Display callback 
 */
void 
cbDisplay(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, drawMode);

    drawCylinder();

    glutSwapBuffers();
}

/*
 * Mouse button callback 
 */
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

/*
 * Mouse motion callback 
 */
void 
cbMotion(int x, int y)
{
    switch (moving) {
    case GLUT_LEFT_BUTTON:
	theta -= 0.01 * (x - px);
	phi -= 0.001 * (y - py);
	break;
    case GLUT_MIDDLE_BUTTON:
	distance *= pow(0.98, y - py);
	break;
    case GLUT_RIGHT_BUTTON:
    default:
	return;
    }
    px = x;
    py = y;
    glLoadIdentity();
    gluLookAt(distance * sin(phi) * cos(theta), distance * sin(phi) * sin(theta), distance * cos(phi),
	      0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
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

    case 'w':
    case 'W':
	drawMode = (drawMode == GL_FILL) ? GL_LINE : GL_FILL;
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

/*
 * Idle callback 
 */
void 
cbIdle(void)
{
    static int sw = 1;
    static int fullcycle = 1;
    static int t = 0;
    int x;

    for (x = 0; x <= 255; x++)
	texture[x][x ^ t] ^= 255;
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_LUMINANCE, GL_UNSIGNED_BYTE, texture);

    if (t == 0) {
	fullcycle = !fullcycle;
	if (fullcycle) {
	    sw++;
	    sw &= 255;
	}
    }
    t = (t + sw) % 255;

    glutPostRedisplay();
}

void 
init(void)
{
    int a,
        b;

    for (a = 0; a < texWidth; a++)
	for (b = 0; b < texHeight; b++)
	    texture[a][b] = 255;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &texName);
    glBindTexture(GL_TEXTURE_2D, texName);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, texWidth, texHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, texture);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(1.0 / tSlices, 1.0 / zSlices, 1.0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0, 1.0, 1, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(distance * sin(phi) * cos(theta), distance * sin(phi) * sin(theta), distance * cos(phi),
	      0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

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
    glutCreateWindow("Munching squares texture replacement");

    glutDisplayFunc(cbDisplay);
    glutKeyboardFunc(cbKeyboard);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);
    glutIdleFunc(cbIdle);

    glutCreateMenu(cbMenu);
    glutAddMenuEntry("Toggle wireframe mode ('w')", 'w');
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    init();
    glutMainLoop();
    return 0;
}
