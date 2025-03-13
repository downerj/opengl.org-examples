#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "glut.h"
#include "texture.h"
#ifdef _WIN32
#define sinf(x) ((float)sin((x)))
#define cosf(x) ((float)cos((x)))
#define sqrtf(x) ((float)sqrt(x))
#define floorf(x) ((float)floor(x))
#define M_PI 3.14159265
#define drand48()   ((float)rand()/(float)RAND_MAX)
#endif

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if(error = glGetError())                                       \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

#ifndef FALSE
enum {FALSE, TRUE};
#endif
enum {OBJ_ANGLE, OBJ_TRANSLATE, OBJ_PICK};
enum {X, Y, Z, W};

/* window dimensions */
int winWidth = 512;
int winHeight = 512;
int active;
int maxobject;

GLfloat objangle[2] = {0.f, 0.f};
GLfloat objpos[2] = {0.f, 0.f};

GLfloat color[4] = {1.f, 1.f, 1.f, 1.f};
GLfloat zero[4] = {0.f, 0.f, 0.f, 1.f};
GLfloat one[4] = {1.f, 1.f, 1.f, 1.f};

GLfloat lightpos[4] = {1.f, 1.f, 1.f, 0.f};
static int flip;

struct bbox_t { float x0, y0, x1, y1; };
struct bbox_t obj_bbox[10];

enum { OCCLUSION_MAP, ALL_OBJECTS, OCCLUDED_OBJECTS, NORMAL_OBJECTS, OCCLUDERS, DEPTH_BUFFER };
static int mode = NORMAL_OBJECTS;

#define MAX_OCCLUDERS 10
#define MAX_OBJECTS 10

struct object_t  {
    int id;
    float x, y, z;
    float r, g, b, a;
    float a0, a1, a2, a3, a4;
    struct bbox_t bbox;
} occluder[MAX_OCCLUDERS] = {
    {
    6, /* octahedron */
    -30.f, 0.f, 0.f,
    1.f, 0.2f, 0.2f, 1.0f,
    0.f, 0.f, 0.f, 0.f, 0.f,
    0.0, 0.0, 0.0, 0.0,
    },

    {
    3, /* sphere */
    70.f, 0.f, 0.f,
    0.2f, 0.2f, 1.f, 1.f,
    0.f, 0.f, 0.f, 0.f,
    0.0, 0.0, 0.0, 0.0,
    }
};

struct object_t object[MAX_OBJECTS] = {
    {
    4,	/* torus */
    -30.f, 0.f, -100.f,
    0.2f, 1.f, 0.2f, 1.0f,
    90.f, 1.f, 0.f, 0.f,
    0.0, 0.0, 0.0, 0.0,
    },
};

void
reshape(int wid, int ht) {
    float a;
    winWidth = wid;
    winHeight = ht;
    glViewport(0, 0, wid, ht);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    a = ((float)winWidth)/winHeight;
    if (a > 1)
	glFrustum(-100.*a, 100.*a, -100., 100., 300., 600.); 
    else
	glFrustum(-100., 100., -100.*a, 100.*a, 300., 600.); 
    glMatrixMode(GL_MODELVIEW);
}

void
motion(int x, int y) {

    switch(active)
    {
    case OBJ_ANGLE:
	objangle[X] = (x - winWidth/2) * 360./winWidth;
	objangle[Y] = (y - winHeight/2) * 360./winHeight;
	glutPostRedisplay();
	break;
    case OBJ_PICK:
	glutPostRedisplay();
	break;
    case OBJ_TRANSLATE:
	objpos[X] = (x - winWidth/2) * 100./winWidth;
	objpos[Y] = (winHeight/2 - y) * 100./winHeight;
	glutPostRedisplay();
	break;
    }
}

void
mouse(int button, int state, int x, int y) {
    if(state == GLUT_DOWN)
	switch(button) {
	case GLUT_LEFT_BUTTON: /* move the light */
	    active = OBJ_ANGLE;
	    motion(x, y);
	    break;
	case GLUT_MIDDLE_BUTTON:
	    active = OBJ_PICK;
	    motion(x, y);
	    break;
	case GLUT_RIGHT_BUTTON: /* move the polygon */
	    active = OBJ_TRANSLATE;
	    motion(x, y);
	    break;
	}
}

void
draw_normal(void) {
    int i;
    for(i = 0; i < MAX_OCCLUDERS; i++) {
	if (occluder[i].id == 0) continue;
	glPushMatrix();
	glTranslatef(occluder[i].x, occluder[i].y, occluder[i].z);
	glColor4f(occluder[i].r, occluder[i].g, occluder[i].b, occluder[i].a);
	if (occluder[i].a0 > 0)
	    glRotatef(occluder[i].a0, occluder[i].a1, occluder[i].a2, occluder[i].a3);
	glCallList(occluder[i].id);
	glPopMatrix();
    }

    for(i = 0; i < MAX_OBJECTS; i++) {
	if (object[i].id == 0) continue;
	glPushMatrix();
	glTranslatef(object[i].x, object[i].y, object[i].z);
	glColor4f(object[i].r, object[i].g, object[i].b, object[i].a);
	if (object[i].a0 > 0)
	    glRotatef(object[i].a0, object[i].a1, object[i].a2, object[i].a3);
	glCallList(object[i].id);
	glPopMatrix();
    }
}

