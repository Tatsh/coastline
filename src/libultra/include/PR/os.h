#ifndef PR_OS_H
#define PR_OS_H

/**
 * @file
 * Public interface of the libultra operating system, as the game links it. The layouts are those
 * of libultra 2.0 before 2.0L. A thread has no profiling pointer, and its context begins at 0x20.
 */

#include <PR/ultratypes.h>

/* Threads. */
typedef s32 OSPri;
typedef s32 OSId;

typedef union {
    struct {
        f32 f_odd;  /*!< The odd single-precision register of the pair. */
        f32 f_even; /*!< The even single-precision register of the pair. */
    } f;            /*!< The pair as two single-precision registers. */
    f64 d;          /*!< The pair as one double-precision register. */
} __OSfp;

typedef struct {
    u64 at, v0, v1, a0, a1, a2, a3; /*!< The saved assembler temporary, results, and arguments. */
    u64 t0, t1, t2, t3, t4, t5, t6, t7; /*!< The saved temporaries t0 to t7. */
    u64 s0, s1, s2, s3, s4, s5, s6, s7; /*!< The saved callee-saved registers s0 to s7. */
    u64 t8, t9, gp, sp, s8, ra; /*!< The saved t8, t9, global, stack, frame, and return address. */
    u64 lo, hi;                 /*!< The saved multiply and divide result registers. */
    u32 sr, pc, cause, badvaddr, rcp; /*!< The status, PC, cause, bad address, and RCP mask. */
    u32 fpcsr;                        /*!< The saved floating-point control and status register. */
    __OSfp fp0, fp2, fp4, fp6, fp8, fp10, fp12, fp14;      /*!< The saved FPU pairs 0 to 14. */
    __OSfp fp16, fp18, fp20, fp22, fp24, fp26, fp28, fp30; /*!< The saved FPU pairs 16 to 30. */
} __OSThreadContext;

typedef struct OSThread_s {
    struct OSThread_s *next;   /*!< The next thread of the queue the thread waits on. */
    OSPri priority;            /*!< The priority, from OS_PRIORITY_IDLE to OS_PRIORITY_MAX. */
    struct OSThread_s **queue; /*!< The queue the thread is on, or NULL. */
    struct OSThread_s *tlnext; /*!< The next thread of the active queue. */
    u16 state;                 /*!< The run state, one of the OS_STATE_* values. */
    u16 flags;                 /*!< The flags of a CPU break or fault in the thread. */
    OSId id;                   /*!< The identifier osCreateThread() received. */
    int fp;                    /*!< Whether the thread has used the FPU and its context saves it. */
    __OSThreadContext context; /*!< The registers saved while the thread does not run. */
} OSThread;

#define OS_STATE_STOPPED 1
#define OS_STATE_RUNNABLE 2
#define OS_STATE_RUNNING 4
#define OS_STATE_WAITING 8

#define OS_PRIORITY_MAX 255
#define OS_PRIORITY_VIMGR 254
#define OS_PRIORITY_RMON 250
#define OS_PRIORITY_RMONSPIN 200
#define OS_PRIORITY_PIMGR 150
#define OS_PRIORITY_SIMGR 140
#define OS_PRIORITY_APPMAX 127
#define OS_PRIORITY_IDLE 0

/* Messages and events. */
typedef u32 OSEvent;
typedef u32 OSIntMask;
typedef u32 OSPageMask;
typedef void *OSMesg;

typedef struct OSMesgQueue_s {
    OSThread *mtqueue;   /*!< The threads blocked receiving from the empty queue. */
    OSThread *fullqueue; /*!< The threads blocked sending to the full queue. */
    s32 validCount;      /*!< The number of messages in the queue. */
    s32 first;           /*!< The index in msg of the first message. */
    s32 msgCount;        /*!< The capacity of the buffer in messages. */
    OSMesg *msg;         /*!< The circular buffer of messages. */
} OSMesgQueue;

