/*
 *  bubble.c
 *  David Blythe, 1997
 *
 *  An example of creating a bubble effect by perturbing a sphere with
 *  a sine function.
 *
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

#ifdef _WIN32
#define sinf(x) ((float)sin((x)))
#define cosf(x) ((float)cos((x)))
#ifndef M_PI
#define M_PI 3.14159265
#endif
#endif

static float transp = .5f;
static int normals;
static float phase = .05f;
static int tess = 10;
static float freq = 10.f, scale = .03f;
static float transx = 1.0f, transy, rotx, roty;
static int ox = -1, oy = -1;
static int mot = 0;
#define PAN	1
#define ROT	2

void
pan(const int x, const int y) {
    transx +=  (x-ox)/5.;
    transy -= (y-oy)/5.;
    ox = x; oy = y;
    glutPostRedisplay();
}

void
rotate(const int x, const int y) {
    rotx += x-ox;
    if (rotx > 360.) rotx -= 360.;
    else if (rotx < -360.) rotx += 360.;
    roty += y-oy;
    if (roty > 360.) roty -= 360.;
    else if (roty < -360.) roty += 360.;
    ox = x; oy = y;
    glutPostRedisplay();
}

void
motion(int x, int y) {
    if (mot == PAN) pan(x, y);
    else if (mot == ROT) rotate(x,y);
}

void
mouse(int button, int state, int x, int y) {

    /* hack for 2 button mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;
    
	if(state == GLUT_DOWN) {
	switch(button) {
	case GLUT_LEFT_BUTTON:
	    mot = PAN;
	    motion(ox = x, oy = y);
	    break;
	case GLUT_RIGHT_BUTTON:
	    mot = ROT;
	    motion(ox = x, oy = y);
	    break;
	case GLUT_MIDDLE_BUTTON:
	    break;
	}
    } else if (state == GLUT_UP) {
	mot = 0;
    }
}

void
init(char *fname) {
    int width, height, components;
    GLubyte *image = NULL;

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    image = (GLubyte*)read_texture(fname, &width, &height, &components);
    if (!image) {
	perror(fname);
	exit(EXIT_FAILURE);
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glMaterialf (GL_FRONT, GL_SHININESS, 64.0);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
    {
    GLfloat pos[4] = { 0, 0, 1, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_BLEND);
}

/*
 * bilinear (longitude/lattitude) tesselation
 * x = cos(theta)*cos(phi)
 * y = sin(phi)
 * z = sin(theta)*cos(phi)
 */

float*
normalize(float *v) {
    static float vv[3];
    float len;

    len = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
    vv[0] = v[0]/len; vv[1] = v[1]/len; vv[2] = v[2]/len;
    return vv;
}

#define N tess

void
bubble(float rad) {
    float v[3],s;
    float phi, mod;
    static float off;
    int i, j;

    off += phase;

    /* top cap */
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.f, 1.f, 0.f);
    glVertex3f(0.f, 1.f, 0.f);
    v[1] = rad*sinf(phi = M_PI*(N+2)/(2*N));
    mod = 1+scale*sinf(freq*v[1]+off);
    s = cosf(phi)*rad*mod;
    for(i = 0; i <= 2*N; i++) {
	v[0] = s*cosf((M_PI/N)*i);
	v[2] = s*sinf((M_PI/N)*i);
	glNormal3fv(v);
	glVertex3fv(v);
    }
    glEnd();
    for(i = 1; i < N-1; i++) {
	float v0[3], v1[3], s0, s1;
	v0[1] = rad*sinf(phi=M_PI/2+M_PI/N*i); s0 = rad*cosf(phi);
	mod = mod = 1+scale*sin(freq*v0[1]+off); s0 *= mod;
	v1[1] = rad*sinf(phi=M_PI/2+M_PI/N*(i+1)); s1 = rad*cosf(phi);
	mod = mod = 1+scale*sin(freq*v1[1]+off); s1 *= mod;
	glBegin(GL_TRIANGLE_STRIP);
	for(j = 0; j <= 2*N; j++) {
	    float x, z;
	    v0[0] = s0*(x = sinf(M_PI/N*j));
	    v0[2] = s0*(z = cosf(M_PI/N*j));
	    v1[0] = s1*x; v1[2] = s1*z;
	    glNormal3fv(normalize(v1));
	    glVertex3fv(v1);
	    glNormal3fv(normalize(v0));
	    glVertex3fv(v0);
	}
	glEnd();
	if (normals) {
	float nscale = 1.2f;
	glBegin(GL_LINES);
	for(j = 0; j <= 2*N; j++) {
	    GLfloat x[3], x1, z1, *vv;
	    v0[0] = s0*(x1 = sinf(M_PI/N*j));
	    v0[2] = s0*(z1 = cosf(M_PI/N*j));
	    glVertex3fv(vv = normalize(v0));
	    x[0] = nscale*vv[0]; x[1] = nscale*vv[1]; x[2] = nscale*vv[2];
	    glVertex3fv(x);

	    v1[0] = s1*x1; v1[2] = s1*z1;
	    glVertex3fv(vv = normalize(v1));
	    x[0] = nscale*vv[0]; x[1] = nscale*vv[1]; x[2] = nscale*vv[2];
	    glVertex3fv(x);
	}
	glEnd();
	}
    }
    /* bottom cap */
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.f, -1.f, 0.f);
    glVertex3f(0.f, -1.f, 0.f);
    v[1] = -rad*sinf(phi=M_PI*(N+2)/(2*N));
    mod = 1+scale*sin(freq*v[1]+off);
    s = cosf(phi)*rad*mod;
    for(i = 2*N; i >= 0; --i) {
	v[0] = s*cosf((M_PI/N)*i);
	v[2] = s*sinf((M_PI/N)*i);
	glNormal3fv(v);
	glVertex3fv(v);
    }
    glEnd();
}

