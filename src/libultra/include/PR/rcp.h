#ifndef PR_RCP_H
#define PR_RCP_H

/**
 * @file
 * Registers of the RCP, the RDRAM interface, and the peripheral interfaces.
 */

/** Read the 32-bit register at physical address addr through the uncached segment. */
#define IO_READ(addr) (*(vu32 *)PHYS_TO_K1(addr))
/** Write data to the 32-bit register at physical address addr through the uncached segment. */
#define IO_WRITE(addr, data) (*(vu32 *)PHYS_TO_K1(addr) = (u32)(data))

/* RDRAM and the RSP memories. */
#define RDRAM_BASE_REG 0x03F00000 /*!< The base address of the RDRAM registers. */
#define SP_DMEM_START 0x04000000  /*!< The first byte of the RSP data memory. */
#define SP_DMEM_END 0x04000FFF    /*!< The last byte of the RSP data memory. */
#define SP_IMEM_START 0x04001000  /*!< The first byte of the RSP instruction memory. */
#define SP_IMEM_END 0x04001FFF    /*!< The last byte of the RSP instruction memory. */

/* Signal processor. */
#define SP_BASE_REG 0x04040000                /*!< The base address of the RSP registers. */
#define SP_MEM_ADDR_REG (SP_BASE_REG + 0x00)  /*!< The RSP memory address of a DMA. */
#define SP_DRAM_ADDR_REG (SP_BASE_REG + 0x04) /*!< The RDRAM address of a DMA. */
#define SP_RD_LEN_REG (SP_BASE_REG + 0x08)   /*!< Writing starts a DMA from RDRAM of this length. */
#define SP_WR_LEN_REG (SP_BASE_REG + 0x0C)   /*!< Writing starts a DMA to RDRAM of this length. */
#define SP_STATUS_REG (SP_BASE_REG + 0x10)   /*!< The status on read and the control on write. */
#define SP_DMA_FULL_REG (SP_BASE_REG + 0x14) /*!< Whether a second DMA is queued. */
#define SP_DMA_BUSY_REG (SP_BASE_REG + 0x18) /*!< Whether a DMA is in progress. */
#define SP_SEMAPHORE_REG (SP_BASE_REG + 0x1C) /*!< The semaphore shared with the CPU. */
#define SP_PC_REG 0x04080000                  /*!< The program counter of the RSP. */

/* Bits of the RSP status on read. */
#define SP_STATUS_HALT 0x0001       /*!< The RSP is halted. */
#define SP_STATUS_BROKE 0x0002      /*!< The RSP ran a break instruction. */
#define SP_STATUS_DMA_BUSY 0x0004   /*!< A DMA is in progress. */
#define SP_STATUS_DMA_FULL 0x0008   /*!< A second DMA is queued. */
#define SP_STATUS_IO_FULL 0x0010    /*!< The I/O is full. */
#define SP_STATUS_SSTEP 0x0020      /*!< Single-step mode is on. */
#define SP_STATUS_INTR_BREAK 0x0040 /*!< A break raises an interrupt. */
#define SP_STATUS_YIELD 0x0080      /*!< Signal 0, a request to yield. */
#define SP_STATUS_YIELDED 0x0100    /*!< Signal 1, the task yielded. */
#define SP_STATUS_TASKDONE 0x0200   /*!< Signal 2, the task finished. */

/* Bits of the RSP control on write. */
#define SP_CLR_HALT 0x00000001       /*!< Start the RSP. */
#define SP_SET_HALT 0x00000002       /*!< Halt the RSP. */
#define SP_CLR_BROKE 0x00000004      /*!< Clear the break bit. */
#define SP_CLR_INTR 0x00000008       /*!< Clear the RSP interrupt. */
#define SP_SET_INTR 0x00000010       /*!< Raise the RSP interrupt. */
#define SP_CLR_SSTEP 0x00000020      /*!< Turn single-step mode off. */
#define SP_SET_SSTEP 0x00000040      /*!< Turn single-step mode on. */
#define SP_CLR_INTR_BREAK 0x00000080 /*!< A break no longer raises an interrupt. */
#define SP_SET_INTR_BREAK 0x00000100 /*!< A break raises an interrupt. */
#define SP_CLR_SIG0 0x00000200       /*!< Clear signal 0. */
#define SP_SET_SIG0 0x00000400       /*!< Set signal 0. */
#define SP_CLR_SIG1 0x00000800       /*!< Clear signal 1. */
#define SP_SET_SIG1 0x00001000       /*!< Set signal 1. */
#define SP_CLR_SIG2 0x00002000       /*!< Clear signal 2. */
#define SP_SET_SIG2 0x00004000       /*!< Set signal 2. */
#define SP_CLR_SIG3 0x00008000       /*!< Clear signal 3. */
#define SP_SET_SIG3 0x00010000       /*!< Set signal 3. */
#define SP_CLR_SIG4 0x00020000       /*!< Clear signal 4. */
#define SP_SET_SIG4 0x00040000       /*!< Set signal 4. */
#define SP_CLR_SIG5 0x00080000       /*!< Clear signal 5. */
#define SP_SET_SIG5 0x00100000       /*!< Set signal 5. */
#define SP_CLR_SIG6 0x00200000       /*!< Clear signal 6. */
#define SP_SET_SIG6 0x00400000       /*!< Set signal 6. */
#define SP_CLR_SIG7 0x00800000       /*!< Clear signal 7. */
#define SP_SET_SIG7 0x01000000       /*!< Set signal 7. */

