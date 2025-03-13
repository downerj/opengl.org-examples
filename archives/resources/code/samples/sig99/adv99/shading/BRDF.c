#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#ifdef WIN32
#ifndef M_PI
#define M_PI	3.14159265f
#endif
#endif

int px, py, moving = -1;
float rtheta = 2.62f, rphi = 1.01f, distance = 1.5f;
int drawMode = GL_LINE, showAmbient = 1, showDiffuse = 1, showSpecular = 1, useHdotN = 0;
GLfloat position0[4] = {3.0f, 3.0f, 3.0f, 1.0f};
GLfloat position1[4] = {-5.0f, -1.0f, 2.0f, 1.0f};
GLfloat ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
GLfloat diffuse[4] = {0.5f, 0.5f, 0.5f, 1.0f};
GLfloat white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
GLfloat black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
GLfloat shininess = 27.0f;

#define phi_divs	32
#define theta_divs	128
float brdf[theta_divs][phi_divs];

/*
 * Display callback 
 */
void 
cbDisplay(void)
{
    int i, j;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(distance * sin(rphi) * cos(rtheta), distance * sin(rphi) * sin(rtheta), distance * cos(rphi) + 1,
	      0.0, 0.0, 1.0, 0.0, 0.0, 1.0);
    glLightfv(GL_LIGHT0, GL_POSITION, position0);
    glLightfv(GL_LIGHT1, GL_POSITION, position1);

    glEnable(GL_LIGHTING);
    glPolygonMode(GL_FRONT, GL_FILL);
    glutSolidSphere(1.0, 128, 128);
    glDisable(GL_LIGHTING);

    glColor3f(1.0, 1.0, 0.0);
    glLineWidth(3.0);
    if (glIsEnabled(GL_LIGHT0)) {
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 1.0);
	glVertex3fv(position0);
	glEnd();
    }
    if (glIsEnabled(GL_LIGHT1)) {
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 1.0);
	glVertex3fv(position1);
	glEnd();
    }
    glLineWidth(1.0);
    glPolygonMode(GL_FRONT_AND_BACK, drawMode);
    glPushMatrix();
    glTranslatef(0.0, 0.0, 1.0);
    glScalef(0.5, 0.5, 0.5);
    for (i = 0; i < theta_divs; i++) {
	float st = sin(i * 2 * M_PI / theta_divs);
	float ct = cos(i * 2 * M_PI / theta_divs);
	float stp = sin(((i + 1) % theta_divs) * 2 * M_PI / theta_divs);
	float ctp = cos(((i + 1) % theta_divs) * 2 * M_PI / theta_divs);

	glBegin(GL_TRIANGLE_STRIP);
	for (j = 0; j < phi_divs; j++) {
	    float sp = sin(j * 0.5 * M_PI / phi_divs);
	    float cp = cos(j * 0.5 * M_PI / phi_divs);

	    glColor4f(brdf[i][j], brdf[i][j], brdf[i][j], 1.0);
	    glVertex3f(brdf[i][j] * sp * ct, brdf[i][j] * sp * st, brdf[i][j] * cp);
	    glColor4f(brdf[(i + 1) % theta_divs][j], brdf[(i + 1) % theta_divs][j], brdf[(i + 1) % theta_divs][j], 1.0);
	    glVertex3f(brdf[(i + 1) % theta_divs][j] * sp * ctp, brdf[(i + 1) % theta_divs][j] * sp * stp, brdf[(i + 1) % theta_divs][j] * cp);
	}
	glEnd();
    }
    glPopMatrix();

    glutSwapBuffers();
}

/*
 * Mouse button callback 
 */
void 
cbMouse(int button, int state, int x, int y)
{
    if (moving == -1 && state == GLUT_DOWN) {
	moving = button;
	px = x;
	py = y;
    } else if (button == moving && state == GLUT_UP) {
	moving = -1;
    }
}

/*
 * Mouse motion callback 
 */
void 
cbMotion(int x, int y)
{
    switch (moving) {
    case GLUT_LEFT_BUTTON:
	rtheta -= 0.03 * (x - px);
	rphi -= 0.03 * (y - py);
	break;
    case GLUT_MIDDLE_BUTTON:
	distance *= pow(0.98, y - py);
	break;
    case GLUT_RIGHT_BUTTON:
    default:
	return;
    }
    px = x;
    py = y;
    glutPostRedisplay();
}

#ifndef _WIN32
float 
max(float a, float b)
{
    return (a > b) ? a : b;
}
#endif

float 
ambient0(void)
{
    return 0.2f;
}

float 
ambient1(void)
{
    return 0.0;
}

float 
diffuse0(void)
{
    float two_s22 = 2.0 / sqrt(22.0);

    return 0.5 * 1 * two_s22;
}

float 
diffuse1(void)
{
    float one_s27 = 1.0 / sqrt(27.0);

    return 0.5 * 1 * one_s27;
}

float 
specular0(float theta, float phi)
{
    float two_s22 = 2.0 / sqrt(22.0);
    float three_s22 = 3.0 / sqrt(22.0);
    float V[3], R[3], H2, VdotR;

    V[0] = sin(phi) * cos(theta);
    V[1] = sin(phi) * sin(theta);
    V[2] = cos(phi);

    R[0] = -three_s22;
    R[1] = -three_s22;
    R[2] = two_s22;

    /*
     * the V and R terms are really computing V + L 
     */
    H2 = (V[2] + R[2]) / sqrt((V[0] - R[0]) * (V[0] - R[0]) + (V[1] - R[1]) * (V[1] - R[1]) + (V[2] + R[2]) * (V[2] + R[2]));
    VdotR = V[0] * R[0] + V[1] * R[1] + V[2] * R[2];

    return max(1.0 * 1.0 * pow(useHdotN ? H2 : VdotR, shininess), 0);
}

