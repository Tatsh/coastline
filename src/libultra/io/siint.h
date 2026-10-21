#ifndef LIBULTRA_IO_SIINT_H
#define LIBULTRA_IO_SIINT_H

/**
 * @file
 * The SI access queue and the raw SI routines.
 */

#include <ultra64.h>

/** @ghidraAddress USA: 0x802a6280 */
extern u32 __osSiAccessQueueEnabled;

/** @ghidraAddress USA: 0x80381888 */
extern OSMesgQueue __osSiAccessQueue;

/** @ghidraAddress USA: 0x80381880 */
extern OSMesg siAccessBuf[1];

/** @ghidraAddress USA: 0x80269410 */
void __osSiCreateAccessQueue(void);

/** @ghidraAddress USA: 0x80269460 */
void __osSiGetAccess(void);

/** @ghidraAddress USA: 0x802694a4 */
void __osSiRelAccess(void);

/** @ghidraAddress USA: 0x802694d0 */
s32 __osSiRawStartDma(s32 direction, void *dramAddr);

/** @ghidraAddress USA: 0x8026ba20 */
s32 __osSiRawReadIo(u32 devAddr, u32 *data);

/** @ghidraAddress USA: 0x8026ba70 */
s32 __osSiRawWriteIo(u32 devAddr, u32 data);

/** @ghidraAddress USA: 0x8026dbb0 */
int __osSiDeviceBusy(void);

#endif
