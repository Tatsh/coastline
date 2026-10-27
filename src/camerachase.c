#include "camera.h"
#include "game.h"

/* The distances of the chase camera behind and above the vehicle. */
#define CHASE_DISTANCE 11.0f
#define CHASE_DISTANCE_SPLIT_SCREEN 19.8f
#define CHASE_HEIGHT 2.4f
#define CHASE_DISTANCE_AFTER_SEASON 13.0f

void UpdateCameraChase(Vehicle *vehicle, CameraFrame *camera, f32 share) {
    Vec3f offset;
    f32 previous[3];
    /* The original reserves stack for eight bytes of locals it does not use. */
    f32 unused[2];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double length;
#else
    f32 length;
#endif

    if (vehicle->flyMode) {
        Vec3fMulAdd(&camera->position, &vehicle->frame.position, &vehicle->frame.up, CHASE_HEIGHT);
        Vec3fMulAdd(&camera->position,
                    &camera->position,
                    &vehicle->frame.forward,
                    g_dwNumViewports == 1 ? -CHASE_DISTANCE : -CHASE_DISTANCE_SPLIT_SCREEN);
        return;
    }

    /* The original keeps a copy of the old position that it does not use. */
    previous[0] = camera->position.x;
    previous[1] = camera->position.y;
    previous[2] = camera->position.z;
    (void)previous;
    (void)unused;

    /* The direction the camera was in from the vehicle, at the chase distance. */
    camera->position.z -= CHASE_HEIGHT;
    Vec3fSubtract(&offset, &camera->position, &vehicle->frame.position);
    length = Vec3fLength(&offset);
    if (length != 0.0f) {
        if (g_dwNumViewports == 1) {
            Vec3fScaleInPlace(&offset, CHASE_DISTANCE / length);
        } else {
            Vec3fScaleInPlace(&offset, CHASE_DISTANCE_SPLIT_SCREEN / length);
        }
    }

    /* The direction behind the vehicle, or ahead of it in an instant replay. */
    if (gInstantReplayActive) {
        Vec3fScale(&camera->position, &vehicle->frame.forward, CHASE_DISTANCE);
    } else if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        Vec3fScale(&camera->position, &vehicle->frame.side, -CHASE_DISTANCE);
        Vec3fMulAddInPlace(
            &camera->position, &vehicle->frame.forward, -CHASE_DISTANCE_AFTER_SEASON);
    } else {
        Vec3fScale(&camera->position, &vehicle->frame.forward, -CHASE_DISTANCE);
    }
    length = Vec3fLength(&camera->position);
    if (length != 0.0f) {
        Vec3fScaleInPlace(&camera->position, CHASE_DISTANCE / length);
    }

    Vec3fLerp(&camera->position, &camera->position, &offset, share);
    Vec3fAddInPlace(&camera->position, &vehicle->frame.position);
    camera->position.z += CHASE_HEIGHT;
}
