#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <math.h>
#include "texture.h"

/*
 * Point structure 
 */
typedef struct {
    int x, y;
    unsigned char r, g, b;
} Point;

/*
 * The reference image 
 */
static char mandrill[] = "../../data/mandrill.rgb";
GLuint *img;
int w, h, comp;

/*
 * The brush image 
 */
#define brushh 32
#define brushw 32
GLubyte brush[brushh][brushw][4];

#define maxPoints 65536
int numPoints = 0, viewImage = 0;
int moving = 0;
Point points[maxPoints];

/*
 * Create the brush texture 
 */
void 
createBrush(void)
{
    int x, y, dx, dy, value;
    float d, b;

    b = sqrt(brushw * brushw + brushh * brushh);

    for (y = 0; y < brushh; y++)
	for (x = 0; x < brushw; x++) {
	    dx = x - (brushw / 2);
	    dy = y - (brushh / 2);
	    d = sqrt(dx * dx + dy * dy);

	    value = (pow((rand() / (double) RAND_MAX), 0.75) * brushw / 2 > d) ? 255 : 0;

	    brush[y][x][0] = brush[y][x][1] = brush[y][x][2] = brush[y][x][3] = value;

	    /*
	     * Set the falloff 
	     */
	    if (value == 255)
		brush[y][x][3] = (unsigned char) (255 * ((0.66 * 2 * d / b) + 0.34));
	}
}

/*
 * Take a point sample of the image 
 */
void 
samplePoint(int x, int y)
{
    GLubyte *bi = (GLubyte *) img;

    if (numPoints < maxPoints) {
	points[numPoints].x = x;
	points[numPoints].y = y;
	points[numPoints].r = bi[4 * (y * w + x)];
	points[numPoints].g = bi[4 * (y * w + x) + 1];
	points[numPoints].b = bi[4 * (y * w + x) + 2];
	numPoints++;
    }
}

/*
 * Display callback 
 */
void 
cbDisplay(void)
{
    int i;

    glClear(GL_COLOR_BUFFER_BIT);

    if (viewImage) {
	glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, (GLubyte *) img);
    } else
	for (i = 0; i < numPoints; i++) {
	    glPushMatrix();
	    glTranslatef(points[i].x - brushw / 2, points[i].y - brushh / 2, 0);
	    glScalef(brushw, brushh, 0);

	    glColor3ub(points[i].r, points[i].g, points[i].b);
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
	}

    glutSwapBuffers();
}

/*ARGSUSED1*/
void 
cbKeyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 27:
    case 'q':
    case 'Q':
	exit(0);

    case 'c':
    case 'C':
	numPoints = 0;
	break;
    case 'i':
    case 'I':
	viewImage = !viewImage;

	if (viewImage)
	    glDisable(GL_TEXTURE_2D);
	else
	    glEnable(GL_TEXTURE_2D);
	glutChangeToMenuEntry(2, viewImage ? "View artistic image ('i')" : "View source image ('i')", 'i');
	break;

    default:
	return;
    }
    glutPostRedisplay();
}

void 
menu(int option)
{
    cbKeyboard((unsigned char) option, 0, 0);
}

void 
cbMotion(int x, int y)
{
    if (moving && x >= 0 && x < w && y >= 0 && y < w) {
	samplePoint(x, w - 1 - y);
	glutPostRedisplay();
    }
}

void 
cbMouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON) {
	moving = (state == GLUT_DOWN);
	cbMotion(x, y);
    }
}

void 
init(char *fname)
{
    GLuint brushName;

    /*
     * Load the reference image 
     */
    if (!(img = (GLuint *)read_texture(fname, &w, &h, &comp))) {
	printf("Could not open image\n");
	exit(1);
    }
    glutReshapeWindow(w, h);

    /*
     * Brush texture 
     */
    glGenTextures(1, &brushName);
    glBindTexture(GL_TEXTURE_2D, brushName);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    createBrush();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, brushw, brushh, 0, GL_RGBA, GL_UNSIGNED_BYTE, brush);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_TEXTURE_2D);

    /*
     * Matrices 
     */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /*
     * Misc 
     */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


/*
 * Parse arguments, and set up interface between OpenGL and window system 
 */
int
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitWindowSize(20, 20);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutCreateWindow("Impressionist");
    glutDisplayFunc(cbDisplay);
    glutKeyboardFunc(cbKeyboard);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);

    glutCreateMenu(menu);
    glutAddMenuEntry("Clear ('c')", 'c');
    glutAddMenuEntry("View source image ('i')", 'i');
    glutAddMenuEntry("Exit (<Esc>)", 27);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    init((argc > 1) ? argv[1] : mandrill);
    glutMainLoop();
    return 0;
}
