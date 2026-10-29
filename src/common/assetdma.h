#ifndef COASTLINE_ASSET_DMA_H
#define COASTLINE_ASSET_DMA_H

#include "common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Copy bytes from the cartridge into memory by DMA, in blocks of 4 KB.
 *
 * @param dst The memory to copy into.
 * @param romOffset The cartridge offset to copy from.
 * @param size The number of bytes to copy.
 * @ghidraAddress USA: 0x8021c748
 * @ghidraAddress EUR: 0x8021d948
 * @ghidraAddress JPN: 0x8021d218
 * @ghidraAddress BRD3D: 0x100378a0
 * @ghidraAddress BRGlide: 0x10030f40
 */
void DmaRead(void *dst, u32 romOffset, s32 size);

/**
 * Read a word from the cartridge.
 *
 * @param romOffset The cartridge offset of the word.
 * @return The word.
 * @ghidraAddress USA: 0x8021c848
 * @ghidraAddress EUR: 0x8021da48
 * @ghidraAddress JPN: 0x8021d318
 * @ghidraAddress BRD3D: 0x10034ca8
 * @ghidraAddress BRGlide: 0x1002e359
 */
u32 DmaReadWord(u32 romOffset);

/**
 * Read the decompressed size from the header of the asset at a cartridge offset.
 *
 * @param romOffset The cartridge offset of the asset.
 * @return The size of the asset once decompressed, in bytes.
 * @ghidraAddress USA: 0x8021c814
 * @ghidraAddress EUR: 0x8021da14
 * @ghidraAddress JPN: 0x8021d2e4
 * @ghidraAddress BRD3D: 0x10034c88
 * @ghidraAddress BRGlide: 0x1002e339
 */
u32 DmaReadAssetSize(u32 romOffset);

/** A decompression that runs over several calls of DecompressFromRom(), a step a call. */
typedef struct {
    u32 romOffset;  /*!< Where the next chunk is on the cartridge. */
    void *dst;      /*!< Where the decompression writes next, or NULL before it starts. */
    u8 *buffer;     /*!< The memory the compressed data is read into, two halves of 16000 bytes. */
    u32 bufferHalf; /*!< The half being read into, 0 or 16000. */
    u32 compressedSize;   /*!< The compressed size of the asset, in bytes. */
    u32 decompressedSize; /*!< The decompressed size of the asset, in bytes. */
    u32 remaining; /*!< The compressed bytes remaining to read, 0 once the decompression is done. */
    u32 pendingChunkSize; /*!< The size of the chunk being read, in bytes. */
} DecompressJob;

/** The size of each of g_aDmaWorkBuffers. */
#define DMA_WORK_BUFFER_SIZE 0x8000

/**
 * The buffers that decompressions read their compressed data into.
 *
 * @ghidraAddress USA: 0x802f7f00
 * @ghidraAddress EUR: 0x802f9bf0
 * @ghidraAddress JPN: 0x802f8630
 */
extern u8 g_aDmaWorkBuffers[];

/**
 * Start a decompression with a buffer for its compressed data.
 *
 * @param job The decompression to start.
 * @param buffer The memory to read the compressed data into.
 * @ghidraAddress USA: 0x8021cd24
 * @ghidraAddress EUR: 0x8021df24
 * @ghidraAddress JPN: 0x8021d7f4
 * @ghidraAddress BRD3D: 0x10035041
 * @ghidraAddress BRGlide: 0x1002e6f2
 */
void InitDmaRequest(DecompressJob *job, u8 *buffer);

/**
 * Decompress the asset at a cartridge offset into RAM.
 *
 * @param dst The memory to decompress into.
 * @param romOffset The cartridge offset of the asset.
 * @param job The decompression to continue, which DecompressFromRom() leaves where it stopped, or
 * NULL to decompress the asset whole.
 * @return The size of the asset once decompressed, or zero on Windows.
 * @ghidraAddress USA: 0x8021cd30
 * @ghidraAddress EUR: 0x8021df30
 * @ghidraAddress JPN: 0x8021d800
 * @ghidraAddress BRD3D: 0x10035059
 * @ghidraAddress BRGlide: 0x1002e70a
 */
u32 DecompressFromRom(void *dst, u32 romOffset, DecompressJob *job);

/**
 * The words the DMA reads sizes and chunk lengths into.
 *
 * @ghidraAddress USA: 0x8031b328
 * @ghidraAddress EUR: 0x8031d838
 * @ghidraAddress JPN: 0x8031c468
 * @ghidraAddress BRD3D: 0x106c0938
 * @ghidraAddress BRGlide: 0x106e79c8
 */
extern u32 g_dwDmaAssetSizeBuffer;

/**
 * @ghidraAddress USA: 0x8031b330
 * @ghidraAddress EUR: 0x8031d840
 * @ghidraAddress JPN: 0x8031c470
 * @ghidraAddress BRD3D: 0x106c3300
 * @ghidraAddress BRGlide: 0x106ea390
 */
extern u32 g_dwDmaReadWordBuffer;

#if TGR_BOSS_RALLY
/**
 * @ghidraAddress BRD3D: 0x106c7c44
 * @ghidraAddress BRGlide: 0x106eecd4
 */
extern s32 g_dwDmaReadPending;
#endif

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_ASSET_DMA_H */
