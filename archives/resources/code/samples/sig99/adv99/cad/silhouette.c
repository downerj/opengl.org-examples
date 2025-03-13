/*
**	Kurt Akeley
**	December 1995
**
**	Render a solid as a "useful" silhouette image.  Useful
**	means that visible real edges (whether silhouette or not) are
**	drawn, and silhouette edges are drawn, but tesselation edges
**	that are not silhouette edges are not drawn.
**
**	Note that the XXX enable/disable pairs are necessary for one
**	machine (I think it is Indy) that doesn't implement OpenGL
**	quite correctly :-)
**
**	June 1998
**
**	The silhouette algorithm works regardless of the orientation
**	of the polygons in an object.  Thus objects with opposite
**	orientations can be drawn in a single scene without substantial
**	errors in the resulting image.
*/

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <math.h>
#ifdef _WIN32
#define sinf(x) ((float)sin((x)))
#define cosf(x) ((float)cos((x)))
#define sqrtf(x) ((float)sqrt(x))
#define floorf(x) ((float)floor(x))
#ifndef M_PI
#define M_PI 3.14159265
#endif
#define drand48()   ((float)rand()/(float)RAND_MAX)
#endif
#ifdef GL_POLYGON_OFFSET_FILL
#undef GL_POLYGON_OFFSET_EXT
#define GL_POLYGON_OFFSET_EXT	GL_POLYGON_OFFSET_FILL
#endif

typedef float vector[3];

#define FACETS (64)
#define RIBS (2)
#define RADIUS (1)
#define LENGTH (2)

enum {SOLID, SILHOUETTE, SILHOUETTE_ONLY, SILHOUETTE2, HIDDEN_LINE, EDGES_ONLY};
int mode = SOLID;


/*
** global variables
*/
vector* cv = 0;
vector* cn = 0;
int mousex, mousey;
int leftmouse, middlemouse, rightmouse;
int winWidth = 400, winHeight = 400;


/*
** routine declarations
*/
static void BuildCylinder(vector** vlist, vector** nlist, int facets, int ribs,
			  float radius, float length);
static void DrawCylinder(vector* vlist, vector* nlist, int facets, int ribs);
static void DrawCylinderEdges(vector* vlist, vector* nlist, int facets,
			      int ribs);
static void InitGfx(void);
static void FillWindow(void);
static void DrawObjects(void);
static void DrawObjectEdges(void);
static void DrawIt(void);
static void CheckErrors(void);


/*
** Generate the vertex and normal data for a cylinder
*/
static void BuildCylinder(vector** vlist, vector** nlist, int facets, int ribs,
			  float radius, float length) {
    double angle;
    int facet, rib;

    /*
    ** Malloc the arrays
    */
    if (*vlist)
	free(*vlist);
    *vlist = (vector*)malloc(facets * (ribs+1) * sizeof(vector));
    if (nlist) {
	if (*nlist)
	    free(*nlist);
	*nlist = (vector*)malloc(facets * (ribs+1) * sizeof(vector));
    }

    /*
    ** Fill the vertex and normal arrays
    */
    for (rib=0; rib <= ribs; rib++) {
	for (facet=0; facet < facets; facet++) {
	    angle = facet * (2.0 * M_PI / facets);
	    (*vlist)[facets*rib+facet][0] = sin(angle) * radius;
	    (*vlist)[facets*rib+facet][1] = cos(angle) * radius;
	    (*vlist)[facets*rib+facet][2] = rib * (length / ribs);
	    if (nlist) {
		(*nlist)[facets*rib+facet][0] = sin(angle);
		(*nlist)[facets*rib+facet][1] = cos(angle);
		(*nlist)[facets*rib+facet][2] = 0;
	    }
	}
    }
}


