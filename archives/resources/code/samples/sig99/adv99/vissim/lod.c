#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if((error = glGetError()) != GL_NO_ERROR)                      \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

GLboolean dblbuf = GL_TRUE;

enum {LOD, DIST_AND_LOD};
enum {SWITCH, BLEND, MORPH, EXIT};

GLfloat lod = 0.f, dist = 0.f;
int active;

GLint winHeight = 512, winWidth = 512;

void
reshape(int wid, int ht)
{
    winWidth = wid;
    winHeight = ht;
    glViewport(0, 0, wid, ht);
}


/*ARGSUSED*/
void
motion(int x, int y)
{
    lod = (winHeight - y) * 1.f/winHeight;
    if(lod > 1.f)
	lod = 1.f;
    if(lod < 0.f)
	lod = 0.f;
    switch(active)
    {
    case LOD:
	glutPostRedisplay();
	break;
    case DIST_AND_LOD:
	dist = 2000 * lod;
	glutPostRedisplay();
	break;
    }
    CHECK_ERROR("motion");
}

void
mouse(int button, int state, int x, int y)
{
    /* hack for 2 button mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;

    if(state == GLUT_DOWN)
	switch(button) {
	case GLUT_LEFT_BUTTON: /* move the light */
	    active = LOD;
	    motion(x, y);
	    break;
	case GLUT_MIDDLE_BUTTON:
	    active = DIST_AND_LOD;
	    motion(x, y);
	    break;
	case GLUT_RIGHT_BUTTON: /* menu: never happens */
	    break;
	}
}

/* morph between a cube and a pyramid 0 = cube */
/* objects are centered about origin (sort of) */
/* right hand winding points out from center */
/* TODO: if-tests for lod == 0 */
void
morph_obj(GLfloat size, GLfloat lod)
{
    glBegin(GL_QUADS);
    /* base of square (doesn't change) */
    glNormal3f(0.f, 0.f, -1.f);
    glVertex3f(-size, -size, -size);
    glVertex3f(-size,  size, -size);
    glVertex3f( size,  size, -size);
    glVertex3f( size, -size, -size);

    /* top (morphs to point) */
    glNormal3f(0.f, 0.f, 1.f);
    glVertex3f(-size * lod, -size * lod, size);
    glVertex3f( size * lod, -size * lod, size);
    glVertex3f( size * lod,  size * lod, size);
    glVertex3f(-size * lod,  size * lod, size);
    
    /* front (morphs to triangle) */
    glNormal3f(0.f, -1.f, lod/2.f);
    glVertex3f(-size, -size, -size);
    glVertex3f( size, -size, -size);
    glVertex3f( size * lod, -size * lod,  size);
    glVertex3f(-size * lod, -size * lod,  size);

    /* back (morphs to triangle) */
    glNormal3f(0.f,  1.f, lod/2.f);
    glVertex3f( size,  size, -size);
    glVertex3f(-size,  size, -size);
    glVertex3f(-size * lod,  size * lod,  size);
    glVertex3f( size * lod,  size * lod,  size);

    /* right (morphs to triangle) */
    glNormal3f(1.f,  0.f, lod/2.f);
    glVertex3f( size, -size, -size);
    glVertex3f( size,  size, -size);
    glVertex3f( size * lod,  size * lod,  size);
    glVertex3f( size * lod, -size * lod,  size);

    /* left (morphs to triangle) */
    glNormal3f(-1.f,  0.f, lod/2.f);
    glVertex3f(-size,  size, -size);
    glVertex3f(-size, -size, -size);
    glVertex3f(-size * lod, -size * lod,  size);
    glVertex3f(-size * lod,  size * lod,  size);

    glEnd();

}

void
draw_obj(GLfloat lod, GLfloat alpha)
{
    lod = 1.f - lod;

    glColor4f(1.f, 1.f, 1.f, alpha);
    glPolygonMode(GL_FRONT, GL_LINE);
    morph_obj(50.f, lod);

    glEnable(GL_POLYGON_OFFSET_FILL);

    glColor4f(1.f, .5f, 0.f, alpha);
    glPolygonMode(GL_FRONT, GL_FILL);
    morph_obj(50.f, lod);

    glDisable(GL_POLYGON_OFFSET_FILL);
}

