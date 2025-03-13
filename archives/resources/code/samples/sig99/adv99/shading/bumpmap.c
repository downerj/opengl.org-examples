/* bump.c - David G Yu, SGI */

/*
** Demonstrates a simple embossing technique which approximates bump mapping.
** 
** INTRODUCTION
**
** Bump mapping is a surface shading technique which can be used to increase
** apparent surface complexity.
**
** A bump map is an image in which each pixel represents the magnitude of
** displacement away from the surface being shaded, i.e. a displacement in
** the direction of the surface normal vector.  Traditionally, a bump map
** is applied during per-pixel lighting evaluation.  At each pixel, the
** bump map is sampled and the gradient of the displacement values at the
** sample position is computed.  This gradient corresponds to a displacement
** vector which is used to perturb the lighting normal.  When lighting is
** evaluated using the perturbed normal vector, the effect is as if the
** surface itself had been displaced by the bump map.
**
** These notes describe an alternative implementation which uses texture
** mapping and multiple rendering passes instead of per-pixel lighting.
**
** DESCRIPTION
**
** Ordinary lighting:
**    N -- lighting normal vector
**    L -- light direction vector
**
**    diffuse = N dot L
**
** Bump map lighting:
**    d  -- displacement vector
**    N' -- perturbed lighting normal vector
**
**           N + d
**    N' = ---------
**         | N + d |
**
**    diffuse' = N' dot L
**
** This can be simplified if we assume that the displacement is small, since
** we can skip the re-normalization of the lighting normal vector:
**    N' = N + d
**
**    diffuse' = (N + d) dot L
**             = (N dot L) + (d dot L)
**
** We observe that the term (N dot L) is just ordinary lighting.  Next we 
** show how to evaluate (d dot L).
**
** We will simplify the evaluation of (d dot L) by transforming it into
** the tangent space of the point on the surface being shaded.  The tangent
** space is defined by the lighting normal vector and two additional
** orthogonal vectors: the tangent vector and binormal vector:
**    N -- lighting normal vector
**    T -- tangent vector
**    B -- binormal vector
**
**    B = N cross T
**
** In tangent space, we assume that the axes of the bump map are aligned
** with the tangent and binormal vectors.  Because of this, the component
** of the displacement vector aligned with the normal vector will be small.
** If we assume that this component is zero, the evaluation of (d dot L) can
** be further simplified:
**    d_tan -- displacement vector (in tangent space)
**    L_tan -- light direction vector (in tangent space)
**    d dot L = d_tan dot L_tan
**            = d_tan_x*L_tan_x + d_tan_y*L_tan_y + 0*L_tan_z
**            = d_tan_x*L_tan_x + d_tan_y*L_tan_y
**
** To evaluate this expression we need to find the displacement vector
** and light direction vector in tangent space.
**
** The light direction vector can be transformed to tangent space using
** a matrix with elements from the three tangent space basis vectors:
**    M_tan -- matrix to transform object space to tangent space
**    M_tan = T_x B_x N_x
**            T_y B_y N_y
**            T_z B_z N_z
**    L_tan = L M_tan
**
** The displacement vector at a given (u,v) can be computed from the
** function defined by the bump map displacement values:
**    bump_map[u,v] -- bump map image
**    p(u,v)        -- displacement function
**
**    p(u,v) = bump_map[u,v]
**
** We can consider each component separately:
**    pu(u) = bump_map[u]
**    pv(v) = bump_map[v]
**
** The gradient of the displacement values at (u,v) is the slope of
** the displacement function.  The displacement vector is the negative
** gradient.  It's negative because we are displacing the surface normal
** vector, when the surface slopes upward, the surface normal vector
** should be displaced backward:
**                dpu
**    d_tan_x = - ---
**                du
**
**                dpv
**    d_tan_y = - ---
**                dv
**
** A brief digression concerning the computation of the slope of a function:
**    given some function:
**        y = f(x) 
**
**    its slope is:
**        dy   f(x+dx) - f(x)
**        -- = --------------
**        dx         dx
**
**    if we want to scale the slope:
**            dy   k * f(x+dx) - f(x)
**        k * -- = ------------------
**            dx           dx
**
**    if we let dx == k then:
**            dy   k * f(x+k) - f(x)
**        k * -- = ------------------
**            dx           k
**
**               = f(x+k) - f(x)
**
** Therefore, we can re-write the displacement vector equations as:
**                pu(u + du) - pu(u)
**    d_tan_x = - ------------------
**                        du
**
**                pu(u) - pu(u + du)
**            =   ------------------
**                        du
**
**                pv(v + dv) - pu(v)
**    d_tan_y = - ------------------
**                        dv
**
**                pv(v) - pv(v + dv)
**            =   ------------------
**                        dv
**
** Furthermore, we can re-write the expression for (d dot L) as:
**    d dot L = d_tan_x*L_tan_x + d_tan_y*L_tan_y
**
**                         pu(u) - pu(u + du)
**            = L_tan_x *  ------------------ +
**                                 du
**
**                         pv(v) - pu(v + dv)
**              L_tan_y *  ------------------
**                                 dv
**
**            = pu(u) - pu(u + L_tan_x) + pv(v) - pu(v + L_tan_y)
**
** Instead of computing (d dot L) at each pixel (u,v), we will compute
** perturbed texture coordinates at each vertex (s,t) and use texture
** mapping to interpolate between vertices:
**    w -- bump map width
**    h -- bump map height
**
**            1
**    s = u * - 
**            w
**
**            1
**    t = v * - 
**            h
**
** Then, substituting (s,t) for (u,v) we have:
**                             L_tan_x                   L_tan_y
**    d dot L = pu(s) - pu(s + -------) + pv(t) - pu(t + -------)
**                                w                         h
**
** Putting it all together we have:
**
**    diffuse' = (N dot L) +
**                              L_tan_x                   L_tan_y
**               pu(s) - pu(s + -------) + pv(t) - pu(t + -------)
**                                 w                         h
**        
** ALGORITHM
**
** We can evaluate this using three rendering passes:
**
**   1) render using lighting and no texturing
**
**      (N dot L)
**
**   2) render using texturing and no lighting (use unshifted texture coords),
**      add this to the result from the previous pass
**
**      + texture(s, t)
**
**   2) render using texturing and no lighting (use shifted texture coords),
**      subtract this from the result of the previous two passes
**
**
**                    L_tan_x      L_tan_y
**      - texture(s + -------, t + -------)
**                       w            h
**
** Pass 1) computes (N dot L), passes 2) and 3) compute (d dot L).  The
** result is accumulated in the framebuffer.
**
** IMPLEMENTATION NOTES
**
** Most of the code in this sample program is just a demo harness.  The
** code which pertains to the bump mapping algorithm is contained in the
** functions:
**    perturbTexCoords() and drawObject()
**
** For the most part, this sample program was not written to be optimal,
** rather it was written to make the implementation obvious.
**
** The specific rendering passes used by this program are just one example
** of how the bump map lighting equation can be evaluated.  With an OpenGL
** implementation that supports an additive texture environment or multi-
** texture, the three rendering passes shown here can be collapsed into
** two or even one rendering pass.
**
** Additional rendering passes can be used to correct for some of the
** simplifying assumptions made in the discsussion above, or to add the
** effects of specular lighting.
**
** See the references for additional information, then experiment on your
** own.
**
** References:
**    J. F. Blinn, Simulation of Wrinkled Surfaces,
**    Proceedings of SIGGRAPH '78, 1978, pp 286-292
**    -- bump mapping
**
**    Mark Peercy, John Airey, Brian Cabral, Efficient Bump Mapping Hardware,
**    Proceedings of SIGGRAPH '97, 1997, pp 303-306
**    -- tangent space bump mapping
**
**    John Schlag, Fast Embossing Effects on Raster Image Data,
**    Graphics Gems IV, 1994, pp 433-437
**    -- image space embossing effects
**
**    Tom McReynolds, David Blythe, Advanced Graphics Programming Techniques
**    Using OpenGL, SIGGRAPH '98 Course Notes, 1998, pp 123-131
**    -- a different description of this algorithm
**
** cc -g -o bump bump.c -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm
** cl -DWIN32 -Zi bump.c ..\util\texture.c -I..\util glut32.lib glu32.lib opengl32.lib gdi32.lib user32.lib
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>
#include "texture.h"

#if !defined(GL_EXT_blend_subtract)
#define glBlendEquationEXT(mode)
#endif

#if defined(_WIN32) && defined(GL_EXT_blend_subtract)
#include <windows.h>  /* get wglGetProcAddress prototype */
PFNGLBLENDEQUATIONEXTPROC glBlendEquationEXT;
#endif

