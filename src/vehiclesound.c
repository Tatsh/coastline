#include "vehiclesound.h"

#include "game.h"
#include "pause.h"
#include "rumble.h"
#include "soundchannel.h"
#include "soundeffect.h"
#include "soundmix.h"
#include "soundpan.h"
#include "soundsource.h"
#include "vehicleaccess.h"
#include "weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include "audiomix.h"
#endif

#define CHANNELS_PER_VEHICLE 2

/* The engine falls silent above 100000 hertz. */
#define ENGINE_RPM_SCALE (1.0f / 1400.0f)
#define ENGINE_MAX_FREQUENCY 100000.0f
#define EFFECT_MAX_FREQUENCY 20000.0f

/* The echo under a shelter. */
#define SHELTER_ECHO_OFFSET 11000

/* The priorities of vehicle impact, track impact, landing, skid, and rolling effects. */
#define PRIORITY_VEHICLE_IMPACT 100
#define PRIORITY_IMPACT 90
#define PRIORITY_LANDING 80
#define PRIORITY_SKID 40
#define PRIORITY_ROLL 1

/* A skid swells over 16 frames, a twelfth at a time; rolling is loudest at 27 units a second. */
#define SKID_SWELL_FRAMES 16
#define SKID_SWELL_DIVISOR 12
#define ROLL_MAX_VOLUME 64
#define ROLL_FULL_SPEED 27.0f

/* The weather the skids and rolling sound as snow in. */
#define SNOW_WEATHER WEATHER_SNOW

/* Rumble the controller of a local player's vehicle. */
static void Rumble(Vehicle *vehicle, s16 onFrames, s16 offFrames, s16 frames) {
    if (VEHICLE_GET(vehicle, Index) < (s32)g_dwNumLocalPlayers) {
        g_awRumbleCounter[VEHICLE_GET(vehicle, Index)] = 0;
        g_awRumbleMotorOn[VEHICLE_GET(vehicle, Index)] = 0;
        g_awRumbleOnFrames[VEHICLE_GET(vehicle, Index)] = onFrames;
        g_awRumbleOffFrames[VEHICLE_GET(vehicle, Index)] = offFrames;
        g_awRumbleFramesLeft[VEHICLE_GET(vehicle, Index)] = frames;
        g_awRumbleActive[VEHICLE_GET(vehicle, Index)] = 1;
    }
}

/* Rumble for a skid, unless the controller already rumbles. */
static void RumbleSkid(Vehicle *vehicle, s16 offFrames, s16 frames) {
    if (VEHICLE_GET(vehicle, Index) < (s32)g_dwNumLocalPlayers &&
        !g_awRumbleActive[VEHICLE_GET(vehicle, Index)]) {
        Rumble(vehicle, 1, offFrames, frames);
    }
}

/* Take a hit as the effect of a vehicle if it is louder, or matters more than the effect playing.
 */
