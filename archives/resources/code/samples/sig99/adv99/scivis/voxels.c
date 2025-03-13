#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>
#include <math.h>
#include "texture.h"

/* nonzero if not power of 2 */ 
#define NOTPOW2(num) ((num) & (num - 1))

int
makepow2(int val)
{
    int power = 0;
    if(!val)
	return 0;

    while(val >>= 1)
	power++;

    return(1 << power);
}

#define CHECK_ERROR(str)                                           \
{                                                                  \
    GLenum error;                                                  \
    if(error = glGetError())                                       \
       printf("GL Error: %s (%s)\n", gluErrorString(error), str);  \
}

enum {X, Y, Z, W};
enum {R, G, B, A};
enum {OVER, ATTENUATE, NONE, LASTOP};
enum {OBJ_ANGLE, SLICES, CUTTING};

/* window dimensions */
int winWidth = 512;
int winHeight = 512;
int active;
int operator = NONE;
GLboolean textureOn = GL_TRUE;
GLboolean dblbuf = GL_TRUE;
GLboolean cut = GL_FALSE;
GLint cutbias = 50;
int ext_blend_color = 0;
int ext_texture3D = 0;
GLboolean clip = GL_FALSE;

GLfloat lightangle[2] = {0.f, 0.f};
GLfloat objangle[2] = {0.f, 0.f};
GLfloat objpos[2] = {0.f, 0.f};

void turnOnTexture(void)
{
    glEnable(GL_TEXTURE_3D_EXT);
}

void turnOffTexture(void)
{
    glDisable(GL_TEXTURE_3D_EXT);
}

void dummy(void)
{
    //do nothing
}

void (*textureEnable)(void) = dummy; //texture enable function pointer
void (*textureDisable)(void) = dummy; //texture enable function pointer


/* 3d texture data that's read in */
/* XXX TODO; make command line arguments */
int Texwid = 128; /* dimensions of each 2D texture */
int Texht = 128;
int Texdepth = 69; /* number of 2D textures */

/* Actual dimensions of the texture (restricted to max 3d texture size) */
int texwid, texht, texdepth;
int texcomps;
int slices;
GLubyte *tex3ddata;
GLboolean perspective = GL_FALSE;
GLboolean colors = GL_FALSE;

void
reshape(int wid, int ht)
{
    winWidth = wid;
    winHeight = ht;
    glViewport(0, 0, wid, ht);
    /* cube, 300 on a side */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if(perspective)
	glFrustum(-100., 100., -100., 100., 170., 520.);
    else
	glOrtho(-150., 150., -150., 150., -150., 150.);
    glMatrixMode(GL_MODELVIEW);
    /* look at scene from (0, 0, 450) */
    glLoadIdentity();
    if(perspective)
	gluLookAt(0., 0., -350., 0., 0., 0., 0., 1., 0.);
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
    case SLICES:
	slices = x * texwid/winWidth;
	glutPostRedisplay();
	break;
    case CUTTING:
	cutbias = (x - winWidth/2) * 300/winWidth;
	glutPostRedisplay();
	break;
    }
}

void
mouse(int button, int state, int x, int y)
{
    if(state == GLUT_DOWN)
	switch(button)
	{
	case GLUT_LEFT_BUTTON: /* move the light */
	    active = OBJ_ANGLE;
	    motion(x, y);
	    break;
	    break;
	}
}


void
drawSlices(int slices)
{
    int i;
    for(i = 0; i < slices; i++)
    {
	glBegin(GL_QUADS);
	glVertex3f(-100.f, -100.f, -100.f + 200 * i/(slices - 1.f)); 
	glVertex3f( 100.f, -100.f, -100.f + 200 * i/(slices - 1.f)); 
	glVertex3f( 100.f,  100.f, -100.f + 200 * i/(slices - 1.f)); 
	glVertex3f(-100.f,  100.f, -100.f + 200 * i/(slices - 1.f)); 
	glEnd();
    }

    for(i = 0; i < slices; i++)
    {
	glBegin(GL_QUADS);
	glVertex3f(-100.f + 200 * i/(slices - 1.f), -100.f, -100.f);
	glVertex3f(-100.f + 200 * i/(slices - 1.f), -100.f,  100.f);
	glVertex3f(-100.f + 200 * i/(slices - 1.f),  100.f,  100.f);
	glVertex3f(-100.f + 200 * i/(slices - 1.f),  100.f, -100.f);
	glEnd();
    }

    for(i = 0; i < slices; i++)
    {
	glBegin(GL_QUADS);
	glVertex3f(-100.f, -100.f + 200 * i/(slices - 1.f),  100.f);
	glVertex3f( 100.f, -100.f + 200 * i/(slices - 1.f),  100.f);
	glVertex3f( 100.f, -100.f + 200 * i/(slices - 1.f), -100.f);
	glVertex3f(-100.f, -100.f + 200 * i/(slices - 1.f), -100.f);
	glEnd();
    }

}

