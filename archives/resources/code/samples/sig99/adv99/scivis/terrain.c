#include <stdlib.h>
#include <stdio.h>
#include "math.h"
#include <GL/glut.h>
#include "noise.h"
#ifdef _WIN32
#define sinf(x) ((float)sin((x)))
#define cosf(x) ((float)cos((x)))
#define sqrtf(x) ((float)(sqrt(x)))
#define floorf(x) ((float)(floor(x)))
#ifndef M_PI
#define M_PI 3.14159265
#endif
#endif

static float altitudeScale = 1.f;
static float transx = 1.0, transy, rotx = 20, roty = -35;
static int ox = -1, oy = -1;
static int mot = 0;
#define PAN	1
#define ROT	2

void
pan(const int x, const int y) {
    transx +=  (x-ox)/5.;
    transy -= (y-oy)/5.;
    ox = x; oy = y;
    glutPostRedisplay();
}

void
rotate(const int x, const int y) {
    rotx += (y-oy) / 2.;
    if (rotx > 360.) rotx -= 360.;
    else if (rotx < -360.) rotx += 360.;
    roty += (x-ox) / 2.0;
    if (roty > 360.) roty -= 360.;
    else if (roty < -360.) roty += 360.;
    ox = x; oy = y;
    glutPostRedisplay();
}

void
motion(int x, int y) {
    if (mot == PAN) pan(x, y);
    else if (mot == ROT) rotate(x,y);
}

