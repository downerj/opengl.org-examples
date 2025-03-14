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

void fix_texture(int *texwid, int *texht, int *texcomps, unsigned **tex);

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

int object = 2;
int maxobject = 2;

GLfloat lightangle[2] = {0.f, 0.f};
GLfloat objangle[2] = {0.f, 0.f};
GLfloat objpos[2] = {0.f, 0.f};

GLfloat color[4] = {1.f, 1.f, 1.f, 1.f};
GLfloat zero[4] = {0.f, 0.f, 0.f, 1.f};
GLfloat one[4] = {1.f, 1.f, 1.f, 1.f};

int texdim = 128;
GLfloat *lighttex = 0;
GLfloat lightpos[4] = {0.f, 0.f, 1.f, 0.f};
GLboolean lightchanged[2] = {GL_TRUE, GL_TRUE};
enum {UPDATE_OGL, UPDATE_TEX};


/* draw a highly tesselated sphere with a specular highlight */
void
makeHighlight(int shinyness)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glPushMatrix(); /* starts as modelview */
    glLoadIdentity();
    glTranslatef(0.f, 0.f, -texdim/2.f);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-texdim/2., texdim/2., -texdim/2., texdim/2., 0., texdim/2.);

    glPushAttrib(GL_LIGHTING_BIT|GL_VIEWPORT_BIT);
    glViewport(0, 0, texdim, texdim);
    glEnable(GL_LIGHTING);

    glLightfv(GL_LIGHT0, GL_DIFFUSE, zero);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos); /* light direction */

    /* XXX TODO, range check an report errors */
    glMateriali(GL_FRONT, GL_SHININESS, shinyness); /* cosine power */
    glMaterialfv(GL_FRONT, GL_AMBIENT, zero); 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, zero); 
    glMaterialfv(GL_FRONT, GL_SPECULAR, color);
    glDisable(GL_TEXTURE_2D);

    glCallList(1);

    glEnable(GL_TEXTURE_2D);
    glPopAttrib();
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glReadPixels(0, 0, texdim, texdim, GL_RGB, GL_FLOAT, lighttex);

    glBindTextureEXT(GL_TEXTURE_2D, HIGHLIGHT_TEX);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texdim, texdim, 0, GL_RGB,
		 GL_FLOAT, lighttex);
}

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
	lightchanged[UPDATE_TEX] = GL_TRUE;
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
    if(lightchanged[UPDATE_TEX])
    {
	makeHighlight(128);
	lightchanged[UPDATE_TEX] = GL_FALSE;
    }


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

/* just draw opengl highlight */
void redraw_gouraud_highlight(void)
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
    
    glClearColor(.1f, .1f, .1f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    
    /* draw the specular highlight */

    glEnable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glMaterialfv(GL_FRONT, GL_SPECULAR, color); /* turn on ogl highlights */
    glMaterialfv(GL_FRONT, GL_DIFFUSE, zero); /* turn off ogl diffuse */
    glMaterialfv(GL_FRONT, GL_AMBIENT, zero); /* turn off ogl diffuse */

    /* draw the specular highlight */
    glCallList(object);

    glEnable(GL_TEXTURE_2D);
    glPopMatrix(); /* assuming modelview */
    glPopAttrib();

    CHECK_ERROR("OpenGL Error in redraw_tex()");

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}



/* show highlight texture map */
void redraw_map(void)
{
    if(lightchanged[UPDATE_TEX])
    {
	makeHighlight(128);
	lightchanged[UPDATE_TEX] = GL_FALSE;
    }

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glBindTextureEXT(GL_TEXTURE_2D, HIGHLIGHT_TEX);
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

    glEnable(GL_LIGHTING);

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
    if(lightchanged[UPDATE_TEX])
    {
	makeHighlight(128);
	lightchanged[UPDATE_TEX] = GL_FALSE;
    }
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


/* Use OpenGL lighting to get highlight */
void redraw_original(void)
{
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

    glPopMatrix(); /* assuming modelview */

    CHECK_ERROR("OpenGL Error in redraw()");

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
}

/* Draw with no highlight */
void redraw_diffuse(void)
{
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

    /* draw the diffuse portion of the light */
    glEnable(GL_LIGHTING);

    glBindTextureEXT(GL_TEXTURE_2D, SURF_TEX);
    glCallList(object);

    glDisable(GL_LIGHTING);

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
	printf("Use unmodified gouraud highlight\n");
	glutDisplayFunc(redraw_original);
	glutPostRedisplay();
	break;
    case 'd':
	printf("Draw diffuse only\n");
	glutDisplayFunc(redraw_diffuse);
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
	printf("Show highlight texture map\n");
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
    case 'p':
	printf("Show gouraud-shaded phong highlight\n");
	glutDisplayFunc(redraw_gouraud_highlight);
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
    glutAddMenuEntry("single-pass lighting", 's');
    glutAddMenuEntry("multi-pass per-vertex highligt", 'g');
    glutAddMenuEntry("multi-pass texture highlight", 't');
    glutAddMenuEntry("show highlight texture", 'm');
    glutAddMenuEntry("show diffuse lighting", 'd');
    glutAddMenuEntry("show textured specular lighting", 'h');
    glutAddMenuEntry("show per-vertex specular lighting", 'p');
    glutAddMenuEntry("show unlighted object", 'u');
    glutAddMenuEntry("toggle texture", 'n');
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

    GLUquadric *quad;

    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    if(argc > 1)
    {
	char *args = argv[1];
	int done = FALSE;
	while(!done)
	{
	    switch(*args)
	    {
	    case 's': /* single buffer */
		printf("Single Buffered\n");
		dblbuf = FALSE;
		break;
	    case '-': /* do nothing */
		break;
	    case 0:
		done = TRUE;
		break;
	    }
	    args++;
	}
    }
    if(dblbuf)
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
    else
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH);
    (void)glutCreateWindow("example program");
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
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

    glMateriali(GL_FRONT, GL_SHININESS, 128); /* cosine power */

    /* remove back faces to speed things up */
    glCullFace(GL_BACK);
    glBlendFunc(GL_ONE, GL_ONE);

    lightchanged[UPDATE_TEX] = GL_TRUE;
    lightchanged[UPDATE_OGL] = GL_TRUE;

    /* load pattern for current 2d texture */

    tex = read_texture("../../data/wood0.rgb", &texwid, &texht, &texcomps);
    fix_texture(&texwid, &texht, &texcomps, &tex);

    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, texwid, texht, GL_RGBA,
		      GL_UNSIGNED_BYTE, tex);
    free(tex);
    CHECK_ERROR("end of main");

    lighttex = (GLfloat *)malloc(texdim * texdim * sizeof(GL_FLOAT) * 3);



    /* XXX TODO use display list to avoid retesselating */
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


/*ARGSUSED*/
void
fix_texture(int *texwid, int *texht, int *texcomps, unsigned **tex) {
    int w = *texwid;
    int h = *texht;
    int x, y;
    int w2 = 2*w;
    int h2 = 2*h;
    unsigned *t = *tex;

    unsigned int *image = (unsigned int*)malloc(w*h*4*4);

    for(y = 0; y < h; y++)  {
        for(x = 0; x < w; x++) {
	    image[y*w2+x] = t[y*w+x];
	    image[y*w2+w+x] = t[y*w+w-x-1];
	    image[h*w2+y*w2+x] = t[(h-y-1)*w+x];
	    image[h*w2+y*w2+w+x] = t[(h-y-1)*w+w-x-1];
	}
    }
    *texwid = w2;
    *texht = h2;
    *tex = image;
}