#ifndef M_PI
#define M_PI 3.14159265F
#endif

/************************************************************/
/* Utility Functions */
/************************************************************/

GLboolean
extensionSupported(char *extName, const char *extString)
{
    const char *p = extString;

    while ((p = strstr(p, extName)) != NULL) {
	const char *q = p + strlen(extName);
	if (*q == ' ' || *q == '\0') {
	    return GL_TRUE;
	}
    }
    return GL_FALSE;
}

/************************************************************/
/* Vector Operators */
/************************************************************/

/* returns v2 dot v1 */
GLfloat
vecDot(GLfloat v2[4], GLfloat v1[4])
{
    return (v2[0]*v1[0] + v2[1]*v1[1] + v2[2]*v1[2]);
}

/* v3 = v2 cross v1 */
void
vecCross(GLfloat v3[4], GLfloat v2[4], GLfloat v1[4])
{
    GLfloat vx = v2[1]*v1[2] - v2[2]*v1[1];
    GLfloat vy = v2[2]*v1[0] - v2[0]*v1[2];
    GLfloat vz = v2[0]*v1[1] - v2[1]*v1[0];

    v3[0] = vx;
    v3[1] = vy;
    v3[2] = vz;
    v3[3] = 1.0F;
}

/* normalizes v */
void
vecNormalize(GLfloat v[4])
{
    GLfloat len = (GLfloat) sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    GLfloat d = (len != 0.0F) ? 1.0F / len : 1.0F;

    v[0] *= d;
    v[1] *= d;
    v[2] *= d;
    v[3] = 1.0F;
}

/************************************************************/
/* Matrix Operators (transposed to match OpenGL) */
/************************************************************/

typedef GLfloat Matrix[4][4];

/* mat = identity */
void
matrixIdentity(Matrix mat)
{
    mat[0][0] = 1.0F; mat[1][0] = 0.0F; mat[2][0] = 0.0F; mat[3][0] = 0.0F;
    mat[0][1] = 0.0F; mat[1][1] = 1.0F; mat[2][1] = 0.0F; mat[3][1] = 0.0F;
    mat[0][2] = 0.0F; mat[1][2] = 0.0F; mat[2][2] = 1.0F; mat[3][2] = 0.0F;
    mat[0][3] = 0.0F; mat[1][3] = 0.0F; mat[2][3] = 0.0F; mat[3][3] = 1.0F;
}

/* mat2 = mat1 */
void
matrixCopy(Matrix mat2, Matrix mat1)
{
    int i, j;

    for (i=0; i<4; ++i) {
	for (j=0; j<4; ++j) {
	    mat2[i][j] = mat1[i][j];
	}
    }
}

/* mat2 = transpose(mat1) (mat2 can be the same as mat1) */
void
matrixTranspose(Matrix mat2, Matrix mat1)
{
    Matrix tmp;
    int i, j;

    for (i=0; i<4; ++i) {
	for (j=0; j<4; ++j) {
	    tmp[i][j] = mat1[j][i];
	}
    }
    matrixCopy(mat2, tmp);
}

/* mat3 = mat2 * mat1 (mat3 can be the same as mat2 or mat1) */
void
matrixMultiply(Matrix mat3, Matrix mat2, Matrix mat1)
{
    Matrix tmp;
    int i, j;

    for (i=0; i<4; ++i) {
	for (j=0; j<4; ++j) {
	    tmp[i][j] = mat2[0][j]*mat1[i][0] +
			mat2[1][j]*mat1[i][1] +
			mat2[2][j]*mat1[i][2] +
			mat2[3][j]*mat1[i][3];
	}
    }
    matrixCopy(mat3, tmp);
}

/* v2 = matrix * v1 (v1 can be the same as v2) */
void
matrixTransform44(GLfloat v2[4], Matrix mat, GLfloat v1[4])
{
    GLfloat tmp[4];
    int i;

    for (i=0; i<4; ++i) {
       tmp[i] = mat[0][i]*v1[0] +
		mat[1][i]*v1[1] +
		mat[2][i]*v1[2] +
		mat[3][i]*v1[3];
    }

    v2[0] = tmp[0];
    v2[1] = tmp[1];
    v2[2] = tmp[2];
    v2[3] = tmp[3];
}

/* inv = invert(mat) (inv can be the same as mat) */
void
matrixInvert(Matrix inv, Matrix mat)
{
    Matrix tmp;
    GLfloat aug[5][4];
    int h, i, j, k;

    for (h=0; h<4; ++h) {
        for (i = 0; i<4; i++) {
            aug[0][i] = mat[0][i];
            aug[1][i] = mat[1][i];
            aug[2][i] = mat[2][i];
            aug[3][i] = mat[3][i];
            aug[4][i] = (h == i) ? 1.0F : 0.0F;
        }

        for (i=0; i<3; ++i) {
            GLfloat pivot = 0.0F;
	    int pivotIndex;

            for (j=i; j<4; ++j) {
                GLfloat temp = aug[i][j] > 0.0F ? aug[i][j] : -aug[i][j];
                if (pivot < temp) {
                    pivot = temp;
                    pivotIndex = j;
                }
            }
            if (pivot == 0.0F) {
		return; /* matrix is singular */
	    }

            if (pivotIndex != i) {
                for (k=i; k<5; ++k) {
                    GLfloat temp = aug[k][i];
                    aug[k][i] = aug[k][pivotIndex];
                    aug[k][pivotIndex] = temp;
                }
            }

            for (k=i+1; k<4; ++k) {
                GLfloat q = -aug[i][k] / aug[i][i];
                aug[i][k] = 0.0F;
                for (j=i+1; j<5; ++j) {
                    aug[j][k] = q * aug[j][i] + aug[j][k];
                }
            }
        }

        if (aug[3][3] == 0.0F) {
	    return; /* matrix is singular */
	}

        tmp[h][3] = aug[4][3] / aug[3][3];

        for (k=1; k<4; ++k) {
            GLfloat q = 0.0F;
            for (j=1; j<=k; ++j) {
                q = q + aug[4-j][3-k] * tmp[h][4-j];
            }
            tmp[h][3-k] = (aug[4][3-k] - q) / aug[3-k][3-k];
        }
    }
    matrixCopy(inv, tmp);
}

void
matrixRotate(Matrix mat, GLfloat a, GLfloat ax, GLfloat ay, GLfloat az)
{
    float degreesToRadians = M_PI / 180.0F;
    float rad = a * degreesToRadians;
    float c = (float) cos(rad);
    float s = (float) sin(rad);
    float len = (float) sqrt(ax*ax + ay*ay + az*az);
    float d = (len != 0.0F) ? 1.0F / len : 1.0F;

    ax *= d;
    ay *= d;
    az *= d;

    matrixIdentity(mat);
    mat[0][0] = ax*ax * (1.0F-c) + c;
    mat[1][2] = ay*az * (1.0F-c) + ax * s;
    mat[2][1] = ay*az * (1.0F-c) - ax * s;

    mat[1][1] = ay*ay * (1.0F-c) + c;
    mat[0][2] = ax*az * (1.0F-c) - ay * s;
    mat[2][0] = ax*az * (1.0F-c) + ay * s;

    mat[2][2] = az*az * (1.0F-c) + c;
    mat[0][1] = ax*ay * (1.0F-c) + az * s;
    mat[1][0] = ax*ay * (1.0F-c) - az * s;
}

