/*
 * Disable interrupts and return the previous enable bit, and restore that bit.
 */

    .set noreorder
    .set noat
    .set gp=64

    .text
    .balign 4

    .globl __osDisableInt
    .type __osDisableInt, @function
__osDisableInt:
    mfc0       $t0, $12
    addiu      $at, $zero, -0x2
    and        $t1, $t0, $at
    mtc0       $t1, $12
    andi       $v0, $t0, 0x1
    nop
    jr         $ra
    nop
    .size __osDisableInt, . - __osDisableInt

    .globl __osRestoreInt
    .type __osRestoreInt, @function
__osRestoreInt:
    mfc0       $t0, $12
    or         $t0, $t0, $a0
    mtc0       $t0, $12
    nop
    nop
    jr         $ra
    nop
    .size __osRestoreInt, . - __osRestoreInt
