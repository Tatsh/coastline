#ifndef PR_OS_H
#define PR_OS_H

/**
 * @file
 * Public interface of the libultra operating system, as the game links it. The layouts are those
 * of libultra 2.0 before 2.0L. A thread has no profiling pointer, and its context begins at 0x20.
 */

#include <PR/ultratypes.h>

/* Threads. */
typedef s32 OSPri; /*!< The priority of a thread, from OS_PRIORITY_IDLE to OS_PRIORITY_MAX. */
typedef s32 OSId;  /*!< The number that identifies a thread. */

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

/** A thread: its place in the queues, its state, and the registers saved when it last stopped. */
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

/* The states of a thread. */
#define OS_STATE_STOPPED 1  /*!< The thread is stopped. */
#define OS_STATE_RUNNABLE 2 /*!< The thread waits to run. */
#define OS_STATE_RUNNING 4  /*!< The thread runs. */
#define OS_STATE_WAITING 8  /*!< The thread waits on a message queue. */

/* The priorities of threads. */
#define OS_PRIORITY_MAX 255      /*!< The highest priority. */
#define OS_PRIORITY_VIMGR 254    /*!< The priority of the VI manager. */
#define OS_PRIORITY_RMON 250     /*!< The priority of the remote monitor. */
#define OS_PRIORITY_RMONSPIN 200 /*!< The priority of the remote monitor while it spins. */
#define OS_PRIORITY_PIMGR 150    /*!< The priority of the PI manager. */
#define OS_PRIORITY_SIMGR 140    /*!< The priority of the SI manager. */
#define OS_PRIORITY_APPMAX 127   /*!< The highest priority of a thread of the game. */
#define OS_PRIORITY_IDLE 0       /*!< The priority of the idle thread. */

/* Messages and events. */
typedef u32 OSEvent;    /*!< A system event, one of the OS_EVENT_* values. */
typedef u32 OSIntMask;  /*!< A mask of interrupts, a combination of the OS_IM_* values. */
typedef u32 OSPageMask; /*!< A mask of the page size of a TLB entry. */
typedef void *OSMesg;   /*!< A message, a word sent through a message queue. */

/** A message queue: a ring of messages and the threads blocked sending or receiving. */
typedef struct OSMesgQueue_s {
    OSThread *mtqueue;   /*!< The threads blocked receiving from the empty queue. */
    OSThread *fullqueue; /*!< The threads blocked sending to the full queue. */
    s32 validCount;      /*!< The number of messages in the queue. */
    s32 first;           /*!< The index in msg of the first message. */
    s32 msgCount;        /*!< The capacity of the buffer in messages. */
    OSMesg *msg;         /*!< The circular buffer of messages. */
} OSMesgQueue;

/* Whether a send or receive waits. */
#define OS_MESG_NOBLOCK 0 /*!< Return at once when the queue is full or empty. */
#define OS_MESG_BLOCK 1   /*!< Wait until the queue has room or a message. */

/* The system events. */
#define OS_EVENT_SW1 0           /*!< Software interrupt 1. */
#define OS_EVENT_SW2 1           /*!< Software interrupt 2. */
#define OS_EVENT_CART 2          /*!< The cartridge interrupt. */
#define OS_EVENT_COUNTER 3       /*!< The CPU counter reached the compare register. */
#define OS_EVENT_SP 4            /*!< The RSP finished. */
#define OS_EVENT_SI 5            /*!< The serial interface finished. */
#define OS_EVENT_AI 6            /*!< The audio interface needs a buffer. */
#define OS_EVENT_VI 7            /*!< The video interface reached its interrupt line. */
#define OS_EVENT_PI 8            /*!< The peripheral interface finished. */
#define OS_EVENT_DP 9            /*!< The RDP finished. */
#define OS_EVENT_CPU_BREAK 10    /*!< The CPU hit a breakpoint. */
#define OS_EVENT_SP_BREAK 11     /*!< The RSP hit a breakpoint. */
#define OS_EVENT_FAULT 12        /*!< A thread faulted. */
#define OS_EVENT_THREADSTATUS 13 /*!< The status of a thread changed. */
#define OS_EVENT_PRENMI 14       /*!< The reset button was pressed. */

/** The number of messages in a queue. */
#define MQ_GET_COUNT(mq) ((mq)->validCount)
/** Whether a queue has no messages. */
#define MQ_IS_EMPTY(mq) (MQ_GET_COUNT(mq) == 0)
/** Whether a queue has no room for another message. */
#define MQ_IS_FULL(mq) (MQ_GET_COUNT(mq) >= (mq)->msgCount)

