#include <ultra64.h>

/* A word that may alias the bytes of any buffer. */
typedef u32 __attribute__((may_alias)) word;

void *bcopy(const void *src, void *dst, int n) {
    const u8 *s = src;
    u8 *d = dst;
    int aligned = (((u32)s | (u32)d) & 3) == 0;

    if (d == s || n <= 0) {
        return dst;
    }
    if (d < s || d >= s + n) {
        /* Forwards when the destination does not overlap the end of the source. */
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
            /* Copy the odd tail bytes first so that the ends become word aligned. */
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
