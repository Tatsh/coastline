#ifndef COASTLINE_EAR_RUNTIME_H
#define COASTLINE_EAR_RUNTIME_H

#include <cstdint>

/** @ghidraAddress EAR: 0x10003c86 */
int IsEarReady();

/** @ghidraAddress EAR: 0x1000c680 */
int __stdcall EarEarInactiveExportWrapper();

/** @ghidraAddress EAR: 0x1000c68a */
extern "C" int __stdcall EAR_DLL_EarInactive();

/** @ghidraAddress EAR: 0x1003b7a0 */
extern std::int32_t g_bEarCodeValidated;

/** @ghidraAddress EAR: 0x1003b7a4 */
extern std::int32_t g_bEarComponentsInitialized;

/** @ghidraAddress EAR: 0x1003b7a8 */
extern std::int32_t g_bEarInitializationComplete;

#endif