/* Signals the task protocol uses. */
#define SP_CLR_YIELD SP_CLR_SIG0    /*!< Withdraw the request to yield. */
#define SP_SET_YIELD SP_SET_SIG0    /*!< Request that the task yield. */
#define SP_CLR_YIELDED SP_CLR_SIG1  /*!< Clear the mark of a yielded task. */
#define SP_SET_YIELDED SP_SET_SIG1  /*!< Mark the task as yielded. */
#define SP_CLR_TASKDONE SP_CLR_SIG2 /*!< Clear the mark of a finished task. */
#define SP_SET_TASKDONE SP_SET_SIG2 /*!< Mark the task as finished. */

/* Display processor. */
#define DPC_BASE_REG 0x04100000                /*!< The base address of the RDP registers. */
#define DPC_START_REG (DPC_BASE_REG + 0x00)    /*!< The start of the command buffer. */
#define DPC_END_REG (DPC_BASE_REG + 0x04)      /*!< The end of the command buffer. */
#define DPC_CURRENT_REG (DPC_BASE_REG + 0x08)  /*!< The command being read. */
#define DPC_STATUS_REG (DPC_BASE_REG + 0x0C)   /*!< The status on read and the control on write. */
#define DPC_CLOCK_REG (DPC_BASE_REG + 0x10)    /*!< The clock counter. */
#define DPC_BUFBUSY_REG (DPC_BASE_REG + 0x14)  /*!< The buffer busy counter. */
#define DPC_PIPEBUSY_REG (DPC_BASE_REG + 0x18) /*!< The pipe busy counter. */
#define DPC_TMEM_REG (DPC_BASE_REG + 0x1C)     /*!< The texture memory load counter. */

/* MIPS interface. */
#define MI_BASE_REG 0x04300000                /*!< The base address of the MI registers. */
#define MI_INIT_MODE_REG (MI_BASE_REG + 0x00) /*!< The mode register. */
#define MI_MODE_REG MI_INIT_MODE_REG          /*!< Another name of the mode register. */
#define MI_VERSION_REG (MI_BASE_REG + 0x04)   /*!< The versions of the RCP parts. */
#define MI_INTR_REG (MI_BASE_REG + 0x08)      /*!< The pending RCP interrupts. */
#define MI_INTR_MASK_REG (MI_BASE_REG + 0x0C) /*!< The enabled RCP interrupts. */

#define MI_CLR_DP_INTR 0x0800 /*!< Written to the mode register, clear the RDP interrupt. */

/* RCP interrupts in the pending and enabled registers. */
#define MI_INTR_SP 0x01 /*!< The RSP interrupt. */
#define MI_INTR_SI 0x02 /*!< The serial interface interrupt. */
#define MI_INTR_AI 0x04 /*!< The audio interface interrupt. */
#define MI_INTR_VI 0x08 /*!< The video interface interrupt. */
#define MI_INTR_PI 0x10 /*!< The peripheral interface interrupt. */
#define MI_INTR_DP 0x20 /*!< The RDP interrupt. */

