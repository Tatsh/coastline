#ifndef COASTLINE_SYSTEM_H
#define COASTLINE_SYSTEM_H

/**
 * @file
 * The display, DMA and decompression state of the system file of the N64 releases.
 */

#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/os_cont.h>
#include <PR/sptask.h>
#include <PR/ultratypes.h>

#include "displaylistops.h"
#include "frametime.h"
#include "screen.h"

/** The bump allocator zcalloc() draws from. */
typedef struct {
    u8 *cur; /*!< The next free byte. */
    u8 *end; /*!< The end of the memory to allocate from. */
} InflateAllocator;

/**
 * The first of the two frame buffers, of 320 by 240 16-bit pixels. The link script places it.
 *
 * @ghidraAddress USA: 0x801b5000
 * @ghidraAddress EUR: 0x801b5000
 * @ghidraAddress JPN: 0x801b5000
 */
extern u16 g_aFramebuffer0[];

/**
 * @ghidraAddress USA: 0x801da800
 * @ghidraAddress EUR: 0x801da800
 * @ghidraAddress JPN: 0x801da800
 */
extern u16 g_aFramebuffer1[];

/**
 * The depth buffer, of 16-bit depths, near the start of the memory. The link script places it.
 *
 * @ghidraAddress USA: 0x80000400
 */
extern u16 g_aDepthBuffer[];

/**
 * The room the heap of zlib takes while the game loads, and that the sound effects of a race are
 * loaded into.
 *
 * @ghidraAddress USA: 0x80324550
 * @ghidraAddress EUR: 0x80326a90
 * @ghidraAddress JPN: 0x803256c0
 */
extern u8 g_aInflateHeapBuffer[];

/**
 * The heap DecompressFromRom() lends to zlib, or NULL to let inflateInit2_() borrow the frame
 * buffers.
 *
 * @ghidraAddress USA: 0x80368ac0
 * @ghidraAddress EUR: 0x8036afc0
 * @ghidraAddress JPN: 0x80369c30
 */
extern u8 *g_pInflateHeapBase;

/**
 * @ghidraAddress USA: 0x80368ac4
 * @ghidraAddress EUR: 0x8036afc4
 * @ghidraAddress JPN: 0x80369c34
 */
extern u8 *g_pInflateHeapEnd;

/**
 * The state of the allocator inflateInit2_() gives zlib.
 *
 * @ghidraAddress USA: 0x80368ac8
 * @ghidraAddress EUR: 0x8036afc8
 * @ghidraAddress JPN: 0x80369c38
 */
extern InflateAllocator g_inflateAllocator;

/**
 * The frames to wait before the display returns after inflateInit2_() blanks it to use the frame
 * buffers as a heap.
 *
 * @ghidraAddress USA: 0x8028a888
 * @ghidraAddress EUR: 0x8028ca88
 * @ghidraAddress JPN: 0x8028c558
 */
extern u32 g_dwScreenUnblankDelay;

/**
 * Set up the display, the RSP and RDP state, and the system threads.
 *
 * @ghidraAddress USA: 0x8021be88
 * @ghidraAddress EUR: 0x8021d068
 * @ghidraAddress JPN: 0x8021c938
 */
void InitRenderingSubsystem(void);

/**
 * Copy bytes from the cartridge into memory by DMA, in blocks of 4 KB.
 *
 * @param dst The memory to copy into.
 * @param romOffset The cartridge offset to copy from.
 * @param size The number of bytes to copy.
 * @ghidraAddress USA: 0x8021c748
 * @ghidraAddress EUR: 0x8021d948
 * @ghidraAddress JPN: 0x8021d218
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
 */
void InitDmaRequest(DecompressJob *job, u8 *buffer);

/**
 * Decompress the asset at a cartridge offset into RAM.
 *
 * @param dst The memory to decompress into.
 * @param romOffset The cartridge offset of the asset.
 * @param job The decompression to continue, which DecompressFromRom() leaves where it stopped, or
 * NULL to decompress the asset whole.
 * @return The size of the asset once decompressed.
 * @ghidraAddress USA: 0x8021cd30
 * @ghidraAddress EUR: 0x8021df30
 * @ghidraAddress JPN: 0x8021d800
 */
u32 DecompressFromRom(void *dst, u32 romOffset, DecompressJob *job);

/**
 * Whether the display stays black whatever a frame would show.
 *
 * @ghidraAddress USA: 0x8028a884
 * @ghidraAddress EUR: 0x8028ca84
 * @ghidraAddress JPN: 0x8028c554
 */
extern u32 g_dwKeepScreenBlanked;

/**
 * The frames a screen has shown, for screens that count them.
 *
 * @ghidraAddress USA: 0x8028dbb0
 * @ghidraAddress EUR: 0x80290310
 * @ghidraAddress JPN: 0x8028fd80
 */
extern u16 g_wScreenFrameCount;

