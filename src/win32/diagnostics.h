#ifndef COASTLINE_WIN_DIAGNOSTICS_H
#define COASTLINE_WIN_DIAGNOSTICS_H

/**
 * @file
 * The crash reporting and diagnostic log of the patched Windows programs.
 */

#ifdef ENABLE_PATCHES
#ifdef __cplusplus
extern "C" {
#endif

/** Install application crash reporting when patches are enabled. */
void InitializeApplicationDiagnostics(void);

/** Restore the exception filter before unloading the renderer. */
void ShutdownApplicationDiagnostics(void);

/**
 * Append a line to `coastline.log` beside the executable, prefixed with the UTC time and the
 * process and thread identifiers. The last error of the thread is restored afterwards.
 *
 * @param message The text to append, including its line ending.
 */
void WriteApplicationDiagnostic(const char *message);

#ifdef __cplusplus
}
#endif

#else
/** Omit crash-handler installation when patches are disabled. */
#define InitializeApplicationDiagnostics()
/** Omit crash-handler shutdown when patches are disabled. */
#define ShutdownApplicationDiagnostics()
/**
 * Omit diagnostic output when patches are disabled.
 *
 * @param message The text, not evaluated.
 */
#define WriteApplicationDiagnostic(message)
#endif /* ENABLE_PATCHES */

#endif /* COASTLINE_WIN_DIAGNOSTICS_H */