void
draw_map(void) {
    int i;

    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);

    for(i = 0; i < MAX_OCCLUDERS; i++) {
	if (occluder[i].id == 0) continue;
	glPushMatrix();
	glTranslatef(occluder[i].x, occluder[i].y, occluder[i].z);
	/*glColor4f(occluder[i].r, occluder[i].g, occluder[i].b, occluder[i].a);*/
	glColor4f(1.f, 1.f, 1.f, 1.f);
	if (occluder[i].a0 > 0)
	    glRotatef(occluder[i].a0, occluder[i].a1, occluder[i].a2, occluder[i].a3);
	glCallList(occluder[i].id);
	glPopMatrix();
    }

    glClearColor(1.f, 1.f, 1.f, 0.f);
    glEnable(GL_LIGHTING);
}

void
draw_all(void) {
    int i;
    for(i = 0; i < MAX_OCCLUDERS; i++) {
	if (occluder[i].id == 0) continue;
	glPushMatrix();
	glTranslatef(occluder[i].x, occluder[i].y, occluder[i].z);
	glColor4f(occluder[i].r, occluder[i].g, occluder[i].b, occluder[i].a);
	if (occluder[i].a0 > 0)
	    glRotatef(occluder[i].a0, occluder[i].a1, occluder[i].a2, occluder[i].a3);
	glCallList(occluder[i].id);
	glPopMatrix();
    }

    for(i = 0; i < MAX_OBJECTS; i++) {
	if (object[i].id == 0) continue;
	glPushMatrix();
	glTranslatef(object[i].x, object[i].y, object[i].z);
	glColor4f(object[i].r, object[i].g, object[i].b, object[i].a);
	if (object[i].a0 > 0)
	    glRotatef(object[i].a0, object[i].a1, object[i].a2, object[i].a3);
	glCallList(object[i].id);
	glPopMatrix();
    }
}

void
draw_occluders(void) {
    int i;
    for(i = 0; i < MAX_OCCLUDERS; i++) {
	if (occluder[i].id == 0) continue;
	glPushMatrix();
	glTranslatef(occluder[i].x, occluder[i].y, occluder[i].z);
	glColor4f(occluder[i].r, occluder[i].g, occluder[i].b, occluder[i].a);
	if (occluder[i].a0 > 0)
	    glRotatef(occluder[i].a0, occluder[i].a1, occluder[i].a2, occluder[i].a3);
	glCallList(occluder[i].id);
	glPopMatrix();
    }
}

void
draw_occluded(void) {
    int i;
    for(i = 0; i < MAX_OBJECTS; i++) {
	if (object[i].id == 0) continue;
	glPushMatrix();
	glTranslatef(object[i].x, object[i].y, object[i].z);
	glColor4f(object[i].r, object[i].g, object[i].b, object[i].a);
	if (object[i].a0 > 0)
	    glRotatef(object[i].a0, object[i].a1, object[i].a2, object[i].a3);
	glCallList(object[i].id);
	glPopMatrix();
    }
}

void
redraw(void) {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    glPushMatrix(); /* assuming modelview */
if (flip) {
glTranslatef(0.f, -30.f, 0.f);
glRotatef(90.f, 1.f, 0.f, 0.f);
}
    glTranslatef(objpos[X], objpos[Y], 0.f); /* translate object */
    glRotatef(objangle[X], 0.f, 1.f, 0.f); /* rotate object */
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);
    
#if 0
    if(lightchanged[UPDATE_OGL])
    {
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	lightchanged[UPDATE_OGL] = GL_FALSE;
    }
#endif
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    switch(mode) {
    case OCCLUSION_MAP: draw_map(); break;
    case ALL_OBJECTS:   draw_all(); break;
    case OCCLUDED_OBJECTS: draw_occluded(); break;
    case NORMAL_OBJECTS:  draw_normal(); break;
    case OCCLUDERS: draw_occluders();
    }

    glPopMatrix(); /* assuming modelview */

    CHECK_ERROR("OpenGL Error in redraw()");
    glutSwapBuffers(); 
}