/************************************************************/
/* Trackball */
/************************************************************/

typedef struct Trackball {
    GLint x, y, width, height;

    GLboolean spining;

    GLfloat lastPos[3];
    int lastTime;

    GLfloat angle;
    GLfloat axis[3];

    void (*updateViewport)(struct Trackball *track,
		int x, int y, int width, int height);
    void (*startMotion)(struct Trackball *track, int x, int y, int time);
    void (*stopMotion)(struct Trackball *track, int x, int y, int time);
    void (*trackMotion)(struct Trackball *track, int x, int y, int time);
    void (*updateTransform)(struct Trackball *, Matrix mat);
} Trackball;

void
trackballPointToVector(int x, int y, int width, int height, GLfloat pos[3])
{
    GLfloat px, py, pz, len, d;

    /* project x,y onto a hemi-sphere centered within width,height */
    px = (2.0F*x - width) / width;
    py = (height - 2.0F*y) / height;
    d = (GLfloat) sqrt(px*px + py*py);
    pz = (GLfloat) cos((M_PI/2.0F) * ((d < 1.0F) ? d : 1.0F));

    len = sqrt(px*px + py*py + pz*pz);
    d = (len != 0.0F) ? 1.0F / len : 1.0F;
    pos[0] = px * len;
    pos[1] = py * len;
    pos[2] = pz * len;
}

void
trackballUpdateViewport(Trackball *track, int x, int y, int width, int height)
{
    track->x = x;
    track->y = y;
    track->width = width;
    track->height = height;
}

void
trackballStartMotion(Trackball *track, int x, int y, int time)
{
    track->spining = GL_FALSE;
    track->lastTime = time;
    trackballPointToVector(x, y, track->width, track->height, track->lastPos);
}

/*ARGSUSED*/
void
trackballStopMotion(Trackball *track, int x, int y, int time)
{
    if (track->lastTime == time) {
	track->spining = GL_TRUE;
    } else {
	track->spining = GL_FALSE;
	track->angle = 0.0F;
    }
}

void
trackballTrackMotion(Trackball *track, int x, int y, int time)
{
    GLfloat curPos[3], *lastPos = track->lastPos;
    GLfloat ax, ay, az;

    trackballPointToVector(x, y, track->width, track->height, curPos);

    ax = curPos[0] - lastPos[0];
    ay = curPos[1] - lastPos[1];
    az = curPos[2] - lastPos[2];
    track->angle = 90.0F * (GLfloat) sqrt(ax*ax + ay*ay + az*az);

    track->axis[0] = lastPos[1]*curPos[2] - lastPos[2]*curPos[1];
    track->axis[1] = lastPos[2]*curPos[0] - lastPos[0]*curPos[2];
    track->axis[2] = lastPos[0]*curPos[1] - lastPos[1]*curPos[0];

    track->lastTime = time;
    track->lastPos[0] = curPos[0];
    track->lastPos[1] = curPos[1];
    track->lastPos[2] = curPos[2];
}

void
trackballUpdateTransform(Trackball *track, Matrix mat)
{
    Matrix rot;

    matrixRotate(rot,
		track->angle, track->axis[0], track->axis[1], track->axis[2]);
    matrixMultiply(mat, rot, mat);
}

void
trackballInit(Trackball *track)
{
    track->updateViewport = trackballUpdateViewport;
    track->startMotion = trackballStartMotion;
    track->stopMotion = trackballStopMotion;
    track->trackMotion = trackballTrackMotion;
    track->updateTransform = trackballUpdateTransform;

    track->axis[0] = 1.0F;
    track->axis[1] = 0.0F;
    track->axis[2] = 0.0F;
}

/************************************************************/
/* Texture Image */
/************************************************************/

typedef struct Texture {
    GLuint name;
    GLint width, height;
} Texture;

void
rippleTextureInit(Texture *tex)
{
    GLint w = 512;
    GLint h = 512;
    GLubyte *img, *p;
    GLint i, j;

    img = (GLubyte *) malloc(w*h*sizeof(*img));
    if (img == NULL) {
	return;
    }

    p = img;
    for (j=0; j<h; ++j) {
	for (i=0; i<w; ++i) {

	    /* texture is the product of two cosine waves */
	    float g = (float) cos(((float)i/(w-1))*16.0F*M_PI);
	    float f = (float) cos(((float)j/(h-1))*16.0F*M_PI);
	    float b = f * g;

	    /* scale and clamp to add a some high frequency components */
	    b *= 2.5F;
	    if (b > 1.0F) b = 1.0F;
	    if (b < -1.0F) b = -1.0F;

	    /* scale/bias the range to [0,1] */
	    b = b * 0.5F + 0.5;

	    *p++ = (GLubyte) (b * 255.0F); 
	}
    }

    glGenTextures(1, &tex->name);
    tex->width = w;
    tex->height = h;

    glBindTexture(GL_TEXTURE_2D, tex->name);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8,
			w, h , 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, img);
    free(img);
}

void
imageTextureInit(Texture *tex, char* filename, GLenum internalFormat)
{
    int w, h, components;
    GLubyte *img;

    img = (GLubyte *) read_texture(filename, &w, &h, &components);
    if (img == NULL) {
	return;
    }

    glGenTextures(1, &tex->name);
    tex->width = w;
    tex->height = h;

    glBindTexture(GL_TEXTURE_2D, tex->name);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
			w, h , 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
    free(img);
}

/************************************************************/
/* Lights */
/************************************************************/

typedef struct Light {
    GLint index;

    GLfloat pos[4];
    Matrix mat;

    GLfloat eyePos[4];

    void (*update)(struct Light *light, Matrix viewMat);
    void (*draw)(struct Light *light);
} Light;

/*ARGSUSED1*/
void
pointLightUpdate(Light *light, Matrix viewMat)
{
    glEnable(GL_LIGHT0+light->index);
    glLightfv(GL_LIGHT0+light->index, GL_POSITION, light->pos);
    glGetLightfv(GL_LIGHT0+light->index, GL_POSITION, light->eyePos);
}

void
pointLightDraw(Light *light)
{
    glColor3f(1.0F, 1.0F, 1.0F);
    glBegin(GL_LINES);
    glVertex3f(0.0F, 0.0F, 0.0F);
    glVertex3fv(light->pos);
    glEnd();
}

void
pointLightInit(Light *light)
{
    light->index = 0;

    light->pos[0] = 0.5F;
    light->pos[1] = 0.5F;
    light->pos[2] = 0.5F;
    light->pos[3] = 1.0F;

    matrixIdentity(light->mat);
    light->update = pointLightUpdate;
    light->draw = pointLightDraw;
}

/************************************************************/
/* Materials */
/************************************************************/

static GLfloat zeros[4] = { 0.0, 0.0, 0.0, 0.0 };

typedef struct Material {
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat shininess;

    void (*setMaterial)(struct Material *mat);
    void (*setDiffuseMaterial)(struct Material *mat);
    void (*setSpecularMaterial)(struct Material *mat);
} Material;

void
setMaterial(Material *mat)
{
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat->ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat->diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat->specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mat->shininess);
}

void setDiffuseMaterial(Material *mat)
{
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat->ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat->diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zeros);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0);
}

void setSpecularMaterial(Material *mat)
{
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, zeros);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, zeros);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat->specular);
	
	/* The power function will be taken later */
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 1.0);
}


