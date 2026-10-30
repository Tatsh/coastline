// The end of the application and of a race session on SDL. The Windows libraries release
// DirectInput, DirectSound, DirectPlay, the music device, and the front end, and the SDL game
// releases the gamepad, the sound effects, and the music it opened for the race.
#include "common/session.h"

#include <cstdlib>

#include <SDL3/SDL.h>

#include "common/fatalerror.h"
#include "common/gameinit.h"
#include "common/inputpoll.h"
#include "common/paintshop.h"
#include "common/rallyconfig.h"
#include "common/renderbackend.h"
#include "common/season.h"
#include "common/snapshot.h"
#include "common/soundbuffer.h"
#include "common/strings.h"
#include "log.h"
#include "racemusic.h"

[[noreturn]] void ShowFatalErrorScreen(const char *pszMessage) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", pszMessage);
    // The PC release draws the message and waits for Escape. The message box waits for the user
    // in the same way, and does nothing without a display.
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Coastline", pszMessage, nullptr);
    ShutdownApplication(EXIT_FAILURE);
}

[[noreturn]] void ShutdownApplication(int nExitCode) {
    FreeAllRacerColorPanels();
    InvalidateFrameSnapshotSlots();
    if (g_pfnRenderBackendShutdown != nullptr) {
        g_pfnRenderBackendShutdown();
    }
    ReleaseJoystickDevice();
    ShutdownDirectSound();
    coastline::StopRaceMusic();
    if (g_pfnShutdownTextureSubsystem != nullptr) {
        g_pfnShutdownTextureSubsystem();
    }
    FreeAllPaintBitmaps();
    FreeStringTable();
    SDL_Log("The game exits with code %d.", nExitCode);
    coastline::StopGameLog();
    std::exit(nExitCode);
}

void ShutdownGameSession(void) {
    SDL_Log("Shutting down the race session.");
    if (g_dwSavedSfxVolume != g_GlobalRallyConfig.dwSfxVolume ||
        g_dwSavedMusicVolume != g_GlobalRallyConfig.dwMusicVolume) {
        g_GlobalRallyConfig.SaveConfigToFile(g_szConfigFilePath);
    }
    ResetAudioSubsystem();
    ReleaseJoystickDevice();
    FreeCarPaintTextures();
    SDL_Log("The race session is shut down.");
}
