#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include "../util/texture.h"

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if((error = glGetError()) != GL_NO_ERROR)                      \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

int dblbuf = GL_TRUE;
GLfloat floorplane[4], lwallplane[4];


enum {SPHERE = 1, CONE, LIGHT, LEFTWALL, FLOOR, WALLS, SHADOWERS};
enum {MOVE_LIGHT_XY, MOVE_LIGHT_Z};

enum {X, Y, Z, W};
enum {A, B, C, D};
/* create a matrix that will project the desired shadow */
void
shadowmatrix(GLfloat shadowMat[4][4],
	     GLfloat groundplane[4],
	     GLfloat lightpos[4])
{
  GLfloat dot;

  /* find dot product between light position vector and ground plane normal */
  dot = groundplane[X] * lightpos[X] +
        groundplane[Y] * lightpos[Y] +
        groundplane[Z] * lightpos[Z] +
        groundplane[W] * lightpos[W];

  shadowMat[0][0] = dot - lightpos[X] * groundplane[X];
  shadowMat[1][0] = 0.f - lightpos[X] * groundplane[Y];
  shadowMat[2][0] = 0.f - lightpos[X] * groundplane[Z];
  shadowMat[3][0] = 0.f - lightpos[X] * groundplane[W];

  shadowMat[X][1] = 0.f - lightpos[Y] * groundplane[X];
  shadowMat[1][1] = dot - lightpos[Y] * groundplane[Y];
  shadowMat[2][1] = 0.f - lightpos[Y] * groundplane[Z];
  shadowMat[3][1] = 0.f - lightpos[Y] * groundplane[W];

  shadowMat[X][2] = 0.f - lightpos[Z] * groundplane[X];
  shadowMat[1][2] = 0.f - lightpos[Z] * groundplane[Y];
  shadowMat[2][2] = dot - lightpos[Z] * groundplane[Z];
  shadowMat[3][2] = 0.f - lightpos[Z] * groundplane[W];

  shadowMat[X][3] = 0.f - lightpos[W] * groundplane[X];
  shadowMat[1][3] = 0.f - lightpos[W] * groundplane[Y];
  shadowMat[2][3] = 0.f - lightpos[W] * groundplane[Z];
  shadowMat[3][3] = dot - lightpos[W] * groundplane[W];

}

/* find the plane equation given 3 points */
void
findplane(GLfloat plane[4],
	  GLfloat v0[3], GLfloat v1[3], GLfloat v2[3])
{
  GLfloat vec0[3], vec1[3];

  /* need 2 vectors to find cross product */
  vec0[X] = v1[X] - v0[X];
  vec0[Y] = v1[Y] - v0[Y];
  vec0[Z] = v1[Z] - v0[Z];

  vec1[X] = v2[X] - v0[X];
  vec1[Y] = v2[Y] - v0[Y];
  vec1[Z] = v2[Z] - v0[Z];
  
  /* find cross product to get A, B, and C of plane equation */
  plane[A] = vec0[Y] * vec1[Z] - vec0[Z] * vec1[Y];
  plane[B] = -(vec0[X] * vec1[Z] - vec0[Z] * vec1[X]);
  plane[C] = vec0[X] * vec1[Y] - vec0[Y] * vec1[X];
  
  plane[D] = -(plane[A] * v0[X] + plane[B] * v0[Y] + plane[C] * v0[Z]);
}

int winwid, winht;

void
reshape(int wid, int ht)
{
    winwid = wid;
    winht = ht;
    glViewport(0, 0, wid, ht);
}



GLfloat leftwallshadow[4][4];
GLfloat floorshadow[4][4];

GLfloat lightpos[] = {50.f, 50.f, -320.f, 1.f};
int active;