void
grayMaterialInit(Material *mat)
{
    mat->setMaterial = setMaterial;
    mat->setDiffuseMaterial = setDiffuseMaterial;
    mat->setSpecularMaterial = setSpecularMaterial;

    mat->ambient[0] = 0.1F;
    mat->ambient[1] = 0.1F;
    mat->ambient[2] = 0.1F;
    mat->ambient[3] = 1.0F;

    mat->diffuse[0] = 0.6F;
    mat->diffuse[1] = 0.6F;
    mat->diffuse[2] = 0.6F;
    mat->diffuse[3] = 1.0F;

    mat->specular[0] = 0.0F;
    mat->specular[1] = 0.0F;
    mat->specular[2] = 0.0F;
    mat->specular[3] = 0.0F;

    mat->shininess = 1.0F;
}

void
goldMaterialInit(Material *mat)
{
    mat->setMaterial = setMaterial;
    mat->setDiffuseMaterial = setDiffuseMaterial;
    mat->setSpecularMaterial = setSpecularMaterial;

    mat->ambient[0] = 0.50F;
    mat->ambient[1] = 0.40F;
    mat->ambient[2] = 0.25F;
    mat->ambient[3] = 1.0F;

    mat->diffuse[0] = 1.0F;
    mat->diffuse[1] = 1.0F;
    mat->diffuse[2] = 0.5F;
    mat->diffuse[3] = 1.0F;

    mat->specular[0] = 1.0;
    mat->specular[1] = 1.0;
    mat->specular[2] = 1.0;
    mat->specular[3] = 1.0F;

    mat->shininess = 25.0F;
}

/************************************************************/
/* Objects */
/************************************************************/

#define NUM_TEXCOORD_SETS 3

typedef struct Vertex {
    GLfloat normal[4];
    GLfloat vertex[4];

    GLfloat texcoord[NUM_TEXCOORD_SETS][2];
    GLfloat tangent[4];
    GLfloat binormal[4];
} Vertex;

typedef struct Object {
    int numVerts;
    int texCoordSetIndex;
    Vertex *objVerts;

    Matrix mat;

    void (*draw)(struct Object *obj);

    GLint numMajor, numMinor;
} Object;

void
drawSurfaceVectors(Object *obj, GLfloat scale)
{
    int i;

    glBegin(GL_LINES);
    for (i=0; i<obj->numVerts; ++i) {
	Vertex *v = &obj->objVerts[i];

	glColor3f(1.0F, 0.0F, 0.0F);
	glVertex3fv(v->vertex);
	glVertex3f(v->vertex[0] + scale*v->normal[0],
	           v->vertex[1] + scale*v->normal[1],
	           v->vertex[2] + scale*v->normal[2]);
	glColor3f(0.0F, 1.0F, 0.0F);
	glVertex3fv(v->vertex);
	glVertex3f(v->vertex[0] + scale*v->tangent[0],
	           v->vertex[1] + scale*v->tangent[1],
	           v->vertex[2] + scale*v->tangent[2]);
	glColor3f(0.0F, 0.0F, 1.0F);
	glVertex3fv(v->vertex);
	glVertex3f(v->vertex[0] + scale*v->binormal[0],
	           v->vertex[1] + scale*v->binormal[1],
	           v->vertex[2] + scale*v->binormal[2]);
    }
    glEnd();
}

void
squareDraw(Object *square)
{
    GLint numMajor = square->numMajor;
    GLint numMinor = square->numMinor;
    Vertex *verts = square->objVerts;
    int texCoordSetIndex = square->texCoordSetIndex;
    int i, j;

    for (i=0; i<numMajor; ++i) {
	glBegin(GL_TRIANGLE_STRIP);
	for (j=0; j<=numMinor; ++j) {
	    Vertex *v0 = verts + i*(numMinor+1) + j;
	    Vertex *v1 = v0 + (numMinor+1);

	    glTexCoord2fv(v1->texcoord[texCoordSetIndex]);
	    glNormal3fv(v1->normal);
	    glVertex3fv(v1->vertex);

	    glTexCoord2fv(v0->texcoord[texCoordSetIndex]);
	    glNormal3fv(v0->normal);
	    glVertex3fv(v0->vertex);
	}
	glEnd();
    }
}

void
squareInit(Object *square)
{
    GLint numMajor = 16;
    GLint numMinor = 16;
    GLint numVerts = (numMajor+1) * (numMinor+1);
    GLfloat texScaleS = 1.0F;
    GLfloat texScaleT = 1.0F;
    Vertex *pv;
    int i, j;

    square->numMajor = numMajor;
    square->numMinor = numMinor;

    square->numVerts = numVerts;
    square->texCoordSetIndex = 0;
    square->objVerts = (Vertex *) calloc(numVerts, sizeof(Vertex));

    matrixIdentity(square->mat);
    square->draw = squareDraw;

    pv = square->objVerts;
    for (i=0; i<=numMajor; ++i) {
	for (j=0; j<=numMinor; ++j) {
	    GLfloat u = (GLfloat) j / numMinor;
	    GLfloat v = (GLfloat) i / numMajor;

	    pv->texcoord[0][0] = u * texScaleS;
	    pv->texcoord[0][1] = v * texScaleT;

	    pv->normal[0] = 0.0F;
	    pv->normal[1] = 0.0F;
	    pv->normal[2] = 1.0F;
	    pv->normal[3] = 0.0F;

	    pv->vertex[0] = u * 1.0F - 0.5F;
	    pv->vertex[1] = v * 1.0F - 0.5F;
	    pv->vertex[2] = 0.0F;
	    pv->vertex[3] = 1.0F;

	    pv->tangent[0] = 1.0F;
	    pv->tangent[1] = 0.0F;
	    pv->tangent[2] = 0.0F;
	    pv->tangent[3] = 0.0F;

	    vecCross(pv->binormal, pv->normal, pv->tangent);

	    ++pv;
	}
    }
}

void
cylinderDraw(Object *cylinder)
{
    GLint numMajor = cylinder->numMajor;
    GLint numMinor = cylinder->numMinor;
    Vertex *verts = cylinder->objVerts;
    int texCoordSetIndex = cylinder->texCoordSetIndex;
    int i, j;

    for (i=0; i<numMajor; ++i) {
	glBegin(GL_TRIANGLE_STRIP);
	for (j=0; j<=numMinor; ++j) {
	    Vertex *v0 = verts + i*(numMinor+1) + j;
	    Vertex *v1 = v0 + (numMinor+1);

	    glTexCoord2fv(v1->texcoord[texCoordSetIndex]);
	    glNormal3fv(v1->normal);
	    glVertex3fv(v1->vertex);

	    glTexCoord2fv(v0->texcoord[texCoordSetIndex]);
	    glNormal3fv(v0->normal);
	    glVertex3fv(v0->vertex);
	}
	glEnd();
    }
}

