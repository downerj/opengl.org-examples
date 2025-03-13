/*
 * In this exercise, you'll add fog and caustic effects to make the
 * teapot appear to be underwater. The caustic texture has been
 * loaded for you; use texgen to caustic lighting patterns to move
 * back and forth.
 */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

#ifdef WIN32
#define expf(x) ((float)exp((x)))
#define sinf(x) ((float)sin((x)))
#define M_PI 3.14159265
#endif

enum { PAN = 1, SPIN } mouse_mode;

static GLfloat pan_y = 0;
static GLfloat spin_x = 0;
static int old_x, old_y;

static GLfloat s_plane[4] = { 1.f, 0.f, 0.f, 0.f };
static GLfloat t_plane[4] = { 0.f, 0.f, 1.f, 0.f };
static GLfloat fog_color[4] = { 0.1f, 0.2f, 0.2f, 0.1f };
static GLfloat fog_density = 0.015f;


void init(void) {
    GLubyte *image;
    int width, height, depth;
    static GLfloat pos[] = { 0.0, 1.0, 0.0, 0.0 };

    image = (GLubyte*)read_texture("../../data/sea.rgb",
				   &width, &height, &depth);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, image);

    /*
     * enable texgen, using eye linear. define s to vary with X, and
     * t to vary with Z (so the texture is mapped horizontally.
     * be sure to enable texgen and texturing
     */
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glMaterialf(GL_FRONT, GL_SHININESS, 64.0);

    /*
     * use exponential fog, use the fog_density value for fog density,
     * fog_color for the fog colore, and don't forget to enable fogging.
     */
    glEnable(GL_FOG);
    glFogfv(GL_FOG_COLOR, fog_color);
    glFogf(GL_FOG_DENSITY, fog_density);

    /*
     * set the clear color to match the fog color, so the background
     * looks foggy.
     */
    glClearColor(fog_color[0], fog_color[1], fog_color[2], fog_color[3]);

    /* glutTeapot() is wound clockwise */
    glFrontFace(GL_CW);

    free(image);
}

void
reshape(int width, int height) 
{
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40, 1, 10, 1000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -100);
}

void
display(void) 
{
    static GLfloat s, phase = 0;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* translate the caustic texture using the texture matrix */
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glTranslatef(0.5f, -0.5f, 0.f);
    glScalef(0.1f, 0.1f, 1.f);
    phase += (float)M_PI/25.f;
    if (phase > (float)(2.f*M_PI)) phase -= (float)(2.f*M_PI);
    s = sinf(phase);
    glTranslatef(s, s, 0.f);
    glMatrixMode(GL_MODELVIEW);

    glPushMatrix();
    glTranslatef(0.f, 0.f, pan_y);
    glRotatef(spin_x, 0.0f, 1.0f, 0.0f);
    glutSolidTeapot(20.0f); 
    glPopMatrix();

    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glutSwapBuffers();
}


void help(void) {
    printf("underwater help\n");
    printf("h            -  help\n");
    printf("left mouse   -  zoom (up and down)\n");
    printf("right mouse  -  rotate (left and right)\n");
}

/*ARGSUSED1*/
void
keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case '\033': 
	exit(EXIT_SUCCESS); 
	break;
    default:
	help();
	break;
    }
}

void
motion(int x, int y) {
    if (mouse_mode == PAN) {
	pan_y = y - old_y;
    } else if (mouse_mode == SPIN) {
	spin_x = x - old_x;
    }

    glutPostRedisplay();
}

void
mouse(int button, int state, int x, int y) {

    /* hack for 2 button mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;

    if(state == GLUT_DOWN) {
	old_x = x; old_y = y;
	switch(button) {
	case GLUT_LEFT_BUTTON:
	    mouse_mode = PAN;
	    break;
	case GLUT_MIDDLE_BUTTON:
	    break;
	case GLUT_RIGHT_BUTTON:
	    mouse_mode = SPIN;
	    break;
	}
    } else if (state == GLUT_UP) {
	button = 0;
    }
}

void
idle(void)
{
    glutPostRedisplay();
}

int
main(int argc, char** argv) {
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    init();
    glutMainLoop();
    return 0;
}
