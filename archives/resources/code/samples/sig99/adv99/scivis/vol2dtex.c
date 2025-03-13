/*
 * vol2dtex.c - volume rendering using a stack of 2D textures
 *            - this version keeps a single stack rather than 3 stacks
 *              to keep things simple, but will show severe artifacts
 *              when the slices are viewed near edge-on.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>
#include <math.h>
#include "texture.h"

static void cleanup(void);

/* nonzero if not power of 2 */ 
#define NOTPOW2(num) ((num) & (num - 1))

int
makepow2(int val)
{
    int power = 0;
    if(!val)
	return 0;

    while(val >>= 1)
	power++;

    return(1 << power);
}

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if((error = glGetError()) != GL_NO_ERROR)                      \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

enum {X, Y, Z, W};
enum {R, G, B, A};
enum {OVER, ATTENUATE, NONE, LASTOP};
enum {OBJ_ANGLE, SLICES, CUTTING};

/* window dimensions */
int winWidth = 512;
int winHeight = 512;
int active;
int operator = OVER;
GLboolean texture = GL_TRUE;
GLboolean dblbuf = GL_TRUE;
GLboolean cut = GL_FALSE;
GLint cutbias = 50;
int ext_blend_color = 0;

GLfloat lightangle[2] = {0.f, 0.f};
GLfloat objangle[2] = {0.f, 0.f};
GLfloat objpos[2] = {0.f, 0.f};

/* 3d texture data that's read in */
/* XXX TODO; make command line arguments */
int Texwid = 128; /* dimensions of each 2D texture */
int Texht = 128;
int Texdepth = 64; /* number of 2D textures */

/* Actual dimensions of the texture (restricted to max 3d texture size) */
int texwid, texht, texdepth;
int slices;
int slices_X, slices_Y, slices_Z;
int base_X, base_Y, base_Z;


GLfloat *lighttex = 0;
GLfloat lightpos[4] = {0.f, 0.f, 1.f, 0.f};
GLboolean lightchanged[2] = {GL_TRUE, GL_TRUE};


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
    /* cube, 300 on a side */
    if (a > 1)
	glOrtho(-150.*a, 150.*a, -150., 150., -150., 150.);
    else
	glOrtho(-150., 150., -150.*a, 150.*a, -150., 150.);
    glMatrixMode(GL_MODELVIEW);
}


void
motion(int x, int y)
{
    switch(active) {
    case OBJ_ANGLE:
	objangle[X] = (x - winWidth/2) * 360./winWidth;
	objangle[Y] = (y - winHeight/2) * 360./winHeight;
	glutPostRedisplay();
	break;
    case SLICES:
	slices = x * texwid/winWidth;
        if (slices > slices_Z) slices = slices_Z;
	glutPostRedisplay();
	break;
    case CUTTING:
	cutbias = (x - winWidth/2) * 300/winWidth;
	glutPostRedisplay();
	break;
    }
}

void
mouse(int button, int state, int x, int y)
{
    if(state == GLUT_DOWN)
	switch(button) {
	case GLUT_LEFT_BUTTON: /* move the light */
	    active = OBJ_ANGLE;
	    motion(x, y);
	    break;
	case GLUT_MIDDLE_BUTTON:
	    active = CUTTING;
	    motion(x, y);
	    break;
	case GLUT_RIGHT_BUTTON: /* move the polygon */
	    active = SLICES;
	    motion(x, y);
	    break;
	}
}

GLdouble clipplane0[] = {-1.,  0.,  0., 100.};
GLdouble clipplane1[] = { 1.,  0.,  0., 100.};
GLdouble clipplane2[] = { 0., -1.,  0., 100.};
GLdouble clipplane3[] = { 0.,  1.,  0., 100.};
GLdouble clipplane4[] = { 0.,  0., -1., 100.};
GLdouble clipplane5[] = { 0.,  0.,  1., 100.};

/* define a cutting plane */
GLdouble cutplane[] = {0.f, -.5f, -2.f, 50.f};

/* draw the object unlit without surface texture */
void redraw(void)
{
    int i;
    GLfloat offS, offT, offR; /* mapping texture to planes */

    offS = 200.f/texwid;
    offT = 200.f/texht;
    offR = 200.f/texdepth;
    
    clipplane0[W] = 100.f - offS;
    clipplane1[W] = 100.f - offS;
    clipplane2[W] = 100.f - offT;
    clipplane3[W] = 100.f - offT;
    clipplane4[W] = 100.f - offR;
    clipplane5[W] = 100.f - offR;

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    /* GL_MODELVIEW */
    if(cut) {
	cutplane[W] = cutbias;
	glClipPlane(GL_CLIP_PLANE5, cutplane);
    }

    glPushMatrix(); /* identity */
    glRotatef(objangle[X], 0.f, 1.f, 0.f);
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);
    if(!cut)
	glClipPlane(GL_CLIP_PLANE5, clipplane5);
    glPopMatrix(); /* back to identity */

    switch(operator) {
    case OVER:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	break;
    case ATTENUATE:
#ifdef GL_CONSTANT_ALPHA_EXT
	if (ext_blend_color) {
	    glEnable(GL_BLEND);
	    glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE);
	    glBlendColorEXT(1.f, 1.f, 1.f, 1.f/slices);
	    break;
	}
