#include "string.h"
#include "ctype.h"
#include <stdarg.h>
#include <stdint.h>
char *strchr(const char *s, int c) {
  while (*s != (char)c)
    if (!*s++)
      return NULL;
  return (char *)s;
}
void *memcpy(void *dest, const void *src, size_t n) {
  char *d = dest;
  const char *s = src;
  while (n--)
    *d++ = *s++;
  return dest;
}
int memcmp(const void *ptr1, const void *ptr2, size_t num) {
  const unsigned char *p1 = (const unsigned char *)ptr1;
  const unsigned char *p2 = (const unsigned char *)ptr2;
  for (size_t i = 0; i < num; i++) {
    if (p1[i] != p2[i]) {
      return (int)p1[i] - (int)p2[i];
    }
  }
  return 0;
}
void *memset(void *ptr, int value, size_t num) {
  unsigned char *p = ptr;
  while (num--)
    *p++ = (unsigned char)value;
  return ptr;
}
char *strcpy(char *dest, const char *src) {
  char *ret = dest;
  while ((*dest++ = *src++))
    ;
  return ret;
}
char *strncpy(char *dest, const char *src, size_t n) {
  char *ret = dest;
  while (n-- && (*dest++ = *src++))
    ;
  return ret;
}
size_t strlen(const char *str) {
  size_t len = 0;
  while (str[len])
    len++;
  return len;
}
int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2))
    s1++, s2++;
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}
int strncmp(const char *s1, const char *s2, size_t n) {
  while (n && *s1 && (*s1 == *s2))
    s1++, s2++, n--;
  return n ? *(unsigned char *)s1 - *(unsigned char *)s2 : 0;
}
int snprintf(char *str, size_t size, const char *format, ...) {
  va_list args;
  va_start(args, format);
  size_t i = 0;
  while (*format && i < size - 1) {
    if (*format == '%') {
      format++;
      switch (*format) {
      case 's': {
        char *s = va_arg(args, char *);
        while (*s && i < size - 1) {
          str[i++] = *s++;
        }
        break;
      }
      case 'c':
        if (i < size - 1) {
          str[i++] = (char)va_arg(args, int);
        }
        break;
      default:
        if (i < size - 1) {
          str[i++] = *format;
        }
      }
    } else {
      if (i < size - 1) {
        str[i++] = *format;
      }
    }
    format++;
  }
  str[i] = '\0';
  va_end(args);
  return i;
}
void *memmove(void *dest, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dest;
  const unsigned char *s = (const unsigned char *)src;
  if (d == s)
    return dest;
  if (d < s) {
    for (size_t i = 0; i < n; i++) {
      d[i] = s[i];
    }
  } else {
    for (size_t i = n; i != 0; i--) {
      d[i - 1] = s[i - 1];
    }
  }
  return dest;
}
long strtol(const char *str, char **endptr, int base) {
  long result = 0;
  while (*str) {
    int digit;
    if (*str >= '0' && *str <= '9')
      digit = *str - '0';
    else if (*str >= 'a' && *str <= 'f')
      digit = *str - 'a' + 10;
    else if (*str >= 'A' && *str <= 'F')
      digit = *str - 'A' + 10;
    else
      break;
    if (digit >= base)
      break;
    result = result * base + digit;
    str++;
  }
  if (endptr)
    *endptr = (char *)str;
  return result;
}
char *strtok(char *str, const char *delim) {
  static char *saved_ptr = NULL;
  if (str)
    saved_ptr = str;
  if (!saved_ptr || !*saved_ptr)
    return NULL;
  char *start = saved_ptr;
  while (*saved_ptr && !strchr(delim, *saved_ptr))
    saved_ptr++;
  if (*saved_ptr) {
    *saved_ptr = '\0';
    saved_ptr++;
  } else {
    saved_ptr = NULL;
  }
  return start;
}
char *strstr(const char *haystack, const char *needle) {
  if (!haystack || !needle || !*needle)
    return (char *)haystack;
  for (; *haystack; haystack++) {
    if (*haystack == *needle) {
      const char *h = haystack, *n = needle;
      while (*h && *n && *h == *n) {
        h++;
        n++;
      }
      if (!*n)
        return (char *)haystack;
    }
  }
  return NULL;
}
char *strcat(char *dest, const char *src) {
  char *ptr = dest;
  while (*ptr != '\0') {
    ptr++;
  }
  while (*src != '\0') {
    *ptr++ = *src++;
  }
  *ptr = '\0';
  return dest;
}
char *itoa(int value, char *str, int base) {
  char *ptr = str;
  char *ptr1 = str;
  char tmp_char;
  int tmp_value;
  if (base < 2 || base > 36) {
    *str = '\0';
    return str;
  }
  do {
    tmp_value = value;
    value /= base;
    *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrst"
             "uvwxyz"[35 + (tmp_value - value * base)];
  } while (value);
  if (tmp_value < 0 && base == 10) {
    *ptr++ = '-';
  }
  *ptr-- = '\0';
  while (ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr-- = *ptr1;
    *ptr1++ = tmp_char;
  }
  return str;
}
void trim_string(char *str) {
  if (str == NULL)
    return;
  char *start = str;
  while (*start && isspace(*start)) {
    start++;
  }
  if (start != str) {
    char *dst = str;
    while (*start) {
      *dst++ = *start++;
    }
    *dst = '\0';
  }
  char *end = str + strlen(str) - 1;
  while (end >= str && isspace(*end)) {
    *end-- = '\0';
  }
  char *dst = str;
  char *src = str;
  int space_count = 0;
  while (*src) {
    if (isspace(*src)) {
      if (space_count == 0) {
        *dst++ = ' ';
        space_count++;
      }
    } else {
      *dst++ = *src;
      space_count = 0;
    }
    src++;
  }
  *dst = '\0';
}
int sprintf(char *str, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int result = vsprintf(str, format, args);
  va_end(args);
  return result;
}
char *strncat(char *dest, const char *src, size_t n) {
  char *ptr = dest + strlen(dest);
  while (*src && n--) {
    *ptr++ = *src++;
  }
  *ptr = '\0';
  return dest;
}
static void reverse_string(char *str, int length) {
  int start = 0;
  int end = length - 1;
  while (start < end) {
    char temp = str[start];
    str[start] = str[end];
    str[end] = temp;
    start++;
    end--;
  }
}
static int itoa_int(int value, char *str, int base) {
  int i = 0;
  int is_negative = 0;
  if (value == 0) {
    str[i++] = '0';
    str[i] = '\0';
    return i;
  }
  if (value < 0 && base == 10) {
    is_negative = 1;
    value = -value;
  }
  while (value != 0) {
    int rem = value % base;
    str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
    value = value / base;
  }
  if (is_negative) {
    str[i++] = '-';
  }
  str[i] = '\0';
  reverse_string(str, i);
  return i;
}
static int itoa_uint(unsigned int value, char *str, int base) {
  int i = 0;
  if (value == 0) {
    str[i++] = '0';
    str[i] = '\0';
    return i;
  }
  while (value != 0) {
    unsigned int rem = value % base;
    str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
    value = value / base;
  }
  str[i] = '\0';
  reverse_string(str, i);
  return i;
}
static int itoa_hex(uint32_t value, char *str, int uppercase) {
  const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
  int i = 0;
  if (value == 0) {
    str[i++] = '0';
    str[i] = '\0';
    return i;
  }
  while (value != 0) {
    uint32_t rem = value & 0xF;
    str[i++] = digits[rem];
    value >>= 4;
  }
  str[i] = '\0';
  reverse_string(str, i);
  return i;
}
int vsprintf(char *str, const char *format, va_list args) {
  char buffer[32];
  char *ptr = str;
  while (*format) {
    if (*format == '%') {
      format++;
      switch (*format) {
      case 'd': {
        int num = va_arg(args, int);
        itoa(num, buffer, 10);
        char *p = buffer;
        while (*p)
          *ptr++ = *p++;
        break;
      }
      case 's': {
        char *s = va_arg(args, char *);
        while (*s)
          *ptr++ = *s++;
        break;
      }
      case 'c': {
        char c = (char)va_arg(args, int);
        *ptr++ = c;
        break;
      }
      default:
        *ptr++ = '%';
        *ptr++ = *format;
        break;
      }
    } else {
      *ptr++ = *format;
    }
    format++;
  }
  *ptr = '\0';
  return ptr - str;
}
int vsnprintf(char *buffer, size_t size, const char *format, va_list args) {
  if (!buffer || !format || size == 0) {
    return 0;
  }
  char temp_buffer[32];
  size_t buf_index = 0;
  const char *p = format;
  while (*p && buf_index < size - 1) {
    if (*p != '%') {
      buffer[buf_index++] = *p++;
      continue;
    }
    p++;
    switch (*p) {
    case 'd':
    case 'i': {
      int value = va_arg(args, int);
      int len = itoa_int(value, temp_buffer, 10);
      for (int i = 0; i < len && buf_index < size - 1; i++) {
        buffer[buf_index++] = temp_buffer[i];
      }
      break;
    }
    case 'u': {
      unsigned int value = va_arg(args, unsigned int);
      int len = itoa_uint(value, temp_buffer, 10);
      for (int i = 0; i < len && buf_index < size - 1; i++) {
        buffer[buf_index++] = temp_buffer[i];
      }
      break;
    }
    case 'x': {
      uint32_t value = va_arg(args, uint32_t);
      int len = itoa_hex(value, temp_buffer, 0);
      for (int i = 0; i < len && buf_index < size - 1; i++) {
        buffer[buf_index++] = temp_buffer[i];
      }
      break;
    }
    case 'X': {
      uint32_t value = va_arg(args, uint32_t);
      int len = itoa_hex(value, temp_buffer, 1);
      for (int i = 0; i < len && buf_index < size - 1; i++) {
        buffer[buf_index++] = temp_buffer[i];
      }
      break;
    }
    case 'c': {
      char c = (char)va_arg(args, int);
      if (buf_index < size - 1) {
        buffer[buf_index++] = c;
      }
      break;
    }
    case 's': {
      const char *str = va_arg(args, const char *);
      if (!str)
        str = "(null)";
      while (*str && buf_index < size - 1) {
        buffer[buf_index++] = *str++;
      }
      break;
    }
    case 'p': {
      void *ptr = va_arg(args, void *);
      buffer[buf_index++] = '0';
      if (buf_index < size - 1)
        buffer[buf_index++] = 'x';
      uint32_t value = (uint32_t)ptr;
      int len = itoa_hex(value, temp_buffer, 0);
      for (int i = 0; i < len && buf_index < size - 1; i++) {
        buffer[buf_index++] = temp_buffer[i];
      }
      break;
    }
    case '%': {
      if (buf_index < size - 1) {
        buffer[buf_index++] = '%';
      }
      break;
    }
    default:
      if (buf_index < size - 1) {
        buffer[buf_index++] = '%';
      }
      if (buf_index < size - 1) {
        buffer[buf_index++] = *p;
      }
      break;
    }
    p++;
  }
  buffer[buf_index] = '\0';
  return buf_index;
}
char *strrchr(const char *str, int ch) {
  const char *last_occurrence = NULL;
  char target = (char)ch;
  while (*str) {
    if (*str == target) {
      last_occurrence = str;
    }
    str++;
  }
  if (target == '\0') {
    return (char *)str;
  }
  return (char *)last_occurrence;
}
int strcasecmp(const char *s1, const char *s2) {
  while (*s1 && *s2) {
    char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? (*s1 + 32) : *s1;
    char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? (*s2 + 32) : *s2;
    if (c1 != c2) {
      return c1 - c2;
    }
    s1++;
    s2++;
  }
  return *s1 - *s2;
}
static void io_delay(void) {
  for (int i = 0; i < 100; i++) {
    asm volatile("nop");
  }
}