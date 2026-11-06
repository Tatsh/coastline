#ifndef PR_SPTASK_H
#define PR_SPTASK_H

/**
 * @file
 * RSP tasks.
 */

#include <PR/ultratypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#if TGR_BOSS_RALLY
struct GbiCommand;
#endif

/** A task for the RSP: its microcode, its stack and output buffers, and the data it runs. */
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
#if TGR_BOSS_RALLY
    struct GbiCommand *data_ptr; /*!< The display list the backend runs. */
#else
    u64 *data_ptr; /*!< The display list or command list the task runs. */
#endif
    u32 data_size;       /*!< The size of the display list or command list, in bytes. */
    u64 *yield_data_ptr; /*!< The buffer the RSP saves its state to when the task yields. */
    u32 yield_data_size; /*!< The size of the yield buffer, OS_YIELD_DATA_SIZE bytes. */
} OSTask_t;

/** A task for the RSP, aligned to 8 bytes. */
typedef union {
    OSTask_t t;                              /*!< The task. */
    long long int force_structure_alignment; /*!< Aligns the task to 8 bytes. */
} OSTask;

/* The flags of a task. */
#define OS_TASK_YIELDED 0x0001  /*!< The task yielded and resumes from its yield buffer. */
#define OS_TASK_DP_WAIT 0x0002  /*!< The task waits for the RDP before it ends. */
#define OS_TASK_LOADABLE 0x0004 /*!< The microcode of the task loads further microcode. */
#define OS_TASK_SP_ONLY 0x0008  /*!< The task runs on the RSP alone. */

/** The bytes of the buffer a yielding graphics task saves its state to. */
#define OS_YIELD_DATA_SIZE 0xC00

/** The type of a graphics task. */
#define M_GFXTASK 1

/* The sizes of what an RSP task loads. */
#define SP_UCODE_SIZE 4096        /*!< The bytes of the microcode text a task loads. */
#define SP_UCODE_DATA_SIZE 2048   /*!< The bytes of the microcode data a task loads. */
#define SP_DRAM_STACK_SIZE8 0x400 /*!< The bytes of the matrix stack of the microcode. */

/**
 * The physical copy of the last task osSpTaskLoad() loaded.
 *
 * @ghidraAddress USA: 0x8037f330
 * @ghidraAddress EUR: 0x80381270
 * @ghidraAddress JPN: 0x8037fee0
 */
extern OSTask tmp_task;

/**
 * @ghidraAddress USA: 0x80264b60
 * @ghidraAddress EUR: 0x802662f0
 * @ghidraAddress JPN: 0x80265df0
 */
OSTask *_VirtualToPhysicalTask(OSTask *intp);

/**
 * Load a task into the RSP, copying its header to the end of the DMEM and its boot microcode to
 * the IMEM. A yielded task resumes from its yield buffer.
 *
 * @param tp The task, with virtual addresses. Its OS_TASK_YIELDED flag is cleared.
 * @ghidraAddress USA: 0x80264c7c
 * @ghidraAddress EUR: 0x8026640c
 * @ghidraAddress JPN: 0x80265f0c
 */
void osSpTaskLoad(OSTask *tp);

/**
 * Start the RSP on the task osSpTaskLoad() loaded.
 *
 * @param tp The task loaded, not read.
 * @ghidraAddress USA: 0x80264e0c
 * @ghidraAddress EUR: 0x8026659c
 * @ghidraAddress JPN: 0x8026609c
 */
void osSpTaskStartGo(OSTask *tp);

/**
 * @ghidraAddress USA: 0x8026b850
 * @ghidraAddress EUR: 0x8026d820
 * @ghidraAddress JPN: 0x8026d2d0
 */
void __osSpSetStatus(u32 data);

/**
 * @ghidraAddress USA: 0x8026b860
 * @ghidraAddress EUR: 0x8026d830
 * @ghidraAddress JPN: 0x8026d2e0
 */
s32 __osSpSetPc(u32 pc);

/**
 * @ghidraAddress USA: 0x8026b8a0
 * @ghidraAddress EUR: 0x8026d870
 * @ghidraAddress JPN: 0x8026d320
 */
s32 __osSpRawStartDma(s32 direction, u32 devAddr, void *dramAddr, u32 size);

/**
 * @ghidraAddress USA: 0x8026b930
 * @ghidraAddress EUR: 0x8026d900
 * @ghidraAddress JPN: 0x8026d3b0
 */
u32 __osSpDeviceBusy(void);

#ifdef __cplusplus
}
#endif

#endif /* PR_SPTASK_H */