#if TGR_PLATFORM == TGR_PLATFORM_N64
static void TakeHitEffect(Vehicle *vehicle,
                          s32 volume,
                          s32 sound,
                          s32 priority,
                          s16 onFrames,
                          s16 offFrames,
                          s16 frames) {
    if (vehicle->effectVolume < volume || (volume && vehicle->effectPriority < priority)) {
        vehicle->effectVolume = volume;
        vehicle->effectSound = sound;
        vehicle->effectPriority = priority;
        Rumble(vehicle, onFrames, offFrames, frames);
    }
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

/* Whether an effect is a skid. */
static s32 IsSkidSound(s32 sound) {
    return sound >= SOUND_EFFECT_FIRST_SKID && sound < SOUND_EFFECT_FIRST_ROLL;
}

/* Whether an effect is rolling. */
static s32 IsRollSound(s32 sound) {
    return sound >= SOUND_EFFECT_FIRST_ROLL && sound < SOUND_EFFECT_ROLL_END;
}

/* Choose the skid of a vehicle on a surface, louder the longer it lasts. */
static void ChooseSkidEffect(Vehicle *vehicle, s32 surface, s32 skid) {
    VEHICLE_SET(vehicle, EffectVolume, skid);
    if (g_dwCurrentWeather == SNOW_WEATHER) {
        switch (surface) {
        case 0:
        case 1:
        case 2:
        case 3:
            VEHICLE_SET(vehicle, EffectSound, SOUND_EFFECT_SKID_SURFACE_0);
            VEHICLE_SET(vehicle, EffectVolume, skid >> 1);
            break;
        case TRACK_SURFACE_SLOW:
            VEHICLE_SET(vehicle, EffectSound, SOUND_EFFECT_SKID_SURFACE_0);
            VEHICLE_SET(vehicle, EffectVolume, 0);
            break;
        }
    } else {
        switch (surface) {
        case 0:
        case 3:
            RumbleSkid(vehicle, 9, 10);
            VEHICLE_SET(vehicle, EffectSound, SOUND_EFFECT_SKID_SURFACE_0);
            break;
        case 1:
            RumbleSkid(vehicle, 14, 15);
            VEHICLE_SET(vehicle, EffectSound, SOUND_EFFECT_SKID_SURFACE_1);
            break;
        case 2:
            RumbleSkid(vehicle, 12, 13);
            VEHICLE_SET(vehicle, EffectSound, SOUND_EFFECT_SKID_SURFACE_2);
            break;
        case TRACK_SURFACE_SLOW:
            VEHICLE_SET(vehicle, EffectSound, SOUND_EFFECT_SLOW_SURFACE);
            break;
        }
        VEHICLE_SET(vehicle, EffectVolume, VEHICLE_GET(vehicle, Body).skidding >> 1);
    }
    VEHICLE_SET(vehicle, EffectPriority, PRIORITY_SKID);
    if (VEHICLE_ADVANCE_SKID_FRAMES(vehicle) >= SKID_SWELL_FRAMES + 1) {
        VEHICLE_SET(vehicle, SkidFrames, SKID_SWELL_FRAMES);
    }
    VEHICLE_SET(vehicle,
                EffectVolume,
                VEHICLE_GET(vehicle, EffectVolume) * VEHICLE_GET(vehicle, SkidFrames));
    VEHICLE_SET(vehicle, EffectVolume, VEHICLE_GET(vehicle, EffectVolume) / SKID_SWELL_DIVISOR);
}

/* Choose the rolling of a vehicle on a surface, louder the faster it goes. */
static void ChooseRollEffect(Vehicle *vehicle, s32 surface) {
    if (g_dwCurrentWeather == SNOW_WEATHER) {
        switch (surface) {
        case 0:
        case 1:
        case 2:
        case 3:
            VEHICLE_SET(vehicle, EffectSound, SOUND_EFFECT_ROLL_SNOW);
            break;
        case TRACK_SURFACE_SLOW:
            VEHICLE_SET(vehicle, EffectSound, SOUND_EFFECT_ROLL_SURFACE_0);
            break;
        default:
            VEHICLE_SET(vehicle, EffectSound, SOUND_EFFECT_NONE);
            break;
        }
    } else {
        switch (surface) {
        case 0:
        case 3:
            VEHICLE_SET(vehicle, EffectSound, SOUND_EFFECT_ROLL_SURFACE_0);
            break;
        case 1:
            VEHICLE_SET(vehicle, EffectSound, SOUND_EFFECT_ROLL_SURFACE_1);
            break;
        case 2:
            VEHICLE_SET(vehicle, EffectSound, SOUND_EFFECT_ROLL_SURFACE_2);
            break;
        case TRACK_SURFACE_SLOW:
            VEHICLE_SET(vehicle, EffectSound, SOUND_EFFECT_SLOW_SURFACE);
            break;
        default:
            VEHICLE_SET(vehicle, EffectSound, SOUND_EFFECT_NONE);
            break;
        }
    }
    VEHICLE_SET(vehicle, EffectPriority, PRIORITY_ROLL);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    {
        long double volume = Vec3fLength(&VEHICLE_GET(vehicle, PlacedVelocity)) *
                             ((f32)ROLL_MAX_VOLUME / ROLL_FULL_SPEED);
        VEHICLE_SET(
            vehicle, EffectVolume, volume >= -0x1p63L && volume < 0x1p63L ? (s32)(s64)volume : 0);
    }
#else
    vehicle->effectVolume = (s32)(Vec3fLength(&VEHICLE_GET(vehicle, PlacedVelocity)) *
                                  (f32)ROLL_MAX_VOLUME / ROLL_FULL_SPEED);
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    if (VEHICLE_GET(vehicle, EffectVolume) > ROLL_MAX_VOLUME) {
        VEHICLE_SET(vehicle, EffectVolume, ROLL_MAX_VOLUME);
    }
}

/* Choose the effect of a vehicle from its hits, skids and rolling. */
static void
ChooseVehicleEffect(Vehicle *vehicle, s32 channel, s32 previousSound, CameraFrame *listener) {
    s32 sound;
    s32 surface;
    s32 skid;

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (!IsVehicleSoundChannelPlaying(channel + 1) ||
#else
    if (g_aSoundEffectChannels[channel + 1].step == 0 ||
#endif
        (!VEHICLE_GET(vehicle, Body).skidding && IsSkidSound(previousSound))) {
        VEHICLE_SET(vehicle, EffectSound, SOUND_EFFECT_NONE);
        VEHICLE_SET(vehicle, EffectVolume, 0);
        VEHICLE_SET(vehicle, EffectPriority, 0);
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    PlayVehicleEventSounds(vehicle, listener);
#else
    TakeHitEffect(vehicle,
                  VEHICLE_GET(vehicle, Body).vehicleImpactVolume,
                  SOUND_EFFECT_VEHICLE_IMPACT,
                  PRIORITY_VEHICLE_IMPACT,
                  5,
                  1,
                  30);
    VEHICLE_GET(vehicle, Body).vehicleImpactVolume = 0;
    TakeHitEffect(vehicle,
                  VEHICLE_GET(vehicle, Body).impactVolume,
                  SOUND_EFFECT_IMPACT,
                  PRIORITY_IMPACT,
                  100,
                  1,
                  30);
    VEHICLE_GET(vehicle, Body).impactVolume = 0;
    TakeHitEffect(vehicle,
                  VEHICLE_GET(vehicle, Body).landed,
                  SOUND_EFFECT_LANDING,
                  PRIORITY_LANDING,
                  3,
                  2,
                  20);
    VEHICLE_GET(vehicle, Body).landed = 0;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

    sound = VEHICLE_GET(vehicle, EffectSound);
    if (sound != SOUND_EFFECT_NONE && !IsSkidSound(sound) && !IsRollSound(sound)) {
        return;
    }
#if TGR_PLATFORM == TGR_PLATFORM_N64
    VEHICLE_GET(vehicle, Body).unreconstructed202 = 0;
#endif
    surface = VEHICLE_GET(vehicle, WheelBodies)[1].contactFrames ?
                  VEHICLE_GET(vehicle, WheelBodies)[1].surface :
                  -1;
    skid = VEHICLE_GET(vehicle, Body).skidding;
    /* A skid takes over from a quieter effect, from rolling, and from the skid of the last frame.
     */
    if (VEHICLE_GET(vehicle, EffectVolume) < skid ||
        (skid && VEHICLE_GET(vehicle, EffectPriority) < PRIORITY_SKID) ||
        (skid && VEHICLE_GET(vehicle, EffectPriority) == PRIORITY_SKID &&
         IsSkidSound(previousSound))) {
        ChooseSkidEffect(vehicle, surface, skid);
    } else {
        VEHICLE_SET(vehicle, SkidFrames, 0);
    }
    sound = VEHICLE_GET(vehicle, EffectSound);
    if (sound == SOUND_EFFECT_NONE || IsRollSound(sound)) {
        ChooseRollEffect(vehicle, surface);
    }
}

void UpdateVehicleSounds(Vehicle *vehicle) {
    Vehicle *viewed = &g_aVehicles[g_aViewportRects[0].vehicleIndex];
    s32 channel = VEHICLE_GET(vehicle, Index) * CHANNELS_PER_VEHICLE;
    CameraFrame *listener;
    SoundEffect *effect;
    s32 changed;
    s32 previousSound;
    s32 effectVolume;
    s32 volume;
    f32 upper;
    f32 lower;
    SoundReal frequency;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    f32 roundedFrequency;
#endif

    if (g_dwRacePauseState || VEHICLE_GET(vehicle, Racer) == NULL) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        ClearVehicleSoundChannel(channel);
#else
        g_aSoundEffectChannels[channel].volume = 0;
        g_aSoundEffectChannels[channel].step = 0;
#endif
        return;
    }
    changed = FALSE;
    if (g_dwNumViewports == 1) {
        listener = VEHICLE_GET(viewed, ActiveCamera);
        if (VEHICLE_GET(vehicle, CameraChanged) || VEHICLE_GET(viewed, CameraChanged)) {
            changed = TRUE;
        }
    } else {
        listener = VEHICLE_GET(vehicle, ActiveCamera);
        if (VEHICLE_GET(vehicle, CameraChanged) || VEHICLE_GET(viewed, CameraChanged) ||
            VEHICLE_GET(&(g_aVehicles[g_aViewportRects[1].vehicleIndex]), CameraChanged)) {
            changed = TRUE;
        }
    }
    VEHICLE_SET(vehicle,
                DopplerFactor,
                ComputeDopplerFactor(&VEHICLE_GET(vehicle, Frame).position,
                                     &VEHICLE_GET(vehicle, PreviousPosition),
                                     &listener->position,
                                     &VEHICLE_GET(vehicle, PreviousListenerPosition)));
    ComputeStereoPan(
        &VEHICLE_GET(vehicle, Frame).position, listener, &upper, &lower, &volume, FALSE);

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    frequency = (long double)VEHICLE_GET(vehicle, EngineRpm) *
                (0.0f < VEHICLE_GET(vehicle, EngineRpm) ? 0.5f : -0.5f) * 15.714286f *
                VEHICLE_GET(vehicle, DopplerFactor);
#else
    if (0.0f < vehicle->engineRpm) {
        frequency = vehicle->engineRpm * 0.5f * ENGINE_RPM_SCALE * SAMPLE_RATE;
    } else {
        frequency = vehicle->engineRpm * -0.5f * ENGINE_RPM_SCALE * SAMPLE_RATE;
    }
    frequency = vehicle->dopplerFactor * frequency;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    if (ENGINE_MAX_FREQUENCY < frequency) {
        /* Too high, the engine falls silent. */
        frequency = 0.0f;
    } else if (
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        !(frequency >= 0.0f)
#else
        frequency < 0.0f
#endif
    ) {
        frequency = 0.0f;
    }
    if (VEHICLE_GET(vehicle, Index) == 0) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        SetVehicleSoundEchoOffset(
            g_LoadedTrack.instances[VEHICLE_GET(vehicle, GroundInstances)[0]].flags &
                    TRACK_INSTANCE_SHELTER ?
                SHELTER_ECHO_OFFSET :
                0);
#else
        if (g_LoadedTrack.instances[VEHICLE_GET(vehicle, GroundInstances)[0]].flags &
            TRACK_INSTANCE_SHELTER) {
            g_dwSoundEffectEchoOffset = SHELTER_ECHO_OFFSET;
        } else {
            g_dwSoundEffectEchoOffset = 0;
        }
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    }
    if (!changed) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        SetVehicleSoundChannel(channel, FrequencyStep(frequency), PackVolume(volume, upper, lower));
#else
        g_aSoundEffectChannels[channel].step = FrequencyStep(frequency);
        g_aSoundEffectChannels[channel].volume = PackVolume(volume, upper, lower);
#endif
    }

    previousSound = VEHICLE_GET(vehicle, EffectSound);
    ChooseVehicleEffect(vehicle, channel, previousSound, listener);

    /* Only the first vehicle plays its effects. */
    if (VEHICLE_GET(vehicle, EffectSound) == SOUND_EFFECT_NONE) {
        if (channel == 0) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            ClearVehicleSoundChannel(channel + 1);
#else
            g_aSoundEffectChannels[channel + 1].volume = 0;
            g_aSoundEffectChannels[channel + 1].step = 0;
#endif
        }
    } else {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        frequency = (long double)VEHICLE_GET(vehicle, DopplerFactor) * SAMPLE_RATE;
        roundedFrequency = frequency;
        if (EFFECT_MAX_FREQUENCY < frequency || !(roundedFrequency >= 0.0f)) {
            roundedFrequency = 0.0f;
        }
        frequency = roundedFrequency;
#else
        frequency = vehicle->dopplerFactor * SAMPLE_RATE;
        if (EFFECT_MAX_FREQUENCY < frequency) {
            frequency = 0.0f;
        } else if (frequency < 0.0f) {
            frequency = 0.0f;
        }
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
        if (previousSound != VEHICLE_GET(vehicle, EffectSound) && channel == 0) {
            effect = &g_aSoundEffects[VEHICLE_GET(vehicle, EffectSound)];
            StartSoundChannelMuted(
                (s16)(channel + 1), effect->sample, effect->length, effect->loopLength);
        }
        if (!changed && channel == 0) {
            effectVolume = (s32)((u32)VEHICLE_GET(vehicle, EffectVolume) * (u32)volume) >> 7;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            SetVehicleSoundChannel(
                channel + 1, FrequencyStep(frequency), PackVolume(effectVolume, upper, lower));
#else
            g_aSoundEffectChannels[channel + 1].step = FrequencyStep(frequency);
            g_aSoundEffectChannels[channel + 1].volume = PackVolume(effectVolume, upper, lower);
#endif
        }
    }
    if (g_dwNumViewports == 1) {
        VEHICLE_GET(vehicle, PreviousListenerPosition).x =
            VEHICLE_GET(viewed, ActiveCamera)->position.x;
        VEHICLE_GET(vehicle, PreviousListenerPosition).y =
            VEHICLE_GET(viewed, ActiveCamera)->position.y;
        VEHICLE_GET(vehicle, PreviousListenerPosition).z =
            VEHICLE_GET(viewed, ActiveCamera)->position.z;
    } else {
        VEHICLE_GET(vehicle, PreviousListenerPosition).x =
            VEHICLE_GET(vehicle, ActiveCamera)->position.x;
        VEHICLE_GET(vehicle, PreviousListenerPosition).y =
            VEHICLE_GET(vehicle, ActiveCamera)->position.y;
        VEHICLE_GET(vehicle, PreviousListenerPosition).z =
            VEHICLE_GET(vehicle, ActiveCamera)->position.z;
    }
}
