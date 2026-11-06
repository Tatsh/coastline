#include <PR/gu.h>

#include "common/camera.h"
#include "common/driving.h"
#include "common/game.h"
#include "common/input.h"
#include "common/render.h"
#include "common/vehiclephysics.h"
#include "common/vehiclereset.h"

void ProcessVehicleInputAndPhysics(Vehicle *vehicle) {
#if TGR_REGION == TGR_REGION_US
    static s32 replayCockpitNext;
    f32 chosen[3];
    Vec3f *point;
    Vec3f *selected;
    s32 unusedScan[8];
#endif
    Vec3f ahead;
    s32 unusedControl[2];
    f32 velocity[3];
    Vec3f previousOffset;
    Vec3f predicted;
    Vec3f corner;
    f32 best TGR_MATCHING_UNUSED;
    f32 dx TGR_MATCHING_UNUSED;
    f32 dz TGR_MATCHING_UNUSED;
    f32 length;
    f32 jolt;
    s32 i TGR_MATCHING_UNUSED;
    f32 dy;
#if TGR_REGION == TGR_REGION_US
    s32 unusedWord;
#endif

#if TGR_REGION == TGR_REGION_US
    (void)unusedScan;
    (void)unusedWord;
#endif
    (void)unusedControl;
    if (g_dwTrackMirrored != 0) {
        vehicle->input->steering = -vehicle->input->steering;
    }
    if (vehicle->input->steering > 0.1f) {
        vehicle->steering = (vehicle->input->steering - 0.1f) / 0.9f;
        vehicle->steering = vehicle->steering * -vehicle->steering;
    } else if (vehicle->input->steering < -0.1f) {
        vehicle->steering = (vehicle->input->steering + 0.1f) / 0.9f;
        vehicle->steering = vehicle->steering * vehicle->steering;
    } else {
        vehicle->steering = 0.0f;
    }
    if ((vehicle->racer->status & RACER_FINISHED) &&
        (vehicle->index == g_aViewportRects[0].vehicleIndex ||
         (g_dwNumViewports >= 2 && vehicle->index == g_aViewportRects[1].vehicleIndex))) {
        if (g_dwReplayCameraSwitchTimer == 0) {
            goto chooseReplayCamera;
        }
    } else if ((g_dwGameMode == GAME_MODE_DEMO || g_dwGameMode == GAME_MODE_AFTER_SEASON ||
                gInstantReplayActive != 0) &&
               (vehicle->index == g_aViewportRects[0].vehicleIndex ||
                (g_dwNumViewports >= 2 && vehicle->index == g_aViewportRects[1].vehicleIndex))) {
        if (g_dwReplayCameraSwitchTimer == 0) {
        chooseReplayCamera:
#if TGR_REGION != TGR_REGION_US
            ChooseReplayCamera(vehicle);
#else
            best = 16777216.0f;
#ifdef ENABLE_PATCHES
            chosen[0] = chosen[1] = chosen[2] = 0.0f;
#endif
            for (i = 0; i < g_LoadedTrack.cameraPointCount; i++) {
                point = &g_LoadedTrack.cameraPoints[i];
                dx = vehicle->frame.position.x - point->x;
                dy = vehicle->frame.position.y - point->y;
                dx = dx * dx;
                dz = vehicle->frame.position.z - point->z;
                dy = dy * dy;
                if (!(dz > 10.0f)) {
                    dz = dz * dz;
                    if ((length = dx + dy + dz) < best) {
                        selected = &g_LoadedTrack.cameraPoints[i];
                        chosen[0] = selected->x;
                        chosen[1] = selected->y;
                        chosen[2] = selected->z;
                        best = length;
                    }
                }
            }
            if (best < 22500.0f) {
                if (vehicle->activeCamera != &vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE] ||
                    vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].position.x != chosen[0] ||
                    vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].position.y != chosen[1] ||
                    vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].position.z != chosen[2]) {
                    vehicle->cameraChanged = 1;
                }
                vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].position.x = chosen[0];
                vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].position.y = chosen[1];
                vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].position.z = chosen[2];
                vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE];
            } else if (vehicle->activeCamera == &vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE]) {
                vehicle->cameraChanged = 1;
                if (replayCockpitNext != 0 || g_dwReplayCameraHoldTimer != 0) {
                    vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_COCKPIT];
                } else {
                    vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_CHASE];
                }
                replayCockpitNext = replayCockpitNext == 0;
                g_dwReplayCameraSwitchTimer = 60;
            }