void
cylinderInit(Object *cylinder)
{
    GLint numMajor = 256;
    GLint numMinor = 256;
    GLint numVerts = (numMajor+1) * (numMinor+1);
    GLfloat radius = 0.5F;
    GLfloat radialStep = 2.0F * M_PI / numMinor;
    GLfloat texScaleS = 3.0F;
    GLfloat texScaleT = 1.5F;
    Vertex *pv;
    int i, j;

    cylinder->numMajor = numMajor;
    cylinder->numMinor = numMinor;

    cylinder->numVerts = numVerts;
    cylinder->texCoordSetIndex = 0;
    cylinder->objVerts = (Vertex *) calloc(numVerts, sizeof(Vertex));

    matrixIdentity(cylinder->mat);
    cylinder->draw = cylinderDraw;

    pv = cylinder->objVerts;
    for (i=0; i<=numMajor; ++i) {
	GLfloat v = (GLfloat) i / numMajor;

	for (j=0; j<=numMinor; ++j) {
	    GLfloat a0 = j * radialStep;
	    GLfloat u = (GLfloat) j / numMinor;
	    GLfloat x0 = (GLfloat) -cos(a0);
	    GLfloat z0 = (GLfloat) sin(a0);

	    pv->texcoord[0][0] = u * texScaleS;
	    pv->texcoord[0][1] = v * texScaleT;

	    pv->normal[0] = x0;
	    pv->normal[1] = 0.0F;
	    pv->normal[2] = z0;
	    pv->normal[3] = 0.0F;
	    vecNormalize(pv->normal);

	    pv->vertex[0] = x0 * radius;
	    pv->vertex[1] = v * 1.0F - 0.5F;
	    pv->vertex[2] = z0 * radius;
	    pv->vertex[3] = 1.0F;

	    pv->tangent[0] = z0;
	    pv->tangent[1] = 0.0F;
	    pv->tangent[2] = -x0;
	    pv->tangent[3] = 0.0F;
	    vecNormalize(pv->tangent);

	    vecCross(pv->binormal, pv->normal, pv->tangent);
	    vecNormalize(pv->binormal);

	    ++pv;
	}
    }
}

void
torusDraw(Object *torus)
{
    GLint numMajor = torus->numMajor;
    GLint numMinor = torus->numMinor;
    Vertex *verts = torus->objVerts;
    int texCoordSetIndex = torus->texCoordSetIndex;
    int i, j;

    for (i=0; i<numMajor; ++i) {
	glBegin(GL_TRIANGLE_STRIP);
	for (j=0; j<=numMinor; ++j) {
	    Vertex *v0 = verts + i*(numMinor+1) + j;
	    Vertex *v1 = v0 + (numMinor+1);

	    glTexCoord2fv(v0->texcoord[texCoordSetIndex]);
	    glNormal3fv(v0->normal);
	    glVertex3fv(v0->vertex);

	    glTexCoord2fv(v1->texcoord[texCoordSetIndex]);
	    glNormal3fv(v1->normal);
	    glVertex3fv(v1->vertex);
	}
	glEnd();
    }
}

void
torusInit(Object *torus)
{
    GLint numMajor = 256;
    GLint numMinor = 256;
    GLint numVerts = (numMajor+1) * (numMinor+1);
    GLfloat majorRadius = 0.5F;
    GLfloat minorRadius = 0.15F;
    GLfloat majorStep = 2.0F * M_PI / numMajor;
    GLfloat minorStep = 2.0F * M_PI / numMinor;
    GLfloat texScaleS = 4.0F;
    GLfloat texScaleT = 1.5F;
    Vertex *pv;
    int i, j;

    torus->numMajor = numMajor;
    torus->numMinor = numMinor;

    torus->numVerts = numVerts;
    torus->texCoordSetIndex = 0;
    torus->objVerts = (Vertex *) calloc(numVerts, sizeof(Vertex));

    matrixIdentity(torus->mat);
    torus->draw = torusDraw;

    pv = torus->objVerts;
    for (i=0; i<=numMajor; ++i) {
	GLfloat a0 = i * majorStep;
	GLfloat x0 = (GLfloat) cos(a0);
	GLfloat y0 = (GLfloat) sin(a0);

	for (j=0; j<=numMinor; ++j) {
	    GLfloat b = j * minorStep;
	    GLfloat c = (GLfloat) cos(b);
	    GLfloat r = minorRadius * c + majorRadius;
	    GLfloat z = minorRadius * (GLfloat) sin(b);
	    GLfloat u = (GLfloat) i / numMajor;
	    GLfloat v = (GLfloat) j / numMinor;

	    pv->texcoord[0][0] = (u+1.0F) * texScaleS;
	    pv->texcoord[0][1] = (v+1.0F) * texScaleT;

	    pv->normal[0] = x0 * c;
	    pv->normal[1] = y0 * c;
	    pv->normal[2] = z / minorRadius;
	    pv->normal[3] = 0.0F;
	    vecNormalize(pv->normal);

	    pv->vertex[0] = x0 * r;
	    pv->vertex[1] = y0 * r;
	    pv->vertex[2] = z;
	    pv->vertex[3] = 1.0F;

	    pv->tangent[0] = -y0;
	    pv->tangent[1] = x0;
	    pv->tangent[2] = 0.0F;
	    pv->tangent[3] = 0.0F;
	    vecNormalize(pv->tangent);

	    vecCross(pv->binormal, pv->normal, pv->tangent);
	    vecNormalize(pv->binormal);

	    ++pv;
	}
    }
}

/************************************************************/
/* Viewer */
/************************************************************/

typedef struct Viewer {
    GLint x, y;
    GLint width, height;

    Matrix mat;

    void (*updateViewport)(struct Viewer *view,
		int x, int y, int width, int height);
} Viewer;

void
viewerUpdateViewport(Viewer *view, int x, int y, int width, int height)
{
    float sx, sy;

    view->x = x;
    view->y = y;
    view->width = width;
    view->height = height;
    glViewport(0, 0, width, height);

    /* adjust aspect ratio to avoid distortion */
    if (width >= height) {
	sx = (width != 0) ? ((float)width / height) : 1.0F;
	sy = 1.0F;
    } else {
	sx = 1.0F;
	sy = (height != 0) ? ((float)height / width) : 1.0F;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.5*sx, 0.5*sx, -0.5*sy, 0.5*sy, 1.0, 3.0);
    glMatrixMode(GL_MODELVIEW);
}

void
viewerInit(Viewer *view)
{
    view->updateViewport = viewerUpdateViewport;

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0F, 0.0F, -2.0F);
    glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) view->mat);
}

/************************************************************/
/* Application */
/************************************************************/

char *imageFilename = "../../data/ogllogo.bw";

Texture ripple;
Texture image;
Texture *textureList[] = { &image, &ripple };
#define NUM_TEXTURES (sizeof(textureList) / sizeof(textureList[0]))
int textureIndex;

Light pointLight;
Light *lightList[] = { &pointLight };
#define NUM_LIGHTS (sizeof(lightList) / sizeof(lightList[0]))
int lightIndex;

Material grayMaterial;
Material goldMaterial;
Material *materialList[] = { &goldMaterial, &grayMaterial };
#define NUM_MATERIALS (sizeof(materialList) / sizeof(materialList[0]))
int materialIndex;

Object square;
Object cylinder;
Object torus;
Object *objectList[] = { &square, &cylinder, &torus };
#define NUM_OBJECTS (sizeof(objectList) / sizeof(objectList[0]))
int objectIndex;

#define BUMP_MODE_PASS1 0x01
#define BUMP_MODE_PASS2 0x02
#define BUMP_MODE_PASS3 0x04
#define BUMP_MODE_PASS4 0x08
#define BUMP_MODE_PASS5 0x10
#define BUMP_MODE_PASS6 0x20
int bumpModeList[] = {
    BUMP_MODE_PASS1 | BUMP_MODE_PASS2 | BUMP_MODE_PASS3 | BUMP_MODE_PASS4 | BUMP_MODE_PASS5 | BUMP_MODE_PASS6,
    BUMP_MODE_PASS1,
    BUMP_MODE_PASS2 | BUMP_MODE_PASS3,
    BUMP_MODE_PASS1 | BUMP_MODE_PASS2 | BUMP_MODE_PASS3,
    BUMP_MODE_PASS4,
    BUMP_MODE_PASS5 | BUMP_MODE_PASS6,
    BUMP_MODE_PASS4 | BUMP_MODE_PASS5 | BUMP_MODE_PASS6,
};
#define NUM_BUMP_MODES (sizeof(bumpModeList) / sizeof(bumpModeList[0]))
int bumpModeIndex;

typedef enum MoveMode {
    MoveNone, MoveObject, MoveLight
} MoveMode;
MoveMode moveMode = MoveNone;