void
mouse(int button, int state, int x, int y) {

    /* hack for 2 button mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	button = GLUT_MIDDLE_BUTTON;
    
	if(state == GLUT_DOWN) {
	switch(button) {
	case GLUT_LEFT_BUTTON:
	    mot = PAN;
	    motion(ox = x, oy = y);
	    break;
	case GLUT_MIDDLE_BUTTON:
	    mot = ROT;
	    motion(ox = x, oy = y);
	    break;
	case GLUT_RIGHT_BUTTON:
	    break;
	}
    } else if (state == GLUT_UP) {
	mot = 0;
    }
}

#define	stripeImageWidth 32
GLubyte stripeImage[4*stripeImageWidth];

void makeStripeImage(void) {
    int j;
    
    for (j = 0; j < stripeImageWidth; j++) {
	stripeImage[4*j] = (GLubyte) ((j<=4) ? 255 : 0);
	stripeImage[4*j+1] = (GLubyte) ((j>4) ? 255 : 0);
	stripeImage[4*j+2] = (GLubyte) 0;
	stripeImage[4*j+3] = (GLubyte) 255;
    }
}

void
hsv_to_rgb(float h,float s,float v,float *r,float *g,float *b)
{
    int i;
    float f, p, q, t;

    h *= 360.0;
    if (s==0) {
	*r = v;
	*g = v;
	*b = v;
    } else {
	if (h==360) 
	    h = 0;
	h /= 60;
	i = floorf(h);
	f = h - i;
	p = v*(1.0-s);
	q = v*(1.0-(s*f));
	t = v*(1.0-(s*(1.0-f)));
	switch (i) {
	    case 0 : 
		*r = v;
		*g = t;
		*b = p;
		break;
	    case 1 : 
		*r = q;
		*g = v;
		*b = p;
		break;
	    case 2 : 
		*r = p;
		*g = v;
		*b = t;
		break;
	    case 3 : 
		*r = p;
		*g = q;
		*b = v;
		break;
	    case 4 : 
		*r = t;
		*g = p;
		*b = v;
		break;
	    case 5 : 
		*r = v;
		*g = p;
		*b = q;
		break;
	}
    }
}

GLubyte rainbow[4*stripeImageWidth];
void makeRainbow(void) {
    int j;
    for (j = 0; j < stripeImageWidth; j++) {
	float r, g, b;
        hsv_to_rgb((float)j/(stripeImageWidth-1.f), 1.0, 1.0, &r, &g, &b);
        rainbow[4*j] = r*255;
        rainbow[4*j+1] = g*255;
        rainbow[4*j+2] = b*255;
        rainbow[4*j+3] = (GLubyte) 255;
    }
}


GLubyte height[4*stripeImageWidth] = 
{
    32, 200, 32, 255,
    32, 200, 32, 255,
    32, 200, 32, 255,
    32, 200, 32, 255,
    32, 200, 32, 255,
    32, 200, 32, 255,
    32, 200, 32, 255,
    32, 200, 32, 255,
    41, 121, 29, 255,
    51, 35, 26, 255,
    51, 35, 26, 255,
    51, 35, 26, 255,
    51, 35, 26, 255,
    51, 35, 26, 255,
    51, 35, 26, 255,
    51, 35, 26, 255,
    51, 35, 26, 255,
    51, 35, 26, 255,
    51, 35, 26, 255,
    51, 35, 26, 255,
    51, 35, 26, 255,
    51, 35, 26, 255,
    51, 35, 26, 255,
    140, 129, 124, 255,
    228, 228, 240, 255,
    228, 228, 240, 255,
    228, 228, 240, 255,
    228, 228, 240, 255,
    228, 228, 240, 255,
    228, 228, 240, 255,
    228, 228, 240, 255,
    228, 228, 240, 255,
};


void makeHeightTexture(void)
{
    int j;
    for (j = 0; j < stripeImageWidth; j++) {
        float frac = j * 100.0 / stripeImageWidth;
	float alpha;

      if(frac < 1.0){
	  height[4*j] = 20;
	  height[4*j+1] = 20;
	  height[4*j+2] = 170;
      } else if(frac < 2.0){
          alpha = (frac - 30) / 10;
	  height[4*j] = 20 * (1 - alpha) + 32 * alpha;
	  height[4*j+1] = 20 * (1 - alpha) + 200 * alpha;
	  height[4*j+2] = 170 * (1 - alpha) + 32 * alpha;
      } else if(frac < 40.0){
	  height[4*j] = 32;
	  height[4*j+1] = 200;
	  height[4*j+2] = 32;
      } else if(frac < 50.0){
          alpha = (frac - 40) / 10;
	  height[4*j] = 32 * (1 - alpha) + 51 * alpha;
	  height[4*j+1] = 200 * (1 - alpha) + 35 * alpha;
	  height[4*j+2] = 32 * (1 - alpha) + 26 * alpha;
      } else if(frac < 70.0){
	  height[4*j] = 51;
	  height[4*j+1] = 35;
	  height[4*j+2] = 26;
      } else if(frac < 80.0){
          alpha = (frac - 70) / 10;
	  height[4*j] = 51 * (1 - alpha) + 228 * alpha;
	  height[4*j+1] = 35 * (1 - alpha) + 228 * alpha;
	  height[4*j+2] = 26 * (1 - alpha) + 240 * alpha;
      } else {
	  height[4*j] = 228;
	  height[4*j+1] = 228;
	  height[4*j+2] = 240;
      }

      height[4*j+3] = (GLubyte) 255;
    }
}

#define RULE_SIZE	64
GLubyte rule[RULE_SIZE][RULE_SIZE][4];
GLubyte trule[100][100][4];

void
makeRule(void) {
    int i, j, k;
    for(i = 0; i < 100; i++) {
	for(j = 0; j < 100; j++) {
	    trule[i][j][0] = .65*255;
	    trule[i][j][1] = .65*255;
	    trule[i][j][2] = .65*255;
	    trule[i][j][3] = 255;
	}
    }
    for(i = 0; i < 10; i++) {
	for(k = -1 ; k < 2; k++) {
	    int l = i*10-k;
	    if (l < 0) l += 100;
	    if (l > 99) l -= 100;

	    trule[l][49][0] = trule[l][49][1] = trule[l][49][2] = 0;
	    trule[l][50][0] = trule[l][50][1] = trule[l][50][2] = 0;
	    trule[l][51][0] = trule[l][51][1] = trule[l][51][2] = 0;

	    trule[49][l][0] = trule[49][l][1] = trule[49][l][2] = 0;
	    trule[50][l][0] = trule[50][l][1] = trule[50][l][2] = 0;
	    trule[51][l][0] = trule[51][l][1] = trule[51][l][2] = 0;
	}
    }
    for(i = 0; i < 2; i++) {
	for(k = -1; k < 2; k++) {
	    int l = 50*i+k;
	    if (l < 0) l += 100;
	    if (l > 99) l -= 100;
	    trule[l][47][0] = trule[l][47][1] = trule[l][47][2] = 0;
	    trule[l][48][0] = trule[l][48][1] = trule[l][48][2] = 0;
	    trule[l][52][0] = trule[l][52][1] = trule[l][52][2] = 0;
	    trule[l][53][0] = trule[l][53][1] = trule[l][53][2] = 0;

	    trule[47][l][0] = trule[47][l][1] = trule[47][l][2] = 0;
	    trule[48][l][0] = trule[48][l][1] = trule[48][l][2] = 0;
	    trule[52][l][0] = trule[52][l][1] = trule[52][l][2] = 0;
	    trule[53][l][0] = trule[53][l][1] = trule[53][l][2] = 0;
	}
    }

#if 0
    {FILE *fd = fopen("rule.bw", "w");
    int i, j;
    for(i = 0; i < 100; i++)
	for(j = 0; j < 100; j++)
	fwrite(trule[i][j], 1, 1, fd);
    fclose(fd);
    }
#endif
    gluScaleImage(GL_RGBA, 100, 100, GL_UNSIGNED_BYTE, trule,
	RULE_SIZE, RULE_SIZE, GL_UNSIGNED_BYTE, rule);
}

/*  planes for texture coordinate generation  */
static GLfloat xequalzero[] = {1.0, 0.0, 0.0, 0.0};
static GLfloat yequalzero[] = {0.0, 1.0, 0.0, 0.0};
static GLfloat zequalzero[] = {0.0, 0.0, 1.0, 0.0};
static GLfloat slanted[] = {1.0, 1.0, 1.0, 0.0};
static GLfloat *currentCoeff;
static GLenum currentPlane;
static GLint currentGenMode;
static GLfloat lightpos[] = {0, 1, 0, 0};

void makeTerrain(void);

void init(void) {
    glClearColor (0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    makeTerrain();

    makeStripeImage();
    makeRainbow();
    makeHeightTexture();
    makeRule();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    /* glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT); */
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage1D(GL_TEXTURE_1D, 0, 4, stripeImageWidth, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, height);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    currentCoeff = yequalzero;
    currentGenMode = GL_OBJECT_LINEAR;
    currentPlane = GL_OBJECT_PLANE;
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, currentGenMode);
    glTexGenfv(GL_S, currentPlane, currentCoeff);

    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, xequalzero);

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_1D);
    /* glEnable(GL_CULL_FACE); */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glMaterialf (GL_FRONT, GL_SHININESS, 64.0);


    glMatrixMode(GL_PROJECTION);
    glFrustum(-.33, .33, -.33, .33, .5, 40);

    glMatrixMode(GL_MODELVIEW);
    gluLookAt(0, 0, 10.5, 0, 0, 0, 0, 1, 0);
}