void
motion(int x, int y)
{

    switch(active)
    {
    case MOVE_LIGHT_XY:
    {
	lightpos[X]  =  100.f + (x - winwid) * 200.f/winwid;
	lightpos[Y]  = -100.f + (winht - y) * 200.f/winht;
	shadowmatrix(leftwallshadow, lwallplane, lightpos);
	shadowmatrix(floorshadow, floorplane, lightpos);
	/* place light 0 in the right place */
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	glutPostRedisplay();
	break;
    }
    case MOVE_LIGHT_Z:
	lightpos[Z]  = -320.f - (winht - y) * 200.f/winht;
	shadowmatrix(leftwallshadow, lwallplane, lightpos);
	shadowmatrix(floorshadow, floorplane, lightpos);
	/* place light 0 in the right place */
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
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
	    active = MOVE_LIGHT_XY;
	    motion(x, y);
	    break;
	case GLUT_MIDDLE_BUTTON:
	    active = MOVE_LIGHT_Z;
	    motion(x, y);
	    break;
	}
}



void
draw_shadowed_tex(GLint shadowed, GLint shadowers, GLfloat (*xform)[4])
{

    glEnable(GL_STENCIL_TEST);

    /* draw shadowed polygon lit and into stencil buffer */
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glCallList(shadowed);

    glDisable(GL_DEPTH_TEST); /* to avoid z-fighting */
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
    glPushMatrix();
    glMultMatrixf((GLfloat*)xform); /* projection transform */

    glCallList(shadowers); /* clear stencil on polygon */

    glPopMatrix();
    
    /* draw shadowed polygon again, unlit where stencil is cleared */

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_LIGHT0); /* draw shadow with only ambient lighting */
    glStencilFunc(GL_EQUAL, 0, 1); /*only draw where stencil cleared by shadow*/

    glCallList(shadowed);

    glEnable(GL_LIGHT0);
    glDepthFunc(GL_LESS);
    glDisable(GL_STENCIL_TEST);
}

void
draw_shadowed(GLint shadowed, GLint shadowers, GLfloat (*xform)[4])
{

    glEnable(GL_STENCIL_TEST);

    /* draw shadowed polygon lit and into stencil buffer */
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glCallList(shadowed);

    glDisable(GL_DEPTH_TEST); /* to avoid z-fighting */
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
    glPushMatrix();
    glMultMatrixf((GLfloat*)xform); /* projection transform */

    glCallList(shadowers); /* clear stencil on polygon */

    glPopMatrix();
    
    /* draw shadowed polygon again, unlit where stencil is cleared */

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_LIGHTING); /* draw shadow with only ambient lighting */
    glStencilFunc(GL_EQUAL, 0, 1); /*only draw where stencil cleared by shadow*/

    glColor3f(0.f, 0.f, 0.f);
    glCallList(shadowed);

    glEnable(GL_LIGHTING);
    glDepthFunc(GL_LESS);
    glDisable(GL_STENCIL_TEST);
}

void
redraw(void)
{

    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    /*
    ** Note: wall verticies are ordered so they are all front facing
    ** this lets me do back face culling to speed things up.
    */
 
    glColor3f(1.f, 1.f, 1.f);

    glCallList(FLOOR);
    glCallList(LEFTWALL);
    glCallList(WALLS);

    glPushMatrix();
    glTranslatef(lightpos[X], lightpos[Y], lightpos[Z]);
    glDisable(GL_LIGHTING);
    glColor3f(.7f, .1f, .7f);
    glCallList(LIGHT);
    glEnable(GL_LIGHTING);
    glPopMatrix();


    glColor3f(0.f, .5f, 1.f);
    glCallList(CONE);
    glColor3f(1.f, .5f, 0.f);
    glCallList(SPHERE);

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("redraw_normal()");
}


void
redraw_shadow_black(void)
{
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);


    /* Floor with Shadow */

    glColor3f(1.f, 1.f, 1.f);
    draw_shadowed(FLOOR, SHADOWERS, floorshadow); /* draw shadowed floor */
    glColor3f(1.f, 1.f, 1.f);
    draw_shadowed(LEFTWALL, SHADOWERS, leftwallshadow);/* draw shadowed floor*/

    /* Unshadowed Surfaces */

    glCallList(WALLS);

    glPushMatrix();
    glTranslatef(lightpos[X], lightpos[Y], lightpos[Z]);
    glDisable(GL_LIGHTING);
    glColor3f(.7f, .1f, .7f);
    glCallList(LIGHT);
    glEnable(GL_LIGHTING);
    glPopMatrix();


    glColor3f(0.f, .5f, 1.f);
    glCallList(CONE);
    glColor3f(1.f, .5f, 0.f);
    glCallList(SPHERE);

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("redraw_shadow_black()");
}


