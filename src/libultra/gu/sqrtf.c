#include <ultra64.h>

#ifndef __GNUC__
#pragma intrinsic(sqrtf)
#endif

float sqrtf(float x) {
#ifdef __GNUC__
    float r;

    /* The FPU instruction directly, since GCC would call this routine for negative values to
     * set errno. */
    __asm__("sqrt.s %0, %1" : "=f"(r) : "f"(x));
    return r;
#else
    return sqrtf(x);
#endif
}
