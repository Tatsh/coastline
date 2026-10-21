#ifndef PR_SPTASK_H
#define PR_SPTASK_H

/**
 * @file
 * RSP tasks.
 */

#include <PR/ultratypes.h>

typedef struct {
    u32 type;              /*!< The kind of task, such as M_GFXTASK for graphics. */
    u32 flags;             /*!< The OS_TASK_ bits. */
    u64 *ucode_boot;       /*!< The microcode that boots the RSP and loads the task microcode. */
    u32 ucode_boot_size;   /*!< The size of the boot microcode, in bytes. */
    u64 *ucode;            /*!< The text of the task microcode. */
    u32 ucode_size;        /*!< The size of the text of the task microcode, in bytes. */
    u64 *ucode_data;       /*!< The data of the task microcode. */
    u32 ucode_data_size;   /*!< The size of the data of the task microcode, in bytes. */
    u64 *dram_stack;       /*!< The matrix stack of the microcode in memory, aligned to 16 bytes. */
    u32 dram_stack_size;   /*!< The size of the matrix stack, in bytes. */
    u64 *output_buff;      /*!< The start of the buffer the RSP writes RDP commands to. */
    u64 *output_buff_size; /*!< The end of the output buffer, despite the name. */
    u64 *data_ptr;         /*!< The display list or command list the task runs. */
    u32 data_size;         /*!< The size of the display list or command list, in bytes. */
    u64 *yield_data_ptr;   /*!< The buffer the RSP saves its state to when the task yields. */
    u32 yield_data_size;   /*!< The size of the yield buffer, OS_YIELD_DATA_SIZE bytes. */
} OSTask_t;

typedef union {
    OSTask_t t;                              /*!< The task. */
    long long int force_structure_alignment; /*!< Aligns the task to 8 bytes. */
} OSTask;

#define OS_TASK_YIELDED 0x0001
#define OS_TASK_DP_WAIT 0x0002
#define OS_TASK_LOADABLE 0x0004
#define OS_TASK_SP_ONLY 0x0008

#define OS_YIELD_DATA_SIZE 0xC00

/** The type of a graphics task. */
#define M_GFXTASK 1

/** The bytes of the microcode text and data an RSP task loads, and of its matrix stack. */
#define SP_UCODE_SIZE 4096
#define SP_UCODE_DATA_SIZE 2048
#define SP_DRAM_STACK_SIZE8 0x400

/**
 * The physical copy of the last task osSpTaskLoad() loaded.
 *
 * @ghidraAddress USA: 0x8037f330
 */
extern OSTask tmp_task;

/**
 * Copy a task to tmp_task with each of its pointers turned physical.
 *
 * @return tmp_task.
 * @ghidraAddress USA: 0x80264b60
 */
OSTask *_VirtualToPhysicalTask(OSTask *intp);

/**
 * Load a task into the RSP, copying its header to the end of the DMEM and its boot microcode to
 * the IMEM. A yielded task resumes from its yield buffer.
 *
 * @ghidraAddress USA: 0x80264c7c
 */
void osSpTaskLoad(OSTask *tp);

/**
 * Start the RSP on the task osSpTaskLoad() loaded.
 *
 * @ghidraAddress USA: 0x80264e0c
 */
void osSpTaskStartGo(OSTask *tp);

/**
 * Write the RSP status register.
 *
 * @ghidraAddress USA: 0x8026b850
 */
void __osSpSetStatus(u32 data);

/**
 * Set the RSP program counter.
 *
 * @return 0, or -1 when the RSP is not halted.
 * @ghidraAddress USA: 0x8026b860
 */
s32 __osSpSetPc(u32 pc);

/**
 * Start a DMA between RDRAM and the RSP memories.
 *
 * @param direction OS_READ to read the RSP memory, OS_WRITE to write it.
 * @return 0, or -1 when the RSP DMA is busy.
 * @ghidraAddress USA: 0x8026b8a0
 */
s32 __osSpRawStartDma(s32 direction, u32 devAddr, void *dramAddr, u32 size);

/**
 * Return whether the RSP DMA is busy or full, or the RSP I/O is full.
 *
 * @ghidraAddress USA: 0x8026b930
 */
u32 __osSpDeviceBusy(void);

#endif
