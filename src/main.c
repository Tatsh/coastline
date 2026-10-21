/**
 * @file
 * Entry point of the Nintendo 64 releases.
 */
#include <ultra64.h>

#include "platform/n64/boot.h"

/**
 * Clears the BSS segment, two words at a time, and enters the OS boot.
 *
 * The original is hand-written assembly. It sets the stack pointer (0x803168d0 in the USA
 * release, 0x803171e0 in the JPN release, and 0x803185b0 in the EUR release) before it jumps to
 * OsBootEntry(). A C body cannot set the stack pointer. The link script provides the stack.
 *
 * @ghidraAddress USA: 0x80200000
 * @ghidraAddress JPN: 0x80200000
 * @ghidraAddress EUR: 0x80200000
 */
void ramMain(void) {
    u32 *pWord = (u32 *)_bss_start;
    s32 nRemaining = (s32)_bss_size;

    do {
        nRemaining -= 8;
        pWord[0] = 0;
        pWord[1] = 0;
        pWord += 2;
    } while (nRemaining != 0);
    OsBootEntry();
}
