/*
 * The entry point of the Nintendo 64 releases: clear the BSS segment two words at a time, move the
 * stack to the boot stack, and jump to the OS boot.
 *
 * The instructions follow the original. Their delay slots are explicit, so the assembler must not
 * reorder them.
 */

    .set noreorder
    .set noat

    .text
    .balign 4

    .globl ramMain
    .type ramMain, @function
ramMain:
    lui        $t0, %hi(_bss_start)
    lui        $t1, %hi(_bss_size)
    addiu      $t0, $t0, %lo(_bss_start)
    /* The original has the size of its BSS as a constant, with ori; addiu pairs with %lo for any
     * size the link gives. */
    addiu      $t1, $t1, %lo(_bss_size)
.Lclear:
    addi       $t1, $t1, -0x8
    sw         $zero, 0x0($t0)
    sw         $zero, 0x4($t0)
    bnez       $t1, .Lclear
    addi       $t0, $t0, 0x8
    lui        $t2, %hi(OsBootEntry)
    lui        $sp, %hi(g_abBootStackTop)
    addiu      $t2, $t2, %lo(OsBootEntry)
    jr         $t2
    addiu      $sp, $sp, %lo(g_abBootStackTop)
    .size ramMain, . - ramMain