void
display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glTranslatef(0., 0., -100.+transx);
    glRotatef(roty, 1.0, 0.0, 0.0);
    glScalef(10.f, 10.f, 10.f);
    glColor4f(1.f,1.f,1.f,transp);
    bubble(1.0f);
    glPopMatrix ();
    glutSwapBuffers();
}

void
reshape(int w, int h) {
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40., 1.0, 10.0, 200000.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void
ffunc(void) {
    static int state = 1;
    if (state ^= 1)
	glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);
    else
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void
tfunc(void) {
    static int state = 1;
    if (state ^= 1)
	glEnable(GL_TEXTURE_2D);
    else
	glDisable(GL_TEXTURE_2D);
}

void
lfunc(void) {
    static int state = 1;
    if (state ^= 1)
	glEnable(GL_LIGHTING);
    else
	glDisable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
}

void
bfunc(void) {
    static int state = 1;
    if (state ^= 1) {
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
    } else {
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
    }
}

void
cfunc(void) {
    static int state = 1;
    if (state ^= 1)
	glEnable(GL_CULL_FACE);
    else
	glDisable(GL_CULL_FACE);
}

void
wfunc(void) {
    static int state;
    glPolygonMode(GL_FRONT_AND_BACK, (state ^= 1) ? GL_LINE : GL_FILL);
}

void
ufunc(void) {
	tess *= 2;
	if (tess > 160) tess = 160;
}
void
Ufunc(void) {
	tess /= 2;
	if (tess < 10) tess = 10;
}

void
pfunc(void) {
    phase += .01f;
}

void
Pfunc(void) {
    phase -= .01f;
}

void
nfunc(void) {
    normals ^= 1;
}

void
xfunc(void) {
    transp += .1f;
}

void
Xfunc(void) {
    transp -= .1f;
}

void
yfunc(void) {
    static int state;
    if (state ^= 1)
	glClearColor(.0f,.0f,.65f,0.f);
    else
	glClearColor(0.f, 0.f, 0.f, 0.f);
}

void
up(void) {
    scale += .01f;
}
void
down(void) {
    scale -= .01f;
}
void
left(void) {
    freq -= 1.f;
}
void
right(void) {
    freq += 1.f;
}

/*ARGSUSED1*/
void
key (unsigned char key, int x, int y) {
    switch (key) {
    case 'b': bfunc(); break;
    case 'c': cfunc(); break;
    case 'l': lfunc(); break;
    case 't': tfunc(); break;
    case 'f': ffunc(); break;
    case 'n': nfunc(); break;
    case 'u': ufunc(); break;
    case 'U': Ufunc(); break;
    case 'p': pfunc(); break;
    case 'P': Pfunc(); break;
    case 'w': wfunc(); break;
    case 'x': xfunc(); break;
    case 'X': Xfunc(); break;
    case 'y': yfunc(); break;
    case '\033': exit(EXIT_SUCCESS); break;
    default: break;
    }
}

/*ARGSUSED1*/
void
special(int key, int x, int y) {
    switch(key) {
    case GLUT_KEY_UP:	up(); break;
    case GLUT_KEY_DOWN:	down(); break;
    case GLUT_KEY_LEFT:	left(); break;
    case GLUT_KEY_RIGHT:right(); break;
    }
}

void
animate(void) {
    glutPostRedisplay();
}

void
visible(int state)
{
    if (state == GLUT_VISIBLE)
	glutIdleFunc(animate);
    else
	glutIdleFunc(NULL);
}

int
main(int argc, char** argv) {
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);

    glutCreateWindow(argv[0]);
    init(argc > 1 ? argv[1] : "../../data/spheremap.rgb");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);
    glutIdleFunc(animate);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutVisibilityFunc(visible);

	printf("c   - toggle backface culling\n");
	printf("l   - toggle lighting\n");
	printf("t   - toggle texturing\n");
	printf("u/U - increase/decrease tesselation\n");
	printf("w   - toggle wireframe/filled polygons\n");
	printf("y   - toggle background color\n\n");
	printf("Use the left/right arrows to adjust the frequency of the motion.\n");
	printf("Use the up/down arrows to adjust the scale of the motion.\n\n");

    glutMainLoop();
    return 0;
}
