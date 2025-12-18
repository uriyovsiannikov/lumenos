#include "math.h"
static double _sin_approx(double x) {
  double result = x;
  double term = x;
  double x2 = x * x;
  for (int i = 1; i < 10; i++) {
    term *= -x2 / ((2 * i) * (2 * i + 1));
    result += term;
  }
  return result;
}
static double _cos_approx(double x) {
  double result = 1.0;
  double term = 1.0;
  double x2 = x * x;
  for (int i = 1; i < 10; i++) {
    term *= -x2 / ((2 * i - 1) * (2 * i));
    result += term;
  }
  return result;
}
static double _reduce_angle(double x) {
  while (x > M_PI)
    x -= 2 * M_PI;
  while (x < -M_PI)
    x += 2 * M_PI;
  return x;
}
int abs(int x) { return (x < 0) ? -x : x; }
double fabs(double x) { return (x < 0) ? -x : x; }
double sqrt(double x) {
  if (x < 0)
    return 0;
  if (x == 0)
    return 0;
  double result = x;
  double prev;
  do {
    prev = result;
    result = 0.5 * (result + x / result);
  } while (fabs(result - prev) > 1e-10);
  return result;
}
double pow(double x, double y) {
  if (y == 0)
    return 1;
  if (y == 1)
    return x;
  if (x == 0)
    return 0;
  if (y == (int)y) {
    int n = (int)y;
    double result = 1;
    for (int i = 0; i < abs(n); i++) {
      result *= x;
    }
    return (n < 0) ? 1.0 / result : result;
  }
  return exp(y * log(x));
}
double exp(double x) {
  double result = 1.0;
  double term = 1.0;
  for (int i = 1; i < 20; i++) {
    term *= x / i;
    result += term;
  }
  return result;
}
double log(double x) {
  if (x <= 0)
    return 0;
  double y = (x - 1) / (x + 1);
  double result = 0;
  double y2 = y * y;
  double term = y;
  for (int i = 0; i < 20; i++) {
    result += term / (2 * i + 1);
    term *= y2;
  }
  return 2 * result;
}
double log10(double x) { return log(x) / log(10); }
double sin(double x) {
  x = _reduce_angle(x);
  return _sin_approx(x);
}
double cos(double x) {
  x = _reduce_angle(x);
  return _cos_approx(x);
}
double tan(double x) { return sin(x) / cos(x); }
double asin(double x) {
  if (x < -1 || x > 1)
    return 0;
  return atan(x / sqrt(1 - x * x));
}
double acos(double x) {
  if (x < -1 || x > 1)
    return 0;
  return M_PI / 2 - asin(x);
}
double atan(double x) {
  if (x > 1)
    return M_PI / 2 - atan(1 / x);
  if (x < -1)
    return -M_PI / 2 - atan(1 / x);
  double result = x;
  double term = x;
  double x2 = x * x;
  for (int i = 1; i < 20; i++) {
    term *= -x2;
    result += term / (2 * i + 1);
  }
  return result;
}
double atan2(double y, double x) {
  if (x > 0)
    return atan(y / x);
  if (x < 0 && y >= 0)
    return atan(y / x) + M_PI;
  if (x < 0 && y < 0)
    return atan(y / x) - M_PI;
  if (x == 0 && y > 0)
    return M_PI / 2;
  if (x == 0 && y < 0)
    return -M_PI / 2;
  return 0;
}
double sinh(double x) { return (exp(x) - exp(-x)) / 2; }
double cosh(double x) { return (exp(x) + exp(-x)) / 2; }
double tanh(double x) { return sinh(x) / cosh(x); }
double ceil(double x) {
  int intpart = (int)x;
  if (x > intpart)
    return intpart + 1;
  return intpart;
}
double floor(double x) { return (int)x; }
double round(double x) { return (x >= 0) ? (int)(x + 0.5) : (int)(x - 0.5); }
double fmod(double x, double y) {
  if (y == 0)
    return 0;
  int n = (int)(x / y);
  return x - n * y;
}
double modf(double x, double *intpart) {
  *intpart = (int)x;
  return x - *intpart;
}
