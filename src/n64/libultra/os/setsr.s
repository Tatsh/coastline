/*
 * Write and read the Status register of coprocessor 0.
 */

    .set noreorder
    .set noat
    .set gp=64

    .text
    .balign 4

    .globl __osSetSR
    .type __osSetSR, @function
__osSetSR:
    mtc0       $a0, $12
    nop
    jr         $ra
    nop
    .size __osSetSR, . - __osSetSR

    .globl __osGetSR
    .type __osGetSR, @function
__osGetSR:
    mfc0       $v0, $12
    jr         $ra
    nop
    .size __osGetSR, . - __osGetSR
