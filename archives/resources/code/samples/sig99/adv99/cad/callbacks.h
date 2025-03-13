/* 
 * callbacks.h
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
 * 	Header of generic callbacks module for demo programs.
 */

#ifndef GENERIC_CALLBACKS_HEADER
#define GENERIC_CALLBACKS_HEADER

void mouseCallback(int button, int state, int x, int y);
void specialKeysCallback(int key, int x, int y);
void motionCallback(int x, int y);
void animateCallback(void);
void visibilityCallback(int state);
void reshapeCallback(int w, int h);

#endif /* GENERIC_CALLBACKS_HEADER */
