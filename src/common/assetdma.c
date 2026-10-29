#include "common/assetdma.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include "common/system.h"
#else
s32 g_dwDmaReadPending;
u32 g_dwDmaAssetSizeBuffer;
u32 g_dwDmaReadWordBuffer;
#endif

#define DMA_BLOCK_SIZE 0x1000
#define ASSET_HEADER_WORD 4

void DmaRead(void *dst, u32 romOffset, s32 size) {
#if TGR_PC
    g_dwDmaReadPending = 1;
#else
    osInvalDCache(dst, size);
    while (size > DMA_BLOCK_SIZE) {
        osPiStartDma(AllocIoMessage(),
                     OS_MESG_PRI_NORMAL,
                     OS_READ,
                     romOffset,
                     dst,
                     DMA_BLOCK_SIZE,
                     &g_DmaMessageQueue);
        size -= DMA_BLOCK_SIZE;
        romOffset += DMA_BLOCK_SIZE;
        dst = (u8 *)dst + DMA_BLOCK_SIZE;
    }
    osPiStartDma(
        AllocIoMessage(), OS_MESG_PRI_NORMAL, OS_READ, romOffset, dst, size, &g_DmaMessageQueue);
    WaitForDmaMessages();
#endif /* TGR_PC */
}

u32 DmaReadAssetSize(u32 romOffset) {
    DmaRead(&g_dwDmaAssetSizeBuffer, romOffset + ASSET_HEADER_WORD, sizeof(g_dwDmaAssetSizeBuffer));
    return g_dwDmaAssetSizeBuffer;
}

u32 DmaReadWord(u32 romOffset) {
    DmaRead(&g_dwDmaReadWordBuffer, romOffset, sizeof(g_dwDmaReadWordBuffer));
    return g_dwDmaReadWordBuffer;
}

void InitDmaRequest(DecompressJob *job, u8 *buffer) {
    job->dst = NULL;
    job->buffer = buffer;
}

#if TGR_PC
u32 DecompressFromRom(void *dst, u32 romOffset, DecompressJob *job) {
    return 0;
}
#endif