#define OS_MESG_NOBLOCK 0
#define OS_MESG_BLOCK 1

#define OS_EVENT_SW1 0
#define OS_EVENT_SW2 1
#define OS_EVENT_CART 2
#define OS_EVENT_COUNTER 3
#define OS_EVENT_SP 4
#define OS_EVENT_SI 5
#define OS_EVENT_AI 6
#define OS_EVENT_VI 7
#define OS_EVENT_PI 8
#define OS_EVENT_DP 9
#define OS_EVENT_CPU_BREAK 10
#define OS_EVENT_SP_BREAK 11
#define OS_EVENT_FAULT 12
#define OS_EVENT_THREADSTATUS 13
#define OS_EVENT_PRENMI 14

#define MQ_GET_COUNT(mq) ((mq)->validCount)
#define MQ_IS_EMPTY(mq) (MQ_GET_COUNT(mq) == 0)
#define MQ_IS_FULL(mq) (MQ_GET_COUNT(mq) >= (mq)->msgCount)

/* Interrupt masks. */
#define OS_IM_NONE 0x00000001
#define OS_IM_SW1 0x00000501
#define OS_IM_SW2 0x00000601
#define OS_IM_RCP 0x00000401
#define OS_IM_CART 0x00000C01
#define OS_IM_PRENMI 0x00001401
#define OS_IM_RDBWRITE 0x00002401
#define OS_IM_RDBREAD 0x00004401
#define OS_IM_COUNTER 0x00008401
#define OS_IM_CPU 0x0000FF01
#define OS_IM_SP 0x00010401
#define OS_IM_SI 0x00020401
#define OS_IM_AI 0x00040401
#define OS_IM_VI 0x00080401
#define OS_IM_PI 0x00100401
#define OS_IM_DP 0x00200401
#define OS_IM_ALL 0x003FFF01

/* Time. */
typedef u64 OSTime;

typedef struct OSTimer_s {
    struct OSTimer_s *next; /*!< The next timer of the list of active timers. */
    struct OSTimer_s *prev; /*!< The previous timer of the list of active timers. */
    OSTime interval;        /*!< The period of repetition in CPU counter cycles, or 0 for once. */
    OSTime value;    /*!< The cycles remaining after the previous timer of the list expires. */
    OSMesgQueue *mq; /*!< The queue the timer sends to on expiry. */
    OSMesg msg;      /*!< The message the timer sends on expiry. */
} OSTimer;

#define OS_CLOCK_RATE 62500000LL
#define OS_CPU_COUNTER (OS_CLOCK_RATE * 3 / 4)
#define OS_NSEC_TO_CYCLES(n)                                                                       \
    (((u64)(n) * (OS_CPU_COUNTER / 15625000LL)) / (1000000000LL / 15625000LL))
#define OS_USEC_TO_CYCLES(n) (((u64)(n) * (OS_CPU_COUNTER / 15625LL)) / (1000000LL / 15625LL))
#define OS_CYCLES_TO_NSEC(c)                                                                       \
    (((u64)(c) * (1000000000LL / 15625000LL)) / (OS_CPU_COUNTER / 15625000LL))
#define OS_CYCLES_TO_USEC(c) (((u64)(c) * (1000000LL / 15625LL)) / (OS_CPU_COUNTER / 15625LL))

/* Peripheral access. */
typedef struct {
    u32 errStatus;      /*!< The error code of the block transfer, 0 on success. */
    void *dramAddr;     /*!< The RDRAM address of the sector data. */
    void *C2Addr;       /*!< The RDRAM address of the C2 error correction data. */
    u32 sectorSize;     /*!< The size of a sector in bytes. */
    u32 C1ErrNum;       /*!< The number of sectors with a C1 error. */
    u32 C1ErrSector[4]; /*!< The sectors with a C1 error. */
} __OSBlockInfo;

