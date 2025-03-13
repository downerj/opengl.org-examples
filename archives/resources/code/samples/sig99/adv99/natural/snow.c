/*
 *
 * In this exercise, we'll add the reflection of the scene to a wood floor.
 * The code creates the scene without texturing.
 *
 */

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <GL/glut.h>
#include "../util/texture.h"

#ifdef _WIN32
/* Win32 math.h doesn't define float versions of the trig functions. */
#define sinf sin
#define cosf cos
#define atan2f atan2

/* nor does it define M_PI. */
#ifndef M_PI
#define M_PI 3.14159265
#endif

#define drand48() ((float)rand()/RAND_MAX)
#endif

GLboolean snow = GL_FALSE; /* show the snow */
GLboolean fog = GL_FALSE; /* fog scene */
GLboolean texture = GL_FALSE; /* alpha texture scene */
GLboolean overcast = GL_FALSE; /* overcast sky */
GLboolean snowaa = GL_FALSE; /* snow antialiasing */
GLboolean blend = GL_FALSE; /* snow blending */
GLfloat snowsize = 1.f;

/* Particle System Code */

typedef struct {
    GLfloat pos[3];
    GLfloat Vx, Vy, Vz;
} Particle;

/* particle system state */
typedef struct {
    Particle *array; /* array of particles */
    int total; /* total number of array elements */
    int update; /* number of updates per frame */
    GLfloat Rx, Ry, Rz; /* reset vector */
    GLfloat Vx, Vy, Vz; /* global velocity of particles */
} PSystem;



void (*updateptr)(PSystem *sys);

#define PART_COUNT 1024
PSystem psys; /* particle system */

enum {R, G, B, A};
enum {X, Y, Z, W};

/* initialize the particle data */


/* begin, end xyz arrays; start and end points
   nframes number of frames to go from start to end
   total number of  particles
*/
void
initpart(PSystem *sys, GLfloat *start, GLfloat *end, int nframes, int nparts)
{
    int i;
    Particle *part;

    sys->Rx = start[X] - end[X];
    sys->Ry = start[Y] - end[Y];
    sys->Rz = start[Z] - end[Z];

    sys->Vx = -sys->Rx/nframes;
    sys->Vy = -sys->Ry/nframes;
    sys->Vz = -sys->Rz/nframes;

    if(nparts < nframes)
	printf("should have more particles than frames\n");

    sys->update = nparts/nframes; /* this should have no remainder*/

    sys->total = nparts;
    sys->array = (Particle *)malloc(nparts * sizeof(Particle));

    for(part = sys->array, i = 0; i < nparts; i++)
    {
	/* XXX this part should be more general */
	part->pos[X] = -50 + drand48() * 100.f;
	part->pos[Z] = drand48() * 90.f;

	/* jitter the starting points in y by fall distance per frame (Vy) */
	part->pos[Y] = start[Y]; /* starting height */

	part->Vx = 0.f;
	part->Vy = 0.f; /* updatepart0 starts them moving */
	part->Vz = 0.f;
	part++;
    }
}



/* update the particle data */
void
updatepart(PSystem *sys)
{
    int i;
    static int offset = 0;
    GLfloat displace;
    int count;
    Particle *part;

    /* reset positions of a subset of particles */
    count = sys->update;
    displace = sys->Ry;
    part = &sys->array[offset];
    for(i = 0; i < count; i++)
    {
	part->pos[Y] += displace;
	part++;
    }

    /* fast if sys->total a power of 2 */
    offset = (offset + count) % sys->total;
}

/* update the particle data first time */
void
updatepart0(PSystem *sys)
{
    int i;
    Particle *part;
    static int offset = 0;


    /* reset positions of a subset of particles */
    part = &sys->array[offset];
    for(i = 0; i < sys->update; i++)
    {
	part->Vy = sys->Vy;
	part++;
    }
    offset += sys->update;

    if(offset >= sys->total)
    {
	updateptr = updatepart;
	offset = 0;
    }
}


