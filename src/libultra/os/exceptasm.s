/*
 * The exception handler of libultra, after exceptasm.s of the SDK.
 *
 * __osExceptionPreamble is copied to the exception vectors by osInitialize() and jumps to
 * __osException. The handler saves the context of the running thread, dispatches every pending
 * interrupt to its handler through the tables below, posts the events the handlers raise with
 * send_mesg, and resumes the thread of highest priority through __osDispatchThread.
 *
 * The instructions follow the original. Their delay slots and hazard padding are explicit, so the
 * assembler must not reorder them.
 */

    .set noreorder
    .set noat
    .set gp=64

/* The RCP registers the handlers touch. */
    .equ SP_STATUS_REG, 0xA4040010
    .equ MI_MODE_REG, 0xA4300000
    .equ MI_INTR_REG, 0xA4300008
    .equ MI_INTR_MASK_REG, 0xA430000C
    .equ VI_CURRENT_REG, 0xA4400010
    .equ AI_STATUS_REG, 0xA450000C
    .equ PI_STATUS_REG, 0xA4600010
    .equ SI_STATUS_REG, 0xA4800018

/*
 * The handler of each interrupt, in the order of the offsets of __osIntOffTable. The data of the
 * original holds a copy of this table that refers to the original code.
 */
    .section .rodata
    .balign 4
.L__osIntTable:
    .word .Lredispatch
    .word .Lsw1
    .word .Lsw2
    .word .Lrcp
    .word .Lcart
    .word .Lprenmi
    .word .LIP6_Hdlr
    .word .LIP7_Hdlr
    .word .Lcounter

    .text
    .balign 4
    .globl __osExceptionPreamble
    .type __osExceptionPreamble, @function
__osExceptionPreamble:
    lui        $k0, %hi(__osException)
    addiu      $k0, $k0, %lo(__osException)
    jr         $k0
    nop
    .size __osExceptionPreamble, . - __osExceptionPreamble

    .globl __osException
    .type __osException, @function
__osException:
    lui        $k0, %hi(__osThreadSave)
    addiu      $k0, $k0, %lo(__osThreadSave)
    sd         $at, 0x20($k0)
    mfc0       $k1, $12
    sw         $k1, 0x118($k0)
    addiu      $at, $zero, -0x4
    and        $k1, $k1, $at
    mtc0       $k1, $12
    sd         $t0, 0x58($k0)
    sd         $t1, 0x60($k0)
    sd         $t2, 0x68($k0)
    sw         $zero, 0x18($k0)
    mfc0       $t0, $13
    or         $t0, $k0, $zero
    lui        $k0, %hi(__osRunningThread)
    lw         $k0, %lo(__osRunningThread)($k0)
    ld         $t1, 0x20($t0)
    sd         $t1, 0x20($k0)
    ld         $t1, 0x118($t0)
    sd         $t1, 0x118($k0)
    ld         $t1, 0x58($t0)
    sd         $t1, 0x58($k0)
    ld         $t1, 0x60($t0)
    sd         $t1, 0x60($k0)
    ld         $t1, 0x68($t0)
    sd         $t1, 0x68($k0)
    lw         $k1, 0x118($k0)
    mflo       $t0
    sd         $t0, 0x108($k0)
    mfhi       $t0
    andi       $t1, $k1, 0xFF00
    sd         $v0, 0x28($k0)
    sd         $v1, 0x30($k0)
    sd         $a0, 0x38($k0)
    sd         $a1, 0x40($k0)
    sd         $a2, 0x48($k0)
    sd         $a3, 0x50($k0)
    sd         $t3, 0x70($k0)
    sd         $t4, 0x78($k0)
    sd         $t5, 0x80($k0)
    sd         $t6, 0x88($k0)
    sd         $t7, 0x90($k0)
    sd         $s0, 0x98($k0)
    sd         $s1, 0xA0($k0)
    sd         $s2, 0xA8($k0)
    sd         $s3, 0xB0($k0)
    sd         $s4, 0xB8($k0)
    sd         $s5, 0xC0($k0)
    sd         $s6, 0xC8($k0)
    sd         $s7, 0xD0($k0)
    sd         $t8, 0xD8($k0)
    sd         $t9, 0xE0($k0)
    sd         $gp, 0xE8($k0)
    sd         $sp, 0xF0($k0)
    sd         $fp, 0xF8($k0)
    sd         $ra, 0x100($k0)
    beqz       $t1, .Lsavercp
    sd         $t0, 0x110($k0)
    lui        $t0, %hi(__OSGlobalIntMask)
    addiu      $t0, $t0, %lo(__OSGlobalIntMask)
    lw         $t0, 0x0($t0)
    addiu      $at, $zero, -0x1
    xor        $t2, $t0, $at
    lui        $at, (0xFFFF00FF >> 16)
    andi       $t2, $t2, 0xFF00
    ori        $at, $at, (0xFFFF00FF & 0xFFFF)
    or         $t4, $t1, $t2
    and        $t3, $k1, $at
    andi       $t0, $t0, 0xFF00
    or         $t3, $t3, $t4
    and        $t1, $t1, $t0
    and        $k1, $k1, $at
    sw         $t3, 0x118($k0)
    or         $k1, $k1, $t1
