/**
 * @file
 * Entry point of the Nintendo 64 releases.
 */
#include <ultra64.h>

#include "platform/n64/boot.h"

/**
 * Clears the BSS segment, two words at a time, moves the stack to the boot stack, and enters the
 * OS boot.
 *
 * The original is hand-written assembly. The stack switch is the last statement. The call compiles
 * to a jump, and ramMain() has no frame to lose.
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
    __asm__ volatile("move $sp, %0" : : "r"(g_abBootStackTop));
    OsBootEntry();
}
