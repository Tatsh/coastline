#ifndef COASTLINE_WIN_BUILDINFO_H
#define COASTLINE_WIN_BUILDINFO_H

#ifdef __cplusplus
extern "C" {
#endif

/** Log the renderer build identity and also write to stdout under Wine. */
void PrintWineBuildIdentification(void);

/** Append an application diagnostic and also write to stdout under Wine. */
void PrintWineDiagnostic(const char *message);

/** Draw the build identity during a race while B is held. */
void DrawRaceBuildIdentification(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_WIN_BUILDINFO_H */