#endif
        } else {
            g_dwReplayCameraSwitchTimer--;
        }
        if (vehicle->input->buttons & INPUT_VIEWS) {
            g_dwReplayCameraSwitchTimer = 450;
        }
    } else if (vehicle->index == g_aViewportRects[0].vehicleIndex ||
               (g_dwNumViewports >= 2 && vehicle->index == g_aViewportRects[1].vehicleIndex)) {
        g_dwReplayCameraSwitchTimer = 0;
    }
    if (vehicle->input->buttons & INPUT_VIEW_COCKPIT) {
        vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_COCKPIT];
        vehicle->cameraChanged = 1;
        TransferFlags(vehicle->input, INPUT_VIEW_COCKPIT);
    }
    if (vehicle->input->buttons & INPUT_VIEW_TRACKSIDE) {
        PlaceTrackCamera(vehicle);
        vehicle->cameraChanged = 1;
        TransferFlags(vehicle->input, INPUT_VIEW_TRACKSIDE);
    }
    if (vehicle->input->buttons & INPUT_VIEW_CHASE) {
        vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_CHASE];
        vehicle->cameraChanged = 1;
        TransferFlags(vehicle->input, INPUT_VIEW_CHASE);
    }
    if (g_dwNumViewports == 1 && (vehicle->input->buttons & INPUT_VIEW_BUMPER)) {
        vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_BUMPER];
        vehicle->cameraChanged = 1;
        TransferFlags(vehicle->input, INPUT_VIEW_BUMPER);
    }
    if (vehicle->input->stickY > 0.25f) {
        vehicle->accelerator = (vehicle->input->stickY - 0.25f) / 0.75f;
        vehicle->accelerator = vehicle->accelerator * vehicle->accelerator;
    } else if (vehicle->input->stickY < -0.25f) {
        vehicle->accelerator = (vehicle->input->stickY + 0.25f) / 0.75f;
        vehicle->accelerator = vehicle->accelerator * -vehicle->accelerator;
    } else {
        vehicle->accelerator = 0.0f;
    }
    if (vehicle->flyMode != 0) {
        vehicle->rollInput = 0.0f;
    }
    /* IDO changes register allocation when these 1.f literals use another spelling. */
    if (vehicle->input->buttons & INPUT_Z) {
        if (vehicle->flyMode == 0) {
            vehicle->accelerator = 1.f;
        }
    } else if (vehicle->flyMode != 0) {
        vehicle->pitchInput = vehicle->accelerator;
        vehicle->accelerator = 0.0f;
    }
    if (vehicle->input->buttons & INPUT_UP) {
        vehicle->accelerator = 1.f;
    }
    if (vehicle->input->buttons & INPUT_DOWN) {
        vehicle->accelerator = -1.0f;
    }
    if (vehicle->input->buttons & INPUT_RIGHT) {
        if (vehicle->flyMode != 0) {
            vehicle->rollInput = 1.f;
        } else {
            vehicle->steering = -1.0f;
        }
    }
    if (vehicle->input->buttons & INPUT_LEFT) {
        if (vehicle->flyMode != 0) {
            vehicle->rollInput = -1.0f;
        } else {
            vehicle->steering = 1.f;
        }
    }
    if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        Vec3fMulAddInPlace(&vehicle->frame.position, &vehicle->frame.forward, 15.0f);
        ComputeTrackPosition(vehicle);
        ahead.x = vehicle->trackForward.x;
        ahead.y = vehicle->trackForward.y;
        ahead.z = vehicle->trackForward.z;
        Vec3fMulAddInPlace(&vehicle->frame.position, &vehicle->frame.forward, -15.0f);
        ComputeTrackPosition(vehicle);
        vehicle->input->buttons &= 0xF0C0FFFF;
        if (vehicle->distance < 114.5f) {
            Vec3fScale((Vec3f *)velocity, &vehicle->trackForward, 27.0f);
        } else {
            Vec3fScale(
                (Vec3f *)velocity, &vehicle->trackForward, (168.5f - vehicle->distance) * 0.5f);
            if (167.5f < vehicle->distance) {
                vehicle->input->buttons |= INPUT_BRAKE;
                if (168.4f < vehicle->distance) {
                    velocity[0] = velocity[1] = velocity[2] = 0.0f;
                }
            } else {
                vehicle->input->buttons |= INPUT_EBRAKE;
            }
        }
        vehicle->steerAngle = Vec3fDot(&ahead, &vehicle->frame.side) * 3.0f;
        velocity[0] = velocity[0] - vehicle->trackOffset.x * 0.2f;
        velocity[1] = velocity[1] - vehicle->trackOffset.y * 0.2f;
        SetVehicleVelocity(vehicle, velocity[0], velocity[1], velocity[2]);
    }
    UpdateVehiclePhysicsFrame(vehicle);
    if (vehicle->flyMode == 0) {
        Vec3fAdd(&corner, &vehicle->frame.position, &vehicle->frame.forward);
        Vec3fAddInPlace(&corner, &vehicle->frame.side);
        Vec3fAdd(&predicted, &vehicle->joltPosition, &vehicle->joltVelocity);
        previousOffset.x = vehicle->joltOffset.x;
        previousOffset.y = vehicle->joltOffset.y;
        previousOffset.z = vehicle->joltOffset.z;
        Vec3fSubtract(&vehicle->joltOffset, &vehicle->joltPosition, &corner);
        length = Vec3fLength(&vehicle->joltOffset);
        if (length != 0.0f) {
            Vec3fScaleInPlace(&vehicle->joltOffset, length / (1.0f + length) / length);
        }
        dy = Vec3fDistance(&vehicle->joltOffset, &previousOffset);
        jolt = sqrtf(dy) / (Vec3fLength(&vehicle->joltVelocity) + 1.0f);
        if (0.025f < jolt) {
            AddVehicleJolt(vehicle->index, jolt + jolt);
        }
        vehicle->joltPosition.x = corner.x;
        vehicle->joltPosition.y = corner.y;
        vehicle->joltPosition.z = corner.z;
        Vec3fSubtract(&vehicle->joltVelocity, &vehicle->joltPosition, &predicted);
        vehicle->joltVelocity.z -= 0.32666668f;
    }
    RespawnVehicle(vehicle);
}
