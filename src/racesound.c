#include <PR/os.h>

#include "audio.h"
#include "audiomix.h"
#include "race.h"
#include "rumble.h"
#include "system.h"
#include "track.h"
#include "trackselect.h"
#include "weather.h"

/* The speed of sound, in units a second. */
#define SPEED_OF_SOUND 343.0f

/* A sound pans over 10 units across the camera, from 0 to 1, with the far side at 1.7 and the near
 * side raised 60% of the way to full. A narrow pan moves 40% as far. */
#define PAN_RANGE 10.0f
#define PAN_NARROW 0.4f
#define PAN_LOUD 1.7f
#define PAN_QUIET_RAISE 0.6f

/* The volume of a sound is 1024 over its distance, from no nearer than 32. */
#define SOUND_VOLUME_SCALE 1024.0f
#define SOUND_NEAREST 32.0

/* The distance of no sound, which ResetSoundSourceTracking() starts the search from. */
#define SOUND_FAR 8589934592.0f

/* The channels: two for each vehicle, the engine then its effects, and the track on 3. */
#define CHANNELS_PER_VEHICLE 2
#define TRACK_SOUND_CHANNEL 3

/* The rate the samples play at, which a frequency is given against, as a step of 32.32. */
#define SAMPLE_RATE 11000.0f
#define STEP_ONE 4294967296.0

/* The frequency and volume of the sounds of the track. */
#define TRACK_SOUND_FREQUENCY 11000.0f
#define FAST_AIRPLANE_FREQUENCY 22000.0f
#define DEFAULT_SOUND_VOLUME 0x80
#define LOUD_SOUND_VOLUME 0xFF
#define WATERFALL_VOLUME 0x180

/* The engine: its frequency at an engine speed of 1400 is 5500, and over 100000 it falls silent. */
#define ENGINE_RPM_SCALE (1.0f / 1400.0f)
#define ENGINE_MAX_FREQUENCY 100000.0f
#define EFFECT_MAX_FREQUENCY 20000.0f

/* The echo under a shelter. */
#define SHELTER_ECHO_OFFSET 11000

/* The room the sound effects are loaded into. */
#define SOUND_EFFECT_SPACE 0x29FE0

/* The priorities of the effects of a vehicle: a hit of another vehicle, of the track, a landing, a
 * skid and rolling. */
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

/* The volume halves both channels keep after packing. */
#define VOLUME_HALVES_MASK 0x7FFF7FFF

/* The weather the skids and rolling sound as snow in. */
#define SNOW_WEATHER WEATHER_SNOW

f32 ComputeDopplerFactor(Vec3f *source,
                         Vec3f *previousSource,
                         Vec3f *listener,
                         Vec3f *previousListener) {
    Vec3f direction;
    f32 length;
    Vec3f sourceStep;
    Vec3f listenerStep;
    f32 approach;
    f32 listenerSpeed;

    Vec3fSubtract(&direction, source, listener);
    Vec3fSubtract(&listenerStep, listener, previousListener);
    Vec3fSubtract(&sourceStep, source, previousSource);
    length = Vec3fLength(&direction);
    if (length != 0.0f) {
        Vec3fDivideInPlace(&direction, length);
    }
    approach = -Vec3fDot(&sourceStep, &direction) / g_fFrameDeltaTime;
    listenerSpeed = Vec3fDot(&listenerStep, &direction) / g_fFrameDeltaTime;
    return (listenerSpeed / SPEED_OF_SOUND + 1.0f) / (1.0f - approach / SPEED_OF_SOUND);
}

void ComputeStereoPan(
    Vec3f *source, CameraFrame *camera, f32 *upper, f32 *lower, s32 *volume, s32 narrow) {
    Vec3f offset;
    f32 across;
    f32 share;
    f32 rest;
    f32 a;
    f32 b;
    f32 distance;

    Vec3fSubtract(&offset, source, &camera->position);
    across = offset.x * camera->side.x + offset.y * camera->side.y + camera->side.z * offset.z;
    if (PAN_RANGE < across) {
        across = PAN_RANGE;
    } else if (across < -PAN_RANGE) {
        across = -PAN_RANGE;
    }
    if (narrow) {
        across *= PAN_NARROW;
    }
    share = (across + PAN_RANGE) / (PAN_RANGE + PAN_RANGE);
    rest = 1.0f - share;
    if (share < rest) {
        a = share * PAN_LOUD;
        b = rest + PAN_QUIET_RAISE * (1.0f - rest);
    } else {
        b = rest * PAN_LOUD;
        a = share + PAN_QUIET_RAISE * rest;
    }
    *upper = a;
    *lower = b;
    distance = Vec3fLength(&offset);
    if ((double)distance < SOUND_NEAREST) {
        distance = SOUND_NEAREST;
    }
    *volume = (s32)(SOUND_VOLUME_SCALE / distance);
}

