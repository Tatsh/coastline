#include "xstdio.h"

/* Read a decimal field into i, saturating at 999. */
#define ATOI(i, a)                                                                                 \
    for (i = 0; *a >= '0' && *a <= '9'; a++) {                                                     \
        if (i < 999) {                                                                             \
            i = *a + i * 10 - '0';                                                                 \
        }                                                                                          \
    }

/* Pass a run of output to the routine, and stop formatting when it fails. */
#define PROUT(s, size)                                                                             \
    if ((size) > 0) {                                                                              \
        arg = (void *)prout(arg, s, size);                                                         \
        if (arg != NULL) {                                                                         \
            x.nchar += (size);                                                                     \
        } else {                                                                                   \
            return x.nchar;                                                                        \
        }                                                                                          \
    }

/* Pass m padding characters from src in pieces no longer than src. */
#define PAD(m, src, extracond)                                                                     \
    if ((extracond) && (m) > 0) {                                                                  \
        int i_, j_;                                                                                \
        for (j_ = (m); j_ > 0; j_ -= i_) {                                                         \
            i_ = ((unsigned)j_ > sizeof(src) - 1) ? (int)sizeof(src) - 1 : j_;                     \
            PROUT(src, i_);                                                                        \
        }                                                                                          \
    }

/** @ghidraAddress USA: 0x80268750 */
static void _Putfld(_Pft *px, va_list *pap, char code, char *ac) {
    px->n0 = px->nz0 = px->n1 = px->nz1 = px->n2 = px->nz2 = 0;

    switch (code) {
    case 'c':
        ac[px->n0++] = va_arg(*pap, int);
        break;

    case 'd':
    case 'i':
        if (px->qual == 'l') {
            px->v.ll = va_arg(*pap, int);
        } else if (px->qual == 'L') {
            px->v.ll = va_arg(*pap, long long);
        } else {
            px->v.ll = va_arg(*pap, int);
        }
        if (px->qual == 'h') {
            px->v.ll = (s16)px->v.ll;
        }
        if (px->v.ll < 0) {
            ac[px->n0++] = '-';
        } else if (px->flags & FLAGS_PLUS) {
            ac[px->n0++] = '+';
        } else if (px->flags & FLAGS_SPACE) {
            ac[px->n0++] = ' ';
        }
        px->s = &ac[px->n0];
        _Litob(px, code);
        break;

    case 'x':
    case 'X':
    case 'u':
    case 'o':
        if (px->qual == 'l') {
            px->v.ll = va_arg(*pap, int);
        } else if (px->qual == 'L') {
            px->v.ll = va_arg(*pap, long long);
        } else {
            px->v.ll = va_arg(*pap, int);
        }
        if (px->qual == 'h') {
            px->v.ll = (u16)px->v.ll;
        } else if (px->qual == '\0') {
            px->v.ll = (unsigned int)px->v.ll;
        }
        if (px->flags & FLAGS_HASH) {
            ac[px->n0++] = '0';
            if (code == 'x' || code == 'X') {
                ac[px->n0++] = code;
            }
        }
        px->s = &ac[px->n0];
        _Litob(px, code);
        break;

    case 'e':
    case 'f':
    case 'g':
    case 'E':
    case 'G':
        /* A long double is a double on this target. */
        px->v.ld = (px->qual == 'L') ? va_arg(*pap, long double) : va_arg(*pap, double);
        if (px->v.hw[_D0] & 0x8000) {
            ac[px->n0++] = '-';
        } else if (px->flags & FLAGS_PLUS) {
            ac[px->n0++] = '+';
        } else if (px->flags & FLAGS_SPACE) {
            ac[px->n0++] = ' ';
        }
        px->s = &ac[px->n0];
        _Ldtob(px, code);
        break;

    case 'n':
        if (px->qual == 'h') {
            *va_arg(*pap, u16 *) = px->nchar;
        } else if (px->qual == 'l') {
            *va_arg(*pap, unsigned int *) = px->nchar;
        } else if (px->qual == 'L') {
            *va_arg(*pap, u64 *) = px->nchar;
        } else {
            *va_arg(*pap, unsigned int *) = px->nchar;
        }
        break;

    case 'p':
        px->v.ll = (long)va_arg(*pap, void *);
        px->s = &ac[px->n0];
        _Litob(px, 'x');
        break;

    case 's':
        px->s = va_arg(*pap, char *);
        px->n1 = strlen(px->s);
        if (px->prec >= 0 && px->n1 > px->prec) {
            px->n1 = px->prec;
        }
        break;

    case '%':
        ac[px->n0++] = '%';
        break;

    default:
        ac[px->n0++] = code;
        break;
    }
}

#ifdef __GNUC__
int _Printf(outfun prout, char *arg, const char *fmt, va_list ap) {
#else
int _Printf(outfun prout, char *arg, const char *fmt, va_list args) {
#endif
    _Pft x;
#ifdef __GNUC__
    va_list args;

    /* _Putfld advances the arguments through a pointer, and a pointer to a local copy is
     * portable where a pointer to a parameter is not. */
    va_copy(args, ap);
#endif

    x.nchar = 0;
    for (;;) {
        const char *s = fmt;
        const char *t;
        char c;
        char ac[32];

        /* Copy the text up to the next conversion. */
        for (c = *s; c != '\0' && c != '%';) {
            c = *++s;
        }
        PROUT(fmt, s - fmt);
        if (c == '\0') {
            return x.nchar;
        }
        fmt = ++s;

        x.flags = 0;
        for (; (t = strchr(fchar, *s)) != NULL; s++) {
            x.flags |= fbit[t - fchar];
        }

        if (*s == '*') {
            x.width = va_arg(args, int);
            if (x.width < 0) {
                x.width = -x.width;
                x.flags |= FLAGS_MINUS;
            }
            s++;
        } else {
            ATOI(x.width, s);
        }

        if (*s != '.') {
            x.prec = -1;
        } else {
            s++;
            if (*s == '*') {
                x.prec = va_arg(args, int);
                s++;
            } else {
                ATOI(x.prec, s);
            }
        }

        x.qual = (strchr("hlL", *s) != NULL) ? *s++ : '\0';
        if (x.qual == 'l' && *s == 'l') {
            x.qual = 'L';
            s++;
        }

        _Putfld(&x, &args, *s, ac);
        x.width -= x.n0 + x.nz0 + x.n1 + x.nz1 + x.n2 + x.nz2;
        PAD(x.width, spaces, !(x.flags & FLAGS_MINUS));
        PROUT(ac, x.n0);
        PAD(x.nz0, zeroes, 1);
        PROUT(x.s, x.n1);
        PAD(x.nz1, zeroes, 1);
        PROUT(&x.s[x.n1], x.n2);
        PAD(x.nz2, zeroes, 1);
        PAD(x.width, spaces, x.flags & FLAGS_MINUS);
        fmt = s + 1;
    }
}
