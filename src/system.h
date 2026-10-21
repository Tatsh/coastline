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
 * Return whether a routine is the current screen.
 *
 * @ghidraAddress USA: 0x8021c6d0
 */
s32 IsCurrentScreen(ScreenCallback callback);

/**
 * Call the current screen routine forever.
 *
 * @ghidraAddress USA: 0x8021c718
 */
void GameModeMainLoop(void);

/**
 * Copy bytes from the cartridge into memory by DMA, in blocks of 4 KB.
 *
 * @ghidraAddress USA: 0x8021c748
 */
void DmaRead(void *dst, u32 romOffset, u32 size);

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

/**
 * Whether the display stays black whatever a frame would show.
 *
 * @ghidraAddress USA: 0x8028a884
 */
extern u32 g_dwKeepScreenBlanked;

/** The size of the frame in pixels. @ghidraAddress USA: 0x8028aab0 */
extern s32 g_dwScreenWidth;

/** @ghidraAddress USA: 0x8028aab4 */
extern s32 g_dwScreenHeight;

/**
 * The shift from the size of the frame to the size of the frame buffer, 1 in high resolution.
 *
 * @ghidraAddress USA: 0x8028a850
 */
extern s32 g_dwScreenScaleShift;

/** The two frame buffers. @ghidraAddress USA: 0x8031aa28 */
extern u16 *g_apFramebuffers[2];

/** The time the last frame took. @ghidraAddress USA: 0x8028aadc */
extern s32 g_dwFrameDeltaMs;

/** The time the last frame took, in seconds. @ghidraAddress USA: 0x8028aad8 */
extern f32 g_fFrameDeltaTime;

/**
 * Limit drawing to a rectangle, clipped to the frame.
 *
 * @ghidraAddress USA: 0x80219a78
 */
void SetScissorRect(s32 x, s32 y, s32 width, s32 height);

/**
 * Show a message on the debug display.
 *
 * @ghidraAddress USA: 0x8021e1f4
 */
void SetDebugMessage(const char *message);

/**
 * Add a fill of the whole frame with a colour to the display list.
 *
 * @ghidraAddress USA: 0x80217fb8
 */
void ClearScreen(s32 red, s32 green, s32 blue);

/**
 * Select the normal display mode.
 *
 * @ghidraAddress USA: 0x80219a1c
 */
void SetDisplayModeNormal(void);

/**
 * Select the wide display mode.
 *
 * @ghidraAddress USA: 0x80219a3c
 */
void SetDisplayModeWide(void);

/**
 * End the display list of the frame and run it.
 *
 * @ghidraAddress USA: 0x8021aa08
 */
void FlushDisplayList(void);

/**
 * Set the view to a rectangle of the screen, and the scissor to it as well if asked.
 *
 * @ghidraAddress USA: 0x80219bf0
 */
void SetViewport(s32 x, s32 y, s32 width, s32 height, s32 setScissor);

/**
 * Set up the projection for a screen of a size.
 *
 * @ghidraAddress USA: 0x8021b458
 */
void SetupProjectionMatrix(f32 width, f32 height);

/**
 * Emit the render modes a menu frame starts with.
 *
 * @ghidraAddress USA: 0x80217c94
 */
void SetupFillMode(void);

/**
 * Update the timing of the frames from the CPU timer.
 *
 * @ghidraAddress USA: 0x8021c4b4
 */
void UpdateFrameTiming(void);

/** @ghidraAddress USA: 0x8021c740 */
void StubUpdateCallback(void);

/** @ghidraAddress USA: 0x8021c6b0 */
void StubRenderCallback(void);

#endif
