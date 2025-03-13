#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glut.h>
#include "texture.h"
#ifdef _WIN32
#define sinf(x) ((float)sin((x)))
#define cosf(x) ((float)cos((x)))
#define sqrtf(x) ((float)sqrt(x))
#define floorf(x) ((float)floor(x))
#ifndef M_PI
#define M_PI 3.14159265
#endif
#endif
#ifndef GL_EXT_texture
#define glBindTextureEXT	glBindTexture
#endif

/* TEST PROGRAM */

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if((error = glGetError()) != GL_NO_ERROR)                      \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

#ifndef FALSE
enum {FALSE, TRUE};
#endif
enum {X, Y, Z, W};
enum {OBJ_ANGLE, LIGHT_ANGLE, OBJ_TRANSLATE};
enum {SURF_TEX, HIGHLIGHT_TEX};

/* window dimensions */
int winWidth = 512;
int winHeight = 512;
int active;
int dblbuf = TRUE;
int highlight = TRUE;
int gouraud = FALSE;
int texmap = FALSE;
int notex = FALSE; /* don't texture gouraud */
int fresnelmap = 0;
int transparent;

int object = 2;
int maxobject = 2;

GLfloat lightangle[2] = {0.f, 0.f};
GLfloat objangle[2] = {0.f, 0.f};
GLfloat objpos[2] = {0.f, 0.f};

GLfloat color[4] = {1.f, 1.f, 1.f, 1.f};
GLfloat zero[4] = {0.f, 0.f, 0.f, 1.f};
GLfloat one[4] = {1.f, 1.f, 1.f, 1.f};

int texdim = 128;
unsigned *lighttex = 0;
GLfloat lightpos[4] = {0.f, 0.f, 1.f, 0.f};
GLboolean lightchanged[2] = {GL_TRUE, GL_TRUE};
enum {UPDATE_OGL, UPDATE_TEX};

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
	glFrustum(-100.*a, 100.*a, -100., 100., 300., 600.); 
    else
	glFrustum(-100., 100., -100.*a, 100.*a, 300., 600.); 
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
    case LIGHT_ANGLE:
	lightangle[X] = (x - winWidth/2) * 2 * M_PI/winWidth;
	lightangle[Y] = (winHeight/2 - y) * 2 * M_PI/winHeight;

	lightpos[Y] = sin(lightangle[Y]);
	lightpos[X] = cos(lightangle[Y]) * sin(lightangle[X]);
	lightpos[Z] = cos(lightangle[Y]) * cos(lightangle[X]);
	lightpos[W] = 0.;
	lightchanged[UPDATE_OGL] = GL_TRUE;
	glutPostRedisplay();
	break;
    case OBJ_TRANSLATE:
	objpos[X] = (x - winWidth/2) * 100./winWidth;
	objpos[Y] = (winHeight/2 - y) * 100./winHeight;
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
	    active = LIGHT_ANGLE;
	    motion(x, y);
	    break;
	case GLUT_RIGHT_BUTTON: /* move the polygon */
	    active = OBJ_TRANSLATE;
	    motion(x, y);
	    break;
	}
}

/* draw the object unlit without surface texture */
void redraw_white(void)
{
    glPushMatrix(); /* assuming modelview */
    glTranslatef(objpos[X], objpos[Y], 0.f); /* translate object */
    glRotatef(objangle[X], 0.f, 1.f, 0.f); /* rotate object */
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);
    
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    /* draw the specular highlight */
    
    glCallList(object);

    glEnable(GL_TEXTURE_2D);

    glPopMatrix(); /* assuming modelview */

    CHECK_ERROR("OpenGL Error in redraw_tex()");

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}


/* just draw textured highlight */
void redraw_tex_highlight(void)
{
    glPushMatrix(); /* assuming modelview */
    glTranslatef(objpos[X], objpos[Y], 0.f); /* translate object */
    glRotatef(objangle[X], 0.f, 1.f, 0.f); /* rotate object */
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);
    
    glClearColor(.1f, .1f, .1f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    
    /* draw the specular highlight */
    glBindTextureEXT(GL_TEXTURE_2D, HIGHLIGHT_TEX);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    
    glCallList(object);

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);

    glPopMatrix(); /* assuming modelview */

    CHECK_ERROR("OpenGL Error in redraw_tex()");

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}

/* show highlight texture map */
void redraw_map(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glBindTextureEXT(GL_TEXTURE_2D, HIGHLIGHT_TEX+fresnelmap);
    glDisable(GL_LIGHTING);

    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex2i(-1, -1);

    glTexCoord2i(1, 0);
    glVertex2i(1, -1);

    glTexCoord2i(1, 1);
    glVertex2i(1,  1);

    glTexCoord2i(0, 1);
    glVertex2i(-1,  1);
    glEnd();

    /*glEnable(GL_LIGHTING);*/

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    CHECK_ERROR("OpenGL Error in redraw_map()");

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}

