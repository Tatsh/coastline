/*
 * Copy a2 bytes from a0 to a1 and return a1. The copy runs backwards when the destination overlaps
 * the end of the source. Buffers of the same alignment are aligned with one byte, halfword, or
 * byte and halfword, then copied 32, 16, and 4 bytes at a time, and the rest a byte at a time.
 * The instructions that follow an unconditional branch are never executed, as in the original.
 */

    .set noreorder
    .set noat
    .set gp=64

    .text
    .balign 4

    .globl bcopy
    .type bcopy, @function
bcopy:
    beqz       $a2, .Lbcopy_4
    or         $a3, $a1, $zero
    beq        $a0, $a1, .Lbcopy_4
    slt        $at, $a1, $a0
    bnel       $at, $zero, .Lbcopy_1
    slti       $at, $a2, 0x10
    add        $v0, $a0, $a2
    slt        $at, $a1, $v0
    beql       $at, $zero, .Lbcopy_1
    slti       $at, $a2, 0x10
    b          .Lbcopy_13
    slti       $at, $a2, 0x10
    slti       $at, $a2, 0x10
.Lbcopy_1:
    bnez       $at, .Lbcopy_2
    nop
    andi       $v0, $a0, 0x3
    andi       $v1, $a1, 0x3
    beq        $v0, $v1, .Lbcopy_5
    nop
.Lbcopy_2:
    beqz       $a2, .Lbcopy_4
    nop
    addu       $v1, $a0, $a2
.Lbcopy_3:
    lb         $v0, 0x0($a0)
    addiu      $a0, $a0, 0x1
    addiu      $a1, $a1, 0x1
    bne        $a0, $v1, .Lbcopy_3
    sb         $v0, -0x1($a1)
.Lbcopy_4:
    jr         $ra
    or         $v0, $a3, $zero
.Lbcopy_5:
    beqz       $v0, .Lbcopy_8
    addiu      $at, $zero, 0x1
    beq        $v0, $at, .Lbcopy_7
    addiu      $at, $zero, 0x2
    beql       $v0, $at, .Lbcopy_6
    lh         $v0, 0x0($a0)
    lb         $v0, 0x0($a0)
    addiu      $a0, $a0, 0x1
    addiu      $a1, $a1, 0x1
    addiu      $a2, $a2, -0x1
    b          .Lbcopy_8
    sb         $v0, -0x1($a1)
    lh         $v0, 0x0($a0)
.Lbcopy_6:
    addiu      $a0, $a0, 0x2
    addiu      $a1, $a1, 0x2
    addiu      $a2, $a2, -0x2
    b          .Lbcopy_8
    sh         $v0, -0x2($a1)
.Lbcopy_7:
    lb         $v0, 0x0($a0)
    lh         $v1, 0x1($a0)
    addiu      $a0, $a0, 0x3
    addiu      $a1, $a1, 0x3
    addiu      $a2, $a2, -0x3
    sb         $v0, -0x3($a1)
    sh         $v1, -0x2($a1)
.Lbcopy_8:
    slti       $at, $a2, 0x20
    bnel       $at, $zero, .Lbcopy_10
    slti       $at, $a2, 0x10
    lw         $v0, 0x0($a0)
    lw         $v1, 0x4($a0)
    lw         $t0, 0x8($a0)
    lw         $t1, 0xC($a0)
    lw         $t2, 0x10($a0)
    lw         $t3, 0x14($a0)
    lw         $t4, 0x18($a0)
    lw         $t5, 0x1C($a0)
    addiu      $a0, $a0, 0x20
    addiu      $a1, $a1, 0x20
    addiu      $a2, $a2, -0x20
    sw         $v0, -0x20($a1)
    sw         $v1, -0x1C($a1)
    sw         $t0, -0x18($a1)
    sw         $t1, -0x14($a1)
    sw         $t2, -0x10($a1)
    sw         $t3, -0xC($a1)
    sw         $t4, -0x8($a1)
    b          .Lbcopy_8
    sw         $t5, -0x4($a1)
.Lbcopy_9:
    slti       $at, $a2, 0x10
.Lbcopy_10:
    bnel       $at, $zero, .Lbcopy_12
    slti       $at, $a2, 0x4
    lw         $v0, 0x0($a0)
    lw         $v1, 0x4($a0)
    lw         $t0, 0x8($a0)
    lw         $t1, 0xC($a0)
    addiu      $a0, $a0, 0x10
    addiu      $a1, $a1, 0x10
    addiu      $a2, $a2, -0x10
    sw         $v0, -0x10($a1)
    sw         $v1, -0xC($a1)
    sw         $t0, -0x8($a1)
    b          .Lbcopy_9
    sw         $t1, -0x4($a1)
