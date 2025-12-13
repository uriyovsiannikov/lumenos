#ifndef CTYPE_H
#define CTYPE_H
int isalnum(int c);
int isalpha(int c);
int iscntrl(int c);
int isgraph(int c);
int islower(int c);
int ispunct(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
int isblank(int c);
int tolower(int c);
int toupper(int c);
int atoi(const char *str);
long atol(const char *str);
#endif