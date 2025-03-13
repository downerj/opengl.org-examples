/*
 *  explode.c
 *  David Blythe, 1997
 *
 *  An example of how to use texture mapping to simulate an explosion.
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
#define atan2f(x, y) ((float)atan2((x), (y)))
#ifndef M_PI
#define M_PI 3.14159265
#endif
#endif

GLuint texnames[2];
static int texture = 1;
static float rot = 0.f;
static float opacity = 1.0f;
static float intensity = 1.0f;
static float size = .2f, delta = 0.f;
static float scale = 1.;
static float transx, transy, rotx, roty;
static int ox = -1, oy = -1;
static int mot = 0;
#define PAN	1
#define ROT	2

float corners[30][3];
float traj[10][3];
/*
 * For the flying box pieces 
 */
float startcorners[30][3] = {
    {-1.0, -1.0, -1.0},
    {-1.0, 1.0, -1.0},
    {1.0, 1.0, -1.0},
    {1.0, 1.0, -1.0},
    {1.0, -1.0, -1.0},
    {-1.0, -1.0, -1.0},

    {-1.0, -1.0, 1.0},
    {1.0, -1.0, 1.0},
    {1.0, 1.0, 1.0},
    {1.0, 1.0, 1.0},
    {-1.0, 1.0, 1.0},
    {-1.0, -1.0, 1.0},

    {-1.0, 1.0, -1.0},
    {1.0, 1.0, -1.0},
    {1.0, 1.0, 1.0},
    {1.0, 1.0, 1.0},
    {-1.0, 1.0, 1.0},
    {-1.0, 1.0, -1.0},

    {1.0, -1.0, -1.0},
    {1.0, 1.0, -1.0},
    {1.0, 1.0, 1.0},
    {1.0, 1.0, 1.0},
    {1.0, -1.0, 1.0},
    {1.0, -1.0, -1.0},

    {-1.0, -1.0, -1.0},
    {-1.0, 1.0, -1.0},
    {-1.0, 1.0, 1.0},
    {-1.0, 1.0, 1.0},
    {-1.0, -1.0, 1.0},
    {-1.0, -1.0, -1.0}
};
float normals[10][3] = {
    {0.0, 0.0, -1.0},
    {0.0, 0.0, -1.0},
    {0.0, 0.0, 1.0},
    {0.0, 0.0, 1.0},
    {0.0, 1.0, 0.0},
    {0.0, 1.0, 0.0},
    {1.0, 0.0, 0.0},
    {1.0, 0.0, 0.0},
    {-1.0, 0.0, 0.0},
    {-1.0, 0.0, 0.0}};
float axes[10][3] = {
    {-1.0, 1.0, 0.0},
    {1.0, -1.0, 0.0},
    {-1.0, 1.0, 0.0},
    {1.0, -1.0, 0.0},
    {1.0, 0.0, -1.0},
    {-1.0, 0.0, 1.0},
    {0.0, 1.0, -1.0},
    {0.0, -1.0, 1.0},
    {0.0, 1.0, -1.0},
    {0.0, -1.0, 1.0}};
float starttraj[10][3] = {
    {0.25, -0.25, -1.0},
    {-0.25, 0.25, -1.0},
    {0.25, -0.25, 1.0},
    {-0.25, 0.25, 1.0},
    {0.25, 1.0, -0.25},
    {-0.25, 1.0, 0.25},
    {0.25, -1.0, -0.25},
    {-0.25, -1.0, 0.25},
    {-1.0, 0.25, -0.25},
    {-1.0, -0.25, 0.25}};
float omega[10] = {20.0, 20.0, 20.0, 20.0, 20.0, 20.0, 20.0, 20.0, 20.0, 20.0};


/*
 * For the smoke 
 */
#define MAX_TIME	(2*196.0)
#define MAX_SMOKE	320
/* int smokes = 0; */
/* int smoke[MAX_SMOKE]; */


void
pan(int x, int y)
{
    transx += (x - ox) / 500.;
    transy -= (y - oy) / 500.;
    ox = x;
    oy = y;
    glutPostRedisplay();
}

void
rotate(int x, int y)
{
    rotx += x - ox;
    if (rotx > 360.) rotx -= 360.;
    else if (rotx < -360.) rotx += 360.;
    roty += y - oy;
    if (roty > 360.) roty -= 360.;
    else if (roty < -360.) roty += 360.;
    ox = x;
    oy = y;
    glutPostRedisplay();
}