void PlaySoundEffect(s32 sound) {
    SoundEffect *effect = &g_aSoundEffects[sound];

    InitSoundChannel(TRACK_SOUND_CHANNEL, effect->sample, effect->length, effect->loopLength);
    g_dwChannel3PlayingSfxId = sound;
}

void PlayBeepSound(void) {
    PlaySoundEffect(SOUND_EFFECT_BEEP);
}

void PlayGoSound(void) {
    PlaySoundEffect(SOUND_EFFECT_GO);
}

void ResetSoundSourceTracking(void) {
    g_ClosestSoundSource.sound = -1;
    g_ClosestSoundSource.source = -1;
    g_ClosestSoundSource.distance = SOUND_FAR;
}

void ResetAllSoundState(void) {
    g_dwChannel3PlayingSfxId = -1;
    g_ClosestSoundSource.listener = NULL;
    g_ClosestSoundSource.previousListener = NULL;
    g_ClosestSoundSource.sound = -1;
    g_ClosestSoundSource.previousSound = -1;
    g_ClosestSoundSource.source = -1;
    g_ClosestSoundSource.previousSource = -1;
    g_ClosestSoundSource.playing = FALSE;
    g_ClosestSoundSource.volume = 0;
    g_ClosestSoundSource.position.x = 0.0f;
    g_ClosestSoundSource.position.y = 0.0f;
    g_ClosestSoundSource.position.z = 0.0f;
    g_ClosestSoundSource.previousPosition.x = 0.0f;
    g_ClosestSoundSource.previousPosition.y = 0.0f;
    g_ClosestSoundSource.previousPosition.z = 0.0f;
    g_ClosestSoundSource.previousListenerPosition.x = 0.0f;
    g_ClosestSoundSource.previousListenerPosition.y = 0.0f;
    g_ClosestSoundSource.previousListenerPosition.z = 0.0f;
    /* The frequency is left as it was. */
    g_ClosestSoundSource.distance = SOUND_FAR;
}

void UpdateClosestSoundSource(
    s32 source, s32 sound, s32 volume, f32 frequency, Vec3f *position, CameraFrame *listener) {
    f32 distance = Vec3fDistance(position, &listener->position);

    if (distance < g_ClosestSoundSource.distance) {
        g_ClosestSoundSource.position.x = position->x;
        g_ClosestSoundSource.position.y = position->y;
        g_ClosestSoundSource.position.z = position->z;
        g_ClosestSoundSource.listener = listener;
        g_ClosestSoundSource.distance = distance;
        g_ClosestSoundSource.sound = sound;
        g_ClosestSoundSource.source = source;
        g_ClosestSoundSource.frequency = frequency;
        g_ClosestSoundSource.volume = volume;
    }
}

/* The step of a channel that plays a frequency. */
static u64 FrequencyStep(f32 frequency) {
    return (u64)((double)(frequency * (1.0f / SAMPLE_RATE)) * STEP_ONE);
}

/* The upper half of the volume of a channel, before the lower is added. */
static f32 UpperVolume(s32 volume, f32 upper) {
    return (f32)((s32)((f32)volume * upper) << 16);
}

/* Pack a volume panned between the two halves of the volume of a channel. */
static u32 PackVolume(s32 volume, f32 upper, f32 lower) {
    return (u32)(UpperVolume(volume, upper) + lower * (f32)volume);
}

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

