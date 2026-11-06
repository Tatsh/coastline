#ifndef IDO_STDLIB_H
#define IDO_STDLIB_H

#include <stddef.h>

void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void free(void *pointer);
void exit(int status);
int abs(int value);

#endif
