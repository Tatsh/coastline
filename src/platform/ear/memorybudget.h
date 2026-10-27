#ifndef COASTLINE_EAR_MEMORY_BUDGET_H
#define COASTLINE_EAR_MEMORY_BUDGET_H

#include <cstdint>

/** @ghidraAddress EAR: 0x10008bb0 */
int InitializeSampleMemoryBudget();

/** @ghidraAddress EAR: 0x10008ca2 */
std::uint32_t GetSampleMemoryLimit();

/** @ghidraAddress EAR: 0x10008cac */
std::uint32_t GetSampleMemoryUsed();

/** @ghidraAddress EAR: 0x10008cb6 */
int __stdcall ReserveSampleMemory(std::uint32_t bytes);

/** @ghidraAddress EAR: 0x10008ce6 */
void __stdcall ReleaseSampleMemory(std::uint32_t bytes);

/** @ghidraAddress EAR: 0x10008cfa */
int __stdcall SetSampleMemoryLimit(std::uint32_t bytes);

#endif
