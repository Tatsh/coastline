/*
 * Replace the control and status register of the FPU and return the previous value.
 */

    .set noreorder
    .set noat
    .set gp=64

    .text
    .balign 4

    .globl __osSetFpcCsr
    .type __osSetFpcCsr, @function
__osSetFpcCsr:
    cfc1       $v0, $31
    ctc1       $a0, $31
    jr         $ra
    nop
    .size __osSetFpcCsr, . - __osSetFpcCsr
