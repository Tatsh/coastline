#include "util.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include "trackselect.h"

/* The first grid row of the sections that tracks 3 and 8 extend to row 0. */
#define TRACK_SECTION_ROW_EXTENDED 2

/* QuickSort() hands ranges of this many elements or fewer to ShortSort(). */
#define SHORT_SORT_LIMIT 8

/* The ranges QuickSort() can defer, enough for any count that fits in 32 bits. */
#define SORT_STACK_SIZE 30

void MarkTrackSections(TrackSection *section) {
    for (; section != NULL; section = section->next) {
        if (section->flags & TRACK_SECTION_VISITED) {
            continue;
        }
        if (section->flags & TRACK_SECTION_ALTERNATE) {
            continue;
        }
        section->flags |= TRACK_SECTION_VISITED;
        if (section->gridMinY == TRACK_SECTION_ROW_EXTENDED &&
            (g_dwSelectedTrackId == 3 || g_dwSelectedTrackId == 8)) {
            section->gridMinY = 0;
        }
        MarkTrackSections(section->child);
    }
}

void UnmarkTrackSections(TrackSection *section) {
    for (; section != NULL; section = section->next) {
        if (section->flags & TRACK_SECTION_VISITED) {
            section->flags &= (u16)~TRACK_SECTION_VISITED;
            UnmarkTrackSections(section->child);
        }
    }
}

void PrepareTrackSections(void) {
    MarkTrackSections(g_LoadedTrack.rootSection);
    UnmarkTrackSections(g_LoadedTrack.rootSection);
}
#endif

DisplayListBlock *AllocDisplayListBlock(void) {
    if (g_dwDisplayListBlockCount < FRAME_DISPLAY_LIST_BLOCKS) {
        return &g_aDisplayListBlocks[g_dwFrameBufferIndex][g_dwDisplayListBlockCount++];
    }
    g_dwDisplayListBlockCount++;
    return &g_aDisplayListBlocks[g_dwFrameBufferIndex][FRAME_DISPLAY_LIST_BLOCKS];
}

Hilite *AllocHiliteBlock(void) {
    if (g_dwHiliteBlockCount < FRAME_HILITE_BLOCKS) {
        return &g_aHiliteBlocks[g_dwFrameBufferIndex][g_dwHiliteBlockCount++];
    }
    g_dwHiliteBlockCount++;
    return &g_aHiliteBlocks[g_dwFrameBufferIndex][FRAME_HILITE_BLOCKS];
}

LookAt *AllocLightBlock(void) {
    if (g_dwLightBlockCount < FRAME_LIGHT_BLOCKS) {
        return &g_aLightBlocks[g_dwFrameBufferIndex][g_dwLightBlockCount++];
    }
    g_dwLightBlockCount++;
    return &g_aLightBlocks[g_dwFrameBufferIndex][FRAME_LIGHT_BLOCKS];
}

void ResetFrameBlocks(void) {
    g_dwDisplayListBlockCount = 0;
    g_dwHiliteBlockCount = 0;
    g_dwLightBlockCount = 0;
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
void QuickSort(void *base, u32 count, u32 size, CompareFunc compare) {
    u8 *lo, *hi, *mid, *loguy, *higuy;
    u32 n;
    u8 *lostk[SORT_STACK_SIZE];
    u8 *histk[SORT_STACK_SIZE];
    s32 stkptr;
    SwapFunc swap;

    /* Exchange whole words or halfwords when the size allows. The exchanges take typed buffers,
     * which the swap pointer passes as untyped ones. */
    if ((size & 3) == 0) {
        swap = (SwapFunc)ExchangeWordBuffers;
    } else if ((size & 1) == 0) {
        swap = (SwapFunc)ExchangeHalfwordBuffers;
    } else {
        swap = (SwapFunc)ExchangeByteBuffers;
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

void ExchangeByteBuffers(u8 *a, u8 *b, u32 size) {
    u8 tmp;

    if (a != b) {
        while (size--) {
            tmp = *a;
            *a++ = *b;
            *b++ = tmp;
        }
    }
}

void ExchangeHalfwordBuffers(s16 *a, s16 *b, u32 size) {
    s16 tmp;

    size >>= 1;
    if (a != b) {
        while (size--) {
            tmp = *a;
            *a++ = *b;
            *b++ = tmp;
        }
    }
}

void ExchangeWordBuffers(u32 *a, u32 *b, u32 size) {
    u32 tmp;

    size >>= 2;
    if (a != b) {
        while (size--) {
            tmp = *a;
            *a++ = *b;
            *b++ = tmp;
        }
    }
}
#endif