.Lsavercp:
    lui        $t1, %hi(MI_INTR_MASK_REG)
    lw         $t1, %lo(MI_INTR_MASK_REG)($t1)
    beqz       $t1, .Lsaverest
    nop
    lui        $t0, %hi(__OSGlobalIntMask)
    addiu      $t0, $t0, %lo(__OSGlobalIntMask)
    lw         $t0, 0x0($t0)
    lw         $t4, 0x128($k0)
    addiu      $at, $zero, -0x1
    srl        $t0, $t0, 16
    xor        $t0, $t0, $at
    andi       $t0, $t0, 0x3F
    and        $t0, $t0, $t4
    or         $t1, $t1, $t0
.Lsaverest:
    sw         $t1, 0x128($k0)
    mfc0       $t0, $14
    sw         $t0, 0x11C($k0)
    lw         $t0, 0x18($k0)
    beqz       $t0, .Lsavecause
    nop
    cfc1       $t0, $31
    nop
    sw         $t0, 0x12C($k0)
    sdc1       $f0, 0x130($k0)
    sdc1       $f2, 0x138($k0)
    sdc1       $f4, 0x140($k0)
    sdc1       $f6, 0x148($k0)
    sdc1       $f8, 0x150($k0)
    sdc1       $f10, 0x158($k0)
    sdc1       $f12, 0x160($k0)
    sdc1       $f14, 0x168($k0)
    sdc1       $f16, 0x170($k0)
    sdc1       $f18, 0x178($k0)
    sdc1       $f20, 0x180($k0)
    sdc1       $f22, 0x188($k0)
    sdc1       $f24, 0x190($k0)
    sdc1       $f26, 0x198($k0)
    sdc1       $f28, 0x1A0($k0)
    sdc1       $f30, 0x1A8($k0)
.Lsavecause:
    mfc0       $t0, $13
    sw         $t0, 0x120($k0)
    addiu      $t1, $zero, 0x2
    sh         $t1, 0x10($k0)
    andi       $t1, $t0, 0x7C
    addiu      $t2, $zero, 0x24
    beq        $t1, $t2, .Lhandle_break
    nop
    addiu      $t2, $zero, 0x2C
    beq        $t1, $t2, .Lhandle_CpU
    nop
    addiu      $t2, $zero, 0x0
    bne        $t1, $t2, .Lpanic
    nop
    and        $s0, $k1, $t0
.Lnext_interrupt:
    andi       $t1, $s0, 0xFF00
    srl        $t2, $t1, 12
    bnez       $t2, .Lindex_found
    nop
    srl        $t2, $t1, 8
    addi       $t2, $t2, 0x10
