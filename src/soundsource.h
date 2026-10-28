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

/**
 * Offer the sound of the airplane as the nearest sound of the track, on the tracks that have one.
 *
 * @param instance The placed model of the airplane.
 * @param position Its position.
 * @param camera The camera that hears it.
 * @ghidraAddress USA: 0x8022b954
 * @ghidraAddress EUR: 0x8022ce64
 * @ghidraAddress JPN: 0x8022c61c
 * @ghidraAddress BRD3D: 0x10068180
 * @ghidraAddress BRGlide: 0x100611f0
 */
void UpdateAirplaneSound(s32 instance, Vec3f *position, CameraFrame *camera);

/**
 * Offer the sound of a waterfall as the nearest sound of the track, on the tracks that have one.
 *
 * @param instance The source the sound is kept under.
 * @param position The position of the waterfall.
 * @param camera The camera that hears it.
 * @ghidraAddress USA: 0x8022ba10
 * @ghidraAddress EUR: 0x8022cf20
 * @ghidraAddress JPN: 0x8022c6d8
 * @ghidraAddress BRD3D: 0x10068210
 * @ghidraAddress BRGlide: 0x10061280
 */
void UpdateWaterfallSound(s32 instance, Vec3f *position, CameraFrame *camera);

/**
 * The factor the Doppler effect raises a sound by, from the motion of the source and the listener
 * along the line between them over the frame.
 *
 * @param source The source.
 * @param previousSource Where the source was.
 * @param listener The listener.
 * @param previousListener Where the listener was.
 * @return The factor.
 * @ghidraAddress USA: 0x8022b0f8
 * @ghidraAddress EUR: 0x8022c608
 * @ghidraAddress JPN: 0x8022bdc0
 * @ghidraAddress BRD3D: 0x10067ae0
 * @ghidraAddress BRGlide: 0x10060b50
 */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
long double
#else
f32
#endif
ComputeDopplerFactor(Vec3f *source,
                     Vec3f *previousSource,
                     Vec3f *listener,
                     Vec3f *previousListener);

/**
 * Offer a sound of the track as the nearest this frame; the nearest plays.
 *
 * @param source The source the sound is kept under.
 * @param sound The sound effect.
 * @param volume The volume, out of 256.
 * @param frequency The frequency, in hertz, negative for a source that does not move.
 * @param position The position of the source.
 * @param listener The camera that hears it.
 * @ghidraAddress USA: 0x8022b494
 * @ghidraAddress EUR: 0x8022c9a4
 * @ghidraAddress JPN: 0x8022c15c
 * @ghidraAddress BRD3D: 0x10067e50
 * @ghidraAddress BRGlide: 0x10060ec0
 */
void UpdateClosestSoundSource(
    s32 source, s32 sound, s32 volume, f32 frequency, Vec3f *position, CameraFrame *listener);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SOUND_SOURCE_H */