/* Bits written to the enabled interrupts register. */
#define MI_INTR_MASK_CLR_SP 0x0001 /*!< Disable the RSP interrupt. */
#define MI_INTR_MASK_SET_SP 0x0002 /*!< Enable the RSP interrupt. */
#define MI_INTR_MASK_CLR_SI 0x0004 /*!< Disable the serial interface interrupt. */
#define MI_INTR_MASK_SET_SI 0x0008 /*!< Enable the serial interface interrupt. */
#define MI_INTR_MASK_CLR_AI 0x0010 /*!< Disable the audio interface interrupt. */
#define MI_INTR_MASK_SET_AI 0x0020 /*!< Enable the audio interface interrupt. */
#define MI_INTR_MASK_CLR_VI 0x0040 /*!< Disable the video interface interrupt. */
#define MI_INTR_MASK_SET_VI 0x0080 /*!< Enable the video interface interrupt. */
#define MI_INTR_MASK_CLR_PI 0x0100 /*!< Disable the peripheral interface interrupt. */
#define MI_INTR_MASK_SET_PI 0x0200 /*!< Enable the peripheral interface interrupt. */
#define MI_INTR_MASK_CLR_DP 0x0400 /*!< Disable the RDP interrupt. */
#define MI_INTR_MASK_SET_DP 0x0800 /*!< Enable the RDP interrupt. */

/* Video interface. */
#define VI_BASE_REG 0x04400000               /*!< The base address of the VI registers. */
#define VI_STATUS_REG (VI_BASE_REG + 0x00)   /*!< The control register. */
#define VI_CONTROL_REG VI_STATUS_REG         /*!< Another name of the control register. */
#define VI_ORIGIN_REG (VI_BASE_REG + 0x04)   /*!< The RDRAM address of the frame buffer. */
#define VI_DRAM_ADDR_REG VI_ORIGIN_REG       /*!< Another name of the origin register. */
#define VI_WIDTH_REG (VI_BASE_REG + 0x08)    /*!< The width of the frame buffer in pixels. */
#define VI_H_WIDTH_REG VI_WIDTH_REG          /*!< Another name of the width register. */
#define VI_INTR_REG (VI_BASE_REG + 0x0C)     /*!< The half-line that raises the interrupt. */
#define VI_V_INTR_REG VI_INTR_REG            /*!< Another name of the interrupt register. */
#define VI_CURRENT_REG (VI_BASE_REG + 0x10)  /*!< The current half-line; writing clears the IRQ. */
#define VI_V_CURRENT_LINE_REG VI_CURRENT_REG /*!< Another name of the current line register. */
#define VI_BURST_REG (VI_BASE_REG + 0x14)    /*!< The timing of the colour burst and sync. */
#define VI_TIMING_REG VI_BURST_REG           /*!< Another name of the burst register. */
#define VI_V_SYNC_REG (VI_BASE_REG + 0x18)   /*!< The half-lines per field. */
#define VI_H_SYNC_REG (VI_BASE_REG + 0x1C)   /*!< The length of a line and the leap pattern. */
#define VI_LEAP_REG (VI_BASE_REG + 0x20)     /*!< The two alternate line lengths. */
#define VI_H_SYNC_LEAP_REG VI_LEAP_REG       /*!< Another name of the leap register. */
#define VI_H_START_REG (VI_BASE_REG + 0x24)  /*!< The start and end of the visible pixels. */
#define VI_H_VIDEO_REG VI_H_START_REG        /*!< Another name of the horizontal start register. */
#define VI_V_START_REG (VI_BASE_REG + 0x28)  /*!< The start and end of the visible lines. */
#define VI_V_VIDEO_REG VI_V_START_REG        /*!< Another name of the vertical start register. */
#define VI_V_BURST_REG (VI_BASE_REG + 0x2C)  /*!< The lines of the colour burst. */
#define VI_X_SCALE_REG (VI_BASE_REG + 0x30)  /*!< The horizontal scale and subpixel offset. */
#define VI_Y_SCALE_REG (VI_BASE_REG + 0x34)  /*!< The vertical scale and subpixel offset. */

/* Audio interface. */
#define AI_BASE_REG 0x04500000                /*!< The base address of the AI registers. */
#define AI_DRAM_ADDR_REG (AI_BASE_REG + 0x00) /*!< The RDRAM address of the next buffer. */
#define AI_LEN_REG (AI_BASE_REG + 0x04)       /*!< The length of the buffer; writing queues it. */
#define AI_CONTROL_REG (AI_BASE_REG + 0x08)   /*!< The control register. */
#define AI_STATUS_REG (AI_BASE_REG + 0x0C)    /*!< The status; writing clears the interrupt. */
#define AI_DACRATE_REG (AI_BASE_REG + 0x10)   /*!< The divider of the sample rate. */
#define AI_BITRATE_REG (AI_BASE_REG + 0x14)   /*!< The divider of the bit rate. */

