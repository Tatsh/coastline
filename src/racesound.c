#include <PR/os.h>

#include "audio.h"
#include "audiomix.h"
#include "race.h"
#include "rumble.h"
#include "soundmix.h"
#include "soundpan.h"
#include "system.h"
#include "track.h"
#include "trackselect.h"
#include "weather.h"

/* The channels: two for each vehicle, the engine then its effects, and the track on 3. */
#define CHANNELS_PER_VEHICLE 2
#define TRACK_SOUND_CHANNEL 3

/* The room the sound effects are loaded into. */
#define SOUND_EFFECT_SPACE 0x29FE0

/* The volume halves both channels keep after packing. */
#define VOLUME_HALVES_MASK 0x7FFF7FFF

void StubSoundUpdate(CameraFrame *camera) {
}

void LoadAllSoundEffects(void) {
    SoundEffect *effect;
    u8 *dest = g_aInflateHeapBuffer;
    u32 used;
    s32 i;
    s32 k;

    for (i = 0, effect = g_aSoundEffects; i != SOUND_EFFECTS; i++, effect++) {
        effect->length = DmaReadAssetSize(effect->romStart);
        if (effect->loopLength == -1) {
            effect->loopLength = effect->length;
        }
        if (effect->guardLength == -1) {
            effect->guardLength = effect->length;
        }
        if (&g_aInflateHeapBuffer[SOUND_EFFECT_SPACE] <
            dest + effect->guardLength + effect->length) {
            /* The sound is left pointing at the start of the room, and the room still advances. */
            effect->sample = (s8 *)g_aInflateHeapBuffer;
            osSyncPrintf("ERROR: Sound buffer allocation overflow on car sound %d\n", i);
        } else {
            effect->sample = (s8 *)dest;
            DecompressFromRom(effect->sample, effect->romStart, NULL);
            /* The start is copied after the end, so a loop can read past it. */
            for (k = 0; (u32)k < (u32)effect->guardLength; k++) {
                effect->sample[effect->length + k] = effect->sample[k];
            }
        }
        dest += effect->length + effect->guardLength;
    }
    used = dest - g_aInflateHeapBuffer;
    osSyncPrintf("Car sound effect space used: %d/%d\n", used, SOUND_EFFECT_SPACE);
    if (used > SOUND_EFFECT_SPACE) {
        SetDebugMessage("Car sound effect overflow");
    }
    StartSoundChannelMuted(0,
                           g_aSoundEffects[SOUND_EFFECT_ENGINE].sample,
                           g_aSoundEffects[SOUND_EFFECT_ENGINE].length,
                           g_aSoundEffects[SOUND_EFFECT_ENGINE].loopLength);
    if (g_dwNumActiveVehicles >= 2) {
        StartSoundChannelMuted(CHANNELS_PER_VEHICLE,
                               g_aSoundEffects[SOUND_EFFECT_ENGINE].sample,
                               g_aSoundEffects[SOUND_EFFECT_ENGINE].length,
                               g_aSoundEffects[SOUND_EFFECT_ENGINE].loopLength);
    }
    if (g_dwNumActiveVehicles >= 3) {
        StartSoundChannelMuted(2 * CHANNELS_PER_VEHICLE,
                               g_aSoundEffects[SOUND_EFFECT_ENGINE].sample,
                               g_aSoundEffects[SOUND_EFFECT_ENGINE].length,
                               g_aSoundEffects[SOUND_EFFECT_ENGINE].loopLength);
    }
}
