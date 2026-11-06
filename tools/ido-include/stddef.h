#ifndef IDO_STDDEF_H
#define IDO_STDDEF_H

typedef unsigned int size_t;
typedef int ptrdiff_t;
typedef int wchar_t;

#ifndef NULL
#define NULL 0
#endif

#define offsetof(type, member) ((size_t)&(((type *)0)->member))

#endif