Trackball *trackball;
Viewer *viewer;

GLboolean bumpMapping = GL_TRUE;
GLboolean cullfaceEnabled = GL_FALSE;
GLboolean lightingEnabled = GL_TRUE;
GLboolean textureEnabled = GL_FALSE;
GLboolean displayLightPosition = GL_TRUE;
GLboolean displayBackground = GL_TRUE;
GLboolean displayVectors = GL_FALSE;
GLboolean displayWireframe = GL_FALSE;
GLboolean supportsEXT_blend_subtract = GL_FALSE;
GLboolean useBlending = GL_TRUE;

GLboolean useFalloff = GL_TRUE;
GLboolean useSmoothFalloff = GL_TRUE;

GLfloat displayVectorScale = 0.08F;
GLfloat bumpScale = 0.65F;
GLfloat bumpFalloffBias = 0.3F;

GLuint	specularTex;

#define POWER_TABLE_SIZE 4096
GLfloat power[POWER_TABLE_SIZE], linear[POWER_TABLE_SIZE];

void
perturbTexCoords(Object *obj, Texture *tex, Light *light, Matrix viewMat)
{
    Matrix modelView, modelViewInv;
    GLfloat LobjPos[4], VobjPos[4], origin[4] = { 0.0, 0.0, 0.0, 1.0 };
    int i;

    /*
    ** find light and viewer positions in object space
    */
    matrixMultiply(modelView, viewMat, obj->mat);
    matrixInvert(modelViewInv, modelView);
    matrixTransform44(LobjPos, modelViewInv, light->eyePos);
    matrixTransform44(VobjPos, modelViewInv, origin);

    for (i=0; i<obj->numVerts; ++i)
	{
		Vertex *ov = &obj->objVerts[i];
		GLfloat LobjDir[4], VobjDir[4];
		GLfloat LtanDir[4], VtanDir[4], HtanDir[4];
		Matrix	tanMat;
		GLfloat falloff;

		/*
		** find light direction in object space
		*/
		LobjDir[0] = LobjPos[0] - ov->vertex[0];
		LobjDir[1] = LobjPos[1] - ov->vertex[1];
		LobjDir[2] = LobjPos[2] - ov->vertex[2];
		LobjDir[3] = 1.0F;
		vecNormalize(LobjDir);

		/*
		** find view direction in object space
		*/
		VobjDir[0] = VobjPos[0] - ov->vertex[0];
		VobjDir[1] = VobjPos[1] - ov->vertex[1];
		VobjDir[2] = VobjPos[2] - ov->vertex[2];
		VobjDir[3] = 1.0F;
		vecNormalize(VobjDir);

		/*
		** find transformation from object space to tangent space
		*/
		tanMat[0][0] = ov->tangent[0];
		tanMat[0][1] = ov->tangent[1];
		tanMat[0][2] = ov->tangent[2];
		tanMat[0][3] = 0.0;

		tanMat[1][0] = ov->binormal[0];
		tanMat[1][1] = ov->binormal[1];	
		tanMat[1][2] = ov->binormal[2];
		tanMat[1][3] = 0.0;

		tanMat[2][0] = ov->normal[0];
		tanMat[2][1] = ov->normal[1];
		tanMat[2][2] = ov->normal[2];
		tanMat[2][3] = 0.0;

		tanMat[3][0] = 0.0F;
		tanMat[3][1] = 0.0F;
		tanMat[3][2] = 0.0F;
		tanMat[3][3] = 1.0F;

		/*
		** transform light and view direction vectors to tangent space
		** (use transpose to transform a direction instead of a position)
		*/
		matrixTranspose(tanMat, tanMat);
		matrixTransform44(LtanDir, tanMat, LobjDir);
		matrixTransform44(VtanDir, tanMat, VobjDir);

		/*
		** compute the halfway vector (L + V) / |(L + V)|
		*/
		HtanDir[0] = LtanDir[0] + VtanDir[0];
		HtanDir[1] = LtanDir[1] + VtanDir[1];
		HtanDir[2] = LtanDir[2] + VtanDir[2];
		HtanDir[3] = 1;
		vecNormalize(HtanDir);

		/*
		** scale the bump slope as the normal falls away from the light
		**
		** falloff prevents the bumps from showing up when the surface is
		** facing away from the light source
		**
		** the bias allows the bumps to contribute a little before the
		** base diffuse component does
		*/
		if (useFalloff)
		{
			falloff = vecDot(ov->normal, LobjDir);
			if (useSmoothFalloff)
			{
				falloff += bumpFalloffBias;
			}
			if (falloff > 1.0F) falloff = 1.0F;
			if (falloff < 0.0F) falloff = 0.0F;
		} else {
			falloff = 1.0F;
		}

		/*
		** compute shifted texture coordinates for d dot L 
		*/
		ov->texcoord[1][0] = ov->texcoord[0][0] + falloff * (LtanDir[0] / tex->width);
		ov->texcoord[1][1] = ov->texcoord[0][1] + falloff * (LtanDir[1] / tex->height);

		/*
		** compute shifted texture coordinates for d dot H
		*/
		ov->texcoord[2][0] = ov->texcoord[0][0] + falloff * (HtanDir[0] / tex->width);
		ov->texcoord[2][1] = ov->texcoord[0][1] + falloff * (HtanDir[1] / tex->height);
    }
}

void clearWindow(GLboolean forceBlack)
{
	/* Clear to black */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (displayBackground && (forceBlack == GL_FALSE))
	{
		/* fill the background with a color gradient */
		glDisable(GL_DEPTH_TEST);
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		glBegin(GL_QUADS);
			glColor3f(0.2F, 0.0F, 1.0F); glVertex2f(-1.0F, -1.0F);
			glColor3f(0.2F, 0.0F, 1.0F); glVertex2f( 1.0F, -1.0F);
			glColor3f(0.0F, 0.0F, 0.1F); glVertex2f( 1.0F,  1.0F);
			glColor3f(0.0F, 0.0F, 0.1F); glVertex2f(-1.0F,  1.0F);
		glEnd();

		glEnable(GL_DEPTH_TEST);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
    }
}


