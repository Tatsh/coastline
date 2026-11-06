#include "common/buildinfo.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <windows.h>
#else
#include <SDL3/SDL.h>
#endif

#include "common/hudcanvas.h"
#include "common/settings.h"
#include "common/text.h"
#ifdef ENABLE_PATCHES
static const char projectUri[] = "github.com/Tatsh/coastline";
static const char revision[] = COASTLINE_GIT_SHA;
static const char buildDate[] = __DATE__ " " __TIME__;

void DrawRaceBuildIdentification(void) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (!(GetAsyncKeyState('B') & 0x8000)) {
        return;
    }
#else
    if (!SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_B]) {
        return;
    }
#endif
    const char *lines[] = {projectUri, revision, buildDate};
    int y = GetHudCanvasHeight() - 76;
    SetTextStyleNormal();
    SetTextAlignLeft();
    SetTextSize(15);
    for (unsigned index = 0; index < sizeof(lines) / sizeof(lines[0]); ++index) {
        DrawText(lines[index], 12, y);
        y += 20;
    }
}
#endif /* ENABLE_PATCHES */