void tfunc(void) {
    static int state = 1;

    state += 1;
    if (state > 2) state = 0;
    altitudeScale = 1.f;
    if (state == 2) {
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage1D(GL_TEXTURE_1D, 0, 4, stripeImageWidth, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, stripeImage);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_1D);
        glTexGenfv(GL_S, GL_OBJECT_PLANE, yequalzero);
	altitudeScale = 5.f;
    } else if (state ==  1) {
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage1D(GL_TEXTURE_1D, 0, 4, stripeImageWidth, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, height);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_1D);
        glTexGenfv(GL_S, GL_OBJECT_PLANE, yequalzero);
    } else {
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, RULE_SIZE, RULE_SIZE, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, rule);
	glDisable(GL_TEXTURE_1D);
	glEnable(GL_TEXTURE_2D);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, zequalzero);
    }
    glutPostRedisplay();
}


#define GRIDX	128
#define GRIDY	128


float points[GRIDX * GRIDY][3], normals[GRIDX * GRIDY][3];


#define X 0
#define Y 1
#define Z 2


void crossVec3(float v0[3], float v1[3], float result[3])
{
    result[X] = v0[Y] * v1[Z] - v0[Z] * v1[Y];
    result[Y] = v0[Z] * v1[X] - v0[X] * v1[Z];
    result[Z] = v0[X] * v1[Y] - v0[Y] * v1[X];
}


