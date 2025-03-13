#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <math.h>
#include "texture.h"
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

static int image_w, image_h;
static void* img;

#define D_SIZE	3
static GLubyte D[9] = { 
    (GLubyte)6*32, (GLubyte)8*32, (GLubyte)4*32,
    (GLubyte)1*32, (GLubyte)0*32, (GLubyte)3*32,
    (GLubyte)5*32, (GLubyte)2*32, (GLubyte)7*32,
};

static void
pixel_map(int on) {
    GLushort table[256];
    if (on) {
	int i;
	table[0] = 0;
	for (i=1; i<32; i++)
	    table[i] = 65535;
	glPixelMapusv(GL_PIXEL_MAP_R_TO_R, 32, table);
	glPixelMapusv(GL_PIXEL_MAP_G_TO_G, 32, table);
	glPixelMapusv(GL_PIXEL_MAP_B_TO_B, 32, table);    
	glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
    } else {
	glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
	glPixelMapusv(GL_PIXEL_MAP_R_TO_R, 1, table);
	glPixelMapusv(GL_PIXEL_MAP_G_TO_G, 1, table);
	glPixelMapusv(GL_PIXEL_MAP_B_TO_B, 1, table);    
    }
}

static void
draw_dither(void)
{
    int x, y;
    for(y = 0; y < image_h*3; y += D_SIZE) {
    	for(x = 0; x < image_w*3; x += D_SIZE) {
	    glRasterPos2i(x, y);
	    glDrawPixels(D_SIZE, D_SIZE, GL_LUMINANCE, GL_UNSIGNED_BYTE, (GLubyte *)D);

	}
    }
    glAccum(GL_LOAD, 1.0f);
}

/* Called when window needs to be redrawn */
void redraw(void)
{
    glClear(GL_COLOR_BUFFER_BIT); 

    draw_dither();
    glRasterPos2i(0, 0);
    glPixelZoom(3.f, 3.f);
    glDrawPixels(image_w, image_h, GL_RGBA, GL_UNSIGNED_BYTE, (GLubyte *)img);
    glPixelZoom(1.f, 1.f);
    glAccum(GL_ACCUM, -1.0);
    glAccum(GL_RETURN, -1.f);
    glRasterPos2i(0, 0);
    glDrawBuffer(GL_FRONT); glReadBuffer(GL_FRONT);
    pixel_map(1);
    glCopyPixels(0, 0, 3*image_w, 3*image_h, GL_COLOR);
    pixel_map(0);

    CHECK_ERROR("redraw");
}

void
help(void) {
/*    printf("S    - increase shift scale\n");
    printf("s    - decrease shift scale\n");	*/
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
    glViewport(0, 0, wid, ht);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.f, wid, 0.f, ht, -1.f, 1.f);
    glMatrixMode(GL_MODELVIEW);
}

/* Parse arguments, and set up interface between OpenGL and window system */
int
main(int argc, char *argv[])
{
    int comp;
    glutInit(&argc, argv);
    glutInitWindowSize(512, 512);
    glutInitDisplayMode(GLUT_RGBA|GLUT_ACCUM);
    (void)glutCreateWindow("dither");
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key);
/*    create_menu();	*/

    img = read_texture((argc > 1) ? argv[1] : "../../data/mandrill_small.rgb", &image_w, &image_h, &comp);
    if (!img) {
	fprintf(stderr, "Could not open image.\n");
	exit(1);
    }

    glutDisplayFunc(redraw);
    CHECK_ERROR("main");
    glutMainLoop();
    return 0;
}