/* Interrupt masks. */
#define OS_IM_NONE 0x00000001     /*!< No interrupts. */
#define OS_IM_SW1 0x00000501      /*!< Software interrupt 1. */
#define OS_IM_SW2 0x00000601      /*!< Software interrupt 2. */
#define OS_IM_RCP 0x00000401      /*!< The interrupts of the RCP. */
#define OS_IM_CART 0x00000C01     /*!< The cartridge interrupt. */
#define OS_IM_PRENMI 0x00001401   /*!< The reset button. */
#define OS_IM_RDBWRITE 0x00002401 /*!< A write of the debugger port. */
#define OS_IM_RDBREAD 0x00004401  /*!< A read of the debugger port. */
#define OS_IM_COUNTER 0x00008401  /*!< The CPU counter. */
#define OS_IM_CPU 0x0000FF01      /*!< Every interrupt of the CPU. */
#define OS_IM_SP 0x00010401       /*!< The RSP. */
#define OS_IM_SI 0x00020401       /*!< The serial interface. */
#define OS_IM_AI 0x00040401       /*!< The audio interface. */
#define OS_IM_VI 0x00080401       /*!< The video interface. */
#define OS_IM_PI 0x00100401       /*!< The peripheral interface. */
#define OS_IM_DP 0x00200401       /*!< The RDP. */
#define OS_IM_ALL 0x003FFF01      /*!< Every interrupt. */

/* Time. */
typedef u64 OSTime; /*!< A time in cycles of the CPU counter. */

/** A timer that sends a message when it expires, once or periodically. */
typedef struct OSTimer_s {
    struct OSTimer_s *next; /*!< The next timer of the list of active timers. */
    struct OSTimer_s *prev; /*!< The previous timer of the list of active timers. */
    OSTime interval;        /*!< The period of repetition in CPU counter cycles, or 0 for once. */
    OSTime value;    /*!< The cycles remaining after the previous timer of the list expires. */
    OSMesgQueue *mq; /*!< The queue the timer sends to on expiry. */
    OSMesg msg;      /*!< The message the timer sends on expiry. */
} OSTimer;

/** The clock rate of the RCP, in hertz. */
#define OS_CLOCK_RATE 62500000LL
/** The rate of the CPU counter, in cycles a second. */
#define OS_CPU_COUNTER (OS_CLOCK_RATE * 3 / 4)
/** Convert nanoseconds to cycles of the CPU counter. */
#define OS_NSEC_TO_CYCLES(n)                                                                       \
    (((u64)(n) * (OS_CPU_COUNTER / 15625000LL)) / (1000000000LL / 15625000LL))
/** Convert microseconds to cycles of the CPU counter. */
#define OS_USEC_TO_CYCLES(n) (((u64)(n) * (OS_CPU_COUNTER / 15625LL)) / (1000000LL / 15625LL))
/** Convert cycles of the CPU counter to nanoseconds. */
#define OS_CYCLES_TO_NSEC(c)                                                                       \
    (((u64)(c) * (1000000000LL / 15625000LL)) / (OS_CPU_COUNTER / 15625000LL))
/** Convert cycles of the CPU counter to microseconds. */
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

/** A device on the peripheral interface, with the timing of its PI domain. */
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

/** The type and base address of a device on the peripheral interface. */
typedef struct {
    u8 type;     /*!< The device type, one of the DEVICE_TYPE_* values. */
    u32 address; /*!< The base address of the device. */
} OSPiInfo;

/** The header of an I/O request to a device manager. */
typedef struct {
    u16 type;              /*!< The request type, one of the OS_MESG_TYPE_* values. */
    u8 pri;                /*!< The priority, OS_MESG_PRI_NORMAL or OS_MESG_PRI_HIGH. */
    u8 status;             /*!< The status of the request. */
    OSMesgQueue *retQueue; /*!< The queue the manager sends the finished request to, or NULL. */
} OSIoMesgHdr;

/** An I/O request to a device manager: a DMA between RDRAM and a device. */
typedef struct {
    OSIoMesgHdr hdr;      /*!< The header of the request. */
    void *dramAddr;       /*!< The RDRAM address of the transfer. */
    u32 devAddr;          /*!< The device address of the transfer. */
    u32 size;             /*!< The length of the transfer in bytes. */
    OSPiHandle *piHandle; /*!< The handle of the device of an EDMA request. */
} OSIoMesg;

/** A device manager: the thread and queues that serve the I/O requests of one interface. */
typedef struct {
    s32 active;                        /*!< Whether the manager thread runs. */
    OSThread *thread;                  /*!< The manager thread. */
    OSMesgQueue *cmdQueue;             /*!< The queue of requests to the manager. */
    OSMesgQueue *evtQueue;             /*!< The queue of interrupt events. */
    OSMesgQueue *acsQueue;             /*!< The queue that serialises device access. */
    s32 (*dma)(s32, u32, void *, u32); /*!< The routine that starts a raw DMA. */
    s32 (*edma)(OSPiHandle *, s32, u32, void *, u32); /*!< The routine that starts a handle DMA. */
} OSDevMgr;

/* Directions of a transfer. */
#define OS_READ 0   /*!< From the device to memory. */
#define OS_WRITE 1  /*!< From memory to the device. */
#define OS_OTHERS 2 /*!< Neither, for a request that moves no data. */

/* Priorities of an I/O request. */
#define OS_MESG_PRI_NORMAL 0 /*!< The request joins the end of the queue. */
#define OS_MESG_PRI_HIGH 1   /*!< The request jumps to the front of the queue. */

