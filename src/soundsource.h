#ifndef COASTLINE_SOUND_SOURCE_H
#define COASTLINE_SOUND_SOURCE_H

#include "render.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The nearest sound of the track this frame and the last. */
typedef struct {
    Vec3f position;                 /*!< Where the nearest sound is. */
    Vec3f previousPosition;         /*!< Where the sound was in the last frame. */
    CameraFrame *listener;          /*!< The camera that hears the sound. */
    CameraFrame *previousListener;  /*!< The camera that heard the sound in the last frame. */
    Vec3f previousListenerPosition; /*!< Where the camera was in the last frame. */
    s32 sound;                      /*!< The sound effect, or -1. */
    s32 previousSound;              /*!< The sound effect of the last frame, or -1. */
    s32 source;                     /*!< The source of the sound, or -1. */
    s32 previousSource;             /*!< The source of the last frame, or -1. */
    f32 distance;                   /*!< How far the sound is from its camera. */
    f32 frequency; /*!< The frequency in hertz, negative for a source that does not move. */
    s32 volume;    /*!< The volume out of 256. */
    s32 playing;   /*!< Whether the sound plays. */
} SoundSourceTracking;

/**
 * The nearest sound of the track.
 *
 * @ghidraAddress USA: 0x8028b7a0
 * @ghidraAddress EUR: 0x8028da50
 * @ghidraAddress JPN: 0x8028d480
 * @ghidraAddress BRD3D: 0x10af9b58
 * @ghidraAddress BRGlide: 0x10b1ceb8
 */
extern SoundSourceTracking g_ClosestSoundSource;

/**
 * Forget the nearest sound of the track found the last frame.
 *
 * @ghidraAddress USA: 0x8022b404
 * @ghidraAddress EUR: 0x8022c914
 * @ghidraAddress JPN: 0x8022c0cc
 * @ghidraAddress BRD3D: 0x10067da0
 * @ghidraAddress BRGlide: 0x10060e10
 */
void ResetSoundSourceTracking(void);

/**
 * Stop the sounds of the track and forget their sources.
 *
 * @ghidraAddress USA: 0x8022b428
 * @ghidraAddress EUR: 0x8022c938
 * @ghidraAddress JPN: 0x8022c0f0
 * @ghidraAddress BRD3D: 0x10067dc0
 * @ghidraAddress BRGlide: 0x10060e30
 */
void ResetAllSoundState(void);

#ifdef __cplusplus
}
#endif

#endif
