/*
 * frustum.h
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
 *	Header defining the types and constants used in the frustum demo
 */

#ifndef FRUSTUM_HEADER
#define FRUSTUM_HEADER

#include <GL/gl.h>

/* Array limits for vertex and color arrays */
#define VERTEX_MAX 100000
#define COLOR_MAX 100

/* Constants & initial values for frustum calculations */
#define FRONT_PLANE 2.0
#define BACK_PLANE 100.0
#define EYE_BACK 36.0
#define CAMERA_BACK 50.0
#define PIXELS_PER_INCH 100.0

/* Translation and scaling speeds related values */
#define TRANSLATE_VALUE 0.05f
#define SCREEN_SHIFT_VALUE 0.2	/* translation rate for the 'screen' plane */
#define SCALE_SPEED 0.003
#define MINIMUM_VIEW_SCALE 0.2
#define MINIMUM_MODEL_SCALE 0.5

/* Operation defines */
#define ROTATE 0
#define SCALE 1

/* Stereo mode defines */
#define MONO 0
#define LEFT 1
#define RIGHT 2

/* Menu definitions */
enum {
    DRAW_BEGIN,
    DRAW_SHUTTLE,
    DRAW_X29,
    DRAW_PORSCHE,
    DRAW_END,

    STEREO_ON,
    STEREO_OFF,

    QUIT
};


typedef struct Vertex Vertex;
struct Vertex {
	int draw;                       /* = 0 reset, else draw */
	GLfloat x;
	GLfloat y;
	GLfloat z;
	GLfloat nx;
	GLfloat ny;
	GLfloat nz;
};

typedef struct ColorStruct ColorStruct;
struct ColorStruct {
	int index;			/* Which vertex starts this color */
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

/* model data for 'view' window */
typedef struct ViewContext ViewContext;
struct ViewContext
{
    int windowIdent;

    int animateFlag;

    GLUquadric *eyeball;
    GLuint eyeballListNum;

    GLdouble rotX, rotY;
    GLdouble scaleObj;
    GLdouble rotMat[16];
    GLdouble windowWidth, windowHeight;
    GLdouble minimumScale;
    int op;

    GLdouble motionX, motionY;

    int needToUpdateViewMat;
    int pointerMotion;			/* Flag, multiple mouse moves/redraw */
};

/* model data for 'model' window */
typedef struct ModelContext ModelContext;
struct ModelContext
{
    int windowIdent;
    int menuIdent;
    int objectMenuIdent;
    int objectSetting;
    int stereoMenuNum;
    int stereoSetting;

    int animateFlag;

    Vertex vertexList[VERTEX_MAX];	/* Vertex list for the model */
    int vertexCount;

    Vertex objVertexList[VERTEX_MAX];	/* Vertex list for the model */
    int ovCount;
    int onCount;

    ColorStruct colorList[COLOR_MAX];	/* Color list for the model */
    int colorCount;

    GLdouble rotX, rotY;
    GLdouble scaleObj;
    GLdouble rotMat[16];
    GLdouble windowWidth, windowHeight;
    GLdouble minimumScale;
    int op;

    GLfloat halfEyeDist;                /* 1/2 dist. between eyes in stereo */

    /* Frustum plane locations, distances, etc. */
    GLdouble eyeballZ;
    GLdouble frontPlaneZ, backPlaneZ;
    GLdouble frontPlaneDist, backPlaneDist;
    GLdouble screenZ;
    GLdouble distanceAdjust;
    GLdouble left, right, bottom, top;

    GLdouble motionX, motionY;

    GLdouble rotAngle;
    GLdouble rotAxis[3];

    int haveNormals;
    int needToUpdateViewMat;
    int pointerMotion;			/* Flag, multiple mouse moves/redraw */
};


/* Functions for the view window, in frustum_view.c */
void viewInitWindow (void);
void viewDisplay(void);
void viewDrawImage(void);
void viewKeys(unsigned char key, int x, int y);
void viewMouse(int button, int state, int x, int y);
void viewSpecialKeys(int key, int x, int y);
void viewKeys(unsigned char key, int x, int y);
void viewMotion(int x, int y);
void viewVisibility(int state);
void viewReshape(int w, int h);
void viewSetView (void);

/* Functions for the model window, in frustum_model.c */
void modelInitWindow (void);
void modelDisplay(void);
void modelDrawImage(void);
void modelKeys(unsigned char key, int x, int y);
void modelMenu(int value);
void modelMouse(int button, int state, int x, int y);
void modelSpecialKeys(int key, int x, int y);
void modelKeys(unsigned char key, int x, int y);
void modelMotion(int x, int y);
void modelVisibility(int state);
void modelReshape(int w, int h);
void modelSetView (void);
void readObjData(char *FileName);

/* General functions in frustum_main.c */
void display(void);
void animate(void);

void displayInfo(void);

#endif /* FRUSTUM_HEADER */
