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
#include "common/soundsystem.h"
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

void UpdateAmbientSoundChannel(void) {
#if TGR_BOSS_RALLY
    AMBIENT_CHANNEL_STATE *channel = &AMBIENT_CHANNELS[TRACK_SOUND_CHANNEL];
#else
#define channel (&AMBIENT_CHANNELS[TRACK_SOUND_CHANNEL])
#endif
#if TGR_BOSS_RALLY
    SoundEffect *effect;
#else
    Vec3f *listenerPosition;
#endif
#if TGR_BOSS_RALLY
    SoundReal factor;
#endif
    f32 frequency;
    s32 volume;
    f32 upper;
    f32 lower;

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
        goto select_source;
    }

select_source:
    if (g_ClosestSoundSource.source == -1) {
        channel->AMBIENT_VOLUME = 0;
        channel->AMBIENT_MIX_VOLUME = 0;
        channel->AMBIENT_STEP = 0;
    } else if (g_ClosestSoundSource.previousSource == g_ClosestSoundSource.source &&
               g_ClosestSoundSource.previousListener == g_ClosestSoundSource.listener) {
        if (FREQUENCY_NEGATIVE(g_ClosestSoundSource.frequency)) {
#if TGR_BOSS_RALLY
            factor = ComputeDopplerFactor(&g_ClosestSoundSource.position,
                                          &g_ClosestSoundSource.position,
                                          &g_ClosestSoundSource.listener->position,
                                          &g_ClosestSoundSource.previousListenerPosition);
#else
            frequency = ComputeDopplerFactor(&g_ClosestSoundSource.position,
                                             &g_ClosestSoundSource.position,
                                             &g_ClosestSoundSource.listener->position,
                                             &g_ClosestSoundSource.previousListenerPosition);
#endif
#if TGR_BOSS_RALLY
            frequency = -(factor * g_ClosestSoundSource.frequency);
#else
            frequency = -g_ClosestSoundSource.frequency * frequency;
#endif
            ComputeStereoPan(&g_ClosestSoundSource.position,
                             g_ClosestSoundSource.listener,
                             &upper,
                             &lower,
                             &volume,
                             TRUE);
        } else {
#if TGR_BOSS_RALLY
            factor = ComputeDopplerFactor(&g_ClosestSoundSource.position,
                                          &g_ClosestSoundSource.previousPosition,
                                          &g_ClosestSoundSource.listener->position,
                                          &g_ClosestSoundSource.previousListenerPosition);
            frequency = g_ClosestSoundSource.frequency * factor;
#else
            frequency = ComputeDopplerFactor(&g_ClosestSoundSource.position,
                                             &g_ClosestSoundSource.previousPosition,
                                             &g_ClosestSoundSource.listener->position,
                                             &g_ClosestSoundSource.previousListenerPosition);
            frequency = g_ClosestSoundSource.frequency * frequency;
#endif
            ComputeStereoPan(&g_ClosestSoundSource.position,
                             g_ClosestSoundSource.listener,
                             &upper,
                             &lower,
                             &volume,
                             FALSE);
        }
#if TGR_BOSS_RALLY
        volume = (s32)((u32)volume * (u32)g_ClosestSoundSource.volume) >> 8;
#else
        volume *= g_ClosestSoundSource.volume;
        volume >>= 8;
#endif
        if (!g_ClosestSoundSource.playing) {
            g_ClosestSoundSource.playing = TRUE;
#if TGR_BOSS_RALLY
            effect = &g_aSoundEffects[g_ClosestSoundSource.sound];
            StartSoundChannelMuted(
                TRACK_SOUND_CHANNEL, effect->sample, effect->length, effect->loopLength);
#else
            StartSoundChannelMuted(TRACK_SOUND_CHANNEL,
                                   g_aSoundEffects[g_ClosestSoundSource.sound].sample,
                                   g_aSoundEffects[g_ClosestSoundSource.sound].length,
                                   g_aSoundEffects[g_ClosestSoundSource.sound].loopLength);
#endif
        }
        if (VEHICLE_GET(&(g_aVehicles[g_aViewportRects[0].vehicleIndex]), CameraChanged) == 0 &&
            (g_dwNumViewports == 1 ||
             VEHICLE_GET(&(g_aVehicles[g_aViewportRects[1].vehicleIndex]), CameraChanged) == 0)) {
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
                    ((s32)((f32)((s32)((f32)volume * upper) << 16) + lower * volume) >> 1) &
                    VOLUME_HALVES_MASK;
            } else {
#if TGR_PLATFORM == TGR_PLATFORM_N64
                channel->AMBIENT_VOLUME =
                    (u32)((f32)((s32)((f32)volume * upper) << 16) + lower * volume);
#else
                channel->AMBIENT_VOLUME = PackVolume(volume, upper, lower);
#endif
            }
#endif /* TGR_BOSS_RALLY */
        }
        g_ClosestSoundSource.previousPosition.x = g_ClosestSoundSource.position.x;
        g_ClosestSoundSource.previousPosition.y = g_ClosestSoundSource.position.y;
        g_ClosestSoundSource.previousPosition.z = g_ClosestSoundSource.position.z;
#if TGR_BOSS_RALLY
        g_ClosestSoundSource.previousListenerPosition.x = g_ClosestSoundSource.listener->position.x;
        g_ClosestSoundSource.previousListenerPosition.y = g_ClosestSoundSource.listener->position.y;
        g_ClosestSoundSource.previousListenerPosition.z = g_ClosestSoundSource.listener->position.z;
#else
        listenerPosition = &g_ClosestSoundSource.listener->position;
        g_ClosestSoundSource.previousListenerPosition.x = listenerPosition->x;
        g_ClosestSoundSource.previousListenerPosition.y = listenerPosition->y;
        g_ClosestSoundSource.previousListenerPosition.z = listenerPosition->z;
#endif
    } else {
        /* A new source fades the old one out first. */
        g_ClosestSoundSource.playing = FALSE;
#if TGR_BOSS_RALLY
        channel->AMBIENT_VOLUME = ((u32)channel->AMBIENT_VOLUME >> 1) & VOLUME_HALVES_MASK;
#else
        volume = channel->AMBIENT_VOLUME;
        channel->AMBIENT_VOLUME = ((u32)volume >> 1) & VOLUME_HALVES_MASK;
#endif
    }
    g_ClosestSoundSource.previousSource = g_ClosestSoundSource.source;
    g_ClosestSoundSource.previousListener = g_ClosestSoundSource.listener;
    g_ClosestSoundSource.previousSound = g_ClosestSoundSource.sound;
}
#if !TGR_BOSS_RALLY
#undef channel
#endif
