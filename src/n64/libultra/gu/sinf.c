#include "common/libultra/gu/guint.h"

float sinf(float x) {
    double dx, xsq, poly, dn, result;
    int n, ix, xpt;

    /* The top bits of the exponent sort the argument into ranges. */
    ix = *(s32 *)&x;
    xpt = (ix >> 22) & 0x1FF;

    if (xpt < 0xFF) {
        /* Below about 0.5 in magnitude the polynomial applies without reduction. */
        dx = x;
        if (xpt >= 0xE6) {
            xsq = dx * dx;
            poly = ((g_adSinfP[4] * xsq + g_adSinfP[3]) * xsq + g_adSinfP[2]) * xsq + g_adSinfP[1];
            result = dx + (dx * xsq) * poly;
            return (float)result;
        }
        /* The sine of a tiny argument is the argument. */
        return x;
    }

    if (xpt < 0x136) {
        /* Reduce by the nearest multiple of pi, in two parts for precision. */
        dx = x;
        dn = dx * g_dSinfRpi;
        n = ROUND(dn);
        dn = n;
        dx = dx - dn * g_dSinfPio2Hi;
        dx = dx - dn * g_dSinfPio2Lo;
        xsq = dx * dx;
        poly = ((g_adSinfP[4] * xsq + g_adSinfP[3]) * xsq + g_adSinfP[2]) * xsq + g_adSinfP[1];
        result = dx + (dx * xsq) * poly;
        if ((n & 1) == 0) {
            return (float)result;
        }
        return -(float)result;
    }

    if (x != x) {
        return __libm_qnan_f;
    }
    return g_fSinfZero;
}