/* Types of an I/O request. */
#define OS_MESG_TYPE_BASE 10                           /*!< The first type. */
#define OS_MESG_TYPE_LOOPBACK (OS_MESG_TYPE_BASE + 0)  /*!< A request sent back unchanged. */
#define OS_MESG_TYPE_DMAREAD (OS_MESG_TYPE_BASE + 1)   /*!< A DMA from the cartridge. */
#define OS_MESG_TYPE_DMAWRITE (OS_MESG_TYPE_BASE + 2)  /*!< A DMA to the cartridge. */
#define OS_MESG_TYPE_VRETRACE (OS_MESG_TYPE_BASE + 3)  /*!< A vertical retrace. */
#define OS_MESG_TYPE_COUNTER (OS_MESG_TYPE_BASE + 4)   /*!< An expiry of the counter. */
#define OS_MESG_TYPE_EDMAREAD (OS_MESG_TYPE_BASE + 5)  /*!< A DMA from a device handle. */
#define OS_MESG_TYPE_EDMAWRITE (OS_MESG_TYPE_BASE + 6) /*!< A DMA to a device handle. */

/* Types of a device on the peripheral interface. */
#define DEVICE_TYPE_CART 0 /*!< The cartridge ROM. */
#define DEVICE_TYPE_BULK 1 /*!< Bulk storage. */
#define DEVICE_TYPE_64DD 2 /*!< The 64DD. */
#define DEVICE_TYPE_SRAM 3 /*!< Battery-backed SRAM. */
#define DEVICE_TYPE_INIT 7 /*!< A device not yet probed. */

/* The domains of the peripheral interface. */
#define PI_DOMAIN1 0 /*!< Domain 1, the cartridge ROM. */
#define PI_DOMAIN2 1 /*!< Domain 2, SRAM and the 64DD. */

/* Boot parameters IPL3 writes at the start of RAM. */
extern u32 osTvType;
extern u32 osRomType;
extern u32 osRomBase;
extern u32 osResetType;
extern u32 osCicId;
extern u32 osVersion;
extern u32 osMemSize;
extern u8 osAppNMIBuffer[64];

/* The television standards of osTvType. */
#define OS_TV_PAL 0  /*!< PAL. */
#define OS_TV_NTSC 1 /*!< NTSC. */
#define OS_TV_MPAL 2 /*!< PAL-M. */

/* The frequency of the video clock by television standard. osAiSetFrequency() uses it. */
#define VI_NTSC_CLOCK 48681812 /*!< The video clock of NTSC, in hertz. */
#define VI_PAL_CLOCK 49656530  /*!< The video clock of PAL, in hertz. */
#define VI_MPAL_CLOCK 48628316 /*!< The video clock of PAL-M, in hertz. */

/* Video modes. */
/** The video interface registers a video mode sets for both fields. */
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

/** The video interface registers a video mode sets for one field. */
typedef struct {
    u32 origin; /*!< The offset of the first displayed line in the frame buffer, in bytes. */
    u32 yScale; /*!< The VI vertical scale register value. */
    u32 vStart; /*!< The VI vertical video start and end register value. */
    u32 vBurst; /*!< The VI vertical burst register value. */
    u32 vIntr;  /*!< The VI interrupt half-line register value. */
} OSViFieldRegs;

/** A video mode: the registers of the video interface for a resolution and television standard. */
typedef struct {
    u8 type;                  /*!< The number of the mode in the table of video modes. */
    OSViCommonRegs comRegs;   /*!< The registers common to both fields. */
    OSViFieldRegs fldRegs[2]; /*!< The registers of each field. */
} OSViMode;

/* Features osViSetSpecialFeatures() turns on and off. */
#define OS_VI_GAMMA_ON 0x0001          /*!< Turn the gamma correction on. */
#define OS_VI_GAMMA_OFF 0x0002         /*!< Turn the gamma correction off. */
#define OS_VI_GAMMA_DITHER_ON 0x0004   /*!< Turn the gamma dither on. */
#define OS_VI_GAMMA_DITHER_OFF 0x0008  /*!< Turn the gamma dither off. */
#define OS_VI_DIVOT_ON 0x0010          /*!< Turn the divot removal on. */
#define OS_VI_DIVOT_OFF 0x0020         /*!< Turn the divot removal off. */
#define OS_VI_DITHER_FILTER_ON 0x0040  /*!< Turn the dither filter on. */
#define OS_VI_DITHER_FILTER_OFF 0x0080 /*!< Turn the dither filter off. */

/**
 * @ghidraAddress USA: 0x802a62f0
 * @ghidraAddress EUR: 0x802a7bf0
 * @ghidraAddress JPN: 0x802a7970
 */
extern OSViMode osViModePalLan1;

#if TGR_REGION == TGR_REGION_EU
/** @ghidraAddress EUR: 0x802a7ba0 */
extern OSViMode osViModePalHaf1;
#endif

/**
 * @ghidraAddress USA: 0x802a6340
 * @ghidraAddress EUR: 0x802a7c90
 * @ghidraAddress JPN: 0x802a76e0
 */
extern OSViMode osViModeMpalLan1;

/**
 * @ghidraAddress USA: 0x802a6390
 * @ghidraAddress EUR: 0x802a7e80
 * @ghidraAddress JPN: 0x802a7730
 */
extern OSViMode osViModeNtscLan1;

/**
 * The rate of the CPU counter, OS_CPU_COUNTER until osInitialize() sets it.
 *
 * @ghidraAddress USA: 0x802a6140
 * @ghidraAddress EUR: 0x802a7c50
 * @ghidraAddress JPN: 0x802a7790
 */
