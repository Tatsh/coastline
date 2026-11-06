/*
 * Set the interrupt mask, which combines the CPU interrupts of the Status register and the RCP
 * interrupts of the MI mask register. Return the previous mask in the same form. The global mask
 * limits both.
 */

    .set noreorder
    .set noat
    .set gp=64

    .equ MI_INTR_MASK_REG, 0xA430000C

    .text
    .balign 4

    .globl osSetIntMask
    .type osSetIntMask, @function
osSetIntMask:
    mfc0       $t4, $12
    andi       $v0, $t4, 0xFF01
    lui        $t0, %hi(__OSGlobalIntMask)
    addiu      $t0, $t0, %lo(__OSGlobalIntMask)
    lw         $t3, 0x0($t0)
    addiu      $at, $zero, -0x1
    xor        $t0, $t3, $at
    andi       $t0, $t0, 0xFF00
    or         $v0, $v0, $t0
    lui        $t2, %hi(MI_INTR_MASK_REG)
    lw         $t2, %lo(MI_INTR_MASK_REG)($t2)
    beqz       $t2, .LosSetIntMask_1
    srl        $t1, $t3, 16
    addiu      $at, $zero, -0x1
    xor        $t1, $t1, $at
    andi       $t1, $t1, 0x3F
    or         $t2, $t2, $t1
.LosSetIntMask_1:
    sll        $t2, $t2, 16
    or         $v0, $v0, $t2
    lui        $at, 0x3f
    and        $t0, $a0, $at
    and        $t0, $t0, $t3
    srl        $t0, $t0, 15
    lui        $t2, %hi(__osRcpImTable)
    addu       $t2, $t2, $t0
    lhu        $t2, %lo(__osRcpImTable)($t2)
    lui        $at, %hi(MI_INTR_MASK_REG)
    sw         $t2, %lo(MI_INTR_MASK_REG)($at)
    andi       $t0, $a0, 0xFF01
    andi       $t1, $t3, 0xFF00
    and        $t0, $t0, $t1
    lui        $at, 0xffff
    ori        $at, $at, 0xff
    and        $t4, $t4, $at
    or         $t4, $t4, $t0
    mtc0       $t4, $12
    nop
    nop
    jr         $ra
    nop
    .size osSetIntMask, . - osSetIntMask