.Lindex_found:
    lui        $at, %hi(__osIntOffTable)
    addu       $at, $at, $t2
    lbu        $t2, %lo(__osIntOffTable)($at)
    lui        $at, %hi(.L__osIntTable)
    addu       $at, $at, $t2
    lw         $t2, %lo(.L__osIntTable)($at)
    jr         $t2
    nop
.LIP6_Hdlr:
    addiu      $at, $zero, -0x2001
    b          .Lnext_interrupt
    and        $s0, $s0, $at
.LIP7_Hdlr:
    addiu      $at, $zero, -0x4001
    b          .Lnext_interrupt
    and        $s0, $s0, $at
.Lcounter:
    mfc0       $t1, $11
    mtc0       $t1, $11
    jal        send_mesg
    addiu      $a0, $zero, 0x18
    lui        $at, (0xFFFF7FFF >> 16)
    ori        $at, $at, (0xFFFF7FFF & 0xFFFF)
    b          .Lnext_interrupt
    and        $s0, $s0, $at
.Lcart:
    addiu      $at, $zero, -0x801
    and        $s0, $s0, $at
    addiu      $t2, $zero, 0x4
    lui        $at, %hi(__osHwIntTable)
    addu       $at, $at, $t2
    lw         $t2, %lo(__osHwIntTable)($at)
    lui        $sp, %hi(leoDiskStack)
    addiu      $sp, $sp, %lo(leoDiskStack)
    addiu      $a0, $zero, 0x10
    beqz       $t2, .Lcart_send
    addiu      $sp, $sp, 0xFF0
    jalr       $t2
    nop
    beqz       $v0, .Lcart_send
    addiu      $a0, $zero, 0x10
    b          .Lredispatch
    nop
.Lcart_send:
    jal        send_mesg
    nop
    b          .Lnext_interrupt
    nop
.Lrcp:
    lui        $t0, %hi(__OSGlobalIntMask)
    addiu      $t0, $t0, %lo(__OSGlobalIntMask)
    lw         $t0, 0x0($t0)
    lui        $s1, %hi(MI_INTR_REG)
    lw         $s1, %lo(MI_INTR_REG)($s1)
    srl        $t0, $t0, 16
    and        $s1, $s1, $t0
    andi       $t1, $s1, 0x1
    beqz       $t1, .Lvi
    nop
    lui        $t4, %hi(SP_STATUS_REG)
    lw         $t4, %lo(SP_STATUS_REG)($t4)
    addiu      $t1, $zero, 0x8
    lui        $at, %hi(SP_STATUS_REG)
    andi       $t4, $t4, 0x300
    andi       $s1, $s1, 0x3E
    beqz       $t4, .Lsp_other_break
    sw         $t1, %lo(SP_STATUS_REG)($at)
    jal        send_mesg
    addiu      $a0, $zero, 0x20
    beqz       $s1, .Lrcp_done
    nop
    b          .Lvi
    nop
.Lsp_other_break:
    jal        send_mesg
    addiu      $a0, $zero, 0x58
    beqz       $s1, .Lrcp_done
    nop
.Lvi:
    andi       $t1, $s1, 0x8
    beqz       $t1, .Lai
    lui        $at, %hi(VI_CURRENT_REG)
    andi       $s1, $s1, 0x37
    sw         $zero, %lo(VI_CURRENT_REG)($at)
    jal        send_mesg
    addiu      $a0, $zero, 0x38
    beqz       $s1, .Lrcp_done
    nop
.Lai:
    andi       $t1, $s1, 0x4
    beqz       $t1, .Lsi
    nop
    addiu      $t1, $zero, 0x1
    lui        $at, %hi(AI_STATUS_REG)
    andi       $s1, $s1, 0x3B
    sw         $t1, %lo(AI_STATUS_REG)($at)
    jal        send_mesg
    addiu      $a0, $zero, 0x30
    beqz       $s1, .Lrcp_done
    nop
.Lsi:
    andi       $t1, $s1, 0x2
    beqz       $t1, .Lpi
    lui        $at, %hi(SI_STATUS_REG)
    andi       $s1, $s1, 0x3D
    sw         $zero, %lo(SI_STATUS_REG)($at)
    jal        send_mesg
    addiu      $a0, $zero, 0x28
    beqz       $s1, .Lrcp_done
    nop