#endif
	printf("EXT_blend_color not supported\n");
	break;
    case NONE:
	/* don't blend */
	break;
    }

    if(texture)
       glEnable(GL_TEXTURE_2D);
    else {
       glDisable(GL_TEXTURE_2D);
       glEnable(GL_LIGHTING);
       glEnable(GL_LIGHT0);
    }
    glPushMatrix();
    glRotatef(objangle[X], 0.f, 1.f, 0.f);
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);
    for(i = 0; i < slices; i++) {
	glBindTexture(GL_TEXTURE_2D, i+1);
	glBegin(GL_QUADS);
	glVertex3f(-100.f, -100.f, 
		   -100.f + offR + i * (200.f - 2 * offR)/(slices - 1));
	glVertex3f( 100.f, -100.f,
		   -100.f + offR + i * (200.f - 2 * offR)/(slices - 1));
	glVertex3f( 100.f,  100.f,
		   -100.f + offR + i * (200.f - 2 * offR)/(slices - 1));
	glVertex3f(-100.f,  100.f,
		   -100.f + offR + i * (200.f - 2 * offR)/(slices - 1));
	glEnd();
    }
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
    if(!texture) {
       glDisable(GL_LIGHTING);
    }
    glDisable(GL_BLEND);

    if(operator == ATTENUATE) {
	glPixelTransferf(GL_RED_SCALE, 3.f); /* brighten image */
	glPixelTransferf(GL_GREEN_SCALE, 3.f);
	glPixelTransferf(GL_BLUE_SCALE, 3.f);
	glCopyPixels(0, 0, winWidth, winHeight, GL_COLOR);
    }
    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw()");
}




void
help(void) {
    printf("o    - toggle operator (over, attenuate, none\n");
    printf("l    - toggle texture\n");
    printf("c    - toggle cutting plane\n");
    printf("left mouse    - rotate object\n");
    printf("middle mouse  - move cutting plane\n");
    printf("right mouse   - change # slices\n");
}

/*ARGSUSED1*/
void key(unsigned char key, int x, int y)
{
    switch(key) {
    case 'o':
	operator++;
	if(operator == LASTOP)
	    operator = OVER;
	glutPostRedisplay();
	break;
    case 'l':
	if(texture)
	    texture = GL_FALSE;
	else
	    texture = GL_TRUE;
	glutPostRedisplay();
	break;
    case 'c':
	if(cut)
	    cut = GL_FALSE;
	else
	    cut = GL_TRUE;
	glutPostRedisplay();
	break;
    case '\033':
	cleanup();
	exit(0);
	break;
    default: help(); break;
    }

}

/*
** Create a single component 3d texture map
** GL_LUMINANCE_ALPHA 3D Checkerboard
*/
GLfloat *make_texture3dcheck(int maxs, int maxt, int maxr)
{
    int s, t, r;
    GLfloat value;
    static GLfloat *texture;

    texture = (GLfloat *)malloc(2 * maxs * maxt * maxr * sizeof(GLfloat));
    for(r = 0; r < maxr; r++)
	for(t = 0; t < maxt; t++)
	    for(s = 0; s < maxs; s++) {
		value = (((s >> 2) & 0x3)) ^ 
		        (((t >> 2) & 0x3)) ^ 
		        (((r >> 2) & 0x1));
		texture[    (s + (t + r * maxt) * maxs) * 2] = value;
		texture[1 + (s + (t + r * maxt) * maxs) * 2] = value;
	    }    
    return texture;
}