/* Switch between LOD levels */
void redraw_switch(void)
{

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glTranslatef(-0.f, -0.f, -420.f - dist);
    glRotatef( 20.f, 0.f, 1.f, 0.f);
    glRotatef(-70.f, 1.f, 0.f, 0.f);

    if(lod > .5f)
	draw_obj(1.f, 1.f);
    else
	draw_obj(0.f, 1.f);

    glPopMatrix();

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw()");
}


/* Blend between LOD levels */
void redraw_blend(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glTranslatef(-0.f, -0.f, -420.f - dist);
    glRotatef( 20.f, 0.f, 1.f, 0.f);
    glRotatef(-70.f, 1.f, 0.f, 0.f);

    /* if tests to avoid unnecessary blending */
    if(lod == 0.f)
	draw_obj(0.f, lod);	
    else if(lod == 1.f)
	draw_obj(1.f, lod);	
    else {
	draw_obj(1.f, lod);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClear(GL_DEPTH_BUFFER_BIT); /* to avoid 1st object blocking 2nd */
	draw_obj(0.f, 1.f - lod);
	glDisable(GL_BLEND);
    }

    glPopMatrix();
    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw()");
}


/* Morph between LOD levels */
void redraw_morph(void)
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glTranslatef(-0.f, -0.f, -420.f - dist);
    glRotatef( 20.f, 0.f, 1.f, 0.f);
    glRotatef(-70.f, 1.f, 0.f, 0.f);

    draw_obj(lod, 1.f);

    glPopMatrix();

    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 

    CHECK_ERROR("OpenGL Error in redraw()");
}




/*ARGSUSED*/
void key(unsigned char key, int x, int y)
{
    switch(key) {
    case 's':
    case 'S':
	printf("Switch LOD\n");
	glutDisplayFunc(redraw_switch);
	glutPostRedisplay();
	break;
    case 'b':
    case 'B':
	printf("Blend LOD\n");
	glutDisplayFunc(redraw_blend);
	glutPostRedisplay();
	break;
    case 'm':
    case 'M':
	printf("Morph LOD\n");
	glutDisplayFunc(redraw_morph);
	glutPostRedisplay();
	break;
    case '\033':
	exit(0);
    default:
	fprintf(stderr, "Keyboard Commands:\n"
		"switch LODs (s, S)\n"
		"blend LODs  (b, B)\n"
		"morph LODs  (m, M)\n"
		"exit        <escape key>\n\n");
	break;
    }
}

void
menu(int choice)
{
    switch(choice) {
    case SWITCH:
	key('s', 0, 0);
	break;
    case BLEND:
	key('b', 0, 0);
	break;
    case MORPH:
	key('m', 0, 0);
	break;
    case EXIT:
	exit(0);
	break;
    }
}


/* Parse arguments, and set up interface between OpenGL and window system */
int
main(int argc, char *argv[])
{
    static GLfloat lightpos[] = {50.f, 100.f, -320.f, 1.f};

    glutInit(&argc, argv);
    glutInitWindowSize(512, 512);
    if(argc > 1) {
	char *args = argv[1];
	GLboolean done = GL_FALSE;
	while(!done) {
	    switch(*args) {
	    case 's': /* single buffer */
	    case 'S': /* single buffer */
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

    (void)glutCreateWindow("Geometric LODs");
    glutReshapeFunc(reshape);
    glutDisplayFunc(redraw_switch);
    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    (void)glutCreateMenu(menu);
    glutAddMenuEntry("Switch LODs('s', 'S')", SWITCH);
    glutAddMenuEntry("Blend LODs ('b', 'B')", BLEND);
    glutAddMenuEntry("Morph LODs ('m', 'M')", MORPH);
    glutAddMenuEntry("Exit (escape key)", EXIT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);


    glClearColor(.2f, .2f, .2f, 1.f);

    /* draw a perspective scene */
    glMatrixMode(GL_PROJECTION);
    glFrustum(-100., 100., -100., 100., 320., 3000.); 
    glMatrixMode(GL_MODELVIEW);

    /* turn on features */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* place light 0 in the right place */
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    glPolygonOffset(1.f, 1.f);

    /* remove back faces to speed things up */
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL); 

    /* morphing object doesn't have normalized normals */
    glEnable(GL_NORMALIZE);

    key('?', 0, 0);
    glutMainLoop();
    return 0;
}