.Lpi:
    andi       $t1, $s1, 0x10
    beqz       $t1, .Ldp
    nop
    addiu      $t1, $zero, 0x2
    lui        $at, %hi(PI_STATUS_REG)
    andi       $s1, $s1, 0x2F
    sw         $t1, %lo(PI_STATUS_REG)($at)
    jal        send_mesg
    addiu      $a0, $zero, 0x40
    beqz       $s1, .Lrcp_done
    nop
.Ldp:
    andi       $t1, $s1, 0x20
    beqz       $t1, .Lrcp_done
    nop
    addiu      $t1, $zero, 0x800
    lui        $at, %hi(MI_MODE_REG)
    andi       $s1, $s1, 0x1F
    sw         $t1, %lo(MI_MODE_REG)($at)
    jal        send_mesg
    addiu      $a0, $zero, 0x48
.Lrcp_done:
    addiu      $at, $zero, -0x401
    b          .Lnext_interrupt
    and        $s0, $s0, $at
.Lprenmi:
    lw         $k1, 0x118($k0)
    addiu      $at, $zero, -0x1001
    lui        $t1, %hi(__osShutdown)
    and        $k1, $k1, $at
    sw         $k1, 0x118($k0)
    addiu      $t1, $t1, %lo(__osShutdown)
    lw         $t2, 0x0($t1)
    beqz       $t2, .Lfirst_prenmi
    addiu      $at, $zero, -0x1001
    b          .Lredispatch
    and        $s0, $s0, $at
.Lfirst_prenmi:
    addiu      $t2, $zero, 0x1
    sw         $t2, 0x0($t1)
    jal        send_mesg
    addiu      $a0, $zero, 0x70
    lui        $t2, %hi(__osRunQueue)
    lw         $t2, %lo(__osRunQueue)($t2)
    addiu      $at, $zero, -0x1001
    and        $s0, $s0, $at
    lw         $k1, 0x118($t2)
    and        $k1, $k1, $at
    b          .Lredispatch
    sw         $k1, 0x118($t2)
.Lsw2:
    addiu      $at, $zero, -0x201
    and        $t0, $t0, $at
    mtc0       $t0, $13
    jal        send_mesg
    addiu      $a0, $zero, 0x8
    addiu      $at, $zero, -0x201
    b          .Lnext_interrupt
    and        $s0, $s0, $at
.Lsw1:
    addiu      $at, $zero, -0x101
    and        $t0, $t0, $at
    mtc0       $t0, $13
    jal        send_mesg
    addiu      $a0, $zero, 0x0
    addiu      $at, $zero, -0x101
    b          .Lnext_interrupt
    and        $s0, $s0, $at
.Lhandle_break:
    addiu      $t1, $zero, 0x1
    sh         $t1, 0x12($k0)
    jal        send_mesg
    addiu      $a0, $zero, 0x50
    b          .Lredispatch
    nop
.Lredispatch:
    lui        $t2, %hi(__osRunQueue)
    lw         $t2, %lo(__osRunQueue)($t2)
    lw         $t1, 0x4($k0)
    lw         $t3, 0x4($t2)
    slt        $at, $t1, $t3
    beqz       $at, .LenqueueRunning
    nop
    lui        $a0, %hi(__osRunQueue)
    or         $a1, $k0, $zero
    jal        __osEnqueueThread
    addiu      $a0, $a0, %lo(__osRunQueue)
    j          __osDispatchThread
    nop
.LenqueueRunning:
    lui        $t1, %hi(__osRunQueue)
    addiu      $t1, $t1, %lo(__osRunQueue)
    lw         $t2, 0x0($t1)
    sw         $t2, 0x0($k0)
    j          __osDispatchThread
    sw         $k0, 0x0($t1)