void UpdateAirplaneSound(s32 instance, Vec3f *position, CameraFrame *camera) {
    f32 frequency = TRACK_SOUND_FREQUENCY;
    s32 volume = DEFAULT_SOUND_VOLUME;
    s32 sound = -1;

    switch (g_dwSelectedTrackId) {
    case TRACK_COASTLINE:
    case TRACK_MIRROR_COASTLINE:
        sound = SOUND_EFFECT_ENGINE;
        volume = LOUD_SOUND_VOLUME;
        frequency = FAST_AIRPLANE_FREQUENCY;
        break;
    case TRACK_JUNGLE:
        sound = SOUND_EFFECT_ENGINE;
        volume = LOUD_SOUND_VOLUME;
        break;
    case TRACK_MIRROR_JUNGLE:
        sound = SOUND_EFFECT_ENGINE;
        volume = LOUD_SOUND_VOLUME;
        break;
    }
    if (sound != -1) {
        UpdateClosestSoundSource(instance, sound, volume, frequency, position, camera);
    }
}

void UpdateWaterfallSound(s32 instance, Vec3f *position, CameraFrame *camera) {
    s32 sound = -1;
    f32 frequency = TRACK_SOUND_FREQUENCY;
    s32 volume = DEFAULT_SOUND_VOLUME;

    if (g_dwSelectedTrackId == TRACK_JUNGLE || g_dwSelectedTrackId == TRACK_MIRROR_JUNGLE) {
        sound = SOUND_EFFECT_WATERFALL;
        volume = WATERFALL_VOLUME;
    }
    if (sound != -1) {
        UpdateClosestSoundSource(instance, sound, volume, frequency, position, camera);
    }
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

/* Rumble the controller of a local player's vehicle. */
static void Rumble(Vehicle *vehicle, s16 onFrames, s16 offFrames, s16 frames) {
    if (vehicle->index < (s32)g_dwNumLocalPlayers) {
        g_awRumbleCounter[vehicle->index] = 0;
        g_awRumbleMotorOn[vehicle->index] = 0;
        g_awRumbleOnFrames[vehicle->index] = onFrames;
        g_awRumbleOffFrames[vehicle->index] = offFrames;
        g_awRumbleFramesLeft[vehicle->index] = frames;
        g_awRumbleActive[vehicle->index] = 1;
    }
}

/* Rumble for a skid, unless the controller already rumbles. */
static void RumbleSkid(Vehicle *vehicle, s16 offFrames, s16 frames) {
    if (vehicle->index < (s32)g_dwNumLocalPlayers && !g_awRumbleActive[vehicle->index]) {
        Rumble(vehicle, 1, offFrames, frames);
    }
}

/* Take a hit as the effect of a vehicle if it is louder, or matters more than the effect playing.
 */
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
    vehicle->effectVolume = skid;
    if (g_dwCurrentWeather == SNOW_WEATHER) {
        switch (surface) {
        case 0:
        case 1:
        case 2:
        case 3:
            vehicle->effectSound = SOUND_EFFECT_SKID_SURFACE_0;
            vehicle->effectVolume = skid >> 1;
            break;
        case TRACK_SURFACE_SLOW:
            vehicle->effectSound = SOUND_EFFECT_SKID_SURFACE_0;
            vehicle->effectVolume = 0;
            break;
        }
    } else {
        switch (surface) {
        case 0:
        case 3:
            RumbleSkid(vehicle, 9, 10);
            vehicle->effectSound = SOUND_EFFECT_SKID_SURFACE_0;
            break;
        case 1:
            RumbleSkid(vehicle, 14, 15);
            vehicle->effectSound = SOUND_EFFECT_SKID_SURFACE_1;
            break;
        case 2:
            RumbleSkid(vehicle, 12, 13);
            vehicle->effectSound = SOUND_EFFECT_SKID_SURFACE_2;
            break;
        case TRACK_SURFACE_SLOW:
            vehicle->effectSound = SOUND_EFFECT_SLOW_SURFACE;
            break;
        }
        vehicle->effectVolume = vehicle->body.skidding >> 1;
    }
    vehicle->effectPriority = PRIORITY_SKID;
    if (vehicle->skidFrames++ >= SKID_SWELL_FRAMES + 1) {
        vehicle->skidFrames = SKID_SWELL_FRAMES;
    }
    vehicle->effectVolume = vehicle->effectVolume * vehicle->skidFrames;
    vehicle->effectVolume = vehicle->effectVolume / SKID_SWELL_DIVISOR;
}

