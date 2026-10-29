#include "common/soundsource.h"

#include <stddef.h>

#include "common/frametime.h"
#include "common/soundeffect.h"
#include "common/trackselect.h"

#define SOUND_FAR 8589934592.0f

#define TRACK_SOUND_FREQUENCY 11000.0f
#define FAST_AIRPLANE_FREQUENCY 22000.0f
#define DEFAULT_SOUND_VOLUME 0x80
#define LOUD_SOUND_VOLUME 0xFF
#define WATERFALL_VOLUME 0x180

void UpdateAirplaneSound(s32 instance, Vec3f *position, CameraFrame *camera) {
    s32 sound = -1;
    f32 frequency = TRACK_SOUND_FREQUENCY;
    s32 volume = DEFAULT_SOUND_VOLUME;

    switch (g_dwSelectedTrackId) {
    case TRACK_COASTLINE:
    case TRACK_MIRROR_COASTLINE:
        sound = SOUND_EFFECT_ENGINE;
        frequency = FAST_AIRPLANE_FREQUENCY;
        volume = LOUD_SOUND_VOLUME;
        break;
    case TRACK_JUNGLE:
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

#if TGR_BOSS_RALLY
long double
#else
f32
#endif
ComputeDopplerFactor(Vec3f *source,
                     Vec3f *previousSource,
                     Vec3f *listener,
                     Vec3f *previousListener) {
    Vec3f direction;
    f32 length;
    Vec3f sourceStep;
    Vec3f listenerStep;
    f32 approach;
#if TGR_BOSS_RALLY
    long double listenerSpeed;
    long double rawLength;
#else
    f32 listenerSpeed;
#endif

    Vec3fSubtract(&direction, source, listener);
    Vec3fSubtract(&listenerStep, listener, previousListener);
    Vec3fSubtract(&sourceStep, source, previousSource);
#if TGR_BOSS_RALLY
    rawLength = Vec3fLength(&direction);
    length = rawLength;
    if (rawLength < 0.0L || rawLength > 0.0L) {
#else
    length = Vec3fLength(&direction);
    if (length != 0.0f) {
#endif
        Vec3fDivideInPlace(&direction, length);
    }
    approach = -Vec3fDot(&sourceStep, &direction) / g_fFrameDeltaTime;
    listenerSpeed = Vec3fDot(&listenerStep, &direction) / g_fFrameDeltaTime;
#if TGR_BOSS_RALLY
    return (1.0L - listenerSpeed * -0x1.7e2256p-9f) /
           (1.0L - (long double)approach * 0x1.7e2256p-9f);
#else
    return (listenerSpeed / 343.0f + 1.0f) / (1.0f - approach / 343.0f);
#endif
}

void UpdateClosestSoundSource(
    s32 source, s32 sound, s32 volume, f32 frequency, Vec3f *position, CameraFrame *listener) {
#if TGR_BOSS_RALLY
    long double distance = Vec3fDistance(position, &listener->position);
#else
    f32 distance = Vec3fDistance(position, &listener->position);
#endif

#if TGR_BOSS_RALLY
    if (!(distance >= g_ClosestSoundSource.distance)) {
#else
    if (distance < g_ClosestSoundSource.distance) {
#endif
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
