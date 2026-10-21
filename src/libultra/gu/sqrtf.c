#include <ultra64.h>

float sqrtf(float x) {
    float r;

    /* The FPU instruction directly, since GCC would call this routine for negative values to
     * set errno. */
    __asm__("sqrt.s %0, %1" : "=f"(r) : "f"(x));
    return r;
}
