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
#define fabsf(x) ((float)fabs(x))
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
int object = 2;
int maxobject;

GLfloat objangle[2] = {0.f, 0.f};
GLfloat objpos[2] = {0.f, 0.f};

GLfloat color[4] = {1.f, 1.f, 1.f, 1.f};
GLfloat zero[4] = {0.f, 0.f, 0.f, 1.f};
GLfloat one[4] = {1.f, 1.f, 1.f, 1.f};

GLfloat lightpos[4] = {0.f, 0.f, 1.f, 0.f};

static void texgen_cyl_map(float *pnt, const float *norm, const float *eyePos, float *s, float *t, float r );

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
	switch(button)
	{
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
redraw(void) {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    glPushMatrix(); /* assuming modelview */
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

    glCallList(object);

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
    case 'o':
	/* toggle object type */
	object++;
	if(object > maxobject)
	    object = 1;
	glutPostRedisplay();
	break;
    case 'w':
	if (wire ^= 1)
	    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
	    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glutPostRedisplay();
	break;
    case 's':
	glDisable(GL_LIGHTING);
	glBindTexture(GL_TEXTURE_2D, 1);
	glEnable(GL_TEXTURE_2D);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glutPostRedisplay();
	break;
    case 'n':
	glEnable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glutPostRedisplay();
	break;
    case 'c':
	glBindTexture(GL_TEXTURE_2D, 2);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
#if 0
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
#else
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
#endif
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
    glutAddMenuEntry("spheremap (s)", 's');
    glutAddMenuEntry("stripes (c)", 'c');
    glutAddMenuEntry("no texturing (n)", 'n');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    glutAddMenuEntry("exit", '\033');
}

static void
xformpt(float d[3], const float s[3], float m[16]) {
    d[0] = m[4*0+0]*s[0] + m[4*1+0]*s[1] + m[4*2+0]*s[2] + m[4*3+0];
    d[1] = m[4*0+1]*s[0] + m[4*1+1]*s[1] + m[4*2+1]*s[2] + m[4*3+1];
    d[2] = m[4*0+2]*s[0] + m[4*1+2]*s[1] + m[4*2+2]*s[2] + m[4*3+2];
}

static void
cylinder(int facets, int ribs, float radius, float length) {
    int facet, rib;
    float n[3], v[3], v1[3], eye[3], s, t, r;

    float mat[16]; glGetFloatv(GL_MODELVIEW_MATRIX, mat);

    eye[0] = 0; eye[1] = 0; eye[2] = 450;
    r = 10.0f;

    for (rib=0; rib < ribs; rib++) {
	float x, y, z;
	glBegin(GL_TRIANGLE_STRIP);
	for (facet=0; facet < facets; facet++) {
	    double angle = facet * (2.0 * M_PI / facets);
	    x = sin(angle) * radius;
	    z = cos(angle) * radius;
	    y = rib * (length / ribs);
	    glNormal3f(sin(angle), 0.f, cos(angle));

#if 1
	    n[0] = sin(angle); n[1] = 0.f; n[2] = cos(angle);
	    v[0] = x; v[1] = y; v[2] = z;
	    xformpt(v1, v, mat);
	    texgen_cyl_map(v1, n, eye, &s, &t, r); glTexCoord2f(s, t);
	    glVertex3f(x, y, z);

	    v[1] = (rib+1.f)*(length/ribs);
	    xformpt(v1, v, mat);
	    texgen_cyl_map(v1, n, eye, &s, &t, r); glTexCoord2f(s, t);
	    glVertex3f(x, (rib+1)*(length/ribs), z);
#else
	    glVertex3f(x, y, z);
	    glVertex3f(x, (rib+1)*(length/ribs), z);
#endif
	}
	x = sin(0.) * radius;
	z = cos(0.) * radius;
	y = rib * (length / ribs);
	glNormal3f(sin(0.), 0.f, cos(0.));
#if 1
	    n[0] = sin(0.); n[1] = 0.f; n[2] = cos(0.);
	    v[0] = x; v[1] = y; v[2] = z;
	    xformpt(v1, v, mat);
	    texgen_cyl_map(v1, n, eye, &s, &t, r); glTexCoord2f(s, t);
	    glVertex3f(x, y, z);

	    v[1] = (rib+1)*(length/ribs);
	    xformpt(v1, v, mat);
	    texgen_cyl_map(v1, n, eye, &s, &t, r); glTexCoord2f(s, t);
	    glVertex3f(x, (rib+1)*(length/ribs), z);
#else
	glVertex3f(x, y, z);
	glVertex3f(x, (rib+1)*(length/ribs), z);
#endif
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
#if 1
	    n[0] = 0.f; n[1] = rib == 0 ? -1.f : 1.f; n[2] = 0.f;
	    v[0] = x; v[1] = y; v[2] = z;
	    texgen_cyl_map(v, n, eye, &s, &t, r); glTexCoord2f(s, t);
#endif
	    glVertex3f(x, y, z);
	}
	x = sin(0.) * radius;
	z = cos(0.) * radius;
	y = rib * length;
#if 1
	    n[0] = 0.f; n[1] = rib == 0 ? -1.f : 1.f; n[2] = 0.f;
	    v[0] = x; v[1] = y; v[2] = z;
	    texgen_cyl_map(v, n, eye, &s, &t, r); glTexCoord2f(s, t);
#endif
	glVertex3f(x, y, z);
	glEnd();
    }
}

