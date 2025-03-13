#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glut.h>


/* Materials */

GLfloat ambient[][4] =
{
    {0.329412f, 0.223529f, 0.027451f, 1.0f},
    {0.2125f, 0.1275f, 0.054f, 1.0f},
    {0.25f, 0.148f, 0.06475f, 1.0f},
    {0.25f, 0.25f, 0.25f, 1.0f},
    {0.19125f, 0.0735f, 0.0225f, 1.0f},
    {0.2295f, 0.08825f, 0.0275f, 1.0f},
    {0.24725f, 0.19995f, 0.0745f, 1.0f},
    {0.24725f, 0.2245f, 0.0645f, 1.0f},
    {0.105882f, 0.058824f, 0.113725f, 1.0f},
    {0.19225f, 0.19225f, 0.19225f, 1.0f},
    {0.23125f, 0.23125f, 0.23125f, 1.0f},
    {0.0215f, 0.1745f, 0.0215f, 0.55f},
    {0.135f, 0.2225f, 0.1575f, 0.95f},
    {0.05375f, 0.05f, 0.06625f, 0.82f},
    {0.25f, 0.20725f, 0.20725f, 0.922f},
    {0.1745f, 0.01175f, 0.01175f, 0.55f},
    {0.1f, 0.18725f, 0.1745f, 0.8f},
    {0.0f, 0.0f, 0.0f, 1.0f},
    {0.02f, 0.02f, 0.02f, 1.0f},
    {0.02f, 0.52f, 0.02f, 1.0f}};

GLfloat diffuse[][4] =
{
    {0.780392f, 0.568627f, 0.113725f, 1.0f},
    {0.714f, 0.4284f, 0.18144f, 1.0f},
    {0.4f, 0.2368f, 0.1036f, 1.0f},
    {0.4f, 0.4f, 0.4f, 1.0f},
    {0.7038f, 0.27048f, 0.0828f, 1.0f},
    {0.5508f, 0.2118f, 0.066f, 1.0f},
    {0.75164f, 0.60648f, 0.22648f, 1.0f},
    {0.34615f, 0.3143f, 0.0903f, 1.0f},
    {0.427451f, 0.470588f, 0.541176f, 1.0f},
    {0.50754f, 0.50754f, 0.50754f, 1.0f},
    {0.2775f, 0.2775f, 0.2775f, 1.0f},
    {0.07568f, 0.61424f, 0.07568f, 0.55f},
    {0.54f, 0.89f, 0.63f, 0.95f},
    {0.18275f, 0.17f, 0.22525f, 0.82f},
    {1.0f, 0.829f, 0.829f, 0.922f},
    {0.61424f, 0.04136f, 0.04136f, 0.55f},
    {0.396f, 0.74151f, 0.69102f, 0.8f},
    {0.01f, 0.01f, 0.01f, 1.0f},
    {0.01f, 0.01f, 0.01f, 1.0f},
    {0.01f, 0.51f, 0.01f, 1.0f}};

GLfloat specular[][4] =
{
    {0.992157f, 0.941176f, 0.807843f, 1.0f},
    {0.393548f, 0.271906f, 0.166721f, 1.0f},
    {0.774597f, 0.458561f, 0.200621f, 1.0f},
    {0.774597f, 0.774597f, 0.774597f, 1.0f},
    {0.256777f, 0.137622f, 0.086014f, 1.0f},
    {0.580594f, 0.223257f, 0.0695701f, 1.0f},
    {0.628281f, 0.555802f, 0.366065f, 1.0f},
    {0.797357f, 0.723991f, 0.208006f, 1.0f},
    {0.333333f, 0.333333f, 0.521569f, 1.0f},
    {0.508273f, 0.508273f, 0.508273f, 1.0f},
    {0.773911f, 0.773911f, 0.773911f, 1.0f},
    {0.633f, 0.727811f, 0.633f, 0.55f},
    {0.316228f, 0.316228f, 0.316228f, 0.95f},
    {0.332741f, 0.328634f, 0.346435f, 0.82f},
    {0.296648f, 0.296648f, 0.296648f, 0.922f},
    {0.727811f, 0.626959f, 0.626959f, 0.55f},
    {0.297254f, 0.30829f, 0.306678f, 0.8f},
    {0.50f, 0.50f, 0.50f, 1.0f},
    {0.40f, 0.40f, 0.40f, 1.0f},
    {0.40f, 0.40f, 0.40f, 1.0f}};

GLfloat shininess[] =
{27.8974f, 25.6f, 76.8f, 76.8f, 12.8f, 51.2f, 51.2f,
 83.2f, 9.84615f, 51.2f, 89.6f, 76.8f, 12.8f, 38.4f,
 11.264f, 76.8f, 12.8f, 32.f, 10.f, 10.f};

void 
drawTeapot(int index)
{
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient[index]);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse[index]);
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular[index]);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess[index]);
    glCallList(1);
}

/* Display callback */
void 
cbDisplay(void)
{
    int i, j;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glTranslatef(-6.0, 7.0, 0.0);
    for (i = 0; i < 5; i++) {
	for (j = 0; j < 4; j++) {
	    drawTeapot(i * 4 + j);
	    glTranslatef(4.0, 0.0, 0.0);
	}
	glTranslatef(-16.0, -3.5, 0.0);
    }
    glPopMatrix();

    glutSwapBuffers();
}


/* Keyboard callback */
/*ARGSUSED1*/
void 
cbKeyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case '\033':
    case 'Q':
    case 'q':
	exit(0);
	break;

    default:
	return;
    }
    glutPostRedisplay();
}


void 
initOpenGL(void)
{
    GLfloat position[4] = {0.0f, 0.0f, 4.0f, 0.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glNewList(1, GL_COMPILE);
    glutSolidTeapot(1.0);
    glEndList();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-8.0, 8.0, -8.0, 8.0, -30, 30);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
}

int 
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(200, 200);
    glutInitWindowSize(400, 400);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(cbDisplay);
    glutKeyboardFunc(cbKeyboard);

    initOpenGL();
    glutMainLoop();
    return 0;
}
