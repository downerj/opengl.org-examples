#include <stdlib.h>
#include <stdio.h>
#include "math.h"
#include <GL/glut.h>
#ifdef _WIN32
#define sinf(x) ((float)sin((x)))
#define cosf(x) ((float)cos((x)))
#define sqrtf(x) ((float)(sqrt(x)))
#define floorf(x) ((float)(floor(x)))
#ifndef M_PI
#define M_PI 3.14159265
#endif
#endif


static float transx = 8.0, transy, rotx = 0, roty = 0;
static int ox = -1, oy = -1;
static int mot = 0;
#define PAN	1
#define ROT	2
float scale = 1.f;
float tscale = 1.f;
void makeGrid(void);
void grid(void);
#define GRID_WIDTH	64
#define GRID_HEIGHT	64
int gridWidth = GRID_WIDTH;
int gridHeight = GRID_HEIGHT;
float Grid[GRID_WIDTH*GRID_HEIGHT];
float Grid2[GRID_WIDTH*GRID_HEIGHT];
float GridN[GRID_WIDTH*GRID_HEIGHT*3];
float heightScale = 0.f;

#define FIELD_WIDTH	32
#define FIELD_HEIGHT	32
GLubyte field[FIELD_HEIGHT][FIELD_WIDTH][4];
static int light;

void
cross(GLfloat *x, GLfloat *y, GLfloat *z, float u, float v, float w, float a, float b, float c)
{
    *x = v * c - w * b;
    *y = w * a - u * c;
    *z = u * b - v * a;
}

void
normalize(float* x, float* y, float* z) {
    float l = *x * *x + *y * *y + *z * *z;
    l = sqrtf(l);
    if (l > 0) {
	*x /= l;
	*y /= l;
	*z /= l;
    }
}

void
calcNormals(void) {
    int i, j;

    for(j = 0; j < gridHeight-1; j++) {
	for(i = 0; i < gridWidth-1; i++) {
	    float nx, ny, nz, v0x, v0y, v0z, v1x, v1y, v1z;
	    v0x = 1;
	    v0y = 0;
	    v0z = Grid[j*gridWidth+i+1] - Grid[j*gridWidth+i];
	    v1x = 0;
	    v1y = 1;
	    v1z = Grid[(j+1)*gridWidth+i] - Grid[j*gridWidth+i];
	    cross(&nx, &ny, &nz, v0x, v0y, v0z, v1x, v1y, v1z);
/*printf("%f %f %f\n", nx, ny, nz);*/
	    normalize(&nx, &ny, &nz);
	    GridN[(j*gridWidth+i)*3+0] = nx;
	    GridN[(j*gridWidth+i)*3+1] = nz;
	    GridN[(j*gridWidth+i)*3+2] = ny;
	}
    }
}

void
calc2(void) {
    int i, j;
    for(j = 0; j < gridHeight; j++) {
	for(i = 0; i < gridWidth-1; i++) {
	    Grid2[j*gridWidth+i] = Grid2[j*gridWidth+i] - Grid[j*gridWidth+i+1];
	}
    }
    
    for(i = 0; i < FIELD_HEIGHT; i++) {
       for (j = 0; j < FIELD_WIDTH; j++) {
	  float scale = (float)i/(FIELD_HEIGHT-1.f);
	  int r = (GLubyte) ((j<=4) ? 255 : 128);
	  int g = (GLubyte) ((j>4) ? 128 : 0);
	  int b = (GLubyte) ((j>4) ? 128 : 0);
	  int a = (GLubyte) 255;
	  field[i][j][0] = r*scale;
	  field[i][j][1] = g*scale;
	  field[i][j][2] = b*scale;
	  field[i][j][3] = a;
       }
    }
}


void
readfield(char *file) {
    FILE *fd;
    int i, j;
    if ((fd = fopen(file, "r")) == NULL) {
	perror(file);
	exit(1);
    }
    fread(Grid, sizeof Grid, 1, fd);
    fclose(fd);
    for(j = 0; j < gridHeight; j++) {
	for(i = 0; i < gridWidth-1; i++) {
	    Grid2[j*gridWidth+i] = Grid2[j*gridWidth+i] - Grid[j*gridWidth+i+1];
	}
    }
    calc2();
}

