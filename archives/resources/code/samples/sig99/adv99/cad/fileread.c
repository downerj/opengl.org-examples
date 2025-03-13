/*
 * fileread.c
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
 *	Module which reads in various model formats.  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "modelcontext.h"
#include "fileread.h"

extern ModelContext *model;


/*
 * readLineData
 *
 *	Read the specified data file, convert to polyline list.
 */

void
readLineData(char *FileName)
{
    FILE *dataFile;
    char inputLine[256];
    GLfloat x, y, z;			/* To read input */
    GLfloat r, g, b;
    int count;

    dataFile = fileOpen(FileName, "r");
    if (dataFile == NULL)
	exit(1);

    model->haveNormals = 0;
    model->vertexCount = 0;
    model->colorCount = 0;
    model->colorList[0].index = -1;	/* In case there aren't any colors */

    /* Read in lines until end of file */
    while ((fgets(inputLine, 250, dataFile) != NULL) && (inputLine[0] != 'e')) {

	if (inputLine[0] == 'm') {
	    count = sscanf(inputLine, "m %f %f %f", &x, &y, &z);
	    model->vertexList[model->vertexCount].draw = 0;
	}

	if (inputLine[0] == 'd') {
	    count = sscanf(inputLine, "d %f %f %f", &x, &y, &z);
	    model->vertexList[model->vertexCount].draw = 1;
	}

	if (inputLine[0] == 'c') {
	    count = sscanf(inputLine, "c %f %f %f", &r, &g, &b);
	    model->colorList[model->colorCount].index = model->vertexCount;
	    model->colorList[model->colorCount].rd = r;
	    model->colorList[model->colorCount].gd = g;
	    model->colorList[model->colorCount].bd = b;
            model->colorList[model->colorCount].ad = 1.0;

	    /* Check array bounds */
	    if (model->colorCount + 1 < COLOR_MAX) {
		model->colorCount++;
	    }
	    else {
		/* Too many colors, print out error message and continue */
		fprintf (stderr, "Error, number of colors exceeds limit (%d)\n",
		    COLOR_MAX);
	    }
	    continue;
	}

	if (count != 3)
	    continue;

	model->vertexList[model->vertexCount].x = x;
	model->vertexList[model->vertexCount].y = y;
	model->vertexList[model->vertexCount].z = z;

	/* Check array bounds */
	if (model->vertexCount + 1 < VERTEX_MAX) {
	    model->vertexCount++;
	}
	else {
	    /* Too many vertices, bail out gracefully */
	    fprintf (stderr, "Error, number of vertices exceeds limit (%d)\n",
		VERTEX_MAX);
	    exit (-1);
	}
    }

    /* Mark last one as a move */
    model->vertexList[model->vertexCount].draw = 0;

    fclose(dataFile);
} /* End of readLineData */



/*
 * readObjData
 *
 *	Read a Wavefront .OBJ file and store as triangles.
 *	This version is far from complete and can't deal with all
 *	.OBJ files by a long ways.  This is NOT robust code!
 */