extern u64 osClockRate;

/**
 * Create a stopped thread on the active queue. It runs entry(arg) on the stack that ends at sp,
 * and destroys itself when entry returns.
 *
 * @param t The thread to create.
 * @param id The identifier of the thread, for the debugger.
 * @param entry The routine the thread runs.
 * @param arg The argument passed to entry.
 * @param sp The end of the stack of the thread, aligned to 8 bytes.
 * @param pri The priority, from OS_PRIORITY_IDLE to OS_PRIORITY_MAX.
 * @ghidraAddress USA: 0x80265670
 * @ghidraAddress EUR: 0x80266e00
 * @ghidraAddress JPN: 0x80266900
 */
void osCreateThread(OSThread *t, OSId id, void (*entry)(void *), void *arg, void *sp, OSPri pri);

/**
 * Remove a thread from its queue and from the active queue. A NULL thread is the running one,
 * and destroying the running thread dispatches the next.
 *
 * @param t The thread to destroy, or NULL for the running thread.
 * @ghidraAddress USA: 0x8026e500
 * @ghidraAddress EUR: 0x802704d0
 * @ghidraAddress JPN: 0x8026ff80
 */
void osDestroyThread(OSThread *t);

/**
 * Put the running thread at the end of the threads of its priority and dispatch.
 *
 * @ghidraAddress USA: 0x8026e820
 * @ghidraAddress EUR: 0x802707f0
 * @ghidraAddress JPN: 0x802702a0
 */
void osYieldThread(void);

/**
 * Make a stopped or waiting thread runnable, and yield to it when its priority is higher.
 *
 * @param t The thread to start.
 * @ghidraAddress USA: 0x802657c0
 * @ghidraAddress EUR: 0x80266f50
 * @ghidraAddress JPN: 0x80266a50
 */
void osStartThread(OSThread *t);

/**
 * Change the priority of a thread, a NULL thread being the running one, and yield when a
 * runnable thread now has a higher priority than the running one.
 *
 * @param t The thread, or NULL for the running thread.
 * @param pri The new priority, from OS_PRIORITY_IDLE to OS_PRIORITY_MAX.
 * @ghidraAddress USA: 0x802668f0
 * @ghidraAddress EUR: 0x802680e0
 * @ghidraAddress JPN: 0x80267b80
 */
void osSetThreadPri(OSThread *t, OSPri pri);

/**
 * Return the priority of a thread, a NULL thread being the running one.
 *
 * @param t The thread, or NULL for the running thread.
 * @return The priority of the thread.
 * @ghidraAddress USA: 0x8026c040
 * @ghidraAddress EUR: 0x8026e010
 * @ghidraAddress JPN: 0x8026dac0
 */
OSPri osGetThreadPri(OSThread *t);

/**
 * Initialise an empty message queue over a buffer of count messages.
 *
 * @param mq The queue to initialise.
 * @param msg The buffer of the messages.
 * @param count The number of messages the buffer has room for.
 * @ghidraAddress USA: 0x80265410
 * @ghidraAddress EUR: 0x80266ba0
 * @ghidraAddress JPN: 0x802666a0
 */
void osCreateMesgQueue(OSMesgQueue *mq, OSMesg *msg, s32 count);

/**
 * Append a message to a queue and make its first receiver runnable.
 *
 * @param mq The queue.
 * @param msg The message.
 * @param flag OS_MESG_BLOCK to wait while the queue is full, or OS_MESG_NOBLOCK to return.
 * @return 0, or -1 when the queue is full and flag is OS_MESG_NOBLOCK.
 * @ghidraAddress USA: 0x802654b0
 * @ghidraAddress EUR: 0x80266c40
 * @ghidraAddress JPN: 0x80266740
 */
s32 osSendMesg(OSMesgQueue *mq, OSMesg msg, s32 flag);

/**
 * Insert a message at the front of a queue and make its first receiver runnable.
 *
 * @param mq The queue.
 * @param msg The message.
 * @param flag OS_MESG_BLOCK to wait while the queue is full, or OS_MESG_NOBLOCK to return.
 * @return 0, or -1 when the queue is full and flag is OS_MESG_NOBLOCK.
 * @ghidraAddress USA: 0x8026b5d0
 * @ghidraAddress EUR: 0x8026d5a0
 * @ghidraAddress JPN: 0x8026d050
 */
s32 osJamMesg(OSMesgQueue *mq, OSMesg msg, s32 flag);

/**
 * Take the first message of a queue, store it unless msg is NULL, and make its first sender
 * runnable.
 *
 * @param mq The queue.
 * @param msg Receives the message, unless it is NULL.
 * @param flag OS_MESG_BLOCK to wait while the queue is empty, or OS_MESG_NOBLOCK to return.
 * @return 0, or -1 when the queue is empty and flag is OS_MESG_NOBLOCK.
 * @ghidraAddress USA: 0x802642e0
 * @ghidraAddress EUR: 0x80265a60
 * @ghidraAddress JPN: 0x80265500
 */
s32 osRecvMesg(OSMesgQueue *mq, OSMesg *msg, s32 flag);