typedef struct {
    u32 cmdType;            /*!< The 64DD command type of the transfer. */
    u16 transferMode;       /*!< The 64DD transfer mode, by sector or by track. */
    u16 blockNum;           /*!< The index in block of the block in transfer. */
    s32 sectorNum;          /*!< The sector the transfer is on, or -1 before the first. */
    u32 devAddr;            /*!< The device address of the transfer. */
    u32 bmCtlShadow;        /*!< The last value written to the 64DD buffer manager control. */
    u32 seqCtlShadow;       /*!< The last value written to the 64DD sequencer control. */
    __OSBlockInfo block[2]; /*!< The state of each block of the transfer. */
} __OSTranxInfo;

typedef struct OSPiHandle_s {
    struct OSPiHandle_s *next;  /*!< The next handle of the list of handles. */
    u8 type;                    /*!< The device type, one of the DEVICE_TYPE_* values. */
    u8 latency;                 /*!< The PI domain latency register value. */
    u8 pageSize;                /*!< The PI domain page size register value. */
    u8 relDuration;             /*!< The PI domain release duration register value. */
    u8 pulse;                   /*!< The PI domain pulse width register value. */
    u8 domain;                  /*!< The PI domain, PI_DOMAIN1 or PI_DOMAIN2. */
    u32 baseAddress;            /*!< The uncached base address of the device. */
    u32 speed;                  /*!< The transfer speed of the device. */
    __OSTranxInfo transferInfo; /*!< The state of the current 64DD transfer. */
} OSPiHandle;

typedef struct {
    u8 type;     /*!< The device type, one of the DEVICE_TYPE_* values. */
    u32 address; /*!< The base address of the device. */
} OSPiInfo;

typedef struct {
    u16 type;              /*!< The request type, one of the OS_MESG_TYPE_* values. */
    u8 pri;                /*!< The priority, OS_MESG_PRI_NORMAL or OS_MESG_PRI_HIGH. */
    u8 status;             /*!< The status of the request. */
    OSMesgQueue *retQueue; /*!< The queue the manager sends the finished request to, or NULL. */
} OSIoMesgHdr;

typedef struct {
    OSIoMesgHdr hdr;      /*!< The header of the request. */
    void *dramAddr;       /*!< The RDRAM address of the transfer. */
    u32 devAddr;          /*!< The device address of the transfer. */
    u32 size;             /*!< The length of the transfer in bytes. */
    OSPiHandle *piHandle; /*!< The handle of the device of an EDMA request. */
} OSIoMesg;

typedef struct {
    s32 active;                        /*!< Whether the manager thread runs. */
    OSThread *thread;                  /*!< The manager thread. */
    OSMesgQueue *cmdQueue;             /*!< The queue of requests to the manager. */
    OSMesgQueue *evtQueue;             /*!< The queue of interrupt events. */
    OSMesgQueue *acsQueue;             /*!< The queue that serialises device access. */
    s32 (*dma)(s32, u32, void *, u32); /*!< The routine that starts a raw DMA. */
    s32 (*edma)(OSPiHandle *, s32, u32, void *, u32); /*!< The routine that starts a handle DMA. */
} OSDevMgr;

#define OS_READ 0
#define OS_WRITE 1
#define OS_OTHERS 2

#define OS_MESG_PRI_NORMAL 0
#define OS_MESG_PRI_HIGH 1

#define OS_MESG_TYPE_BASE 10
#define OS_MESG_TYPE_LOOPBACK (OS_MESG_TYPE_BASE + 0)
#define OS_MESG_TYPE_DMAREAD (OS_MESG_TYPE_BASE + 1)
#define OS_MESG_TYPE_DMAWRITE (OS_MESG_TYPE_BASE + 2)
#define OS_MESG_TYPE_VRETRACE (OS_MESG_TYPE_BASE + 3)
#define OS_MESG_TYPE_COUNTER (OS_MESG_TYPE_BASE + 4)
#define OS_MESG_TYPE_EDMAREAD (OS_MESG_TYPE_BASE + 5)
#define OS_MESG_TYPE_EDMAWRITE (OS_MESG_TYPE_BASE + 6)

