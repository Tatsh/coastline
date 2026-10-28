#ifndef COASTLINE_WIN_BUILDINFO_H
#define COASTLINE_WIN_BUILDINFO_H

#ifdef __cplusplus
extern "C" {
#endif

/** Print the renderer build identity to inherited stdout under Wine. */
void PrintWineBuildIdentification(void);

/** Draw the build identity during a race while B is held. */
void DrawRaceBuildIdentification(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_WIN_BUILDINFO_H */
