#include <ultra64.h>

typedef u32 __attribute__((may_alias)) word;

void bzero(void *p, int n) {
    u8 *b = p;

    for (; n > 0 && ((u32)b & 3) != 0; n--) {
        *b++ = 0;
    }
    for (; n >= 4; n -= 4, b += 4) {
        *(word *)b = 0;
    }
    while (n-- > 0) {
        *b++ = 0;
    }
}
