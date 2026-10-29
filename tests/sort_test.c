/* Host check of QuickSort() in src/util.c against random input of each element size. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/util.h"

DisplayListBlock g_aDisplayListBlocks[2][FRAME_DISPLAY_LIST_BLOCKS + 1];
Hilite g_aHiliteBlocks[2][FRAME_HILITE_BLOCKS + 1];
LookAt g_aLightBlocks[2][FRAME_LIGHT_BLOCKS + 1];
s32 g_dwDisplayListBlockCount, g_dwHiliteBlockCount, g_dwLightBlockCount, g_dwFrameBufferIndex;
u32 g_dwSelectedTrackId;
TrackHeader g_LoadedTrack;

static s32 cmp8(const void *a, const void *b) {
    return *(const u8 *)a - *(const u8 *)b;
}
static s32 cmp16(const void *a, const void *b) {
    return *(const s16 *)a - *(const s16 *)b;
}
static s32 cmp32(const void *a, const void *b) {
    s32 x = *(const s32 *)a, y = *(const s32 *)b;
    return (x > y) - (x < y);
}

int main(void) {
    static u8 b[3000];
    static s16 h[3000];
    static s32 w[3000];
    u32 n, i;

    srand(1);
    for (n = 0; n < 3000; n = n * 3 + 1) {
        for (i = 0; i < n; i++) {
            b[i] = rand();
            h[i] = rand();
            w[i] = rand() - RAND_MAX / 2;
        }
        QuickSort(b, n, 1, cmp8);
        QuickSort(h, n, 2, cmp16);
        QuickSort(w, n, 4, cmp32);
        for (i = 1; i < n; i++) {
            assert(b[i - 1] <= b[i]);
            assert(h[i - 1] <= h[i]);
            assert(w[i - 1] <= w[i]);
        }
    }
    puts("sort ok");
    return 0;
}
