#include "guint.h"

float cosf(float x) {
    double dx, xsq, poly, dn, result;
    float xabs;
    int n, xpt;
    union {
        float f; /*!< The argument. */
        s32 i;   /*!< The bits of the argument, for its exponent. */
    } u = {x};

    xpt = (u.i >> 22) & 0x1FF;

    if (xpt < 0x136) {
        /* The cosine is the sine shifted by half pi, so reduce by an odd multiple of it. */
        xabs = (x > 0.0f) ? x : -x;
        dx = xabs;
        dn = dx * g_dCosfRpi + 0.5;
        n = ROUND(dn);
        dn = n;
        dn -= 0.5;
        dx = dx - dn * g_dCosfPio2Hi;
        dx = dx - dn * g_dCosfPio2Lo;
        xsq = dx * dx;
        poly = ((g_adCosfP[4] * xsq + g_adCosfP[3]) * xsq + g_adCosfP[2]) * xsq + g_adCosfP[1];
        result = dx + (dx * xsq) * poly;
        if ((n & 1) == 0) {
            return (float)result;
        }
        return -(float)result;
    }

    if (x != x) {
        return __libm_qnan_f;
    }
    return g_fCosfZero;
}
