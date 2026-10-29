#include "common/rle.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/os_libc.h>
#else
#include <string.h>
#endif

#define RLE_MIN_RUN 3
#define RLE_BLOCK_HEADER 4

s32 DecompressInterleavedRle(u8 *dst, s32 dstSize, u8 *packed, s32 channels) {
    s32 end;
    s32 code;
    s32 out = 0;
    s32 used = 0;
    u32 count;
    s32 length;
    s32 channel;
    s8 *src = (s8 *)packed;

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
                count = code + RLE_MIN_RUN;
                /* The repeated byte replaces the code it follows. */
                code = *src++;
                used++;
                for (; count != 0; count--) {
                    dst[out] = code;
                    out += channels;
                }
            }
        }
    }
    /* The size of the last channel only. */
    return out;
}