/**
 * The color dither the screens draw with, as the other-mode bits gDPSetColorDither() takes.
 *
 * @ghidraAddress USA: 0x8028a8a0
 * @ghidraAddress EUR: 0x8028caa4
 * @ghidraAddress JPN: 0x8028c574
 */
extern u32 g_dwColorDitherMode;

/**
 * The alpha dither the screens draw with, as the other-mode bits gDPSetAlphaDither() takes.
 *
 * @ghidraAddress USA: 0x8028a89c
 * @ghidraAddress EUR: 0x8028caa0
 * @ghidraAddress JPN: 0x8028c570
 */
extern u32 g_dwAlphaDitherMode;

/**
 * The two frame buffers.
 *
 * @ghidraAddress USA: 0x8031aa28
 * @ghidraAddress EUR: 0x8031cf28
 * @ghidraAddress JPN: 0x8031bb58
 */
extern u16 *g_apFramebuffers[2];

/**
 * The time the last frame took.
 *
 * @ghidraAddress USA: 0x8028aadc
 * @ghidraAddress EUR: 0x8028cce4
 * @ghidraAddress JPN: 0x8028c7b4
 */
extern s32 g_dwFrameDeltaMs;

/**
 * The time UpdateFrameTiming() counts, in milliseconds.
 *
 * @ghidraAddress USA: 0x8028aad4
 * @ghidraAddress EUR: 0x8028ccdc
 * @ghidraAddress JPN: 0x8028c7ac
 */
extern s32 g_dwElapsedTimeMs;

/**
 * Limit drawing to a rectangle, clipped to the frame.
 *
 * @param x The left edge, in pixels.
 * @param y The top edge, in pixels.
 * @param width The width, in pixels.
 * @param height The height, in pixels.
 * @ghidraAddress USA: 0x80219a78
 * @ghidraAddress EUR: 0x8021aa98
 * @ghidraAddress JPN: 0x8021a3f0
 */
void SetScissorRect(s32 x, s32 y, s32 width, s32 height);

/**
 * Show a message on the debug display.
 *
 * @param message The message to show.
 * @ghidraAddress USA: 0x8021e1f4
 * @ghidraAddress EUR: 0x8021f3f4
 * @ghidraAddress JPN: 0x8021ecc4
 */
void SetDebugMessage(const char *message);

/**
 * Add a fill of the whole frame with a colour to the display list.
 *
 * @param red The red component, from 0 to 255.
 * @param green The green component, from 0 to 255.
 * @param blue The blue component, from 0 to 255.
 * @ghidraAddress USA: 0x80217fb8
 * @ghidraAddress EUR: 0x80218fd8
 * @ghidraAddress JPN: 0x80218928
 */
void ClearScreen(s32 red, s32 green, s32 blue);

/**
 * Start the display list of a frame in the low resolution, 320 by 240.
 *
 * @ghidraAddress USA: 0x80219a1c
 * @ghidraAddress EUR: 0x8021aa3c
 * @ghidraAddress JPN: 0x8021a394
 */
void StartLowResFrame(void);

/**
 * Start the display list of a frame in the high resolution, 640 by 480.
 *
 * @ghidraAddress USA: 0x80219a3c
 * @ghidraAddress EUR: 0x8021aa5c
 * @ghidraAddress JPN: 0x8021a3b4
 */
void StartHighResFrame(void);

/**
 * End the display list of the frame and run it.
 *
 * @ghidraAddress USA: 0x8021aa08
 * @ghidraAddress EUR: 0x8021bb20
 * @ghidraAddress JPN: 0x8021b3fc
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
 * @ghidraAddress EUR: 0x8021ac10
 * @ghidraAddress JPN: 0x8021a568
 */
void SetViewport(s32 x, s32 y, s32 width, s32 height, s32 setScissor);

/**
 * Set up a fixed projection, looking at (512, 384) from 1000 along z, and load it into the display
 * list.
 *
 * @param width The width of the screen, which is not read.
 * @param height The height of the screen, which is not read.
 * @ghidraAddress USA: 0x8021b458
 * @ghidraAddress EUR: 0x8021c594
 * @ghidraAddress JPN: 0x8021be64
 */
void SetupProjectionMatrix(f32 width, f32 height);

/**
 * Set up the projection of a menu that draws models, looking at the middle of the screen, and load
 * it into the display list.
 *
 * @param width The width of the screen, which is not read.
 * @param height The height of the screen, which is not read.
 * @ghidraAddress USA: 0x8021b5a4
 * @ghidraAddress EUR: 0x8021c6e0
 * @ghidraAddress JPN: 0x8021bfb0
 */
void SetupMenuProjection(f32 width, f32 height);

/**
 * Whether SetupMenuProjection() keeps the near scale and far distance set before it once,
 * instead of its defaults. It clears this.
 *
 * @ghidraAddress USA: 0x8028a868
 * @ghidraAddress EUR: 0x8028ca68
 * @ghidraAddress JPN: 0x8028c538
 */
extern s32 g_dwKeepCustomClipPlanes;

