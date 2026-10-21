#ifndef PR_R4300_H
#define PR_R4300_H

/**
 * @file
 * Address segments, coprocessor 0 registers, and status bits of the VR4300.
 */

#define KUSEG 0x00000000
#define K0BASE 0x80000000
#define K1BASE 0xA0000000
#define K2BASE 0xC0000000

#define IS_KSEG0(x) ((u32)(x) >= K0BASE && (u32)(x) < K1BASE)
#define IS_KSEG1(x) ((u32)(x) >= K1BASE && (u32)(x) < K2BASE)

#define K0_TO_K1(x) ((u32)(x) | 0xA0000000)
#define K1_TO_K0(x) ((u32)(x) & 0x9FFFFFFF)
#define K0_TO_PHYS(x) ((u32)(x) & 0x1FFFFFFF)
#define K1_TO_PHYS(x) ((u32)(x) & 0x1FFFFFFF)
#define KDM_TO_PHYS(x) ((u32)(x) & 0x1FFFFFFF)
#define PHYS_TO_K0(x) ((u32)(x) | 0x80000000)
#define PHYS_TO_K1(x) ((u32)(x) | 0xA0000000)

#define ICACHE_SIZE 0x4000
#define ICACHE_LINESIZE 32
#define DCACHE_SIZE 0x2000
#define DCACHE_LINESIZE 16

/* Status register. */
#define SR_IE 0x00000001
#define SR_EXL 0x00000002
#define SR_ERL 0x00000004
#define SR_KSU_USR 0x00000010
#define SR_IMASK 0x0000FF00
#define SR_IBIT1 0x00000100
#define SR_IBIT2 0x00000200
#define SR_IBIT3 0x00000400
#define SR_IBIT4 0x00000800
#define SR_IBIT5 0x00001000
#define SR_IBIT6 0x00002000
#define SR_IBIT7 0x00004000
#define SR_IBIT8 0x00008000
#define SR_IMASK0 0x0000FF00
#define SR_IMASK1 0x0000FE00
#define SR_IMASK2 0x0000FC00
#define SR_IMASK3 0x0000F800
#define SR_IMASK4 0x0000F000
#define SR_IMASK5 0x0000E000
#define SR_IMASK6 0x0000C000
#define SR_IMASK7 0x00008000
#define SR_IMASK8 0x00000000
#define SR_DE 0x00010000
#define SR_CE 0x00020000
#define SR_CH 0x00040000
#define SR_SR 0x00100000
#define SR_TS 0x00200000
#define SR_BEV 0x00400000
#define SR_ITS 0x01000000
#define SR_RE 0x02000000
#define SR_FR 0x04000000
#define SR_RP 0x08000000
#define SR_CU0 0x10000000
#define SR_CU1 0x20000000

/* Cause register. */
#define CAUSE_BD 0x80000000
#define CAUSE_IP8 0x00008000
#define CAUSE_IP7 0x00004000
#define CAUSE_IP6 0x00002000
#define CAUSE_IP5 0x00001000
#define CAUSE_IP4 0x00000800
#define CAUSE_IP3 0x00000400
#define CAUSE_SW2 0x00000200
#define CAUSE_SW1 0x00000100
#define CAUSE_EXCMASK 0x0000007C

/* Floating-point control and status. */
#define FPCSR_FS 0x01000000
#define FPCSR_C 0x00800000
#define FPCSR_EV 0x00000800
#define FPCSR_EZ 0x00000400
#define FPCSR_EO 0x00000200
#define FPCSR_EU 0x00000100
#define FPCSR_EI 0x00000080
#define FPCSR_RM_RN 0x00000000

/* TLB page sizes and entry flags. */
#define TLBPGMASK_4K 0x00000000
#define TLBPGMASK_16K 0x00006000
#define TLBPGMASK_64K 0x0001E000
#define TLBPGMASK_256K 0x0007E000
#define TLBPGMASK_1M 0x001FE000
#define TLBPGMASK_4M 0x007FE000
#define TLBPGMASK_16M 0x01FFE000

#endif
