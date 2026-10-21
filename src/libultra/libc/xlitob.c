#include "xstdio.h"

#define BUFF_LEN 0x18

void _Litob(_Pft *px, char code) {
    char buff[BUFF_LEN];
    const char *digs = (code == 'X') ? g_szHexDigitsUpper : g_szHexDigitsLower;
    int base = (code == 'o') ? 8 : ((code != 'x' && code != 'X') ? 10 : 16);
    int i = BUFF_LEN;
    unsigned long long ullval = px->v.ll;

    if ((code == 'd' || code == 'i') && px->v.ll < 0) {
        ullval = -ullval;
    }
    /* A zero with a precision of zero has no digits. */
    if (ullval != 0 || px->prec != 0) {
        buff[--i] = digs[ullval % base];
    }
    px->v.ll = ullval / base;
    while (px->v.ll > 0 && i > 0) {
        lldiv_t qr = lldiv(px->v.ll, base);

        px->v.ll = qr.quot;
        buff[--i] = digs[qr.rem];
    }

    px->n1 = BUFF_LEN - i;
    memcpy(px->s, buff + i, px->n1);
    if (px->n1 < px->prec) {
        px->nz0 = px->prec - px->n1;
    }
    if (px->prec < 0 && (px->flags & (FLAGS_ZERO | FLAGS_MINUS)) == FLAGS_ZERO) {
        i = px->width - px->n0 - px->nz0 - px->n1;
        if (i > 0) {
            px->nz0 += i;
        }
    }
}
