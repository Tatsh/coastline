/*
 * Host equivalents of bcopy(), bzero(), and bcmp(), which the cartridge builds from the
 * hand-written assembly of the original.
 */
#include <stdint.h>

#include <ultra64.h>

/* A word that may alias the bytes of any buffer. */
typedef u32 __attribute__((may_alias)) word;

void *lu_bcopy(const void *src, void *dst, int n) {
    const u8 *s = src;
    u8 *d = dst;
    int aligned = (((u32)(uintptr_t)s | (u32)(uintptr_t)d) & 3) == 0;

    if (d == s || n <= 0) {
        return dst;
    }
    if (d < s || d >= s + n) {
        for (; aligned && n >= 4; n -= 4, s += 4, d += 4) {
            *(word *)d = *(const word *)s;
        }
        while (n-- > 0) {
            *d++ = *s++;
        }
    } else {
        s += n;
        d += n;
        if (aligned) {
            for (; (n & 3) != 0; n--) {
                *--d = *--s;
            }
            for (; n >= 4; n -= 4) {
                s -= 4;
                d -= 4;
                *(word *)d = *(const word *)s;
            }
        }
        while (n-- > 0) {
            *--d = *--s;
        }
    }
    return dst;
}

void lu_bzero(void *p, int n) {
    u8 *b = p;

    for (; n > 0 && ((uintptr_t)b & 3) != 0; n--) {
        *b++ = 0;
    }
    for (; n >= 4; n -= 4, b += 4) {
        *(word *)b = 0;
    }
    while (n-- > 0) {
        *b++ = 0;
    }
}

int lu_bcmp(const void *a, const void *b, int n) {
    const u8 *pa = a;
    const u8 *pb = b;

    while (n-- > 0) {
        if (*pa++ != *pb++) {
            return 1;
        }
    }
    return 0;
}
