#include <math.h>
#include <stdlib.h>
#include "curve.h"
#include "noise.h"


#define NOISESZ 64


float lattice[NOISESZ][NOISESZ][NOISESZ];


void noiseInit(void)
{
    int i, j, k;

    for(i = 0; i < NOISESZ; i++)
	for(j = 0; j < NOISESZ; j++)
	    for(k = 0; k < NOISESZ; k++)
#ifdef _WIN32
		lattice[i][j][k] = (float)rand()/(float)RAND_MAX;
#else
		lattice[i][j][k] = (float)drand48();
#endif
}


INLINE float noiseDiscrete3f(float x, float y, float z)
{
    int ix, iy, iz;

    if(x < 0) x += NOISESZ * ((int)(-x / NOISESZ) + 1);
    if(y < 0) y += NOISESZ * ((int)(-y / NOISESZ) + 1);
    if(z < 0) z += NOISESZ * ((int)(-z / NOISESZ) + 1);

    ix = (int)x;
    iy = (int)y;
    iz = (int)z;

    return(lattice[ix % NOISESZ][iy % NOISESZ][iz % NOISESZ]);
}


INLINE float noiseLinear3f(float x, float y, float z)
{
    int ix, iy, iz;
    int iu, iv, iw;
    float fx, fy, fz;
    float y1, y2;
    float x1, x2, x3, x4;

    if(x < 0) x += NOISESZ * ((int)(-x / NOISESZ) + 1);
    if(y < 0) y += NOISESZ * ((int)(-y / NOISESZ) + 1);
    if(z < 0) z += NOISESZ * ((int)(-z / NOISESZ) + 1);

    ix = (int)x;
    iy = (int)y;
    iz = (int)z;

    fx = x - ix;
    fy = y - iy;
    fz = z - iz;

    iu = (ix + 1) % NOISESZ;
    iv = (iy + 1) % NOISESZ;
    iw = (iz + 1) % NOISESZ;

    ix = ix % NOISESZ;
    iy = iy % NOISESZ;
    iz = iz % NOISESZ;

    x1 = lattice[ix][iy][iz] * (1 - fx) + lattice[iu][iy][iz] * fx;
    x2 = lattice[ix][iy][iw] * (1 - fx) + lattice[iu][iy][iw] * fx;
    x3 = lattice[ix][iv][iz] * (1 - fx) + lattice[iu][iv][iz] * fx;
    x4 = lattice[ix][iv][iw] * (1 - fx) + lattice[iu][iv][iw] * fx;

    y1 = x1 * (1 - fy) + x2 * fy;
    y2 = x3 * (1 - fy) + x4 * fy;

    return(y1 * (1 - fz) + y2 * fz);
}


INLINE float noiseBicubic2f(float x, float y)
{
    int ix[4];
    int iy[4];
    float u, v;
    float xnoise[4];
    float ynoise;
    int i;

    if(x < 0) x += NOISESZ * ((int)(-x / NOISESZ) + 1);
    if(y < 0) y += NOISESZ * ((int)(-y / NOISESZ) + 1);

    ix[1] = (int)x;
    iy[1] = (int)y;

    u = x - ix[1];
    v = y - iy[1];

    ix[0] = (ix[1] - 1 + NOISESZ) % NOISESZ;
    iy[0] = (iy[1] - 1 + NOISESZ) % NOISESZ;

    ix[2] = (ix[1] + 1) % NOISESZ;
    iy[2] = (iy[1] + 1) % NOISESZ;

    ix[3] = (ix[1] + 2) % NOISESZ;
    iy[3] = (iy[1] + 2) % NOISESZ;

    ix[1] = ix[1] % NOISESZ;
    iy[1] = iy[1] % NOISESZ;

#if 0
    for(i = 0; i < 4; i++)
        xnoise[i] = bsplineEval1D(u, lattice[ix[0]][iy[i]][0],
	    lattice[ix[1]][iy[i]][0], lattice[ix[2]][iy[i]][0],
	    lattice[ix[3]][iy[i]][0]);
#else
    bsplineIncInit(u);
    for(i = 0; i < 4; i++)
        xnoise[i] = bsplineIncEval1D(lattice[ix[0]][iy[i]][0],
	    lattice[ix[1]][iy[i]][0], lattice[ix[2]][iy[i]][0],
	    lattice[ix[3]][iy[i]][0]);
#endif

    ynoise = bsplineEval1D(v, xnoise[0], xnoise[1], xnoise[2], xnoise[3]);

    return(ynoise);
}