void
pan(const int x, const int y) {
    transx +=  (x-ox)/10.;
    transy -= (y-oy)/10.;
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
      stripeImage[4*j] = (GLubyte) ((j<=4) ? 255 : 128);
      stripeImage[4*j+1] = (GLubyte) ((j>4) ? 128 : 0);
      stripeImage[4*j+2] = (GLubyte) ((j>4) ? 128 : 0);
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

void init(void) {
   glClearColor (0.0, 0.0, 0.0, 0.0);
   glEnable(GL_DEPTH_TEST);
   glShadeModel(GL_SMOOTH);

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
#if 0
   glEnable(GL_LIGHTING);
#endif
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
    if (state > 3) state = 0;
    if (state == 3) {
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, FIELD_WIDTH, FIELD_HEIGHT, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, field);
	glDisable(GL_TEXTURE_1D);
	glEnable(GL_TEXTURE_2D);
    } else if (state == 2) {
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage1D(GL_TEXTURE_1D, 0, 4, stripeImageWidth, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, stripeImage);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_1D);
        glTexGenfv(GL_S, GL_OBJECT_PLANE, yequalzero);
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

void plate(void)
{
   glMatrixMode(GL_TEXTURE);
   glLoadIdentity();
   /*glScalef(terrainScale*altitudeScale, 1, 1);	*/
   glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    /* glScalef(1, terrainScale, 1); */
    glNormal3f(0.f, 0.f, -1.f);
    glColor4f(1.f, 1.f, 1.f, 1.f);
    glBegin(GL_TRIANGLE_STRIP);
    glVertex3f(-1.f, -1.f, 0.f);
    glVertex3f(-1.f,  1.f, 0.f);
    glVertex3f( 1.f, -1.f, 0.f);
    glVertex3f( 1.f,  1.f, 0.f);
    glEnd();
    glPopMatrix();
}

void
makeGrid(void) {
    int i, j;
#if 0
    for(j = 0; j < gridHeight; j++) {
	for(i = 0; i < gridWidth; i++) {
	    float x = -1.f + i*2.f/gridWidth;
	    float y = -1.f + j*2.f/gridHeight;
	    Grid[j*gridWidth+i] = sinf(x*y)+cosf(x+y);
	}
    }
#endif
    for(i = 0; i < gridWidth; i++) {
	float x = -1.f + 2.f*(i-1)/(gridWidth-1.f);
	for(j = 0; j < gridHeight; j++) {
	    float y = -1.f + 2.f*(j-1)/(gridHeight-1.f);
	    float f = 10.f*y + 5.f*y / (x*x + y*y);
	    Grid[j*gridWidth+i] =  f < 0 ? log(-f) : log(f);
	}
    }
    calc2();
    calcNormals();
}

void grid(void)
{
   int i, j;
   glMatrixMode(GL_TEXTURE);
   glLoadIdentity();
   glScalef(scale, tscale, 1);
   glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glNormal3f(0.f, 1.f, 0.f);
    glColor4f(1.f, 1.f, 1.f, 1.f);
    for(j = 0; j < gridHeight-1; j++) {
	float xstep = 2.f/(gridWidth-1.f);
	float y1 = -1.f + (2.f/(gridHeight-1.f))*j;
	float y2 = y1 + (2.f/(gridHeight-1.f));
	glBegin(GL_TRIANGLE_STRIP);
	for(i = 0; i < gridWidth; i++) {
	    glTexCoord2f(Grid[j*gridWidth+i], Grid2[j*gridWidth+i]);
	    glNormal3fv(&GridN[(j*gridWidth+i)*3]);
	    glVertex3f(-1.f+xstep*i, y1, Grid[j*gridWidth+i]*heightScale);
	    glTexCoord2f(Grid[(j+1)*gridWidth+i], Grid2[(j+1)*gridWidth+i]);
	    glNormal3fv(&GridN[((j+1)*gridWidth+i)*3]);
	    glVertex3f(-1.f+xstep*i, y2, Grid[(j+1)*gridWidth+i]*heightScale);
	}
	glEnd();
    }
    glPopMatrix();
}


void display(void) {
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


   glPushMatrix();
   glTranslatef(0., 0., transx);
   glRotatef(rotx, 1.0, 0.0, 0.0);
   glRotatef(roty, 0.0, 1.0, 0.0);

    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

   /* glRotatef(45.0, 0.0, 0.0, 1.0); */
   /* glRotatef(90, 1, 0, 0); */

#if 1
   grid();
#else
   glutSolidTeapot(1.0);
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
    printf("t      - toggle texture function\n\n");
}

/*ARGSUSED1*/
void key (unsigned char key, int x, int y) {
   static int wire = 0;
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
         scale *= 1.1f;
	 /* makeTerrain(); */
         glutPostRedisplay();
         break;
      case '<': case ',':
         scale /= 1.1f;
	 /* makeTerrain(); */
         glutPostRedisplay();
         break;
      case '+':
	 tscale *= 1.1f;
	 break;
      case '-':
	 tscale /= 1.1f;
	 break;
      case 't': tfunc(); break;
      case 'w':
	    if (wire ^= 1) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	    } else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	    }
	    break;
      case 'g':
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	break;
      case 'h':
	if (heightScale == 0.f)
	    heightScale = 0.1f;
	else
	    heightScale = 0.f;
	break;
      case 'l':
	if (light ^= 1) {
	    glEnable(GL_LIGHTING);
	} else {
	    glDisable(GL_LIGHTING);
	}
	break;
      case 27:
         exit(0);
         break;
      default:
	 help();
         return;
    }
    glutPostRedisplay();
}

void
menu(int which) {
    key((char)which, 0, 0);
}

void
create_menu(void) {
    glutCreateMenu(menu);
    glutAddMenuEntry("change texture", 't');
    glutAddMenuEntry("toggle lighting", 'l');
    glutAddMenuEntry("exit", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

int main(int argc, char*argv[]) {
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(100, 100);
    glutInit(&argc, argv);
    glutCreateWindow(argv[0]);
    init();
    if (argc > 1) readfield(argv[1]);
    else makeGrid();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    create_menu();
    key('g', 0, 0);
    key('t', 0, 0);
	help();
    glutMainLoop();
    return 0;
}
