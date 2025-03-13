/* 
 * modelcontext.h
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
 *	Header defining the ModelContext struct and related types and
 *      constants.  ModelContext is a struct defining a model and its context
 *      and is used in several of the demo programs.  Each program will most
 *      likely not use all of the functionality defined here.
 */

#ifndef MODEL_CONTEXT_HEADER
#define MODEL_CONTEXT_HEADER

#include <GL/glut.h>

/* Sizes of the mammoth arrays, must be big enough for the desired model, 
 * since bounds checking is lax
 */
#define VERTEX_MAX 66000
#define LINE_MAX 29000
#define LINE_STRIP_MAX 18000
#define EDGE_MAX 15000
#define OBJ_VERTEX_MAX 8500
#define COLOR_MAX 100

/* Constants defining the parameters of scaling behaviour */
#define SCALE_SPEED 0.003
#define MINIMUM_SCALE 0.5

/* Constants used in calculating the view frustum */
#define PIXELS_PER_INCH 100.0
#define EYE_BACK 36.0


typedef struct Vertex Vertex;
struct Vertex {
	int draw;                       /* = 0 reset, else draw */
	GLfloat x;
	GLfloat y;
	GLfloat z;
	GLfloat nx;
	GLfloat ny;
	GLfloat nz;

	int colorIndex;	
	int vertexIndex;		/* in 'objVertexList' to point */
					/* back to the official vertex */
	int facets[90];
	int facetsNum;

	int edges[60];
	int edgesNum;
};

typedef struct ColorStruct ColorStruct;
struct ColorStruct {
	int index;          	/* Which vertex starts this color */
	GLfloat ra, ga, ba;
	GLfloat rd, gd, bd, ad;
	GLfloat rs, gs, bs;
	GLfloat spec;
};

typedef struct MaterialColor MaterialColor;
struct MaterialColor {
    char name[100];
    GLfloat ra, ga, ba;			/* Ambient */
    GLfloat rd, gd, bd, ad;		/* Diffuse */
    GLfloat rs, gs, bs;			/* Specular */
    GLfloat spec;			/* Specular power (shininess) */
};

typedef struct ModelContext ModelContext;
struct ModelContext
{
    int facetCount;                     /* count of facets in model */

    int triangleFlag;			/* Flag to read in vertices as */
					/* triangle strips, not polygons */
    Vertex vertexList[VERTEX_MAX];	/* Vertex list for the model */
    int vertexCount;

    Vertex lineList[LINE_MAX];		/* Array of vertices for lines */
    int lineCount;			/* How many vertices */

    Vertex lineStripList[LINE_STRIP_MAX]; /* Vertices for line strips */
    int lineStripCount;			/* How many vertices */

    int edgeList[EDGE_MAX][2];
    int edgeCount;

    Vertex objVertexList[OBJ_VERTEX_MAX]; /* Vertices from .obj file */
    int ovCount;
    int onCount;

    ColorStruct colorList[COLOR_MAX];	/* Color list for the model */
    int colorCount;

    /* The bounding box of the model */
    GLfloat boundBoxLeft, boundBoxBottom, boundBoxNear;
    GLfloat boundBoxRight, boundBoxTop, boundBoxFar;

    GLdouble rotX, rotY;
    GLdouble scaleObj;
    GLdouble rotMat[16];
    GLdouble windowWidth, windowHeight;
    GLdouble minimumScale;

    int haveNormals;
    int needToUpdateViewMat;
    int pointerMotion;			/* Flag, multiple mouse moves/redraw */
};

#endif /* MODEL_CONTEXT_HEADER */
