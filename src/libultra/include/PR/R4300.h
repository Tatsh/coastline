#ifndef PR_R4300_H
#define PR_R4300_H

/**
 * @file
 * Address segments, coprocessor 0 registers, and status bits of the VR4300.
 */

/* Bases of the address segments. */
#define KUSEG 0x00000000  /*!< The user segment, mapped through the TLB. */
#define K0BASE 0x80000000 /*!< The kernel segment, unmapped and cached. */
#define K1BASE 0xA0000000 /*!< The kernel segment, unmapped and uncached. */
#define K2BASE 0xC0000000 /*!< The supervisor segment, mapped through the TLB. */

/** Whether address x lies in the cached unmapped segment. */
#define IS_KSEG0(x) ((u32)(x) >= K0BASE && (u32)(x) < K1BASE)
/** Whether address x lies in the uncached unmapped segment. */
#define IS_KSEG1(x) ((u32)(x) >= K1BASE && (u32)(x) < K2BASE)

/* Conversions between the segments and physical addresses. */
#define K0_TO_K1(x) ((u32)(x) | 0xA0000000)    /*!< The uncached alias of a cached address. */
#define K1_TO_K0(x) ((u32)(x) & 0x9FFFFFFF)    /*!< The cached alias of an uncached address. */
#define K0_TO_PHYS(x) ((u32)(x) & 0x1FFFFFFF)  /*!< The physical address of a cached address. */
#define K1_TO_PHYS(x) ((u32)(x) & 0x1FFFFFFF)  /*!< The physical address of an uncached address. */
#define KDM_TO_PHYS(x) ((u32)(x) & 0x1FFFFFFF) /*!< The physical address of a kernel address. */
#define PHYS_TO_K0(x) ((u32)(x) | 0x80000000)  /*!< The cached address of a physical address. */
#define PHYS_TO_K1(x) ((u32)(x) | 0xA0000000)  /*!< The uncached address of a physical address. */

/* Cache geometry. */
#define ICACHE_SIZE 0x4000 /*!< The size of the instruction cache in bytes. */
#define ICACHE_LINESIZE 32 /*!< The size of an instruction cache line in bytes. */
#define DCACHE_SIZE 0x2000 /*!< The size of the data cache in bytes. */
#define DCACHE_LINESIZE 16 /*!< The size of a data cache line in bytes. */

/* Status register. */
#define SR_IE 0x00000001      /*!< Interrupts are enabled. */
#define SR_EXL 0x00000002     /*!< The exception level. */
#define SR_ERL 0x00000004     /*!< The error level. */
#define SR_KSU_USR 0x00000010 /*!< User mode. */
#define SR_IMASK 0x0000FF00   /*!< The bits of the interrupt mask. */
#define SR_IBIT1 0x00000100   /*!< Software interrupt 0 is enabled. */
#define SR_IBIT2 0x00000200   /*!< Software interrupt 1 is enabled. */
#define SR_IBIT3 0x00000400   /*!< Interrupt 0, the RCP, is enabled. */
#define SR_IBIT4 0x00000800   /*!< Interrupt 1, the cartridge, is enabled. */
#define SR_IBIT5 0x00001000   /*!< Interrupt 2, the pre-NMI, is enabled. */
#define SR_IBIT6 0x00002000   /*!< Interrupt 3 is enabled. */
#define SR_IBIT7 0x00004000   /*!< Interrupt 4 is enabled. */
#define SR_IBIT8 0x00008000   /*!< Interrupt 5, the timer, is enabled. */
#define SR_IMASK0 0x0000FF00  /*!< The mask enabling every interrupt. */
#define SR_IMASK1 0x0000FE00  /*!< The mask enabling interrupts above level 1. */
#define SR_IMASK2 0x0000FC00  /*!< The mask enabling interrupts above level 2. */
#define SR_IMASK3 0x0000F800  /*!< The mask enabling interrupts above level 3. */
#define SR_IMASK4 0x0000F000  /*!< The mask enabling interrupts above level 4. */
#define SR_IMASK5 0x0000E000  /*!< The mask enabling interrupts above level 5. */
#define SR_IMASK6 0x0000C000  /*!< The mask enabling interrupts above level 6. */
#define SR_IMASK7 0x00008000  /*!< The mask enabling the timer interrupt only. */
#define SR_IMASK8 0x00000000  /*!< The mask disabling every interrupt. */
#define SR_DE 0x00010000      /*!< Cache parity errors raise no exception. */
#define SR_CE 0x00020000      /*!< The ECC register replaces the cache check bits. */
#define SR_CH 0x00040000      /*!< A cache instruction hit. */
#define SR_SR 0x00100000      /*!< A soft reset or NMI occurred. */
#define SR_TS 0x00200000      /*!< A TLB shutdown occurred. */
#define SR_BEV 0x00400000     /*!< Exceptions use the bootstrap vectors. */
#define SR_ITS 0x01000000     /*!< Instruction trace support is enabled. */
#define SR_RE 0x02000000      /*!< User mode runs with reversed endianness. */
#define SR_FR 0x04000000      /*!< Thirty-two 64-bit floating-point registers are enabled. */
#define SR_RP 0x08000000      /*!< Reduced power mode. */
#define SR_CU0 0x10000000     /*!< Coprocessor 0 is usable in user mode. */
#define SR_CU1 0x20000000     /*!< Coprocessor 1, the floating-point unit, is usable. */