.Lpanic:
    lui        $at, %hi(__osFaultedThread)
    sw         $k0, %lo(__osFaultedThread)($at)
    addiu      $t1, $zero, 0x1
    sh         $t1, 0x10($k0)
    addiu      $t1, $zero, 0x2
    sh         $t1, 0x12($k0)
    mfc0       $t2, $8
    sw         $t2, 0x124($k0)
    jal        send_mesg
    addiu      $a0, $zero, 0x60
    j          __osDispatchThread
    nop
    .size __osException, . - __osException

    # @ghidraAddress USA: 0x8026b204
    # @ghidraAddress EUR: 0x8026d094
    # @ghidraAddress JPN: 0x8026cb44
    .globl send_mesg
    .type send_mesg, @function
send_mesg:
    lui        $t2, %hi(__osEventStateTab)
    addiu      $t2, $t2, %lo(__osEventStateTab)
    addu       $t2, $t2, $a0
    lw         $t1, 0x0($t2)
    or         $s2, $ra, $zero
    beqz       $t1, .Lsend_done
    nop
    lw         $t3, 0x8($t1)
    lw         $t4, 0x10($t1)
    slt        $at, $t3, $t4
    beqz       $at, .Lsend_done
    nop
    lw         $t5, 0xC($t1)
    addu       $t5, $t5, $t3
    div        $zero, $t5, $t4
    bnez       $t4, .Lsend_mesg_div
    nop
    break      7
.Lsend_mesg_div:
    addiu      $at, $zero, -0x1
    bne        $t4, $at, .Lsend_mesg_store
    lui        $at, (0x80000000 >> 16)
    bne        $t5, $at, .Lsend_mesg_store
    nop
    break      6
.Lsend_mesg_store:
    lw         $t4, 0x14($t1)
    mfhi       $t5
    sll        $t5, $t5, 2
    addu       $t4, $t4, $t5
    lw         $t5, 0x4($t2)
    addiu      $t2, $t3, 0x1
    sw         $t5, 0x0($t4)
    sw         $t2, 0x8($t1)
    lw         $t2, 0x0($t1)
    lw         $t3, 0x0($t2)
    beqz       $t3, .Lsend_done
    nop
    jal        __osPopThread
    or         $a0, $t1, $zero
    or         $t2, $v0, $zero
    lui        $a0, %hi(__osRunQueue)
    or         $a1, $t2, $zero
    jal        __osEnqueueThread
    addiu      $a0, $a0, %lo(__osRunQueue)
.Lsend_done:
    jr         $s2
    nop
.Lhandle_CpU:
    lui        $at, (0x30000000 >> 16)
    and        $t1, $t0, $at
    srl        $t1, $t1, 28
    addiu      $t2, $zero, 0x1
    bne        $t1, $t2, .Lpanic
    nop
    lw         $k1, 0x118($k0)
    lui        $at, (0x20000000 >> 16)
    addiu      $t1, $zero, 0x1
    or         $k1, $k1, $at
    sw         $t1, 0x18($k0)
    b          .LenqueueRunning
    sw         $k1, 0x118($k0)
    .size send_mesg, . - send_mesg

    .globl __osEnqueueAndYield
    .type __osEnqueueAndYield, @function
__osEnqueueAndYield:
    lui        $a1, %hi(__osRunningThread)
    lw         $a1, %lo(__osRunningThread)($a1)
    mfc0       $t0, $12
    lw         $k1, 0x18($a1)
    ori        $t0, $t0, 0x2
    sw         $t0, 0x118($a1)
    sd         $s0, 0x98($a1)
    sd         $s1, 0xA0($a1)
    sd         $s2, 0xA8($a1)
    sd         $s3, 0xB0($a1)
    sd         $s4, 0xB8($a1)
    sd         $s5, 0xC0($a1)
    sd         $s6, 0xC8($a1)
    sd         $s7, 0xD0($a1)
    sd         $gp, 0xE8($a1)
    sd         $sp, 0xF0($a1)
    sd         $fp, 0xF8($a1)
    sd         $ra, 0x100($a1)
    beqz       $k1, .Lyield_sr
    sw         $ra, 0x11C($a1)
    cfc1       $k1, $31
    sdc1       $f20, 0x180($a1)
    sdc1       $f22, 0x188($a1)
    sdc1       $f24, 0x190($a1)
    sdc1       $f26, 0x198($a1)
    sdc1       $f28, 0x1A0($a1)
    sdc1       $f30, 0x1A8($a1)
    sw         $k1, 0x12C($a1)
