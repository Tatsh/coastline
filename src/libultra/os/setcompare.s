/*
 * Write the Compare register of coprocessor 0, which raises the timer interrupt when Count reaches
 * it.
 */

    .set noreorder
    .set noat
    .set gp=64

    .text
    .balign 4

    .globl __osSetCompare
    .type __osSetCompare, @function
__osSetCompare:
    mtc0       $a0, $11
    jr         $ra
    nop
    .size __osSetCompare, . - __osSetCompare