/**
 * The near plane SetupMenuProjection() uses.
 *
 * @ghidraAddress USA: 0x8028a86c
 * @ghidraAddress EUR: 0x8028ca6c
 * @ghidraAddress JPN: 0x8028c53c
 */
extern f32 g_fCameraNearScale;

/**
 * The far plane SetupMenuProjection() uses.
 *
 * @ghidraAddress USA: 0x8028a870
 * @ghidraAddress EUR: 0x8028ca70
 * @ghidraAddress JPN: 0x8028c540
 */
extern f32 g_fCameraFarDistance;

/**
 * Fill the whole depth buffer with the farthest depth, then draw to the frame buffer again.
 *
 * @ghidraAddress USA: 0x80217c94
 * @ghidraAddress EUR: 0x80218cb4
 * @ghidraAddress JPN: 0x80218604
 */
void ClearDepthBuffer(void);

/**
 * Update the timing of the frames from the CPU timer.
 *
 * @ghidraAddress USA: 0x8021c4b4
 * @ghidraAddress EUR: 0x8021d6a8
 * @ghidraAddress JPN: 0x8021cf78
 */
void UpdateFrameTiming(void);

/**
 * Whether the screen of a race runs, which makes UpdateFrameTiming() step a fixed 1/30 of a second
 * a frame rather than count the timer of the CPU.
 *
 * @ghidraAddress USA: 0x8028ab10
 * @ghidraAddress EUR: 0x8028cd18
 * @ghidraAddress JPN: 0x8028c7e8
 */
extern s32 g_bRaceScreenActive;

/**
 * Fill a rectangle of the depth buffer with the farthest depth.
 *
 * @param x The left edge.
 * @param y The upper edge.
 * @param width The width.
 * @param height The height, which the bottom edge adds to x rather than y.
 * @ghidraAddress USA: 0x80217e20
 * @ghidraAddress EUR: 0x80218e40
 * @ghidraAddress JPN: 0x80218790
 */
void ClearZBufferRect(s32 x, s32 y, s32 width, s32 height);

/**
 * Restart the clock of the frames from zero.
 *
 * @ghidraAddress USA: 0x8021c46c
 * @ghidraAddress EUR: 0x8021d660
 * @ghidraAddress JPN: 0x8021cf30
 */
void ResetFrameClock(void);

/**
 * The messages of the DMA, which AllocIoMessage() hands out in turn.
 *
 * @ghidraAddress USA: 0x8031a020
 * @ghidraAddress EUR: 0x8031c500
 * @ghidraAddress JPN: 0x8031b130
 */
extern OSIoMesg g_aIoMessagePool[32];

/**
 * The DMA started and not yet waited for.
 *
 * @ghidraAddress USA: 0x80272d44
 * @ghidraAddress EUR: 0x80274f44
 * @ghidraAddress JPN: 0x80274a14
 */
extern s32 g_dwIoMessagePendingCount;

/**
 * The message of g_aIoMessagePool last handed out.
 *
 * @ghidraAddress USA: 0x80272d40
 * @ghidraAddress EUR: 0x80274f40
 * @ghidraAddress JPN: 0x80274a10
 */
extern s32 g_dwIoMessagePoolIndex;

/**
 * The queue a finished DMA sends to.
 *
 * @ghidraAddress USA: 0x80319f88
 * @ghidraAddress EUR: 0x8031c468
 * @ghidraAddress JPN: 0x8031b098
 */
extern OSMesgQueue g_DmaMessageQueue;

/**
 * @ghidraAddress USA: 0x80319fa0
 * @ghidraAddress EUR: 0x8031c480
 * @ghidraAddress JPN: 0x8031b0b0
 */
extern OSMesg g_aDmaMsgBuf[32];

/**
 * The queue the RSP event reaches, and the queue the thread forwards it to.
 *
 * @ghidraAddress USA: 0x8031a338
 * @ghidraAddress EUR: 0x8031c818
 * @ghidraAddress JPN: 0x8031b448
 */
extern OSMesgQueue g_RspEventMsgQueue;

/**
 * @ghidraAddress USA: 0x8031a354
 * @ghidraAddress EUR: 0x8031c834
 * @ghidraAddress JPN: 0x8031b464
 */
extern OSMesg g_aRspEventMsgBuf[1];

/**
 * @ghidraAddress USA: 0x8031a320
 * @ghidraAddress EUR: 0x8031c800
 * @ghidraAddress JPN: 0x8031b430
 */
extern OSMesgQueue g_RspDoneMsgQueue;

/**
 * @ghidraAddress USA: 0x8031a350
 * @ghidraAddress EUR: 0x8031c830
 * @ghidraAddress JPN: 0x8031b460
 */
extern OSMesg g_aRspDoneMsgBuf[1];

/**
 * The queue the RDP event reaches, and the queue the thread forwards it to.
 *
 * @ghidraAddress USA: 0x8031a370
 * @ghidraAddress EUR: 0x8031c850
 * @ghidraAddress JPN: 0x8031b480
 */
