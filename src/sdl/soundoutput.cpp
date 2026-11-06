#include "common/soundoutput.h"

#include <SDL3/SDL.h>

#include "ear/configuration.h"
#include "ear/error.h"
#include "ear/initialization.h"
#include "ear/shutdown.h"

namespace {
bool g_bEarSoundStarted;
}

bool InitializePlatformSoundOutput(void) {
    // The mixer of EAR plays the buffers, and EAR mixes at the format of the primary buffer.
    EAR_DLL_AAA_Validate(0x9be9c9);
    g_bEarSoundStarted = EAR_DLL_InitializeEar(0) != 0;
    if (!g_bEarSoundStarted) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
                    "The sound effects are silent. EAR did not start, error %u.",
                    EAR_DLL_GetLastError());
    }
    return g_bEarSoundStarted;
}

void ShutdownPlatformSoundOutput(void) {
    if (g_bEarSoundStarted) {
        EAR_DLL_ShutDownEar();
        g_bEarSoundStarted = false;
    }
}

bool IsPlatformSoundOutputActive(void) {
    return g_bEarSoundStarted;
}