/*
** Draw all the facets of a cylinder
*/
static void DrawCylinder(vector* vlist, vector* nlist, int facets, int ribs) {
    int rib, facet;
    int i, j, next;
    float height;

    for (rib=0; rib < ribs; rib++) {
	i = rib * facets;
	j = i + facets;
	glBegin(GL_QUADS);
	for (facet=0; facet < facets; facet+=1, i+=1, j+=1) {
	    next = (facet == (facets-1)) ? 1-facets : 1;
	    glNormal3fv(nlist[i]);
	    glVertex3fv(vlist[i]);
	    glNormal3fv(nlist[i+next]);
	    glVertex3fv(vlist[i+next]);
	    glNormal3fv(nlist[j+next]);
	    glVertex3fv(vlist[j+next]);
	    glNormal3fv(nlist[j]);
	    glVertex3fv(vlist[j]);
	}
	glEnd();
    }

    glBegin(GL_TRIANGLES);
    i = 0;
    j = ribs * facets;
    height = vlist[j][2];
    for (facet=0; facet < facets; facet+=1, i+=1, j+=1) {
	next = (facet == (facets-1)) ? 1-facets : 1;
	glNormal3f(0,0,-1);
	glVertex3f(0,0,0);
	glVertex3fv(vlist[i+next]);
	glVertex3fv(vlist[i]);
	glNormal3f(0,0,1);
	glVertex3f(0,0,height);
	glVertex3fv(vlist[j]);
	glVertex3fv(vlist[j+next]);
    }
    glEnd();
}


/*
** Draw only the true edges of the cylinder (the circles at the top and
**   the bottom), not the edges between polygons on the same surface.
*/
/*ARGSUSED*/
static void DrawCylinderEdges(vector* vlist, vector* nlist, int facets,
			      int ribs) {
    int facet;
    int i, j, next;

    glBegin(GL_LINES);
    i = 0;
    j = ribs * facets;
    for (facet=0; facet < facets; facet+=1, i+=1, j+=1) {
	next = (facet == (facets-1)) ? 1-facets : 1;
	glVertex3fv(vlist[i]);
	glVertex3fv(vlist[i+next]);
	glVertex3fv(vlist[j]);
	glVertex3fv(vlist[j+next]);
    }
    glEnd();
}

void
help(void) {
    printf("ESC  - quit\n");
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case '\033':
	exit(0); break;
    case 's':
	mode = SOLID;
	glutPostRedisplay(); break;
    case 'x':
	mode = SILHOUETTE2;
	glutPostRedisplay(); break;
    case 'e':
	mode = SILHOUETTE;
	glutPostRedisplay(); break;
    case 't':
	mode = SILHOUETTE_ONLY;
	glutPostRedisplay(); break;
    case 'h':
	mode = HIDDEN_LINE;
	glutPostRedisplay(); break;
    case 'o':
	mode = EDGES_ONLY;
	glutPostRedisplay(); break;
    default:
	help();
	break;
    }
}

void
menu(int which) {
    key((char)which, 0, 0);
}

void
create_menu(void) {
    glutCreateMenu(menu);
    glutAddMenuEntry("solid", 's');
    glutAddMenuEntry("hidden line", 'h');
    glutAddMenuEntry("silhouette", 't');
    glutAddMenuEntry("silhouette2", 'x');
    glutAddMenuEntry("silhouettes + edges", 'e');
    glutAddMenuEntry("edges", 'o');
    glutAddMenuEntry("exit", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void
mouse(int button, int state, int x, int y)
{
    /* hack for 2 button mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;

    if(state == GLUT_DOWN)
	switch(button) {
	case GLUT_LEFT_BUTTON:
	    leftmouse = 1;
	    mousex = x; mousey = y;
	    break;
	case GLUT_MIDDLE_BUTTON:
	    middlemouse = 1;
	    mousex = x; mousey = y;
	    break;
	case GLUT_RIGHT_BUTTON:
	    rightmouse = 1;
	    mousex = x; mousey = y;
	    break;
	}
    else if(state == GLUT_UP)
	switch(button) {
	case GLUT_LEFT_BUTTON:
	    leftmouse = 0;
	    break;
	case GLUT_MIDDLE_BUTTON:
	    middlemouse = 0;
	    break;
	case GLUT_RIGHT_BUTTON:
	    rightmouse = 0;
	    break;
	}
}

void
motion(int x, int y) {
    mousex = x; mousey = y;
    glutPostRedisplay();
}

static void
redraw(void)
{
    DrawIt();
}

static void
reshape(int wid, int ht) {
    winWidth = wid;
    winHeight = ht;
    InitGfx();
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    /*glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL|GLUT_DOUBLE);*/
    glutInitDisplayString("samples stencil>=2 rgb double depth");
    (void)glutCreateWindow("silhouette");
    glutDisplayFunc(redraw);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);
    create_menu();

    /*
    ** initialize parameters
    */
    mousex = mousey = 0;
    leftmouse = middlemouse = rightmouse = 0;
    InitGfx();
    glutMainLoop();
    return 0;
}


/*
** Initialize graphics state.  Routines that change these initial
** state values reset them before returning.
*/
static void InitGfx(void) {
    static int done = 0;
    if (!done) {
	done = 1;
	glClearColor(0,0,0,1);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90, 1, 0.5, 10.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -5);
	glClearStencil(0);
#ifdef GL_POLYGON_OFFSET_FILL
	glPolygonOffset(4.0, 1.0);
#else
	/* XXX should change to use glPolygonOffset in OpenGL 1.1 */
	glPolygonOffsetEXT(4.0, (1.0 / (1<<22)));
#endif
	glFrontFace(GL_CW);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	BuildCylinder(&cv, &cn, FACETS, RIBS, RADIUS, LENGTH);
    }
    glViewport(0, 0, winWidth, winHeight);
}


