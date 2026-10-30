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

/* Elements of a size, the first byte an identity and the second the key. */
static s32 cmpKey(const void *a, const void *b) {
    return ((const u8 *)a)[1] - ((const u8 *)b)[1];
}

/* The order of the qsort() of the C runtime of the PC release on ten elements with equal keys,
 * traced by hand: the middle element is the pivot of the range of ten, the halves of three and
 * four are sorted by selection, and equal keys do not keep their order. */
static void checkEqualKeys(u32 size) {
    static const u8 keys[10] = {2, 1, 2, 0, 1, 2, 0, 1, 2, 1};
    static const u8 expected[10] = {6, 3, 1, 4, 9, 7, 2, 8, 0, 5};
    u32 words[10];
    u8 *elements = (u8 *)words;
    u32 i;

    for (i = 0; i < 10; i++) {
        elements[i * size] = (u8)i;
        elements[i * size + 1] = keys[i];
    }
    QuickSort(elements, 10, size, cmpKey);
    for (i = 0; i < 10; i++) {
        assert(elements[i * size] == expected[i]);
    }
}

int main(void) {
    static u8 b[3000];
    static s16 h[3000];
    static s32 w[3000];
    u32 n, i;

    /* Words exchanged whole, and bytes one by one. */
    checkEqualKeys(4);
    checkEqualKeys(3);
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
