/*
	ripple_precalc.c
	Drew Olbrich, 1992

	This program precomputes the data used to generate the
	ripple pattern.  A C source file containing the data is
	produced.  This file is compiled and linked into the
	final executable.
*/

#include <stdio.h>
#include <math.h>

#include "defs.h"
#include "ripple.h"

/*
	Precompute ripple displacement vectors.
*/

void precalc_ripple_vector()
{
  int i, j;
  float x, y, l;

  printf("RIPPLE_VECTOR ripple_vector[GRID_SIZE_X][GRID_SIZE_Y] =\n");
  printf("{\n");

  for (i = 0; i < GRID_SIZE_X; i++)
  {
    for (j = 0; j < GRID_SIZE_Y; j++)
    {
      x = (float) i/(GRID_SIZE_X - 1);
      y = (float) j/(GRID_SIZE_Y - 1);

      l = (float) sqrt(x*x + y*y);
      if (l == 0.0)
      {
	x = 0.0f;
	y = 0.0f;
      }
      else
      {
	x /= l;
	y /= l;
      }

      printf("  %g, %g, %d", x, y, (int) (l*WIN_SIZE_X*2));

      if (i == GRID_SIZE_X - 1 && j == GRID_SIZE_Y - 1)
	printf("\n");
      else
	printf(",\n");
    }
  }

  printf("};\n");
}

/*
	Precompute ripple amplitude decay.
*/

void precalc_ripple_amp()
{
  int i;
  double t;
  double a;

  printf("RIPPLE_AMP ripple_amp[RIPPLE_LENGTH] =\n");
  printf("{\n");

  for (i = 0; i < RIPPLE_LENGTH; i++)
  {
    t = 1.0 - i/(RIPPLE_LENGTH - 1.0);
    a = (-cos(t*2.0*3.1428571*RIPPLE_CYCLES)*0.5 + 0.5)
      *RIPPLE_AMPLITUDE*t*t*t*t*t*t*t*t;
    if (i == 0)
      a = 0.0;

    printf("  %g", a);

    if (i == RIPPLE_LENGTH - 1)
      printf("\n");
    else
      printf(",\n");
  }

  printf("};\n");
}

/*
	Generate the source file.
*/

int main()
{
  printf("/* THIS FILE WAS MACHINE-GENERATED */\n");
  printf("\n");

  printf("#include \"defs.h\"\n");
  printf("#include \"ripple.h\"\n");
  printf("\n");

  precalc_ripple_vector();

  printf("\n");

  precalc_ripple_amp();

  return 0;
}