void
motion(int x, int y)
{
    if (mot == PAN)
	pan(x, y);
    else if (mot == ROT)
	rotate(x, y);
}

void
mouse(int button, int state, int x, int y)
{

    /*
     * hack for 2 button mouse 
     */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;

    if (state == GLUT_DOWN) {
	switch (button) {
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
afunc(void)
{
    static int state;
    if (state ^= 1) {
	glAlphaFunc(GL_GREATER, .01f);
	glEnable(GL_ALPHA_TEST);
    } else {
	glDisable(GL_ALPHA_TEST);
    }
}

void
bfunc(void)
{
    static int state;
    if (state ^= 1) {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
    } else {
	glDisable(GL_BLEND);
    }
}

void
tfunc(void)
{
    texture ^= 1;
}

void 
up(void)
{
    scale += .1f;
}
void 
down(void)
{
    scale -= .1f;
}
void 
left(void)
{
    intensity -= .05f;
    if (intensity < 0.f)
	intensity = 0.0f;
}
void 
right(void)
{
    intensity += .05f;
    if (intensity > 1.f)
	intensity = 1.0f;
}

void
help(void)
{
    printf("Usage: explode [image]\n");
    printf("'h'            - help\n");
    printf("'a'            - toggle alpha test\n");
    printf("'b'            - toggle blend\n");
    printf("'t'            - toggle texturing\n");
    printf("'UP'           - scale up\n");
    printf("'DOWN'         - scale down\n");
    printf("'LEFT'	   - darken\n");
    printf("'RIGHT'	   - brighten\n");
    printf("left mouse     - pan\n");
    printf("right mouse    - rotate\n");
}

void explode(void);

void 
init(void)
{
    static unsigned *explosionimage,
            *smokeimage;
    static int width,
        height,
        components;

    explode();

    glGenTextures(2, texnames);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /*
     * Load the explosion texture 
     */
    explosionimage = read_texture("../../data/explosion0.rgba", &width, &height, &components);
    if (explosionimage == NULL) {
	fprintf(stderr, "Error: Can't load image file \"../../data/explosion0.rgba\".\n");
	exit(EXIT_FAILURE);
    } else
	printf("%d x %d image loaded\n", width, height);

    if (components != 2 && components != 4)
	printf("warning: texture should be an RGBA or LA image\n");

    glBindTexture(GL_TEXTURE_2D, texnames[0]);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, components, width, height, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, explosionimage);

    /*
     * Load the smoke texture 
     */
    smokeimage = read_texture("../../data/smoke.la", &width, &height, &components);
    if (explosionimage == NULL) {
	fprintf(stderr, "Error: Can't load image file \"../../data/smoke.la\".\n");
	exit(EXIT_FAILURE);
    } else
	printf("%d x %d image loaded\n", width, height);

    if (components != 2 && components != 4)
	printf("warning: texture should be an RGBA or LA image\n");

    glBindTexture(GL_TEXTURE_2D, texnames[1]);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, components, width, height, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, smokeimage);

    glEnable(GL_TEXTURE_2D);

    /*
     * Setup other OpenGL stuff 
     */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50., 1., .1, 20.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0., 0., -5.5);
    glClearColor(.25f, .25f, .75f, .25f);

    glAlphaFunc(GL_GREATER, 0.016f);
    glEnable(GL_ALPHA_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);

}

#define sgn(a) ((a) >= 0 ? 1.0 : -1.0)



void
animate(void)
{
    static int time = 0;
    int i;

    /*
     * Update the explosion texture map 
     */
    size += .07f;
    delta += .04f;
    rot += .4f;
    opacity -= .003f;

    /*
     * Move the fragments outward 
     */
    for (i = 0; i < 30; i++) {
	corners[i][0] += 0.7 * traj[i / 3][0];
	corners[i][1] += 0.7 * traj[i / 3][1];
	corners[i][2] += 0.7 * traj[i / 3][2];
    }

    /*
     * Make the fragments fall downwards 
     */
    for (i = 0; i < 10; i++)
	traj[i][1] -= 0.025f;

#if 0
    /*
     * Take care of the smoke 
     */
    if (smokes < MAX_SMOKE && (time % 3) == 0) {
	smoke[smokes++] = 5;
	time = 0;
    } else
#endif
	time++;

    glutPostRedisplay();
}

/*
 * blow it up again 
 */
