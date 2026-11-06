#include <SDL3/SDL.h>

#include "common/types.h"
#include "ear/backend.h"
#include "ear/configuration.h"

int __stdcall QueryDirectSoundProductVersion(void) {
    // The SDL game outputs through SDL, and expects the version of SDL it was built with.
    int linked = SDL_GetVersion();
    if (linked == SDL_VERSION) {
        return TRUE;
    }
    // The original compares major and minor independently.
    if (SDL_VERSIONNUM_MAJOR(linked) < SDL_MAJOR_VERSION ||
        SDL_VERSIONNUM_MINOR(linked) < SDL_MINOR_VERSION) {
        g_earAudioComponentValidationFlags |= 8;
    } else if (SDL_VERSIONNUM_MAJOR(linked) > SDL_MAJOR_VERSION ||
               SDL_VERSIONNUM_MINOR(linked) > SDL_MINOR_VERSION) {
        g_earAudioComponentValidationFlags |= 2;
    }
    return FALSE;
}