void
help(void) {
    printf("ESC  - quit\n");
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y) {
    static int wire;
    switch(key)
    {
    case 'f':
	flip ^= 1;
	glutPostRedisplay();
	break;
    case 'w':
	if (wire ^= 1)
	    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
	    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glutPostRedisplay();
	break;
    case 'm':
	mode = OCCLUSION_MAP;
	glutPostRedisplay();
	break;
    case 's':
	mode = NORMAL_OBJECTS;
	glutPostRedisplay();
	break;
    case 'o':
	mode = OCCLUDERS;
	glutPostRedisplay();
	break;
    case 'e':
	mode = OCCLUDED_OBJECTS;
	glutPostRedisplay();
	break;
    case 'd':
	mode = DEPTH_BUFFER;
	glutPostRedisplay();
	break;
    case 'a':
	mode = ALL_OBJECTS;
	glutPostRedisplay();
	break;
    case '\033':
	exit(0);
	break;
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
    glutAddMenuEntry("show occlusion map", 'm');
    glutAddMenuEntry("show occluded geometry", 'e');
    glutAddMenuEntry("show scene", 's');
    glutAddMenuEntry("show depth estimation buffer", 'd');
    glutAddMenuEntry("show occluders only", 'o');
    glutAddMenuEntry("show all", 'a');
    glutAddMenuEntry("exit", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

static void
cylinder(int facets, int ribs, float radius, float length) {
    int facet, rib;

    for (rib=0; rib < ribs; rib++) {
	float x, y, z;
	glBegin(GL_TRIANGLE_STRIP);
	for (facet=0; facet < facets; facet++) {
	    double angle = facet * (2.0 * M_PI / facets);
	    x = sin(angle) * radius;
	    z = cos(angle) * radius;
	    y = rib * (length / ribs);
	    glNormal3f(sin(angle), 0.f, cos(angle));
	    glVertex3f(x, y, z);
	    glVertex3f(x, (rib+1)*(length/ribs), z);
	}
	x = sin(0.) * radius;
	z = cos(0.) * radius;
	y = rib * (length / ribs);
	glNormal3f(sin(0.), 0.f, cos(0.));
	glVertex3f(x, y, z);
	glVertex3f(x, (rib+1)*(length/ribs), z);
	glEnd();
    }
    for(rib = 0; rib < 2; rib++) {
        float x, y, z;
	glNormal3f(0.f, rib == 0 ? -1.f : 1.f, 0.f);
	glBegin(GL_TRIANGLE_FAN);
	for (facet=0; facet < facets; facet++) {
	    double angle = facet * (2.0 * M_PI / facets);
	    x = sin(angle) * radius;
	    z = cos(angle) * radius;
	    y = rib * length;
	    glVertex3f(x, y, z);
	}
	x = sin(0.) * radius;
	z = cos(0.) * radius;
	y = rib * length;
	glVertex3f(x, y, z);
	glEnd();
    }
}

main(int argc, char *argv[]) {
    GLUquadric *quad;

    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
    (void)glutCreateWindow("occull");
    glutDisplayFunc(redraw);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);
    create_menu();

    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-100., 100., -100., 100., 300., 600.); 
    glMatrixMode(GL_MODELVIEW);
    /* look at scene from (0, 0, 450) */
    gluLookAt(0., 0., 450., 0., 0., 0., 0., 1., 0.);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    
    glCullFace(GL_BACK);
    glReadBuffer(GL_BACK);
    glDisable(GL_DITHER);

    CHECK_ERROR("end of main");

    glNewList(1, GL_COMPILE);
    glutSolidCube(50.f);
    glEndList();
    obj_bbox[1].x0 = -50.f;
    obj_bbox[1].y0 = -50.f;
    obj_bbox[1].x1 = 50.f;
    obj_bbox[1].y1 = 50.f;

    glNewList(2, GL_COMPILE);
    glutSolidTeapot(50.f);
    glEndList();
    obj_bbox[2].x0 = -50.f;
    obj_bbox[2].y0 = -50.f;
    obj_bbox[2].x1 = 50.f;
    obj_bbox[2].y1 = 50.f;

    quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);

    glNewList(3, GL_COMPILE);
    gluSphere(quad, 40., 50, 50);
    glEndList();
    obj_bbox[3].x0 = -50.f;
    obj_bbox[3].y0 = -50.f;
    obj_bbox[3].x1 = 50.f;
    obj_bbox[3].y1 = 50.f;

    gluDeleteQuadric(quad);

    glNewList(4, GL_COMPILE);
    glutSolidTorus(13.f, 33.f, 40, 60);
    glEndList();

    glNewList(5, GL_COMPILE);
    glPushMatrix();
    glTranslatef(0.f, -80.f, 0.f);
    cylinder(40, 3, 20, 160);
    glTranslatef(0.f, 20.f, 0.f);
    cylinder(40, 3, 60, 20);
    glTranslatef(0.f, 20.f, 0.f);
    cylinder(40, 3, 30, 20);
    glTranslatef(0.f, 60.f, 0.f);
    cylinder(40, 3, 30, 20);
    glTranslatef(0.f, 20.f, 0.f);
    cylinder(40, 3, 60, 20);
    glPopMatrix();
    glEndList();

    glNewList(6, GL_COMPILE);
    glScalef(50.f, 50.f, 50.f);
    glutSolidOctahedron();
    glEndList();
    maxobject = 6;
    glEnable(GL_NORMALIZE);
    glClearColor(1.f, 1.f, 1.f, 1.f);
    {
    float specular[] = { 1.0f, 1.0f, 1.f, 1.f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    }
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50);

    glutMainLoop();
    return 0;
}
