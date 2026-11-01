#ifndef COASTLINE_WIN_BUILDINFO_H
#define COASTLINE_WIN_BUILDINFO_H

#ifdef ENABLE_PATCHES
#ifdef __cplusplus
extern "C" {
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/** Log the renderer build identity and also write to stdout under Wine. */
void PrintWineBuildIdentification(void);

/**
 * Append an application diagnostic and also write to stdout under Wine.
 *
 * @param message The null-terminated diagnostic text.
 */
void PrintWineDiagnostic(const char *message);
#else
/** Omit the Windows renderer build diagnostic on other platforms. */
#define PrintWineBuildIdentification()
/**
 * Omit a Windows renderer diagnostic on other platforms.
 *
 * @param message Not evaluated.
 */
#define PrintWineDiagnostic(message)
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

/** Draw the build identity during a race while B is held. */
void DrawRaceBuildIdentification(void);

#ifdef __cplusplus
}
#endif

#else
/** Omit build diagnostics when optional patches are disabled. */
#define PrintWineBuildIdentification()
/**
 * Omit application diagnostics when optional patches are disabled.
 *
 * @param message Not evaluated.
 */
#define PrintWineDiagnostic(message)
/** Omit the race build-identity overlay when optional patches are disabled. */
#define DrawRaceBuildIdentification()
#endif /* ENABLE_PATCHES */

#endif /* COASTLINE_WIN_BUILDINFO_H */
