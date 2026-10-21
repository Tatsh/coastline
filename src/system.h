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
 * @param callback The routine to call each frame.
 * @ghidraAddress USA: 0x8021c6e4
 */
void SetCurrentScreenCallback(ScreenCallback callback);

/**
 * Set a routine FlushDisplayList() calls once, when the RDP has finished the frame before, so that
 * it can change data that frame drew from.
 *
 * @param callback The routine to call once.
 * @ghidraAddress USA: 0x8021c6b8
 */
void SetNextScreenCallback(ScreenCallback callback);

/**
 * Return whether a routine is the current screen.
 *
 * @param callback The routine to compare with the current screen.
 * @return 1 if the routine is the current screen, 0 otherwise.
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
 * @param dst The memory to copy into.
 * @param romOffset The cartridge offset to copy from.
 * @param size The number of bytes to copy.
 * @ghidraAddress USA: 0x8021c748
 */
void DmaRead(void *dst, u32 romOffset, u32 size);

/**
 * Read the decompressed size from the header of the asset at a cartridge offset.
 *
 * @param romOffset The cartridge offset of the asset.
 * @return The size of the asset once decompressed, in bytes.
 * @ghidraAddress USA: 0x8021c814
 */
u32 DmaReadAssetSize(u32 romOffset);

/**
 * A decompression that runs over several calls of DecompressFromRom(). Its fields join this
 * structure as they are reconstructed.
 */
typedef struct {
    u32 romOffset;
    /** Where the decompression writes next, or NULL before it starts. */
    void *dst;
    /** The memory the compressed data is read into. */
    u8 *buffer;
    u32 unreconstructedC;
    u32 unreconstructed10;
    u32 unreconstructed14;
    /** The compressed bytes left to read, 0 once the decompression is done. */
    u32 remaining;
    u32 unreconstructed1C;
} DecompressJob;

/** The size of each of g_aDmaWorkBuffers. */
#define DMA_WORK_BUFFER_SIZE 0x8000

/**
 * The buffers that decompressions read their compressed data into.
 *
 * @ghidraAddress USA: 0x802f7f00
 */
extern u8 g_aDmaWorkBuffers[];

/**
 * Start a decompression with a buffer for its compressed data.
 *
 * @param job The decompression to start.
 * @param buffer The memory to read the compressed data into.
 * @ghidraAddress USA: 0x8021cd24
 */
void InitDmaRequest(DecompressJob *job, u8 *buffer);

/**
 * Decompress the asset at a cartridge offset into RAM.
 *
 * @param dst The memory to decompress into.
 * @param romOffset The cartridge offset of the asset.
 * @param job The decompression to continue, which DecompressFromRom() leaves where it stopped, or
 * NULL to decompress the asset whole.
 * @ghidraAddress USA: 0x8021cd30
 */
void DecompressFromRom(void *dst, u32 romOffset, DecompressJob *job);

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

/** The frames a screen has shown, for screens that count them. @ghidraAddress USA: 0x8028dbb0 */
extern u16 g_wScreenFrameCount;

/**
 * The shift from the size of the frame to the size of the frame buffer, 1 in high resolution.
 *
 * @ghidraAddress USA: 0x8028a850
 */
extern s32 g_dwScreenScaleShift;

/**
 * The color dither the screens draw with, as the other-mode bits gDPSetColorDither() takes.
 *
 * @ghidraAddress USA: 0x8028a8a0
 */
extern u32 g_dwColorDitherMode;

/**
 * The alpha dither the screens draw with, as the other-mode bits gDPSetAlphaDither() takes.
 *
 * @ghidraAddress USA: 0x8028a89c
 */
extern u32 g_dwAlphaDitherMode;

/** The two frame buffers. @ghidraAddress USA: 0x8031aa28 */
extern u16 *g_apFramebuffers[2];

/** The time the last frame took. @ghidraAddress USA: 0x8028aadc */
extern s32 g_dwFrameDeltaMs;

/** The time the last frame took, in seconds. @ghidraAddress USA: 0x8028aad8 */
extern f32 g_fFrameDeltaTime;

/** The time UpdateFrameTiming() counts, in milliseconds. @ghidraAddress USA: 0x8028aad4 */
extern s32 g_dwElapsedTimeMs;

/**
 * Limit drawing to a rectangle, clipped to the frame.
 *
 * @param x The left edge, in pixels.
 * @param y The top edge, in pixels.
 * @param width The width, in pixels.
 * @param height The height, in pixels.
 * @ghidraAddress USA: 0x80219a78
 */
void SetScissorRect(s32 x, s32 y, s32 width, s32 height);

/**
 * Show a message on the debug display.
 *
 * @param message The message to show.
 * @ghidraAddress USA: 0x8021e1f4
 */
void SetDebugMessage(const char *message);

/**
 * Add a fill of the whole frame with a colour to the display list.
 *
 * @param red The red component, from 0 to 255.
 * @param green The green component, from 0 to 255.
 * @param blue The blue component, from 0 to 255.
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
 * @param x The left edge, in pixels.
 * @param y The top edge, in pixels.
 * @param width The width, in pixels.
 * @param height The height, in pixels.
 * @param setScissor Non-zero to set the scissor to the rectangle as well.
 * @ghidraAddress USA: 0x80219bf0
 */
void SetViewport(s32 x, s32 y, s32 width, s32 height, s32 setScissor);

/**
 * Set up the projection for a screen of a size.
 *
 * @param width The width of the screen.
 * @param height The height of the screen.
 * @ghidraAddress USA: 0x8021b458
 */
void SetupProjectionMatrix(f32 width, f32 height);

/**
 * Set up the projection of a menu that draws models, looking at the middle of the screen.
 *
 * @param width The width of the screen.
 * @param height The height of the screen.
 * @ghidraAddress USA: 0x8021b5a4
 */
void SetupMenuProjection(f32 width, f32 height);

/**
 * Whether SetupMenuProjection() keeps the near scale and far distance set before it once,
 * instead of its defaults. It clears this.
 *
 * @ghidraAddress USA: 0x8028a868
 */
extern s32 g_dwKeepCustomClipPlanes;

/** The near plane SetupMenuProjection() uses. @ghidraAddress USA: 0x8028a86c */
extern f32 g_fCameraNearScale;

/** The far plane SetupMenuProjection() uses. @ghidraAddress USA: 0x8028a870 */
extern f32 g_fCameraFarDistance;

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
