/*
 * Clear a1 bytes from a0. An unaligned head is cleared with one unaligned store, then the body 32
 * bytes and 4 bytes at a time, and the tail a byte at a time.
 */

    .set noreorder
    .set noat
    .set gp=64

    .text
    .balign 4

    .globl bzero
    .type bzero, @function
bzero:
    slti       $at, $a1, 0xC
    bnez       $at, .Lbzero_5
    negu       $v1, $a0
    andi       $v1, $v1, 0x3
    beqz       $v1, .Lbzero_1
    subu       $a1, $a1, $v1
    swl        $zero, 0x0($a0)
    addu       $a0, $a0, $v1
.Lbzero_1:
    addiu      $at, $zero, -0x20
    and        $a3, $a1, $at
    beqz       $a3, .Lbzero_3
    subu       $a1, $a1, $a3
    addu       $a3, $a3, $a0
.Lbzero_2:
    addiu      $a0, $a0, 0x20
    sw         $zero, -0x20($a0)
    sw         $zero, -0x1C($a0)
    sw         $zero, -0x18($a0)
    sw         $zero, -0x14($a0)
    sw         $zero, -0x10($a0)
    sw         $zero, -0xC($a0)
    sw         $zero, -0x8($a0)
    bne        $a0, $a3, .Lbzero_2
    sw         $zero, -0x4($a0)
.Lbzero_3:
    addiu      $at, $zero, -0x4
    and        $a3, $a1, $at
    beqz       $a3, .Lbzero_5
    subu       $a1, $a1, $a3
    addu       $a3, $a3, $a0
.Lbzero_4:
    addiu      $a0, $a0, 0x4
    bne        $a0, $a3, .Lbzero_4
    sw         $zero, -0x4($a0)
.Lbzero_5:
    blez       $a1, .Lbzero_7
    nop
    addu       $a1, $a1, $a0
.Lbzero_6:
    addiu      $a0, $a0, 0x1
    bne        $a0, $a1, .Lbzero_6
    sb         $zero, -0x1($a0)
.Lbzero_7:
    jr         $ra
    nop
    .size bzero, . - bzero
