#include "xstdio.h"

#define BUFF_LEN 0x20

/* The halfwords of a double, most significant first, and the fields of the first. */
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define _D0 3
#define _D1 2
#define _D2 1
#define _D3 0
#else
#define _D0 0
#define _D1 1
#define _D2 2
#define _D3 3
#endif
#define _DFRAC 0x000F
#define _DMASK 0x7FF0
#define _DMAX 0x7FF
#define _DOFF 4

/** @ghidraAddress USA: 0x8026d0f0 */
static void _Genld(_Pft *px, char code, const char *p, s16 nsig, s16 xexp) {
    const char point = '.';

    if (nsig <= 0) {
        nsig = 1;
        p = "0";
    }

    if (code == 'f' || ((code == 'g' || code == 'G') && xexp >= -4 && xexp < px->prec)) {
        /* Fixed-point notation. */
        xexp++;
        if (code != 'f') {
            if (!(px->flags & FLAGS_HASH) && nsig < px->prec) {
                px->prec = nsig;
            }
            px->prec -= xexp;
            if (px->prec < 0) {
                px->prec = 0;
            }
        }
        if (xexp <= 0) {
            /* The value is below 1, so zeroes follow the point before the digits. */
            px->s[px->n1++] = '0';
            if (px->prec > 0 || (px->flags & FLAGS_HASH)) {
                px->s[px->n1++] = point;
            }
            if (px->prec < -xexp) {
                xexp = -px->prec;
            }
            px->nz1 = -xexp;
            px->prec += xexp;
            if (px->prec < nsig) {
                nsig = px->prec;
            }
            memcpy(&px->s[px->n1], p, px->n2 = nsig);
            px->nz2 = px->prec - nsig;
        } else if (nsig < xexp) {
            /* The digits run out before the point, so zeroes pad the integer part. */
            memcpy(&px->s[px->n1], p, nsig);
            px->n1 += nsig;
            px->nz1 = xexp - nsig;
            if (px->prec > 0 || (px->flags & FLAGS_HASH)) {
                px->s[px->n1] = point;
                px->n2++;
            }
            px->nz2 = px->prec;
        } else {
            memcpy(&px->s[px->n1], p, xexp);
            px->n1 += xexp;
            nsig -= xexp;
            if (px->prec > 0 || (px->flags & FLAGS_HASH)) {
                px->s[px->n1++] = point;
            }
            if (px->prec < nsig) {
                nsig = px->prec;
            }
            memcpy(&px->s[px->n1], p + xexp, nsig);
            px->n1 += nsig;
            px->nz1 = px->prec - nsig;
        }
    } else {
        /* Exponential notation. */
        char *e;

        if (code == 'g' || code == 'G') {
            if (nsig < px->prec) {
                px->prec = nsig;
            }
            if (--px->prec < 0) {
                px->prec = 0;
            }
            code = (code == 'g') ? 'e' : 'E';
        }
        px->s[px->n1++] = *p++;
        if (px->prec > 0 || (px->flags & FLAGS_HASH)) {
            px->s[px->n1++] = point;
        }
        if (px->prec > 0) {
            if (px->prec < --nsig) {
                nsig = px->prec;
            }
            memcpy(&px->s[px->n1], p, nsig);
            px->n1 += nsig;
            px->nz1 = px->prec - nsig;
        }
        e = &px->s[px->n1];
        *e++ = code;
        if (xexp >= 0) {
            *e++ = '+';
        } else {
            *e++ = '-';
            xexp = -xexp;
        }
        if (xexp >= 100) {
            if (xexp >= 1000) {
                *e++ = xexp / 1000 + '0';
                xexp %= 1000;
            }
            *e++ = xexp / 100 + '0';
            xexp %= 100;
        }
        *e++ = xexp / 10 + '0';
        xexp %= 10;
        *e++ = xexp + '0';
        px->n2 = e - px->s - px->n1;
    }

    if ((px->flags & (FLAGS_ZERO | FLAGS_MINUS)) == FLAGS_ZERO) {
        int n = px->n0 + px->n1 + px->nz1 + px->n2 + px->nz2;

        if (n < px->width) {
            px->nz0 = px->width - n;
        }
    }
}

