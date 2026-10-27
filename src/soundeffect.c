#include "soundeffect.h"

#include "soundchannel.h"

void PlaySoundEffect(s32 sound) {
    SoundEffect *effect = &g_aSoundEffects[sound];

    InitSoundChannel(3, effect->sample, effect->length, effect->loopLength);
    g_dwChannel3PlayingSfxId = sound;
}

void PlayBeepSound(void) {
    PlaySoundEffect(SOUND_EFFECT_BEEP);
}

void PlayGoSound(void) {
    PlaySoundEffect(SOUND_EFFECT_GO);
}
