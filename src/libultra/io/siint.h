#ifndef LIBULTRA_IO_SIINT_H
#define LIBULTRA_IO_SIINT_H

/**
 * @file
 * The SI access queue and the raw SI routines.
 */

#include <ultra64.h>

/** Whether __osSiCreateAccessQueue() has run. @ghidraAddress USA: 0x802a6280 */
extern u32 __osSiAccessQueueEnabled;

/** @ghidraAddress USA: 0x80381888 */
extern OSMesgQueue __osSiAccessQueue;

/** @ghidraAddress USA: 0x80381880 */
extern OSMesg siAccessBuf[1];

/**
 * Create the SI access queue with its one message.
 *
 * @ghidraAddress USA: 0x80269410
 */
void __osSiCreateAccessQueue(void);

/**
 * Wait for the SI access message, creating the queue on first use.
 *
 * @ghidraAddress USA: 0x80269460
 */
void __osSiGetAccess(void);

/**
 * Return the SI access message.
 *
 * @ghidraAddress USA: 0x802694a4
 */
void __osSiRelAccess(void);

/**
 * Start a 64-byte DMA between RDRAM and the PIF RAM.
 *
 * @param direction OS_READ to read the PIF RAM, OS_WRITE to write it.
 * @return 0, or -1 when the SI is busy.
 * @ghidraAddress USA: 0x802694d0
 */
s32 __osSiRawStartDma(s32 direction, void *dramAddr);

/**
 * Read a word of the PIF.
 *
 * @return 0, or -1 when the SI is busy.
 * @ghidraAddress USA: 0x8026ba20
 */
s32 __osSiRawReadIo(u32 devAddr, u32 *data);

/**
 * Write a word of the PIF.
 *
 * @return 0, or -1 when the SI is busy.
 * @ghidraAddress USA: 0x8026ba70
 */
s32 __osSiRawWriteIo(u32 devAddr, u32 data);

/**
 * Return whether the SI is busy with a DMA or a read.
 *
 * @ghidraAddress USA: 0x8026dbb0
 */
int __osSiDeviceBusy(void);

#endif