/*
** Fill the window with a single rectangle
*/
static void FillWindow(void) {
    /*
    ** set the matrix state
    */
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);

    /*
    ** draw a rectangle the size of the window
    */
    glDisable(GL_DEPTH_TEST);
    glRecti(0, 0, 1, 1);
    glEnable(GL_DEPTH_TEST);

    /*
    ** pop the matrixes
    */
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}


/*
** draw two cylinders, one each with CW and CCW polygon orientation
*/
static void DrawObjects(void) {
    /*
    ** draw the first cylinder
    */
    DrawCylinder(cv, cn, FACETS, RIBS);

    /*
    ** draw a second cylinder with opposite polygon orientation
    */
    glPushMatrix();
    glTranslatef(0.f, 1.2f*LENGTH, 0);
    glFrontFace(GL_CCW);
    DrawCylinder(cv, cn, FACETS, RIBS);
    glFrontFace(GL_CW);
    glPopMatrix();
}


/*
** draw the true edges of two cylinders
*/
static void DrawObjectEdges(void) {
    DrawCylinderEdges(cv, cn, FACETS, RIBS);

    glPushMatrix();
    glTranslatef(0.f, 1.2f*LENGTH, 0);
    DrawCylinderEdges(cv, cn, FACETS, RIBS);
    glPopMatrix();
}


