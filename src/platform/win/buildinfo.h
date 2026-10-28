#ifndef COASTLINE_WIN_BUILDINFO_H
#define COASTLINE_WIN_BUILDINFO_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Print the renderer build identity to inherited stdout under Wine. */
void PrintWineBuildIdentification(void);

/** Toggle and draw the build identity with Ctrl+Shift+B on the root menu. */
void DrawMenuBuildIdentification(bool atRootMenu);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_WIN_BUILDINFO_H */
