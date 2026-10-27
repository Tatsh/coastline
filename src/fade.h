#ifndef COASTLINE_FADE_H
#define COASTLINE_FADE_H

/**
 * @file
 * The fades of the screen, the sound effects and the music, and the fog over the view.
 *
 * A fade level runs from 0, black or silent, to 1, the picture shown or the full volume. The screen
 * fades in and out as a wipe: black covers the screen from the right as it fades in, and from the
 * left as it fades out.
 */

#include <PR/ultratypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The edges of the picture the wipe leaves, in pixels.
 *
 * @ghidraAddress USA: 0x8028b740
 * @ghidraAddress EUR: 0x8028d9f0
 * @ghidraAddress JPN: 0x8028d420
 * @ghidraAddress BRD3D: 0x10575508
 */
extern s32 g_dwFadeRectLeft;

/**
 * @ghidraAddress USA: 0x8028b744
 * @ghidraAddress EUR: 0x8028d9f4
 * @ghidraAddress JPN: 0x8028d424
 * @ghidraAddress BRD3D: 0x10575500
 */
extern s32 g_dwFadeRectRight;

/**
 * @ghidraAddress USA: 0x8028b748
 * @ghidraAddress EUR: 0x8028d9f8
 * @ghidraAddress JPN: 0x8028d428
 * @ghidraAddress BRD3D: 0x1057550c
 */
extern s32 g_dwFadeRectTop;

/**
 * @ghidraAddress USA: 0x8028b74c
 * @ghidraAddress EUR: 0x8028d9fc
 * @ghidraAddress JPN: 0x8028d42c
 * @ghidraAddress BRD3D: 0x105754fc
 */
extern s32 g_dwFadeRectBottom;

/**
 * How much the headlights shining at the camera whiten the view, from 0 to 1.
 *
 * @ghidraAddress USA: 0x8028b750
 * @ghidraAddress EUR: 0x8028da00
 * @ghidraAddress JPN: 0x8028d430
 */
extern f32 g_fHeadlightGlare;

/**
 * The level the screen fades to.
 *
 * @ghidraAddress USA: 0x8028b754
 * @ghidraAddress EUR: 0x8028da04
 * @ghidraAddress JPN: 0x8028d434
 * @ghidraAddress BRD3D: 0x10575510
 */
extern f32 g_fFadeTarget;

/**
 * The change of the level each second, below 0 when fading out.
 *
 * @ghidraAddress USA: 0x8028b758
 * @ghidraAddress EUR: 0x8028da08
 * @ghidraAddress JPN: 0x8028d438
 */
extern f32 g_fFadeRate;

/**
 * @ghidraAddress USA: 0x8028b75c
 * @ghidraAddress EUR: 0x8028da0c
 * @ghidraAddress JPN: 0x8028d43c
 * @ghidraAddress BRD3D: 0x10575518
 */
extern f32 g_fFadeLevel;

/**
 * The fade of the music: its target, its change each second and its level.
 *
 * @ghidraAddress USA: 0x8028b760
 * @ghidraAddress EUR: 0x8028da10
 * @ghidraAddress JPN: 0x8028d440
 */
extern f32 g_fMusicFadeTarget;

/**
 * @ghidraAddress USA: 0x8028b764
 * @ghidraAddress EUR: 0x8028da14
 * @ghidraAddress JPN: 0x8028d444
 */
extern f32 g_fMusicFadeSpeed;

/**
 * @ghidraAddress USA: 0x8028b768
 * @ghidraAddress EUR: 0x8028da18
 * @ghidraAddress JPN: 0x8028d448
 */
extern f32 g_fMusicFadeCurrent;

/**
 * The fade of the sound effects: its target, its change each second and its level.
 *
 * @ghidraAddress USA: 0x8028b76c
 * @ghidraAddress EUR: 0x8028da1c
 * @ghidraAddress JPN: 0x8028d44c
 */
extern f32 g_fSfxFadeTarget;

/**
 * @ghidraAddress USA: 0x8028b770
 * @ghidraAddress EUR: 0x8028da20
 * @ghidraAddress JPN: 0x8028d450
 */
