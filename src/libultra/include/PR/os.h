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
        f32 f_odd;
        f32 f_even;
    } f;
    f64 d;
} __OSfp;

typedef struct {
    u64 at, v0, v1, a0, a1, a2, a3;
    u64 t0, t1, t2, t3, t4, t5, t6, t7;
    u64 s0, s1, s2, s3, s4, s5, s6, s7;
    u64 t8, t9, gp, sp, s8, ra;
    u64 lo, hi;
    u32 sr, pc, cause, badvaddr, rcp;
    u32 fpcsr;
    __OSfp fp0, fp2, fp4, fp6, fp8, fp10, fp12, fp14;
    __OSfp fp16, fp18, fp20, fp22, fp24, fp26, fp28, fp30;
} __OSThreadContext;

typedef struct OSThread_s {
    struct OSThread_s *next;
    OSPri priority;
    struct OSThread_s **queue;
    struct OSThread_s *tlnext;
    u16 state;
    u16 flags;
    OSId id;
    int fp;
    __OSThreadContext context;
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
    OSThread *mtqueue;
    OSThread *fullqueue;
    s32 validCount;
    s32 first;
    s32 msgCount;
    OSMesg *msg;
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
    struct OSTimer_s *next;
    struct OSTimer_s *prev;
    OSTime interval;
    OSTime value;
    OSMesgQueue *mq;
    OSMesg msg;
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
    u32 errStatus;
    void *dramAddr;
    void *C2Addr;
    u32 sectorSize;
    u32 C1ErrNum;
    u32 C1ErrSector[4];
} __OSBlockInfo;

typedef struct {
    u32 cmdType;
    u16 transferMode;
    u16 blockNum;
    s32 sectorNum;
    u32 devAddr;
    u32 bmCtlShadow;
    u32 seqCtlShadow;
    __OSBlockInfo block[2];
} __OSTranxInfo;

typedef struct OSPiHandle_s {
    struct OSPiHandle_s *next;
    u8 type;
    u8 latency;
    u8 pageSize;
    u8 relDuration;
    u8 pulse;
    u8 domain;
    u32 baseAddress;
    u32 speed;
    __OSTranxInfo transferInfo;
} OSPiHandle;

typedef struct {
    u8 type;
    u32 address;
} OSPiInfo;

typedef struct {
    u16 type;
    u8 pri;
    u8 status;
    OSMesgQueue *retQueue;
} OSIoMesgHdr;

typedef struct {
    OSIoMesgHdr hdr;
    void *dramAddr;
    u32 devAddr;
    u32 size;
    OSPiHandle *piHandle;
} OSIoMesg;

typedef struct {
    s32 active;
    OSThread *thread;
    OSMesgQueue *cmdQueue;
    OSMesgQueue *evtQueue;
    OSMesgQueue *acsQueue;
    s32 (*dma)(s32, u32, void *, u32);
    s32 (*edma)(OSPiHandle *, s32, u32, void *, u32);
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
 * Append a message to a queue and wake its first receiver.
 *
 * @return 0, or -1 when the queue is full and flag is OS_MESG_NOBLOCK.
 * @ghidraAddress USA: 0x802654b0
 */
s32 osSendMesg(OSMesgQueue *mq, OSMesg msg, s32 flag);

/**
 * Insert a message at the front of a queue and wake its first receiver.
 *
 * @return 0, or -1 when the queue is full and flag is OS_MESG_NOBLOCK.
 * @ghidraAddress USA: 0x8026b5d0
 */
s32 osJamMesg(OSMesgQueue *mq, OSMesg msg, s32 flag);

/**
 * Take the first message of a queue, store it unless msg is NULL, and wake its first sender.
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
OSIntMask osSetIntMask(OSIntMask im);
u32 osGetCount(void);
void osInvalDCache(void *vaddr, s32 nbytes);
void osInvalICache(void *vaddr, s32 nbytes);
void osWritebackDCache(void *vaddr, s32 nbytes);
void osWritebackDCacheAll(void);
u32 osVirtualToPhysical(void *vaddr);
void osMapTLBRdb(void);

/* Time. */
OSTime osGetTime(void);
void osSetTime(OSTime time);
int osSetTimer(OSTimer *t, OSTime countdown, OSTime interval, OSMesgQueue *mq, OSMesg msg);
int osStopTimer(OSTimer *t);

/* Peripheral access. */
OSPiHandle *osCartRomInit(void);
OSPiHandle *osDriveRomInit(void);
void osCreatePiManager(OSPri pri, OSMesgQueue *cmdQ, OSMesg *cmdBuf, s32 cmdMsgCnt);
OSMesgQueue *osPiGetCmdQueue(void);
s32 osPiRawReadIo(u32 devAddr, u32 *data);
s32 osPiRawStartDma(s32 direction, u32 devAddr, void *dramAddr, u32 size);
s32 osPiStartDma(OSIoMesg *mb,
                 s32 priority,
                 s32 direction,
                 u32 devAddr,
                 void *vAddr,
                 u32 nbytes,
                 OSMesgQueue *mq);
s32 osEPiRawReadIo(OSPiHandle *pihandle, u32 devAddr, u32 *data);
s32 osEPiRawWriteIo(OSPiHandle *pihandle, u32 devAddr, u32 data);
s32 osEPiRawStartDma(OSPiHandle *pihandle, s32 direction, u32 devAddr, void *dramAddr, u32 size);

/* Start-up. */
void osInitialize(void);

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
                   unsigned short divisor);
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