#define DEVICE_TYPE_CART 0
#define DEVICE_TYPE_BULK 1
#define DEVICE_TYPE_64DD 2
#define DEVICE_TYPE_SRAM 3
#define DEVICE_TYPE_INIT 7

#define PI_DOMAIN1 0
#define PI_DOMAIN2 1

/* Boot parameters IPL3 writes at the start of RAM. */
extern u32 osTvType;
extern u32 osRomType;
extern u32 osRomBase;
extern u32 osResetType;
extern u32 osCicId;
extern u32 osVersion;
extern u32 osMemSize;
extern u8 osAppNMIBuffer[64];

#define OS_TV_PAL 0
#define OS_TV_NTSC 1
#define OS_TV_MPAL 2

/** The frequency of the video clock in Hz by television standard. osAiSetFrequency() uses it. */
#define VI_NTSC_CLOCK 48681812
#define VI_PAL_CLOCK 49656530
#define VI_MPAL_CLOCK 48628316

/* Video modes. */
typedef struct {
    u32 ctrl;     /*!< The VI control register value. */
    u32 width;    /*!< The VI width register value, the width of the frame buffer in pixels. */
    u32 burst;    /*!< The VI colour burst register value. */
    u32 vSync;    /*!< The VI vertical sync register value. */
    u32 hSync;    /*!< The VI horizontal sync register value. */
    u32 leap;     /*!< The VI horizontal sync leap register value. */
    u32 hStart;   /*!< The VI horizontal video start and end register value. */
    u32 xScale;   /*!< The VI horizontal scale register value. */
    u32 vCurrent; /*!< The VI current line register value. */
} OSViCommonRegs;

typedef struct {
    u32 origin; /*!< The offset of the first displayed line in the frame buffer, in bytes. */
    u32 yScale; /*!< The VI vertical scale register value. */
    u32 vStart; /*!< The VI vertical video start and end register value. */
    u32 vBurst; /*!< The VI vertical burst register value. */
    u32 vIntr;  /*!< The VI interrupt half-line register value. */
} OSViFieldRegs;

typedef struct {
    u8 type;                  /*!< The number of the mode in the table of video modes. */
    OSViCommonRegs comRegs;   /*!< The registers common to both fields. */
    OSViFieldRegs fldRegs[2]; /*!< The registers of each field. */
} OSViMode;

/* Features osViSetSpecialFeatures() turns on and off. */
#define OS_VI_GAMMA_ON 0x0001
#define OS_VI_GAMMA_OFF 0x0002
#define OS_VI_GAMMA_DITHER_ON 0x0004
#define OS_VI_GAMMA_DITHER_OFF 0x0008
#define OS_VI_DIVOT_ON 0x0010
#define OS_VI_DIVOT_OFF 0x0020
#define OS_VI_DITHER_FILTER_ON 0x0040
#define OS_VI_DITHER_FILTER_OFF 0x0080

/** @ghidraAddress USA: 0x802a62f0 */
extern OSViMode osViModePalLan1;

/** @ghidraAddress USA: 0x802a6340 */
extern OSViMode osViModeMpalLan1;

/** @ghidraAddress USA: 0x802a6390 */
extern OSViMode osViModeNtscLan1;

/**
 * The rate of the CPU counter, OS_CPU_COUNTER until osInitialize() sets it.
 *
 * @ghidraAddress USA: 0x802a6140
 */
extern u64 osClockRate;

/**
 * Create a stopped thread on the active queue. It runs entry(arg) on the stack that ends at sp,
 * and destroys itself when entry returns.
 *
 * @ghidraAddress USA: 0x80265670
 */
void osCreateThread(OSThread *t, OSId id, void (*entry)(void *), void *arg, void *sp, OSPri pri);

/**
 * Remove a thread from its queue and from the active queue. A NULL thread is the running one,
 * and destroying the running thread dispatches the next.
 *
 * @ghidraAddress USA: 0x8026e500
 */
