#include <PR/gu.h>

/* The fixed-point form has 16 fractional bits. */
#define FTOFIX32(x) (s32)((x) * (float)0x00010000)
#define FIX32TOF(x) ((float)(x) / (float)0x00010000)

void guMtxF2L(float mf[4][4], Mtx *m) {
    int i, j;
    s32 e1, e2;
    /* The integer halves fill the first two rows of the matrix and the fractions the last two. */
    s32 *ai = &m->m[0][0];
    s32 *af = &m->m[2][0];

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 2; j++) {
            e1 = FTOFIX32(mf[i][j * 2]);
            e2 = FTOFIX32(mf[i][j * 2 + 1]);
            *(ai++) = (e1 & 0xFFFF0000) | ((e2 >> 16) & 0xFFFF);
            *(af++) = ((e1 << 16) & 0xFFFF0000) | (e2 & 0xFFFF);
        }
    }
}

void guMtxL2F(float mf[4][4], Mtx *m) {
    int i, j;
    u32 e1, e2;
    s32 q1, q2;
    u32 *ai = (u32 *)&m->m[0][0];
    u32 *af = (u32 *)&m->m[2][0];

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 2; j++) {
            e1 = (*ai & 0xFFFF0000) | ((*af >> 16) & 0xFFFF);
            e2 = ((*(ai++) << 16) & 0xFFFF0000) | (*(af++) & 0xFFFF);
            /* The original reads the halves back through their addresses, from the stack. */
            q1 = *(s32 *)&e1;
            q2 = *(s32 *)&e2;
            mf[i][j * 2] = FIX32TOF(q1);
            mf[i][j * 2 + 1] = FIX32TOF(q2);
        }
    }
}

void guMtxIdentF(float mf[4][4]) {
    int i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            mf[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}

void guMtxIdent(Mtx *m) {
    float mf[4][4];

    guMtxIdentF(mf);
    guMtxF2L(mf, m);
}
