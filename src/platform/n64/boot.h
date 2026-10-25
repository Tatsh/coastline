#ifndef PLATFORM_N64_BOOT_H
#define PLATFORM_N64_BOOT_H

/**
 * @file
 * The entry of the cartridge: the routine the console jumps to, the BSS segment it clears, the
 * stack it starts on, and the start of the operating system.
 */

#include <ultra64.h>

/** Start of the BSS segment, from the link script. */
extern u8 _bss_start[];

/** Length of the BSS segment in bytes, from the link script. A multiple of 8. */
extern u8 _bss_size[];

/**
 * Top of the stack ramMain() hands to OsBootEntry(), from the link script.
 *
 * @ghidraAddress USA: 0x803168d0
 * @ghidraAddress EUR: 0x803185b0
 * @ghidraAddress JPN: 0x803171e0
 */
extern u8 g_abBootStackTop[];

/**
 * Clear the BSS segment, two words at a time, move the stack to the boot stack, and jump to
 * OsBootEntry(). The routine is hand-written assembly in main.s.
 *
 * @ghidraAddress USA: 0x80200000
 * @ghidraAddress EUR: 0x80200000
 * @ghidraAddress JPN: 0x80200000
 */
void ramMain(void);

/**
 * Initialise the operating system, then create and start the boot thread. The European release
 * first sets the television standard to PAL.
 *
 * @ghidraAddress USA: 0x8021e5c4
 * @ghidraAddress EUR: 0x8021f8a4
 * @ghidraAddress JPN: 0x8021f0b4
 */
void OsBootEntry(void);

#endif
