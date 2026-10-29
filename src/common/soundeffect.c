#include "common/soundeffect.h"

#include "common/soundchannel.h"

#if TGR_BOSS_RALLY
/** @ghidraAddress BRD3D: 0x10aa3470 */
s32 g_dwChannel3PlayingSfxId;
#endif /* TGR_BOSS_RALLY */

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