extern OSMesgQueue g_RdpEventMsgQueue;

/**
 * @ghidraAddress USA: 0x8031a38c
 * @ghidraAddress EUR: 0x8031c86c
 * @ghidraAddress JPN: 0x8031b49c
 */
extern OSMesg g_aRdpEventMsgBuf[1];

/**
 * @ghidraAddress USA: 0x8031a358
 * @ghidraAddress EUR: 0x8031c838
 * @ghidraAddress JPN: 0x8031b468
 */
extern OSMesgQueue g_RdpDoneMsgQueue;

/**
 * @ghidraAddress USA: 0x8031a388
 * @ghidraAddress EUR: 0x8031c868
 * @ghidraAddress JPN: 0x8031b498
 */
extern OSMesg g_aRdpDoneMsgBuf[1];

/**
 * The queue of the retrace of ViRetraceThread(), which nothing starts, and the queue each retrace
 * reaches.
 *
 * @ghidraAddress USA: 0x8031a3b0
 * @ghidraAddress EUR: 0x8031c8b0
 * @ghidraAddress JPN: 0x8031b4e0
 */
extern OSMesgQueue g_ViRetracePollMesgQueue;

/**
 * @ghidraAddress USA: 0x8031a3c8
 * @ghidraAddress EUR: 0x8031c8c8
 * @ghidraAddress JPN: 0x8031b4f8
 */
extern OSMesg g_ViRetracePollMesgBuf[1];

/**
 * @ghidraAddress USA: 0x8031a390
 * @ghidraAddress EUR: 0x8031c890
 * @ghidraAddress JPN: 0x8031b4c0
 */
extern OSMesgQueue g_ViRetraceMesgQueue;

/**
 * @ghidraAddress USA: 0x8031a3a8
 * @ghidraAddress EUR: 0x8031c8a8
 * @ghidraAddress JPN: 0x8031b4d8
 */
extern OSMesg g_ViRetraceMesgBuf[1];

/**
 * The value the events send.
 *
 * @ghidraAddress USA: 0x8031a3cc
 * @ghidraAddress EUR: 0x8031c8cc
 * @ghidraAddress JPN: 0x8031b4fc
 */
extern u32 g_dwDummyOsMesg;

/**
 * The retraces ViRetraceThread() forwarded, from 0 to 15.
 *
 * @ghidraAddress USA: 0x8028adec
 * @ghidraAddress EUR: 0x8028d09c
 * @ghidraAddress JPN: 0x8028cacc
 */
extern s32 g_dwVblankCounter;

/**
 * The threads of the system.
 *
 * @ghidraAddress USA: 0x802729e0
 * @ghidraAddress EUR: 0x80274a30
 * @ghidraAddress JPN: 0x80274500
 */
extern OSThread g_RspDoneThreadObj;

/**
 * @ghidraAddress USA: 0x80272b90
 * @ghidraAddress EUR: 0x80274be0
 * @ghidraAddress JPN: 0x802746b0
 */
extern OSThread g_RdpDoneThreadObj;

#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
/**
 * The thread that waits for the reset button.
 *
 * @ghidraAddress EUR: 0x80274d90
 * @ghidraAddress JPN: 0x80274860
 */
extern OSThread g_PreNmiResetHandlerThreadObj;
#endif

/**
 * @ghidraAddress USA: 0x80272830
 * @ghidraAddress EUR: 0x80274880
 * @ghidraAddress JPN: 0x80274350
 */
extern OSThread g_MainGameThreadObj;

/**
 * @ghidraAddress USA: 0x80272680
 * @ghidraAddress EUR: 0x802746d0
 * @ghidraAddress JPN: 0x802741a0
 */
extern OSThread g_BootIdleThreadObj;

/**
 * @ghidraAddress USA: 0x8031ac00
 * @ghidraAddress EUR: 0x8031d100
 * @ghidraAddress JPN: 0x8031bd30
 */
extern OSThread g_FaultThreadObj;

/* The words of the stacks of the threads. */
#define THREAD_STACK_WORDS 0x200 /*!< The words of the stack of each service thread. */
#define MAIN_STACK_WORDS 0x800   /*!< The words of the stack of the main thread. */
#define IDLE_STACK_WORDS 0x100   /*!< The words of the stack of the idle thread. */
#define FAULT_STACK_WORDS 0x100  /*!< The words of the stack of the fault thread. */

/**
 * The stacks of the threads.
 *
 * @ghidraAddress USA: 0x80318ed0
 * @ghidraAddress EUR: 0x8031a9b0
 * @ghidraAddress JPN: 0x803195e0
 */
extern u32 g_adwRspDoneThreadStack[THREAD_STACK_WORDS];

/**
 * @ghidraAddress USA: 0x803196d0
 * @ghidraAddress EUR: 0x8031b1b0
 * @ghidraAddress JPN: 0x80319de0
 */
