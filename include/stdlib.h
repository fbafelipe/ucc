#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>

extern void *calloc(size_t nmemb, size_t size);

extern void *malloc(size_t size);

extern void free(void *ptr);

extern void *realloc(void *ptr, size_t size);

extern void abort(void);

#endif
