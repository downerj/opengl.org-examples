/*
 * common.c
 *
 *	Written by Scott R. Nelson and Greg Taylor of Sun Microsystems, Inc.
 *
 * Copyright (c) 1998, Sun Microsystems, Inc.
 *		All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Sun Microsystems, Inc. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 *	Common useful functions module for demo programs.
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif

#include <GL/glut.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#include "modelcontext.h"
#include "common.h"

#define FRONT_PLANE 2.0			/* Needs work to be adjustable */

extern ModelContext *model;

static void drawStrip(int points, int x_pos, int y_pos, int z_pos);
static void fillSphereArray(int points);



/*
 * setView
 *
 *	Set up the proper view.
 */

void
setView(void)
{
    GLdouble distanceAdjust;		/* Front plane is NOT at screen */

    /* Compute a correct projection matrix */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    /* Compute adjustment factor */
    distanceAdjust = FRONT_PLANE / EYE_BACK; /* Ratio: front plane to screen */
    /* Convert to measuring in inches, then adjust for center to edge */
    distanceAdjust /= PIXELS_PER_INCH / FRONT_PLANE;

    glFrustum(-model->windowWidth * distanceAdjust, model->windowWidth * distanceAdjust,
	-model->windowHeight * distanceAdjust, model->windowHeight * distanceAdjust,
	FRONT_PLANE, 1000.0);		/* Front clip at (2), back at 1000 */
} /* End of setView */



/*
 * setColor
 *
 *	A common routine to set "material" model->colorList.  If we're shading,
 *	make all of the glMaterial calls.  If we're not shading, make
 *	a reasonable glColor call.  If we're in stereo mode, but the
 *	monitor isn't, get a uniform intensity so that the red/blue
 *	stereo works.
 */

void
setColor(const GLfloat *ambient,
    const GLfloat *diffuse,
    const GLfloat *specular,
    GLfloat shininess,
    GLboolean stereo)
{
    GLfloat c;				/* Color converted to intensity */
    GLfloat ca[4];

    if (!stereo) {
	glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);

	glColor4fv(diffuse);
    }
    else {
	/* Masking is done elsewhere */
	c = 0.27 * ambient[0] + 0.59 * ambient[1] + 0.14 * ambient[2];
	ca[0] = ca[1] = ca[2] = c;
	glMaterialfv(GL_FRONT, GL_AMBIENT, ca);

	ca[0] = ca[1] = ca[2] = c;
	ca[3] = diffuse[3];
	c = 0.27 * diffuse[0] + 0.59 * diffuse[1] + 0.14 * diffuse[2];
	glMaterialfv(GL_FRONT, GL_DIFFUSE, ca);

	glColor4f(c, c, c, diffuse[3]);

	ca[0] = ca[1] = ca[2] = c;
	c = 0.27 * specular[0] + 0.59 * specular[1] + 0.14 * specular[2];
	glMaterialfv(GL_FRONT, GL_SPECULAR, ca);

	glMaterialf(GL_FRONT, GL_SHININESS, shininess);
    }

} /* End of setColor */



/*
 * setLights
 *
 *	Turn on the lights mode as dictated by the parameter:
 *	0 - no lights (disable lighting)
 *	1 - regular view lights
 */

void
setLights(int lights)
{
static GLfloat light1[4] = {1.0f, 1.0f, 0.9f, 1.0f};
static GLfloat light2[4] = {0.3f, 0.3f, 0.4f, 1.0f};
static GLfloat pos1[4] = {-4.0f,  7.0f,  6.0f, 0.0f};
static GLfloat pos2[4] = { 6.0f,  2.0f,  1.0f, 0.0f};
static GLfloat ambient[3] = {0.2f, 0.2f, 0.2f};
static GLfloat diffuse[4] = {0.8f, 0.8f, 0.8f, 1.0f};
static GLfloat specular[3] = {1.0f, 1.0f, 1.0f};

    if (lights == 0) {
	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHT1);
	glDisable(GL_LIGHT2);
	return;
    }
    else  {
	glEnable(GL_LIGHTING);

	/* We scale without changing the normal, choose the fastest method */
/*
	if (glutExtensionSupported("GL_EXT_rescale_normal"))
	    glEnable(GL_RESCALE_NORMAL_EXT);
	else */
	    glEnable(GL_NORMALIZE);

	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glDisable(GL_LIGHT2);

	glLightfv(GL_LIGHT0, GL_DIFFUSE, light1);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light1);
	glLightfv(GL_LIGHT0, GL_POSITION, pos1);

	glLightfv(GL_LIGHT1, GL_DIFFUSE, light2);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light2);
	glLightfv(GL_LIGHT1, GL_POSITION, pos2);


	/* Set some material properties, in case the object has none */
	setColor(ambient, diffuse, specular, 50.0, 0);
    }
} /* End of setLights */