/**
 * Register the queue and message that event e sends.
 *
 * @param e The event, such as OS_EVENT_SI or OS_EVENT_PI.
 * @param mq The queue the event sends to.
 * @param m The message the event sends.
 * @ghidraAddress USA: 0x80265440
 * @ghidraAddress EUR: 0x80266bd0
 * @ghidraAddress JPN: 0x802666d0
 */
void osSetEventMesg(OSEvent e, OSMesgQueue *mq, OSMesg m);

/* Interrupts and the processor. */

/**
 * Return the CPU and RCP interrupts that may occur. The game does not link this routine.
 *
 * @return The interrupt mask.
 */
OSIntMask osGetIntMask(void);

/**
 * Set the CPU and RCP interrupts that may occur, within the global mask.
 *
 * @param im The new interrupt mask, from the OS_IM_* bits.
 * @return The previous mask.
 * @ghidraAddress USA: 0x8026e460
 * @ghidraAddress EUR: 0x80270430
 * @ghidraAddress JPN: 0x8026fee0
 */
OSIntMask osSetIntMask(OSIntMask im);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read the CPU counter, or advance the emulated counter on Windows.
 *
 * @return The counter value.
 * @ghidraAddress USA: 0x802649f0
 * @ghidraAddress EUR: 0x80266250
 * @ghidraAddress JPN: 0x80265c80
 * @ghidraAddress BRD3D: 0x10060e90
 */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
u64 osGetCount(void);
#else
u32 osGetCount(void);
#endif

#ifdef __cplusplus
}
#endif

/**
 * Invalidate the data cache over a range.
 *
 * @param vaddr The start of the range, a virtual address.
 * @param nbytes The length of the range in bytes. A length of 0 or less does nothing.
 * @ghidraAddress USA: 0x802662e0
 * @ghidraAddress EUR: 0x80267a70
 * @ghidraAddress JPN: 0x80267570
 */
void osInvalDCache(void *vaddr, s32 nbytes);

/**
 * Invalidate the instruction cache over a range.
 *
 * @param vaddr The start of the range, a virtual address.
 * @param nbytes The length of the range in bytes. A length of 0 or less does nothing.
 * @ghidraAddress USA: 0x8026bac0
 * @ghidraAddress EUR: 0x8026da90
 * @ghidraAddress JPN: 0x8026d540
 */
void osInvalICache(void *vaddr, s32 nbytes);

/**
 * Write back the data cache over a range.
 *
 * @param vaddr The start of the range, a virtual address.
 * @param nbytes The length of the range in bytes. A length of 0 or less does nothing.
 * @ghidraAddress USA: 0x8026b7d0
 * @ghidraAddress EUR: 0x8026d7a0
 * @ghidraAddress JPN: 0x8026d250
 */
void osWritebackDCache(void *vaddr, s32 nbytes);

/**
 * Write back the whole data cache.
 *
 * @ghidraAddress USA: 0x802649c0
 * @ghidraAddress EUR: 0x80266220
 * @ghidraAddress JPN: 0x80265c50
 */
void osWritebackDCacheAll(void);

/**
 * Translate a virtual address to a physical one.
 *
 * @param vaddr The virtual address, in KSEG0, KSEG1, or a segment the TLB maps.
 * @return The physical address, or -1 when the TLB does not map the address.
 * @ghidraAddress USA: 0x8026b750
 * @ghidraAddress EUR: 0x8026d720
 * @ghidraAddress JPN: 0x8026d1d0
 */
u32 osVirtualToPhysical(void *vaddr);

/**
 * Map the RDB port of the development hardware.
 *
 * @ghidraAddress USA: 0x8026bb40
 * @ghidraAddress EUR: 0x8026db10
 * @ghidraAddress JPN: 0x8026d5c0
 */
void osMapTLBRdb(void);

/**
 * Return the time since the VI manager started, in CPU counter cycles.
 *
 * @return The time in CPU counter cycles, OS_CPU_COUNTER of them per second.
 * @ghidraAddress USA: 0x8026b960
 * @ghidraAddress EUR: 0x8026d930
 * @ghidraAddress JPN: 0x8026d3e0
 */
OSTime osGetTime(void);

/**
 * Start a timer that sends msg to mq after countdown cycles, or after interval cycles when
 * countdown is 0, and again every interval cycles unless interval is 0.
 *
 * @param t The timer, not reused until it expires for the last time.
 * @param countdown The cycles of the CPU counter until the first message.
 * @param interval The cycles between later messages, or 0 for a single message.
 * @param mq The queue the timer sends to.
 * @param msg The message the timer sends.
 * @return 0.
 * @ghidraAddress USA: 0x80268390
 * @ghidraAddress EUR: 0x80269b80
 * @ghidraAddress JPN: 0x80269630
 */
int osSetTimer(OSTimer *t, OSTime countdown, OSTime interval, OSMesgQueue *mq, OSMesg msg);

/* Video. */

/**
 * Start the VI manager thread at priority pri. The call raises the calling thread to pri while
 * it creates the manager, and does nothing once the manager runs.
 *
 * @param pri The priority of the manager thread.
 * @ghidraAddress USA: 0x80266400
 * @ghidraAddress EUR: 0x80267b90
 * @ghidraAddress JPN: 0x80267690
 */
