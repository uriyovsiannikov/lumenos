#include "ctype.h"
int isalnum(int c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z');
}
int isalpha(int c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
int iscntrl(int c) { return (c >= 0 && c <= 31) || c == 127; }
int isgraph(int c) { return (c >= 33 && c <= 126); }
int islower(int c) { return (c >= 'a' && c <= 'z'); }
int ispunct(int c) {
  return (c >= 33 && c <= 47) || (c >= 58 && c <= 64) || (c >= 91 && c <= 96) ||
         (c >= 123 && c <= 126);
}
int isspace(int c) {
  return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' ||
          c == '\v');
}
int isupper(int c) { return (c >= 'A' && c <= 'Z'); }
int isxdigit(int c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
         (c >= 'a' && c <= 'f');
}
int isblank(int c) { return (c == ' ' || c == '\t'); }
int tolower(int c) {
  if (isupper(c)) {
    return c + ('a' - 'A');
  }
  return c;
}
int toupper(int c) {
  if (islower(c)) {
    return c - ('a' - 'A');
  }
  return c;
}
int atoi(const char *str) {
  int result = 0;
  int sign = 1;
  while (isspace(*str))
    str++;
  if (*str == '-') {
    sign = -1;
    str++;
  } else if (*str == '+') {
    str++;
  }
  while (isdigit(*str)) {
    result = result * 10 + (*str - '0');
    str++;
  }
  return sign * result;
}
long atol(const char *str) {
  long result = 0;
  int sign = 1;
  while (isspace(*str))
    str++;
  if (*str == '-') {
    sign = -1;
    str++;
  } else if (*str == '+') {
    str++;
  }
  while (isdigit(*str)) {
    result = result * 10 + (*str - '0');
    str++;
  }
  return sign * result;
}
