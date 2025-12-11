#ifndef MATH_H
#define MATH_H
#define M_PI 3.14159265358979323846
#define M_E 2.71828182845904523536
int abs(int x);
double fabs(double x);
double sqrt(double x);
double pow(double x, double y);
double exp(double x);
double log(double x);
double log10(double x);
double sin(double x);
double cos(double x);
double tan(double x);
double asin(double x);
double acos(double x);
double atan(double x);
double atan2(double y, double x);
double sinh(double x);
double cosh(double x);
double tanh(double x);
double ceil(double x);
double floor(double x);
double round(double x);
double fmod(double x, double y);
double modf(double x, double *intpart);
#endif