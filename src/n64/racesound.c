#include <PR/os.h>

#include "common/audio.h"
#include "common/audiomix.h"
#include "common/race.h"
#include "common/rumble.h"
#include "common/soundmix.h"
#include "common/soundpan.h"
#include "common/system.h"
#include "common/track.h"
#include "common/trackselect.h"
#include "common/weather.h"

/* The channels: two for each vehicle, the engine then its effects, and the track on 3. */
#define CHANNELS_PER_VEHICLE 2
#define TRACK_SOUND_CHANNEL 3

/* The room the sound effects are loaded into. */
#if TGR_REGION == TGR_REGION_JP
#define SOUND_EFFECT_SPACE 0x29428
#else
#define SOUND_EFFECT_SPACE 0x29FE0
#endif

/* The volume halves both channels keep after packing. */
#define VOLUME_HALVES_MASK 0x7FFF7FFF

void StubSoundUpdate(CameraFrame *camera) {
}

void LoadAllSoundEffects(void) {
    u32 base;
    u32 pos;
    SoundEffect *samples;
    s32 i;
    u32 j;

    base = (u32)g_aInflateHeapBuffer;
    pos = base;
    for (i = 0; i < SOUND_EFFECTS; i++) {
        g_aSoundEffects[i].length = DmaReadAssetSize(g_aSoundEffects[i].romStart);
        if (g_aSoundEffects[i].loopLength == -1) {
            g_aSoundEffects[i].loopLength = g_aSoundEffects[i].length;
        }
        if (g_aSoundEffects[i].guardLength == -1) {
            g_aSoundEffects[i].guardLength = g_aSoundEffects[i].length;
        }
        if (pos + g_aSoundEffects[i].length + g_aSoundEffects[i].guardLength >
            base + SOUND_EFFECT_SPACE) {
            /* The sound is left pointing at the start of the room, and the room still advances. */
            g_aSoundEffects[i].sample = (s8 *)base;
            osSyncPrintf("ERROR: Sound buffer allocation overflow on car sound %d\n", i);
        } else {
            g_aSoundEffects[i].sample = (s8 *)pos;
            DecompressFromRom((void *)pos, g_aSoundEffects[i].romStart, NULL);
            /* The start is copied after the end, so a loop can read past it. */
            for (j = 0; j < (u32)g_aSoundEffects[i].guardLength; j++) {
                ((u8 *)g_aSoundEffects[i].sample)[j + g_aSoundEffects[i].length] =
                    ((u8 *)g_aSoundEffects[i].sample)[j];
            }
        }
        pos += g_aSoundEffects[i].length + g_aSoundEffects[i].guardLength;
    }
    samples = g_aSoundEffects;
    osSyncPrintf("Car sound effect space used: %d/%d\n", pos - base, SOUND_EFFECT_SPACE);
    if (pos - base > SOUND_EFFECT_SPACE) {
        SetDebugMessage("Car sound effect overflow");
    }
    StartSoundChannelMuted(0, samples->sample, samples->length, samples->loopLength);
    if (g_dwNumActiveVehicles > 1) {
        StartSoundChannelMuted(
            CHANNELS_PER_VEHICLE, samples->sample, samples->length, samples->loopLength);
    }
    if (g_dwNumActiveVehicles > 2) {
        StartSoundChannelMuted(
            2 * CHANNELS_PER_VEHICLE, samples->sample, samples->length, samples->loopLength);
    }
}