extern u32 g_adwRdpDoneThreadStack[THREAD_STACK_WORDS];

#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
/**
 * @ghidraAddress EUR: 0x8031b9b0
 * @ghidraAddress JPN: 0x8031a5e0
 */
extern u32 g_adwPreNmiResetThreadStack[THREAD_STACK_WORDS];
#endif

/**
 * @ghidraAddress USA: 0x80316cd0
 * @ghidraAddress EUR: 0x803189b0
 * @ghidraAddress JPN: 0x803175e0
 */
extern u32 g_adwMainThreadStack[MAIN_STACK_WORDS];

/**
 * @ghidraAddress USA: 0x803168d0
 * @ghidraAddress EUR: 0x803185b0
 * @ghidraAddress JPN: 0x803171e0
 */
extern u32 g_adwIdleThreadStack[IDLE_STACK_WORDS];

/**
 * @ghidraAddress USA: 0x8031adb0
 * @ghidraAddress EUR: 0x8031d2b0
 * @ghidraAddress JPN: 0x8031bee0
 */
extern u32 g_adwFaultThreadStack[FAULT_STACK_WORDS];

/**
 * The memory the display lists are built in.
 *
 * @ghidraAddress USA: 0x80272d68
 * @ghidraAddress EUR: 0x80274f68
 * @ghidraAddress JPN: 0x80274a38
 */
extern Gfx g_aGfxDisplayListPool[];

/**
 * The start of the display lists.
 *
 * @ghidraAddress USA: 0x8028a848
 * @ghidraAddress EUR: 0x8028ca48
 * @ghidraAddress JPN: 0x8028c518
 */
extern Gfx *g_pDisplayListBase;

/**
 * The counts of the CPU counter a millisecond.
 *
 * @ghidraAddress USA: 0x8028a8b0
 * @ghidraAddress EUR: 0x8028cab4
 * @ghidraAddress JPN: 0x8028c584
 */
extern f32 g_fCountsPerMillisecond;

/**
 * What the controllers reported at the start.
 *
 * @ghidraAddress USA: 0x8031a3d0
 * @ghidraAddress EUR: 0x8031c8d0
 * @ghidraAddress JPN: 0x8031b500
 */
extern OSContStatus g_aContStatus[MAXCONTROLLERS];

/**
 * @ghidraAddress USA: 0x80272d60
 * @ghidraAddress EUR: 0x80274f60
 * @ghidraAddress JPN: 0x80274a30
 */
extern OSMesg g_aSiEventMesgBuf[1];

/**
 * The clock of the frames, in counts of the CPU counter.
 *
 * @ghidraAddress USA: 0x8028adf8
 * @ghidraAddress EUR: 0x8028d0a8
 * @ghidraAddress JPN: 0x8028cad8
 */
extern u64 g_qwTimestampBase;

/**
 * The CPU counter at the last frame.
 *
 * @ghidraAddress USA: 0x8028ae00
 * @ghidraAddress EUR: 0x8028d0b0
 * @ghidraAddress JPN: 0x8028cae0
 */
extern u32 g_dwFrameTimestamp;

/**
 * The time of the frame before, in milliseconds.
 *
 * @ghidraAddress USA: 0x8028aad0
 * @ghidraAddress EUR: 0x8028ccd8
 * @ghidraAddress JPN: 0x8028c7a8
 */
extern s32 g_dwPrevElapsedTimeMs;

/**
 * The words the DMA reads sizes and chunk lengths into.
 *
 * @ghidraAddress USA: 0x8031b328
 * @ghidraAddress EUR: 0x8031d838
 * @ghidraAddress JPN: 0x8031c468
 */
extern u32 g_dwDmaAssetSizeBuffer;

/**
 * @ghidraAddress USA: 0x8031b330
 * @ghidraAddress EUR: 0x8031d840
 * @ghidraAddress JPN: 0x8031c470
 */
extern u32 g_dwDmaReadWordBuffer;

/**
 * @ghidraAddress USA: 0x8031b348
 * @ghidraAddress EUR: 0x8031d868
 * @ghidraAddress JPN: 0x8031c498
 */
extern u32 g_dwDmaChunkSize;

/**
 * The message the debug display shows.
 *
 * @ghidraAddress USA: 0x8028a88c
 * @ghidraAddress EUR: 0x8028ca90
 * @ghidraAddress JPN: 0x8028c560
 */
extern const char *g_pszDebugMessage;

/**
 * The queue of the fault event and the thread that faulted.
 *
 * @ghidraAddress USA: 0x8031b1b0
 * @ghidraAddress EUR: 0x8031d6b0
 * @ghidraAddress JPN: 0x8031c2e0
 */
extern OSMesgQueue g_FaultMsgQueue;

/**
 * @ghidraAddress USA: 0x8031b1c8
 * @ghidraAddress EUR: 0x8031d6c8
 * @ghidraAddress JPN: 0x8031c2f8
 */
extern OSMesg g_aFaultMsgBuf[1];

