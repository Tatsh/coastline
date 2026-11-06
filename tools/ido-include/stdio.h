#ifndef IDO_STDIO_H
#define IDO_STDIO_H

#include <stddef.h>

typedef struct FILE FILE;
extern FILE *stderr;
int fprintf(FILE *stream, const char *format, ...);
int sprintf(char *buffer, const char *format, ...);

#endif