/*
 * buildCube
 *
 *	Store a cube into the data array.
 */

void
buildCube(void)
{
static Vertex cubeData[4 * 6] = {
	{ 0,  1.0, -1.0, -1.0, 0.0, 0.0, -1.0},
	{ 1, -1.0, -1.0, -1.0, 0.0, 0.0, -1.0},
	{ 1,  1.0,  1.0, -1.0, 0.0, 0.0, -1.0},
	{ 1, -1.0,  1.0, -1.0, 0.0, 0.0, -1.0},

	{ 0, -1.0,  1.0,  1.0, 0.0, 0.0, 1.0},
	{ 1, -1.0, -1.0,  1.0, 0.0, 0.0, 1.0},
	{ 1,  1.0,  1.0,  1.0, 0.0, 0.0, 1.0},
	{ 1,  1.0, -1.0,  1.0, 0.0, 0.0, 1.0},

	{ 0, -1.0,  1.0, -1.0, -1.0, 0.0, 0.0},
	{ 1, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0},
	{ 1, -1.0,  1.0,  1.0, -1.0, 0.0, 0.0},
	{ 1, -1.0, -1.0,  1.0, -1.0, 0.0, 0.0},

	{ 0,  1.0, -1.0,  1.0, 1.0, 0.0, 0.0},
	{ 1,  1.0, -1.0, -1.0, 1.0, 0.0, 0.0},
	{ 1,  1.0,  1.0,  1.0, 1.0, 0.0, 0.0},
	{ 1,  1.0,  1.0, -1.0, 1.0, 0.0, 0.0},

	{ 0,  1.0, -1.0, -1.0, 0.0, -1.0, 0.0},
	{ 1,  1.0, -1.0,  1.0, 0.0, -1.0, 0.0},
	{ 1, -1.0, -1.0, -1.0, 0.0, -1.0, 0.0},
	{ 1, -1.0, -1.0,  1.0, 0.0, -1.0, 0.0},

	{ 0, -1.0,  1.0,  1.0, 0.0, 1.0, 0.0},
	{ 1,  1.0,  1.0,  1.0, 0.0, 1.0, 0.0},
	{ 1, -1.0,  1.0, -1.0, 0.0, 1.0, 0.0},
	{ 1,  1.0,  1.0, -1.0, 0.0, 1.0, 0.0},
};
static ColorStruct cubeColors[6] = {
	{  0, 1.0f, 0.0f, 0.0f},
	{  4, 0.0f, 1.0f, 0.0f},
	{  8, 1.0f, 1.0f, 0.0f},
	{ 12, 0.5f, 0.0f, 1.0f},
	{ 16, 0.1f, 0.2f, 1.0f},
	{ 20, 1.0f, 0.4f, 0.0f},
};
    int i;

    glFrontFace(GL_CCW);
    model->haveNormals = 1;
    for (i = 0; i < 6 * 4; i++) {
	model->vertexList[i].draw = cubeData[i].draw;
	model->vertexList[i].x = cubeData[i].x;
	model->vertexList[i].y = cubeData[i].y;
	model->vertexList[i].z = cubeData[i].z;
	model->vertexList[i].nx = cubeData[i].nx;
	model->vertexList[i].ny = cubeData[i].ny;
	model->vertexList[i].nz = cubeData[i].nz;
    }
    model->vertexCount = i;
    model->vertexList[model->vertexCount].draw = 0;

    for (i = 0; i < 6; i++) {
	model->colorList[i].index = cubeColors[i].index;
	model->colorList[i].ra = cubeColors[i].ra;
	model->colorList[i].ga = cubeColors[i].ga;
	model->colorList[i].ba = cubeColors[i].ba;
	model->colorList[i].rd = cubeColors[i].ra;
	model->colorList[i].gd = cubeColors[i].ga;
	model->colorList[i].bd = cubeColors[i].ba;
	model->colorList[i].ad = 1.0;
	model->colorList[i].rs = 1.0;
	model->colorList[i].gs = 1.0;
	model->colorList[i].bs = 1.0;
	model->colorList[i].spec = 15.0;
    }
    model->colorCount = i;

} /* End of buildCube */