/**
 * @ghidraAddress USA: 0x8031b1cc
 * @ghidraAddress EUR: 0x8031d6cc
 * @ghidraAddress JPN: 0x8031c2fc
 */
extern OSThread *g_pFaultedThread;

/**
 * Cleared once by the fault thread.
 *
 * @ghidraAddress USA: 0x8031b1d0
 * @ghidraAddress EUR: 0x8031d6d0
 * @ghidraAddress JPN: 0x8031c300
 */
extern s32 g_dwFaultState;

/**
 * The arguments of a development kit and their count, which starts at 1, and the copies of the
 * argument vector and of the address of the count given to the threads.
 *
 * @ghidraAddress USA: 0x80316450
 * @ghidraAddress EUR: 0x80318130
 * @ghidraAddress JPN: 0x80316d60
 */
extern char *g_apszBootArgv[];

/**
 * @ghidraAddress USA: 0x8028b2ec
 * @ghidraAddress EUR: 0x8028d59c
 * @ghidraAddress JPN: 0x8028cfcc
 */
extern s32 g_dwBootArgc;

/**
 * @ghidraAddress USA: 0x8028b2f0
 * @ghidraAddress EUR: 0x8028d5a0
 * @ghidraAddress JPN: 0x8028cfd0
 */
extern char **g_ppszBootArgv;

/**
 * @ghidraAddress USA: 0x8026ff04
 * @ghidraAddress EUR: 0x80271ed4
 * @ghidraAddress JPN: 0x80271984
 */
extern char **g_ppszMainArgv;

/**
 * @ghidraAddress USA: 0x8026ff00
 * @ghidraAddress EUR: 0x80271ed0
 * @ghidraAddress JPN: 0x80271980
 */
extern s32 *g_pdwMainArgc;

/**
 * The queue of the commands of the PI manager.
 *
 * @ghidraAddress USA: 0x80319ed0
 * @ghidraAddress EUR: 0x8031c3b0
 * @ghidraAddress JPN: 0x8031ade0
 */
extern OSMesgQueue g_PiMgrCmdQueue;

/**
 * @ghidraAddress USA: 0x80319ee8
 * @ghidraAddress EUR: 0x8031c3c8
 * @ghidraAddress JPN: 0x8031aff8
 */
extern OSMesg g_aPiMgrCmdMsgBuf[32];

/** The video modes of the NTSC and the MPAL consoles. @ghidraAddress USA: 0x802a54b0 */
extern OSViMode g_ViModeNtscLan1;

/** @ghidraAddress USA: 0x802a5d70 */
extern OSViMode g_ViModeMpalLan1;

/**
 * Take a message of the DMA, waiting for a DMA to finish when every one is in flight.
 *
 * @return The message.
 * @ghidraAddress USA: 0x802172d0
 * @ghidraAddress EUR: 0x802182f0
 * @ghidraAddress JPN: 0x80217c40
 */
OSIoMesg *AllocIoMessage(void);

/**
 * Wait for every DMA started to finish.
 *
 * @ghidraAddress USA: 0x8021735c
 * @ghidraAddress EUR: 0x8021837c
 * @ghidraAddress JPN: 0x80217ccc
 */
void WaitForDmaMessages(void);

/**
 * Return 0. Nothing calls it.
 *
 * @param unused0 Not read.
 * @param unused1 Not read.
 * @return 0.
 * @ghidraAddress USA: 0x802173b8
 * @ghidraAddress EUR: 0x802183d8
 * @ghidraAddress JPN: 0x80217d28
 */
s32 UnusedVblankQueryStub(s32 unused0, s32 unused1);

/**
 * Forward each finished task of the RSP to g_RspDoneMsgQueue, marking the timing bars.
 *
 * @param arg Not read.
 * @ghidraAddress USA: 0x8021bbdc
 * @ghidraAddress EUR: 0x8021cd18
 * @ghidraAddress JPN: 0x8021c5e8
 */
void RspDoneThread(void *arg);

/**
 * Forward each finished frame of the RDP to g_RdpDoneMsgQueue, marking the timing bars.
 *
 * @param arg Not read.
 * @ghidraAddress USA: 0x8021bca8
 * @ghidraAddress EUR: 0x8021cdc8
 * @ghidraAddress JPN: 0x8021c698
 */
void RdpDoneThread(void *arg);

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
/**
 * The queue of the reset button, and its buffer.
 *
 * @ghidraAddress EUR: 0x8031c870
 * @ghidraAddress JPN: 0x8031b4a0
 */
extern OSMesgQueue g_PreNmiMesgQueue;

/**
 * @ghidraAddress EUR: 0x8031c888
 * @ghidraAddress JPN: 0x8031b4b8
 */
extern OSMesg g_aPreNmiMesgBuf[1];

/**
 * Set once the reset button has been pressed.
 *
 * @ghidraAddress EUR: 0x8028ca8c
 * @ghidraAddress JPN: 0x8028c55c
 */