void
readObjData(char *FileName)
{
    FILE *dataFile;			/* The file we're reading */
    int count;				/* Count of number read on input */
    char inputLine[512];		/* Place to store one line of input */
    int ilLen;				/* Length of input line */
    GLfloat x, y, z;			/* A vertex coordinate */
    int facetVertex[100];		/* Index array for facets */
    int facetNormal[100];		/* For normals, if there are any */
    int fvCount;			/* How many vertices in the facet */
    int linePos, lp;			/* Position on input line */
    int i, j, k, m;			/* Iterative counters*/
    char materialName[128];		/* Name of material in .mtl file */
    MaterialColor *materials;		/* Pointer to material description */
    MaterialColor *matPtr;		/* Moving ptr to material description */
    int foundMatch;			/* Flag = 1 when a matched is found */
    int v0,v1;				/* Vertex number of last vertex */

   
    glFrontFace(GL_CCW);

    dataFile = fileOpen(FileName, "r");
    if (dataFile == NULL) {
	perror(FileName);
	exit(1);
    }
    materials = NULL;
    model->facetCount = 0;
    model->haveNormals = 0;
    model->ovCount = 1;			/* Vertices start at 1, not 0 */
    model->onCount = 1;
    model->vertexCount = 0;
    model->edgeCount = 0;
    model->lineCount = 0;
    model->lineStripCount = 0;
    model->colorCount = 0;
    model->colorList[0].index = -1;
    model->boundBoxLeft = 0;
    model->boundBoxRight = 0.0;
    model->boundBoxBottom = 0.0;
    model->boundBoxTop = 0.0;
    model->boundBoxNear = 0.0;
    model->boundBoxFar = 0.0;

    for(;;) {
	if (fgets(inputLine, 500, dataFile) == NULL)
	    break;			/* End of file */

	/* Get length of line, no trailing spaces */
	ilLen = strlen(inputLine);
	while ((ilLen > 0) && ((inputLine[ilLen - 1] == ' ') ||
		(inputLine[ilLen - 1] == '\n')))
	    ilLen--;

	if (inputLine[0] == 'v') {
	    /* Read one vertex and store it in the vertex array */

	    if (inputLine[1] == ' ') {
		/* A vertex */
		count = sscanf(inputLine, "v %f %f %f", &x, &y, &z);
		if (count != 3)
		    continue;
		model->objVertexList[model->ovCount].facetsNum = 0;
		model->objVertexList[model->ovCount].edgesNum = 0;
		model->objVertexList[model->ovCount].x = x;
		model->objVertexList[model->ovCount].y = y;
		model->objVertexList[model->ovCount++].z = z;

		/* Update bounding box */
		if (x < model->boundBoxLeft)
		    model->boundBoxLeft = x;
		if (x > model->boundBoxRight)
		    model->boundBoxRight = x;
		if (y < model->boundBoxBottom)
		    model->boundBoxBottom = y;
		if (y > model->boundBoxTop)
		    model->boundBoxTop = y;
		if (z < model->boundBoxNear)
		    model->boundBoxNear = z;
		if (z > model->boundBoxFar)
		    model->boundBoxFar = z;
	    }

	    else if (inputLine[1] == 'n') {
		/* A normal */
		count = sscanf(inputLine, "vn %f %f %f", &x, &y, &z);
		if (count != 3)
		    continue;
		model->objVertexList[model->onCount].nx = x;
		model->objVertexList[model->onCount].ny = y;
		model->objVertexList[model->onCount++].nz = z;
		model->haveNormals = 1;
	    }
	}

	else if (inputLine[0] == 'f') {
	    /* Read one facet, get its vertex coordinates and add to list */
	    fvCount = 0;
	    linePos = 2;
	    while (linePos < ilLen) {
		/* Get the next number */
		sscanf(&inputLine[linePos], "%d%n", &facetVertex[fvCount], &lp);
		if (inputLine[linePos + lp] == '/') {
		    /* We have normals to, assume "//" for now */
		    linePos += lp + 2;
		    sscanf(&inputLine[linePos], "%d%n", &facetNormal[fvCount], 
			&lp);
		}
		fvCount++;
		linePos += lp + 1;
	    }
	    if (fvCount < 3)
		continue;		/* Two vertex polygon?  Not! */
	    facetVertex[fvCount] = facetVertex[0];	/* Wrap at end */

	    /* Convert vertex numbers to XYZ, store in vertex data array */
	    for (i = 0; i < fvCount; i++) {
		/* Check edge i -- i+1 if either has duplicate facets in list */
		if (facetVertex[i] < facetVertex[i+1]) {
		    v0 = facetVertex[i];
		    v1 = facetVertex[i+1];
		}
		else {
		    v0 = facetVertex[i+1];
		    v1 = facetVertex[i];
		}
		foundMatch = 0;
		for (j = 0; j < model->objVertexList[v0].facetsNum; j++) {
		    for (k = 0; k < model->objVertexList[v1].facetsNum; k++) {
			if (model->objVertexList[v0].facets[j] ==
			    model->objVertexList[v1].facets[k]) {
			    /* Make sure shared facet isn't this one */
			    if (model->objVertexList[v0].facets[j] != 
				    model->facetCount) {
			    	/* A match, these vertices already share a */
  			    	/* facet and therefore an edge. */
			    	foundMatch = 1;
			    	break;
			    }
			}
		    }
		    if (foundMatch)
			break;
		}

		/* If no match found, its a new edge, add it to the edgelist */
		if (foundMatch == 0) {

		    /* Update vertices to show the edge for future checks */
		    model->objVertexList[v0].edges[ 
			model->objVertexList[v0].edgesNum++] = model->edgeCount;
		    model->objVertexList[v1].edges[
			model->objVertexList[v1].edgesNum++] = model->edgeCount;

		    model->edgeList[model->edgeCount][0] = v0;
		    model->edgeList[model->edgeCount][1] = v1;
		    model->edgeCount++;
		}

		/* Add this facet to the vertices' records */
		model->objVertexList[v0].facets[
		    model->objVertexList[v0].facetsNum++] = model->facetCount;

		model->objVertexList[v1].facets[
		    model->objVertexList[v1].facetsNum++] = model->facetCount;

		/* Create 'proper' vertex structure, sans edge and facet data */
		/* 0 is move, non-zero is draw */
		j = i;

		/* Read vertices as triangle strips or polys? */
		if (model->triangleFlag) {
		     if (j > 0) {
		         if ((i & 1) == 1)
			     j = (i + 1) >> 1;
		         else
			     j = fvCount - (i >> 1);
		     }
		}
		model->vertexList[model->vertexCount].draw = i;
		model->vertexList[model->vertexCount].x = 
		    model->objVertexList[facetVertex[j]].x;
		model->vertexList[model->vertexCount].y = 
		    model->objVertexList[facetVertex[j]].y;
		model->vertexList[model->vertexCount].z = 
		    model->objVertexList[facetVertex[j]].z;
		if (model->haveNormals) {
		    model->vertexList[model->vertexCount].nx = 
			model->objVertexList[facetNormal[j]].nx;
		    model->vertexList[model->vertexCount].ny = 
			model->objVertexList[facetNormal[j]].ny;
		    model->vertexList[model->vertexCount].nz = 
			model->objVertexList[facetNormal[j]].nz;
		}
		model->objVertexList[facetVertex[j]].vertexIndex = 
		    model->vertexCount;
		model->vertexCount++;
	    }
	    model->facetCount++;
	}

	else if (inputLine[0] == 'l') {
	    /* A line, do this some day */
	}
	else if (inputLine[0] == 'p') {
	    /* A point, do this some day */
	}
	else if (inputLine[0] == 's') {
	    /* A smoothing value */
	}
	else if (inputLine[0] == 'u') {
	    /* Probably usemtl, need to add that too */
	    if (strncmp("usemtl", inputLine, 6) != 0)
		continue;		/* "Who knows" what this is */
	    if (materials == NULL)
		materials = readMaterials();

	    sscanf(&inputLine[7], "%s", materialName);
	    matPtr = materials;
	    while ((strcmp(materialName, matPtr->name) != 0) &&
		    (matPtr->name[0] != 0))
		matPtr++;
	    if (matPtr->name[0] == 0)
		fprintf(stderr, "Can't find %s\n", materialName);

	    /* We have a pointer to the right material */
	    model->colorList[model->colorCount].index = model->vertexCount;
	    model->colorList[model->colorCount].ra = matPtr->ra;
	    model->colorList[model->colorCount].ga = matPtr->ga;
	    model->colorList[model->colorCount].ba = matPtr->ba;
	    model->colorList[model->colorCount].rd = matPtr->rd;
	    model->colorList[model->colorCount].gd = matPtr->gd;
	    model->colorList[model->colorCount].bd = matPtr->bd;
	    model->colorList[model->colorCount].ad = matPtr->ad;
	    model->colorList[model->colorCount].rs = matPtr->rs;
	    model->colorList[model->colorCount].gs = matPtr->gs;
	    model->colorList[model->colorCount].bs = matPtr->bs;
	    model->colorList[model->colorCount].spec = matPtr->spec;
	    model->colorCount++;
	}
	/* Else, a line we can ignore */
    }

    /* Mark last one as a move */
    model->vertexList[model->vertexCount].draw = 0;

    /* Set the 1st index after the end of the list (0..colorCount-1 is */
    /* the list) equal to an out-of-bounds index, so that the following */
    /* color picking algorithms will  work correctly */
    model->colorList[model->colorCount].index = model->vertexCount + 1;

    /* Store the independent line data too */
    model->lineCount = 0;
    for (i = 0; i < model->edgeCount; i++) {
	/* Look up the color for this edge */
	model->lineList[model->lineCount].colorIndex = 0;
	for (j = 0; j <= model->colorCount; j++) {
	    if (model->objVertexList[model->edgeList[i][0]].vertexIndex <
		    model->colorList[j].index) {
		model->lineList[model->lineCount].colorIndex = j-1;
		break;
	    }
	}

	model->lineList[model->lineCount].draw = 0;
	model->lineList[model->lineCount].x = 
	    model->objVertexList[model->edgeList[i][0]].x;
	model->lineList[model->lineCount].y = 
	    model->objVertexList[model->edgeList[i][0]].y;
	model->lineList[model->lineCount].z = 
	    model->objVertexList[model->edgeList[i][0]].z;
	model->lineCount++;

	model->lineList[model->lineCount].draw = 1;
	model->lineList[model->lineCount].x = 
	    model->objVertexList[model->edgeList[i][1]].x;
	model->lineList[model->lineCount].y = 
	    model->objVertexList[model->edgeList[i][1]].y;
	model->lineList[model->lineCount].z = 
	    model->objVertexList[model->edgeList[i][1]].z;
	model->lineCount++;
    }
    model->lineList[model->lineCount].draw = 0;

/* Create the line strip data (very intelligent algorithm) */
{
    int edgeBase;			/* Lowest edge that might be valid */

    model->lineStripCount = 0;
    edgeBase = 0;
    do {
	/* Skip to the next edge to start from */
	while (model->edgeList[edgeBase][0] == -1)
	    edgeBase++;
	if (edgeBase >= model->edgeCount)
	    break;			/* Exit here */

	/* Start off with this line */
	i = edgeBase;

	v0 = model->edgeList[i][0];
	v1 = model->edgeList[i][1];

	/* Look up the color for this edge */
	model->lineStripList[model->lineStripCount].colorIndex = 0;
	for (j = 0; j <= model->colorCount; j++) {
	    if (model->objVertexList[v0].vertexIndex <=
		    model->colorList[j].index) {
		model->lineStripList[model->lineStripCount].colorIndex= j-1;
		break;
	    }
	}

	model->lineStripList[model->lineStripCount].draw = 0;
	model->lineStripList[model->lineStripCount].x = 
	    model->objVertexList[v0].x;
	model->lineStripList[model->lineStripCount].y = 
	    model->objVertexList[v0].y;
	model->lineStripList[model->lineStripCount].z = 
	    model->objVertexList[v0].z;
        model->lineStripCount++;

	model->lineStripList[model->lineStripCount].draw = 1;
	model->lineStripList[model->lineStripCount].x =
            model->objVertexList[v1].x;
	model->lineStripList[model->lineStripCount].y = 
	    model->objVertexList[v1].y;
	model->lineStripList[model->lineStripCount].z = 
	    model->objVertexList[v1].z;
	model->lineStripCount++;

	/* Clear first two vertices' reference to this edge */
	for (j=0; j<model->objVertexList[v0].edgesNum; j++) {
	    if (model->objVertexList[v0].edges[j] == edgeBase) {
	    	model->objVertexList[v0].edges[j] = -1;
		break;
	    }
	}
	for (j=0; j<model->objVertexList[v1].edgesNum; j++) {
	    if (model->objVertexList[v1].edges[j] == edgeBase) {
	    	model->objVertexList[v1].edges[j] = -1;
		break;
	    }
	}

	model->edgeList[i][0] = -1;		/* No longer valid data */	

	/* Look in v1's neighbors for a free line */
	do {

	    foundMatch = 0;

	    for (j=0; j<model->objVertexList[v1].edgesNum; j++) {
		/* Look for an unused edge */
		if (model->objVertexList[v1].edges[j] != -1) { 
		    if (model->edgeList[model->objVertexList[v1].edges[j]][0] 
			    != -1) {
		        foundMatch = 1;
		        k = model->objVertexList[v1].edges[j];
		        model->objVertexList[v1].edges[j] = -1;
		        if (model->edgeList[k][0] == v1) {
			    /* Look up the color for this edge */
			    model->lineStripList[
			        model->lineStripCount].colorIndex = 0;
			    for (m = 0; m <= model->colorCount; m++) {
			        if (model->objVertexList[
				        model->edgeList[k][1]].vertexIndex <=
				        model->colorList[m].index) {
				    model->lineStripList[
				        model->lineStripCount].colorIndex= m-1;
				    break;
			        }
			    }

  		 	    model->lineStripList[model->lineStripCount].draw 
				= 1;
			    model->lineStripList[model->lineStripCount].x = 
			        model->objVertexList[model->edgeList[k][1]].x;
			    model->lineStripList[model->lineStripCount].y = 
			        model->objVertexList[model->edgeList[k][1]].y;
			    model->lineStripList[model->lineStripCount].z = 
			        model->objVertexList[model->edgeList[k][1]].z;
			    model->lineStripCount++;
			    v1 = model->edgeList[k][1];
		        }
		        else {
			    /* Look up the color for this edge */
			    model->lineStripList[
			        model->lineStripCount].colorIndex = 0;
			    for (m = 0; m <= model->colorCount; m++) {
			        if (model->objVertexList[
				        model->edgeList[k][0]].vertexIndex <=
				        model->colorList[m].index) {
				    model->lineStripList[
				        model->lineStripCount].colorIndex= m-1;
				    break;
			        }
			    }

		  	    model->lineStripList[model->lineStripCount].draw 
				= 1;
			    model->lineStripList[model->lineStripCount].x = 
			        model->objVertexList[model->edgeList[k][0]].x;
			    model->lineStripList[model->lineStripCount].y = 
			        model->objVertexList[model->edgeList[k][0]].y;
			    model->lineStripList[model->lineStripCount].z = 
			        model->objVertexList[model->edgeList[k][0]].z;
			    model->lineStripCount++;
		  	    v1 = model->edgeList[k][0];
		        }
		        model->edgeList[k][0] = -1; /* No longer valid data */
		        break;
		    }
	        } 
            }  
	} while (foundMatch != 0);
    } while (edgeBase < model->edgeCount);
    model->lineStripList[model->lineStripCount].draw = 0;
}

    fclose(dataFile);
    free(materials);
} /* End of readObjData */



