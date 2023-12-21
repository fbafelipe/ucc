#ifndef STRING_H
#define STRING_H

#include <stddef.h>

extern void *memcpy(void *dest, const void *src, size_t n);

extern char *strcpy(char *dest, const char *src);

extern char *strncpy(char *dest, const char *src, size_t n);

extern char *strcat(char *dest, const char *src);

extern char *strncat(char *dest, const char *src, size_t n);

size_t strlen(const char *s);

#endif
