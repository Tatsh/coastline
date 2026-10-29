#include "common/ambientsound.h"

#include "common/racerslotaccess.h"
#include "common/soundchannel.h"
#include "common/soundeffect.h"
#include "common/soundmix.h"
#include "common/soundpan.h"
#include "common/soundsource.h"
#include "common/vehicle.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"

#if TGR_BOSS_RALLY
#include "win32/sound.h"
#else
#include "common/audio.h"
#include "common/audiomix.h"
#endif

#if TGR_BOSS_RALLY
#define AMBIENT_CHANNEL_STATE SoundChannelState
#define AMBIENT_CHANNELS g_aSoundChannelRequested
#define AMBIENT_STEP llFrequency
#define AMBIENT_VOLUME dwStereoLevels
#define AMBIENT_MIX_VOLUME dwReserved
#define FREQUENCY_NEGATIVE(value) (!((value) >= 0.0f))
#else
#define AMBIENT_CHANNEL_STATE MixerChannel
#define AMBIENT_CHANNELS g_aSoundEffectChannels
#define AMBIENT_STEP step
#define AMBIENT_VOLUME volume
#define AMBIENT_MIX_VOLUME mixVolume
#define FREQUENCY_NEGATIVE(value) ((value) < 0.0f)
#endif /* TGR_BOSS_RALLY */

#define TRACK_SOUND_CHANNEL 3
#define VOLUME_HALVES_MASK 0x7FFF7FFF

/* Whether the camera of a view changed this frame, which keeps the old pitch and volume. */
static s32 IsAnyViewCameraChanged(void) {
    return VEHICLE_GET(&(g_aVehicles[g_aViewportRects[0].vehicleIndex]), CameraChanged) ||
           (g_dwNumViewports != 1 &&
            VEHICLE_GET(&(g_aVehicles[g_aViewportRects[1].vehicleIndex]), CameraChanged));
}

void UpdateAmbientSoundChannel(void) {
    AMBIENT_CHANNEL_STATE *channel = &AMBIENT_CHANNELS[TRACK_SOUND_CHANNEL];
    SoundEffect *effect;
    CameraFrame *listener;
    SoundReal factor;
    f32 frequency;
    f32 upper;
    f32 lower;
    s32 volume;
    s32 source;

#if TGR_BOSS_RALLY
    u32 levels;

    if (g_dwNumActiveVehicles == 0) {
        return;
    }
#endif
    /* The first vehicle always has its racer. */
    if (RACER_SLOT_GET(VEHICLE_GET(&(g_aVehicles[0]), Racer), Status) & RACER_HELD) {
        return;
    }
    if (g_dwChannel3PlayingSfxId != -1) {
        if (channel->AMBIENT_STEP != 0) {
            /* A sound played once still plays. */
            return;
        }
        g_dwChannel3PlayingSfxId = -1;
        g_ClosestSoundSource.previousSource = -1;
        g_ClosestSoundSource.playing = FALSE;
    }

    source = g_ClosestSoundSource.source;
    if (source == -1) {
        channel->AMBIENT_VOLUME = 0;
        channel->AMBIENT_MIX_VOLUME = 0;
        channel->AMBIENT_STEP = 0;
        listener = g_ClosestSoundSource.listener;
    } else if (g_ClosestSoundSource.previousSource != source ||
               g_ClosestSoundSource.previousListener != g_ClosestSoundSource.listener) {
        /* A new source fades the old one out first. */
        g_ClosestSoundSource.playing = FALSE;
        listener = g_ClosestSoundSource.listener;
        channel->AMBIENT_VOLUME = ((u32)channel->AMBIENT_VOLUME >> 1) & VOLUME_HALVES_MASK;
    } else {
        if (FREQUENCY_NEGATIVE(g_ClosestSoundSource.frequency)) {
            factor = ComputeDopplerFactor(&g_ClosestSoundSource.position,
                                          &g_ClosestSoundSource.position,
                                          &g_ClosestSoundSource.listener->position,
                                          &g_ClosestSoundSource.previousListenerPosition);
#if TGR_BOSS_RALLY
            frequency = -(factor * g_ClosestSoundSource.frequency);
#else
            frequency = -g_ClosestSoundSource.frequency * factor;
#endif
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
            channel->AMBIENT_STEP = FrequencyStep(frequency);
#if TGR_BOSS_RALLY
            levels = PackVolume(volume, upper, lower);
            if (channel->AMBIENT_VOLUME == 0) {
                levels = ((s32)levels >> 1) & VOLUME_HALVES_MASK;
            }
            channel->AMBIENT_VOLUME = levels;
#else
            if (channel->AMBIENT_VOLUME == 0) {
                /* A sound starts at half its volume. */
                channel->AMBIENT_VOLUME =
                    ((s32)(UpperVolume(volume, upper) + lower * (f32)volume) >> 1) &
                    VOLUME_HALVES_MASK;
            } else {
                channel->AMBIENT_VOLUME = PackVolume(volume, upper, lower);
            }
#endif /* TGR_BOSS_RALLY */
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
