#ifndef PLATFORM_N64_BOOT_H
#define PLATFORM_N64_BOOT_H

#include <ultra64.h>

/** Start of the BSS segment, from the link script. */
extern u8 _bss_start[];

/** Length of the BSS segment in bytes, from the link script. */
extern u8 _bss_size[];

/**
 * Initialise the operating system, then create and start the boot thread.
 *
 * @ghidraAddress USA: 0x8021e5c4
 * @ghidraAddress JPN: 0x8021f0b4
 * @ghidraAddress EUR: 0x8021f8a4
 */
void OsBootEntry(void);

#endif
