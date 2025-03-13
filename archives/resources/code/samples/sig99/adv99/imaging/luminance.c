#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

static char defaultFile[] = "../../data/mandrill.rgb";
GLuint *img;
int w, h;
int comp;

#define RW 0.3086f
#define GW 0.6094f
#define BW 0.0820f

GLfloat lum[] = {
    RW, RW, RW, 0,
    GW, GW, GW, 0,
    BW, BW, BW, 0,
    0, 0, 0, 0,
};

void 
init(void)
{
    glMatrixMode(GL_COLOR);
    glLoadMatrixf(lum);
    glMatrixMode(GL_MODELVIEW);
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

    err = glGetError();
    if (err != GL_NO_ERROR)
	printf("Error:  %s\n", gluErrorString(err));
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
    if (argc > 1) {
	load_img(argv[1]);
    } else {
	load_img(defaultFile);
    }
    glutInitWindowSize(w, h);
    glutInitWindowPosition(0, 0);
    glutInitDisplayMode(GLUT_RGB);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(draw);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);

    /*
     * A hack to see if the color matrix is supported 
     */
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