float 
specular1(float theta, float phi)
{
    float one_s27 = 1.0 / sqrt(27.0);
    float five_s27 = 5.0 / sqrt(27.0);
    float V[3], R[3], H2, VdotR;

    V[0] = sin(phi) * cos(theta);
    V[1] = sin(phi) * sin(theta);
    V[2] = cos(phi);

    R[0] = five_s27;
    R[1] = one_s27;
    R[2] = one_s27;

    /*
     * the V and R terms are really computing V + L 
     */
    H2 = (V[2] + R[2]) / sqrt((V[0] - R[0]) * (V[0] - R[0]) + (V[1] - R[1]) * (V[1] - R[1]) + (V[2] + R[2]) * (V[2] + R[2]));
    VdotR = V[0] * R[0] + V[1] * R[1] + V[2] * R[2];

    return max(1.0 * 1.0 * pow(useHdotN ? H2 : VdotR, shininess), 0);
}

void 
calculate(void)
{
    int i, j;

    for (i = 0; i < theta_divs; i++) {
	float theta = i * 2 * M_PI / theta_divs;

	for (j = 0; j < phi_divs; j++) {
	    brdf[i][j] = 0.0;
	    if (showAmbient && glIsEnabled(GL_LIGHT0))
		brdf[i][j] += ambient0();
	    if (showDiffuse && glIsEnabled(GL_LIGHT0))
		brdf[i][j] += diffuse0();
	    if (showSpecular && glIsEnabled(GL_LIGHT0))
		brdf[i][j] += specular0(theta, j * 0.5 * M_PI / phi_divs);
	    if (showAmbient && glIsEnabled(GL_LIGHT0))
		brdf[i][j] += ambient1();
	    if (showDiffuse && glIsEnabled(GL_LIGHT1))
		brdf[i][j] += diffuse1();
	    if (showSpecular && glIsEnabled(GL_LIGHT1))
		brdf[i][j] += specular1(theta, j * 0.5 * M_PI / phi_divs);
	}
    }
}


/*
 * Keyboard callback 
 */
/*ARGSUSED1*/
void 
cbKeyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 27:
    case 'q':
	exit(0);

    case '0':
	if (glIsEnabled(GL_LIGHT0))
	    glDisable(GL_LIGHT0);
	else
	    glEnable(GL_LIGHT0);
	calculate();
	break;
    case '1':
	if (glIsEnabled(GL_LIGHT1))
	    glDisable(GL_LIGHT1);
	else
	    glEnable(GL_LIGHT1);
	calculate();
	break;
    case 'c':
    case 'C':
	useHdotN = !useHdotN;
	calculate();
	glutChangeToMenuEntry(9, useHdotN ? "Calculate with (V dot R) instead of (H dot N) ('c')" :
	     "Calculate with (H dot N) instead of (V dot R) ('c')", 'c');
	break;
    case 'a':
    case 'A':
	showAmbient = !showAmbient;
	calculate();
	break;
    case 'd':
    case 'D':
	showDiffuse = !showDiffuse;
	calculate();
	break;
    case 's':
    case 'S':
	showSpecular = !showSpecular;
	calculate();
	break;
    case 'w':
    case 'W':
	drawMode = (drawMode == GL_FILL) ? GL_LINE : GL_FILL;
	break;

    case '+':
	shininess *= 1.25f;
	calculate();
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);
	break;
    case '-':
	shininess *= 0.80f;
	calculate();
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);
	break;

    default:
	return;
    }

    glutPostRedisplay();
}

/*
 * Menu callback 
 */
void 
cbMenu(int option)
{
    cbKeyboard((unsigned char) option, 0, 0);
}

void 
init(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0, 1.3333, 0.1, 50.0);
    glMatrixMode(GL_MODELVIEW);

    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, white);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, white);
    glLightfv(GL_LIGHT1, GL_SPECULAR, white);
    glDisable(GL_LIGHT1);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.667f, 0.0f);
}


int 
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("BRDF");

    glutDisplayFunc(cbDisplay);
    glutKeyboardFunc(cbKeyboard);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);

    glutCreateMenu(cbMenu);
    glutAddMenuEntry("Toggle light 0 ('0')", '0');
    glutAddMenuEntry("Toggle light 1 ('1')", '1');
    glutAddMenuEntry("Toggle ambient componant of BRDF ('a')", 'a');
    glutAddMenuEntry("Toggle diffuse componant of BRDF ('d')", 'd');
    glutAddMenuEntry("Toggle specular componant of BRDF ('s')", 's');
    glutAddMenuEntry("Increase material shininess ('+')", '+');
    glutAddMenuEntry("Decrease material shininess ('-')", '-');
    glutAddMenuEntry("Toggle wireframe ('w')", 'w');
    glutAddMenuEntry("Calculate with (H dot N) instead of (V dot R) ('c')", 'c');
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    init();
    calculate();
    glutMainLoop();
    return 0;
}
