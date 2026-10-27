#include "mathutil.h"

#ifdef __mips__
/*
 * The original is hand-written, with the move from the FPU in the delay slot of the return, which
 * a compiled body does not reproduce.
 */
__asm__(".set push\n"
        ".set noreorder\n"
        ".text\n"
        ".balign 4\n"
        ".globl FloatToInt\n"
        ".type FloatToInt, @function\n"
        "FloatToInt:\n"
        "    trunc.w.s $f12, $f12\n"
        "    jr $ra\n"
        "    mfc1 $v0, $f12\n"
        ".size FloatToInt, . - FloatToInt\n"
        ".set pop\n");
#else
s32 FloatToInt(float value) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    return (s32)(s64)value;
#else
    return (s32)value;
#endif
}
#endif /* __mips__ */