extern f32 g_fSfxFadeSpeed;

/**
 * @ghidraAddress USA: 0x8028b774
 * @ghidraAddress EUR: 0x8028da24
 * @ghidraAddress JPN: 0x8028d454
 */
extern f32 g_fSfxFadeCurrent;

/**
 * Whether the next UpdateFadeTransitions() leaves the level of each fade as it is, set when a fade
 * starts.
 *
 * @ghidraAddress USA: 0x8028b778
 * @ghidraAddress EUR: 0x8028da28
 * @ghidraAddress JPN: 0x8028d458
 */
extern s32 g_dwFadeSkipFrame;

/**
 * @ghidraAddress USA: 0x8028b77c
 * @ghidraAddress EUR: 0x8028da2c
 * @ghidraAddress JPN: 0x8028d45c
 */
extern s32 g_dwMusicFadeSkipFrame;

/**
 * @ghidraAddress USA: 0x8028b780
 * @ghidraAddress EUR: 0x8028da30
 * @ghidraAddress JPN: 0x8028d460
 */
extern s32 g_dwSfxFadeSkipFrame;

/**
 * Whether the screen fades out as soon as it has faded in, because a fade out was asked for while
 * it faded in.
 *
 * @ghidraAddress USA: 0x8028b784
 * @ghidraAddress EUR: 0x8028da34
 * @ghidraAddress JPN: 0x8028d464
 */
extern s32 g_dwFadeAutoReverse;

/**
 * The edges of the wipe when each framebuffer was last drawn.
 *
 * @ghidraAddress USA: 0x8031b410
 * @ghidraAddress EUR: 0x8031d910
 * @ghidraAddress JPN: 0x8031c540
 */
extern s32 g_adwFadeRectLeftPerBuffer[2];

/**
 * @ghidraAddress USA: 0x8031b418
 * @ghidraAddress EUR: 0x8031d918
 * @ghidraAddress JPN: 0x8031c548
 */
extern s32 g_adwFadeRectRightPerBuffer[2];

/**
 * Clear the glare of the headlights over the view.
 *
 * @ghidraAddress USA: 0x80223470
 * @ghidraAddress EUR: 0x802247b0
 * @ghidraAddress JPN: 0x80223fa0
 */
void ResetHeadlightGlare(void);

/**
 * Draw the glare of the headlights over the view as white of its opacity, when it is strong
 * enough.
 *
 * @ghidraAddress USA: 0x80223480
 * @ghidraAddress EUR: 0x802247c0
 * @ghidraAddress JPN: 0x80223fb0
 */
void RenderHeadlightGlare(void);

/**
 * Fade the screen towards a level over a time in seconds. A fade out asked for while the screen
 * fades in waits until it has faded in.
 *
 * @param target The level to fade to, from 0 for black to 1 for the whole picture.
 * @param seconds The time a fade across the whole range takes, in seconds.
 * @ghidraAddress USA: 0x80223688
 * @ghidraAddress EUR: 0x802249c8
 * @ghidraAddress JPN: 0x802241b8
 */
void SetFadeTarget(float target, float seconds);

/**
 * Fade the volume of the sound effects towards a level over a time in seconds.
 *
 * @param target The volume to fade to, from 0 for silence to 1 for full volume.
 * @param seconds The time a fade across the whole range takes, in seconds.
 * @ghidraAddress USA: 0x80223750
 * @ghidraAddress EUR: 0x80224a90
 * @ghidraAddress JPN: 0x80224280
 */
void FadeSfxTo(float target, float seconds);

/**
 * Fade the volume of the music towards a level over a time in seconds.
 *
 * @param target The volume to fade to, from 0 for silence to 1 for full volume.
 * @param seconds The time a fade across the whole range takes, in seconds.
 * @ghidraAddress USA: 0x802237d0
 * @ghidraAddress EUR: 0x80224b10
 * @ghidraAddress JPN: 0x80224300
 */
void FadeMusicTo(float target, float seconds);