/* Choose the rolling of a vehicle on a surface, louder the faster it goes. */
static void ChooseRollEffect(Vehicle *vehicle, s32 surface) {
    if (g_dwCurrentWeather == SNOW_WEATHER) {
        switch (surface) {
        case 0:
        case 1:
        case 2:
        case 3:
            vehicle->effectSound = SOUND_EFFECT_ROLL_SNOW;
            break;
        case TRACK_SURFACE_SLOW:
            vehicle->effectSound = SOUND_EFFECT_ROLL_SURFACE_0;
            break;
        default:
            vehicle->effectSound = SOUND_EFFECT_NONE;
            break;
        }
    } else {
        switch (surface) {
        case 0:
        case 3:
            vehicle->effectSound = SOUND_EFFECT_ROLL_SURFACE_0;
            break;
        case 1:
            vehicle->effectSound = SOUND_EFFECT_ROLL_SURFACE_1;
            break;
        case 2:
            vehicle->effectSound = SOUND_EFFECT_ROLL_SURFACE_2;
            break;
        case TRACK_SURFACE_SLOW:
            vehicle->effectSound = SOUND_EFFECT_SLOW_SURFACE;
            break;
        default:
            vehicle->effectSound = SOUND_EFFECT_NONE;
            break;
        }
    }
    vehicle->effectPriority = PRIORITY_ROLL;
    vehicle->effectVolume =
        (s32)(Vec3fLength(&vehicle->placedVelocity) * (f32)ROLL_MAX_VOLUME / ROLL_FULL_SPEED);
    if (vehicle->effectVolume > ROLL_MAX_VOLUME) {
        vehicle->effectVolume = ROLL_MAX_VOLUME;
    }
}

/* Choose the effect of a vehicle from its hits, skids and rolling. */
static void ChooseVehicleEffect(Vehicle *vehicle, s32 channel, s32 previousSound) {
    s32 sound;
    s32 surface;
    s32 skid;

    if (g_aSoundEffectChannels[channel + 1].step == 0 ||
        (!vehicle->body.skidding && IsSkidSound(previousSound))) {
        vehicle->effectSound = SOUND_EFFECT_NONE;
        vehicle->effectVolume = 0;
        vehicle->effectPriority = 0;
    }
    TakeHitEffect(vehicle,
                  vehicle->body.vehicleImpactVolume,
                  SOUND_EFFECT_VEHICLE_IMPACT,
                  PRIORITY_VEHICLE_IMPACT,
                  5,
                  1,
                  30);
    vehicle->body.vehicleImpactVolume = 0;
    TakeHitEffect(
        vehicle, vehicle->body.impactVolume, SOUND_EFFECT_IMPACT, PRIORITY_IMPACT, 100, 1, 30);
    vehicle->body.impactVolume = 0;
    TakeHitEffect(vehicle, vehicle->body.landed, SOUND_EFFECT_LANDING, PRIORITY_LANDING, 3, 2, 20);
    vehicle->body.landed = 0;

    sound = vehicle->effectSound;
    if (sound != SOUND_EFFECT_NONE && !IsSkidSound(sound) && !IsRollSound(sound)) {
        return;
    }
    vehicle->body.unreconstructed202 = 0;
    surface = vehicle->wheelBodies[1].contactFrames ? vehicle->wheelBodies[1].surface : -1;
    skid = vehicle->body.skidding;
    /* A skid takes over from a quieter effect, from rolling, and from the skid of the last frame.
     */
    if (vehicle->effectVolume < skid || (skid && vehicle->effectPriority < PRIORITY_SKID) ||
        (skid && vehicle->effectPriority == PRIORITY_SKID && IsSkidSound(previousSound))) {
        ChooseSkidEffect(vehicle, surface, skid);
    } else {
        vehicle->skidFrames = 0;
    }
    sound = vehicle->effectSound;
    if (sound == SOUND_EFFECT_NONE || IsRollSound(sound)) {
        ChooseRollEffect(vehicle, surface);
    }
}