.Lbcopy_11:
    slti       $at, $a2, 0x4
.Lbcopy_12:
    bnez       $at, .Lbcopy_2
    nop
    lw         $v0, 0x0($a0)
    addiu      $a0, $a0, 0x4
    addiu      $a1, $a1, 0x4
    addiu      $a2, $a2, -0x4
    b          .Lbcopy_11
    sw         $v0, -0x4($a1)
    slti       $at, $a2, 0x10
.Lbcopy_13:
    add        $a0, $a0, $a2
    bnez       $at, .Lbcopy_14
    add        $a1, $a1, $a2
    andi       $v0, $a0, 0x3
    andi       $v1, $a1, 0x3
    beq        $v0, $v1, .Lbcopy_16
    nop
.Lbcopy_14:
    beqz       $a2, .Lbcopy_4
    nop
    addiu      $a0, $a0, -0x1
    addiu      $a1, $a1, -0x1
    subu       $v1, $a0, $a2
.Lbcopy_15:
    lb         $v0, 0x0($a0)
    addiu      $a0, $a0, -0x1
    addiu      $a1, $a1, -0x1
    bne        $a0, $v1, .Lbcopy_15
    sb         $v0, 0x1($a1)
    jr         $ra
    or         $v0, $a3, $zero
.Lbcopy_16:
    beqz       $v0, .Lbcopy_19
    addiu      $at, $zero, 0x3
    beq        $v0, $at, .Lbcopy_18
    addiu      $at, $zero, 0x2
    beql       $v0, $at, .Lbcopy_17
    lh         $v0, -0x2($a0)
    lb         $v0, -0x1($a0)
    addiu      $a0, $a0, -0x1
    addiu      $a1, $a1, -0x1
    addiu      $a2, $a2, -0x1
    b          .Lbcopy_19
    sb         $v0, 0x0($a1)
    lh         $v0, -0x2($a0)
.Lbcopy_17:
    addiu      $a0, $a0, -0x2
    addiu      $a1, $a1, -0x2
    addiu      $a2, $a2, -0x2
    b          .Lbcopy_19
    sh         $v0, 0x0($a1)
.Lbcopy_18:
    lb         $v0, -0x1($a0)
    lh         $v1, -0x3($a0)
    addiu      $a0, $a0, -0x3
    addiu      $a1, $a1, -0x3
    addiu      $a2, $a2, -0x3
    sb         $v0, 0x2($a1)
    sh         $v1, 0x0($a1)
.Lbcopy_19:
    slti       $at, $a2, 0x20
    bnel       $at, $zero, .Lbcopy_21
    slti       $at, $a2, 0x10
    lw         $v0, -0x4($a0)
    lw         $v1, -0x8($a0)
    lw         $t0, -0xC($a0)
    lw         $t1, -0x10($a0)
    lw         $t2, -0x14($a0)
    lw         $t3, -0x18($a0)
    lw         $t4, -0x1C($a0)
    lw         $t5, -0x20($a0)
    addiu      $a0, $a0, -0x20
    addiu      $a1, $a1, -0x20
    addiu      $a2, $a2, -0x20
    sw         $v0, 0x1C($a1)
    sw         $v1, 0x18($a1)
    sw         $t0, 0x14($a1)
    sw         $t1, 0x10($a1)
    sw         $t2, 0xC($a1)
    sw         $t3, 0x8($a1)
    sw         $t4, 0x4($a1)
    b          .Lbcopy_19
    sw         $t5, 0x0($a1)
.Lbcopy_20:
    slti       $at, $a2, 0x10
.Lbcopy_21:
    bnel       $at, $zero, .Lbcopy_23
    slti       $at, $a2, 0x4
    lw         $v0, -0x4($a0)
    lw         $v1, -0x8($a0)
    lw         $t0, -0xC($a0)
    lw         $t1, -0x10($a0)
    addiu      $a0, $a0, -0x10
    addiu      $a1, $a1, -0x10
    addiu      $a2, $a2, -0x10
    sw         $v0, 0xC($a1)
    sw         $v1, 0x8($a1)
    sw         $t0, 0x4($a1)
    b          .Lbcopy_20
    sw         $t1, 0x0($a1)
.Lbcopy_22:
    slti       $at, $a2, 0x4
.Lbcopy_23:
    bnez       $at, .Lbcopy_14
    nop
    lw         $v0, -0x4($a0)
    addiu      $a0, $a0, -0x4
    addiu      $a1, $a1, -0x4
    addiu      $a2, $a2, -0x4
    b          .Lbcopy_22
    sw         $v0, 0x0($a1)
    .size bcopy, . - bcopy
