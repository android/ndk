#include <math.h>

double test3 (double a, double b, double c)
{
  return - (a + b * c);
}
double test4 (double a, double b, double c)
{
  return - (-a + b * c);
}
double test5 (double a)
{
  return 1/a;
}
double test6 (double a)
{
  return 1/sqrt(a);
}

double a, b, c;

int main()
{
    test3(a, b, c);
    test4(a, b, c);
    test5(a);
    test6(a);
}
