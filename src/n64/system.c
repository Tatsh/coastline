#include "common/system.h"

#include <PR/R4300.h>
#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_cont.h>
#include <PR/os_libc.h>

#include "common/audio.h"
#include "common/camera.h"
#include "common/debugtext.h"
#include "common/fade.h"
#include "common/game.h"
#include "common/input.h"
#include "common/libultra/os/osint.h"
#include "common/model.h"
#include "common/race.h"
#include "common/render.h"
#include "common/rumble.h"
#include "common/texrect.h"
#include "common/text.h"
#include "common/timing.h"
#include "common/track.h"
#include "common/trackmenuitem.h"
#include "common/trackrender.h"
#include "common/trackselect.h"
#include "common/util.h"
#include "common/vecmath.h"
#include "common/vehicle.h"
#include "common/weather.h"
#include "n64/boot.h"
#include "zlib.h"

/* Each half of a buffer of compressed data, and the room the decompression of one gets. */
#define CHUNK_BUFFER_SIZE 16000
#define CHUNK_OUTPUT_SIZE 16000

/* The heap lent to zlib by a decompression that runs over several calls. */
#define INFLATE_HEAP_SIZE 100000

/* The words of the header of a compressed asset: its compressed and decompressed sizes. */
#define ASSET_HEADER_WORD 4

/* The offsets a file holds are below this; the pointers they become are above it. */
#define MODEL_OFFSET_LIMIT ((void *)0x7FFFFFFF)
#define MODEL_BASE ((void *)base)

/* The size of a chunk on the cartridge: its length word, then its bytes, padded to an even count.
 */
#define CHUNK_SPAN(size) (((size) + 5) & ~1)

u32 DecompressFromRom(void *dst, u32 romOffset, DecompressJob *job) {
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
        g_dwDmaChunkSize = job->pendingChunkSize;
        first = FALSE;
    } else {
        DmaRead(&g_dwDmaChunkSize, romOffset, sizeof(g_dwDmaChunkSize));
        compressedSize = g_dwDmaChunkSize;
        remaining = g_dwDmaChunkSize;
        romOffset += ASSET_HEADER_WORD;
        DmaRead(&g_dwDmaChunkSize, romOffset, sizeof(g_dwDmaChunkSize));
        romOffset += ASSET_HEADER_WORD;
        decompressedSize = g_dwDmaChunkSize;
        remaining -= 2 * ASSET_HEADER_WORD;
        if (job != NULL) {
            buffer = job->buffer;
        } else {
            buffer = g_aDmaWorkBuffers;
        }
        half = 0;
        first = TRUE;
        g_dwDmaChunkSize = 0;
    }
    start = osGetCount();

    /* While one half of the buffer is read, the chunk in the other is decompressed. */
    for (;;) {
        chunkSize = g_dwDmaChunkSize;
        if ((u32)((s32)(g_dwDmaChunkSize + 5) & ~1) < remaining) {
            if (romOffset & 1) {
                romOffset++;
            }
            DmaRead(&g_dwDmaChunkSize, romOffset, sizeof(g_dwDmaChunkSize));
            romOffset += ASSET_HEADER_WORD;
            osInvalDCache(&buffer[half], CHUNK_BUFFER_SIZE);
            osPiStartDma(AllocIoMessage(),
                         OS_MESG_PRI_NORMAL,
                         OS_READ,
                         romOffset,
                         &buffer[half],
                         (g_dwDmaChunkSize + 1) & ~1,
                         &g_DmaMessageQueue);
            romOffset += (g_dwDmaChunkSize + 1) & ~1;
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
                g_pInflateHeapEnd = &g_aInflateHeapBuffer[INFLATE_HEAP_SIZE];
            }
            uncompress(dst, &outputSize, &buffer[half], chunkSize);
            if (job != NULL) {
                g_pInflateHeapBase = g_pInflateHeapEnd = NULL;
            }
            remaining -= CHUNK_SPAN(chunkSize);
            dst = (u8 *)dst + outputSize;
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
        job->remaining = remaining;
        job->dst = dst;
        job->compressedSize = compressedSize;
        job->bufferHalf = half;
        job->decompressedSize = decompressedSize;
        job->buffer = buffer;
        job->pendingChunkSize = g_dwDmaChunkSize;
    }
    return decompressedSize;
}

s32 IsVehicleModelValid(s32 car) {
    return g_aCarModels[car].modelRomEnd != 0;
}

void RelocateModelPointers(Model *model) {
    u32 base = (u32)model;
    s32 i;
    s32 k;

    /* The fields of the file hold offsets from its start until they are remapped. */
    if (model->animations != NULL) {
        RemapPointer((u32 *)&model->animations, 0, MODEL_OFFSET_LIMIT, MODEL_BASE);
        for (i = 0; i < model->animations->count; i++) {
            RemapPointer((u32 *)&model->animations->entries[i], 0, MODEL_OFFSET_LIMIT, MODEL_BASE);
            RemapPointer(
                (u32 *)&model->animations->entries[i]->vertices, 0, MODEL_OFFSET_LIMIT, MODEL_BASE);
            RemapPointer((u32 *)&model->animations->entries[i]->unreconstructed8,
                         0,
                         MODEL_OFFSET_LIMIT,
                         MODEL_BASE);
            for (k = 0; k < model->animations->entries[i]->keyCount; k++) {
                RemapPointer((u32 *)&model->animations->entries[i]->keys[k],
                             0,
                             MODEL_OFFSET_LIMIT,
                             MODEL_BASE);
            }
        }
    }
    for (i = 0; (u32)i < model->partCount; i++) {
        RemapPointer((u32 *)&model->parts[i].displayList, 0, MODEL_OFFSET_LIMIT, MODEL_BASE);
        /* An offset of 0 has become the model itself, and is walked as a display list. */
        if (model->parts[i].displayList != NULL) {
            RemapPointer((u32 *)&model->parts[i].displayList, 0, MODEL_OFFSET_LIMIT, MODEL_BASE);
            FixupDisplayListPointers(
                model->parts[i].displayList, 0, MODEL_OFFSET_LIMIT, MODEL_BASE);
        }
    }
}

Model *DmaReadAndFixupTrackModel(void *dst, u32 romStart, u32 romEnd) {
    DmaRead(dst, romStart, romEnd - romStart);
    RelocateModelPointers(dst);
    return dst;
}
