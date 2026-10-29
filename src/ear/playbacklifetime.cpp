#include "ear/earplayback.h"

EarPlayback::~EarPlayback() {
    FinalizeDirectSoundPlaybackState();
}
