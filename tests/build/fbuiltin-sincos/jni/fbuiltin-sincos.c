# include <math.h>

#ifdef PROVIDE_SINCOS
void sincos(double x, double *sin_result, double *cos_result)
{
    *sin_result = sin(x);
    *cos_result = cos(x);
}
#endif

double func(double a)
{
    double c, s;
    c = cos(a);
    s = sin(a);
    return c + s;
}

int main()
{
    func(99.0);
}
