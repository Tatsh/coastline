/*
 * Compare two buffers of a2 bytes, returning 0 when they are equal and 1 otherwise. Buffers of
 * the same alignment are compared a word at a time, and other buffers with unaligned word loads.
 * The partial first word of aligned buffers is compared with v1 merged into its unloaded bytes, as
 * the original does.
 */

    .set noreorder
    .set noat
    .set gp=64

    .text
    .balign 4

    .globl bcmp
    .type bcmp, @function
bcmp:
    slti       $at, $a2, 0x10
    bnez       $at, .Lbcmp_7
    xor        $v0, $a0, $a1
    andi       $v0, $v0, 0x3
    bnez       $v0, .Lbcmp_3
    negu       $t8, $a0
    andi       $t8, $t8, 0x3
    beqz       $t8, .Lbcmp_1
    subu       $a2, $a2, $t8
    or         $v0, $v1, $zero
    lwl        $v0, 0x0($a0)
    lwl        $v1, 0x0($a1)
    addu       $a0, $a0, $t8
    addu       $a1, $a1, $t8
    bne        $v0, $v1, .Lbcmp_10
.Lbcmp_1:
    addiu      $at, $zero, -0x4
    and        $a3, $a2, $at
    beqz       $a3, .Lbcmp_7
    subu       $a2, $a2, $a3
    addu       $a3, $a3, $a0
    lw         $v0, 0x0($a0)
.Lbcmp_2:
    lw         $v1, 0x0($a1)
    addiu      $a0, $a0, 0x4
    addiu      $a1, $a1, 0x4
    bne        $v0, $v1, .Lbcmp_10
    nop
    bnel       $a0, $a3, .Lbcmp_2
    lw         $v0, 0x0($a0)
    b          .Lbcmp_7
    nop
.Lbcmp_3:
    negu       $a3, $a1
    andi       $a3, $a3, 0x3
    beqz       $a3, .Lbcmp_5
    subu       $a2, $a2, $a3
    addu       $a3, $a3, $a0
    lbu        $v0, 0x0($a0)
.Lbcmp_4:
    lbu        $v1, 0x0($a1)
    addiu      $a0, $a0, 0x1
    addiu      $a1, $a1, 0x1
    bne        $v0, $v1, .Lbcmp_10
    nop
    bnel       $a0, $a3, .Lbcmp_4
    lbu        $v0, 0x0($a0)
.Lbcmp_5:
    addiu      $at, $zero, -0x4
    and        $a3, $a2, $at
    beqz       $a3, .Lbcmp_7
    subu       $a2, $a2, $a3
    addu       $a3, $a3, $a0
    lwl        $v0, 0x0($a0)
.Lbcmp_6:
    lw         $v1, 0x0($a1)
    lwr        $v0, 0x3($a0)
    addiu      $a0, $a0, 0x4
    addiu      $a1, $a1, 0x4
    bne        $v0, $v1, .Lbcmp_10
    nop
    bnel       $a0, $a3, .Lbcmp_6
    lwl        $v0, 0x0($a0)
.Lbcmp_7:
    blez       $a2, .Lbcmp_9
    addu       $a3, $a2, $a0
    lbu        $v0, 0x0($a0)
.Lbcmp_8:
    lbu        $v1, 0x0($a1)
    addiu      $a0, $a0, 0x1
    addiu      $a1, $a1, 0x1
    bne        $v0, $v1, .Lbcmp_10
    nop
    bnel       $a0, $a3, .Lbcmp_8
    lbu        $v0, 0x0($a0)
.Lbcmp_9:
    jr         $ra
    or         $v0, $zero, $zero
.Lbcmp_10:
    jr         $ra
    addiu      $v0, $zero, 0x1
    .size bcmp, . - bcmp
