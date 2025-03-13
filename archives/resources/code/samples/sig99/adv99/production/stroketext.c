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
static char *the_string = "some text";
static int bitmap = 0;

void
draw_string(const char* string) {
    if (bitmap) {
	glRasterPos2f(0, 0);
	while (*string)
	    glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *string++);
    } else {
       glPushMatrix();
       glScalef(.0006f, .0006f, .0006f);
	while(*string)
	    glutStrokeCharacter(GLUT_STROKE_ROMAN, *string++);
	glPopMatrix();
    }
}

/* Called when window needs to be redrawn */
void redraw(void)
{
    glClear(GL_COLOR_BUFFER_BIT); 

    glLineWidth(2.f);
    glPushMatrix();
    glTranslatef(-.5f, 0.f, 0.f);
    /*glColor4f(.1f, .1f, 1.f, 1.f);*/
    draw_string(the_string);
    glPopMatrix();

    glFlush();
    CHECK_ERROR("redraw");
}

void
help(void) {
    printf("S    - use bitmap text\n");
    printf("s    - use stroke text\n");
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case '\033': exit(EXIT_SUCCESS); break;
    case 'S': bitmap = 1; break;
    case 's': bitmap = 0; break;
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
    glutAddMenuEntry("bitmap", 'S');
    glutAddMenuEntry("stroke", 's');
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
    glutInitDisplayMode(GLUT_RGBA);
    (void)glutCreateWindow("stroketext");
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key);
    create_menu();

    if (argc > 1)
    	the_string = argv[1];
    /*glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);*/

    glutDisplayFunc(redraw);
    CHECK_ERROR("main");
    glutMainLoop();
    return 0;
}