GLubyte *
loadtex3d(int *texwid, int *texht, int *texdepth, int *texcomps)
{
    char *filename;
    GLubyte *tex3ddata;
    GLuint *texslice; /* 2D slice of 3D texture */
    GLint max3dtexdims; /* maximum allowed 3d texture dimension */
    GLint newval;
    int i;

    /* load 3D texture data */
    filename = (char*)malloc(sizeof(char) * strlen("../../data/skull/skullXX.la"));

    tex3ddata = (GLubyte *)malloc(Texwid * Texht * Texdepth * 
				  4 * sizeof(GLubyte));
    for(i = 0; i < Texdepth; i++) {
	sprintf(filename, "../../data/skull/skull%d.la", i);
	/* read_texture reads as RGBA */
	texslice = read_texture(filename, texwid, texht, texcomps);
#if 1
	gluScaleImage(GL_RGBA, Texwid, Texht, GL_UNSIGNED_BYTE, texslice,
                       Texwid/2, Texht/2, GL_UNSIGNED_BYTE, &tex3ddata[i * Texwid/2 * Texht/2 * 4]);
#else
	memcpy(&tex3ddata[i * Texwid * Texht * 4],  /* copy in a slice */
	       texslice, 
	       Texwid * Texht * 4 * sizeof(GLubyte));
#endif
	free(texslice);
    }
#if 1
    Texwid /= 2;
    Texht /= 2;
    *texwid /= 2;
    *texht /= 2;
#endif
    free(filename);

    *texdepth = Texdepth;
    max3dtexdims = 64;

    /* adjust width */
    newval = *texwid;
    if(*texwid > max3dtexdims)
	newval = max3dtexdims;
    if(NOTPOW2(*texwid))
        newval = makepow2(*texwid);
    if(newval != *texwid) {
	glPixelStorei(GL_UNPACK_ROW_LENGTH, *texwid);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, (*texwid - newval)/2);
	*texwid = newval;
    }

    /* adjust height */
    newval = *texht;
    if(*texht > max3dtexdims)
	newval = max3dtexdims;
    if(NOTPOW2(*texht))
        newval = makepow2(*texht);
    if(*texht > newval) {
	glPixelStorei(GL_UNPACK_SKIP_ROWS, (*texht - newval)/2);
	*texht = newval;
    }

    /* adjust depth */
    newval = *texdepth;
    if(*texdepth > max3dtexdims)
	newval = max3dtexdims;
    if(NOTPOW2(*texdepth))
        newval = makepow2(*texdepth);
    if(*texdepth > newval) {
	*texdepth = newval;
    }
    return tex3ddata;
}


GLubyte *tex3ddata;
static void
cleanup(void) {
    free(tex3ddata);
}

int
main(int argc, char *argv[])
{
    int z;
    int texcomps;
    static GLfloat splane[4] = {1.f/200.f, 0.f, 0.f, .5f};
    static GLfloat tplane[4] = {0, 1.f/200.f, 0, .5f};
    static GLfloat lightpos[4] = {150., 150., 150., 1.f};


    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    if(argc > 1) {
	char *args = argv[1];
	GLboolean done = GL_FALSE;
	while(!done) {
	    switch(*args) {
	    case 's': /* single buffer */
		printf("Single Buffered\n");
		dblbuf = GL_FALSE;
		break;
	    case '-': /* do nothing */
		break;
	    case 0:
		done = GL_TRUE;
		break;
	    }
	    args++;
	}
    }
    if(dblbuf)
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
    else
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH);

    (void)glutCreateWindow("volume rendering demo");
    glutDisplayFunc(redraw);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);
    ext_blend_color = glutExtensionSupported("GL_EXT_blend_color");

    /* Initialize OpenGL State */

    /* draw a perspective scene */
#if 0
    glMatrixMode(GL_PROJECTION);
    /* cube, 300 on a side */
    glFrustum(-150., 150., -150., 150., 300., 600.);
    glMatrixMode(GL_MODELVIEW);
    /* look at scene from (0, 0, 450) */
    gluLookAt(0., 0., 450., 0., 0., 0., 0., 1., 0.);
#else
    glMatrixMode(GL_PROJECTION);
    /* cube, 300 on a side */
    glOrtho(-150., 150., -150., 150., -150., 150.);
    glMatrixMode(GL_MODELVIEW);
    /* look at scene from (0, 0, 150) */
    gluLookAt(0., 0., 150., 0., 0., 0., 0., 1., 0.);
#endif
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);

    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

    glTexGenfv(GL_S, GL_OBJECT_PLANE, splane);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, tplane);

    glEnable(GL_CLIP_PLANE5);

    glDisable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    tex3ddata = loadtex3d(&texwid, &texht, &texdepth, &texcomps);

    slices = texdepth;
    slices_Z = slices;
    base_Z = 1;
    base_X = base_Z + slices;
    slices_X = texwid;
    base_Y = base_X + slices_X;
    slices_Y = texht;

    for(z = 0; z < texdepth; z++) {
	glBindTexture(GL_TEXTURE_2D, z+base_Z);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
		    texwid, texht,
		    0,
		    GL_RGBA, GL_UNSIGNED_BYTE, tex3ddata+texwid*texht*z*sizeof(GLubyte)*4);
    }


    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    free(tex3ddata);

    CHECK_ERROR("end of main");

    glutMainLoop();
    return 0;
}
