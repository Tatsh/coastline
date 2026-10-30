#ifndef COASTLINE_WIN_BUILDINFO_H
#define COASTLINE_WIN_BUILDINFO_H

#ifdef ENABLE_PATCHES
#ifdef __cplusplus
extern "C" {
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/** Log the renderer build identity and also write to stdout under Wine. */
void PrintWineBuildIdentification(void);

/** Append an application diagnostic and also write to stdout under Wine. */
void PrintWineDiagnostic(const char *message);
#else
#define PrintWineBuildIdentification()
#define PrintWineDiagnostic(message)
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

/** Draw the build identity during a race while B is held. */
void DrawRaceBuildIdentification(void);

#ifdef __cplusplus
}
#endif

#else
#define PrintWineBuildIdentification()
#define PrintWineDiagnostic(message)
#define DrawRaceBuildIdentification()
#endif /* ENABLE_PATCHES */

#endif /* COASTLINE_WIN_BUILDINFO_H */