/* display particles and fast update */
void
displaypart(PSystem *sys)
{
    int i, total;
    Particle *part;

    glDisable(GL_LIGHTING);
    glColor3f(1.f, 1.f, 1.f);
    glBegin(GL_POINTS);
    part = sys->array;
    total = sys->total;
    for(i = 0; i < total; i++)
    {
	glVertex3fv(part->pos);
	part->pos[X] += part->Vx;
	part->pos[Y] += part->Vy;
	part->pos[Z] += part->Vz;
	part++;
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if((error = glGetError()) != GL_NO_ERROR)                      \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

#define RAD(x) (((x)*M_PI)/180.)

enum {NODLIST, HOUSE, BLDG, LIGHT};
enum {OVERCAST, FOG, SNOW, BIGGER, SMALLER, RESETSIZE, ANTIALIAS, BLEND, EXIT};


GLfloat viewangle[] = {0.f, 0.f};
GLfloat viewdist = 400.f;
GLfloat viewpos[]   = {0.f, 100.f, 400.f}; /* position of viewer */

GLfloat lightpos[]  = {50.f, 100.f, 100.f, 0.f};

enum {MOVE_ANGLE, MOVE_DIST, FOG_START, FOG_END};
int active;
int dblbuf = GL_TRUE;

int winwid = 512;
int winht = 512;

void
updateMV(void)
{
	/* assume GL_MODELVIEW matrix is current */
	glLoadIdentity();
	gluLookAt(0., -80., 100.,
		  0.,  -100., -100., 
		  0., 1., 0.);
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
}

void
idle(void)
{
    glutPostRedisplay();
}

/*ARGSUSED*/
void
motion(int x, int y)
{
    switch(active)
    {
    case MOVE_ANGLE:
	glutPostRedisplay();
	break;
    case MOVE_DIST:
	glutPostRedisplay();
	break;
    case FOG_START:
	glFogf(GL_FOG_START,(winht - y) * 40.f/winht);
	printf("fog start = %.2f\n", (winht - y) * 100.f/winht);
	glutPostRedisplay();
	break;
    case FOG_END:
	glFogf(GL_FOG_END,(winht - y) * 40.f/winht);
	printf("fog end = %.2f\n", (winht - y) * 100.f/winht);
	glutPostRedisplay();
	break;
    }
}

/*ARGSUSED2*/
void
mouse(int button, int state, int x, int y)
{
    /* hack for 2 button mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;

    if(state == GLUT_DOWN)
	switch(button)
	{
	case GLUT_LEFT_BUTTON: /* change fog */
	    break;
	case GLUT_MIDDLE_BUTTON:
	    break;
	}
}

void
reshape(int wid, int ht)
{
    winwid = wid;
    winht = ht;
    glViewport(0, 0, wid, ht);
}



/* Called when window needs to be redrawn */
void
draw(void)
{
    int i, j;
    int xoffset;

    /* ground */
    glBegin(GL_QUADS);
    glColor3f(1.f, 1.f, 1.f);
    glNormal3f(0.f, 1.f, 0.f);

    glVertex3f(-200.f, -100.f,  100.f);
    glVertex3f(-10.f, -100.f,  100.f);
    glVertex3f(-10.f, -100.f, -200.f);
    glVertex3f(-200.f, -100.f, -200.f);

    glVertex3f(  10.f, -100.f,  100.f);
    glVertex3f( 200.f, -100.f,  100.f);
    glVertex3f( 200.f, -100.f, -200.f);
    glVertex3f(  10.f, -100.f, -200.f);
    glEnd();

    /* road */
    glBegin(GL_QUADS);
    glColor3f(0.1f, 0.1f, 0.1f);
    glNormal3f(0.f, 1.f, 0.f);
    glTexCoord2i(0, 0);
    glVertex3f(-10.f, -100.f,  100.f);
    glTexCoord2i(2, 0);
    glVertex3f( 10.f, -100.f,  100.f);
    glTexCoord2i(2, 2);
    glVertex3f( 10.f, -100.f, -200.f);
    glTexCoord2i(0, 2);
    glVertex3f(-10.f, -100.f, -200.f);
    glEnd();

    /* overcast sky */
    if(overcast)
    {
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glBegin(GL_QUADS);
	glColor3f(.8f, .8f, .8f);
	glTexCoord2i(1, 0);
	glVertex3f( 300.f, -50.f,  100.f);
	glTexCoord2i(0, 0);
	glVertex3f(-300.f, -50.f,  100.f);
	glTexCoord2i(0, 1);
	glColor3f(.4f, .4f, .4f);
	glVertex3f(-300.f, -50.f, -300.f);
	glTexCoord2i(1, 1);
	glVertex3f( 200.f, -50.f, -300.f);
	glEnd();

	glEnable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
    }

    for(xoffset = -25, j = 0; j < 2; j++)
    {
	for(i = 0; i < 7; i++)
	{
	    glPushMatrix();
	    glTranslatef((GLfloat)xoffset, -100.f, 100.f - 40.f * i);
	    glCallList(HOUSE);
	    glPopMatrix();
	}
	xoffset += 40;
    }

    if(snow)
    {
	if(blend)
	    glEnable(GL_BLEND);
	displaypart(&psys);
	if(blend)
	    glDisable(GL_BLEND);
	(*updateptr)(&psys);
    }
    CHECK_ERROR("draw()");
}



/* Called when window needs to be redrawn */
void
redraw(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    draw(); /* draw the unreflected scene */

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("redraw()");
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 's':
    case 'S': /* toggle snow machine */
	snow = !snow;
	glutPostRedisplay();
	break;
    case 'f':
    case 'F': /* toggle fog */
	fog = !fog;
	if(fog)
	{
	    glClearColor(.4f, .4f, .4f, 1.f);
	    glEnable(GL_FOG);
	}
	else
	{
	    glClearColor(0.f, 0.f, 1.f, 1.f);
	    glDisable(GL_FOG);
	}

	glutPostRedisplay();
	break;
    case 'o':
    case 'O': /* toggle overcast */
	overcast = !overcast;
	glutPostRedisplay();
	break;
    case '+': /* bigger snow */
	snowsize += .5f;
	glPointSize(snowsize);
	glutPostRedisplay();
	break;
    case '-': /* smaller snow */
	snowsize -= .5f;
	if(snowsize <= 0.f)
	    snowsize = .5f;
	glPointSize(snowsize);
	glutPostRedisplay();
	break;
    case 'r': 
    case 'R': /* reset snow size to 1 */
	snowsize = 1.f;
	glPointSize(snowsize);
	glutPostRedisplay();
	break;
    case 'b': 
    case 'B': /* toggle snow blending */
	blend = !blend;
	glutPostRedisplay();
	break;
    case 'a': 
    case 'A': /* toggle point antialiasing */
	snowaa = !snowaa;
	if(snowaa)
	    glEnable(GL_POINT_SMOOTH);
	else
	    glDisable(GL_POINT_SMOOTH);
	glutPostRedisplay();
	break;
    case '\033':
	exit(0);
    default:
	fprintf(stderr, 
		"unreflective floor (n, N)\n"
		"show floor reflection (r, R)\n"
		"shiny (partially reflective) floo (s, S)\n"
		"exit (escape key)\n\n");
	break;
    }
}

