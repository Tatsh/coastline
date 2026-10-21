/* Host check of racesound.c: the Doppler factor, the pan and the volume packing. */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "racesound.c"

f32 g_fFrameDeltaTime = 1.0f;
SoundEffect g_aSoundEffects[SOUND_EFFECTS];
s32 g_dwChannel3PlayingSfxId;
SoundSourceTracking g_ClosestSoundSource;

#define NEAR(a, b) (fabsf((a) - (b)) < 0.001f)

int main(void) {
    Vec3f source = {0, 0, 0};
    Vec3f previousSource = {0, 0, 0};
    Vec3f listener = {100, 0, 0};
    Vec3f previousListener = {100, 0, 0};
    CameraFrame camera;
    f32 upper;
    f32 lower;
    s32 volume;
    Vec3f at = {0, 10, 0};

    assert(
        NEAR(ComputeDopplerFactor(&source, &previousSource, &listener, &previousListener), 1.0f));
    /* A source coming 34.3 a second nearer raises the sound by a tenth over 0.9. */
    previousSource.x = -34.3f;
    assert(NEAR(ComputeDopplerFactor(&source, &previousSource, &listener, &previousListener),
                1.0f / 0.9f));

    camera.position.x = camera.position.y = camera.position.z = 0.0f;
    camera.side.x = 0.0f;
    camera.side.y = 1.0f;
    camera.side.z = 0.0f;
    ComputeStereoPan(&at, &camera, &upper, &lower, &volume, FALSE);
    /* Fully to the side: the upper half full, the lower at 0, and 1024/32 for a near source. */
    assert(NEAR(upper, 1.0f) && NEAR(lower, 0.0f) && volume == 32);
    at.y = 0.0f;
    at.x = 64.0f;
    ComputeStereoPan(&at, &camera, &upper, &lower, &volume, FALSE);
    /* At the middle the halves are not equal: 0.8 and 0.85. */
    assert(NEAR(upper, 0.8f) && NEAR(lower, 0.85f) && volume == 16);
    assert(PackVolume(16, 1.0f, 1.0f) == ((16u << 16) | 16u));

    ResetAllSoundState();
    UpdateClosestSoundSource(7, SOUND_EFFECT_WATERFALL, 0x180, 11000.0f, &at, &camera);
    assert(g_ClosestSoundSource.source == 7 &&
           g_ClosestSoundSource.sound == SOUND_EFFECT_WATERFALL);

    puts("racesound ok");
    return 0;
}