extern s32 g_dwPreNmiResetPending;

/**
 * Wait for the reset button, then silence the sound, record the reset, and blank the screen, for
 * each press.
 *
 * @param arg Not read.
 * @ghidraAddress EUR: 0x8021ce88
 * @ghidraAddress JPN: 0x8021c758
 */
void PreNmiResetHandlerThread(void *arg);
#endif

/**
 * Forward each retrace to g_ViRetraceMesgQueue and count it. Nothing starts it.
 *
 * @param arg Not read.
 * @ghidraAddress USA: 0x8021bd68
 * @ghidraAddress EUR: 0x8021cf50
 * @ghidraAddress JPN: 0x8021c820
 */
void ViRetraceThread(void *arg);

/**
 * Do nothing.
 *
 * @ghidraAddress USA: 0x8021c6a8
 * @ghidraAddress EUR: 0x8021d89c
 * @ghidraAddress JPN: 0x8021d16c
 */
void UnusedFrameTimingStub(void);

/**
 * Wait for the next retrace.
 *
 * @ghidraAddress USA: 0x8021e1c0
 * @ghidraAddress EUR: 0x8021f3c0
 * @ghidraAddress JPN: 0x8021ec90
 */
void WaitForVblankMessage(void);

/**
 * Do nothing; where a debugger would stop.
 *
 * @param unused Not read.
 * @ghidraAddress USA: 0x8021e1ec
 * @ghidraAddress EUR: 0x8021f3ec
 * @ghidraAddress JPN: 0x8021ecbc
 */
void StubFunction(s32 unused);

/**
 * Wait for faults and keep the thread that faulted.
 *
 * @param arg Not read.
 * @ghidraAddress USA: 0x8021e21c
 * @ghidraAddress EUR: 0x8021f41c
 * @ghidraAddress JPN: 0x8021ecec
 */
void FaultThread(void *arg);

/**
 * Read the arguments of a development kit, start the managers, the fault thread and the main
 * thread, and become the idle thread.
 *
 * @param arg The argument of the main thread.
 * @ghidraAddress USA: 0x8021e2c8
 * @ghidraAddress EUR: 0x8021f4c8
 * @ghidraAddress JPN: 0x8021ed98
 */
void BootIdleThread(void *arg);

/** The commands of the display list of each frame. */
#define DISPLAY_LIST_COMMANDS 6000

/** The commands at the start of the display list of each frame that the frame does not use. */
#define DISPLAY_LIST_RESERVED 64

/**
 * Start the display list of a frame in a resolution, with the render modes every frame starts
 * with. A change of resolution takes effect at the next FlushDisplayList().
 *
 * @param highRes 1 for the high resolution, 0 for the low.
 * @ghidraAddress USA: 0x80219470
 * @ghidraAddress EUR: 0x8021a490
 * @ghidraAddress JPN: 0x80219de0
 */
void BeginDisplayList(s32 highRes);

/**
 * Do nothing. The arguments are variable, as of a print compiled out.
 *
 * @param unused0 Not read.
 * @ghidraAddress USA: 0x80219a5c
 * @ghidraAddress EUR: 0x8021aa7c
 * @ghidraAddress JPN: 0x8021a3d4
 */
void StubEmptyVoid(s32 unused0, ...);

/**
 * The tasks of the RSP, one for each frame buffer.
 *
 * @ghidraAddress USA: 0x8031a9a8
 * @ghidraAddress EUR: 0x8031cea8
 * @ghidraAddress JPN: 0x8031bad8
 */
extern OSTask g_aGfxTask[2];

/**
 * The boot microcode of the RSP. The text of the graphics microcode follows it.
 *
 * @ghidraAddress USA: 0x8026ea00
 * @ghidraAddress EUR: 0x802709d0
 * @ghidraAddress JPN: 0x80270480
 */
extern u64 g_aRspBootUcode[];

/**
 * @ghidraAddress USA: 0x8026ead0
 * @ghidraAddress EUR: 0x80270aa0
 * @ghidraAddress JPN: 0x80270550
 */
extern u64 g_aGspUcodeText[];

/**
 * The data of the graphics microcode.
 *
 * @ghidraAddress USA: 0x802abc00
 * @ghidraAddress EUR: 0x802ad8f0
 * @ghidraAddress JPN: 0x802ac330
 */
extern u64 g_abRspUcodeData[];

/**
 * The memory of the matrix stack of the RSP, 16 bytes more than the stack to align it.
 *
 * @ghidraAddress USA: 0x8031a598
 * @ghidraAddress EUR: 0x8031ca98
 * @ghidraAddress JPN: 0x8031b6c8
 */
extern u8 g_abRspDramStackBuffer[];

/**
 * The buffer the RDP commands go to, and its end.
 *
 * @ghidraAddress USA: 0x8028a860
 * @ghidraAddress EUR: 0x8028ca60
 * @ghidraAddress JPN: 0x8028c530
 */
extern u64 *g_pRdpOutputBuffer;

