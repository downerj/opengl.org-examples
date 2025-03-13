#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>
#include <math.h>
#include "texture.h"

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
    if(error = glGetError())                                       \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

enum {X, Y, Z, W};
enum {R, G, B, A};
enum {OVER, ATTENUATE, NONE, LASTOP};
enum {OBJ_ANGLE, SLICES, CUTTING};

/* window dimensions */
int accumCount = 5;
int winWidth = 512;
int winHeight = 512;
int active;
int operator = OVER;
GLboolean texture = GL_TRUE;
GLboolean dblbuf = GL_TRUE;
GLboolean cut = GL_FALSE;
GLint cutbias = 50;
int ext_blend_color = 0;
int ext_texture3D = 0;

GLfloat lightangle[2] = {0.f, 0.f};
GLfloat objangle[2] = {0.f, 0.f};
GLfloat objpos[2] = {0.f, 0.f};

/* 3d texture data that's read in */
/* XXX TODO; make command line arguments */
int Texwid = 128; /* dimensions of each 2D texture */
int Texht = 128;
int Texdepth = 128; /* number of 2D textures */

/* Actual dimensions of the texture (restricted to max 3d texture size) */
int texwid, texht, texdepth;
int slices;


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
    switch(active)
    {
    case OBJ_ANGLE:
	objangle[X] = (x - winWidth/2) * 360./winWidth;
	objangle[Y] = (y - winHeight/2) * 360./winHeight;
	glutPostRedisplay();
	break;
    case SLICES:
	slices = x * texwid/winWidth;
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
    /* hack for 2 button mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;
    
	if(state == GLUT_DOWN)
	switch(button)
	{
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
    int i, a;
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

    glClear(GL_COLOR_BUFFER_BIT); /* clear both front and back buffers */
    glDrawBuffer(GL_FRONT);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawBuffer(GL_BACK);

    glPushMatrix(); /* identity */
    glClipPlane(GL_CLIP_PLANE0, clipplane0);
    glClipPlane(GL_CLIP_PLANE1, clipplane1);
    glClipPlane(GL_CLIP_PLANE2, clipplane2);
    glClipPlane(GL_CLIP_PLANE3, clipplane3);
    glClipPlane(GL_CLIP_PLANE4, clipplane4);
    glPopMatrix(); /* back to identity */

    switch(operator)
    {
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
       glEnable(GL_TEXTURE_3D_EXT);
    else
       glDisable(GL_TEXTURE_3D_EXT);
    
    for(i = 0; i < slices; i++)
    {
	/* XXX Loop over each slice; change tex coords, accumulate; read back; blend into fb */
	glDisable(GL_BLEND); /* turn off blending */
	glDrawBuffer(GL_BACK); /* render to back buffer */
	glMatrixMode(GL_TEXTURE);
	for(a = 0; a < accumCount; a++)
	{
	    glPushMatrix(); /* identity */
	    glTranslatef( .5f,  .5f, .5f);
	    glRotatef(a * 2.f, 1.f, 1.f, 1.f); /*2 degrees per accumulation*/
	    glTranslatef( -.5f,  -.5f, -.5f);

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
	    if(a == 0)
		glAccum(GL_LOAD, .5f/(GLfloat)accumCount);
	    else
		glAccum(GL_ACCUM, .5f/(GLfloat)accumCount);
	    glPopMatrix(); /* identity */
	}

	glAccum(GL_RETURN, .9f); /* lic'ed value for 1 slice in back buffer */

	glDrawBuffer(GL_FRONT); /* copy to front buffer */
	glEnable(GL_BLEND); /* turn on blending */
	    
#if 0
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glRasterPos2i(-1, -1); /* set rasterpos */
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
#endif	    
	glCopyPixels(0, 0, winWidth, winHeight, GL_COLOR);
    }
#ifdef GL_TEXTURE_3D_EXT
    glDisable(GL_TEXTURE_3D_EXT);
#endif
    if(!texture)
    {
       glDisable(GL_LIGHTING);
    }
    glDisable(GL_BLEND);


    if(operator == ATTENUATE)
    {
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
    switch(key)
    {
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
	    for(s = 0; s < maxs; s++)
	    {
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
    GLubyte *tex3ddata, *tptr;
    GLuint *texslice; /* 2D slice of 3D texture */
    GLint max3dtexdims; /* maximum allowed 3d texture dimension */
    GLint newval;
    int i;

    /* generate 3D texture data */

    tex3ddata = (GLubyte *)malloc(128 * 128 * 128 * sizeof(GLubyte) * 4);

    for(tptr = tex3ddata, i = 0; i < 128 * 128 * 128; i++)
    {
#ifdef _WIN32
	*tptr = ((float)rand()/(float)RAND_MAX) * 256.f; /* random 0 - 255 */
#else
	*tptr = drand48() * 256.f; /* random 0 - 255 */
#endif
	*(tptr + 1) = *tptr;
	*(tptr + 2) = *tptr;
	*(tptr + 3) = *tptr;
	tptr += 4;
    }

    
    *texht = *texwid = *texdepth = 128;

#ifdef GL_TEXTURE_3D_EXT
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE_EXT, &max3dtexdims);
#endif

#ifdef GL_TEXTURE_3D_EXT
	glPixelStorei(GL_UNPACK_IMAGE_HEIGHT_EXT, *texht);
#endif

    return tex3ddata;
}


main(int argc, char *argv[])
{
    int texcomps;
    GLubyte *tex3ddata;
    static GLfloat splane[4] = {1.f/200.f, 0.f, 0.f, .5f};
    static GLfloat rplane[4] = {0, 1.f/200.f, 0, .5f};
    static GLfloat tplane[4] = {0, 0, 1.f/200.f, .5f};
    static GLfloat lightpos[4] = {150., 150., 150., 1.f};


    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    if(argc > 1)
    {
	char *args = argv[1];
	GLboolean done = GL_FALSE;
	while(!done)
	{
	    switch(*args)
	    {
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
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_ACCUM);

    (void)glutCreateWindow("volume rendering demo");
    glutDisplayFunc(redraw);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);
    ext_blend_color = glutExtensionSupported("EXT_blend_color");
    ext_texture3D = glutExtensionSupported("EXT_texture3D");
    /*XXXblythe bug in glut for EXT_texture3D?*/
    ext_texture3D = (strstr((const char*)glGetString(GL_EXTENSIONS), "GL_EXT_texture3D") != 0);
	
#ifndef GL_TEXTURE_3D_EXT
//    ext_texture3D = 0;
#endif

    /* Initialize OpenGL State */

    glDisable(GL_DITHER);
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
    if (!ext_texture3D)
	{
		printf("\nThis demo requires the 3D texture extension to OpenGL.\n");
		exit(1);
    }

#ifdef GL_TEXTURE_3D_EXT
    glEnable(GL_TEXTURE_3D_EXT);
#endif

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_R);

    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

    glTexGenfv(GL_S, GL_OBJECT_PLANE, splane);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, tplane);
    glTexGenfv(GL_R, GL_OBJECT_PLANE, rplane);

    /* to avoid boundary problems */
#ifdef GL_TEXTURE_3D_EXT
    glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_R_EXT, GL_REPEAT);
#endif

    glEnable(GL_CLIP_PLANE0);
    glEnable(GL_CLIP_PLANE1);
    glEnable(GL_CLIP_PLANE2);
    glEnable(GL_CLIP_PLANE3);
    glEnable(GL_CLIP_PLANE4);
    glEnable(GL_CLIP_PLANE5);

    glDisable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);



    tex3ddata = loadtex3d(&texwid, &texht, &texdepth, &texcomps);

    slices = texht;

#ifdef GL_TEXTURE_3D_EXT
    glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage3DEXT(GL_TEXTURE_3D_EXT, 0, GL_LUMINANCE_ALPHA,
		    texwid, texht, texdepth,
		    0,
		    GL_RGBA, GL_UNSIGNED_BYTE, tex3ddata);
#endif

    free(tex3ddata);

    CHECK_ERROR("end of main");

    glutMainLoop();
    return 0;
}
