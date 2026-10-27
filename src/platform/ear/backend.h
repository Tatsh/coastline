#ifndef COASTLINE_EAR_BACKEND_H
#define COASTLINE_EAR_BACKEND_H

#include <cstdint>

/** @ghidraAddress EAR: 0x1001010b */
int __stdcall GetDirectSoundVersion();

/**
 * @ghidraAddress EAR: 0x1000fc00
 */
int __stdcall QueryDirectSoundProductVersion();

/** @ghidraAddress EAR: 0x1003b7b4 */
extern std::int32_t g_earAudioComponentsValidated;

/** @ghidraAddress EAR: 0x1003b798 */
extern std::uint16_t g_earAudioComponentValidationFlags;

#endif