void makeNormal(float common[3], float p1[3], float p2[3], float n[3])
{
    float 	v1[3], v2[3];
    float	d;

    v1[X] = p1[X] - common[X];
    v1[Y] = p1[Y] - common[Y];
    v1[Z] = p1[Z] - common[Z];
    v2[X] = p2[X] - common[X];
    v2[Y] = p2[Y] - common[Y];
    v2[Z] = p2[Z] - common[Z];
    crossVec3(v1, v2, n);

    d = sqrt(n[X] * n[X] + n[Y] * n[Y] + n[Z] * n[Z]);
    n[X] /= d;
    n[Y] /= d;
    n[Z] /= d;
}


void makeMeshNormals(float (*points)[3], float (*normals)[3], int w, int h)
{
    int x, y;

    for(y = 0; y < h - 1; y++)
	for(x = 0; x < w - 1; x++) {
	    /* normal at x, y is cross of (x,y)(x,y+1) and (x,y)(x+1,y) */
	    makeNormal(points[y * w + x], points[(y + 1) * w + x],
	        points[y * w + x + 1], normals[y * w + x]);
	}

    for(x = 0; x < w - 1; x++) {
	/* normal at x, h-1 is cross of (x,h-1)(x+1,h-1) and (x,h-1)(x,h-2) */
	makeNormal(points[(h - 1) * w + x], points[(h - 1) * w + x + 1],
	    points[(h - 2) * w + x], normals[(h - 1) * w + x]);
    }

    for(y = 0; y < h - 1; y++) {
	/* normal at w-1, y is cross of (w-1,y)(w-2,y) and (w-1,y)(w-1,y+1) */
	makeNormal(points[y * w + w - 1], points[y * w + w - 2],
	    points[(y + 1) * w + w - 1], normals[y * w + w - 1]);
    }

    /* normal at w-1, h-1 is cross of (w-1,h-1)(w-1,h-2) and (w-1,h-1)(w-2,h-1) */
    makeNormal(points[(h - 1) * w + w - 1], points[(h - 2) * w + w - 1],
	points[(h - 1) * w + w - 2], normals[(h - 1) * w + w - 1]);
}



void drawGLMesh(float (*points)[3], float (*normals)[3], int w, int h)
{
    int x, y;

    for(y = 0; y < h - 1; y++) {
        glBegin(GL_QUAD_STRIP);
	for(x = 0; x < w; x++) {
	    glNormal3fv(normals[y * w + x]);
	    glVertex3fv(points[y * w + x]);
	    glNormal3fv(normals[(y + 1) * w + x]);
	    glVertex3fv(points[(y + 1) * w + x]);
	}
	glEnd();
    }

}


void makeTerrain(void)
{
    int x, y;

    for(x = 0; x < GRIDX; x++)
	for(y = 0; y < GRIDY; y++) {
	    float s, t, d;

	    s = x / (float)(GRIDX - 1);
	    t = y / (float)(GRIDY - 1);
	    d = pow(
	        (
		    noiseBicubic3f(s * 10, t * 10, 0) +
		    (noiseBicubic3f(s * 20, t * 20, 0) - .5) / 2 +
		    (noiseBicubic3f(s * 40, t * 40, 0) - .5) / 4 +
		    (noiseBicubic3f(s * 80, t * 80, 0) - .5) / 8
		), 1.5) * 3 - .5;
	    if(d < 0) d = 0;
	    points[y * GRIDX + x][X] = -4 + 8.0 * x / (GRIDX - 1);
	    points[y * GRIDX + x][Y] = d;
	    points[y * GRIDX + x][Z] = -4 + 8.0 * y / (GRIDY - 1);
	}

    makeMeshNormals(points, normals, GRIDX, GRIDY);
}