void osDestroyThread(OSThread *t);

/**
 * Put the running thread at the end of the threads of its priority and dispatch.
 *
 * @ghidraAddress USA: 0x8026e820
 */
void osYieldThread(void);

/**
 * Make a stopped or waiting thread runnable, and yield to it when its priority is higher.
 *
 * @ghidraAddress USA: 0x802657c0
 */
void osStartThread(OSThread *t);

/**
 * Change the priority of a thread, a NULL thread being the running one, and yield when a
 * runnable thread now has a higher priority than the running one.
 *
 * @ghidraAddress USA: 0x802668f0
 */
void osSetThreadPri(OSThread *t, OSPri pri);

/**
 * Return the priority of a thread, a NULL thread being the running one.
 *
 * @ghidraAddress USA: 0x8026c040
 */
OSPri osGetThreadPri(OSThread *t);

/**
 * Initialise an empty message queue over a buffer of count messages.
 *
 * @ghidraAddress USA: 0x80265410
 */
void osCreateMesgQueue(OSMesgQueue *mq, OSMesg *msg, s32 count);

/**
 * Append a message to a queue and make its first receiver runnable.
 *
 * @return 0, or -1 when the queue is full and flag is OS_MESG_NOBLOCK.
 * @ghidraAddress USA: 0x802654b0
 */
s32 osSendMesg(OSMesgQueue *mq, OSMesg msg, s32 flag);

/**
 * Insert a message at the front of a queue and make its first receiver runnable.
 *
 * @return 0, or -1 when the queue is full and flag is OS_MESG_NOBLOCK.
 * @ghidraAddress USA: 0x8026b5d0
 */
s32 osJamMesg(OSMesgQueue *mq, OSMesg msg, s32 flag);

/**
 * Take the first message of a queue, store it unless msg is NULL, and make its first sender
 * runnable.
 *
 * @return 0, or -1 when the queue is empty and flag is OS_MESG_NOBLOCK.
 * @ghidraAddress USA: 0x802642e0
 */
s32 osRecvMesg(OSMesgQueue *mq, OSMesg *msg, s32 flag);

/**
 * Register the queue and message that event e sends.
 *
 * @ghidraAddress USA: 0x80265440
 */
void osSetEventMesg(OSEvent e, OSMesgQueue *mq, OSMesg m);

/* Interrupts and the processor. */
OSIntMask osGetIntMask(void);

/**
 * Set the CPU and RCP interrupts that may occur, within the global mask.
 *
 * @return The previous mask.
 * @ghidraAddress USA: 0x8026e460
 */
OSIntMask osSetIntMask(OSIntMask im);

/**
 * Read the Count register, which increments at half the CPU clock.
 *
 * @ghidraAddress USA: 0x802649f0
 */
u32 osGetCount(void);

/**
 * Invalidate the data cache over a range.
 *
 * @ghidraAddress USA: 0x802662e0
 */
void osInvalDCache(void *vaddr, s32 nbytes);

/**
 * Invalidate the instruction cache over a range.
 *
 * @ghidraAddress USA: 0x8026bac0
 */
void osInvalICache(void *vaddr, s32 nbytes);

/**
 * Write back the data cache over a range.
 *
 * @ghidraAddress USA: 0x8026b7d0
 */
void osWritebackDCache(void *vaddr, s32 nbytes);

/**
 * Write back the whole data cache.
 *
 * @ghidraAddress USA: 0x802649c0
 */
void osWritebackDCacheAll(void);

/**
 * Translate a virtual address to a physical one.
 *
 * @ghidraAddress USA: 0x8026b750
 */
u32 osVirtualToPhysical(void *vaddr);

/**
 * Map the RDB port of the development hardware.
 *
 * @ghidraAddress USA: 0x8026bb40
 */
void osMapTLBRdb(void);

/**
 * Return the time since the VI manager started, in CPU counter cycles.
 *
 * @ghidraAddress USA: 0x8026b960
 */
