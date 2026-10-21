#ifndef COASTLINE_SYSTEM_H
#define COASTLINE_SYSTEM_H

/**
 * @file
 * The display, DMA and decompression state of the system file of the N64 releases.
 */

#include <PR/ultratypes.h>

/** The bump allocator zcalloc() draws from. */
typedef struct {
    u8 *cur;
    u8 *end;
} InflateAllocator;

/**
 * The first of the two frame buffers, of 320 by 240 16-bit pixels. The link script places it.
 *
 * @ghidraAddress USA: 0x801b5000
 */
extern u16 g_aFramebuffer0[];

/** @ghidraAddress USA: 0x801da800 */
extern u16 g_aFramebuffer1[];

/**
 * The heap DecompressFromRom() lends to zlib, or NULL to let inflateInit2_() borrow the frame
 * buffers.
 *
 * @ghidraAddress USA: 0x80368ac0
 */
extern u8 *g_pInflateHeapBase;

/** @ghidraAddress USA: 0x80368ac4 */
extern u8 *g_pInflateHeapEnd;

/**
 * The state of the allocator inflateInit2_() gives zlib.
 *
 * @ghidraAddress USA: 0x80368ac8
 */
extern InflateAllocator g_inflateAllocator;

/**
 * The frames to wait before the display returns after inflateInit2_() blanks it to use the frame
 * buffers as a heap.
 *
 * @ghidraAddress USA: 0x8028a888
 */
extern u32 g_dwScreenUnblankDelay;

/** A routine the main loop calls once per frame. */
typedef void (*ScreenCallback)(void);

/**
 * Set up the display, the RSP and RDP state, and the system threads.
 *
 * @ghidraAddress USA: 0x8021be88
 */
void InitRenderingSubsystem(void);

/**
 * Set the routine the main loop calls each frame.
 *
 * @ghidraAddress USA: 0x8021c6e4
 */
void SetCurrentScreenCallback(ScreenCallback callback);

/**
 * Call the current screen routine forever.
 *
 * @ghidraAddress USA: 0x8021c718
 */
void GameModeMainLoop(void);

/**
 * Read the decompressed size from the header of the asset at a cartridge offset.
 *
 * @ghidraAddress USA: 0x8021c814
 */
u32 DmaReadAssetSize(u32 romOffset);

/**
 * Decompress the asset at a cartridge offset into RAM.
 *
 * @param stats Receives eight words of timing figures, unless it is NULL.
 * @ghidraAddress USA: 0x8021cd30
 */
void DecompressFromRom(void *dst, u32 romOffset, void *stats);

/**
 * Patch the render modes of the track and vehicle display lists for the current weather.
 *
 * @ghidraAddress USA: 0x8021b97c
 */
void ProcessAllDisplayLists(void);

/**
 * Probe the controllers and the Controller and Rumble Paks.
 *
 * @ghidraAddress USA: 0x8021c188
 */
void InitControllersAndPaks(void);

/** @ghidraAddress USA: 0x8021c740 */
void StubUpdateCallback(void);

/** @ghidraAddress USA: 0x8021c6b0 */
void StubRenderCallback(void);

#endif