void
drawHatch(int slices)
{
    int i;
    //XY Plane
    for(i = 0; i < slices; i++)
    {
	glBegin(GL_LINES); //horizontal
	glVertex3f(-100.f, -100.f + 200 * i/(slices - 1.f), 100.f);
	glVertex3f( 100.f, -100.f + 200 * i/(slices - 1.f), 100.f);
	glEnd();
    }

    for(i = 0; i < slices; i++)
    {
	glBegin(GL_LINES); //vertical
	glVertex3f(-100.f + 200 * i/(slices - 1.f), -100.f, 100.f);
	glVertex3f(-100.f + 200 * i/(slices - 1.f),  100.f, 100.f);
	glEnd();
    }


    //YZ Plane
    for(i = 0; i < slices; i++)
    {
	glBegin(GL_LINES); //horizontal
	glVertex3f( 100.f, -100.f + 200 * i/(slices - 1.f), -100.f);
	glVertex3f( 100.f, -100.f + 200 * i/(slices - 1.f),  100.f);
	glEnd();
    }

    for(i = 0; i < slices; i++)
    {
	glBegin(GL_LINES); //vertical
	glVertex3f(100.f, -100.f, -100.f + 200 * i/(slices - 1.f));
	glVertex3f(100.f,  100.f, -100.f + 200 * i/(slices - 1.f));
	glEnd();
    }

    //XZ Plane
    for(i = 0; i < slices; i++)
    {
	glBegin(GL_LINES);
	glVertex3f(-100.f,  100.f, -100.f + 200 * i/(slices - 1.f));
	glVertex3f( 100.f,  100.f, -100.f + 200 * i/(slices - 1.f));
	glEnd();
    }

    for(i = 0; i < slices; i++)
    {
	glBegin(GL_LINES);
	glVertex3f(-100.f + 200 * i/(slices - 1.f), 100.f, -100.f);
	glVertex3f(-100.f + 200 * i/(slices - 1.f), 100.f,  100.f);
	glEnd();
    }
}


void redrawCube(void)
{
    int i;
    GLdouble clipPlane[] = {-1., -1., -1., 175.};

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glRotatef(objangle[X], 0.f, 1.f, 0.f);
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);

    glMatrixMode(GL_TEXTURE);
    glPushMatrix(); /* identity */
    glTranslatef(.5f, .5f, .5f);
    glRotatef(objangle[X], 0.f, 1.f, 0.f);
    glRotatef(objangle[Y], 1.f, 0.f, 0.f);
    glTranslatef(-.5f, -.5f, -.5f);


    if(clip)
	glEnable(GL_CLIP_PLANE0);
    else
	glDisable(GL_CLIP_PLANE0);

    glClipPlane(GL_CLIP_PLANE0, clipPlane);

    glEnable(GL_BLEND);
    glDepthFunc(GL_ALWAYS);

    glColor4f(1.f, 1.f, 1.f, .08f); 
    drawSlices(slices);

    glDisable(GL_BLEND);
    glColor3f(0.f, 0.f, 1.f);
    drawHatch(slices);

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    
    if(dblbuf)
	glutSwapBuffers(); 
    else
	glFlush(); 
    
}




void redrawCubeInit(void)
{
    glClearColor(0.f, 0.f, 0.f, 1.f);

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    colors = GL_FALSE;
    perspective = GL_TRUE;
    reshape(winWidth, winHeight);

    slices = 12;

    glEnable(GL_DEPTH_TEST);

    glutDisplayFunc(redrawCube);
    glutPostRedisplay();
}


void
help(void) {
    printf("t    - toggle texture\n");
    printf("left mouse    - rotate object\n");
}



/*ARGSUSED1*/
void key(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 't':
	if(textureEnable != dummy) //turn it off
	{
	    textureDisable();
	    textureEnable = dummy;
	    textureDisable = dummy;
	}
	else
	{
	    textureEnable = turnOnTexture;
	    textureDisable = turnOffTexture;
	    textureEnable();
	}
	glutPostRedisplay();
	break;
    case 'C':
    case 'c':
    {
	if(clip)
	    clip = GL_FALSE;
	else
	    clip = GL_TRUE;
	break;
    }
    case 'f':
    {
	int row;
	FILE *fp;
	unsigned char *image = NULL;
	fprintf(stderr, "Saving Image... ");
	image = (unsigned char *)malloc(winWidth * winHeight * 3);
	if(!image)
	{
	    fprintf(stderr, "snapshot failed; couldn't malloc buffer\n");
	    break;
	}
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, winWidth, winHeight, GL_RGB, GL_UNSIGNED_BYTE, image);
	glReadBuffer(GL_BACK);
	fp = fopen("snapshot.ppm", "w");
	fprintf(fp, "P6 %d %d 255\n", winWidth, winHeight);
	for(row = winHeight - 1; row >= 0; row--)
	    fwrite(&image[row * winWidth * 3], winWidth, 3, fp);
	fclose(fp);
	free(image);
	fprintf(stderr, "Image Saved\n");
	break;
    }
    case 'p':
    case 'P': //preview mode
	slices = 6;
	break;
    case 'm': //medium quality
	slices = 8;
	break;
    case 'M': //medium quality
	slices = 16;
	break;
    case 'q':
    case 'Q': //quality mode
	slices = 32;
	break;
    case 'b': //toggle bricking
	if(colors)
	    colors = GL_FALSE;
	else
	    colors = GL_TRUE;
	break;
    case '\033':
	exit(0);
	break;
    default: help(); 
	return; //don't redisplay
    }
    glutPostRedisplay();
}

