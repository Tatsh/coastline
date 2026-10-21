/*
 * Invalidate the lines of the data cache that hold a range. A partial line at either end is
 * written back first, since other data shares it. A range of the cache size or more invalidates
 * the whole cache.
 */

    .set noreorder
    .set noat
    .set gp=64

    .text
    .balign 4

    .globl osInvalDCache
    .type osInvalDCache, @function
osInvalDCache:
    blez       $a1, .LosInvalDCache_3
    nop
    addiu      $t3, $zero, 0x2000
    sltu       $at, $a1, $t3
    beqz       $at, .LosInvalDCache_4
    nop
    or         $t0, $a0, $zero
    addu       $t1, $a0, $a1
    sltu       $at, $t0, $t1
    beqz       $at, .LosInvalDCache_3
    nop
    andi       $t2, $t0, 0xF
    beqz       $t2, .LosInvalDCache_1
    addiu      $t1, $t1, -0x10
    subu       $t0, $t0, $t2
    cache      0x15, 0x0($t0)
    sltu       $at, $t0, $t1
    beqz       $at, .LosInvalDCache_3
    nop
    addiu      $t0, $t0, 0x10
.LosInvalDCache_1:
    andi       $t2, $t1, 0xF
    beqz       $t2, .LosInvalDCache_2
    nop
    subu       $t1, $t1, $t2
    cache      0x15, 0x10($t1)
    sltu       $at, $t1, $t0
    bnez       $at, .LosInvalDCache_3
    nop
.LosInvalDCache_2:
    cache      0x11, 0x0($t0)
    sltu       $at, $t0, $t1
    bnez       $at, .LosInvalDCache_2
    addiu      $t0, $t0, 0x10
.LosInvalDCache_3:
    jr         $ra
    nop
.LosInvalDCache_4:
    lui        $t0, 0x8000
    addu       $t1, $t0, $t3
    addiu      $t1, $t1, -0x10
.LosInvalDCache_5:
    cache      0x01, 0x0($t0)
    sltu       $at, $t0, $t1
    bnez       $at, .LosInvalDCache_5
    addiu      $t0, $t0, 0x10
    jr         $ra
    nop
    .size osInvalDCache, . - osInvalDCache
