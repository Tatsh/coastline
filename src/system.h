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

/** The bump allocator zcalloc() draws from. */
typedef struct {
    u8 *cur; /*!< The next free byte. */
    u8 *end; /*!< The end of the memory to allocate from. */
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
 */
extern u8 g_aInflateHeapBuffer[];

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
void DmaRead(void *dst, u32 romOffset, s32 size);

/**
 * Read a word from the cartridge.
 *
 * @param romOffset The cartridge offset of the word.
 * @return The word.
 * @ghidraAddress USA: 0x8021c848
 */
u32 DmaReadWord(u32 romOffset);

/**
 * Read the decompressed size from the header of the asset at a cartridge offset.
 *
 * @param romOffset The cartridge offset of the asset.
 * @return The size of the asset once decompressed, in bytes.
 * @ghidraAddress USA: 0x8021c814
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
 * @return The size of the asset once decompressed.
 * @ghidraAddress USA: 0x8021cd30
 */
u32 DecompressFromRom(void *dst, u32 romOffset, DecompressJob *job);

/**
 * Patch the render modes of the track and vehicle display lists for the current weather.
 *
 * @ghidraAddress USA: 0x8021b97c
 */
void ProcessAllDisplayLists(void);

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
 * Start the display list of a frame in the low resolution, 320 by 240.
 *
 * @ghidraAddress USA: 0x80219a1c
 */
void StartLowResFrame(void);

/**
 * Start the display list of a frame in the high resolution, 640 by 480.
 *
 * @ghidraAddress USA: 0x80219a3c
 */
void StartHighResFrame(void);

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
 * Set up a fixed projection, looking at (512, 384) from 1000 along z, and load it into the display
 * list.
 *
 * @param width The width of the screen, which is not read.
 * @param height The height of the screen, which is not read.
 * @ghidraAddress USA: 0x8021b458
 */
void SetupProjectionMatrix(f32 width, f32 height);

/**
 * Set up the projection of a menu that draws models, looking at the middle of the screen, and load
 * it into the display list.
 *
 * @param width The width of the screen, which is not read.
 * @param height The height of the screen, which is not read.
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
 * Fill the whole depth buffer with the farthest depth, then draw to the frame buffer again.
 *
 * @ghidraAddress USA: 0x80217c94
 */
void ClearDepthBuffer(void);

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

/**
 * Whether the screen of a race runs, which makes UpdateFrameTiming() step a fixed 1/30 of a second
 * a frame rather than count the timer of the CPU.
 *
 * @ghidraAddress USA: 0x8028ab10
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
 */
void ClearZBufferRect(s32 x, s32 y, s32 width, s32 height);

/**
 * Restart the clock of the frames from zero.
 *
 * @ghidraAddress USA: 0x8021c46c
 */
void ResetFrameClock(void);

/**
 * The messages of the DMA, which AllocIoMessage() hands out in turn.
 *
 * @ghidraAddress USA: 0x8031a020
 */
extern OSIoMesg g_aIoMessagePool[32];

/** The DMA started and not yet waited for. @ghidraAddress USA: 0x80272d44 */
extern s32 g_dwIoMessagePendingCount;

/** The message of g_aIoMessagePool last handed out. @ghidraAddress USA: 0x80272d40 */
extern s32 g_dwIoMessagePoolIndex;

/** The queue a finished DMA sends to. @ghidraAddress USA: 0x80319f88 */
extern OSMesgQueue g_DmaMessageQueue;

/** @ghidraAddress USA: 0x80319fa0 */
extern OSMesg g_aDmaMsgBuf[32];

/**
 * The queue the RSP event reaches, and the queue the thread forwards it to.
 *
 * @ghidraAddress USA: 0x8031a338
 */
extern OSMesgQueue g_RspEventMsgQueue;

/** @ghidraAddress USA: 0x8031a354 */
extern OSMesg g_aRspEventMsgBuf[1];

/** @ghidraAddress USA: 0x8031a320 */
extern OSMesgQueue g_RspDoneMsgQueue;

/** @ghidraAddress USA: 0x8031a350 */
extern OSMesg g_aRspDoneMsgBuf[1];

/**
 * The queue the RDP event reaches, and the queue the thread forwards it to.
 *
 * @ghidraAddress USA: 0x8031a370
 */
extern OSMesgQueue g_RdpEventMsgQueue;

/** @ghidraAddress USA: 0x8031a38c */
extern OSMesg g_aRdpEventMsgBuf[1];

/** @ghidraAddress USA: 0x8031a358 */
extern OSMesgQueue g_RdpDoneMsgQueue;

/** @ghidraAddress USA: 0x8031a388 */
extern OSMesg g_aRdpDoneMsgBuf[1];

/**
 * The queue of the retrace of ViRetraceThread(), which nothing starts, and the queue each retrace
 * reaches.
 *
 * @ghidraAddress USA: 0x8031a3b0
 */
extern OSMesgQueue g_ViRetracePollMesgQueue;

/** @ghidraAddress USA: 0x8031a3c8 */
extern OSMesg g_ViRetracePollMesgBuf[1];

/** @ghidraAddress USA: 0x8031a390 */
extern OSMesgQueue g_ViRetraceMesgQueue;

/** @ghidraAddress USA: 0x8031a3a8 */
extern OSMesg g_ViRetraceMesgBuf[1];

/** The value the events send. @ghidraAddress USA: 0x8031a3cc */
extern u32 g_dwDummyOsMesg;

/** The retraces ViRetraceThread() forwarded, from 0 to 15. @ghidraAddress USA: 0x8028adec */
extern s32 g_dwVblankCounter;

/** The threads of the system. @ghidraAddress USA: 0x802729e0 */
extern OSThread g_RspDoneThreadObj;

/** @ghidraAddress USA: 0x80272b90 */
extern OSThread g_RdpDoneThreadObj;

/** @ghidraAddress USA: 0x80272830 */
extern OSThread g_MainGameThreadObj;

/** @ghidraAddress USA: 0x80272680 */
extern OSThread g_BootIdleThreadObj;

/** @ghidraAddress USA: 0x8031ac00 */
extern OSThread g_FaultThreadObj;

/** The words of the stacks of the threads. */
#define THREAD_STACK_WORDS 0x200
#define MAIN_STACK_WORDS 0x800
#define IDLE_STACK_WORDS 0x100
#define FAULT_STACK_WORDS 0x100

/** The stacks of the threads. @ghidraAddress USA: 0x80318ed0 */
extern u32 g_adwRspDoneThreadStack[THREAD_STACK_WORDS];

/** @ghidraAddress USA: 0x803196d0 */
extern u32 g_adwRdpDoneThreadStack[THREAD_STACK_WORDS];

/** @ghidraAddress USA: 0x80316cd0 */
extern u32 g_adwMainThreadStack[MAIN_STACK_WORDS];

/** @ghidraAddress USA: 0x803168d0 */
extern u32 g_adwIdleThreadStack[IDLE_STACK_WORDS];

/** @ghidraAddress USA: 0x8031adb0 */
extern u32 g_adwFaultThreadStack[FAULT_STACK_WORDS];

/** The memory the display lists are built in. @ghidraAddress USA: 0x80272d68 */
extern Gfx g_aGfxDisplayListPool[];

/** The start of the display lists. @ghidraAddress USA: 0x8028a848 */
extern Gfx *g_pDisplayListBase;

/** The counts of the CPU counter a millisecond. @ghidraAddress USA: 0x8028a8b0 */
extern f32 g_fCountsPerMillisecond;

/** What the controllers reported at the start. @ghidraAddress USA: 0x8031a3d0 */
extern OSContStatus g_aContStatus[MAXCONTROLLERS];

/** @ghidraAddress USA: 0x80272d60 */
extern OSMesg g_aSiEventMesgBuf[1];

/** The clock of the frames, in counts of the CPU counter. @ghidraAddress USA: 0x8028adf8 */
extern u64 g_qwTimestampBase;

/** The CPU counter at the last frame. @ghidraAddress USA: 0x8028ae00 */
extern u32 g_dwFrameTimestamp;

/** The time of the frame before, in milliseconds. @ghidraAddress USA: 0x8028aad0 */
extern s32 g_dwPrevElapsedTimeMs;

/**
 * The routines of the screen: the current one, the next, and a second.
 *
 * @ghidraAddress USA: 0x8031b318
 */
extern ScreenCallback g_pfnCurrentScreen;

/** @ghidraAddress USA: 0x8031b31c */
extern ScreenCallback g_pfnNextScreen;

/** @ghidraAddress USA: 0x8031b320 */
extern ScreenCallback g_pfnSecondaryScreen;

/** The words the DMA reads sizes and chunk lengths into. @ghidraAddress USA: 0x8031b328 */
extern u32 g_dwDmaAssetSizeBuffer;

/** @ghidraAddress USA: 0x8031b330 */
extern u32 g_dwDmaReadWordBuffer;

/** @ghidraAddress USA: 0x8031b348 */
extern u32 g_dwDmaChunkSize;

/** The message the debug display shows. @ghidraAddress USA: 0x8028a88c */
extern const char *g_pszDebugMessage;

/** The queue of the fault event and the thread that faulted. @ghidraAddress USA: 0x8031b1b0 */
extern OSMesgQueue g_FaultMsgQueue;

/** @ghidraAddress USA: 0x8031b1c8 */
extern OSMesg g_aFaultMsgBuf[1];

/** @ghidraAddress USA: 0x8031b1cc */
extern OSThread *g_pFaultedThread;

/** Cleared once by the fault thread. @ghidraAddress USA: 0x8031b1d0 */
extern s32 g_dwFaultState;

/**
 * The arguments of a development kit and their count, which starts at 1, and the copies of the
 * argument vector and of the address of the count given to the threads.
 *
 * @ghidraAddress USA: 0x80316450
 */
extern char *g_apszBootArgv[];

/** @ghidraAddress USA: 0x8028b2ec */
extern s32 g_dwBootArgc;

/** @ghidraAddress USA: 0x8028b2f0 */
extern char **g_ppszBootArgv;

/** @ghidraAddress USA: 0x8026ff04 */
extern char **g_ppszMainArgv;

/** @ghidraAddress USA: 0x8026ff00 */
extern s32 *g_pdwMainArgc;

/** The queue of the commands of the PI manager. @ghidraAddress USA: 0x80319ed0 */
extern OSMesgQueue g_PiMgrCmdQueue;

/** @ghidraAddress USA: 0x80319ee8 */
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
 */
OSIoMesg *AllocIoMessage(void);

/**
 * Wait for every DMA started to finish.
 *
 * @ghidraAddress USA: 0x8021735c
 */
void WaitForDmaMessages(void);

/**
 * Return 0. Nothing calls it.
 *
 * @param unused0 Not read.
 * @param unused1 Not read.
 * @return 0.
 * @ghidraAddress USA: 0x802173b8
 */
s32 UnusedVblankQueryStub(s32 unused0, s32 unused1);

/**
 * Forward each finished task of the RSP to g_RspDoneMsgQueue, marking the timing bars.
 *
 * @param arg Not read.
 * @ghidraAddress USA: 0x8021bbdc
 */
void RspDoneThread(void *arg);

/**
 * Forward each finished frame of the RDP to g_RdpDoneMsgQueue, marking the timing bars.
 *
 * @param arg Not read.
 * @ghidraAddress USA: 0x8021bca8
 */
void RdpDoneThread(void *arg);

/**
 * Forward each retrace to g_ViRetraceMesgQueue and count it. Nothing starts it.
 *
 * @param arg Not read.
 * @ghidraAddress USA: 0x8021bd68
 */
void ViRetraceThread(void *arg);

/** Do nothing. @ghidraAddress USA: 0x8021c6a8 */
void UnusedFrameTimingStub(void);

/**
 * Set the second routine of the screen.
 *
 * @param callback The routine.
 * @ghidraAddress USA: 0x8021c6c4
 */
void SetSecondaryScreenCallback(ScreenCallback callback);

/** Call the routine of the current screen. @ghidraAddress USA: 0x8021c6f0 */
void CallCurrentGameMode(void);

/** Wait for the next retrace. @ghidraAddress USA: 0x8021e1c0 */
void WaitForVblankMessage(void);

/**
 * Do nothing; where a debugger would stop.
 *
 * @param unused Not read.
 * @ghidraAddress USA: 0x8021e1ec
 */
void StubFunction(s32 unused);

/**
 * Wait for faults and keep the thread that faulted.
 *
 * @param arg Not read.
 * @ghidraAddress USA: 0x8021e21c
 */
void FaultThread(void *arg);

/**
 * Read the arguments of a development kit, start the managers, the fault thread and the main
 * thread, and become the idle thread.
 *
 * @param arg The argument of the main thread.
 * @ghidraAddress USA: 0x8021e2c8
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
 */
void BeginDisplayList(s32 highRes);

/**
 * Do nothing.
 *
 * @param unused0 Not read.
 * @param unused1 Not read.
 * @param unused2 Not read.
 * @param unused3 Not read.
 * @ghidraAddress USA: 0x80219a5c
 */
void StubEmptyVoid(s32 unused0, s32 unused1, s32 unused2, s32 unused3);

/**
 * Patch the render modes and the colour combines of a display list in place for the weather. A
 * render mode found in the first column of a table becomes the one of the column of the weather.
 *
 * @param displayList The display list, or NULL.
 * @param table The render modes: a row for each mode, with the mode first, then its replacements
 * for the clear weather, for a weather, and for a weather with the rain or the night. The last
 * three rows are translucent.
 * @return 1 if a translucent render mode was found, 0 otherwise.
 * @ghidraAddress USA: 0x8021b72c
 */
s32 ProcessDisplayList(Gfx *displayList, Gfx table[][4]);

/** The rows of a table of render modes of ProcessDisplayList(), and the first translucent one. */
#define RENDER_MODE_ROWS 6
#define RENDER_MODE_TRANSLUCENT_ROW 3

/**
 * The colour combine of the clear weather and the night replaces, and its replacement.
 *
 * @ghidraAddress USA: 0x8028ab88
 */
extern Gfx g_aDlClearCombinePatch[1][2];

/** The colour combines that the snow tints. @ghidraAddress USA: 0x8028ab98 */
extern Gfx g_aDlSnowTintCombines[2];

/** The render modes of the track and of the vehicles. @ghidraAddress USA: 0x8028aba8 */
extern Gfx g_aaDlTrackRenderModes[RENDER_MODE_ROWS][4];

/** The render modes of the body of a ghost. @ghidraAddress USA: 0x8028ac68 */
extern Gfx g_aaDlGhostRenderModes[RENDER_MODE_ROWS][4];

/** The render modes of the wheels of a ghost. @ghidraAddress USA: 0x8028ad28 */
extern Gfx g_aaDlGhostWheelRenderModes[RENDER_MODE_ROWS][4];

/**
 * Whether ProcessDisplayList() tints the colour combines of the snow. It clears this.
 *
 * @ghidraAddress USA: 0x8028ade8
 */
extern s32 g_bDisplayListSnowTint;

/** The tasks of the RSP, one for each frame buffer. @ghidraAddress USA: 0x8031a9a8 */
extern OSTask g_aGfxTask[2];

/**
 * The boot microcode of the RSP. The text of the graphics microcode follows it.
 *
 * @ghidraAddress USA: 0x8026ea00
 */
extern u64 g_aRspBootUcode[];

/** @ghidraAddress USA: 0x8026ead0 */
extern u64 g_aGspUcodeText[];

/** The data of the graphics microcode. @ghidraAddress USA: 0x802abc00 */
extern u64 g_abRspUcodeData[];

/**
 * The memory of the matrix stack of the RSP, 16 bytes more than the stack to align it.
 *
 * @ghidraAddress USA: 0x8031a598
 */
extern u8 g_abRspDramStackBuffer[];

/** The buffer the RDP commands go to, and its end. @ghidraAddress USA: 0x8028a860 */
extern u64 *g_pRdpOutputBuffer;

/** @ghidraAddress USA: 0x8028a864 */
extern u64 *g_pRdpOutputBufferEnd;

/**
 * The doublewords at the end of the output buffer the tasks do not use.
 *
 * @ghidraAddress USA: 0x8028ab84
 */
extern s32 g_dwOutputBuffReserve;

/** The commands of the display list of the last frame. @ghidraAddress USA: 0x8028ab70 */
extern s32 g_dwDisplayListCommandCount;

/**
 * The most commands of a shadow, vertices of a shadow, and commands of a frame seen.
 *
 * @ghidraAddress USA: 0x8028ab74
 */
extern s32 g_dwPeakShadowCommandCount;

/** @ghidraAddress USA: 0x8028ab78 */
extern s32 g_dwPeakShadowVertexCount;

/** @ghidraAddress USA: 0x8028ab7c */
extern s32 g_dwPeakDisplayListCommands;

/** Whether a frame has been sent to the RSP. @ghidraAddress USA: 0x8028ab80 */
extern s32 g_dwFirstFlushDone;

/**
 * The counts of the CPU counter the end of a frame took, between the ends of two frames, and at
 * the end of the last frame.
 *
 * @ghidraAddress USA: 0x8028aae8
 */
extern u32 g_dwFlushDuration;

/** @ghidraAddress USA: 0x8028aae0 */
extern u32 g_dwFlushInterval;

/** @ghidraAddress USA: 0x8028aae4 */
extern u32 g_dwFlushStartTime;

/**
 * A word FlushDisplayList() copies each frame, and the copy. No other routine reads either.
 *
 * @ghidraAddress USA: 0x8028aa20
 */
extern u32 g_dwFlushValueSource;

/** @ghidraAddress USA: 0x8028aa24 */
extern u32 g_dwLastFlushValue;

/**
 * The frames until the video mode changes to the resolution BeginDisplayList() was given, or 0.
 *
 * @ghidraAddress USA: 0x8028a84c
 */
extern s32 g_dwDisplayModeChangeCountdown;

/** The resolution the video mode was last set to. @ghidraAddress USA: 0x8028a854 */
extern s32 g_dwAppliedScreenScaleShift;

/** The video modes of the high resolution. @ghidraAddress USA: 0x802a5780 */
extern OSViMode g_ViModeNtscHaf1;

/** @ghidraAddress USA: 0x802a6040 */
extern OSViMode g_ViModeMpalHaf1;

/** Whether the video interface corrects the gamma. @ghidraAddress USA: 0x8028aa4c */
extern s32 g_dwViGammaEnabled;

#endif