.Lyield_sr:
    lw         $k1, 0x118($a1)
    andi       $t1, $k1, 0xFF00
    beqz       $t1, .Lyield_rcp
    nop
    lui        $t0, %hi(__OSGlobalIntMask)
    addiu      $t0, $t0, %lo(__OSGlobalIntMask)
    lw         $t0, 0x0($t0)
    addiu      $at, $zero, -0x1
    xor        $t0, $t0, $at
    lui        $at, (0xFFFF00FF >> 16)
    andi       $t0, $t0, 0xFF00
    ori        $at, $at, (0xFFFF00FF & 0xFFFF)
    or         $t1, $t1, $t0
    and        $k1, $k1, $at
    or         $k1, $k1, $t1
    sw         $k1, 0x118($a1)
.Lyield_rcp:
    lui        $k1, %hi(MI_INTR_MASK_REG)
    lw         $k1, %lo(MI_INTR_MASK_REG)($k1)
    beqz       $k1, .Lyield_enqueue
    nop
    lui        $k0, %hi(__OSGlobalIntMask)
    addiu      $k0, $k0, %lo(__OSGlobalIntMask)
    lw         $k0, 0x0($k0)
    lw         $t0, 0x128($a1)
    addiu      $at, $zero, -0x1
    srl        $k0, $k0, 16
    xor        $k0, $k0, $at
    andi       $k0, $k0, 0x3F
    and        $k0, $k0, $t0
    or         $k1, $k1, $k0
.Lyield_enqueue:
    beqz       $a0, .Lyield_dispatch
    sw         $k1, 0x128($a1)
    jal        __osEnqueueThread
    nop
.Lyield_dispatch:
    j          __osDispatchThread
    nop
    .size __osEnqueueAndYield, . - __osEnqueueAndYield

    .globl __osEnqueueThread
    .type __osEnqueueThread, @function
__osEnqueueThread:
    lw         $t8, 0x0($a0)
    lw         $t7, 0x4($a1)
    or         $t9, $a0, $zero
    lw         $t6, 0x4($t8)
    slt        $at, $t6, $t7
    bnez       $at, .Lenqueue_insert
    nop
.Lenqueue_scan:
    or         $t9, $t8, $zero
    lw         $t8, 0x0($t8)
    lw         $t6, 0x4($t8)
    slt        $at, $t6, $t7
    beqz       $at, .Lenqueue_scan
    nop
.Lenqueue_insert:
    lw         $t8, 0x0($t9)
    sw         $t8, 0x0($a1)
    sw         $a1, 0x0($t9)
    jr         $ra
    sw         $a0, 0x8($a1)
    .size __osEnqueueThread, . - __osEnqueueThread

    .globl __osPopThread
    .type __osPopThread, @function
__osPopThread:
    lw         $v0, 0x0($a0)
    lw         $t9, 0x0($v0)
    jr         $ra
    sw         $t9, 0x0($a0)
    .size __osPopThread, . - __osPopThread

    .globl __osDispatchThread
    .type __osDispatchThread, @function
