#ifndef COASTLINE_EAR_WAVECHUNKS_H
#define COASTLINE_EAR_WAVECHUNKS_H

#include <stdint.h>

#include <windows.h>
// Multimedia declarations require the core Windows SDK types first.
#include <mmsystem.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress EAR: 0x1003b880 */
extern MMCKINFO g_RiffWaveChunk;
/** @ghidraAddress EAR: 0x1003b898 */
extern MMCKINFO g_WaveDataChunk;
/** @ghidraAddress EAR: 0x1003b8b0 */
extern MMCKINFO g_WaveFormatChunk;
/** @ghidraAddress EAR: 0x10037268 */
extern uint16_t g_earMaximumStreamingBufferBytes;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_WAVECHUNKS_H */
