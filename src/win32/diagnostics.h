#ifndef COASTLINE_WIN_DIAGNOSTICS_H
#define COASTLINE_WIN_DIAGNOSTICS_H

#ifdef ENABLE_PATCHES
#ifdef __cplusplus
extern "C" {
#endif

/** Install application crash reporting when patches are enabled. */
void InitializeApplicationDiagnostics(void);

/** Restore the exception filter before unloading the renderer. */
void ShutdownApplicationDiagnostics(void);

/** Append a diagnostic beside the executable when patches are enabled. */
void WriteApplicationDiagnostic(const char *message);

#ifdef __cplusplus
}
#endif

#else
/** Omit crash-handler installation when patches are disabled. */
#define InitializeApplicationDiagnostics()
/** Omit crash-handler shutdown when patches are disabled. */
#define ShutdownApplicationDiagnostics()
/** Omit diagnostic output when patches are disabled. */
#define WriteApplicationDiagnostic(message)
#endif /* ENABLE_PATCHES */

#endif /* COASTLINE_WIN_DIAGNOSTICS_H */
