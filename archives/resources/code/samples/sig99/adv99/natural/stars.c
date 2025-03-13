#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glut.h>
#include "texture.h"

#ifdef _WIN32
#define drand48() ((float)rand()/RAND_MAX)
#endif

static int pstyle;
static int mesh = 1;
static float ttrans[2];
static float transx, transy, rotx, roty;
static float amplitude = 0.03f;
static float freq = 5.0f;
static float phase = .00003f;
static int ox = -1, oy = -1;
static int show_t = 1;
static int mot;
#define PAN	1
#define ROT	2

void
pan(int x, int y) {
    transx +=  (x-ox)/500.;
    transy -= (y-oy)/500.;
    ox = x; oy = y;
    glutPostRedisplay();
}

void
rotate(int x, int y) {
    rotx += x-ox;
    if (rotx > 360.) rotx -= 360.;
    else if (rotx < -360.) rotx += 360.;
    roty += y-oy;
    if (roty > 360.) roty -= 360.;
    else if (roty < -360.) roty += 360.;
    ox = x; oy = y;
    glutPostRedisplay();
}

void
motion(int x, int y) {
    if (mot == PAN) pan(x, y);
    else if (mot == ROT) rotate(x,y);
}

void
mouse(int button, int state, int x, int y) {
    if(state == GLUT_DOWN) {
	switch(button) {
	case GLUT_LEFT_BUTTON:
	    mot = PAN;
	    motion(ox = x, oy = y);
	    break;
	case GLUT_MIDDLE_BUTTON:
	    mot = ROT;
	    motion(ox = x, oy = y);
	    break;
	case GLUT_RIGHT_BUTTON:
	    break;
	}
    } else if (state == GLUT_UP) {
	mot = 0;
    }
}

void toggle_t(void) {
    show_t ^= 1;
}

void bfunc(void) {
    static int state;
    if (state ^= 1) {
	glEnable(GL_BLEND);
    } else {
	glDisable(GL_BLEND);
    }
}

void pfunc(void) { pstyle = (pstyle+1) % 4; }

void ffunc(void) { freq *= 2.f; }
void Ffunc(void) { freq /= 2.f; }
void mfunc(void) { mesh ^= 1; }

void wire(void) {
    static int w;
    if (w ^= 1) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_BLEND);
    } else {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_BLEND);
    }
}

void light(void) {
    static int l;
    if (l ^= 1)
	glEnable(GL_LIGHTING);
    else
	glDisable(GL_LIGHTING);
}

void up(void) { amplitude += .01f; }
void down(void) { amplitude -= .01f; }
void left(void) { phase -= .00001f; }
void right(void) { phase += .00001f; }

void
animate(void) {
    ttrans[0] += .005f;
    if (ttrans[0] == 1.0f) ttrans[0] = 0.0f;
    ttrans[1] -= .0025f;
    if (ttrans[1] <= 0.0f) ttrans[1] = 1.0f;
    glutPostRedisplay();
}

void xfunc(void) {
    static int state = 1;
    glutIdleFunc((state ^= 1) ? animate : NULL);
}

void help(void) {
    printf("Usage: lightp [image]\n");
    printf("'h'            - help\n");
    printf("'l'            - toggle lighting\n");
    printf("'f'            - increase frequency\n");
    printf("'F'            - decrease frequency\n");
    printf("'m'            - toggle mesh\n");
    printf("'t'            - toggle wireframe\n");
    printf("'x'            - toggle water motion\n");
    printf("'UP'           - increase amplitude\n");
    printf("'DOWN'         - decrease amplitude\n");
    printf("'RIGHT'        - increase phase change\n");
    printf("'LEFT'         - decreae phase change\n");
    printf("left mouse     - pan\n");
    printf("middle mouse   - rotate\n");
}

