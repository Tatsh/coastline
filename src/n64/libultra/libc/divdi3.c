#include <ultra64.h>

/* Code IDO compiles calls the routines of ll.c for 64-bit division, and none of these. */
#ifndef __sgi

/*
 * GCC calls these for 64-bit division under the 32-bit ABI. The software versions of libgcc are
 * several kilobytes, while the VR4300 divides 64-bit registers in one instruction, as the code of
 * IDO did. The exception handler saves whole 64-bit registers, so the upper halves remain intact
 * across an interrupt. Each routine joins the halves of both operands in temporaries, divides, and
 * splits the result back into sign-extended halves. Division by zero gives an undefined result.
 */
#define DIV64(insn, move, a, b)                                                                    \
    ({                                                                                             \
        u32 hi_, lo_;                                                                              \
        __asm__("dsll32 $8, %2, 0\n\t"                                                             \
                "dsll32 $9, %3, 0\n\t"                                                             \
                "dsrl32 $9, $9, 0\n\t"                                                             \
                "or $8, $8, $9\n\t"                                                                \
                "dsll32 $9, %4, 0\n\t"                                                             \
                "dsll32 $10, %5, 0\n\t"                                                            \
                "dsrl32 $10, $10, 0\n\t"                                                           \
                "or $9, $9, $10\n\t" insn " $0, $8, $9\n\t" move " $8\n\t"                         \
                "dsra32 %0, $8, 0\n\t"                                                             \
                "dsll32 $8, $8, 0\n\t"                                                             \
                "dsra32 %1, $8, 0"                                                                 \
                : "=r"(hi_), "=r"(lo_)                                                             \
                : "r"((u32)((a) >> 32)), "r"((u32)(a)), "r"((u32)((b) >> 32)), "r"((u32)(b))       \
                : "$8", "$9", "$10", "hi", "lo");                                                  \
        ((u64)hi_ << 32) | lo_;                                                                    \
    })

u64 __udivdi3(u64 a, u64 b) {
    return DIV64("ddivu", "mflo", a, b);
}

u64 __umoddi3(u64 a, u64 b) {
    return DIV64("ddivu", "mfhi", a, b);
}

s64 __divdi3(s64 a, s64 b) {
    return DIV64("ddiv", "mflo", a, b);
}

s64 __moddi3(s64 a, s64 b) {
    return DIV64("ddiv", "mfhi", a, b);
}
#endif /* __sgi */
