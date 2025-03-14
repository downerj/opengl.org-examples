#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#ifdef _WIN32
#include <sys/timeb.h>
#define random	rand
#else
#include <sys/time.h>
#define gettimeofday(a) gettimeofday(a, NULL)
#endif

GLUquadricObj *cone, *base, *sphere;
GLuint conePattern[32], spherePattern[32];

void 
create_stipple_pattern(GLuint * pat, GLfloat opacity)
{
    int x, y;
    long threshold = (long) ((float) 0x7fffffff * (1.f - opacity));

    for (y = 0; y < 32; y++) {
	pat[y] = 0;
	for (x = 0; x < 32; x++) {
	    if (random() > threshold)
		pat[y] |= (1 << x);
	}
    }
}

void 
init(void)
{
    static GLfloat lightpos[] = {.5, .75, 1.5, 1};

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    cone = gluNewQuadric();
    base = gluNewQuadric();
    sphere = gluNewQuadric();
    gluQuadricOrientation(base, GLU_INSIDE);

    create_stipple_pattern(spherePattern, .5);
    create_stipple_pattern(conePattern, .5);
}

void 
reshape(int w, int h)
{
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, 1, .01, 10);
    gluLookAt(0, 0, 2.577, 0, 0, -5, 0, 1, 0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void 
draw_room(void)
{
    /* material for the walls, floor, ceiling */
    static GLfloat wall_mat[] = {1.f, 1.f, 1.f, 1.f};

    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wall_mat);

    glBegin(GL_QUADS);

    /* floor */
    glNormal3f(0, 1, 0);
    glVertex3f(-1, -1, 1);
    glVertex3f(1, -1, 1);
    glVertex3f(1, -1, -1);
    glVertex3f(-1, -1, -1);

    /* ceiling */
    glNormal3f(0, -1, 0);
    glVertex3f(-1, 1, -1);
    glVertex3f(1, 1, -1);
    glVertex3f(1, 1, 1);
    glVertex3f(-1, 1, 1);

    /* left wall */
    glNormal3f(1, 0, 0);
    glVertex3f(-1, -1, -1);
    glVertex3f(-1, -1, 1);
    glVertex3f(-1, 1, 1);
    glVertex3f(-1, 1, -1);

    /* right wall */
    glNormal3f(-1, 0, 0);
    glVertex3f(1, 1, -1);
    glVertex3f(1, 1, 1);
    glVertex3f(1, -1, 1);
    glVertex3f(1, -1, -1);

    /* far wall */
    glNormal3f(0, 0, 1);
    glVertex3f(-1, -1, -1);
    glVertex3f(1, -1, -1);
    glVertex3f(1, 1, -1);
    glVertex3f(-1, 1, -1);

    glEnd();
}

void 
draw_cone(void)
{
    static GLfloat cone_mat[] = {0.f, .5f, 1.f, 1.f};

    glPushMatrix();
    glTranslatef(0, -1, 0);
    glRotatef(-90, 1, 0, 0);

    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cone_mat);
    gluCylinder(cone, .3, 0, 1.25, 20, 1);
    gluDisk(base, 0., .3, 20, 1);

    glPopMatrix();
}

void 
draw_sphere(GLdouble angle)
{
    static GLfloat sphere_mat[] = {1.f, .5f, 0.f, 1.f};

    glPushMatrix();
    glTranslatef(0.f, -.3f, 0.f);
    glRotatef(angle, 0.f, 1.f, 0.f);
    glTranslatef(0.f, 0.f, .6f);

    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
    gluSphere(sphere, .3, 20, 20);

    glPopMatrix();
}

GLdouble 
get_secs(void)
{
#ifdef _WIN32
    struct timeb t;
    ftime(&t);
    return (t.time % 60) + t.millitm / 1000.;
#else
    struct timeval t;
    gettimeofday(&t);
    return (t.tv_sec % 60) + t.tv_usec / 1000000.;
#endif
}

void 
draw(void)
{
    GLenum err;
    GLdouble secs;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_room();

    /* draw the transparent objects... */
    glEnable(GL_POLYGON_STIPPLE);
    glPolygonStipple((GLubyte *) conePattern);
    draw_cone();

    glPolygonStipple((GLubyte *) spherePattern);
    secs = get_secs();
    draw_sphere(secs * 360. / 10.);
    glDisable(GL_POLYGON_STIPPLE);

    err = glGetError();
    if (err != GL_NO_ERROR)
	printf("Error:  %s\n", gluErrorString(err));

    glutSwapBuffers();
}

/*ARGSUSED1*/
void 
key(unsigned char key, int x, int y)
{
    if (key == 27)
	exit(0);
}

int
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(0, 0);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(draw);
    glutIdleFunc(draw);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    init();

    glutMainLoop();
    return 0;
}
