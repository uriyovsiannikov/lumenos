#include "stdio.h"
#include <stdarg.h>
int sscanf(const char *str, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int count = 0;
    while (*fmt && *str) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd': {
                    int *ptr = va_arg(args, int*);
                    *ptr = 0;
                    int sign = 1;
                    while (*str == ' ' || *str == '\t') str++;
                    if (*str == '-') {
                        sign = -1;
                        str++;
                    } else if (*str == '+') {
                        str++;
                    }
                    while (*str >= '0' && *str <= '9') {
                        *ptr = (*ptr * 10) + (*str - '0');
                        str++;
                    }
                    *ptr *= sign;
                    count++;
                    break;
                }
                case 'c': {
                    char *ptr = va_arg(args, char*);
                    while (*str == ' ' || *str == '\t') str++;
                    *ptr = *str++;
                    count++;
                    break;
                }
                case 's': {
                    char *ptr = va_arg(args, char*);
                    while (*str == ' ' || *str == '\t') str++;
                    while (*str && *str != ' ' && *str != '\t' && *str != '\n') {
                        *ptr++ = *str++;
                    }
                    *ptr = '\0';
                    count++;
                    break;
                }
                default:
                    break;
            }
            fmt++;
        } else {
            if (*fmt == ' ' || *fmt == '\t') {
                fmt++;
                while (*str == ' ' || *str == '\t') str++;
            } else {
                if (*fmt++ != *str++) {
                    break;
                }
            }
        }
    }
    va_end(args);
    return count;
}