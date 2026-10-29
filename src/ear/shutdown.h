#ifndef COASTLINE_EAR_SHUTDOWN_H
#define COASTLINE_EAR_SHUTDOWN_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Shut down the timer, every registration, the music devices, and DirectSound, and clear the
 * state of the library.
 *
 * @ghidraAddress EAR: 0x1000f27a
 */
void __stdcall EAR_DLL_ShutDownEar(void);

/** @ghidraAddress EAR: 0x1000e650 */
void __stdcall EarResetEarExportWrapper(void);

/**
 * Shut down every channel, matrix, and environment, and register the default environment again.
 *
 * @ghidraAddress EAR: 0x1000e65a
 */
void __stdcall EAR_DLL_ResetEar(void);

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * Close the CD audio and MIDI devices and restore the mixer controls.
 *
 * @ghidraAddress EAR: 0x10004d27
 */
void ShutdownMusicDevicesAndRestoreMixer(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_SHUTDOWN_H */