float terrainScale = 1.0;


void terrain(void)
{
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(terrainScale*altitudeScale, 1, 1);	
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    /* glScalef(1, terrainScale, 1); */
    drawGLMesh(points, normals, GRIDX, GRIDY);
    glPopMatrix();
}


void display(void) {
#if 0
    static GLUquadric *q;
#endif
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glPushMatrix();
    glTranslatef(0., 0., transx);
    glRotatef(rotx, 1.0, 0.0, 0.0);
    glRotatef(roty, 0.0, 1.0, 0.0);

    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    /* glRotatef(45.0, 0.0, 0.0, 1.0); */
    /* glRotatef(90, 1, 0, 0); */

    terrain();
    /* glutSolidTeapot(1.0); */

#if 0
    if (!q) q = gluNewQuadric();
    gluQuadricTexture(q, GL_TRUE);
    gluCylinder(q, 1.0, 2.0, 3.0, 10, 10);
#endif

    glPopMatrix();
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);
#if 0
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (w <= h)
	glOrtho (-3.5, 3.5, -3.5*(GLfloat)h/(GLfloat)w, 
               3.5*(GLfloat)h/(GLfloat)w, -3.5, 3.5);
    else
	glOrtho (-3.5*(GLfloat)w/(GLfloat)h, 
               3.5*(GLfloat)w/(GLfloat)h, -3.5, 3.5, -3.5, 3.5);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
#endif
}

void
help(void) {
    printf("e/E    - select eye linear\n");
    printf("o/O    - select object linear\n");
    printf("s/S    - select rotated function\n");
    printf("./>    - scale terrain up\n");
    printf(",/<    - scale terrain down\n");
    printf("t      - toggle texture function\n");
}

/*ARGSUSED1*/
void keyboard (unsigned char key, int x, int y) {
    switch (key) {
    case 'e':
    case 'E':
	 currentGenMode = GL_EYE_LINEAR;
	 currentPlane = GL_EYE_PLANE;
	 glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, currentGenMode);
	 glTexGenfv(GL_S, currentPlane, currentCoeff);
	 glutPostRedisplay();
	 break;
    case 'o':
    case 'O':
         currentGenMode = GL_OBJECT_LINEAR;
         currentPlane = GL_OBJECT_PLANE;
         glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, currentGenMode);
         glTexGenfv(GL_S, currentPlane, currentCoeff);
         glutPostRedisplay();
         break;
    case 's':
    case 'S':
         currentCoeff = slanted;
         glTexGenfv(GL_S, currentPlane, currentCoeff);
         glutPostRedisplay();
         break;
    case 'x':
    case 'X':
         currentCoeff = xequalzero;
         glTexGenfv(GL_S, currentPlane, currentCoeff);
         glutPostRedisplay();
         break;
    case '.': case '>':
         terrainScale *= 1.1f;
	 /* makeTerrain(); */
         glutPostRedisplay();
         break;
    case '<': case ',':
         terrainScale /= 1.1f;
	 /* makeTerrain(); */
         glutPostRedisplay();
         break;
    case 't': tfunc(); break;
    case 27:
         exit(0);
         break;
    default:
	 help();
         break;
    }
}

int main(int argc, char*argv[]) {
    noiseInit();
    
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(256, 256);
    glutInitWindowPosition(100, 100);
    glutInit(&argc, argv);
    glutCreateWindow(argv[0]);
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutMainLoop();
    return 0;
}
