#ifndef COASTLINE_EAR_MEMORY_BUDGET_H
#define COASTLINE_EAR_MEMORY_BUDGET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise a sample budget from available Windows page-file capacity or SDL system RAM.
 * Repeated calls preserve an established budget and current usage.
 * @return One on success; zero with EAR error 0x10 when available capacity is below 512000 bytes.
 * @ghidraAddress EAR: 0x10008bb0
 */
int InitializeSampleMemoryBudget(void);

/**
 * @return The configured sample-memory limit in bytes.
 * @ghidraAddress EAR: 0x10008ca2
 */
uint32_t GetSampleMemoryLimit(void);

/**
 * @return The tracked reserved sample-memory byte count.
 * @ghidraAddress EAR: 0x10008cac
 */
uint32_t GetSampleMemoryUsed(void);

/**
 * Account for a reservation without allocating memory.
 * Native addition wraps at 32 bits before comparison with the limit.
 * @param bytes The requested byte count to add to current usage.
 * @return One when wrapped total usage does not exceed the limit; zero with usage unchanged.
 * @ghidraAddress EAR: 0x10008cb6
 */
int __stdcall ReserveSampleMemory(uint32_t bytes);

/**
 * Subtract released storage from tracked usage without freeing memory or checking underflow.
 * @param bytes The byte count to subtract using unsigned 32-bit arithmetic.
 * @ghidraAddress EAR: 0x10008ce6
 */
void __stdcall ReleaseSampleMemory(uint32_t bytes);

/**
 * Change the budget limit only when the requested limit does not exceed current usage.
 * @param bytes The requested limit in bytes.
 * @return One when the limit is stored; zero with the limit unchanged when bytes exceeds usage.
 * @ghidraAddress EAR: 0x10008cfa
 */
int __stdcall SetSampleMemoryLimit(uint32_t bytes);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_MEMORY_BUDGET_H */
