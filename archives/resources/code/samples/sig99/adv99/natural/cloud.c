#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include "texture.h"
#include "GL/glut.h"

#ifdef _WIN32
#define expf	exp
#endif

static float ttrans[2];
static float scale = 1.;
static float transx, transy, rotx, roty;
static int ox = -1, oy = -1;
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

    /* hack for 2 button mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;
    
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

void wire(void) {
    static int w;
    if (w ^= 1)
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void up(void) { scale += .1f; }
void down(void) { scale -= .1f; }

void
animate(void) {
    ttrans[0] += .01f;
    if (ttrans[0] == 1.0f) ttrans[0] = 0.f;
    ttrans[1] += .005f;
    if (ttrans[1] == 1.0f) ttrans[1] = 0.f;
    glutPostRedisplay();
}

void help(void) {
    printf("Usage: cloud [image]\n");
    printf("'h'            - help\n");
    printf("'w'            - toggle wireframe\n");
    printf("'UP'           - scale up\n");
    printf("'DOWN'         - scale down\n");
    printf("left mouse     - pan\n");
    printf("middle mouse   - rotate\n");
}

GLfloat fog_color[4];

void init(char *filename) {
    GLfloat cloud_color[4] = { 1.f, 1.f, 1.f, 0.f, };
    GLfloat fog_density = 0.05f, density, far_cull;
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
	if (components != 1) {
	    printf("must be a bw image\n");
	    exit(EXIT_FAILURE);
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
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, cloud_color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, components, width,
                 height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 image);
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.,1.,.1,far_cull = 10.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.,0.,-5.5);

    density = 1.- expf(-5.5 * fog_density * fog_density *
			      far_cull * far_cull);

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
    density = MAX(MIN(density, 1.), 0.);

    fog_color[0] = .23 + density *.57;
    fog_color[1] = .35 + density *.45;
    fog_color[2] = .78 + density *.22;

    glClearColor(fog_color[0], fog_color[1], fog_color[2], 1.f);

    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, fog_density);
    glFogfv(GL_FOG_COLOR, fog_color);
    if (fog_density > 0)
	glEnable(GL_FOG);
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    glPushMatrix();
    glTranslatef(transx, transy, 0.f);
    glRotatef(rotx, 0., 1., 0.);
    glRotatef(roty, 1., 0., 0.);
    glScalef(scale,scale,1.);
    glScalef(10,1,10);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glTranslatef(ttrans[0], ttrans[1], 0.);
    glBegin(GL_QUADS);
    glColor3f(.19f, .25f, .70f);
    glTexCoord2f(0.f, 0.f); glVertex3f(-1.f, 1.f, -1.f);
    glTexCoord2f(0.f, 5.f); glVertex3f(-1.f, 1.f,  1.f);
    glTexCoord2f(5.f, 5.f); glVertex3f( 1.f, 1.f,  1.f);
    glTexCoord2f(5.f, 0.f); glVertex3f( 1.f, 1.f, -1.f);
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
}

/*ARGSUSED1*/
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case 'w': wire(); break;
    case 'h': help(); break;
    case '\033': exit(EXIT_SUCCESS); break;
    default: help(); break;
    }
    glutPostRedisplay();
}

/*ARGSUSED1*/
void
special(int key, int x, int y) {
    switch(key) {
    case GLUT_KEY_UP:	up(); break;
    case GLUT_KEY_DOWN:	down(); break;
    }
}

int main(int argc, char** argv) {
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE);
    (void)glutCreateWindow(argv[0]);
    if(argc < 1)
	init(argv[1]);
    else
	init("../../data/clouds.bw");
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(animate);

    help();
    glutMainLoop();
    return 0;
}