void osCreateViManager(OSPri pri);

/**
 * Blank the screen from the next field on, or show it again.
 *
 * @param active TRUE to blank the screen, FALSE to show it.
 * @ghidraAddress USA: 0x80260ab0
 * @ghidraAddress EUR: 0x80265e40
 * @ghidraAddress JPN: 0x80265870
 */
void osViBlack(u8 active);

/**
 * Select the video mode from the next field on.
 *
 * @param modep The mode, such as osViModeNtscLan1.
 * @ghidraAddress USA: 0x80264a60
 * @ghidraAddress EUR: 0x80265dd0
 * @ghidraAddress JPN: 0x80265cf0
 */
void osViSetMode(OSViMode *modep);

/**
 * Show a frame buffer from the next field on.
 *
 * @param frameBufPtr The virtual address of the frame buffer.
 * @ghidraAddress USA: 0x80264ad0
 * @ghidraAddress EUR: 0x80266260
 * @ghidraAddress JPN: 0x80265d60
 */
void osViSwapBuffer(void *frameBufPtr);

/**
 * Return the frame buffer the screen shows.
 *
 * @return The frame buffer of the current context.
 * @ghidraAddress USA: 0x80264b20
 * @ghidraAddress EUR: 0x802662b0
 * @ghidraAddress JPN: 0x80265db0
 */
void *osViGetCurrentFramebuffer(void);

/**
 * Send m to mq every retraceCount vertical retraces.
 *
 * @param mq The queue the VI manager sends to.
 * @param m The message the VI manager sends.
 * @param retraceCount The number of retraces between messages, 1 for every retrace.
 * @ghidraAddress USA: 0x80265600
 * @ghidraAddress EUR: 0x80266d90
 * @ghidraAddress JPN: 0x80266890
 */
void osViSetEvent(OSMesgQueue *mq, OSMesg m, u32 retraceCount);

/**
 * Turn the gamma, gamma dither, divot, and dither filter features on and off, from the
 * OS_VI_* flags.
 *
 * @param func The OS_VI_*_ON and OS_VI_*_OFF flags to apply.
 * @ghidraAddress USA: 0x80264490
 * @ghidraAddress EUR: 0x80265c10
 * @ghidraAddress JPN: 0x802656b0
 */
void osViSetSpecialFeatures(u32 func);

/* Peripheral access. */

/**
 * Return the handle of the cartridge ROM, creating it from the timing its first word records.
 *
 * @return The handle of the cartridge ROM.
 * @ghidraAddress USA: 0x8026c3d0
 * @ghidraAddress EUR: 0x8026e3a0
 * @ghidraAddress JPN: 0x8026de50
 */
OSPiHandle *osCartRomInit(void);

/**
 * Create the handle of the 64DD and select its domain 2 timing.
 *
 * @return The handle of the 64DD.
 * @ghidraAddress USA: 0x8026c4c0
 * @ghidraAddress EUR: 0x8026e490
 * @ghidraAddress JPN: 0x8026df40
 */
OSPiHandle *osLeoDiskInit(void);

/**
 * Start the PI manager thread at priority pri, serving the requests of cmdQ. The call raises the
 * calling thread to pri while it creates the manager, and does nothing once the manager runs.
 *
 * @param pri The priority of the manager thread.
 * @param cmdQ The queue the manager receives requests on, created by the call.
 * @param cmdBuf The buffer of the messages of cmdQ.
 * @param cmdMsgCnt The number of messages cmdBuf has room for.
 * @ghidraAddress USA: 0x80266760
 * @ghidraAddress EUR: 0x80267f50
 * @ghidraAddress JPN: 0x802679f0
 */
void osCreatePiManager(OSPri pri, OSMesgQueue *cmdQ, OSMesg *cmdBuf, s32 cmdMsgCnt);

/**
 * Return the command queue of the PI manager, or NULL when it does not run.
 *
 * @return The command queue, or NULL.
 * @ghidraAddress USA: 0x8026b720
 * @ghidraAddress EUR: 0x8026d6f0
 * @ghidraAddress JPN: 0x8026d1a0
 */
OSMesgQueue *osPiGetCmdQueue(void);

/**
 * Read a word of the cartridge domain at an offset from the ROM base.
 *
 * @param devAddr The offset from the ROM base, a multiple of 4.
 * @param data Receives the word.
 * @return 0.
 * @ghidraAddress USA: 0x802663a0
 * @ghidraAddress EUR: 0x80267b30
 * @ghidraAddress JPN: 0x80267630
 */
s32 osPiRawReadIo(u32 devAddr, u32 *data);

/**
 * Start a DMA between RDRAM and the cartridge domain at an offset from the ROM base.
 *
 * @param direction OS_READ to read the cartridge, OS_WRITE to write it.
 * @param devAddr The offset from the ROM base, a multiple of 2.
 * @param dramAddr The virtual address of the buffer in RDRAM, aligned to 8 bytes.
 * @param size The count of bytes to move.
 * @return 0, or -1 for a direction other than OS_READ and OS_WRITE.
 * @ghidraAddress USA: 0x8026c680
 * @ghidraAddress EUR: 0x8026e650
 * @ghidraAddress JPN: 0x8026e100
 */