main(int argc, char *argv[]) {
    GLUquadric *quad;

    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
    (void)glutCreateWindow("locate");
    glutDisplayFunc(redraw);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);
    create_menu();

    glClearColor(0.0f, 0.3f, 0.0f, 0.f);
    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-100., 100., -100., 100., 300., 600.); 
    glMatrixMode(GL_MODELVIEW);
    /* look at scene from (0, 0, 450) */
    gluLookAt(0., 0., 450., 0., 0., 0., 0., 1., 0.);
    {
    int i;
    float mat[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mat);
    for(i = 0; i < 4; i++)
	printf("%f %f %f %f\n", mat[4*i+0], mat[4*i+1], mat[4*i+2], mat[4*i+3]);
    }

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

    glNewList(2, GL_COMPILE);
    glutSolidTeapot(50.f);
    glEndList();

    quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);

    glNewList(3, GL_COMPILE);
    gluSphere(quad, 70., 20, 20);
    glEndList();

    gluDeleteQuadric(quad);

    glNewList(4, GL_COMPILE);
    glutSolidTorus(20, 50, 20, 20);
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
    glPushMatrix();
    glTranslatef(0.f, -80.f, 0.f);
    cylinder(40, 3, 80, 160);
    glPopMatrix();
    glEndList();

    maxobject = 6;
    {
    GLubyte* tex;
    int texwid, texht, texcomps;
    tex = (GLubyte*)read_texture("../../data/spheremap.rgb", &texwid, &texht, &texcomps);
    glBindTexture(GL_TEXTURE_2D, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texwid, texht, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    free(tex);
    }
    {

    GLubyte tex2[1024];
    int i;
    for(i = 0; i < 1024; i++) {
	if ((i/10)&1)
	    tex2[i] = 0;
	else tex2[i] = 255;
    }
    glBindTexture(GL_TEXTURE_2D, 2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 1024, 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, tex2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glutMainLoop();
    return 0;
}

/* A fast atan2 approximation */

static float act(float x,float y)
{
   register float z,r,r2;
   register int flag;

#define ac0  0.9998660
#define ac1 -0.3302995
#define ac2  0.1801410
#define ac3 -0.0851330
#define ac4  0.0208351

   if (fabsf(x) >= fabsf(y))
   {
      r = y/x;
      flag = 1;
   }
   else 
   {
      r = x/y;
      flag = 0;
   }

   r2 = r*r;

   z = (float) (r*(ac0 + r2*(ac1 + r2*(ac2 + r2*(ac3 + r2*ac4)))));

   if (flag)
   {
      if (x >= (float)0.0)
	 z = (float)( 0.5*M_PI) - z;
      else
	 z = (float)(-0.5*M_PI) - z;
   }
   else if (y < (float)0.0)
   {
      if (x >= (float)0.0)
	 z = (float)( M_PI) + z;
      else
	 z = (float)(-M_PI) + z;
   }

   return(z);
}

static void
texgen_cyl_map(float *pnt, const float *norm, const float *eyePos, float *s, float *t, float r )
{
   float u;          /* Parametric value of ray cylinder intersection */
   float rr;         /* Reflection direction */
   float rd[3];      /* The reflection vector */
   float a, b, c, d; /* Quadractic equation coefficients */
   float y, z;       /* Intersection point on cylinder */
   float eyeDir[3];  /* Direction of the local viewer */


   { float t;
   t = pnt[0] ; pnt[0] = pnt[2]; pnt[2] = t;
   }
   /* Find the direction of the local viewer */
   eyeDir[0] = eyePos[0] - pnt[0];
   eyeDir[1] = eyePos[1] - pnt[1];
   eyeDir[2] = eyePos[2] - pnt[2];

   /* Reflect the viewing direction abount the normal */
   rr    = (float) (2.0 * (eyeDir[0]*norm[0] + eyeDir[1]*norm[1] +  eyeDir[2]*norm[2]));

   rd[0] = rr*norm[0] - eyeDir[0];
   rd[1] = rr*norm[1] - eyeDir[1];
   rd[2] = rr*norm[2] - eyeDir[2];

   /* cylinder intersection; cylinder is along x axis */
   a = rd[1]*rd[1] + rd[2]*rd[2];
   b = (float) (2.0*(pnt[1]*rd[1] + pnt[2]*rd[2]));
   c = pnt[1]*pnt[1] + pnt[2]*pnt[2] - r*r;
   d = (float) (b*b-4.0 * a*c);

   if ( a == 0.0 || d <= 0.0 ) {
      d = 0; /* set to zero if multiple imaginary solutions exist */
      a = 1;
      *s = 0.5;
      *t = 0.5;
   } else {
      d = (float) sqrt(d);

      /* because a and d are positive, we know +d is larger */
      u = (float) (0.5*(-b+d) / a);

      y = pnt[1] + u*rd[1];
      z = pnt[2] + u*rd[2];

      /* Find the angle and map it to [0..1] texture coord space */
      *s = (float) ((act(-y,z)/M_PI  + 1.0) * 0.5);
      *t = 0.5;
   }
}
