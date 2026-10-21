#include <ultra64.h>

void *memcpy(void *dst, const void *src, size_t n) {
    char *d = dst;
    const char *s = src;

    while (n > 0) {
        *d++ = *s++;
        n--;
    }
    return dst;
}

size_t strlen(const char *s) {
    const char *sc = s;

    while (*sc != '\0') {
        sc++;
    }
    return sc - s;
}

const char *strchr(const char *s, int c) {
    const char ch = c;

    while (*s != ch) {
        if (*s == '\0') {
            return NULL;
        }
        s++;
    }
    return s;
}
