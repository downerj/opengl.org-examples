/*
 * fileread.h
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
 *	Header for module which reads in various model formats.
 */

#ifndef FILE_READ_MODULE_HEADER
#define FILE_READ_MODULE_HEADER

void readLineData(char *FileName);
void readObjData(char *FileName);
FILE *fileOpen(const char *filename, const char *mode);
MaterialColor *readMaterials(void);

#endif

/* End of fileread.h */
