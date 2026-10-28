#include "win32/ear/earplayback.h"

EarPlayback::~EarPlayback() {
    FinalizeDirectSoundPlaybackState();
}
