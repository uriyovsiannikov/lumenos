#ifndef STRING_H
#define STRING_H
#include <stdarg.h>
#include <stddef.h>
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *ptr, int value, size_t num);
char *strcpy(char *dest, const char *src);
int memcmp(const void *ptr1, const void *ptr2, size_t num);
char *strncpy(char *dest, const char *src, size_t n);
size_t strlen(const char *str);
char *strchr(const char *s, int c);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int snprintf(char *str, size_t size, const char *format, ...);
int isdigit(int c);
long strtol(const char *str, char **endptr, int base);
char *strtok(char *str, const char *delim);
char *strstr(const char *haystack, const char *needle);
char *strcat(char *dest, const char *src);
char *itoa(int value, char *str, int base);
void trim_string(char *str);
int sprintf(char *str, const char *format, ...);
char *strncat(char *dest, const char *src, size_t n);
int vsprintf(char *str, const char *format, va_list args);
int vsnprintf(char *buffer, size_t size, const char *format, va_list args);
char *strrchr(const char *str, int ch);
int strcasecmp(const char *s1, const char *s2);
#endif
