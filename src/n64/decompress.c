#include <PR/os.h>
#include <PR/os_libc.h>

#include "common/system.h"
#include "zlib.h"

/* Each half of a buffer of compressed data, and the room the decompression of one gets. */
#define CHUNK_BUFFER_SIZE 16000
#define CHUNK_OUTPUT_SIZE 16000

/* The heap lent to zlib by a decompression that runs over several calls. */
#define INFLATE_HEAP_SIZE 100000

/* The words of the header of a compressed asset: its compressed and decompressed sizes. */
#define ASSET_HEADER_WORD 4

u32 DecompressFromRom(void *dst, u32 romOffset, DecompressJob *job) {
#if defined(__GNUC__)
    static u32 len __attribute__((section(".bss.decompress_len")));
#else
    static u32 len;
#endif
    u32 compressedSize;
    u32 decompressedSize;
    u32 remaining;
    uLong outputSize;
    u32 half;
    s32 first;
    u32 chunkSize;
    u8 *buffer;
    u32 start;

    if (job != NULL && job->dst != NULL) {
        romOffset = job->romOffset;
        dst = job->dst;
        remaining = job->remaining;
        compressedSize = job->compressedSize;
        decompressedSize = job->decompressedSize;
        buffer = job->buffer;
        half = job->bufferHalf;
        len = job->pendingChunkSize;
        first = FALSE;
    } else {
        DmaRead(&len, romOffset, sizeof(len));
        compressedSize = len;
        remaining = len;
        romOffset += ASSET_HEADER_WORD;
        DmaRead(&len, romOffset, sizeof(len));
        romOffset += ASSET_HEADER_WORD;
        decompressedSize = len;
        remaining -= 2 * ASSET_HEADER_WORD;
        if (job != NULL) {
            buffer = job->buffer;
        } else {
            buffer = g_aDmaWorkBuffers;
        }
        half = 0;
        first = TRUE;
        len = 0;
    }
    start = osGetCount();

    /* While one half of the buffer is read, the chunk in the other is decompressed. */
    for (;;) {
        chunkSize = len;
        if ((u32)((s32)(len + 5) & ~1) < remaining) {
            if (romOffset & 1) {
                romOffset++;
            }
            DmaRead(&len, romOffset, sizeof(len));
            romOffset += ASSET_HEADER_WORD;
            osInvalDCache(&buffer[half], CHUNK_BUFFER_SIZE);
            osPiStartDma(AllocIoMessage(),
                         OS_MESG_PRI_NORMAL,
                         OS_READ,
                         romOffset,
                         &buffer[half],
                         (len + 1) & ~1,
                         &g_DmaMessageQueue);
            romOffset += (s32)(len + 1) & ~1;
        } else {
            WaitForDmaMessages();
        }
        osInvalDCache(&buffer[half], CHUNK_BUFFER_SIZE);
        half ^= CHUNK_BUFFER_SIZE;
        if (first) {
            first = FALSE;
        } else {
            outputSize = CHUNK_OUTPUT_SIZE;
            if (job != NULL) {
                g_pInflateHeapBase = g_aInflateHeapBuffer;
                g_pInflateHeapEnd = (u8 *)(INFLATE_HEAP_SIZE + (char *)g_pInflateHeapBase);
            }
            uncompress(dst, &outputSize, &buffer[half], chunkSize);
            if (job != NULL) {
                g_pInflateHeapBase = g_pInflateHeapEnd = NULL;
            }
            dst = (u8 *)dst + outputSize;
            remaining -= (chunkSize + 5) & ~1;
            if (remaining == 0) {
                break;
            }
        }
        /* A decompression over several calls takes one step a call. */
        if (job != NULL) {
            break;
        }
    }
    /* The time it took, in microseconds, is worked out and not kept. */
    start = osGetCount() - start;
    (void)((u64)start * 1000000 / osClockRate);

    if (job != NULL) {
        job->romOffset = romOffset;
        job->dst = dst;
        job->remaining = remaining;
        job->compressedSize = compressedSize;
        job->decompressedSize = decompressedSize;
        job->bufferHalf = half;
        job->buffer = buffer;
        job->pendingChunkSize = len;
    }
    return decompressedSize;
}