/**
 * Whether the fade of the sound effects has reached its target.
 *
 * @return 1 if the volume equals its target, 0 otherwise.
 * @ghidraAddress USA: 0x80223850
 * @ghidraAddress EUR: 0x80224b90
 * @ghidraAddress JPN: 0x80224380
 */
s32 HasSfxFadeFinished(void);

/**
 * Set the volume of the sound effects and its target at once.
 *
 * @param level The volume, from 0 for silence to 1 for full volume.
 * @ghidraAddress USA: 0x80223880
 * @ghidraAddress EUR: 0x80224bc0
 * @ghidraAddress JPN: 0x802243b0
 */
void SetSfxFadeLevelImmediate(float level);

/**
 * Set the level of the screen and its target at once.
 *
 * @param level The level, from 0 for black to 1 for the whole picture.
 * @ghidraAddress USA: 0x8022389c
 * @ghidraAddress EUR: 0x80224bdc
 * @ghidraAddress JPN: 0x802243cc
 * @ghidraAddress BRD3D: 0x1002b280
 */
void SnapScreenFade(float level);

/**
 * Whether the screen fades in and does not turn back by itself.
 *
 * @return 1 if the screen fades in, 0 otherwise.
 * @ghidraAddress USA: 0x802238b8
 * @ghidraAddress EUR: 0x80224bf8
 * @ghidraAddress JPN: 0x802243e8
 */
s32 IsFadingIn(void);

/**
 * Whether the screen fades out, or will once it has faded in.
 *
 * @return 1 if the screen fades out or will, 0 otherwise.
 * @ghidraAddress USA: 0x802238fc
 * @ghidraAddress EUR: 0x80224c3c
 * @ghidraAddress JPN: 0x8022442c
 */
s32 IsFadingOut(void);

/**
 * Whether the screen has reached its target and does not turn back by itself.
 *
 * @return 1 if the fade is complete, 0 otherwise.
 * @ghidraAddress USA: 0x80223940
 * @ghidraAddress EUR: 0x80224c80
 * @ghidraAddress JPN: 0x80224470
 */
s32 IsFadeComplete(void);

/**
 * Whether the screen has faded out to black.
 *
 * @return 1 if the screen is black, 0 otherwise.
 * @ghidraAddress USA: 0x80223988
 * @ghidraAddress EUR: 0x80224cc8
 * @ghidraAddress JPN: 0x802244b8
 */
s32 IsFadedToBlack(void);

/**
 * Whether the screen has faded in to the whole picture.
 *
 * @return 1 if the whole picture shows, 0 otherwise.
 * @ghidraAddress USA: 0x802239f4
 * @ghidraAddress EUR: 0x80224d34
 * @ghidraAddress JPN: 0x80224524
 */
s32 IsFadedToWhite(void);

/**
 * @ghidraAddress USA: 0x80223a68
 * @ghidraAddress EUR: 0x80224da8
 * @ghidraAddress JPN: 0x80224598
 */
void UnusedScreenFadeStub(void);

/**
 * Draw the wipe of the frame: black over the edges the fade has covered.
 *
 * @ghidraAddress USA: 0x80223a70
 * @ghidraAddress EUR: 0x80224db0
 * @ghidraAddress JPN: 0x802245a0
 */
void ApplyScreenFade(void);

/**
 * Draw black over the whole screen, more opaque the lower a level from 0 to 1.
 *
 * @param level The level, from 0 for opaque black to 1 for no overlay.
 * @ghidraAddress USA: 0x80223de0
 * @ghidraAddress EUR: 0x80225120
 * @ghidraAddress JPN: 0x80224910
 */
void DrawScreenFadeOverlay(float level);

/**
 * Step the fades of the screen, the sound effects and the music by the time of the frame, move
 * the edges of the wipe, and set the volumes.
 *
 * @ghidraAddress USA: 0x80223f54
 * @ghidraAddress EUR: 0x80225294
 * @ghidraAddress JPN: 0x80224a84
 */
void UpdateFadeTransitions(void);

#ifdef __cplusplus
}
#endif

#endif
