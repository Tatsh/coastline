#pragma once

/**
 * @file
 * The native sound device of each platform (DirectSound on Windows, EAR on SDL).
 */

/**
 * Initialise the native sound device after the shared sound tables are reset.
 *
 * @return True when the device started. On Windows, a DirectSound object created as null also
 * returns true.
 */
bool InitializePlatformSoundOutput(void);

/** Release the native sound device after its buffers are reset. */
void ShutdownPlatformSoundOutput(void);

/**
 * Return whether the native sound device can play sound effects.
 *
 * @return True while the device created by InitializePlatformSoundOutput() exists.
 */
bool IsPlatformSoundOutputActive(void);
