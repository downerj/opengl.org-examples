#include <stdio.h>
#include <stdlib.h>
/* #include <unistd.h> */
#include <math.h>
#include <GL/glut.h>

#ifdef _WIN32
#define sinf(x) ((float)sin((x)))
#define cosf(x) ((float)cos((x)))
#define sqrtf(x) ((float)(sqrt(x)))
#define floorf(x) ((float)(floor(x)))
#ifndef M_PI
#define M_PI 3.14159265
#define M_SQRT2 1.4142135
#endif
#endif

#undef USE_ACCUM 

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if(error = glGetError())                                       \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

#define unitrand() (rand() / (double)RAND_MAX)

int windowWidth, windowHeight;

float oldProjection[16 * 20]; /* ugh */
int projStackTop = 0;

/* near, far name mangling because of x86 reserved words */
void pushOrthoView(float left, float right, float bottom, float top,
    float v_near, float v_far)
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glGetFloatv(GL_PROJECTION_MATRIX, oldProjection + projStackTop);
    projStackTop += 16;
    glLoadIdentity();
    glOrtho(left, right, bottom, top, v_near, v_far);
}

void popView(void)
{
    glMatrixMode(GL_PROJECTION);
    projStackTop -= 16;
    glLoadMatrixf(oldProjection + projStackTop);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void sendHexVerts(void)
{
    glVertex2f(0, 0.5);
    glVertex2f(.5, 0.25);
    glVertex2f(.5, -0.25);
    glVertex2f(0, -0.5);
    glVertex2f(-0.5, -0.25);
    glVertex2f(-0.5, 0.25);
}

void
drawHexagon(void)
{
    glBegin(GL_POLYGON);
    sendHexVerts();
    glEnd();
}

#define REGION_SIZE 40
#define JITTER_WIDTH 4
#define JITTER_HEIGHT 4

float jitter[JITTER_WIDTH][JITTER_HEIGHT][2];

void makeDistribJitter(void)
{
    int i, j;
    float x, y;
    double alpha, dist;

    for(i = 0; i < JITTER_WIDTH; i++)
	for(j = 0; j < JITTER_HEIGHT; j++) {
	    /* uniform circle coverage */
	    dist = pow(unitrand(), .5) * M_SQRT2;
	    alpha = unitrand() * M_PI * 2;
	    x = .5 + dist * .5 * cos(alpha);
	    y = .5 + dist * .5 * sin(alpha);
	    jitter[i][j][0] = x;
	    jitter[i][j][1] = y;
	}
}

void makeGridJitter(void)
{
    int i, j;
    float x, y;

    for(i = 0; i < JITTER_WIDTH; i++)
	for(j = 0; j < JITTER_HEIGHT; j++) {
	    x = 1.0 / JITTER_WIDTH * (i + .05 + unitrand() * .9);
	    y = 1.0 / JITTER_HEIGHT * (j + .05 + unitrand() * .9);
	    jitter[i][j][0] = x;
	    jitter[i][j][1] = y;
	    /* printf("jitter[%d][%d] = (%g, %g)\n", i, j, x, y); */
	}
}

void makeNoJitter(void)
{
    int i, j;

    for(i = 0; i < JITTER_WIDTH; i++)
	for(j = 0; j < JITTER_HEIGHT; j++) {
	    jitter[i][j][0] = .5;
	    jitter[i][j][1] = .5;
	    /* printf("jitter[%d][%d] = (%g, %g)\n", i, j, x, y); */
	}
}

void
display(void)
{
    int i, j;

    pushOrthoView(0, windowWidth, 0, windowHeight, -1, 1);

#ifndef USE_ACCUM
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glDisable(GL_DITHER);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
#endif
    for(i = 0; i < JITTER_WIDTH; i++)
	for(j = 0; j < JITTER_HEIGHT; j++) {
#ifdef USE_ACCUM
	    glClearColor(0, 0, 0, 0);
	    glClear(GL_COLOR_BUFFER_BIT);
#endif
	    glPushMatrix();
	    glColor4f(1, 1, 1, 1.0 / (JITTER_WIDTH * JITTER_HEIGHT));
	    glTranslatef(REGION_SIZE / 2.0 + jitter[i][j][0] + .5,
	        REGION_SIZE / 2.0 + jitter[i][j][1] + .5, 0);
	    glScalef(REGION_SIZE / 2.0, REGION_SIZE / 2.0, 1);
	    drawHexagon();
	    glPopMatrix();
#ifdef USE_ACCUM
	    if(i == 0 && j == 0)
		glAccum(GL_LOAD, 1.0 / (JITTER_WIDTH * JITTER_HEIGHT));
	    else
		glAccum(GL_ADD, 1.0 / (JITTER_WIDTH * JITTER_HEIGHT));
	    /* goto foo; */
#endif
	}
/* foo: */
#ifdef USE_ACCUM
    glAccum(GL_RETURN, 1.0);
#endif
    popView();

    glDisable(GL_BLEND);
    if(windowWidth < windowHeight)
        glPixelZoom(windowWidth / REGION_SIZE, windowWidth / REGION_SIZE);
    else
        glPixelZoom(windowHeight / REGION_SIZE, windowHeight / REGION_SIZE);
    if(windowWidth > windowHeight)
        pushOrthoView((windowHeight - windowWidth) / 2, windowHeight +
            (windowWidth - windowHeight) / 2, 0, windowHeight, -1, 1);
    else 
        pushOrthoView(0, windowWidth, (windowWidth - windowHeight) / 2,
            windowWidth + (windowHeight - windowWidth) / 2, -1, 1);
    glCopyPixels(0, 0, REGION_SIZE, REGION_SIZE, GL_COLOR);
    glRasterPos2f(0.01f, 0.01f);
    popView();
    glutSwapBuffers();
}

void help(void)
{
    fprintf(stderr, "ESC        - quit\n");
    fprintf(stderr, "a          - use one sample at center of pixel\n");
    fprintf(stderr, "b          - use samples within one pixel\n");
    fprintf(stderr, "c          - use samples within 1.414 pixel diameter circle\n");
}

/*ARGSUSED1*/
void key(unsigned char k, int x, int y)
{
     switch(k) {
     case 27: case 'q':
	 exit(0);

     case 'c':
	 makeDistribJitter();
	 glutPostRedisplay();
	 break;

     case 'b':
	 makeGridJitter();
	 glutPostRedisplay();
	 break;

     case 'a':
	 makeNoJitter();
	 glutPostRedisplay();
	 break;

     default:
	 help();
     }
}

/* used to get current width and height of viewport */
void
reshape(int wid, int ht)
{
  glViewport(0, 0, wid, ht);
  windowWidth = wid;
  windowHeight = ht;
  glutPostRedisplay();
}

int
main(int argc, char **argv)
{
    makeNoJitter();
#ifdef _WIN32
    glutInitWindowSize(windowWidth = 256, windowHeight = 256);
#else
    glutInitWindowSize(windowWidth = 512, windowHeight = 512);
#endif
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_ACCUM|GLUT_DEPTH|GLUT_ALPHA);
    glutCreateWindow("Object recognition");
    glutKeyboardFunc(key);
    /* glutSpecialFunc(special); */
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(display);
    /* glutMouseFunc(mouse); */
    /* glutMotionFunc(motion); */
    pushOrthoView(0, windowWidth, 0, windowHeight, 0, 1);
    help();
    glutMainLoop();
    return 0;
}