void
drawObject(Object *obj, Texture *bumpmap, Light *light, Material *material)
{
	/* so we can scale the accum buffer by the right amount if we're showing steps */
	float  inAccum = 0.0;

    glColor3f(1.0F, 1.0F, 1.0F);

    if (bumpMapping) {
        int bumpMode = bumpModeList[bumpModeIndex];

	/* compute shifted texture coordinates */
	perturbTexCoords(obj, bumpmap, light, viewer->mat);

	/* use bump displacement texture */
	glBindTexture(GL_TEXTURE_2D, bumpmap->name);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	if (bumpMode & BUMP_MODE_PASS1) {
	    /*
	    ** First Pass of Specular (First overall) -- draw untextured with specular lighting
	    */
	    material->setSpecularMaterial(material);
	    glEnable(GL_LIGHTING);
	    obj->draw(obj);
	    glDisable(GL_LIGHTING);
	}

	/* use blending or accum buffer to accumulate additional passes */
	if (useBlending) {
	    glEnable(GL_BLEND);
	} else {
	    glAccum(GL_LOAD, 0.5F);
		inAccum = 0.5;
	}

	glDepthMask(GL_FALSE);

	if (bumpMode & BUMP_MODE_PASS2) {
	    /*
	    ** Second pass of Specular (Second overall) -- add textured without lighting (un-shifted)
	    **							                 use modulated color to scale bump contribution
	    */
	    glColor3f(material->specular[0]*bumpScale,
		      material->specular[1]*bumpScale,
		      material->specular[2]*bumpScale);

	    if (useBlending) {
		glBlendFunc(GL_ONE, GL_ONE);
		glBlendEquationEXT(GL_FUNC_ADD_EXT);
	    }

	    glEnable(GL_TEXTURE_2D);
	    obj->texCoordSetIndex = 0;
	    obj->draw(obj);
	    glDisable(GL_TEXTURE_2D);

	    if (!useBlending) {
		glAccum(GL_ACCUM, 0.5F);
		inAccum += 0.5;
	    }
	}

	if (bumpMode & BUMP_MODE_PASS3) {
	    /*
	    ** Third Pass of Specular (Third overall) -- subtract textured without lighting (shifted)
	    **											use modulated color to scale bump contribution
	    */
	    glColor3f(material->specular[0]*bumpScale,
		      material->specular[1]*bumpScale,
		      material->specular[2]*bumpScale);

	    if (useBlending) {
		glBlendFunc(GL_ONE, GL_ONE);
		glBlendEquationEXT(GL_FUNC_REVERSE_SUBTRACT_EXT);
	    }

	    glEnable(GL_TEXTURE_2D);
	    obj->texCoordSetIndex = 2;
	    obj->draw(obj);
	    glDisable(GL_TEXTURE_2D);

	    if (!useBlending) {
		glAccum(GL_ACCUM, -0.5F);
		inAccum -= 0.5;
	    }
	}
	glDepthMask(GL_TRUE);

	if (useBlending) {
	    glDisable(GL_BLEND);
	} else {
		if (inAccum == 0)
			 glAccum(GL_RETURN, 5.0);
		else glAccum(GL_RETURN, 1.0 / inAccum);
	}
	
	/* Copy through the pixel transfer pipeline to do the power function */
	glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
	glCopyPixels(0, 0, 512, 512, GL_COLOR);
	glPixelTransferi(GL_MAP_COLOR, GL_FALSE);

	if (!useBlending) {
	glAccum(GL_LOAD, 0.4f);
	inAccum = 0.5;
	}

	/*   Diffuse   */
    material->setDiffuseMaterial(material);
	if (bumpMode & (BUMP_MODE_PASS4 | BUMP_MODE_PASS5 | BUMP_MODE_PASS6))
		clearWindow(GL_FALSE);
	if (useBlending)
		glEnable(GL_BLEND);

	if (bumpMode & BUMP_MODE_PASS4) {
	    /*
	    ** First Pass of Diffuse (Fourth overall) -- draw untextured with diffuse lighting
	    */

	    if (useBlending) {
		glBlendFunc(GL_ONE, GL_ONE);
		glBlendEquationEXT(GL_FUNC_ADD_EXT);
	    }

	    glEnable(GL_LIGHTING);
	    obj->draw(obj);
	    glDisable(GL_LIGHTING);

	    if (!useBlending) {
		glAccum(GL_ACCUM, 0.5F);
		inAccum += 0.5;
	    }
	}

	glDepthMask(GL_FALSE);

	/*	These next two steps are in the opposite order from above.
		This is to keep the values in the accumulation buffer between 0.0 and 1.0 */

	if (bumpMode & BUMP_MODE_PASS5) {
	    /*
	    ** Second Pass of Diffuse (Fifth overall) -- subtract textured without lighting (shifted)
	    **										use modulated color to scale bump contribution
	    */
	    glColor3f(material->diffuse[0]*bumpScale,
		      material->diffuse[1]*bumpScale,
		      material->diffuse[2]*bumpScale);

	    if (useBlending) {
		glBlendFunc(GL_ONE, GL_ONE);
		glBlendEquationEXT(GL_FUNC_REVERSE_SUBTRACT_EXT);
	    }

	    glEnable(GL_TEXTURE_2D);
	    obj->texCoordSetIndex = 1;
	    obj->draw(obj);
	    glDisable(GL_TEXTURE_2D);

	    if (!useBlending) {
		glAccum(GL_ACCUM, -0.5F);
		inAccum -= 0.5;
	    }
	}

	if (bumpMode & BUMP_MODE_PASS6) {
	    /*
	    ** Third pass of Diffuse (Sixth overall) -- add textured without lighting (un-shifted)
	    **							             use modulated color to scale bump contribution
	    */
	    glColor3f(material->diffuse[0]*bumpScale,
		      material->diffuse[1]*bumpScale,
		      material->diffuse[2]*bumpScale);

	    if (useBlending) {
		glBlendFunc(GL_ONE, GL_ONE);
		glBlendEquationEXT(GL_FUNC_ADD_EXT);
	    }

	    glEnable(GL_TEXTURE_2D);
	    obj->texCoordSetIndex = 0;
	    obj->draw(obj);
	    glDisable(GL_TEXTURE_2D);

	    if (!useBlending) {
		glAccum(GL_ACCUM, 0.5F);
		inAccum += 0.5;
	    }
	}

	glDepthMask(GL_TRUE);

	if (useBlending) {
	    glDisable(GL_BLEND);
	} else {
		if (inAccum == 0)
			 glAccum(GL_RETURN, 5.0);
		else glAccum(GL_RETURN, 1.0 / inAccum);
	}

    } else {
	/* use bump displacement texture as image */
	glBindTexture(GL_TEXTURE_2D, bumpmap->name);

	if (lightingEnabled) glEnable(GL_LIGHTING);
	if (textureEnabled) glEnable(GL_TEXTURE_2D);

	obj->texCoordSetIndex = 0;
	obj->draw(obj);

	if (lightingEnabled) glDisable(GL_LIGHTING);
	if (textureEnabled) glDisable(GL_TEXTURE_2D);
    }
}

void
init(void)
{
	int i;

    supportsEXT_blend_subtract =
	extensionSupported("GL_EXT_blend_subtract",
			   (const char *) glGetString(GL_EXTENSIONS));
#if defined(_WIN32) && defined(GL_EXT_blend_subtract)
    if (supportsEXT_blend_subtract) {
	glBlendEquationEXT = (PFNGLBLENDEQUATIONEXTPROC)
		    wglGetProcAddress("glBlendEquationEXT");
    }
#endif
    useBlending = supportsEXT_blend_subtract;

    viewer = (Viewer *) calloc(1, sizeof(Viewer));
    viewerInit(viewer);

    trackball = (Trackball *) calloc(1, sizeof(Trackball));
    trackballInit(trackball);

    rippleTextureInit(&ripple);
    imageTextureInit(&image, imageFilename, GL_LUMINANCE8);

    grayMaterialInit(&grayMaterial);
    goldMaterialInit(&goldMaterial);

    pointLightInit(&pointLight);

    squareInit(&square);
    cylinderInit(&cylinder);
    torusInit(&torus);

	glGenTextures(1, &specularTex);

	/* Setup power function for specularity */
	for (i=0; i<POWER_TABLE_SIZE; i++)
	{
		power[i]  = exp(materialList[materialIndex]->shininess * log(i/(POWER_TABLE_SIZE - 1.0)));
		linear[i] = i/(POWER_TABLE_SIZE - 1.0);
	}
	glPixelMapfv(GL_PIXEL_MAP_R_TO_R, POWER_TABLE_SIZE, power);
	glPixelMapfv(GL_PIXEL_MAP_G_TO_G, POWER_TABLE_SIZE, power);
	glPixelMapfv(GL_PIXEL_MAP_B_TO_B, POWER_TABLE_SIZE, power);
	glPixelMapfv(GL_PIXEL_MAP_A_TO_A, POWER_TABLE_SIZE, linear);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
}

