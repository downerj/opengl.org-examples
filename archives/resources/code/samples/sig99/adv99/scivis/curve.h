#ifndef __curve_h__
#define __curve_h__

#if defined(__sgi) || defined(_WIN32) /* sgi CC doesn't like inline */

float bsplineEval1D(float t, float p0, float p1, float p2, float p3);

#else /* GCC loves it */

static inline float bsplineEval1D(float t, float p0, float p1, float p2,
    float p3)
{
    float t2 = t * t;
    float t3 = t2 * t;
    float sum = 0;

    t2 *= 3;
    t *= 3;
    
    sum += p0 * (-t3 + t2 - t + 1);
    sum += p1 * (3 * t3 - 2 * t2 + 4);
    sum += p2 * (-3 * t3 + t2 + t + 1);
    sum += p3 * t3;

    return(sum / 6);
}

#endif /* compiler type */


float bsplineIncInit(float t);
float bsplineIncEval1D(float p0, float p1, float p2, float p3);


#endif /* __curve_h__ */