/* Cause register. */
#define CAUSE_BD 0x80000000      /*!< The exception occurred in a branch delay slot. */
#define CAUSE_IP8 0x00008000     /*!< Interrupt 5, the timer, is pending. */
#define CAUSE_IP7 0x00004000     /*!< Interrupt 4 is pending. */
#define CAUSE_IP6 0x00002000     /*!< Interrupt 3 is pending. */
#define CAUSE_IP5 0x00001000     /*!< Interrupt 2, the pre-NMI, is pending. */
#define CAUSE_IP4 0x00000800     /*!< Interrupt 1, the cartridge, is pending. */
#define CAUSE_IP3 0x00000400     /*!< Interrupt 0, the RCP, is pending. */
#define CAUSE_SW2 0x00000200     /*!< Software interrupt 1 is pending. */
#define CAUSE_SW1 0x00000100     /*!< Software interrupt 0 is pending. */
#define CAUSE_EXCMASK 0x0000007C /*!< The bits of the exception code. */

/* Floating-point control and status. */
#define FPCSR_FS 0x01000000    /*!< Denormalised results flush to zero. */
#define FPCSR_C 0x00800000     /*!< The condition bit of a comparison. */
#define FPCSR_EV 0x00000800    /*!< An invalid operation raises an exception. */
#define FPCSR_EZ 0x00000400    /*!< A division by zero raises an exception. */
#define FPCSR_EO 0x00000200    /*!< An overflow raises an exception. */
#define FPCSR_EU 0x00000100    /*!< An underflow raises an exception. */
#define FPCSR_EI 0x00000080    /*!< An inexact result raises an exception. */
#define FPCSR_RM_RN 0x00000000 /*!< Round to the nearest value. */

/* TLB page masks. */
#define TLBPGMASK_4K 0x00000000   /*!< The mask of a 4 KB page. */
#define TLBPGMASK_16K 0x00006000  /*!< The mask of a 16 KB page. */
#define TLBPGMASK_64K 0x0001E000  /*!< The mask of a 64 KB page. */
#define TLBPGMASK_256K 0x0007E000 /*!< The mask of a 256 KB page. */
#define TLBPGMASK_1M 0x001FE000   /*!< The mask of a 1 MB page. */
#define TLBPGMASK_4M 0x007FE000   /*!< The mask of a 4 MB page. */
#define TLBPGMASK_16M 0x01FFE000  /*!< The mask of a 16 MB page. */

#endif /* PR_R4300_H */
