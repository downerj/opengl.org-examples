float bsplineEval1D(float t, float p0, float p1, float p2, float p3)
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

/*
 *  sum += p0 * (-t3 + 3 * t2 - 3 * t + 1);
 *  sum += p1 * (3 * t3 - 6 * t2 + 4);
 *  sum += p2 * (-3 * t3 + 3 * t2 + 3 * t + 1);
 *  sum += p3 * t3;
 */

    return(sum / 6);
}

static float savedCo1, savedCo2, savedCo3, savedCo4;

void bsplineIncInit(float t)
{
    float t2 = t * t;
    float t3 = t2 * t;

    t2 *= 3;
    t *= 3;

    /*  Store precalculated coefficients. */
    /* Also predivide by 6. */
    savedCo1 = (-t3 + t2 - t + 1) / 6;
    savedCo2 = (3 * t3 - 2 * t2 + 4) / 6;
    savedCo3 = (-3 * t3 + t2 + t + 1) / 6;
    savedCo4 = (t3) / 6;
}


float bsplineIncEval1D(float p0, float p1, float p2, float p3)
{
    float sum;

    sum = p0 * savedCo1;
    sum += p1 * savedCo2;
    sum += p2 * savedCo3;
    sum += p3 * savedCo4;

    return(sum);
}