void init(char *filename) {
    unsigned *image;
    int width, height, components;
    if (filename) {
	image = read_texture(filename, &width, &height, &components);
	if (image == NULL) {
	    fprintf(stderr, "Error: Can't load image file \"%s\".\n",
		    filename);
	    exit(EXIT_FAILURE);
	} else {
	    printf("%d x %d image loaded\n", width, height);
	}
	if (components != 1 && components != 2) {
	    printf("must be a l or la image\n");
	    exit(EXIT_FAILURE);
	}
	if (components == 1) {
	    /* hack for RE */
	    int i;
	    GLubyte *p = (GLubyte *)image;
	    for(i = 0; i < width*height; i++) {
		p[i*4+3] = p[i*4+0];
	    }
	    components = 2;
	}
    } else {
	int i, j;
	unsigned char *img;
	components = 4; width = height = 512;
	image = (unsigned *) malloc(width*height*sizeof(unsigned));
	img = (unsigned char *)image;
	for (j = 0; j < height; j++)
	    for (i = 0; i < width; i++) {
		int w2 = width/2, h2 = height/2;
		if (i & 32)
		    img[4*(i+j*width)+0] = 0xff;
		else
		    img[4*(i+j*width)+1] = 0xff;
		if (j&32)
		    img[4*(i+j*width)+2] = 0xff;
		if ((i-w2)*(i-w2) + (j-h2)*(j-h2) > 64*64 &&
		    (i-w2)*(i-w2) + (j-h2)*(j-h2) < 300*300) img[4*(i+j*width)+3] = 0xff;
	    }

    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, components, width,
                 height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 image);
    /*glEnable(GL_TEXTURE_2D);*/
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /*gluPerspective(50.,1.,.1,far_cull = 10.);*/
    glOrtho(-1.,1.,-1.,1.,-1.,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glLineWidth(2.0f);
    glEnable(GL_LINE_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(10.f);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
}

void xform(float *v0, float *mat, float *v1);

void quad(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(90.f, 1.f, 0.f, 0.f);
    glScalef(.03f, .03f, .03f);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-1.f, 0.f, -1.f);
    glTexCoord2f(0, 1); glVertex3f(-1.f, 0.f,  1.f);
    glTexCoord2f(1, 1); glVertex3f( 1.f, 0.f,  1.f);
    glTexCoord2f(1, 0); glVertex3f( 1.f, 0.f, -1.f);
    glEnd();
    glPopMatrix();
}

void draw_light(float *v, float size) {
#if 0
    glGetFloatv(GL_MODELVIEW, matv);
    glGetFloatv(GL_PROJECTION, matp);
#endif
    pstyle = 2;

    glEnable(GL_BLEND);
    glEnable(GL_POINT_SMOOTH);
    glColor4f(1.f, 1.f, 1.f, 1.0);

	glPointSize(size*5.f);
	glBegin(GL_POINTS);
	glVertex2f(v[0], v[1]);
	glEnd();
#if 0
    } else {
	glEnable(GL_TEXTURE_2D);
	glScalef(1.f,10.f,1.f);
	for(i = 0; i <= 20; i++) {
	    float v = -1.f+2.f/20*i;
	    quad(-.1f, 0.f, v);
	    quad( .1f, 0.f, v);
	}
	glDisable(GL_TEXTURE_2D);
    }
#endif
    /*glDisable(GL_BLEND);*/
}

void display(void) {
    int i;
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    for(i = 0; i < 1000; i++) {
	float v[2];
	v[0] = 2.f*drand48() - 1.f;
	v[1] = 2.f*drand48() - 1.f;
	draw_light(v, drand48());
    }
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case 'b': bfunc(); break;
    case 'l': light(); break;
    case 'f': ffunc(); break;
    case 'F': Ffunc(); break;
    case 't': toggle_t(); break;
    case 'm': mfunc(); break;
    case 'w': wire(); break;
    case 'x': xfunc(); break;
    case 'p': pfunc(); break;
    case 'h': help(); break;
    case '\033': exit(EXIT_SUCCESS); break;
    default: break;
    }
    glutPostRedisplay();
}

/*ARGSUSED1*/
void
special(int key, int x, int y) {
    switch(key) {
    case GLUT_KEY_UP:	up(); break;
    case GLUT_KEY_DOWN:	down(); break;
    case GLUT_KEY_LEFT:	left(); break;
    case GLUT_KEY_RIGHT:right(); break;
    }
}

int main(int argc, char** argv) {
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE);
    (void)glutCreateWindow(argv[0]);
    init(argv[1]);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(animate);
    glutMainLoop();
    return 0;
}
