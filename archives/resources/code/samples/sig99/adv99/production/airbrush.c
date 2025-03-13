#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include "texture.h"

/* For platform-independent timing */
#ifdef _WIN32
    #include <windows.h>
    typedef int timer;
    #define getTime(a)      (a = GetTickCount())
    #define timeDiff(a, b)  ((b - a) * 1000)
#else
    #include <sys/time.h>
    typedef struct timeval timer;
    #define getTime(a)      gettimeofday(&a, NULL)
    #define timeDiff(a, b)  (((b.tv_sec - a.tv_sec) * 1000000) + b.tv_usec - a.tv_usec)
#endif


#define brushSize 128
GLuint *brush1, *brush2, *brush3, *currentBrush;

int     clearWindow = 1, moving = 0;
int     mouseX, mouseY;
timer   last;

/* Make the brush images */
void createBrushes(void)
{
    int w, h, comp;

    brush1 = (GLuint *)read_texture("../../data/mandrill_small.rgb", &w, &h, &comp);
    brush2 = (GLuint *)read_texture("../../data/flowers.rgb", &w, &h, &comp);
    brush3 = (GLuint *)read_texture("../../data/sea.rgb", &w, &h, &comp);

    /* Set the alpha on the images to be pretty low */
    for (h=0; h<brushSize; h++)
        for (w=0; w<brushSize; w++) {
            ((unsigned char *)brush1)[4*(brushSize*h+w)+3] = 8;
            ((unsigned char *)brush2)[4*(brushSize*h+w)+3] = 8;
            ((unsigned char *)brush3)[4*(brushSize*h+w)+3] = 8;
        }

    currentBrush = brush1;
}

/* Display callback */
void cbDisplay(void)
{
    /* Clear the window if desired */
    if (clearWindow) {
        glClear(GL_COLOR_BUFFER_BIT);
        glFlush();
        clearWindow = 0;
    }
}

/* Mouse button callback */
void cbMouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON) {
        moving = (state == GLUT_DOWN);
        mouseX = x;
        mouseY = y;

        if (moving) getTime(last);
    }
}

/* Mouse motion callback */
void cbMotion(int x, int y)
{
    mouseX = x;
    mouseY = y;
}

/* Idle callback */
void cbIdle(void)
{
    timer current;

    if (moving) {
        getTime(current);

        if (timeDiff(last, current) > 1000) {
            glRasterPos2f(mouseX - brushSize/2, mouseY + brushSize/2);
            glDrawPixels(brushSize, brushSize, GL_RGBA, GL_UNSIGNED_BYTE, currentBrush);    
            glutPostRedisplay();
            getTime(last);
        }
    }
}

/* Keyboard callback */
/*ARGSUSED1*/
void cbKeyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 27:
    case 'q':
    case 'Q':   exit(0);
		break;
    case 'c':
    case 'C':   clearWindow = 1;
		break;
    case '1':   currentBrush = brush1;
		break;
    case '2':   currentBrush = brush2;
		break;
    case '3':   currentBrush = brush3;
		break;

    default:    return;
    }
    glutPostRedisplay();
}

/* Menu callback */
void cbMenu(int option)
{
    clearWindow = 1;
    cbKeyboard((unsigned char)option, 0, 0);
}

void init(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 512.0, 512.0, 0.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    createBrushes();
}

int
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
    glutInitWindowSize(512, 512);
	glutInitWindowPosition(100, 100);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(cbDisplay);
    glutKeyboardFunc(cbKeyboard);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);
    glutIdleFunc(cbIdle);

    glutCreateMenu(cbMenu);
    glutAddMenuEntry("Mandrill brush ('1')", '1');
    glutAddMenuEntry("Flowers brush ('2')", '2');
    glutAddMenuEntry("Cloudy brush ('3')", '3');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    printf("Due to the way Windows handles menus, this program clears the window if you\n");
    printf("change brushes with the menu.  To avoid clearing, use the keyboard commands.\n");

    init();
    glutMainLoop();
    return 0;
}