__osDispatchThread:
    lui        $a0, %hi(__osRunQueue)
    jal        __osPopThread
    addiu      $a0, $a0, %lo(__osRunQueue)
    lui        $at, %hi(__osRunningThread)
    sw         $v0, %lo(__osRunningThread)($at)
    addiu      $t0, $zero, 0x4
    sh         $t0, 0x10($v0)
    or         $k0, $v0, $zero
    lui        $t0, %hi(__OSGlobalIntMask)
    lw         $k1, 0x118($k0)
    addiu      $t0, $t0, %lo(__OSGlobalIntMask)
    lw         $t0, 0x0($t0)
    lui        $at, (0xFFFF00FF >> 16)
    andi       $t1, $k1, 0xFF00
    ori        $at, $at, (0xFFFF00FF & 0xFFFF)
    andi       $t0, $t0, 0xFF00
    and        $t1, $t1, $t0
    and        $k1, $k1, $at
    or         $k1, $k1, $t1
    mtc0       $k1, $12
    ld         $k1, 0x108($k0)
    ld         $at, 0x20($k0)
    ld         $v0, 0x28($k0)
    mtlo       $k1
    ld         $k1, 0x110($k0)
    ld         $v1, 0x30($k0)
    ld         $a0, 0x38($k0)
    ld         $a1, 0x40($k0)
    ld         $a2, 0x48($k0)
    ld         $a3, 0x50($k0)
    ld         $t0, 0x58($k0)
    ld         $t1, 0x60($k0)
    ld         $t2, 0x68($k0)
    ld         $t3, 0x70($k0)
    ld         $t4, 0x78($k0)
    ld         $t5, 0x80($k0)
    ld         $t6, 0x88($k0)
    ld         $t7, 0x90($k0)
    ld         $s0, 0x98($k0)
    ld         $s1, 0xA0($k0)
    ld         $s2, 0xA8($k0)
    ld         $s3, 0xB0($k0)
    ld         $s4, 0xB8($k0)
    ld         $s5, 0xC0($k0)
    ld         $s6, 0xC8($k0)
    ld         $s7, 0xD0($k0)
    ld         $t8, 0xD8($k0)
    ld         $t9, 0xE0($k0)
    ld         $gp, 0xE8($k0)
    mthi       $k1
    ld         $sp, 0xF0($k0)
    ld         $fp, 0xF8($k0)
    ld         $ra, 0x100($k0)
    lw         $k1, 0x11C($k0)
    mtc0       $k1, $14
    lw         $k1, 0x18($k0)
    beqz       $k1, .Lrestore_rcp
    nop
    lw         $k1, 0x12C($k0)
    ctc1       $k1, $31
    ldc1       $f0, 0x130($k0)
    ldc1       $f2, 0x138($k0)
    ldc1       $f4, 0x140($k0)
    ldc1       $f6, 0x148($k0)
    ldc1       $f8, 0x150($k0)
    ldc1       $f10, 0x158($k0)
    ldc1       $f12, 0x160($k0)
    ldc1       $f14, 0x168($k0)
    ldc1       $f16, 0x170($k0)
    ldc1       $f18, 0x178($k0)
    ldc1       $f20, 0x180($k0)
    ldc1       $f22, 0x188($k0)
    ldc1       $f24, 0x190($k0)
    ldc1       $f26, 0x198($k0)
    ldc1       $f28, 0x1A0($k0)
    ldc1       $f30, 0x1A8($k0)
.Lrestore_rcp:
    lw         $k1, 0x128($k0)
    lui        $k0, %hi(__OSGlobalIntMask)
    addiu      $k0, $k0, %lo(__OSGlobalIntMask)
    lw         $k0, 0x0($k0)
    srl        $k0, $k0, 16
    and        $k1, $k1, $k0
    sll        $k1, $k1, 1
    lui        $k0, %hi(__osRcpImTable)
    addiu      $k0, $k0, %lo(__osRcpImTable)
    addu       $k1, $k1, $k0
    lhu        $k1, 0x0($k1)
    lui        $k0, %hi(MI_INTR_MASK_REG)
    addiu      $k0, $k0, %lo(MI_INTR_MASK_REG)
    sw         $k1, 0x0($k0)
    nop
    nop
    nop
    nop
    eret
    .size __osDispatchThread, . - __osDispatchThread

    .globl __osCleanupThread
    .type __osCleanupThread, @function
__osCleanupThread:
    jal        osDestroyThread
    or         $a0, $zero, $zero
    nop
    nop
    .size __osCleanupThread, . - __osCleanupThread
