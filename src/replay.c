#include "rle.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/os_libc.h>
#else
#include <string.h>
#endif

/* A run is kept from 3 repeats, up to 132; a literal block holds up to 128 bytes. */
#define RLE_MIN_RUN 3
#define RLE_MAX_RUN 132
#define RLE_MAX_LITERAL 128

/* The size of the length a channel starts with. */
#define RLE_BLOCK_HEADER 4

/* On the console the first comparison of the first channel is against an uninitialised byte, and
 * of each later channel against the byte read past the end of the last. */
s32 CompressRleChannels(u8 *dst, s32 dstSize, u8 *src, s32 srcSize, s32 channels) {
    s32 out = 0;
    s32 channel = 0;
    s32 header;
    s32 blockLength = 0;
    s32 i;
    s32 literal;
    s32 run;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    s32 current;
    s32 previous;
#else
    s8 current;
    s8 previous;
#endif

    do {
        header = out;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        if (dstSize < (s32)((u32)out + RLE_BLOCK_HEADER)) {
#else
        if ((u32)dstSize < (u32)(out + RLE_BLOCK_HEADER)) {
#endif
            return -1;
        }
        out += RLE_BLOCK_HEADER;
        i = 0;
        literal = 0;
        run = 0;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        previous = -256;
#else
#ifdef ENABLE_PATCHES
        /* The first comparison is with the first byte of the channel. */
        current = src[i];
#endif /* ENABLE_PATCHES */
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
        for (;;) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            current = (s8)src[i];
#else
            /* The scan starts at the second byte, and its last read is past the end. */
            previous = current;
            i += channels;
            current = src[i];
#endif
            if (current == previous && i - run <= channels * RLE_MAX_RUN &&
                i - literal <= channels * RLE_MAX_LITERAL && i < srcSize) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
                goto advanceByte;
#else
                continue;
#endif
            }
            if (i - run < channels * RLE_MIN_RUN) {
                run = i;
                if (i < srcSize) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
                    goto advanceByte;
#else
                    continue;
#endif
                }
            } else if (literal == run) {
                goto flushRun;
            }
            /* The bytes before the run, as a negative count and the bytes. The limit of 128 is
             * checked only while bytes repeat, so a longer stretch with no run of three overflows
             * the count. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            if ((s32)((u32)out + (u32)((run - literal) / channels) + 1u) > dstSize) {
#else
            if (out + (run - literal) / channels >= dstSize) {
#endif
                return -1;
            }
            dst[out++] = -(run - literal) / channels;
            for (; literal < run; literal += channels) {
                dst[out++] = src[literal];
            }
        flushRun:
            literal = i;
            if (run != i) {
                /* The run, as its count less 3 and the byte. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
                if (dstSize < (s32)((u32)out + 2u)) {
#else
                if (dstSize < out + 2) {
#endif
                    return -1;
                }
                dst[out++] = ((i - run) - channels * RLE_MIN_RUN) / channels;
                dst[out++] = previous;
            }
            run = i;
            if (i >= srcSize) {
                break;
            }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        advanceByte:
            previous = current;
            i += channels;
#endif
        }
        blockLength = out - header - RLE_BLOCK_HEADER;
        memcpy(&dst[header], &blockLength, RLE_BLOCK_HEADER);
        src++;
        channel++;
    } while (channel < channels);
    return out;
}
