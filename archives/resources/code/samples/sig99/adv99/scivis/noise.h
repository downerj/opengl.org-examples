	/* sidestep SGI's compiler */
#ifndef INLINE
#if defined(__sgi) || defined(_WIN32)
#define INLINE
#else /* ! __sgi */
#define INLINE inline
#endif /* __sgi */
#endif /* inline */


void noiseInit(void);
INLINE float noiseDiscrete3f(float x, float y, float z);
INLINE float noiseLinear3f(float x, float y, float z);
INLINE float noiseBicubic2f(float x, float y);
INLINE float noiseBicubic3f(float x, float y, float z);
INLINE float noiseTurbulence2f(float x, float y, int levels);
INLINE float noiseTurbulence3f(float x, float y, float z, int levels);
INLINE float noiseMarble2f(float x, float y);
INLINE float noiseMarble3f(float x, float y, float z);
INLINE float noiseCloud2f(float x, float y);
INLINE float noiseCloud3f(float x, float y, float z);
