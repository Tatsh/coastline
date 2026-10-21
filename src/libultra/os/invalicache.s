/*
 * Invalidate the lines of the instruction cache that hold a range, or the whole cache for a range
 * of its size or more.
 */

    .set noreorder
    .set noat
    .set gp=64

    .text
    .balign 4

    .globl osInvalICache
    .type osInvalICache, @function
osInvalICache:
    blez       $a1, .LosInvalICache_2
    nop
    addiu      $t3, $zero, 0x4000
    sltu       $at, $a1, $t3
    beqz       $at, .LosInvalICache_3
    nop
    or         $t0, $a0, $zero
    addu       $t1, $a0, $a1
    sltu       $at, $t0, $t1
    beqz       $at, .LosInvalICache_2
    nop
    andi       $t2, $t0, 0x1F
    addiu      $t1, $t1, -0x20
    subu       $t0, $t0, $t2
.LosInvalICache_1:
    cache      0x10, 0x0($t0)
    sltu       $at, $t0, $t1
    bnez       $at, .LosInvalICache_1
    addiu      $t0, $t0, 0x20
.LosInvalICache_2:
    jr         $ra
    nop
.LosInvalICache_3:
    lui        $t0, 0x8000
    addu       $t1, $t0, $t3
    addiu      $t1, $t1, -0x20
.LosInvalICache_4:
    cache      0x00, 0x0($t0)
    sltu       $at, $t0, $t1
    bnez       $at, .LosInvalICache_4
    addiu      $t0, $t0, 0x20
    jr         $ra
    nop
    .size osInvalICache, . - osInvalICache