/* multipass, use texture to make highlight */
void redraw_tex(void)
{
    if(lightchanged[UPDATE_OGL])
    {
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	lightchanged[UPDATE_OGL] = GL_FALSE;
    }

    glPushMatrix(); /* assuming modelview */
    glTranslatef(objpos[X], objpos[Y], 0.f); /* translate object */
    glRotatef(objangle[X], 0.f, 1.f, 0.f); /* rotate object */
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);
    

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    /* draw the diffuse portion of the light */
    glEnable(GL_LIGHTING);

    glBindTextureEXT(GL_TEXTURE_2D, SURF_TEX);

    if(notex)
	glDisable(GL_TEXTURE_2D);
    glCallList(object);
    if(notex)
	glEnable(GL_TEXTURE_2D);

    glDisable(GL_LIGHTING);

    /* draw the specular highlight */
    glEnable(GL_BLEND);
    glBindTextureEXT(GL_TEXTURE_2D, HIGHLIGHT_TEX);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glDepthFunc(GL_LEQUAL);
    
    glCallList(object);

    glDepthFunc(GL_LESS);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_BLEND);

    glPopMatrix(); /* assuming modelview */

    CHECK_ERROR("OpenGL Error in redraw_tex()");

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}

/* multipass; use opengl to make highlight */
void redraw_phong(void)
{
    if(lightchanged[UPDATE_OGL])
    {
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	lightchanged[UPDATE_OGL] = GL_FALSE;
    }

    glPushAttrib(GL_LIGHTING);
    glPushMatrix(); /* assuming modelview */
    glTranslatef(objpos[X], objpos[Y], 0.f); /* translate object */
    glRotatef(objangle[X], 0.f, 1.f, 0.f); /* rotate object */
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);
    
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    /* draw the diffuse portion of the light */
    glEnable(GL_LIGHTING);

    glBindTextureEXT(GL_TEXTURE_2D, SURF_TEX);

    if(notex)
	glDisable(GL_TEXTURE_2D);
    glCallList(object);
    if(notex)
	glEnable(GL_TEXTURE_2D);

    /* turn off texturing so phong highlight color is pure */
    glDisable(GL_TEXTURE_2D); 

    glMaterialfv(GL_FRONT, GL_SPECULAR, color); /* turn on ogl highlights */
    glMaterialfv(GL_FRONT, GL_DIFFUSE, zero); /* turn off ogl diffuse */
    glMaterialfv(GL_FRONT, GL_AMBIENT, zero); /* turn off ogl diffuse */

    /* draw the specular highlight */
    glEnable(GL_BLEND);
    glDepthFunc(GL_LEQUAL);
    
    glCallList(object);

    glEnable(GL_TEXTURE_2D); 
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);

    glPopMatrix(); /* assuming modelview */
    glPopAttrib();

    CHECK_ERROR("OpenGL Error in redraw_tex()");

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}


void redraw_original(void)
{
    glClearColor(0.f, 0.f, .5f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    glPushMatrix(); /* assuming modelview */
    glTranslatef(objpos[X], objpos[Y], 0.f); /* translate object */
    glRotatef(objangle[X], 0.f, 1.f, 0.f); /* rotate object */
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);
    
    if(lightchanged[UPDATE_OGL])
    {
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	lightchanged[UPDATE_OGL] = GL_FALSE;
    }

#if 0
    glEnable(GL_LIGHTING);

    glBindTextureEXT(GL_TEXTURE_2D, SURF_TEX);
    glMaterialfv(GL_FRONT, GL_SPECULAR, color); /* turn on ogl highlights */
    if(notex)
	glDisable(GL_TEXTURE_2D);
    glCallList(object);
    if(notex)
	glEnable(GL_TEXTURE_2D);
    glMaterialfv(GL_FRONT, GL_SPECULAR, zero);

    glDisable(GL_LIGHTING);
#endif

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glBindTextureEXT(GL_TEXTURE_2D, HIGHLIGHT_TEX+fresnelmap);
    glCallList(object);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glPopMatrix(); /* assuming modelview */

    CHECK_ERROR("OpenGL Error in redraw()");

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}


