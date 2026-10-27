#include "earplayback.h"

EarPlayback::~EarPlayback() {
    FinalizeDirectSoundPlaybackState();
}
