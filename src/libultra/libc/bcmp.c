#include <ultra64.h>

int bcmp(const void *a, const void *b, int n) {
    const u8 *pa = a;
    const u8 *pb = b;

    while (n-- > 0) {
        if (*pa++ != *pb++) {
            return 1;
        }
    }
    return 0;
}
