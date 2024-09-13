/*  
 *  CS 453 - Final project : An OpenGL version of the pegboard game IQ
 *  Due : June 5, 1997
 *  Author : Kiri Wagstaff
 *
 *  File : gliq.h
 *  Description : Main header file
 *
 */

#ifndef GLIQ_H
#define GLIQ_H

#include <stdio.h>
#include <math.h>
#include <GL/glut.h>
#include "trackball.h"

/* defines */
#define BOARDSIZE 9   /* on a side, total of 81 holes */
#define SELECT_BUFFER 256

/* enums */
enum {UNUSED, EMPTY, FULL, CANMOVE, CANTMOVE};    /* for each hole */
enum {SELBOARD, PLAY, HIGHSC, VIEWSCORES};        /* current state */
enum {NONE, LEFTARR=100, SELECT, RIGHTARR, QUIT}; /* board selection */

/* from gliq.c */
extern  int curstate;
extern  int lastpicked;
extern  int pegs;
extern  int totalpegs;
extern  void display();

/* from board.c */
extern  int*** boards;
extern  int    curboard;
extern  int    numboards;  
extern  int    filled[BOARDSIZE][BOARDSIZE];
extern  void   selectboard();
extern  void   readboards();
extern  void   drawboard();
extern  void   drawpegs();
extern  void   drawpeg();
extern  void   displaybuttons();

/* from game.c */
extern  int playdone;
extern  void playgame();
extern  int legalmove();
extern  int canmove(int peg);
extern  int movesexist();
extern  void drawquit(float x, float y, float r1, float r2);

/* from score.c */
extern  int minscore;
extern  int minpegs;
extern  int numentered;
extern  int written;
extern  void highscore();
extern  void readscores();
extern  void showhighscores();
extern  void keyscores(char key, int x, int y);
extern  void idlescore(void);

/* from pick.c */
extern  int       picked;
extern  GLuint    select_buffer[];
extern  GLboolean selection;
extern  GLuint    pick(int x, int y);
extern  void      passive(int x, int y);
extern  void      text(GLuint x, GLuint y, GLfloat scale, char *format, ...);

#endif
