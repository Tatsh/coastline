#include "common/libultra/gu/guint.h"

s16 sins(u16 x) {
    s16 value;

    /* The table has 1024 entries for a quarter turn. */
    x >>= 4;
    if (x & 0x400) {
        value = sintable[0x3FF - (x & 0x3FF)];
    } else {
        value = sintable[x & 0x3FF];
    }
    if (x & 0x800) {
        return -value;
    }
    return value;
}
