/*
 * frustum_main.c
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
 *	Read in various Wavefront .OBJ models.  Allow them to be rotated
 *	and set various modes while the object moves.  Show view frustum
 *	in a second window and allow manipulation of the frustum by the user.
 *      'model' window contains the view of the model, 'view' window
 *      displays the view frustum of the model window graphically.
 */

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif

#include "frustum.h"
#include "common.h"

/* Data view and model windows */
ViewContext *view;
ModelContext *model;

int stereoMode = MONO;
GLboolean hasStereo = 37;	/* Flag: 1 = stereo hardware */

void displayHelp(void);



/*
 * main
 */

int
main(int argc, char *argv[])
{
    /* Display help listing on startup */
    displayHelp();

    /* Allocate both window contexts */
    view = (ViewContext *)calloc(1, sizeof (ViewContext));
    if (view == NULL) {
        printf ("Not enough memory count be allocated for view window\n");
        exit (-1);
    }
    model = (ModelContext *)calloc(1, sizeof(ModelContext));
    if (model == NULL) {
        printf ("Not enough memory count be allocated for model window\n");
        exit (-1);
    }

    /* Go grab the data before doing anything serious */
    readObjData("shuttle.obj");

    /* Initialize: */

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE); /* | GLUT_STEREO);*/

    viewInitWindow();
    modelInitWindow();

    glGetBooleanv(GL_STEREO, &hasStereo);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);


    glutIdleFunc(animate);

    glutMainLoop();

    return 0;

} /* End of main */



/*
 * display
 *
 *	Calls both windows' display functions (for a simultaineous update)
 */

void
display (void)
{
    glutSetWindow(model->windowIdent);
    modelDisplay();
    glutSetWindow(view->windowIdent);
    viewDisplay();
} /* End of display */



/*
 * animate
 *
 *	The animation function.
 */

void
animate(void)
{
    if (view->animateFlag == 1)
        view->needToUpdateViewMat = 1;
    if (model->animateFlag == 1)
        model->needToUpdateViewMat = 1;
    glutPostRedisplay();
} /* End of animate */



/*
 * displayInfo
 *
 *      Tells the user the current frustum info
 */

void
displayInfo(void)
{
    printf("------------------------------\n");
    printf("Current frustum configuration:\n\n");
    printf("Eyepoint (0, 0, %g)\n", model->eyeballZ);
    printf("Lookat point (0, 0, %g)\n", model->screenZ);
    printf("Left, right clipping planes: %g, %g\n", model->left, model->right);
    printf("Bottom, top clipping plane: %g, %g\n", model->bottom, model->top);
    printf("Near, far clipping plane (distance from eye): %g, %g\n",
        model->frontPlaneDist, model->backPlaneDist);
    printf("Near:far ratio: 1:%g\n",
        model->backPlaneDist / model->frontPlaneDist);
} /* End of displayInfo */



/*
 * displayHelp
 *
 *	Tells the user what they can do.
 */
void
displayHelp(void)
{
    printf("-------\n");
    printf("Frustum\n\n");
    printf("Mouse: Left   = motion\n");
    printf("       Middle = scale\n");
    printf("       Right  = menu\n");
    printf("Keys: +, -    = scale\n");
    printf("      [, ]    = translate far plane\n");
    printf("      ;, '    = translate near plane\n");
    printf("      <, >    = translate eye position\n");
    printf("      (, )    = translate screen position\n");
    printf("      o, p    = dec./inc. intereye distance\n");
    printf("      t, T    = toggle Stereo mode\n");
    printf("      f, F    = display current frustum data\n");
    printf("      ?, h, H = display this help menu\n");
    printf("      esc,q,Q = quit\n");
} /* End of displayHelp */

/* End of frustum_main.c */