/*
** draw the entire scene in one of three modes
*/
static void DrawIt(void) {
    /*
    ** clear all the buffers
    */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    /*
    ** track the mouse
    */
    glPushMatrix();
    glRotatef(0.5 * mousex, 0, 1, 0);
    glRotatef(0.5 * mousey, 0, 0, 1);
    glTranslatef(-0, -1., (-LENGTH / 2.0));

    /*
    ** render visible lines only
    */
    if (mode == HIDDEN_LINE) {
	/*
	** render the offset depth image
	*/
	glEnable(GL_POLYGON_OFFSET_EXT);
	glColorMask(0,0,0,0);
	DrawObjects();
	glColorMask(1,1,1,1);
	glDisable(GL_POLYGON_OFFSET_EXT);

	/*
	** make no further changes to the depth image
	*/
	glDepthMask(0);
	glDisable(GL_DEPTH_TEST); /* XXX */
	glEnable(GL_DEPTH_TEST); /* XXX */

	/*
	** render all polygons in outline mode, testing against the
	**   depth image to eliminate hidden lines.
	*/
	glColor3f(1,1,1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	DrawObjects();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	/*
	** return state to default values
	*/
	glDepthMask(1);
	glDisable(GL_DEPTH_TEST); /* XXX */
	glEnable(GL_DEPTH_TEST); /* XXX */
    }

    /*
    ** render silhouette edges only
    */
    else if (mode == SILHOUETTE_ONLY) {
	/*
	** render the offset depth image
	*/
	glEnable(GL_POLYGON_OFFSET_EXT);
	glColorMask(0,0,0,0);
	DrawObjects();
	glColorMask(1,1,1,1);
	glDisable(GL_POLYGON_OFFSET_EXT);

	/*
	** make no further changes to the depth image
	*/
	glDepthMask(0);
	glDisable(GL_DEPTH_TEST); /* XXX */
	glEnable(GL_DEPTH_TEST); /* XXX */

	/*
	** cull all facets of one (arbitrary) orientation.  render the
	**   remaining facets in outline mode, toggling the stencil bit
	**   at each pixel.
	*/
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 0, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
	glEnable(GL_CULL_FACE);
	glColorMask(0,0,0,0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	DrawObjects();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColorMask(1,1,1,1);
	glDisable(GL_CULL_FACE);

	/*
	** color all pixels in the framebuffer with stencil value 1
	*/
	glStencilFunc(GL_EQUAL, 1, 1);
	glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
	glColor3f(1,1,1);
	FillWindow();
	glDisable(GL_STENCIL_TEST);

	/*
	** return state to default values
	*/
	glDepthMask(1);
	glDisable(GL_DEPTH_TEST); /* XXX */
	glEnable(GL_DEPTH_TEST); /* XXX */
    }

    /*
    ** render silhouette edges and true edges
    */
    else if (mode == SILHOUETTE) {
	/*
	** render the offset depth image
	*/
	glEnable(GL_POLYGON_OFFSET_EXT);
	glColorMask(0,0,0,0);
	DrawObjects();
	glColorMask(1,1,1,1);
	glDisable(GL_POLYGON_OFFSET_EXT);

	/*
	** make no further changes to the depth image
	*/
	glDepthMask(0);
	glDisable(GL_DEPTH_TEST); /* XXX */
	glEnable(GL_DEPTH_TEST); /* XXX */

	/*
	** cull all facets of one (arbitrary) orientation.  render the
	**   remaining facets in outline mode, toggling the stencil bit
	**   at each pixel.
	*/
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 0, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
	glEnable(GL_CULL_FACE);
	glColorMask(0,0,0,0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	DrawObjects();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColorMask(1,1,1,1);
	glDisable(GL_CULL_FACE);

	/*
	** color all pixels in the framebuffer with stencil value 1
	*/
	glStencilFunc(GL_EQUAL, 1, 1);
	glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
	glColor3f(1,1,1);
	FillWindow();
	glDisable(GL_STENCIL_TEST);

	/*
	** draw all true edges, testing against the depth image
	*/
	glColor3f(1,1,1);
	DrawObjectEdges();

	/*
	** return state to default values
	*/
	glDepthMask(1);
	glDisable(GL_DEPTH_TEST); /* XXX */
	glEnable(GL_DEPTH_TEST); /* XXX */
    }

    /*
    ** render silhouette edges using a different algorithm
    */
    else if (mode == SILHOUETTE2) {
	/*
	** render image translated in 4 directions
	** counting each time a pixel pass the depth test
	*/
	glEnable(GL_CULL_FACE);
	glColorMask(0,0,0,0);
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 0, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(1,0,winWidth,winHeight);
	DrawObjects();
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0,1,winWidth,winHeight);
	DrawObjects();
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(-1,0,winWidth,winHeight);
	DrawObjects();
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0,-1,winWidth,winHeight);
	DrawObjects();
	glViewport(0,0,winWidth,winHeight);
	glColorMask(1,1,1,1);
	glDisable(GL_CULL_FACE);

	/*
	** color all pixels in the framebuffer with stencil value 1
	*/
#if 1
	glStencilFunc(GL_EQUAL, 2, 0xff);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glColor3f(1,1,1);
	FillWindow();
#endif
	glStencilFunc(GL_EQUAL, 3, 0xff);
	glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
	glColor3f(1,1,1);
	FillWindow();
	glDisable(GL_STENCIL_TEST);

	/*
	** return state to default values
	*/
	glDepthMask(1);
	glDisable(GL_DEPTH_TEST); /* XXX */
	glEnable(GL_DEPTH_TEST); /* XXX */
    }
    /*
    ** render boundary edges only
    */
    else if(mode == EDGES_ONLY) {
	glColor3f(1,1,1);
	DrawObjectEdges();
    }

    /*
    ** render shaded surfaces with hidden surfaces eliminated
    */
    else {
	glColor3f(1, 0, 0);
	glEnable(GL_LIGHTING);
	DrawObjects();
	glDisable(GL_LIGHTING);
    }

    glPopMatrix();

    glutSwapBuffers();
    CheckErrors();
}


/*
** check for OpenGL errors.
*/
static void CheckErrors(void) {
    int i;
    int goterror;

    goterror = 0;
    while ((i = glGetError()) != GL_NO_ERROR) {
	goterror = 1;
	fprintf(stderr, "ERROR: 0x%x\n", i);
	fprintf(stderr, "       %s\n", gluErrorString(i));
    }
    if (goterror)
	exit(1);
}
