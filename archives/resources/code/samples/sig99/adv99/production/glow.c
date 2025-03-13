#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <math.h>
#ifdef _WIN32
#define sqrtf(x)    ((float)sqrt(x))
#define drand48()   ((float)rand()/(float)RAND_MAX)
#define sinf(x) ((float)sin((x)))
#define cosf(x) ((float)cos((x)))
#define floorf(x) ((float)(floor(x)))
#ifndef M_PI
#define M_PI 3.14159265
#endif
#endif

#define CHECK_ERROR(string)                                              \
{                                                                        \
    GLenum error_value;                                                  \
    while((error_value = glGetError()) != GL_NO_ERROR)                   \
	printf("Error Encountered: %s (%s)\n", string,                   \
	       gluErrorString(error_value));                             \
}

static int winWidth, winHeight;
static char *the_string = "glow";

#define SIZE	64
struct point { float x, y; } points[SIZE];

void
makepoints(void) {
    int i;
    for(i = 0; i < SIZE; i++) {
	points[i].x = drand48()*2.f-1.f;
	points[i].y = drand48()*2.f-1.f;
    }
}

/*
 * cone with base at (0,0,0) and top at (0,0,1)
 * one unit wide and high.
 */
void
makecone(void)
{
#define SIDES 10
    int i;
    glNewList(1, GL_COMPILE);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.f, 0.f, 1.f);
    for(i = 0; i <= SIDES; i++) {
	float s = sinf((2.f*M_PI*i)/SIDES)*.5f;
	float c = cosf((2.f*M_PI*i)/SIDES)*.5f;
	glVertex3f(s, c, 0.f);
    }
    glEnd();
    glEndList();
    CHECK_ERROR("makecone");
}


void
draw_string(const char* string) {
#if 0
    glRasterPos2f(0, 0);
    while (*string)
	glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *string++);

#endif
   glPushMatrix();
   glScalef(.005f, .005f, .005f);
#if 1
    while(*string)
	glutStrokeCharacter(GLUT_STROKE_ROMAN, *string++);
#endif
    glPopMatrix();
}

/* Called when window needs to be redrawn */
void redraw(void)
{
    int i;
    glClear(GL_COLOR_BUFFER_BIT); 
    glColor4f(.1f, .1f, 1.f, .007f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    for(i = 0; i < 2000; i++) {
#if 1
	float x = (drand48()+drand48()-1.f)/10.f;
	float y = (drand48()+drand48()-1.f)/10.f;
	glLineWidth(2.0f);
	glPushMatrix();
	glTranslatef(-.5f+x, 0.f+y, 0.f);
	draw_string(the_string);
	glPopMatrix();
#endif
#if 0
	glPushMatrix();
	glTranslatef(-.5f, 0.f, 0.f);
	glLineWidth((i+1.f)/100.f*30.f);
	draw_string(the_string);
	glPopMatrix();
#endif
    }
    glLineWidth(2.f);
    glPushMatrix();
    glTranslatef(-.5f, 0.f, 0.f);
    glColor4f(.1f, .1f, 1.f, 1.f);
    draw_string(the_string);
    glPopMatrix();

    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
    glFlush();
    glutSwapBuffers();
    CHECK_ERROR("redraw");
}

void
help(void) {
    printf("S    - increase shift scale\n");
    printf("s    - decrease shift scale\n");
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case '\033': exit(EXIT_SUCCESS); break;
    default: help(); return;
    }
    glutPostRedisplay();
}

void
menu(int which) {
    key((char)which, 0, 0);
}

void
create_menu(void) {
    glutCreateMenu(menu);
    glutAddMenuEntry("increase shift scale", 'S');
    glutAddMenuEntry("decrease shift scale", 's');
    glutAddMenuEntry("exit", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void
reshape(int wid, int ht)
{
    float a;
    winWidth = wid;
    winHeight = ht;
    glViewport(0, 0, wid, ht);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    a = ((float)winWidth)/winHeight;
    if (a > 1)
	glOrtho(-1.f*a, 1.f*a, -1.f, 1.f, -1.f, 1.f);
    else
	glOrtho(-1.f, 1.f, -1.f*a, 1.f*a, -1.f, 1.f);
    glMatrixMode(GL_MODELVIEW);
}

/* Parse arguments, and set up interface between OpenGL and window system */
int
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitWindowSize(512, 512);
    if (argc > 1)
	glutInitDisplayString("red=5");
    else
	glutInitDisplayMode(GLUT_RGBA);
    (void)glutCreateWindow("glow");
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key);
    create_menu();

    makepoints();
    makecone();
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);


    if (argc > 1)
    	the_string = argv[1];
    /*glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);*/

    glutDisplayFunc(redraw);
    CHECK_ERROR("main");
    glutMainLoop();
    return 0;
}