/* Degrees to radians */
#define RAD(x) ((x) * M_PI / 180.0)

/* Double-precision tolerance */
#define DP_TOL (1.0e-8)

/* Maximum tessellation factor plus 1 (must be an even number) */
#define SPH_MAX 100



/* Global data */
static float xa[SPH_MAX][SPH_MAX];	/* Pre-computed points x, y, z */
static float ya[SPH_MAX][SPH_MAX];
static float za[SPH_MAX][SPH_MAX];



/*
 *----------------------------------------------------------------------
 *
 * fillSphereArray
 *
 *	Compute the points for 1/8 of a sphere, store it into a 2D (row
 *	by column) array as a triangle.  These are later pulled out to
 *	make the true sphere.
 *
 *----------------------------------------------------------------------
 */

static void
fillSphereArray(int points)		/* Number of points in 90 degrees */
{
    int step;				/* How many segments in 90 degrees */
    double full_angle;			/* Degrees in the arc */
    double a_step, b_step;		/* Degrees per step across and up */
    int r, c;				/* Row and column */
    double a, b;			/* Angle across and up */
    double x, y, z;			/* The coordinates */
    double xz;				/* Scaling factor while going up */

/* Fill the array first with tesselated points */

    step = points - 1;			/* Segments around the arc */
    full_angle = 90.0;			/* Degrees in the arc */
    b_step = full_angle / (double) step; /* Degrees per step */
    r = 0;				/* Row */
    c = 0;				/* Column */
    for (b = 0.0; b <= (90.0 + DP_TOL); b += b_step) {
	a_step = full_angle / (double) step;
	y = sin(RAD(b));
	if (fabs(y) < DP_TOL)
	    y = 0.0;
	xz = cos(RAD(b));

	for (a = (90.0 + DP_TOL); a >= 0; a -= a_step) {
	    x = sin(RAD(a)) * xz;
	    if (fabs(x) < DP_TOL)
		x = 0.0;
	    z = cos(RAD(a)) * xz;	/* Note that 90 != 0.0 (fp error) */
	    if (fabs(z) < DP_TOL)
		z = 0.0;
	    xa[r][c] = (float)x;
	    ya[r][c] = (float)y;
	    za[r][c] = (float)z;
	    r += 1;
	}
	r = 0;				/* Start back at first row again */
	c += 1;				/* Move on to next column */
	step -= 1;			/* 1 less step each time */
    }

} /* End of fillSphereArray */



/*
 *----------------------------------------------------------------------
 *
 * drawStrip
 *
 *	Draw the 1/8 surface as colored triangle strips.
 *	The array contains points along the surface in a triangular form.
 *	Row zero contains "points" vertices, row one contains one less,
 *	and continues until row "points" contains just one vertex.
 *	A strip contains all vertices common to two rows and must start
 *	on the lower numbered row.
 *
 *----------------------------------------------------------------------
 */