void 
explode(void)
{
    static int freeze = 0;
    int i,
        j;
    for (i = 0; i < 30; i++)
	for (j = 0; j < 3; j++)
	    corners[i][j] = startcorners[i][j];

    for (i = 0; i < 10; i++)
	for (j = 0; j < 3; j++)
	    traj[i][j] = starttraj[i][j];


    size = 0.f;
    rot = 0.f;
    opacity = 1.f;
    intensity = 1.f;
    delta = 0.f;
    scale = 1.f;

    if (freeze) {
	freeze = 0;
	glutIdleFunc(NULL);
    } else {
	freeze = 1;
	glutIdleFunc(animate);
    }
}


static void calcMatrix(void);

void 
exploder(float x, float y, float z, float size, float intensity, float opacity, float delay, float scale)
{
    if (size - delay <= 0.f)
	return;

    /*
     * explosion 
     */
    glPushMatrix();
    calcMatrix();
    glTranslatef(x, y, z);
    glScalef((size - delay) * scale, (size - delay) * scale, 1.);
    if (texture)
	glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texnames[0]);
    glColor4f(intensity, intensity, intensity, opacity);
    glBegin(GL_POLYGON);
    glTexCoord2f(0.0, 0.0);
    glVertex2f(-1.0, -1.0);
    glTexCoord2f(1.0, 0.0);
    glVertex2f(1.0, -1.0);
    glTexCoord2f(1.0, 1.0);
    glVertex2f(1.0, 1.0);
    glTexCoord2f(0.0, 1.0);
    glVertex2f(-1.0, 1.0);
    glEnd();
    glPopMatrix();
}


void 
display(void)
{
    static int time = 0;
    int i;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
#define RAD(x) (((x)*M_PI)/180.)
    gluLookAt(-sinf(RAD(rotx)) * 5.5, transy, cosf(RAD(rotx)) * 5.5, 0., 0., 0., 0., 1., 0.);

    /*
     * floor 
     */
    glColor4f(0.f, .2f, 0.f, 1.f);
    glBegin(GL_POLYGON);
    glVertex3f(-4.0, -1.0, -4.0);
    glVertex3f(4.0, -1.0, -4.0);
    glVertex3f(4.0, -1.0, 4.0);
    glVertex3f(-4.0, -1.0, 4.0);
    glEnd();

    glEnable(GL_LIGHTING);
    glPushMatrix();
    glColor3f(.3f, .3f, .3f);
    glPushMatrix();
    glTranslatef(-1.f, -1.f + .2f, -1.5f);
    glScalef(.2f, .2f, .2f);

    /*
     * the box 
     */
    for (i = 0; i < 10; i++) {
	float ax = (corners[3 * i][0] + corners[3 * i + 1][0] + corners[3 * i + 2][0]) / 3,
	      ay = (corners[3 * i][1] + corners[3 * i + 1][1] + corners[3 * i + 2][1]) / 3,
	      az = (corners[3 * i][2] + corners[3 * i + 1][2] + corners[3 * i + 2][2]) / 3;

	glPushMatrix();
	glTranslatef(ax, ay, az);
	glRotatef(omega[i] * delta, axes[i][0], axes[i][1], axes[i][2]);
	glTranslatef(-ax, -ay, -az);

	glBegin(GL_TRIANGLES);
	glNormal3fv(normals[i]);
	glVertex3fv(corners[3 * i]);
	glVertex3fv(corners[3 * i + 1]);
	glVertex3fv(corners[3 * i + 2]);
	glEnd();

	glPopMatrix();
    }

    /*
     * base of the box (stays there) 
     */
    glBegin(GL_QUADS);
    glNormal3f(0.0, 1.0, 0.0);
    glVertex3f(-1.0, -1.0, -1.0);
    glVertex3f(-1.0, -1.0, 1.0);
    glVertex3f(1.0, -1.0, 1.0);
    glVertex3f(1.0, -1.0, -1.0);
    glEnd();
    glPopMatrix();
    glDisable(GL_LIGHTING);


    glTranslatef(-1.f, -1.f, -1.5f);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texnames[1]);
    glDepthMask(0);

