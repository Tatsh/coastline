/*
 * Write back every line of the data cache.
 */

    .set noreorder
    .set noat
    .set gp=64

    .text
    .balign 4

    .globl osWritebackDCacheAll
    .type osWritebackDCacheAll, @function
osWritebackDCacheAll:
    lui        $t0, 0x8000
    addiu      $t2, $zero, 0x2000
    addu       $t1, $t0, $t2
    addiu      $t1, $t1, -0x10
.LosWritebackDCacheAll_1:
    cache      0x01, 0x0($t0)
    sltu       $at, $t0, $t1
    bnez       $at, .LosWritebackDCacheAll_1
    addiu      $t0, $t0, 0x10
    jr         $ra
    nop
    .size osWritebackDCacheAll, . - osWritebackDCacheAll
