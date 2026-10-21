/*
 * Translate a virtual address through the TLB. The result is the physical address, or -1 when no
 * valid entry maps it.
 */

    .set noreorder
    .set noat
    .set gp=64

    .text
    .balign 4

    .globl __osProbeTLB
    .type __osProbeTLB, @function
__osProbeTLB:
    mfc0       $t0, $10
    andi       $t1, $t0, 0xFF
    addiu      $at, $zero, -0x2000
    and        $t2, $a0, $at
    or         $t1, $t1, $t2
    mtc0       $t1, $10
    nop
    nop
    nop
    tlbp
    nop
    nop
    mfc0       $t3, $0
    lui        $at, 0x8000
    and        $t3, $t3, $at
    bnez       $t3, .L__osProbeTLB_3
    nop
    tlbr
    nop
    nop
    nop
    mfc0       $t3, $5
    addi       $t3, $t3, 0x2000
    srl        $t3, $t3, 1
    and        $t4, $t3, $a0
    bnez       $t4, .L__osProbeTLB_1
    addi       $t3, $t3, -0x1
    mfc0       $v0, $2
    b          .L__osProbeTLB_2
    nop
.L__osProbeTLB_1:
    mfc0       $v0, $3
.L__osProbeTLB_2:
    andi       $t5, $v0, 0x2
    beqz       $t5, .L__osProbeTLB_3
    nop
    lui        $at, 0x3fff
    ori        $at, $at, 0xffc0
    and        $v0, $v0, $at
    sll        $v0, $v0, 6
    and        $t5, $a0, $t3
    add        $v0, $v0, $t5
    b          .L__osProbeTLB_4
    nop
.L__osProbeTLB_3:
    addiu      $v0, $zero, -0x1
.L__osProbeTLB_4:
    mtc0       $t0, $10
    jr         $ra
    nop
    .size __osProbeTLB, . - __osProbeTLB
