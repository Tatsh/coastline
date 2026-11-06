#pragma once

/** Initialise the native sound device after the shared sound tables are reset. */
bool InitializePlatformSoundOutput(void);

/** Release the native sound device after its buffers are reset. */
void ShutdownPlatformSoundOutput(void);

/** Return whether the native sound device can play sound effects. */
bool IsPlatformSoundOutputActive(void);