/*
 * fileOpen
 *
 * 	Attempts to fopen the file in many locations, dictated by an internal
 *	paths array.  fileOpen returns the first successful opened file in
 *	the path or NULL if the file was not found.
 *
 * Assumes strlen(path + filename + NULL) < 256
 */
FILE *fileOpen(const char *filename,
    const char *mode) 
{
    static char *paths[] = {
	"./",
	"../../data/",
	NULL
	};
    char fullFilename[256];
    int index;
    FILE *retVal;
    
    for (index=0; paths[index] != NULL; index++) {
	strcpy (fullFilename, paths[index]);
	strcat (fullFilename, filename);
	retVal = fopen(fullFilename, mode);
        if (retVal != NULL)
	    break;
    }

    return retVal;
}



/*
 * readMaterials
 *
 *	Read the contents of the materials file
 */

MaterialColor *
readMaterials(void)
{
    FILE *mtlFile;			/* The material file we're reading */
    char inputLine[256];		/* Read a line here */
    MaterialColor *matPtr;		/* Pointer to allocated space */
    int i;				/* Index into array */
    int count;				/* How many values read in */
    GLfloat r, g, b;			/* To read colors into */
    GLfloat spec;			/* To read specular value into */

    mtlFile = fileOpen("materials.mtl", "r");
    if (mtlFile == NULL) {
	fprintf(stderr, "Error, could not open 'materials.mtl', exiting.\n");
	exit(1);
    }

    matPtr = (MaterialColor *)malloc(100 * sizeof(MaterialColor));

    i = -1;
    for(;;) {
	if (fgets(inputLine, 250, mtlFile) == NULL)
	    break;			/* End of file */

	if (strncmp("newmtl", inputLine, 6) == 0) {
	    i++;
	    sscanf(&inputLine[7], "%s", matPtr[i].name);
	}
	else if (strncmp("Ka", inputLine, 2) == 0) {
	    count = sscanf(inputLine, "Ka %f %f %f", &r, &g, &b);
	    if (count != 3)
		continue;
	    matPtr[i].ra = r;
	    matPtr[i].ga = g;
	    matPtr[i].ba = b;
	}
	else if (strncmp("Kd", inputLine, 2) == 0) {
	    count = sscanf(inputLine, "Kd %f %f %f", &r, &g, &b);
	    if (count != 3)
		continue;
	    matPtr[i].rd = r;
	    matPtr[i].gd = g;
	    matPtr[i].bd = b;
	    matPtr[i].ad = 1.0;
	}
	else if (strncmp("Ks", inputLine, 2) == 0) {
	    count = sscanf(inputLine, "Ks %f %f %f", &r, &g, &b);
	    if (count != 3)
		continue;
	    matPtr[i].rs = r;
	    matPtr[i].gs = g;
	    matPtr[i].bs = b;
	}
	else if (strncmp("Ns", inputLine, 2) == 0) {
	    count = sscanf(inputLine, "Ns %f", &spec);
	    if (count != 1)
		continue;
	    matPtr[i].spec = spec;
	}
    }
    i++;

    matPtr[i].name[0] = 0;		/* After last valid entry */
    matPtr[i].ra = 0.0f;
    matPtr[i].ga = 0.1f;
    matPtr[i].ba = 0.0f;
    matPtr[i].rd = 0.2f;
    matPtr[i].gd = 1.0f;
    matPtr[i].bd = 0.0f;
    matPtr[i].ad = 1.0f;
    matPtr[i].rs = 1.0f;
    matPtr[i].gs = 0.8f;
    matPtr[i].bs = 0.0f;
    matPtr[i].spec = 25.0f;

    return matPtr;
} /* End of readMaterials */



/* End of fileread.c */
