#ifndef PLATFORM_N64_BOOT_H
#define PLATFORM_N64_BOOT_H

#include <ultra64.h>

/** Start of the BSS segment, from the link script. */
extern u8 _bss_start[];

/** Length of the BSS segment in bytes, from the link script. A multiple of 8. */
extern u8 _bss_size[];

/**
 * Top of the stack ramMain() hands to OsBootEntry(), from the link script.
 *
 * @ghidraAddress USA: 0x803168d0
 */
extern u8 g_abBootStackTop[];

/**
 * Initialise the operating system, then create and start the boot thread.
 *
 * @ghidraAddress USA: 0x8021e5c4
 * @ghidraAddress JPN: 0x8021f0b4
 * @ghidraAddress EUR: 0x8021f8a4
 */
void OsBootEntry(void);

#endif
