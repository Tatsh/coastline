#include "ear/directsoundoutput.h"
#include "ear/error.h"
#include "ear/sdlmixer.h"

void ShutdownDirectSoundOutput() {
    EarCloseSdlMixer();
}

int InitializeDirectSoundOutput() {
    // SDL has no cooperative level, and the output does not need the window of the game.
    if (!EarOpenSdlMixer()) {
        SetEarLastError(3);
        return 0;
    }
    return 1;
}
