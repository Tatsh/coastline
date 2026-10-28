#ifndef COASTLINE_EAR_MEMORY_BUDGET_H
#define COASTLINE_EAR_MEMORY_BUDGET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress EAR: 0x10008bb0 */
int InitializeSampleMemoryBudget(void);

/** @ghidraAddress EAR: 0x10008ca2 */
uint32_t GetSampleMemoryLimit(void);

/** @ghidraAddress EAR: 0x10008cac */
uint32_t GetSampleMemoryUsed(void);

/** @ghidraAddress EAR: 0x10008cb6 */
int __stdcall ReserveSampleMemory(uint32_t bytes);

/** @ghidraAddress EAR: 0x10008ce6 */
void __stdcall ReleaseSampleMemory(uint32_t bytes);

/** @ghidraAddress EAR: 0x10008cfa */
int __stdcall SetSampleMemoryLimit(uint32_t bytes);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_MEMORY_BUDGET_H */
