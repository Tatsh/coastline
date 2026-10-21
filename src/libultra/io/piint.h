#ifndef LIBULTRA_IO_PIINT_H
#define LIBULTRA_IO_PIINT_H

/**
 * @file
 * The PI manager, the PI access queue, and the PI handles of the cartridge and the 64DD.
 */

#include <ultra64.h>

/** Whether __osPiCreateAccessQueue() has run. @ghidraAddress USA: 0x802a62e0 */
extern u32 __osPiAccessQueueEnabled;

/** @ghidraAddress USA: 0x803819e8 */
extern OSMesgQueue __osPiAccessQueue;

/** @ghidraAddress USA: 0x803819e0 */
extern OSMesg piAccessBuf[1];

/**
 * The handle of each PI domain whose timing is in the domain registers.
 *
 * @ghidraAddress USA: 0x802a61a0
 */
extern OSPiHandle *__osCurrentHandle[2];

/** @ghidraAddress USA: 0x802a6180 */
extern OSDevMgr __osPiDevMgr;

/**
 * The list of every PI handle osCartRomInit() and osLeoDiskInit() created.
 *
 * @ghidraAddress USA: 0x802a619c
 */
extern OSPiHandle *__osPiTable;

/** @ghidraAddress USA: 0x803806b0 */
extern OSThread piThread;

/** @ghidraAddress USA: 0x80380860 */
extern u64 piThreadStack[0x1000 / sizeof(u64)];

/** @ghidraAddress USA: 0x80381860 */
extern OSMesgQueue piEventQueue;

/** @ghidraAddress USA: 0x80381878 */
extern OSMesg piEventBuf[1];

/** @ghidraAddress USA: 0x803818e0 */
extern OSPiHandle CartRomHandle;

/** @ghidraAddress USA: 0x80381960 */
extern OSPiHandle LeoDiskHandle;

/** @ghidraAddress USA: 0x803819d4 */
extern OSPiHandle *__osDiskHandle;

/**
 * Create the PI access queue with its one message.
 *
 * @ghidraAddress USA: 0x8026c5c0
 */
void __osPiCreateAccessQueue(void);

/**
 * Wait for the PI access message, creating the queue on first use.
 *
 * @ghidraAddress USA: 0x8026c610
 */
void __osPiGetAccess(void);

/**
 * Return the PI access message.
 *
 * @ghidraAddress USA: 0x8026c654
 */
void __osPiRelAccess(void);

/**
 * Body of the PI manager thread. It serves the DMA requests of its command queue.
 *
 * @ghidraAddress USA: 0x8026c990
 */
void __osDevMgrMain(void *arg);

/* Wait while the PI is busy with a DMA or an I/O access. */
#define WAIT_ON_IOBUSY(stat)                                                                       \
    do {                                                                                           \
        stat = IO_READ(PI_STATUS_REG);                                                             \
    } while (stat & (PI_STATUS_IO_BUSY | PI_STATUS_DMA_BUSY))

#endif
