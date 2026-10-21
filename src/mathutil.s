/*
 * The hand-written conversion of a float to an integer. The instructions follow the original, with
 * the move from the FPU in the delay slot of the return.
 */

    .set noreorder

    .text
    .balign 4

/* Convert the float in f12 to an integer in v0, truncating towards zero. */
    .globl FloatToInt
    .type FloatToInt, @function
FloatToInt:
    trunc.w.s  $f12, $f12
    jr         $ra
    mfc1       $v0, $f12
    .size FloatToInt, . - FloatToInt
