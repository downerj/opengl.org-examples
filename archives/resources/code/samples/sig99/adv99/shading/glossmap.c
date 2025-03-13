/*
 *  glossmap.c
 *  Brad Grantham, 1999
 *
 *  Demonstrates how to use second pass and alpha map to draw a "gloss map";
 *  use texture to modulate the specular component on a surface.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"

/* Win32 math.h doesn't define M_PI. */
#ifdef _WIN32
#ifndef M_PI
#define M_PI 3.14159265
#endif
#endif

int 	winWidth, winHeight;

/*
 * Probably want to do the following before using as a floor.
 * glScalef(floorSize, 1.0f, floorSize);
 * glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
 * glTranslatef(-0.5f, -0.5f, 0.0f);
 */
void drawTessQuad(int slicesU, int slicesV)
{
    int i, j;
    float dx, dy;
    float x, y;

    dx = 1.0 / slicesU;
    dy = 1.0 / slicesV;
    y = 0; 
    for(j = 0; j < slicesV; j++)
    {
	x = 0;
	glBegin(GL_TRIANGLE_STRIP);
	for(i = 0; i < slicesU + 1; i++) {
	    glNormal3f(0.0f, 0.0f, 1.0f);
	    glTexCoord2f(x, y + dy);
	    glVertex2f(x, y + dy);
	    glNormal3f(0.0f, 0.0f, 1.0f);
	    glTexCoord2f(x, y);
	    glVertex2f(x, y);
	    x += dx;
	}
	glEnd();
	y += dy;
    }
}

void init(void)
{
    unsigned int *glossMap;
    int width, height, comps;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glMatrixMode(GL_PROJECTION);
    glFrustum(-.33, .33, -.33, .33, .5, 40);

    glMatrixMode(GL_MODELVIEW);
    gluLookAt(0, 10, 7, 0, 0, 0, 0, 1, 0);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glEnable(GL_NORMALIZE);

    glossMap = read_texture("../../data/gloss.rgb", &width, &height, &comps);
    gluBuild2DMipmaps(GL_TEXTURE_2D, 1, width, height, GL_RGBA,
        GL_UNSIGNED_BYTE, glossMap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR);
    /* should be GL_ALPHA texture */

    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 20.0f);
}

static GLfloat 	zeroVec[] = {0.0f, 0.0f, 0.0f, 1.0f};
static GLfloat 	black[] = {0.0f, 0.0f, 0.0f, 1.0f};

void setupLight(void)
{
    static GLfloat 	lightpos[] = {0.0f, 2.0f, -3.0f, 1.0f};
    static GLfloat	specular[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glPushMatrix();
    glTranslatef(lightpos[0], lightpos[1], lightpos[2]);
    glLightfv(GL_LIGHT0, GL_POSITION, zeroVec);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glDisable(GL_LIGHTING);
    glutSolidSphere(.2, 10, 10);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void drawFloor(void)
{
    glPushMatrix();
    glScalef(10.0f, 1.0f, 10.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glTranslatef(-0.5f, -0.5f, 0.0f);
    drawTessQuad(10, 10);
    glPopMatrix();
}

int drawSpecular = 1;
int drawUsingGlossMap = 1;

void redraw(void)
{
    GLfloat specularMtl[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat diffuseMtl[4] = {0.5f, 0.5f, 0.5f, 1.0f};

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    
    setupLight();

    /* draw floor with no specular and no texture */
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuseMtl);
    drawFloor();

    /* draw floor with gloss alpha texture and specular */
    if(drawSpecular) {
	glDepthFunc(GL_EQUAL);
	glEnable(GL_BLEND);
	if(drawUsingGlossMap)
	    glEnable(GL_TEXTURE_2D);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularMtl);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, black);
	drawFloor();
	if(drawUsingGlossMap)
	    glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	glDepthFunc(GL_LEQUAL);
    }

    glPopMatrix();

    glutSwapBuffers();
}

void reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    winWidth = width;
    winHeight = height;
    glutPostRedisplay();
}

static int ox, oy;
static int mode;

/*ARGSUSED*/
void button(int b, int state, int x, int y)
{
    ox = x;
    oy = y;
    mode = b;
}


void motion(int x, int y)
{
    static float ang = 0;
    static float height = 10;
    float eyex, eyez;

    if(mode == GLUT_LEFT_BUTTON)
    {
        ang += (x - ox) / 512.0 * M_PI;
        height += (y - oy) / 512.0 * 10;
	eyex = sin(-ang) * 7;
	eyez = cos(-ang) * 7;
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(eyex, height, eyez, 0, 0, 0, 0, 1, 0);
        glutPostRedisplay();
	ox = x;
	oy = y;
    }
}

/*ARGSUSED1*/
void keyboard(unsigned char key, int x, int y)
{
    switch(key) {
	case 'q': case 'Q': case '\033':
	    exit(0);
	    break;

	default:
	    fprintf(stderr, "Push right mouse button for menu\n");
	    break;
    }
}

int mainMenu;

void mainMenuFunc(int entry)
{
    glutSetMenu(mainMenu);
    if(entry == 1){
	drawUsingGlossMap = !drawUsingGlossMap;
	glutChangeToMenuEntry(1, drawUsingGlossMap ? "Turn off glossmap" :
	    "Turn on glossmap", 1);
	glutPostRedisplay();
    }
    if(entry == 2){
	drawSpecular = !drawSpecular;
	glutChangeToMenuEntry(2, drawSpecular ? "Show specular and diffuse" :
	    "Show only diffuse lighting", 2);
	glutPostRedisplay();
    }
}

int main(int argc, char **argv)
{

    glutInitWindowSize(winWidth = 512, winHeight = 512);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
    (void)glutCreateWindow("Gloss Map");
    glutDisplayFunc(redraw);
    glutKeyboardFunc(keyboard);
    glutMotionFunc(motion);
    glutMouseFunc(button);
    glutReshapeFunc(reshape);

    mainMenu = glutCreateMenu(mainMenuFunc);
    glutAddMenuEntry("Turn off glossmap", 1);
    glutAddMenuEntry("Show only diffuse lighting", 2);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    init();
    glutMainLoop();

    return 0;
}