/*ARGSUSED1*/
void key(unsigned char key, int x, int y)
{
    static int wire;
    switch(key)
    {
    case 's': /* unmodified highlight */
	printf("Show environment mapped object\n");
	glutDisplayFunc(redraw_original);
	glutPostRedisplay();
	break;
    case 'g': /* separate gouraud shaded highlight */
	printf("Use separate gouraud shaded highlight\n");
	glutDisplayFunc(redraw_phong);
	glutPostRedisplay();
	break;
    case 't': /* separate textured highlight */
	printf("Use separate texture highlight\n");
	glutDisplayFunc(redraw_tex);
	glutPostRedisplay();
	break;
    case 'o':
	/* toggle object type */
	object++;
	if(object > maxobject)
	    object = 2;
	glutPostRedisplay();
	break;
    case 'm':
	/* show highlight texture map */
	printf("Show environment map\n");
	glutDisplayFunc(redraw_map);
	glutPostRedisplay();
	break;
    case 'n':
	if(notex)
	    notex = FALSE;
	else
	    notex = TRUE;
	glutPostRedisplay();
	break;
    case 'h':
	printf("Show textured phong highlight\n");
	glutDisplayFunc(redraw_tex_highlight);
	glutPostRedisplay();
	break;
    case 'u':
	printf("Show white object\n");
	glutDisplayFunc(redraw_white);
	glutPostRedisplay();
	break;
    case 'w':
	if (wire ^= 1)
	    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
	    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glutPostRedisplay();
	break;
    case 'b':
	if (transparent ^= 1) {
	    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	    glEnable(GL_BLEND);
		glClear(GL_DEPTH_BUFFER_BIT);
	    glDepthMask(0);
	} else {
	    glDisable(GL_BLEND);
	    glDepthMask(1);
	}
	glutPostRedisplay();
	break;
    case 'f':
	fresnelmap ^= 1;
	printf("%s map\n", fresnelmap ? "fresnel" : "non-fresnel");
	glutPostRedisplay();
	break;
    case '\033':
	exit(0);
	break;
    default:
	fprintf(stderr, "Keyboard commands:\n\n"
		"s - (unmodified opengl lighting)\n"
		"d - diffuse only\n"
		"g - multipass gouraud highlight\n"
		"t - multipass texture highlight\n"
		"o - toggle object\n"
		"m - show highlight texture map\n"
		"n - toggle surface texturing\n"
		"h - show textured phong highlight\n"
		"p - show gouraud-shaded phong highlight\n"
		"u - show unlit white object\n"
		"w - show object tessellation\n");
	break;
    }

}

void
menu(int which) {
    key((char)which, 0, 0);
}

void
create_menu(void) {
    glutCreateMenu(menu);
    glutAddMenuEntry("show envmapped object", 's');
    glutAddMenuEntry("show envmap", 'm');
    glutAddMenuEntry("toggle map", 'f');
    glutAddMenuEntry("toggle transparency", 'b');
    glutAddMenuEntry("show object tessellation", 'w');
    glutAddMenuEntry("switch object", 'o');
    glutAddMenuEntry("exit", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

int
main(int argc, char *argv[])
{
    unsigned *tex;
    int texwid, texht, texcomps;
    char *envmap = "../../data/fresnelgoldmap.rgb";
    char *envmap2 = "../../data/nofresnelgoldmap.rgb";

    GLUquadric *quad;

    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    while (argc > 1 && argv[1][0] == '-') {
	switch(argv[1][1]) {
	case 's': /* single buffer */
	    printf("Single Buffered\n");
	    dblbuf = FALSE;
	    break;
	case '-': /* do nothing */
	    break;
	}
	argc--; argv++;
    }
    if (argc > 2) {
	envmap = argv[1];
	envmap2 = argv[2];
    } else if (argc > 1) envmap2 = argv[1];
    if(dblbuf)
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
    else
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH);
    (void)glutCreateWindow("envmap");
    glutDisplayFunc(redraw_original);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);
    create_menu();

    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-100., 100., -100., 100., 300., 600.); 
    glMatrixMode(GL_MODELVIEW);
    /* look at scene from (0, 0, 450) */
    gluLookAt(0., 0., 450., 0., 0., 0., 0., 1., 0.);

    /* turn on features */
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_TEXTURE_2D);
    
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

    glMateriali(GL_FRONT, GL_SHININESS, 128); /* cosine power */

    /* remove back faces to speed things up */
    glCullFace(GL_BACK);
    glBlendFunc(GL_ONE, GL_ONE);

    lightchanged[UPDATE_OGL] = GL_TRUE;

    /* load pattern for current 2d texture */

    tex = read_texture("../../data/wood0.rgb", &texwid, &texht, &texcomps);

    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, texwid, texht, GL_RGBA,
		      GL_UNSIGNED_BYTE, tex);
    free(tex);
    CHECK_ERROR("end of main");

    glBindTextureEXT(GL_TEXTURE_2D, HIGHLIGHT_TEX);
    lighttex = read_texture(envmap, &texwid, &texht, &texcomps);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texwid, texht, 0, GL_RGBA, GL_UNSIGNED_BYTE, lighttex);
    free(lighttex);

    glBindTextureEXT(GL_TEXTURE_2D, HIGHLIGHT_TEX+1);
    lighttex = read_texture(envmap2, &texwid, &texht, &texcomps);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texwid, texht, 0, GL_RGBA, GL_UNSIGNED_BYTE, lighttex);
    free(lighttex);

    glNewList(1, GL_COMPILE);
    glutSolidSphere((GLdouble)texdim/2., 50, 50);
    glEndList();

    glNewList(2, GL_COMPILE);
    glutSolidTeapot(70.);
    glEndList();

    quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);

    glNewList(3, GL_COMPILE);
    gluSphere(quad, 70., 20, 20);
    glEndList();

    gluDeleteQuadric(quad);
    maxobject = 3;

    glutMainLoop();
    return 0;
}
