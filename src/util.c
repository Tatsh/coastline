#include "util.h"

#include "debugmessage.h"
#include "texrect.h"
#include "trackselect.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/os_libc.h>
#else
#include <stdio.h>
#endif

/* The original stores the address of a local message for the debug display. */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-pointer"
#endif
void GetTextureScaleAndMask(u32 size, s32 *scale, s32 *maskBits) {
    char message[40];

    /* The size becomes the largest coordinate. */
    size--;
    if (size >> 8) {
        if (size >> 10) {
            /* The message may overrun the buffer, and the debug display retains a pointer to the
             * stack this returns from. */
            sprintf(message, "ERROR: unhandled texture size: %d", size);
            SetDebugMessage(message);
        } else if (size >> 9) {
            *maskBits = 10;
        } else {
            *maskBits = 9;
        }
    } else if (size & 0xF0) {
        if (size & 0xC0) {
            if (size & 0x80) {
                *maskBits = 8;
            } else {
                *maskBits = 7;
            }
        } else if (size & 0xE0) {
            *maskBits = 6;
        } else {
            *maskBits = 5;
        }
    } else if (size & 0xFC) {
        if (size & 0xF8) {
            *maskBits = 4;
        } else {
            *maskBits = 3;
        }
    } else if (size & 0xFE) {
        *maskBits = 2;
    } else if (size != 0) {
        *maskBits = 1;
    } else {
        *maskBits = 0;
    }
    *scale = 0xFFFF;
}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

/* The first grid row extended to row 0 on the strip mine and its mirrored version. */
#define TRACK_SECTION_ROW_EXTENDED 2

#if TGR_PLATFORM == TGR_PLATFORM_N64
/* QuickSort() hands ranges of this many elements or fewer to ShortSort(). */
#define SHORT_SORT_LIMIT 8

/* The ranges QuickSort() can defer, enough for any count that fits in 32 bits. */
#define SORT_STACK_SIZE 30
#endif

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
            (g_dwSelectedTrackId == TRACK_STRIP_MINE ||
             g_dwSelectedTrackId == TRACK_MIRROR_STRIP_MINE)) {
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
