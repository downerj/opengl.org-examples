#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#ifdef _WIN32
#ifndef M_PI
#define M_PI 3.14159265f
#endif
#endif

#define texWidth 128
#define texHeight 128

int px, py, moving = -1;
GLuint drawMode = GL_FILL;
int showTexture = 0;
float theta = M_PI, phi = M_PI / 3.f, distance = 5.0f;
GLubyte texture[texHeight][texWidth][4];
GLuint texName[2];

/*
 * Generate the mosaic texture 
 */
void 
generateTexture(void)
{
    int x, y;

    /*
     * Sides 
     */
    for (y = 0; y < texHeight / 4; y++) {
	for (x = 0; x < texWidth; x++) {
	    texture[y][x][0] = 255;
	    texture[y][x][1] = (x < texWidth / 8 || x > 7 * texWidth / 8 ||
		 y < texHeight / 16 || y > 3 * texHeight / 16) ? 255 : 0;
	    texture[y][x][2] = 255;
	    texture[y][x][3] = 255;
	}
    }

    /*
     * Top/bottom 
     */
    for (y = texHeight / 4; y < texHeight / 2; y++) {
	for (x = 0; x < texWidth; x++) {
	    texture[y][x][0] = 0;
	    texture[y][x][1] = ((x % (texWidth / 4)) < 4) ? 0 : 255;
	    texture[y][x][2] = texture[y][x][1];
	    texture[y][x][3] = 255;
	}
    }

    /*
     * First end 
     */
    for (y = texHeight / 2; y < texHeight; y++) {
	for (x = 0; x < texWidth / 2; x++) {
	    int flag = (x > 3 * texWidth / 16 && x <= 5 * texWidth / 16 && y > 5 * texHeight / 8 && y <= 7 * texHeight / 8) ||
	    (x > texWidth / 8 && x <= 3 * texWidth / 8 && y > 11 * texHeight / 16 && y <= 13 * texHeight / 16);

	    texture[y][x][0] = flag ? 255 : 0;
	    texture[y][x][1] = flag ? 255 : 0;
	    texture[y][x][2] = flag ? 0 : 255;
	    texture[y][x][3] = 255;
	}
    }

    /*
     * Second end 
     */
    for (y = texHeight / 2; y < texHeight; y++) {
	for (x = texWidth / 2; x < texWidth; x++) {
	    int flag = ((x - 3 * texWidth / 4) * (x - 3 * texWidth / 4) + (y - 3 * texHeight / 4) * (y - 3 * texHeight / 4)) <
	    ((texWidth * texWidth / 64) + (texHeight * texHeight / 64));

	    texture[y][x][0] = flag ? 0 : 255;
	    texture[y][x][1] = flag ? 255 : 0;
	    texture[y][x][2] = 0;
	    texture[y][x][3] = 255;
	}
    }
}

/*
 * Draw a box using the mosaic texture 
 */
void 
drawBox(void)
{
    glPushMatrix();
    glTranslatef(-0.5, -0.5, 0.0);

    glTranslatef(0.0, 0.0, -1.0);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0.0, 0.5);
    glVertex2f(0.0, 0.0);
    glTexCoord2f(0.5, 0.5);
    glVertex2f(1.0, 0.0);
    glTexCoord2f(0.0, 1.0);
    glVertex2f(0.0, 1.0);
    glTexCoord2f(0.5, 1.0);
    glVertex2f(1.0, 1.0);
    glEnd();

    glTranslatef(0.0, 0.0, 2.0);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0.5, 0.5);
    glVertex2f(0.0, 0.0);
    glTexCoord2f(0.5, 1.0);
    glVertex2f(0.0, 1.0);
    glTexCoord2f(1.0, 0.5);
    glVertex2f(1.0, 0.0);
    glTexCoord2f(1.0, 1.0);
    glVertex2f(1.0, 1.0);
    glEnd();

    glTranslatef(0.0, 0.0, -1.0);
    glRotatef(90, 0.0, 1.0, 0.0);
    glTranslatef(0.0, 0.0, 1.0);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0.0, 0.0);
    glVertex2f(-1.0, 0.0);
    glTexCoord2f(0.0, 0.25);
    glVertex2f(-1.0, 1.0);
    glTexCoord2f(1.0, 0.0);
    glVertex2f(1.0, 0.0);
    glTexCoord2f(1.0, 0.25);
    glVertex2f(1.0, 1.0);
    glEnd();
    glTranslatef(0.0, 0.0, -1.0);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0.0, 0.0);
    glVertex2f(-1.0, 0.0);
    glTexCoord2f(0.0, 0.25);
    glVertex2f(-1.0, 1.0);
    glTexCoord2f(1.0, 0.0);
    glVertex2f(1.0, 0.0);
    glTexCoord2f(1.0, 0.25);
    glVertex2f(1.0, 1.0);
    glEnd();

    glRotatef(90, 1.0, 0.0, 0.0);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0.0, 0.25);
    glVertex2f(-1.0, 0.0);
    glTexCoord2f(0.0, 0.5);
    glVertex2f(-1.0, 1.0);
    glTexCoord2f(1.0, 0.25);
    glVertex2f(1.0, 0.0);
    glTexCoord2f(1.0, 0.5);
    glVertex2f(1.0, 1.0);
    glEnd();
    glTranslatef(0.0, 0.0, -1.0);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0.0, 0.25);
    glVertex2f(-1.0, 0.0);
    glTexCoord2f(0.0, 0.5);
    glVertex2f(-1.0, 1.0);
    glTexCoord2f(1.0, 0.25);
    glVertex2f(1.0, 0.0);
    glTexCoord2f(1.0, 0.5);
    glVertex2f(1.0, 1.0);
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

    if (showTexture) {
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, 1.0, 0.0, 1.0);

	glBegin(GL_QUADS);
	glTexCoord2i(0, 0);
	glVertex2i(0, 0);
	glTexCoord2i(1, 0);
	glVertex2i(1, 0);
	glTexCoord2i(1, 1);
	glVertex2i(1, 1);
	glTexCoord2i(0, 1);
	glVertex2i(0, 1);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
    } else
	drawBox();

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
	phi -= 0.01 * (y - py);
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

    case 'm':
    case 'M':
	glBindTexture(GL_TEXTURE_2D, texName[1]);
	break;
    case 's':
    case 'S':
	glBindTexture(GL_TEXTURE_2D, texName[0]);
	break;
    case 't':
    case 'T':
	showTexture = !showTexture;
	glutChangeToMenuEntry(4, showTexture ? "Show object ('t')" : "Show texture ('t')", 't');
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
    generateTexture();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(2, texName);
    glBindTexture(GL_TEXTURE_2D, texName[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glBindTexture(GL_TEXTURE_2D, texName[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, texWidth, texHeight, GL_RGBA, GL_UNSIGNED_BYTE, texture);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0, 1.0, 1, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(distance * sin(phi) * cos(theta), distance * sin(phi) * sin(theta), distance * cos(phi),
	      0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    glBindTexture(GL_TEXTURE_2D, texName[0]);
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
    glutCreateWindow("Texture mosaicing");

    glutDisplayFunc(cbDisplay);
    glutKeyboardFunc(cbKeyboard);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);

    glutCreateMenu(cbMenu);
    glutAddMenuEntry("Use single level texture ('s')", 's');
    glutAddMenuEntry("Use mipmapped texture ('m')", 'm');
    glutAddMenuEntry("Toggle wireframe mode ('w')", 'w');
    glutAddMenuEntry("Show texture ('t')", 't');
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    init();
    glutMainLoop();
    return 0;
}
