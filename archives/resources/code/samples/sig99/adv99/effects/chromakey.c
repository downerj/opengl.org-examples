#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

static char defaultFile0[] = "../../data/ogllogo.bw";
static char defaultFile1[] = "../../data/mandrill.rgb";
static char defaultFile2[] = "../../data/brick.rgb";

GLuint *img0, *img1, *img2;
GLsizei w, h;
int w0, w1, w2, h0, h1, h2;
int comp;
GLfloat key[3] = {0, 0, 0};

#define RW 0.3086f
#define GW 0.6094f
#define BW 0.0820f

void 
init(void)
{
}


#define CHECK(a) \
    { \
	GLenum err; \
        err = glGetError(); \
        if (err != GL_NO_ERROR) printf(a ", error:  %s\n", gluErrorString(err)); \
    }

GLuint *
load_img(const char *fname, int *imgW, int *imgH)
{
    GLuint *img;

    img = (GLuint *)read_texture(fname, imgW, imgH, &comp);
    if (!img) {
	fprintf(stderr, "Could not open %s\n", fname);
	exit(1);
    }
    return img;
}

GLuint *
resize_img(GLuint * img, GLsizei curW, GLsizei curH)
{

    glPixelZoom((float) w / (float) curW, (float) h / (float) curH);
    glRasterPos2i(0, 0);
    glDrawPixels(curW, curH, GL_RGBA, GL_UNSIGNED_BYTE, img);
    free(img);
    img = (GLuint *) malloc(w * h * sizeof(GLuint));
    if (!img) {
	fprintf(stderr, "Malloc of %d bytes failed.\n",
		(int)(curW * curH * sizeof(GLuint)));
	exit(1);
    }
    glPixelZoom(1, 1);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, img);

    return img;
}

/*ARGSUSED1*/
void 
reshape(int winW, int winH)
{
    glViewport(0, 0, 2 * w, 2 * h);
    glLoadIdentity();
    glOrtho(0, 2 * w, 0, 2 * h, 0, 5);
}

void 
compute_matte(void)
{
    glClear(GL_ACCUM_BUFFER_BIT);

    /*
     * draw rectangle in (key color + 1) / 2 
     */
    glBegin(GL_QUADS);
    glColor3f(key[0], key[1], key[2]);
    glVertex2f(0, 0);
    glVertex2f(w, 0);
    glVertex2f(w, h);
    glVertex2f(0, h);
    glEnd();
    glFlush();
    CHECK("draw rectangle");

    /*
     * negate & accumulate  
     */
    glAccum(GL_LOAD, -1);
    CHECK("negate and accum");

    /*
     * compute & return (image - key) 
     */
    glRasterPos2f(0, 0);
    glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, img0);
    glAccum(GL_ACCUM, 1);
    glAccum(GL_RETURN, 1);
    CHECK("compute & return (image - key)");

    /*
     * move to right hand side of window 
     */
    glRasterPos2f(w, 0);
    glCopyPixels(0, 0, w, h, GL_COLOR);
    CHECK("move to right hand side of window");

    /*
     * compute & return (key - image) 
     */
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, w, h);
    glAccum(GL_MULT, -1);
    glAccum(GL_RETURN, 1);
    glScissor(0, 0, 2 * w, h);
    glDisable(GL_SCISSOR_TEST);
    CHECK("compute & return (key - image)")
    /*
     * assemble to get fabs(key - image) 
     */
	glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);
    glRasterPos2i(0, 0);
    glCopyPixels(w, 0, w, h, GL_COLOR);
    glDisable(GL_BLEND);
    CHECK("assemble to get fabs(key - image)");

    /*
     * assemble into alpha channel 
     */
    {
	GLfloat mat[] =
	{
	    RW, RW, RW, RW,
	    GW, GW, GW, GW,
	    BW, BW, BW, BW,
	    0, 0, 0, 0,
	};
	glMatrixMode(GL_COLOR);
	glLoadMatrixf(mat);

	glRasterPos2i(w, 0);
	glCopyPixels(0, 0, w, h, GL_COLOR);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
    }
    CHECK("assemble into alpha");

    /*
     * copy matte to right 
     */
    glRasterPos2i(0, 0);
    glCopyPixels(w, 0, w, h, GL_COLOR);
    CHECK("copy matte to right");

    /*
     * draw the third image 
     */
    glColorMask(1, 1, 1, 0);
    glRasterPos2i(w, 0);
    glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, img2);
    glColorMask(1, 1, 1, 1);

    glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
    glEnable(GL_BLEND);
    glRasterPos2i(w, 0);
    glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, img1);
    CHECK("draw third image");

    /*
     * this is for matte display... 
     */
    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(w, 0);
    glVertex2f(w, h);
    glVertex2f(0, h);
    glEnd();
    CHECK("matte display");

    glDisable(GL_BLEND);
}