void UpdateVehicleSounds(Vehicle *vehicle) {
    Vehicle *viewed = &g_aVehicles[g_aViewportRects[0].vehicleIndex];
    s32 channel = vehicle->index * CHANNELS_PER_VEHICLE;
    CameraFrame *listener;
    SoundEffect *effect;
    s32 changed;
    s32 previousSound;
    s32 effectVolume;
    s32 volume;
    f32 upper;
    f32 lower;
    f32 frequency;

    if (g_dwRacePauseState || vehicle->racer == NULL) {
        g_aSoundEffectChannels[channel].volume = 0;
        g_aSoundEffectChannels[channel].step = 0;
        return;
    }
    changed = FALSE;
    if (g_dwNumViewports == 1) {
        listener = viewed->activeCamera;
        if (vehicle->cameraChanged || viewed->cameraChanged) {
            changed = TRUE;
        }
    } else {
        listener = vehicle->activeCamera;
        if (vehicle->cameraChanged || viewed->cameraChanged ||
            g_aVehicles[g_aViewportRects[1].vehicleIndex].cameraChanged) {
            changed = TRUE;
        }
    }
    vehicle->dopplerFactor = ComputeDopplerFactor(&vehicle->frame.position,
                                                  &vehicle->previousPosition,
                                                  &listener->position,
                                                  &vehicle->previousListenerPosition);
    ComputeStereoPan(&vehicle->frame.position, listener, &upper, &lower, &volume, FALSE);

    if (0.0f < vehicle->engineRpm) {
        frequency = vehicle->engineRpm * 0.5f * ENGINE_RPM_SCALE * SAMPLE_RATE;
    } else {
        frequency = vehicle->engineRpm * -0.5f * ENGINE_RPM_SCALE * SAMPLE_RATE;
    }
    frequency = vehicle->dopplerFactor * frequency;
    if (ENGINE_MAX_FREQUENCY < frequency) {
        /* Too high, the engine falls silent. */
        frequency = 0.0f;
    } else if (frequency < 0.0f) {
        frequency = 0.0f;
    }
    if (vehicle->index == 0) {
        if (g_LoadedTrack.instances[vehicle->groundInstances[0]].flags & TRACK_INSTANCE_SHELTER) {
            g_dwSoundEffectEchoOffset = SHELTER_ECHO_OFFSET;
        } else {
            g_dwSoundEffectEchoOffset = 0;
        }
    }
    if (!changed) {
        g_aSoundEffectChannels[channel].step = FrequencyStep(frequency);
        g_aSoundEffectChannels[channel].volume = PackVolume(volume, upper, lower);
    }

    previousSound = vehicle->effectSound;
    ChooseVehicleEffect(vehicle, channel, previousSound);

    /* Only the first vehicle plays its effects. */
    if (vehicle->effectSound == SOUND_EFFECT_NONE) {
        if (channel == 0) {
            g_aSoundEffectChannels[channel + 1].volume = 0;
            g_aSoundEffectChannels[channel + 1].step = 0;
        }
    } else {
        frequency = vehicle->dopplerFactor * SAMPLE_RATE;
        if (EFFECT_MAX_FREQUENCY < frequency) {
            frequency = 0.0f;
        } else if (frequency < 0.0f) {
            frequency = 0.0f;
        }
        if (previousSound != vehicle->effectSound && channel == 0) {
            effect = &g_aSoundEffects[vehicle->effectSound];
            StartSoundChannelMuted(
                (s16)(channel + 1), effect->sample, effect->length, effect->loopLength);
        }
        if (!changed && channel == 0) {
            effectVolume = (s32)((u32)vehicle->effectVolume * (u32)volume) >> 7;
            g_aSoundEffectChannels[channel + 1].step = FrequencyStep(frequency);
            g_aSoundEffectChannels[channel + 1].volume = PackVolume(effectVolume, upper, lower);
        }
    }
    if (g_dwNumViewports == 1) {
        vehicle->previousListenerPosition.x = viewed->activeCamera->position.x;
        vehicle->previousListenerPosition.y = viewed->activeCamera->position.y;
        vehicle->previousListenerPosition.z = viewed->activeCamera->position.z;
    } else {
        vehicle->previousListenerPosition.x = vehicle->activeCamera->position.x;
        vehicle->previousListenerPosition.y = vehicle->activeCamera->position.y;
        vehicle->previousListenerPosition.z = vehicle->activeCamera->position.z;
    }
}
