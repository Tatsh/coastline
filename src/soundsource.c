#include "soundsource.h"

#include <stddef.h>

#include "soundeffect.h"

#define SOUND_FAR 8589934592.0f

void UpdateClosestSoundSource(
    s32 source, s32 sound, s32 volume, f32 frequency, Vec3f *position, CameraFrame *listener) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double distance = Vec3fDistance(position, &listener->position);
#else
    f32 distance = Vec3fDistance(position, &listener->position);
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
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