void
redraw(void)
{
    Object *obj = objectList[objectIndex];
    Texture *bumpmap = textureList[textureIndex];
    Light *light = lightList[lightIndex];
    Material *material = materialList[materialIndex];

    if (moveMode == MoveObject) {
	trackball->updateTransform(trackball, obj->mat);
    } else if (moveMode == MoveLight) {
	trackball->updateTransform(trackball, light->mat);
    }

	clearWindow(GL_TRUE);

    /*
    ** Position light source
    */
    glPushMatrix();
    glMultMatrixf((GLfloat *) light->mat);
    light->update(light, viewer->mat);
    glPopMatrix();

    /*
    ** Position and draw object
    */
    glPushMatrix();
    glMultMatrixf((GLfloat *) obj->mat);

    if (displayVectors) {
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0F, 1.0F);
	glLineWidth(2.0F);
    }

    if (cullfaceEnabled) {
	glEnable(GL_CULL_FACE);
    }

    if (displayWireframe) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    drawObject(obj, bumpmap, light, material);

    if (displayWireframe) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (cullfaceEnabled) {
	glDisable(GL_CULL_FACE);
    }

    if (displayVectors) {
	drawSurfaceVectors(obj, displayVectorScale);

	glDisable(GL_POLYGON_OFFSET_FILL);
	glLineWidth(1.0F);
    }

    glPopMatrix();

    /*
    ** Draw light source
    */
    glPushMatrix();
    glMultMatrixf((GLfloat *) light->mat);
    if (displayLightPosition) light->draw(light);
    glPopMatrix();
}

/************************************************************/
/* User interface */
/************************************************************/

void
display(void)
{
    redraw();
    glutSwapBuffers();

    if (trackball->spining) {
	glutIdleFunc(display);
    } else {
	glutIdleFunc(NULL);
    }
}

void
visibility(int state)
{
    if (state == GLUT_NOT_VISIBLE && trackball->spining) {
	glutIdleFunc(NULL);
    }
}

void
reshape(int width, int height)
{
    viewer->updateViewport(viewer, 0, 0, width, height);
    trackball->updateViewport(trackball, 0, 0, width, height);
}

void
mouse(int button, int state, int x, int y)
{
    switch (button) {
    case GLUT_LEFT_BUTTON:
	if (state == GLUT_DOWN) {
	    moveMode = MoveObject;
	    trackball->startMotion(trackball, x, y, glutGet(GLUT_ELAPSED_TIME));
	} else {
	    trackball->stopMotion(trackball, x, y, glutGet(GLUT_ELAPSED_TIME));
	    if (!trackball->spining) {
		moveMode = MoveNone;
	    }
	}
	glutPostRedisplay();
	break;
    case GLUT_MIDDLE_BUTTON:
	if (state == GLUT_DOWN) {
	    moveMode = MoveLight;
	    trackball->startMotion(trackball, x, y, glutGet(GLUT_ELAPSED_TIME));
	} else {
	    trackball->stopMotion(trackball, x, y, glutGet(GLUT_ELAPSED_TIME));
	    if (!trackball->spining) {
		moveMode = MoveNone;
	    }
	}
	glutPostRedisplay();
	break;
    case GLUT_RIGHT_BUTTON:
	break;
    }
}

void
motion(int x, int y)
{
    if (moveMode != MoveNone) {
	trackball->trackMotion(trackball, x, y, glutGet(GLUT_ELAPSED_TIME));
	glutPostRedisplay();
    }
}

/*ARGSUSED1*/
void
keyboard(unsigned char key, int x, int y)
{
	int i;

    switch (key) {
    case 27: /* esc */
	exit(0);
	return;
	
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	bumpModeIndex = (key - '1');
	break;
	case '8':
	case '9':
	objectIndex = (key - '8');
	break;
	case '0':
	objectIndex = 2;
	break;
    case 'a':
	if (supportsEXT_blend_subtract) {
	    useBlending = !useBlending;
	}
	break;
    case 'b':
	bumpMapping = !bumpMapping;
	if (bumpMapping) bumpModeIndex = 0;
	break;
    case 'c':
	cullfaceEnabled = !cullfaceEnabled;
	break;
    case 'f':
	useFalloff = !useFalloff;
	break;
    case 'r':
	useSmoothFalloff = !useSmoothFalloff;
	break;
    case 'g':
	displayBackground = !displayBackground;
	break;
    case 'i':
	++textureIndex;
	if (textureIndex >= NUM_TEXTURES) textureIndex = 0;
	break;
    case 'k':
		++materialIndex;
		if (materialIndex >= NUM_MATERIALS) materialIndex = 0;
		
		/* Setup power function for specularity */
		for (i=0; i<POWER_TABLE_SIZE; i++)
			power[i] = exp(materialList[materialIndex]->shininess * log(i/(POWER_TABLE_SIZE - 1.0)));
		glPixelMapfv(GL_PIXEL_MAP_R_TO_R, POWER_TABLE_SIZE, power);
		glPixelMapfv(GL_PIXEL_MAP_G_TO_G, POWER_TABLE_SIZE, power);
		glPixelMapfv(GL_PIXEL_MAP_B_TO_B, POWER_TABLE_SIZE, power);
		break;

    case 'l':
	lightingEnabled = !lightingEnabled;
	break;
    case 'm':
	++bumpModeIndex;
	if (bumpModeIndex >= NUM_BUMP_MODES) bumpModeIndex = 0;
	break;
    case 'o':
	++objectIndex;
	if (objectIndex >= NUM_OBJECTS) objectIndex = 0;
	break;
    case 's':
	displayLightPosition = !displayLightPosition;
	break;
    case 't':
	textureEnabled = !textureEnabled;
	break;
    case 'v':
	displayVectors = !displayVectors;
	break;
    case 'w':
	displayWireframe = !displayWireframe;
	break;
    default:
	return;
    }
    glutPostRedisplay();
}

void menu(int option)
{
	keyboard((unsigned char)option, 0, 0);
}

int
main(int argc, char **argv)
{
	int stepMenu, objectMenu;

    glutInit(&argc, argv);
    if (argc > 1) {
	imageFilename = argv[1];
    }
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ACCUM);
    glutInitWindowSize(512, 512);
    glutCreateWindow("OpenGL Bump Mapping");
    init();
    glutDisplayFunc(display);
    glutVisibilityFunc(visibility);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);

	stepMenu = glutCreateMenu(menu);
	glutAddMenuEntry("Show all bump mapping steps ('1')", '1');
	glutAddMenuEntry("Show specular on geometry only ('2')", '2');
	glutAddMenuEntry("Show specular on bumps only ('3')", '3');
	glutAddMenuEntry("Show all specular steps ('4')", '4');
	glutAddMenuEntry("Show diffuse on geometry only ('5')", '5');
	glutAddMenuEntry("Show diffuse on bumps only ('6')", '6');
	glutAddMenuEntry("Show all diffuse steps ('7')", '7');

    objectMenu = glutCreateMenu(menu);
    glutAddMenuEntry("Rectangle ('8')", '8');
    glutAddMenuEntry("Cylinder ('9')", '9');
    glutAddMenuEntry("Torus ('0')", '0');

    glutCreateMenu(menu);
    glutAddMenuEntry("Toggle bump mapping ('b')", 'b');
    glutAddSubMenu("Show steps:", stepMenu);
    glutAddSubMenu("Show object:", objectMenu);
    glutAddMenuEntry("Toggle texture ('t')", 't');
    glutAddMenuEntry("Change texture image ('i')", 'i');
    glutAddMenuEntry("Change material constants ('k')", 'k');
    glutAddMenuEntry("Toggle using accumulation or blending ('a')", 'a');
    glutAddMenuEntry("Toggle back face culling ('c')", 'c');
    glutAddMenuEntry("Toggle background ('g')", 'g');
    glutAddMenuEntry("Toggle vertex lighting ('l')", 'l');
    glutAddMenuEntry("Toggle surface vectors ('v')", 'v');
    glutAddMenuEntry("Toggle lighting vector ('s')", 's');
    glutAddMenuEntry("Toggle bump intensity falloff ('f')", 'f');
    glutAddMenuEntry("Toggle smooth bump intensity falloff ('r')", 'r');
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    glutMainLoop();

    return 0;
}