/*
 * Classify the double of px and scale its stored value into [1, 2), with its binary exponent in
 * *pex. Returns 2 for a NaN, 1 for an infinity, 0 for zero, and -1 for a finite value. The
 * original compiles this into _Ldtob.
 */
static s16 _Ldunscale(s16 *pex, _Pft *px) {
    u16 *ps = px->v.hw;
    s16 xchar = (ps[_D0] & _DMASK) >> _DOFF;

    if (xchar == _DMAX) {
        *pex = 0;
        return (ps[_D0] & _DFRAC) || ps[_D1] || ps[_D2] || ps[_D3] ? 2 : 1;
    } else if (xchar > 0) {
        /* No caller reads the scaled value again. */
        ps[_D0] = (ps[_D0] & ~_DMASK) | 0x3FF0;
        *pex = xchar - 0x3FE;
        return -1;
    } else if (xchar < 0) {
        /* The mask leaves the exponent positive, so the original never takes this branch. */
        return 2;
    } else {
        /* Subnormal values are treated as zero. */
        *pex = 0;
        return 0;
    }
}

void _Ldtob(_Pft *px, char code) {
    char buff[BUFF_LEN];
    char *p = buff;
    double ldval = px->v.ld;
    s16 err;
    s16 nsig;
    s16 xexp;

    if (px->prec < 0) {
        px->prec = 6;
    } else if (px->prec == 0 && (code == 'g' || code == 'G')) {
        px->prec = 1;
    }

    err = _Ldunscale(&xexp, px);
    if (err > 0) {
        memcpy(px->s, (err == 2) ? "NaN" : "Inf", px->n1 = 3);
        return;
    }

    if (err == 0) {
        /* The original clears the exponent again after _Ldunscale() cleared it. */
        nsig = 0;
        xexp = 0;
    } else {
        int i, n;

        if (ldval < 0) {
            ldval = -ldval;
        }
        /* Estimate the decimal exponent from the binary one, then scale towards 1e8. */
        xexp = xexp * 30103 / 100000 - 4;
        if (xexp < 0) {
            n = (-xexp + 3) & ~3;
            xexp = -n;
            for (i = 0; n > 0; n >>= 1, i++) {
                if (n & 1) {
                    ldval *= g_adblPowersOfTen[i];
                }
            }
        } else if (xexp > 0) {
            double factor = 1;

            xexp &= ~3;
            for (n = xexp, i = 0; n > 0; n >>= 1, i++) {
                if (n & 1) {
                    factor *= g_adblPowersOfTen[i];
                }
            }
            ldval /= factor;
        }

        {
            int gen = px->prec + ((code == 'f') ? 10 + xexp : 6);

            if (gen > 0x13) {
                gen = 0x13;
            }
            /* Generate the digits eight at a time, after a 0 that a carry may round into. */
            *p++ = '0';
            while (gen > 0 && ldval > 0) {
                int j;
                long lo = ldval;

                if ((gen -= 8) > 0) {
                    ldval = (ldval - lo) * 1e8;
                }
                p += 8;
                for (j = 8; lo > 0 && --j >= 0;) {
                    ldiv_t qr = ldiv(lo, 10);

                    *--p = qr.rem + '0';
                    lo = qr.quot;
                }
                while (--j >= 0) {
                    *--p = '0';
                }
                p += 8;
            }

            gen = p - &buff[1];
            for (p = &buff[1], xexp += 7; *p == '0'; p++) {
                --gen;
                --xexp;
            }

            nsig = px->prec + ((code == 'f') ? xexp + 1 : ((code == 'e' || code == 'E') ? 1 : 0));
            if (gen < nsig) {
                nsig = gen;
            }
            if (nsig > 0) {
                /* Round to nsig digits by dropping trailing nines or zeroes. */
                const char drop = (nsig < gen && p[nsig] >= '5') ? '9' : '0';
                int n2;

                for (n2 = nsig; p[--n2] == drop;) {
                    --nsig;
                }
                if (drop == '9') {
                    ++p[n2];
                }
                if (n2 < 0) {
                    --p;
                    ++nsig;
                    ++xexp;
                }
            }
        }
    }
    _Genld(px, code, p, nsig, xexp);
}