void
menu(int choice)
{
    switch(choice)
    {
    case OVERCAST:
	key('o', 0, 0);
	break;
    case FOG:
	key('f', 0, 0);
	break;
    case SNOW:
	key('s', 0, 0);
	break;
    case BIGGER:
	key('+', 0, 0);
	break;
    case SMALLER:
	key('-', 0, 0);
	break;
    case RESETSIZE:
	key('r', 0, 0);
	break;
    case ANTIALIAS:
	key('a', 0, 0);
	break;
    case BLEND:
	key('b', 0, 0);
	break;
    case EXIT:
	exit(0);
    }
}

GLfloat one[] = {1.f, 1.f, 1.f, 1.f};


int
main(int argc, char *argv[])
{
    unsigned *cloud;
    int texcomps, texwid, texht;

    GLUquadricObj *sphere;

    /* start and end of particles */
    static GLfloat begin[] = {0.f, -25.f, 0.f};
    static GLfloat   end[] = {0.f,-100.f, 0.f};
    static GLfloat fogcolor[] = {.4f, .4f, .4f, 1.f};

    glutInitWindowSize(winwid, winht);
    glutInit(&argc, argv);
    if(argc > 1)
    {
	char *args = argv[1];
	int done = GL_FALSE;
	while(!done)
	{
	    switch(*args)
	    {
	    case 's': /* single buffer */
		printf("Single Buffered\n");
		dblbuf = GL_FALSE;
		break;
	    case '-': /* do nothing */
		break;
	    case 0:
		done = GL_TRUE;
		break;
	    }
	    args++;
	}
    }
    if(dblbuf)
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL|GLUT_DOUBLE);
    else
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL);

    (void)glutCreateWindow("snow demo");

    glutDisplayFunc(redraw);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);
    glutIdleFunc(idle);

    glutCreateMenu(menu);
    glutAddMenuEntry("Toggle Overcast (o, O)", OVERCAST);
    glutAddMenuEntry("Toggle Fog (f, F)", FOG);
    glutAddMenuEntry("Toggle Snow (s, S)", SNOW);
    glutAddMenuEntry("Bigger Flakes (+)", BIGGER);
    glutAddMenuEntry("Smaller Flakes (-)", SMALLER);
    glutAddMenuEntry("Reset Flake Size to One (r, R)", RESETSIZE);
    glutAddMenuEntry("Toggle Point Antialiasing (a, A)", ANTIALIAS);
    glutAddMenuEntry("Snow Blending (b, B)", BLEND);
    glutAddMenuEntry("Exit Program", EXIT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    printf("OpenGL Version %s\n", glGetString(GL_VERSION));

    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-5., 5., -5., 5., 10., 1000.); 
    glMatrixMode(GL_MODELVIEW);
    updateMV();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* turn on features */
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* place light 0 in the right place */
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    glClearColor(0.f, 0.f, 1.f, 1.f);
    glFogfv(GL_FOG_COLOR, fogcolor);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, -200.f);
    glFogf(GL_FOG_END, 200.f);
    glHint(GL_FOG_HINT, GL_NICEST);


    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    /* makes texturing faster, and looks better than GL_LINEAR */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glNewList(LIGHT, GL_COMPILE);
    glDisable(GL_LIGHTING);
    sphere = gluNewQuadric();
    glColor3f(.7f, .2f, .7f);
    gluSphere(sphere, 5.f, 10, 10);
    gluDeleteQuadric(sphere);
    glEnable(GL_LIGHTING);
    glEndList();

    

    /* 10 X 20; vary size with transforms */
    /* one corner of house on origin; bottom on xz plane */
    glNewList(HOUSE, GL_COMPILE);
    glBegin(GL_QUADS);
    /* walls of house */
    glColor3f(1.f, 1.f, 0.f);
    /* front */
    glNormal3f( 0.f, 0.f, 1.f);
    glVertex3i( 0, 0, 0);
    glVertex3i(10, 0, 0);
    glVertex3i(10,10, 0);
    glVertex3i( 0,10, 0);
    /* back */
    glNormal3f( 0.f, 0.f, -1.f);
    glVertex3i( 0, 0, -20);
    glVertex3i( 0,10, -20);
    glVertex3i(10,10, -20);
    glVertex3i(10, 0, -20);
    /* left */
    glNormal3f(-1,  0.f,   0.f);
    glVertex3i( 0,  0,   0);
    glVertex3i( 0, 10,   0);
    glVertex3i( 0, 10, -20);
    glVertex3i( 0,  0, -20);
    /* right */
    glNormal3f( 1.f,  0.f,   0.f);
    glVertex3i(10,  0,   0);
    glVertex3i(10,  0, -20);
    glVertex3i(10, 10, -20);
    glVertex3i(10, 10,   0);
    /* roof of house */
    glColor3f(.8f, .1f, .1f);
    /* left top */
    glNormal3f(-.707f, .707f, 0.f);
    glVertex3i( 0, 10,   0);
    glVertex3i( 5, 15,   0);
    glVertex3i( 5, 15, -20);
    glVertex3i( 0, 10, -20);
    /* right top */
    glNormal3f( .707f, .707f, 0.f);
    glVertex3i(10, 10,   0);
    glVertex3i(10, 10, -20);
    glVertex3i( 5, 15, -20);
    glVertex3i( 5, 15,   0);
    glEnd();

    glBegin(GL_TRIANGLES);
    /* front */
    glNormal3f( 0.f,  0.f, 1.f);
    glVertex3i( 0, 10, 0);
    glVertex3i(10, 10, 0);
    glVertex3i( 5, 15, 0);
    /* back */
    glNormal3f( 0.f,  0.f,  -1.f);
    glVertex3i( 0, 10, -20);
    glVertex3i( 5, 15, -20);
    glVertex3i(10, 10, -20);
    glEnd();
    glEndList();

    glEnable(GL_CULL_FACE);
    /* load pattern for current 2d texture */

    cloud = read_texture("../../data/clouds.bw",
			    &texwid, &texht, &texcomps);

    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE, texwid, texht, GL_RGBA,
		      GL_UNSIGNED_BYTE, cloud);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);

    free(cloud);
    initpart(&psys, begin, end, 200, 6000);
    updateptr = updatepart0;

    CHECK_ERROR("main()");
    glutMainLoop();

    return 0;
}
