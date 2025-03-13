/*
 * common.h
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
 *	Header of common functions module for demo programs.
 */

#ifndef _COMMON_H_
#define _COMMON_H_

void setView(void);
void setColor(const GLfloat *ambient, const GLfloat *diffuse,
    const GLfloat *specular, GLfloat shininess, GLboolean stereo);
void setLights(int lights);
FILE *fileOpen(const char *filename, const char *mode);
void buildCube(void);
GLuint buildSphere(int tess);

#endif /* _COMMON_H_ */
