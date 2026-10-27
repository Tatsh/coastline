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

/* Whether the camera of a view changed this frame, which keeps the old pitch and volume. */
static s32 IsAnyViewCameraChanged(void) {
    return g_aVehicles[g_aViewportRects[0].vehicleIndex].cameraChanged ||
           (g_dwNumViewports != 1 && g_aVehicles[g_aViewportRects[1].vehicleIndex].cameraChanged);
}

void UpdateAmbientSoundChannel(void) {
    MixerChannel *channel = &g_aSoundEffectChannels[TRACK_SOUND_CHANNEL];
    SoundEffect *effect;
    CameraFrame *listener;
    f32 factor;
    f32 frequency;
    f32 upper;
    f32 lower;
    s32 volume;
    s32 source;

    /* The first vehicle always has its racer. */
    if (g_aVehicles[0].racer->status & RACER_HELD) {
        return;
    }
    if (g_dwChannel3PlayingSfxId != -1) {
        if (channel->step != 0) {
            /* A sound played once still plays. */
            return;
        }
        g_dwChannel3PlayingSfxId = -1;
        g_ClosestSoundSource.previousSource = -1;
        g_ClosestSoundSource.playing = FALSE;
    }

    source = g_ClosestSoundSource.source;
    if (source == -1) {
        channel->volume = 0;
        channel->mixVolume = 0;
        channel->step = 0;
        listener = g_ClosestSoundSource.listener;
    } else if (g_ClosestSoundSource.previousSource != source ||
               g_ClosestSoundSource.previousListener != g_ClosestSoundSource.listener) {
        /* A new source fades the old one out first. */
        g_ClosestSoundSource.playing = FALSE;
        listener = g_ClosestSoundSource.listener;
        channel->volume = ((u32)channel->volume >> 1) & VOLUME_HALVES_MASK;
    } else {
        if (g_ClosestSoundSource.frequency < 0.0f) {
            factor = ComputeDopplerFactor(&g_ClosestSoundSource.position,
                                          &g_ClosestSoundSource.position,
                                          &g_ClosestSoundSource.listener->position,
                                          &g_ClosestSoundSource.previousListenerPosition);
            frequency = -g_ClosestSoundSource.frequency * factor;
            ComputeStereoPan(&g_ClosestSoundSource.position,
                             g_ClosestSoundSource.listener,
                             &upper,
                             &lower,
                             &volume,
                             TRUE);
        } else {
            factor = ComputeDopplerFactor(&g_ClosestSoundSource.position,
                                          &g_ClosestSoundSource.previousPosition,
                                          &g_ClosestSoundSource.listener->position,
                                          &g_ClosestSoundSource.previousListenerPosition);
            frequency = g_ClosestSoundSource.frequency * factor;
            ComputeStereoPan(&g_ClosestSoundSource.position,
                             g_ClosestSoundSource.listener,
                             &upper,
                             &lower,
                             &volume,
                             FALSE);
        }
        volume = (s32)((u32)volume * (u32)g_ClosestSoundSource.volume) >> 8;
        if (!g_ClosestSoundSource.playing) {
            g_ClosestSoundSource.playing = TRUE;
            effect = &g_aSoundEffects[g_ClosestSoundSource.sound];
            StartSoundChannelMuted(
                TRACK_SOUND_CHANNEL, effect->sample, effect->length, effect->loopLength);
        }
        if (!IsAnyViewCameraChanged()) {
            channel->step = FrequencyStep(frequency);
            if (channel->volume == 0) {
                /* A sound starts at half its volume. */
                channel->volume = ((s32)(UpperVolume(volume, upper) + lower * (f32)volume) >> 1) &
                                  VOLUME_HALVES_MASK;
            } else {
                channel->volume = PackVolume(volume, upper, lower);
            }
        }
        g_ClosestSoundSource.previousPosition.x = g_ClosestSoundSource.position.x;
        g_ClosestSoundSource.previousPosition.y = g_ClosestSoundSource.position.y;
        g_ClosestSoundSource.previousPosition.z = g_ClosestSoundSource.position.z;
        g_ClosestSoundSource.previousListenerPosition.x = g_ClosestSoundSource.listener->position.x;
        g_ClosestSoundSource.previousListenerPosition.y = g_ClosestSoundSource.listener->position.y;
        g_ClosestSoundSource.previousListenerPosition.z = g_ClosestSoundSource.listener->position.z;
        source = g_ClosestSoundSource.source;
        listener = g_ClosestSoundSource.listener;
    }
    g_ClosestSoundSource.previousSource = source;
    g_ClosestSoundSource.previousListener = listener;
    g_ClosestSoundSource.previousSound = g_ClosestSoundSource.sound;
}

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