#if 0
    for (i = smokes - 1; i >= 0; i--) {
	int t = smoke[i];

	glPushMatrix();
	glTranslatef(0.05 * sin(M_PI * (t + time) / 64.0), 0.02 * t, 0.05 * cos(M_PI * (t + time) / 64.0));

	glScalef((t + 60) / MAX_TIME, (t + 60) / MAX_TIME, 1.0);

	glColor4f(0.2, 0.2, 0.2, 10.0 / t);

	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex3f(-1.0, 0.0, -1.0);
	glTexCoord2f(1, 0);
	glVertex3f(1.0, 0.0, -1.0);
	glTexCoord2f(1, 1);
	glVertex3f(1.0, 0.0, 1.0);
	glTexCoord2f(0, 1);
	glVertex3f(-1.0, 0.0, 1.0);
	glEnd();
	glPopMatrix();

	smoke[i]++;
    }
#endif
    time++;
    glDisable(GL_TEXTURE_2D);

    glDisable(GL_DEPTH_TEST);
    exploder(0.f, 0.f, 0.f, size, intensity, opacity, 0.f, 3.f);

    exploder(0.f, 0.f, 1.f, size, intensity, opacity, .2f, 1.4f);

    exploder(0.f, .8f, 1.4f, size, intensity, opacity, .4f, 1.6f);

    exploder(0.f, 1.2f, 0.f, size - .4f, intensity, opacity, .4f, 2.f);

    exploder(1.6f, .3f, 0.f, size - 1.f, intensity, opacity, .5f, 3.f);
    glEnable(GL_DEPTH_TEST);


    glDepthMask(1);
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);

    glutSwapBuffers();
}

void 
reshape(int w, int h)
{
    glViewport(0, 0, w, h);
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y)
{
    switch (key) {
    case 'a':
	afunc();
	break;
    case 'b':
	bfunc();
	break;
    case 'h':
	help();
	break;
    case 't':
	tfunc();
	break;
    case 'e':
	explode();
	break;
    case '\033':
	exit(EXIT_SUCCESS);
	break;
    default:
	break;
    }
    glutPostRedisplay();
}

/*ARGSUSED1*/
void
special(int key, int x, int y)
{
    switch (key) {
    case GLUT_KEY_UP:
	up();
	break;
    case GLUT_KEY_DOWN:
	down();
	break;
    case GLUT_KEY_LEFT:
	left();
	break;
    case GLUT_KEY_RIGHT:
	right();
	break;
    }
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
main(int argc, char **argv)
{
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    glutCreateWindow(argv[0]);
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(animate);
    glutVisibilityFunc(visible);
    glutMainLoop();
    return 0;
}

void
printmat(float *m)
{
    int i;
    for (i = 0; i < 4; i++) {
	printf("%f %f %f %f\n", m[4 * i + 0], m[4 * i + 1], m[4 * i + 2], m[4 * i + 3]);
    }
}

void
buildRot(float theta, float x, float y, float z, float m[16])
{
    float d = x * x + y * y + z * z;
    float ct = cosf(RAD(theta)),
          st = sinf(RAD(theta));

    /*
     * normalize 
     */
    if (d > 0) {
	d = 1 / d;
	x *= d;
	y *= d;
	z *= d;
    }
    m[0] = 1; m[1] = 0; m[2] = 0; m[3] = 0;
    m[4] = 0; m[5] = 1; m[6] = 0; m[7] = 0;
    m[8] = 0; m[9] = 0; m[10] = 1; m[11] = 0;
    m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;

    /*
     * R = uu' + cos(theta)*(I-uu') + sin(theta)*S
     *  
     *   S =  0  -z   y    u' = (x, y, z)
     *        z   0  -x
     *       -y   x   0
     */

    m[0] = x * x + ct * (1 - x * x) + st * 0;
    m[4] = x * y + ct * (0 - x * y) + st * -z;
    m[8] = x * z + ct * (0 - x * z) + st * y;

    m[1] = y * x + ct * (0 - y * x) + st * z;
    m[5] = y * y + ct * (1 - y * y) + st * 0;
    m[9] = y * z + ct * (0 - y * z) + st * -x;

    m[2] = z * x + ct * (0 - z * x) + st * -y;
    m[6] = z * y + ct * (0 - z * y) + st * x;
    m[10] = z * z + ct * (1 - z * z) + st * 0;
}

static void
calcMatrix(void)
{
    float mat[16];

    glGetFloatv(GL_MODELVIEW_MATRIX, mat);

    buildRot(-180 * atan2f(mat[8], mat[10]) / M_PI, 0, 1, 0, mat);
    glMultMatrixf(mat);
}