/**
 * @ghidraAddress USA: 0x8028a864
 * @ghidraAddress EUR: 0x8028ca64
 * @ghidraAddress JPN: 0x8028c534
 */
extern u64 *g_pRdpOutputBufferEnd;

/**
 * The doublewords at the end of the output buffer the tasks do not use.
 *
 * @ghidraAddress USA: 0x8028ab84
 * @ghidraAddress EUR: 0x8028ce30
 * @ghidraAddress JPN: 0x8028c860
 */
extern s32 g_dwOutputBuffReserve;

/**
 * The commands of the display list of the last frame.
 *
 * @ghidraAddress USA: 0x8028ab70
 * @ghidraAddress EUR: 0x8028ce1c
 * @ghidraAddress JPN: 0x8028c84c
 */
extern s32 g_dwDisplayListCommandCount;

/**
 * The most commands of a shadow, vertices of a shadow, and commands of a frame seen.
 *
 * @ghidraAddress USA: 0x8028ab74
 * @ghidraAddress EUR: 0x8028ce20
 * @ghidraAddress JPN: 0x8028c850
 */
extern s32 g_dwPeakShadowCommandCount;

/**
 * @ghidraAddress USA: 0x8028ab78
 * @ghidraAddress EUR: 0x8028ce24
 * @ghidraAddress JPN: 0x8028c854
 */
extern s32 g_dwPeakShadowVertexCount;

/**
 * @ghidraAddress USA: 0x8028ab7c
 * @ghidraAddress EUR: 0x8028ce28
 * @ghidraAddress JPN: 0x8028c858
 */
extern s32 g_dwPeakDisplayListCommands;

/**
 * Whether a frame has been sent to the RSP.
 *
 * @ghidraAddress USA: 0x8028ab80
 * @ghidraAddress EUR: 0x8028ce2c
 * @ghidraAddress JPN: 0x8028c85c
 */
extern s32 g_dwFirstFlushDone;

/**
 * The counts of the CPU counter the end of a frame took, between the ends of two frames, and at
 * the end of the last frame.
 *
 * @ghidraAddress USA: 0x8028aae8
 * @ghidraAddress EUR: 0x8028ccf0
 * @ghidraAddress JPN: 0x8028c7c0
 */
extern u32 g_dwFlushDuration;

/**
 * @ghidraAddress USA: 0x8028aae0
 * @ghidraAddress EUR: 0x8028cce8
 * @ghidraAddress JPN: 0x8028c7b8
 */
extern u32 g_dwFlushInterval;

/**
 * @ghidraAddress USA: 0x8028aae4
 * @ghidraAddress EUR: 0x8028ccec
 * @ghidraAddress JPN: 0x8028c7bc
 */
extern u32 g_dwFlushStartTime;

/**
 * A word FlushDisplayList() copies each frame, and the copy. No other routine reads either.
 *
 * @ghidraAddress USA: 0x8028aa20
 * @ghidraAddress EUR: 0x8028cc28
 * @ghidraAddress JPN: 0x8028c6f8
 */
extern u32 g_dwFlushValueSource;

/**
 * @ghidraAddress USA: 0x8028aa24
 * @ghidraAddress EUR: 0x8028cc2c
 * @ghidraAddress JPN: 0x8028c6fc
 */
extern u32 g_dwLastFlushValue;

/**
 * The frames until the video mode changes to the resolution BeginDisplayList() was given, or 0.
 *
 * @ghidraAddress USA: 0x8028a84c
 * @ghidraAddress EUR: 0x8028ca4c
 * @ghidraAddress JPN: 0x8028c51c
 */
extern s32 g_dwDisplayModeChangeCountdown;

/**
 * The resolution the video mode was last set to.
 *
 * @ghidraAddress USA: 0x8028a854
 * @ghidraAddress EUR: 0x8028ca54
 * @ghidraAddress JPN: 0x8028c524
 */
extern s32 g_dwAppliedScreenScaleShift;

/**
 * The video modes of the high resolution.
 *
 * @ghidraAddress USA: 0x802a5780
 * @ghidraAddress JPN: 0x802a7690
 */
extern OSViMode g_ViModeNtscHaf1;

/**
 * @ghidraAddress USA: 0x802a6040
 * @ghidraAddress JPN: 0x802a7640
 */
extern OSViMode g_ViModeMpalHaf1;

#if TGR_REGION == TGR_REGION_EU
/** The copies of the PAL video modes the game sets. @ghidraAddress EUR: 0x8028cd74 */
extern OSViMode g_ViModePalHaf1;

/** @ghidraAddress EUR: 0x8028cdc4 */
extern OSViMode g_ViModePalLan1;
#endif

/**
 * Whether the video interface corrects the gamma.
 *
 * @ghidraAddress USA: 0x8028aa4c
 * @ghidraAddress EUR: 0x8028cc54
 * @ghidraAddress JPN: 0x8028c724
 */
extern s32 g_dwViGammaEnabled;

#endif