void
redraw_shadow_tex(void)
{
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);


    /* Floor with Shadow */

    glColor3f(1.f, 1.f, 1.f);
    draw_shadowed_tex(FLOOR, SHADOWERS, floorshadow);
    glColor3f(1.f, 1.f, 1.f);
    draw_shadowed_tex(LEFTWALL, SHADOWERS, leftwallshadow);

    /* Unshadowed Surfaces */

    glCallList(WALLS);

    glPushMatrix();
    glTranslatef(lightpos[X], lightpos[Y], lightpos[Z]);
    glDisable(GL_LIGHTING);
    glColor3f(.7f, .1f, .7f);
    glCallList(LIGHT);
    glEnable(GL_LIGHTING);
    glPopMatrix();


    glColor3f(0.f, .5f, 1.f);
    glCallList(CONE);
    glColor3f(1.f, .5f, 0.f);
    glCallList(SPHERE);

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("redraw_normal()");
}

enum {NONE, SHADOW_BLACK, SHADOW};

/*ARGSUSED1*/
void key(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 'n':
    case 'N':
	glutDisplayFunc(redraw);
	glutPostRedisplay();
	break;
    case 'b':
    case 'B':
	glutDisplayFunc(redraw_shadow_black);
	glutPostRedisplay();
	break;
    case 's':
    case 'S':
	glutDisplayFunc(redraw_shadow_tex);
	glutPostRedisplay();
	break;
    case '\033':
	exit(0);
    }
}

void
menu(int choice)
{
    switch(choice)
    {
    case NONE:
	key('n', 0, 0);
	break;
    case SHADOW_BLACK:
	key('b', 0, 0);
	break;
    case SHADOW:
	key('s', 0, 0);
	break;
    }
}




