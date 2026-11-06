/*
 * Write back the lines of the data cache that hold a range, or the whole cache for a range of its
 * size or more.
 */

    .set noreorder
    .set noat
    .set gp=64

    .text
    .balign 4

    .globl osWritebackDCache
    .type osWritebackDCache, @function
osWritebackDCache:
    blez       $a1, .LosWritebackDCache_2
    nop
    addiu      $t3, $zero, 0x2000
    sltu       $at, $a1, $t3
    beqz       $at, .LosWritebackDCache_3
    nop
    or         $t0, $a0, $zero
    addu       $t1, $a0, $a1
    sltu       $at, $t0, $t1
    beqz       $at, .LosWritebackDCache_2
    nop
    andi       $t2, $t0, 0xF
    addiu      $t1, $t1, -0x10
    subu       $t0, $t0, $t2
.LosWritebackDCache_1:
    cache      0x19, 0x0($t0)
    sltu       $at, $t0, $t1
    bnez       $at, .LosWritebackDCache_1
    addiu      $t0, $t0, 0x10
.LosWritebackDCache_2:
    jr         $ra
    nop
.LosWritebackDCache_3:
    lui        $t0, 0x8000
    addu       $t1, $t0, $t3
    addiu      $t1, $t1, -0x10
.LosWritebackDCache_4:
    cache      0x01, 0x0($t0)
    sltu       $at, $t0, $t1
    bnez       $at, .LosWritebackDCache_4
    addiu      $t0, $t0, 0x10
    jr         $ra
    nop
    .size osWritebackDCache, . - osWritebackDCache