s32 osPiRawStartDma(s32 direction, u32 devAddr, void *dramAddr, u32 size);

/**
 * Queue a DMA request with the PI manager. A request of priority OS_MESG_PRI_HIGH goes to the
 * front of the queue.
 *
 * @param mb The request, not reused until the manager sends it to mq.
 * @param priority OS_MESG_PRI_NORMAL or OS_MESG_PRI_HIGH.
 * @param direction OS_READ to read the cartridge, OS_WRITE to write it.
 * @param devAddr The offset from the ROM base, a multiple of 2.
 * @param vAddr The virtual address of the buffer in RDRAM, aligned to 8 bytes.
 * @param nbytes The count of bytes to move.
 * @param mq The queue the manager sends mb to when the transfer ends.
 * @return The result of the send, or -1 when the PI manager does not run.
 * @ghidraAddress USA: 0x80264650
 * @ghidraAddress EUR: 0x80265eb0
 * @ghidraAddress JPN: 0x802658e0
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
 * @param pihandle The handle of the device.
 * @param devAddr The offset from the base address of the device, a multiple of 4.
 * @param data Receives the word.
 * @return 0.
 * @ghidraAddress USA: 0x8026e780
 * @ghidraAddress EUR: 0x80270750
 * @ghidraAddress JPN: 0x80270200
 */
s32 osEPiRawReadIo(OSPiHandle *pihandle, u32 devAddr, u32 *data);

/**
 * Write a word of the device a handle describes.
 *
 * @param pihandle The handle of the device.
 * @param devAddr The offset from the base address of the device, a multiple of 4.
 * @param data The word to write.
 * @return 0.
 * @ghidraAddress USA: 0x8026e730
 * @ghidraAddress EUR: 0x80270700
 * @ghidraAddress JPN: 0x802701b0
 */
s32 osEPiRawWriteIo(OSPiHandle *pihandle, u32 devAddr, u32 data);

/**
 * Start a DMA with the device a handle describes, selecting its domain timing when another
 * handle selected different timing.
 *
 * @param pihandle The handle of the device.
 * @param direction OS_READ to read the device, OS_WRITE to write it.
 * @param devAddr The offset from the base address of the device, a multiple of 2.
 * @param dramAddr The virtual address of the buffer in RDRAM, aligned to 8 bytes.
 * @param size The count of bytes to move.
 * @return 0, or -1 for a direction other than OS_READ and OS_WRITE.
 * @ghidraAddress USA: 0x8026c760
 * @ghidraAddress EUR: 0x8026e730
 * @ghidraAddress JPN: 0x8026e1e0
 */
s32 osEPiRawStartDma(OSPiHandle *pihandle, s32 direction, u32 devAddr, void *dramAddr, u32 size);

/* Audio. */

/**
 * Set the sample rate of the audio DAC to the nearest the video clock allows.
 *
 * @param frequency The requested sample rate in hertz.
 * @return The rate set in hertz, or -1 when the frequency is too high.
 * @ghidraAddress USA: 0x80268230
 * @ghidraAddress EUR: 0x80269a20
 * @ghidraAddress JPN: 0x802694d0
 */
s32 osAiSetFrequency(u32 frequency);

/**
 * Queue a buffer of samples for the audio DMA.
 *
 * @param bufPtr The virtual address of the samples, aligned to 8 bytes.
 * @param size The length of the buffer in bytes, a multiple of 8.
 * @return 0, or -1 when the queue of the audio interface is full.
 * @ghidraAddress USA: 0x80268470
 * @ghidraAddress EUR: 0x80269c60
 * @ghidraAddress JPN: 0x80269710
 */
s32 osAiSetNextBuffer(void *bufPtr, u32 size);

/**
 * Return the audio status register.
 *
 * @return The value of the AI status register.
 * @ghidraAddress USA: 0x80268520
 * @ghidraAddress EUR: 0x80269d10
 * @ghidraAddress JPN: 0x802697c0
 */
u32 osAiGetStatus(void);

/**
 * Return the remaining length of the buffer the audio DMA plays.
 *
 * @return The bytes the audio DMA has yet to play.
 * @ghidraAddress USA: 0x80268530
 * @ghidraAddress EUR: 0x80269d20
 * @ghidraAddress JPN: 0x802697d0
 */
u32 osAiGetLength(void);

/**
 * @ghidraAddress USA: 0x8026ce20
 * @ghidraAddress EUR: 0x8026edf0
 * @ghidraAddress JPN: 0x8026e8a0
 */
s32 __osAiDeviceBusy(void);

/**
 * The frequency of the video clock. osInitialize() selects it by television standard.
 *
 * @ghidraAddress USA: 0x802a6148
 * @ghidraAddress EUR: 0x802a7c58
 * @ghidraAddress JPN: 0x802a7798
 */
extern s32 osViClock;

/**
 * Whether the last buffer osAiSetNextBuffer() queued ended 0x2000 bytes into a 16 KiB page, a
 * case the audio DMA mishandles.
 *
 * @ghidraAddress USA: 0x802a61b0
 * @ghidraAddress EUR: 0x802a7d10
 * @ghidraAddress JPN: 0x802a7800
 */
