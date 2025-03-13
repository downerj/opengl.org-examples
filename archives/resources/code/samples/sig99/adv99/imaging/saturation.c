#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

GLuint *img;
int w, h;
int comp;

#define RW 0.3086
#define GW 0.6094
#define BW 0.0820

GLfloat saturate[4*4];
GLfloat s = 1.0;

int moving, bx;

void
update_matrix(void)
{
    GLfloat a, b, c, d, e, f, g, h, i;

    /* this routine is optimized to match to course notes, not to be 
    * efficient... */

    a = (1-s)*RW + s;
    b = (1-s)*RW;
    c = (1-s)*RW;
    d = (1-s)*GW;
    e = (1-s)*GW + s;
    f = (1-s)*GW;
    g = (1-s)*BW;
    h = (1-s)*BW;
    i = (1-s)*BW + s;

    saturate[0]  = a;
    saturate[1]  = b;
    saturate[2]  = c;
    saturate[3]  = 0;
    saturate[4]  = d;
    saturate[5]  = e;
    saturate[6]  = f;
    saturate[7]  = 0;
    saturate[8]  = g;
    saturate[9]  = h;
    saturate[10] = i;
    saturate[11] = 0;

    glMatrixMode(GL_COLOR);
    glLoadMatrixf(saturate);
    glMatrixMode(GL_MODELVIEW);
}

void
init(void)
{
    update_matrix();
}

void
load_img(const char *fname)
{
    img = (GLuint *)read_texture(fname, &w, &h, &comp);
    if (!img) {
	fprintf(stderr, "Could not open %s\n", fname);
	exit(1);
    }
}

void
reshape(int winW, int winH) 
{
    glViewport(0, 0, w, h);
    glLoadIdentity();
    glOrtho(0, winW, 0, winH, 0, 5);
}

void
draw(void)
{
    GLenum err;

    glClear(GL_COLOR_BUFFER_BIT);
    glRasterPos2i(0, 0);
    glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, img);
	glutSwapBuffers(); 

    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));
}


/*ARGSUSED1*/
void
motion(int x, int y)
{
    if (moving) {
	s += 0.1 * (x - bx) / (float)w;
	update_matrix();
	glutPostRedisplay();
    }
}


/*ARGSUSED3*/
void
mouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON) {
	moving = (state == GLUT_DOWN);
	bx = x;
    }
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y)
{
  if (key == 27) exit(0);
}

int
main(int argc, char *argv[])
{
    load_img("../../data/mandrill.rgb");
	
    glutInit(&argc, argv);
    glutInitWindowSize(w, h);
    glutInitWindowPosition(0, 0);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(draw);
    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutReshapeFunc(reshape);

    /* A hack to see if the color matrix is supported */
    while (glGetError() != GL_NO_ERROR);
    glMatrixMode(GL_COLOR);
    if (glGetError() != GL_NO_ERROR) {
	printf("This demo requires OpenGL 1.2 or the color matrix extension.\n");
	exit(0);
    }
    glMatrixMode(GL_MODELVIEW);
    
    init();

    glutMainLoop();
    return 0;
}