void
menu(int which) {
    key((char)which, 0, 0);
}

void
createMenu(void) {
    glutCreateMenu(menu);
    glutAddMenuEntry("preview mode (P)", 'p');
    glutAddMenuEntry("medium quality (m)", 'm');
    glutAddMenuEntry("medium quality (M)", 'M');
    glutAddMenuEntry("quality mode (Q)", 'q');
    glutAddMenuEntry("show as slices (S)", 's');
    glutAddMenuEntry("show volume (V)", 'v');
    glutAddMenuEntry("toggle texture (T)", 't');
    glutAddMenuEntry("toggle bricking (b)", 'b');
    glutAddMenuEntry("exit", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

GLubyte *
loadtex3d(int *texwid, int *texht, int *texdepth, int *texcomps)
{
    char *filename;
    GLubyte *tex3ddata;
    GLuint *texslice; /* 2D slice of 3D texture */
    GLint max3dtexdims; /* maximum allowed 3d texture dimension */
    GLint newval;
    int i;

    /* load 3D texture data */
    filename = (char*)malloc(sizeof(char) * 
			     strlen("../data/skull/skullXX.la"));

    tex3ddata = (GLubyte *)malloc(Texwid * Texht * Texdepth * 
				  4 * sizeof(GLubyte));
    for(i = 0; i < Texdepth; i++)
    {
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	sprintf(filename, "../data/skull/skull%d.la", i);
	/* read_texture reads as RGBA */
	texslice = read_texture(filename, texwid, texht, texcomps);

	if(!texslice)
	{
	    fprintf(stderr, "Couldn't read texture file skull%d.1a\n", i);
	    exit -1;
	}
	memcpy(&tex3ddata[i * Texwid * Texht * 4],  /* copy in a slice */
	       texslice, 
	       Texwid * Texht * 4 * sizeof(GLubyte));
	free(texslice);

    }
    free(filename);

    *texdepth = Texdepth;

#ifdef GL_EXT_texture3D
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE_EXT, &max3dtexdims);
    fprintf(stderr, "Maximum 3d texture size: %d\n", max3dtexdims);
#endif

    /* adjust width */
    newval = *texwid;
    if(*texwid > max3dtexdims)
	newval = max3dtexdims;
    if(NOTPOW2(*texwid))
        newval = makepow2(*texwid);
    fprintf(stderr, "Texture width: %d Adjusted texture width: %d\n", *texwid, newval);
    if(newval != *texwid)
    {
	glPixelStorei(GL_UNPACK_ROW_LENGTH, *texwid);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, (*texwid - newval)/2);
	*texwid = newval;
    }

    /* adjust height */
    newval = *texht;
    if(*texht > max3dtexdims)
	newval = max3dtexdims;
    if(NOTPOW2(*texht))
        newval = makepow2(*texht);
    fprintf(stderr, "Texture height: %d Adjusted texture height: %d\n", *texht, newval);
    if(*texht > newval)
    {
#ifdef GL_EXT_texture3D
	glPixelStorei(GL_UNPACK_IMAGE_HEIGHT_EXT, *texht);
#endif
	glPixelStorei(GL_UNPACK_SKIP_ROWS, (*texht - newval)/2);
	*texht = newval;
    }

    /* adjust depth */
    newval = *texdepth;
    if(*texdepth > max3dtexdims)
	newval = max3dtexdims;
    if(NOTPOW2(*texdepth))
        newval = makepow2(*texdepth);
    fprintf(stderr, "Texture depth: %d Adjusted texture depth: %d\n", *texdepth, newval);
    if(*texdepth > newval)
    {
	*texdepth = newval;
    }
    return tex3ddata;
}


main(int argc, char *argv[])
{
    int i;

    glutInit(&argc, argv);
    glutInitWindowSize(winWidth, winHeight);
    if(argc > 1)
    {
	char *args = argv[1];
	GLboolean done = GL_FALSE;
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
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
    else
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH);

    (void)glutCreateWindow("volume rendering demo");
    glutDisplayFunc(redrawCubeInit);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    createMenu();
    glutKeyboardFunc(key);

    /* Initialize OpenGL State */


    key('p', 0, 0); //start in preview mode

    CHECK_ERROR("end of main");

    glutMainLoop();
    return 0;
}