void 
draw(void)
{
    static int first = 1;

    if (first) {
	printf("Scaling images to %d by %d\n", (int)w, (int)h);


	if (w0 != w || h0 != h) {
	    img0 = resize_img(img0, w0, h0);

	}
	if (w1 != w || h1 != h) {
	    img1 = resize_img(img1, w1, h1);
	}
	if (w2 != w || h2 != h) {
	    img2 = resize_img(img2, w2, h2);
	}
	first = 0;
    }
    glClear(GL_COLOR_BUFFER_BIT);
    compute_matte();

    glRasterPos2i(w / 2, h);
    glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, img0);

    CHECK("DrawPixels");
#if 0
    err = glGetError();
    if (err != GL_NO_ERROR)
	printf("Error:  %s\n", gluErrorString(err));
#endif
}

/*ARGSUSED*/
void 
button(int button, int state, int xpos, int ypos)
{
    if (state != GLUT_UP)
	return;

    ypos = 2 * h - ypos;
    glReadPixels(xpos, ypos, 1, 1, GL_RGB, GL_FLOAT, key);
    printf("Key is (%f %f %f)\n", key[0], key[1], key[2]);
    draw();
}

/*ARGSUSED1*/
void 
keyPress(unsigned char whichKey, int x, int y)
{
    if (whichKey == 27)
	exit(0);
}

void 
show_usage(void)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "chromakey mattefile file0 file1 [matteR matteG matteB]\n");
    fprintf(stderr, "chromakey mattefileAndfile0 file1 [matteR matteG matteB]\n");
}

int
main(int argc, char *argv[])
{
    char *fileName0 = defaultFile0,
        *fileName1 = defaultFile1,
        *fileName2 = defaultFile2;

    glutInit(&argc, argv);
    if (argc > 1) {
	fileName0 = fileName1 = argv[1];
    }
    if (argc > 2) {
	fileName2 = argv[2];
    }
    if (argc > 3) {
	fileName1 = fileName2;
	fileName2 = argv[3];
    }
    if (argc > 4) {
	if (argc == 6 || argc == 7) {
	    key[0] = atof(argv[argc - 3]);
	    key[1] = atof(argv[argc - 2]);
	    key[2] = atof(argv[argc - 1]);
	} else {
	    show_usage();
	    exit(1);
	}
    }
    printf("Matte file is %s\n", fileName0);
    printf("Image file 1 is %s\n", fileName1);
    printf("Image file 2 is %s\n", fileName2);
    printf("Key is (%f %f %f)\n", key[0], key[1], key[2]);
    img0 = load_img(fileName0, &w0, &h0);
    img1 = load_img(fileName1, &w1, &h1);
    img2 = load_img(fileName2, &w2, &h2);

#define MAX(a, b) ((a) > (b) ? (a) : (b))
    w = MAX(MAX(w0, w1), w2);
    h = MAX(MAX(h0, h1), h2);

    glutInitWindowSize(2 * w, 2 * h);
    glutInitWindowPosition(0, 0);
    glutInitDisplayMode(GLUT_RGBA | GLUT_ACCUM | GLUT_ALPHA);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(draw);
    glutKeyboardFunc(keyPress);
    glutReshapeFunc(reshape);
    glutMouseFunc(button);

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

    reshape(w, h);
    glutMainLoop();
    return 0;
}