static void
drawStrip(int points,			/* Number of points in 90 degrees */
    int x_pos,				/* Flag for negative X quadrant */
    int y_pos,				/* Flag for negative Y quadrant */
    int z_pos)				/* Flag for negative Z quadrant */
{
    int r, c;				/* Row and column */
    int p;				/* Temporary */
    float x, y, z;			/* The vertex value */
    int quadrant;			/* Which quadrant to do */
    int draw;

/* Pull data back out of the array to make triangle strips */

    p = points;				/* Point counter */
    r = 0;				/* Row */
    c = 0;				/* Column */
    quadrant = x_pos + (y_pos << 1) + (z_pos << 2);

    do {
	draw = 0;

	do {

	    /* Write out a point */
	    switch (quadrant) {
	      case 0:			/* +x +y +z */
		x = xa[r][c];
		y = ya[r][c];
		z = za[r][c];
		break;
	      case 1:			/* -x +y +z */
		x = -za[r][c];
		y = ya[r][c];
		z = xa[r][c];
		break;
	      case 2:			/* +x -y +z */
		x = ya[r][c];
		y = -xa[r][c];
		z = za[r][c];
		break;
	      case 3:			/* -x -y +z */
		x = -xa[r][c];
		y = -ya[r][c];
		z = za[r][c];
		break;
	      case 4:			/* +x +y -z */
		x = za[r][c];
		y = ya[r][c];
		z = -xa[r][c];
		break;
	      case 5:			/* -x +y -z */
		x = -za[r][c];
		y = xa[r][c];
		z = -ya[r][c];
		break;
	      case 6:			/* +x -y -z */
		z = -za[r][c];
		y = -ya[r][c];
		x = xa[r][c];
		break;
	      case 7:			/* -x -y -z */
		z = -xa[r][c];
		y = -ya[r][c];
		x = -za[r][c];
		break;
	    }

	    model->vertexList[model->vertexCount].draw = draw;
	    model->vertexList[model->vertexCount].x = x;
	    model->vertexList[model->vertexCount].y = y;
	    model->vertexList[model->vertexCount].z = z;
	    model->vertexList[model->vertexCount].nx = x;
	    model->vertexList[model->vertexCount].ny = y;
	    model->vertexList[model->vertexCount++].nz = z;
	    draw = 1;

	    /* Write out a point from the next row up */
	    switch (quadrant) {
	      case 0:
		x = xa[r + 1][c];
		y = ya[r + 1][c];
		z = za[r + 1][c];
		break;
	      case 1:
		x = -za[r + 1][c];
		y = ya[r + 1][c];
		z = xa[r + 1][c];
		break;
	      case 2:
		x = ya[r + 1][c];
		y = -xa[r + 1][c];
		z = za[r + 1][c];
		break;
	      case 3:
		x = -xa[r + 1][c];
		y = -ya[r + 1][c];
		z = za[r + 1][c];
		break;
	      case 4:
		x = za[r + 1][c];
		y = ya[r + 1][c];
		z = -xa[r + 1][c];
		break;
	      case 5:
		x = -za[r + 1][c];
		y = xa[r + 1][c];
		z = -ya[r + 1][c];
		break;
	      case 6:
		z = -za[r + 1][c];
		y = -ya[r + 1][c];
		x = xa[r + 1][c];
		break;
	      case 7:
		z = -xa[r + 1][c];
		y = -ya[r + 1][c];
		x = -za[r + 1][c];
		break;
	    }

	    model->vertexList[model->vertexCount].draw = draw;
	    model->vertexList[model->vertexCount].x = x;
	    model->vertexList[model->vertexCount].y = y;
	    model->vertexList[model->vertexCount].z = z;
	    model->vertexList[model->vertexCount].nx = x;
	    model->vertexList[model->vertexCount].ny = y;
	    model->vertexList[model->vertexCount++].nz = z;

	    c += 1;
	} while (c < p);		/* This exit is never actually taken */

	model->vertexCount--;

	p -= 1;				/* Less points this time across */
	r += 1;				/* On to next row */
	c = 0;				/* Beginning of column again */
    } while (r < (points - 1));
    model->vertexList[model->vertexCount].draw = 0;

    model->colorList[0].index = 0;
    model->colorList[0].ra = 0.07f;
    model->colorList[0].ga = 0.02f;
    model->colorList[0].ba = 0.12f;
    model->colorList[0].rd = 0.35f;
    model->colorList[0].gd = 0.1f;
    model->colorList[0].bd = 0.6f;
    model->colorList[0].rs = 1.0f;
    model->colorList[0].gs = 1.0f;
    model->colorList[0].bs = 1.0f;
    model->colorList[0].spec = 15.0f;

} /* End of drawStrip */



/*
 * buildSphere
 *
 *	create a sphere tessellated to tess.
 */

GLuint
buildSphere(int tess)
{
    glFrontFace(GL_CW);
    fillSphereArray(tess);

    model->haveNormals = 1;
    model->vertexCount = 0;

    drawStrip(tess, 1, 1, 1);
#if 1
    drawStrip(tess, 0, 1, 1);
    drawStrip(tess, 1, 0, 1);
    drawStrip(tess, 0, 0, 1);
    drawStrip(tess, 1, 1, 0);
    drawStrip(tess, 0, 1, 0);
    drawStrip(tess, 1, 0, 0);
    drawStrip(tess, 0, 0, 0);
#endif

    return (1);

} /* End of buildSphere */

/* End of common.c */
