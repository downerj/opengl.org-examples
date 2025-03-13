#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#define maxVertices 6

enum {
    None, Round, Miter, Bevel
} join = None;
int lineWidth = 3.0,
    closed = 1,
    maxPoint;
int v[maxVertices][2],
    numVertices = 0,
    drawLines = 1;

static int winWidth = 400;
static int winHeight = 400;

/*
 * Display callback 
 */
void
cbDisplay(void)
{
    int i;

    glClear(GL_COLOR_BUFFER_BIT);

    /*
     * Draw the lines 
     */
    glLineWidth(lineWidth);
    glBegin(closed ? GL_LINE_LOOP : GL_LINE_STRIP);
    for (i = 0; i < numVertices; i++)
	if (drawLines)
	    glVertex2i(v[i][0], v[i][1]);
    glEnd();

    /*
     * Draw the joins 
     */
    switch (join) {
    case Bevel:
	glBegin(GL_TRIANGLES);
	for (i = (closed ? 0 : 1); i < (closed ? numVertices : numVertices - 1); i++) {
	    float dx0 = v[i][0] - v[(i + numVertices - 1) % numVertices][0];
	    float dy0 = v[i][1] - v[(i + numVertices - 1) % numVertices][1];
	    float dx1 = v[(i + 1) % numVertices][0] - v[i][0];
	    float dy1 = v[(i + 1) % numVertices][1] - v[i][1];

	    glVertex2i(v[i][0], v[i][1]);

	    if (fabs(dx0) > fabs(dy0))
		glVertex2i(v[i][0], v[i][1] - (lineWidth - 1) / 2 + ((dy1 < 0) ? lineWidth : 0));
	    else
		glVertex2i(v[i][0] - (lineWidth - 1) / 2 + ((dx1 < 0) ? lineWidth : 0), v[i][1]);

	    if (fabs(dx1) > fabs(dy1))
		glVertex2i(v[i][0], v[i][1] - (lineWidth - 1) / 2 + ((dy0 > 0) ? lineWidth : 0));
	    else
		glVertex2i(v[i][0] - (lineWidth - 1) / 2 + ((dx0 > 0) ? lineWidth : 0), v[i][1]);
	}
	glEnd();
	break;

    case Miter:
	glBegin(GL_TRIANGLES);
	for (i = (closed ? 0 : 1); i < (closed ? numVertices : numVertices - 1); i++) {
	    float dx0 = v[i][0] - v[(i + numVertices - 1) % numVertices][0];
	    float dy0 = v[i][1] - v[(i + numVertices - 1) % numVertices][1];
	    float dx1 = v[(i + 1) % numVertices][0] - v[i][0];
	    float dy1 = v[(i + 1) % numVertices][1] - v[i][1];

	    glVertex2i(v[i][0], v[i][1]);

	    if (fabs(dx0) > fabs(dy0))
		glVertex2i(v[i][0], v[i][1] - (lineWidth - 1) / 2 + ((dy1 < 0) ? lineWidth : 0));
	    else
		glVertex2i(v[i][0] - (lineWidth - 1) / 2 + ((dx1 < 0) ? lineWidth : 0), v[i][1]);

	    if (fabs(dx1) > fabs(dy1))
		glVertex2i(v[i][0], v[i][1] - (lineWidth - 1) / 2 + ((dy0 > 0) ? lineWidth : 0));
	    else
		glVertex2i(v[i][0] - (lineWidth - 1) / 2 + ((dx0 > 0) ? lineWidth : 0), v[i][1]);
	}
	glEnd();
	break;

    case Round:
	if (lineWidth <= maxPoint || 1) {
	    /*
	     * Use antialiased points 
	     */
	    glPointSize(lineWidth - 1);
	    glBegin(GL_POINTS);
	    for (i = (closed ? 0 : 1); i < (closed ? numVertices : numVertices - 1); i++)
		glVertex2i(v[i][0], v[i][1]);
	    glEnd();
	} else {
	    /*
	     * Too big for an antialiased point, simulate it with a triangle fan 
	     */
	    printf("draw big points\n");

	}
	break;

    case None:
	break;
    }

    glutSwapBuffers();
}

/*
 * Mouse button callback 
 */
void
cbMouse(int button, int state, int x, int y)
{
    y = winHeight - y - 1;

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
	if (closed) {
	    closed = 0;
	    numVertices = 1;
	    v[0][0] = x;
	    v[0][1] = y;
	    glutPostRedisplay();
	} else {
	    v[numVertices][0] = x;
	    v[numVertices][1] = y;
	    closed = (++numVertices == maxVertices);
	    glutPostRedisplay();
	}
    } else if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN) {
	closed = 1;
	glutPostRedisplay();
    }
}

/*
 * Keyboard callback 
 */
/*ARGSUSED*/
void
cbKeyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 27:
    case 'q':
	exit(0);

    case '+':
	lineWidth++;
	break;
    case '-':
	if (lineWidth > 1)
	    lineWidth--;
	break;

    case 'b':
    case 'B':
	join = Bevel;
	break;
    case 'm':
    case 'M':
	join = Miter;
	break;
    case 'n':
    case 'N':
	join = None;
	break;
    case 'r':
    case 'R':
	join = Round;
	break;

    case 'd':
	drawLines = !drawLines;
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
    int sizes[2];

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, winWidth-1.0, 0.0, winHeight-1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.375, 0.375, 0.0);

    glGetIntegerv(GL_POINT_SIZE_RANGE, sizes);
    maxPoint = sizes[1];

    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0, 0.0, 0.667, 1.0);
}

void
reshape(int wid, int ht)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /*gluOrtho2D(0.0, winWidth = wid, 0.0, winHeight = ht);*/
    glOrtho(0.0, winWidth = wid, 0.0, winHeight = ht, -1., 1.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.375, 0.375, 0.0);
    glViewport(0, 0, wid, ht);
}

void
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(winWidth, winHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("2D Line Joins");

    glutDisplayFunc(cbDisplay);
    glutKeyboardFunc(cbKeyboard);
    glutMouseFunc(cbMouse);
    glutReshapeFunc(reshape);

    glutCreateMenu(cbMenu);
    glutAddMenuEntry("Increase line width ('+')", '+');
    glutAddMenuEntry("Decrease line width ('-')", '-');

    glutAddMenuEntry("Use bevel join ('b')", 'b');
    glutAddMenuEntry("Use miter join ('m')", 'm');
    glutAddMenuEntry("Use round join ('r')", 'r');
    glutAddMenuEntry("Use no join ('n')", 'n');
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    init();
    glutMainLoop();
}