OSTime osGetTime(void);

/**
 * Start a timer that sends msg to mq after countdown cycles, or after interval cycles when
 * countdown is 0, and again every interval cycles unless interval is 0.
 *
 * @return 0.
 * @ghidraAddress USA: 0x80268390
 */
int osSetTimer(OSTimer *t, OSTime countdown, OSTime interval, OSMesgQueue *mq, OSMesg msg);

/* Video. */

/**
 * Start the VI manager thread at priority pri. The call raises the calling thread to pri while
 * it creates the manager, and does nothing once the manager runs.
 *
 * @ghidraAddress USA: 0x80266400
 */
void osCreateViManager(OSPri pri);

/**
 * Blank the screen from the next field on, or show it again.
 *
 * @ghidraAddress USA: 0x80260ab0
 */
void osViBlack(u8 active);

/**
 * Select the video mode from the next field on.
 *
 * @ghidraAddress USA: 0x80264a60
 */
void osViSetMode(OSViMode *modep);

/**
 * Show a frame buffer from the next field on.
 *
 * @ghidraAddress USA: 0x80264ad0
 */
void osViSwapBuffer(void *frameBufPtr);

/**
 * Return the frame buffer the screen shows.
 *
 * @ghidraAddress USA: 0x80264b20
 */
void *osViGetCurrentFramebuffer(void);

/**
 * Send m to mq every retraceCount vertical retraces.
 *
 * @ghidraAddress USA: 0x80265600
 */
void osViSetEvent(OSMesgQueue *mq, OSMesg m, u32 retraceCount);

/**
 * Turn the gamma, gamma dither, divot, and dither filter features on and off, from the
 * OS_VI_* flags.
 *
 * @ghidraAddress USA: 0x80264490
 */
void osViSetSpecialFeatures(u32 func);

/* Peripheral access. */

/**
 * Return the handle of the cartridge ROM, creating it from the timing its first word records.
 *
 * @ghidraAddress USA: 0x8026c3d0
 */
OSPiHandle *osCartRomInit(void);

/**
 * Create the handle of the 64DD and select its domain 2 timing.
 *
 * @ghidraAddress USA: 0x8026c4c0
 */
OSPiHandle *osLeoDiskInit(void);

/**
 * Start the PI manager thread at priority pri, serving the requests of cmdQ. The call does
 * nothing once the manager runs.
 *
 * @ghidraAddress USA: 0x80266760
 */
void osCreatePiManager(OSPri pri, OSMesgQueue *cmdQ, OSMesg *cmdBuf, s32 cmdMsgCnt);

/**
 * Return the command queue of the PI manager, or NULL when it does not run.
 *
 * @ghidraAddress USA: 0x8026b720
 */
OSMesgQueue *osPiGetCmdQueue(void);

/**
 * Read a word of the cartridge domain at an offset from the ROM base.
 *
 * @return 0.
 * @ghidraAddress USA: 0x802663a0
 */
s32 osPiRawReadIo(u32 devAddr, u32 *data);

/**
 * Start a DMA between RDRAM and the cartridge domain at an offset from the ROM base.
 *
 * @return 0, or -1 for a direction other than OS_READ and OS_WRITE.
 * @ghidraAddress USA: 0x8026c680
 */
s32 osPiRawStartDma(s32 direction, u32 devAddr, void *dramAddr, u32 size);

/**
 * Queue a DMA request with the PI manager. A request of priority OS_MESG_PRI_HIGH goes to the
 * front of the queue.
 *
 * @return The result of the send, or -1 when the PI manager does not run.
 * @ghidraAddress USA: 0x80264650
 */
s32 osPiStartDma(OSIoMesg *mb,
                 s32 priority,
                 s32 direction,
                 u32 devAddr,
                 void *vAddr,
                 u32 nbytes,
                 OSMesgQueue *mq);