extern u8 aiHdwrBugFlag;

/**
 * Initialise the system: the FPU, the PIF, the exception vectors and the clock rates.
 *
 * @ghidraAddress USA: 0x80265d90
 * @ghidraAddress EUR: 0x80267520
 * @ghidraAddress JPN: 0x80267020
 */
void osInitialize(void);

/**
 * Print to the debugger. The retail library discards the output.
 *
 * @param fmt The printf format, followed by its arguments.
 * @ghidraAddress USA: 0x802607dc
 * @ghidraAddress EUR: 0x80261cdc
 * @ghidraAddress JPN: 0x80261a3c
 */
void osSyncPrintf(const char *fmt, ...);

/**
 * Print to the remote monitor. The retail library discards the output.
 *
 * @param fmt The printf format, followed by its arguments.
 * @ghidraAddress USA: 0x802607c0
 * @ghidraAddress EUR: 0x80261cc0
 * @ghidraAddress JPN: 0x80261a20
 */
void rmonPrintf(const char *fmt, ...);

/**
 * @ghidraAddress USA: 0x80266020
 * @ghidraAddress EUR: 0x802677b0
 * @ghidraAddress JPN: 0x802672b0
 */
unsigned long long __ull_rshift(unsigned long long a0, unsigned long long a1);

/**
 * @ghidraAddress USA: 0x8026604c
 * @ghidraAddress EUR: 0x802677dc
 * @ghidraAddress JPN: 0x802672dc
 */
unsigned long long __ull_rem(unsigned long long a0, unsigned long long a1);

/**
 * @ghidraAddress USA: 0x80266088
 * @ghidraAddress EUR: 0x80267818
 * @ghidraAddress JPN: 0x80267318
 */
unsigned long long __ull_div(unsigned long long a0, unsigned long long a1);

/**
 * @ghidraAddress USA: 0x802660c4
 * @ghidraAddress EUR: 0x80267854
 * @ghidraAddress JPN: 0x80267354
 */
long long __ll_lshift(long long a0, long long a1);

/**
 * @ghidraAddress USA: 0x802660f0
 * @ghidraAddress EUR: 0x80267880
 * @ghidraAddress JPN: 0x80267380
 */
long long __ll_rem(unsigned long long a0, long long a1);

/**
 * @ghidraAddress USA: 0x8026612c
 * @ghidraAddress EUR: 0x802678bc
 * @ghidraAddress JPN: 0x802673bc
 */
long long __ll_div(long long a0, long long a1);

/**
 * @ghidraAddress USA: 0x80266188
 * @ghidraAddress EUR: 0x80267918
 * @ghidraAddress JPN: 0x80267418
 */
long long __ll_mul(long long a0, long long a1);

/**
 * @ghidraAddress USA: 0x802661b8
 * @ghidraAddress EUR: 0x80267948
 * @ghidraAddress JPN: 0x80267448
 */
void __ull_divremi(unsigned long long *quotient,
                   unsigned long long *remainder,
                   unsigned long long dividend,
                   unsigned short divisor);

/**
 * @ghidraAddress USA: 0x80266218
 * @ghidraAddress EUR: 0x802679a8
 * @ghidraAddress JPN: 0x802674a8
 */
long long __ll_mod(long long a0, long long a1);

/**
 * @ghidraAddress USA: 0x802662b4
 * @ghidraAddress EUR: 0x80267a44
 * @ghidraAddress JPN: 0x80267544
 */
long long __ll_rshift(long long a0, long long a1);

/**
 * @ghidraAddress USA: 0x802669d0
 * @ghidraAddress EUR: 0x802681c0
 * @ghidraAddress JPN: 0x80267c70
 */
long long __d_to_ll(double d);

/**
 * @ghidraAddress USA: 0x802669ec
 * @ghidraAddress EUR: 0x802681dc
 * @ghidraAddress JPN: 0x80267c8c
 */
long long __f_to_ll(float f);

/**
 * @ghidraAddress USA: 0x80266a08
 * @ghidraAddress EUR: 0x802681f8
 * @ghidraAddress JPN: 0x80267ca8
 */
unsigned long long __d_to_ull(double d);

/**
 * @ghidraAddress USA: 0x80266aa8
 * @ghidraAddress EUR: 0x80268298
 * @ghidraAddress JPN: 0x80267d48
 */
unsigned long long __f_to_ull(float f);

/**
 * @ghidraAddress USA: 0x80266b44
 * @ghidraAddress EUR: 0x80268334
 * @ghidraAddress JPN: 0x80267de4
 */
double __ll_to_d(long long s);

/**
 * @ghidraAddress USA: 0x80266b5c
 * @ghidraAddress EUR: 0x8026834c
 * @ghidraAddress JPN: 0x80267dfc
 */
float __ll_to_f(long long s);

/**
 * @ghidraAddress USA: 0x80266b74
 * @ghidraAddress EUR: 0x80268364
 * @ghidraAddress JPN: 0x80267e14
 */
double __ull_to_d(unsigned long long u);

/**
 * @ghidraAddress USA: 0x80266ba8
 * @ghidraAddress EUR: 0x80268398
 * @ghidraAddress JPN: 0x80267e48
 */
float __ull_to_f(unsigned long long u);

#endif