#define AI_CONTROL_DMA_ON 0x01        /*!< Enable the audio DMA. */
#define AI_STATUS_FIFO_FULL (1 << 31) /*!< Both buffers are queued. */
#define AI_STATUS_DMA_BUSY (1 << 30)  /*!< A buffer is playing. */
#define AI_MAX_BIT_RATE 16            /*!< The largest bit rate divider. */
#define AI_MAX_DAC_RATE 16384         /*!< The largest sample rate divider. */
#define AI_MIN_DAC_RATE 132           /*!< The smallest sample rate divider. */

/* Peripheral interface. */
#define PI_BASE_REG 0x04600000                /*!< The base address of the PI registers. */
#define PI_DRAM_ADDR_REG (PI_BASE_REG + 0x00) /*!< The RDRAM address of a DMA. */
#define PI_CART_ADDR_REG (PI_BASE_REG + 0x04) /*!< The cartridge address of a DMA. */
#define PI_RD_LEN_REG (PI_BASE_REG + 0x08)    /*!< The length of a DMA to the cartridge. */
#define PI_WR_LEN_REG (PI_BASE_REG + 0x0C)    /*!< The length of a DMA from the cartridge. */
#define PI_STATUS_REG (PI_BASE_REG + 0x10)    /*!< The status on read and the control on write. */
#define PI_BSD_DOM1_LAT_REG (PI_BASE_REG + 0x14) /*!< The latency of domain 1. */
#define PI_BSD_DOM1_PWD_REG (PI_BASE_REG + 0x18) /*!< The pulse width of domain 1. */
#define PI_BSD_DOM1_PGS_REG (PI_BASE_REG + 0x1C) /*!< The page size of domain 1. */
#define PI_BSD_DOM1_RLS_REG (PI_BASE_REG + 0x20) /*!< The release duration of domain 1. */
#define PI_BSD_DOM2_LAT_REG (PI_BASE_REG + 0x24) /*!< The latency of domain 2. */
#define PI_BSD_DOM2_PWD_REG (PI_BASE_REG + 0x28) /*!< The pulse width of domain 2. */
#define PI_BSD_DOM2_PGS_REG (PI_BASE_REG + 0x2C) /*!< The page size of domain 2. */
#define PI_BSD_DOM2_RLS_REG (PI_BASE_REG + 0x30) /*!< The release duration of domain 2. */

/* Bits of the PI status. */
#define PI_STATUS_DMA_BUSY 0x01 /*!< On read, a DMA is in progress. */
#define PI_STATUS_IO_BUSY 0x02  /*!< On read, an I/O access is in progress. */
#define PI_STATUS_ERROR 0x04    /*!< On read, the last access failed. */
#define PI_STATUS_RESET 0x01    /*!< On write, reset the controller. */
#define PI_STATUS_CLR_INTR 0x02 /*!< On write, clear the PI interrupt. */

#define PI_DOM1_ADDR2 0x10000000 /*!< The cartridge ROM in domain 1. */
#define PI_DOM2_ADDR2 0x08000000 /*!< SRAM in domain 2. */

/* RDRAM interface. */
#define RI_BASE_REG 0x04700000             /*!< The base address of the RI registers. */
#define RI_SELECT_REG (RI_BASE_REG + 0x0C) /*!< Zero until RDRAM is set up. */

/* Serial interface. */
#define SI_BASE_REG 0x04800000                     /*!< The base address of the SI registers. */
#define SI_DRAM_ADDR_REG (SI_BASE_REG + 0x00)      /*!< The RDRAM address of a DMA. */
#define SI_PIF_ADDR_RD64B_REG (SI_BASE_REG + 0x04) /*!< Writing starts 64 bytes from PIF RAM. */
#define SI_PIF_ADDR_WR64B_REG (SI_BASE_REG + 0x10) /*!< Writing starts 64 bytes to PIF RAM. */
#define SI_STATUS_REG (SI_BASE_REG + 0x18)         /*!< The status; writing clears the interrupt. */

#define SI_STATUS_DMA_BUSY 0x0001  /*!< A DMA is in progress. */
#define SI_STATUS_RD_BUSY 0x0002   /*!< An I/O read is in progress. */
#define SI_STATUS_DMA_ERROR 0x0008 /*!< The last DMA overlapped another. */
#define SI_STATUS_INTERRUPT 0x1000 /*!< The SI interrupt is pending. */

/* PIF RAM, where the controller commands go. */
#define PIF_RAM_START 0x1FC007C0 /*!< The first byte of PIF RAM. */
#define PIF_RAM_END 0x1FC007FF   /*!< The last byte of PIF RAM. */

#endif
