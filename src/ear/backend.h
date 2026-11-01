#ifndef COASTLINE_EAR_BACKEND_H
#define COASTLINE_EAR_BACKEND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Forward the audio-component version check to QueryDirectSoundProductVersion().
 * @return Nonzero for an exact expected version match; zero otherwise.
 * @ghidraAddress EAR: 0x1001010b
 */
int __stdcall GetDirectSoundVersion(void);

/**
 * Compare the system DirectSound product version or linked SDL version with the expected version.
 * Windows reads c:\\windows\\system\\dsound.dll. Mismatches may set audio validation bits 8
 * for an older version or 2 for a newer version, comparing major and minor independently.
 * @return Nonzero for an exact version match; zero for a mismatch or failed Windows version query.
 * @ghidraAddress EAR: 0x1000fc00
 */
int __stdcall QueryDirectSoundProductVersion(void);

/**
 * Reject a requested unsupported stereo solution.
 * @param enabled Zero to accept normal operation, or nonzero to request the unsupported solution.
 * @return One for zero; otherwise zero with EAR error 0x20.
 * @ghidraAddress EAR: 0x10010115
 */
int __stdcall ValidateEarStereoSolution(uint32_t enabled);
/**
 * Store a reserved initialisation value without validation or further processing.
 * @param option The uninterpreted 32-bit option value.
 * @return One.
 * @ghidraAddress EAR: 0x10010132
 */
int __stdcall SetReservedEarInitializationOption(uint32_t option);
/**
 * @return Zero because initialisation feature 0x4000 is unsupported.
 * @ghidraAddress EAR: 0x100100fd
 */
int RejectUnsupportedInitializationFeature4000(void);
/**
 * @return Zero because initialisation feature 0x2000 is unsupported.
 * @ghidraAddress EAR: 0x10010104
 */
int RejectUnsupportedInitializationFeature2000(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_BACKEND_H */