/* Parse arguments, and set up interface between OpenGL and window system */
int
main(int argc, char *argv[])
{
    GLUquadricObj *sphere, *cone, *base;
    GLfloat v0[3], v1[3], v2[3];
    unsigned *floortex;
    int texcomps, texwid, texht;

    glutInit(&argc, argv);
    glutInitWindowSize(512, 512);
    if(argc > 1)
    {
	char *args = argv[1];
	int done = GL_FALSE;
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
    if(dblbuf)
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL|GLUT_DOUBLE);
    else
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL);

    (void)glutCreateWindow("projection shadows");
    glutDisplayFunc(redraw);
    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutReshapeFunc(reshape);

    glutCreateMenu(menu);
    glutAddMenuEntry("No Shadows", NONE);
    glutAddMenuEntry("Black Shadows", SHADOW_BLACK);
    glutAddMenuEntry("Shadows on Texture", SHADOW);
    glutAttachMenu(GLUT_RIGHT_BUTTON);


    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-100., 100., -100., 100., 320., 640.); 
    glMatrixMode(GL_MODELVIEW);

    /* turn on features */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* make shadow matricies */

      /* 3 points on floor */
      v0[X] = -100.f; v0[Y] = -100.f; v0[Z] = -320.f;
      v1[X] =  100.f; v1[Y] = -100.f; v1[Z] = -320.f;
      v2[X] =  100.f; v2[Y] = -100.f; v2[Z] = -520.f;
      
      findplane(floorplane, v0, v1, v2);
      shadowmatrix(floorshadow, floorplane, lightpos);

      /* 3 points on left wall */
      v0[X] = -100.f; v0[Y] = -100.f; v0[Z] = -320.f;
      v1[X] = -100.f; v1[Y] = -100.f; v1[Z] = -520.f;
      v2[X] = -100.f; v2[Y] =  100.f; v2[Z] = -520.f;

      findplane(lwallplane, v0, v1, v2);
      shadowmatrix(leftwallshadow, lwallplane, lightpos);

    /* place light 0 in the right place */
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    /* remove back faces to speed things up */
    glCullFace(GL_BACK);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    /* make display lists for sphere and cone; for efficiency */

    glNewList(WALLS, GL_COMPILE);
    glColor3f(1.f, 1.f, 1.f);
    glBegin(GL_QUADS);
    /* right wall */
    glNormal3f(-1.f, 0.f, 0.f);
    glVertex3f( 100.f, -100.f, -320.f);
    glVertex3f( 100.f,  100.f, -320.f);
    glVertex3f( 100.f,  100.f, -520.f);
    glVertex3f( 100.f, -100.f, -520.f);

    /* ceiling */
    glNormal3f(0.f, -1.f, 0.f);
    glVertex3f(-100.f,  100.f, -320.f);
    glVertex3f(-100.f,  100.f, -520.f);
    glVertex3f( 100.f,  100.f, -520.f);
    glVertex3f( 100.f,  100.f, -320.f);

    /* back wall */
    glNormal3f(0.f, 0.f, 1.f);
    glVertex3f(-100.f, -100.f, -520.f);
    glVertex3f( 100.f, -100.f, -520.f);
    glVertex3f( 100.f,  100.f, -520.f);
    glVertex3f(-100.f,  100.f, -520.f);
    glEnd();
    glEndList();


    glNewList(SPHERE, GL_COMPILE);
    sphere = gluNewQuadric();
    glPushMatrix();
    glTranslatef(60.f, -50.f, -360.f);
    gluSphere(sphere, 20.f, 20, 20);
    gluDeleteQuadric(sphere);
    glPopMatrix();
    glEndList();


    glNewList(LIGHT, GL_COMPILE);
    sphere = gluNewQuadric();
    gluSphere(sphere, 5.f, 20, 20);
    gluDeleteQuadric(sphere);
    glEndList();

    glNewList(CONE, GL_COMPILE);
    cone = gluNewQuadric();
    base = gluNewQuadric();
    glPushMatrix();
    glTranslatef(-40.f, -40.f, -400.f);
    glRotatef(-90.f, 1.f, 0.f, 0.f);
    gluDisk(base, 0., 20., 20, 1);
    gluCylinder(cone, 20., 0., 60., 20, 20);
    gluDeleteQuadric(cone);
    gluDeleteQuadric(base);
    glPopMatrix();
    glEndList();


    glNewList(SHADOWERS, GL_COMPILE);
    glCallList(CONE);
    glCallList(SPHERE);
    glEndList();

    glNewList(FLOOR, GL_COMPILE);
    /* make the floor textured */
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glNormal3f(0.f, 1.f, 0.f);
    glTexCoord2i(0, 0);
    glVertex3f(-100.f, -100.f, -320.f);
    glTexCoord2i(1, 0);
    glVertex3f( 100.f, -100.f, -320.f);
    glTexCoord2i(1, 1);
    glVertex3f( 100.f, -100.f, -520.f);
    glTexCoord2i(0, 1);
    glVertex3f(-100.f, -100.f, -520.f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glEndList();

    glNewList(LEFTWALL, GL_COMPILE);
    glBegin(GL_QUADS);
    /* left wall */
    glNormal3f(1.f, 0.f, 0.f);
    glVertex3f(-100.f, -100.f, -320.f);
    glVertex3f(-100.f, -100.f, -520.f);
    glVertex3f(-100.f,  100.f, -520.f);
    glVertex3f(-100.f,  100.f, -320.f);
    glEnd();
    glEndList();

    floortex = read_texture("../../data/wood0.rgb",
			    &texwid, &texht, &texcomps);

    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, texwid, texht, GL_RGBA,
		      GL_UNSIGNED_BYTE, floortex);

    free(floortex);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    glutMainLoop();
    return 0;
}