/**
 * Read a word of the device a handle describes.
 *
 * @return 0.
 * @ghidraAddress USA: 0x8026e780
 */
s32 osEPiRawReadIo(OSPiHandle *pihandle, u32 devAddr, u32 *data);

/**
 * Write a word of the device a handle describes.
 *
 * @return 0.
 * @ghidraAddress USA: 0x8026e730
 */
s32 osEPiRawWriteIo(OSPiHandle *pihandle, u32 devAddr, u32 data);

/**
 * Start a DMA with the device a handle describes, selecting its domain timing when another
 * handle selected different timing.
 *
 * @return 0, or -1 for a direction other than OS_READ and OS_WRITE.
 * @ghidraAddress USA: 0x8026c760
 */
s32 osEPiRawStartDma(OSPiHandle *pihandle, s32 direction, u32 devAddr, void *dramAddr, u32 size);

/* Audio. */

/**
 * Set the sample rate of the audio DAC to the nearest the video clock allows.
 *
 * @return The rate set, or -1 when the frequency is too high.
 * @ghidraAddress USA: 0x80268230
 */
s32 osAiSetFrequency(u32 frequency);

/**
 * Queue a buffer of samples for the audio DMA.
 *
 * @return 0, or -1 when the queue of the audio interface is full.
 * @ghidraAddress USA: 0x80268470
 */
s32 osAiSetNextBuffer(void *bufPtr, u32 size);

/**
 * Return the audio status register.
 *
 * @ghidraAddress USA: 0x80268520
 */
u32 osAiGetStatus(void);

/**
 * Return the remaining length of the buffer the audio DMA plays.
 *
 * @ghidraAddress USA: 0x80268530
 */
u32 osAiGetLength(void);

/**
 * Return whether the queue of the audio interface is full.
 *
 * @ghidraAddress USA: 0x8026ce20
 */
s32 __osAiDeviceBusy(void);

/**
 * The frequency of the video clock. osInitialize() selects it by television standard.
 *
 * @ghidraAddress USA: 0x802a6148
 */
extern s32 osViClock;

/**
 * Whether the last buffer osAiSetNextBuffer() queued ended 0x2000 bytes into a 16 KiB page, a
 * case the audio DMA mishandles.
 *
 * @ghidraAddress USA: 0x802a61b0
 */
extern u8 aiHdwrBugFlag;

/**
 * Initialise the system: the FPU, the PIF, the exception vectors and the clock rates.
 *
 * @ghidraAddress USA: 0x80265d90
 */
void osInitialize(void);

/**
 * Print to the debugger. The retail library discards the output.
 *
 * @ghidraAddress USA: 0x802607dc
 */
void osSyncPrintf(const char *fmt, ...);

/**
 * Print to the remote monitor. The retail library discards the output.
 *
 * @ghidraAddress USA: 0x802607c0
 */
void rmonPrintf(const char *fmt, ...);

/* The 64-bit helpers the compiler calls for long long arithmetic and conversions. */
unsigned long long __ull_rshift(unsigned long long a0, unsigned long long a1);
unsigned long long __ull_rem(unsigned long long a0, unsigned long long a1);
unsigned long long __ull_div(unsigned long long a0, unsigned long long a1);
long long __ll_lshift(long long a0, long long a1);
long long __ll_rem(unsigned long long a0, long long a1);
long long __ll_div(long long a0, long long a1);
long long __ll_mul(long long a0, long long a1);
void __ull_divremi(unsigned long long *quotient,
                   unsigned long long *remainder,
                   unsigned long long dividend,
                   short divisor);
long long __ll_mod(long long a0, long long a1);
long long __ll_rshift(long long a0, long long a1);
long long __d_to_ll(double d);
long long __f_to_ll(float f);
unsigned long long __d_to_ull(double d);
unsigned long long __f_to_ull(float f);
double __ll_to_d(long long s);
float __ll_to_f(long long s);
double __ull_to_d(unsigned long long u);
float __ull_to_f(unsigned long long u);

#endif
