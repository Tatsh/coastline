#include "replay.h"

#include <PR/os_libc.h>

/* A run is kept from 3 repeats, up to 132; a literal block holds up to 128 bytes. */
#define RLE_MIN_RUN 3
#define RLE_MAX_RUN 132
#define RLE_MAX_LITERAL 128

/* The size of the length a channel starts with. */
#define RLE_BLOCK_HEADER 4

s32 CompressRleChannels(u8 *dst, s32 dstSize, u8 *src, s32 srcSize, s32 channels) {
    s32 out = 0;
    s32 channel;
    s32 header;
    s32 blockLength = 0;
    s32 i;
    s32 literal;
    s32 run;
    /* Uninitialised in the original: the first comparison of the first channel is against an
     * unknown byte, and of each later channel against the byte read past the end of the last. */
    s8 current = 0;
    s8 previous;

    for (channel = 0;; channel++, src++) {
        header = out;
        if ((u32)dstSize < (u32)(out + RLE_BLOCK_HEADER)) {
            return -1;
        }
        out += RLE_BLOCK_HEADER;
        i = 0;
        literal = 0;
        run = 0;
        for (;;) {
            /* The scan starts at the second byte, and its last read is past the end. */
            previous = current;
            i += channels;
            current = src[i];
            if (current == previous && i - run <= channels * RLE_MAX_RUN &&
                i - literal <= channels * RLE_MAX_LITERAL && i < srcSize) {
                continue;
            }
            if (i - run < channels * RLE_MIN_RUN) {
                run = i;
                if (i < srcSize) {
                    continue;
                }
            } else if (literal == run) {
                goto flushRun;
            }
            /* The bytes before the run, as a negative count and the bytes. The limit of 128 is
             * checked only while bytes repeat, so a longer stretch with no run of three overflows
             * the count. */
            if (out + (run - literal) / channels >= dstSize) {
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
                if (dstSize < out + 2) {
                    return -1;
                }
                dst[out++] = ((i - run) - channels * RLE_MIN_RUN) / channels;
                dst[out++] = previous;
            }
            run = i;
            if (i >= srcSize) {
                break;
            }
        }
        blockLength = out - header - RLE_BLOCK_HEADER;
        memcpy(&dst[header], &blockLength, RLE_BLOCK_HEADER);
        if (channel + 1 >= channels) {
            return out;
        }
    }
}

s32 DecompressInterleavedRle(u8 *dst, s32 dstSize, u8 *src, s32 channels) {
    s32 out = 0;
    s32 used = 0;
    s32 channel;
    s32 length;
    s32 end;
    s32 count;
    s8 code;
    u8 value;

    for (channel = 0; channel < channels; channel++, dst++) {
        out = 0;
        memcpy(&length, src, RLE_BLOCK_HEADER);
        used += RLE_BLOCK_HEADER;
        end = used + length;
        src += RLE_BLOCK_HEADER;
        while (used < end) {
            code = *src++;
            used++;
            if (code < 0) {
                for (count = -code; count != 0; count--) {
                    dst[out] = *src++;
                    out += channels;
                    used++;
                }
            } else {
                value = *src++;
                used++;
                for (count = code + RLE_MIN_RUN; count != 0; count--) {
                    dst[out] = value;
                    out += channels;
                }
            }
        }
    }
    /* The size of the last channel only. */
    return out;
}
