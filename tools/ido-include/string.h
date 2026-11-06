#ifndef IDO_STRING_H
#define IDO_STRING_H

#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *dst, int value, size_t n);
size_t strlen(const char *s);
char *strchr(const char *s, int c);

#endif
