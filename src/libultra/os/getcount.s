/*
 * Read the Count register of coprocessor 0, which increments at half the CPU clock.
 */

    .set noreorder
    .set noat
    .set gp=64

    .text
    .balign 4

    .globl osGetCount
    .type osGetCount, @function
osGetCount:
    mfc0       $v0, $9
    jr         $ra
    nop
    .size osGetCount, . - osGetCount