INLINE float noiseBicubic3f(float x, float y, float z)
{
    int ix[4];
    int iy[4];
    int iz[4];
    float u, v, w;
    float xnoise[16], ynoise[4], znoise;
    int i, j;

    if(x < 0) x += NOISESZ * ((int)(-x / NOISESZ) + 1);
    if(y < 0) y += NOISESZ * ((int)(-y / NOISESZ) + 1);
    if(z < 0) z += NOISESZ * ((int)(-z / NOISESZ) + 1);

    ix[1] = (int)x;
    iy[1] = (int)y;
    iz[1] = (int)z;

    u = x - ix[1];
    v = y - iy[1];
    w = z - iz[1];

    ix[0] = (ix[1] - 1 + NOISESZ) % NOISESZ;
    iy[0] = (iy[1] - 1 + NOISESZ) % NOISESZ;
    iz[0] = (iz[1] - 1 + NOISESZ) % NOISESZ;

    ix[2] = (ix[1] + 1) % NOISESZ;
    iy[2] = (iy[1] + 1) % NOISESZ;
    iz[2] = (iz[1] + 1) % NOISESZ;

    ix[3] = (ix[1] + 2) % NOISESZ;
    iy[3] = (iy[1] + 2) % NOISESZ;
    iz[3] = (iz[1] + 2) % NOISESZ;

    ix[1] = ix[1] % NOISESZ;
    iy[1] = iy[1] % NOISESZ;
    iz[1] = iz[1] % NOISESZ;

#if 0
    for(i = 0; i < 4; i++)
        for(j = 0; j < 4; j++)
            xnoise[i * 4 + j] = bsplineEval1D(u, lattice[ix[0]][iy[j]][iz[i]],
	        lattice[ix[1]][iy[j]][iz[i]], lattice[ix[2]][iy[j]][iz[i]],
		lattice[ix[3]][iy[j]][iz[i]]);
#else
    bsplineIncInit(u);
    for(i = 0; i < 4; i++)
        for(j = 0; j < 4; j++)
            xnoise[i * 4 + j] = bsplineIncEval1D(lattice[ix[0]][iy[j]][iz[i]],
	        lattice[ix[1]][iy[j]][iz[i]], lattice[ix[2]][iy[j]][iz[i]],
		lattice[ix[3]][iy[j]][iz[i]]);
#endif

#if 0
    for(i = 0; i < 16; i += 4)
        ynoise[i / 4] = bsplineEval1D(v, xnoise[i], xnoise[i + 1],
	    xnoise[i + 2], xnoise[i + 3]);
#else
    bsplineIncInit(v);
    for(i = 0; i < 16; i += 4)
        ynoise[i / 4] = bsplineIncEval1D(xnoise[i], xnoise[i + 1],
	    xnoise[i + 2], xnoise[i + 3]);
#endif

    znoise = bsplineEval1D(w, ynoise[0], ynoise[1], ynoise[2], ynoise[3]);

    return(znoise);
}


INLINE float noiseTurbulence2f(float x, float y, int levels)
{
    float t = 0;
    float scale = 1;

    while(levels-- > 0)
    {
        t += noiseBicubic2f(x / scale, y / scale) * scale;
	scale /= 2;
    }
    return(t);
}


INLINE float noiseTurbulence3f(float x, float y, float z, int levels)
{
    float t = 0;
    float scale = 1;

    while(levels-- > 0)
    {
        t += noiseBicubic3f(x / scale, y / scale, z / scale) * scale;
	scale /= 2;
    }
    return(t);
}


INLINE float noiseMarble2f(float x, float y)
{
    float t;

    t = y + 3 * noiseTurbulence2f(x, y, 4);
    return((float)pow(.5 + .5 * sin(t * 2), .3));
}


INLINE float noiseMarble3f(float x, float y, float z)
{
    float t;

    t = x + 3 * noiseTurbulence3f(x, y, z, 4);
    return((float)pow(.5 + .5 * sin(t * 2), .3));
}


/*
 * Cloudy function courtesy of Lawrence Kesteloot, lk@pdi.com
 */
INLINE float noiseCloud2f(float x, float y)
{
    float t, size;
    int i;

    x /= 10.f;
    y /= 10.f;

    t = 0.f;
    size = 4.f;
    for (i = 0; i < 6; i++) {
        t += 4.f * (float)fabs(.5f - noiseBicubic2f(x * size, y * size)) / size;
        size *= 2.f;
    }

    t = t*t*t*10.f;

    return(t);
}


INLINE float noiseCloud3f(float x, float y, float z)
{
    float t, size;
    int i;

    x /= 10.f;
    y /= 10.f;
    z /= 10.f;

    t = 0.f;
    size = 4.f;
    for (i = 0; i < 6; i++) {
        t += 4.f * (float)fabs(.5f - noiseBicubic3f(x * size, y * size, z * size)) /
	    size;
        size *= 2.f;
    }

    t = t*t*t*10.f;

    return(t);
}
