#include "util.h"

#include "trackselect.h"

/* The node kind that tracks 3 and 8 replace with kind 0. */
#define MODEL_NODE_KIND_REPLACED 2

/* QuickSort() hands ranges of this many elements or fewer to ShortSort(). */
#define SHORT_SORT_LIMIT 8

/* The ranges QuickSort() can defer, enough for any count that fits in 32 bits. */
#define SORT_STACK_SIZE 30

void MarkModelNodes(ModelNode *node) {
    for (; node != NULL; node = node->next) {
        if (node->flags & MODEL_NODE_VISITED) {
            continue;
        }
        if (node->flags & MODEL_NODE_SKIP) {
            continue;
        }
        node->flags |= MODEL_NODE_VISITED;
        if (node->kind == MODEL_NODE_KIND_REPLACED &&
            (g_dwSelectedTrackId == 3 || g_dwSelectedTrackId == 8)) {
            node->kind = 0;
        }
        MarkModelNodes(node->child);
    }
}

void UnmarkModelNodes(ModelNode *node) {
    for (; node != NULL; node = node->next) {
        if (node->flags & MODEL_NODE_VISITED) {
            node->flags &= ~MODEL_NODE_VISITED;
            UnmarkModelNodes(node->child);
        }
    }
}

void PrepareTrackModelNodes(void) {
    MarkModelNodes(g_LoadedTrack.modelRoot);
    UnmarkModelNodes(g_LoadedTrack.modelRoot);
}

DisplayListBlock *AllocDisplayListBlock(void) {
    s32 count = g_dwDisplayListBlockCount;

    if (count < FRAME_DISPLAY_LIST_BLOCKS) {
        g_dwDisplayListBlockCount = count + 1;
        return &g_aDisplayListBlocks[g_dwFrameBufferIndex][count];
    }
    g_dwDisplayListBlockCount = count + 1;
    return &g_aDisplayListBlocks[g_dwFrameBufferIndex][FRAME_DISPLAY_LIST_BLOCKS];
}

Hilite *AllocHiliteBlock(void) {
    s32 count = g_dwHiliteBlockCount;

    if (count < FRAME_HILITE_BLOCKS) {
        g_dwHiliteBlockCount = count + 1;
        return &g_aHiliteBlocks[g_dwFrameBufferIndex][count];
    }
    g_dwHiliteBlockCount = count + 1;
    return &g_aHiliteBlocks[g_dwFrameBufferIndex][FRAME_HILITE_BLOCKS];
}

LookAt *AllocLightBlock(void) {
    s32 count = g_dwLightBlockCount;

    if (count < FRAME_LIGHT_BLOCKS) {
        g_dwLightBlockCount = count + 1;
        return &g_aLightBlocks[g_dwFrameBufferIndex][count];
    }
    g_dwLightBlockCount = count + 1;
    return &g_aLightBlocks[g_dwFrameBufferIndex][FRAME_LIGHT_BLOCKS];
}

void ResetFrameBlocks(void) {
    g_dwDisplayListBlockCount = 0;
    g_dwHiliteBlockCount = 0;
    g_dwLightBlockCount = 0;
}

void QuickSort(void *base, u32 count, u32 size, CompareFunc compare) {
    u8 *lo, *hi, *mid, *loguy, *higuy;
    u32 n;
    u8 *lostk[SORT_STACK_SIZE];
    u8 *histk[SORT_STACK_SIZE];
    s32 stkptr;
    SwapFunc swap;

    /* Exchange whole words or halfwords when the size allows. */
    if ((size & 3) == 0) {
        swap = ExchangeWordBuffers;
    } else if ((size & 1) == 0) {
        swap = ExchangeHalfwordBuffers;
    } else {
        swap = ExchangeByteBuffers;
    }

    if (count < 2 || size == 0) {
        return;
    }
    stkptr = 0;
    lo = base;
    hi = (u8 *)base + size * (count - 1);

    for (;;) {
        n = (u32)(hi - lo) / size + 1;
        if (n <= SHORT_SORT_LIMIT) {
            ShortSort(lo, hi, size, compare, swap);
        } else {
            /* Partition about the middle element, which moves to the front first. */
            mid = lo + (n / 2) * size;
            swap(mid, lo, size);
            loguy = lo;
            higuy = hi + size;
            for (;;) {
                do {
                    loguy += size;
                } while (loguy <= hi && compare(loguy, lo) <= 0);
                do {
                    higuy -= size;
                } while (higuy > lo && compare(higuy, lo) >= 0);
                if (higuy < loguy) {
                    break;
                }
                swap(loguy, higuy, size);
            }
            swap(lo, higuy, size);

            /* Defer the larger part and continue with the smaller one. */
            if (higuy - 1 - lo >= hi - loguy) {
                if (lo + size < higuy) {
                    lostk[stkptr] = lo;
                    histk[stkptr] = higuy - size;
                    stkptr++;
                }
                if (loguy < hi) {
                    lo = loguy;
                    continue;
                }
            } else {
                if (loguy < hi) {
                    lostk[stkptr] = loguy;
                    histk[stkptr] = hi;
                    stkptr++;
                }
                if (lo + size < higuy) {
                    hi = higuy - size;
                    continue;
                }
            }
        }

        stkptr--;
        if (stkptr < 0) {
            return;
        }
        lo = lostk[stkptr];
        hi = histk[stkptr];
    }
}

void ShortSort(u8 *lo, u8 *hi, u32 size, CompareFunc compare, SwapFunc swap) {
    u8 *p, *max;

    /* Move the largest remaining element to the end of the range each pass. */
    for (; hi > lo; hi -= size) {
        max = lo;
        for (p = lo + size; p <= hi; p += size) {
            if (compare(p, max) > 0) {
                max = p;
            }
        }
        swap(max, hi, size);
    }
}

void ExchangeByteBuffers(void *a, void *b, u32 size) {
    u8 *pa = a;
    u8 *pb = b;
    u8 tmp;

    if (pa == pb) {
        return;
    }
    while (size-- != 0) {
        tmp = *pa;
        *pa++ = *pb;
        *pb++ = tmp;
    }
}

void ExchangeHalfwordBuffers(void *a, void *b, u32 size) {
    s16 *pa = a;
    s16 *pb = b;
    s16 tmp;

    size >>= 1;
    if (pa == pb) {
        return;
    }
    while (size-- != 0) {
        tmp = *pa;
        *pa++ = *pb;
        *pb++ = tmp;
    }
}

void ExchangeWordBuffers(void *a, void *b, u32 size) {
    u32 *pa = a;
    u32 *pb = b;
    u32 tmp;

    size >>= 2;
    if (pa == pb) {
        return;
    }
    while (size-- != 0) {
        tmp = *pa;
        *pa++ = *pb;
        *pb++ = tmp;
    }
}
