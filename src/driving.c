#include "driving.h"

#include <PR/gu.h>
#include <PR/os_libc.h>

#include "camera.h"
#include "game.h"
#include "input.h"
#include "racemenu.h"
#include "render.h"
#include "system.h"
#include "timing.h"
#include "weather.h"

/* The dead zone of the steering and of the stick that drives, and the share of the stick past
 * each. */
#define STEER_DEAD_ZONE 0.1f
#define STEER_RANGE 0.9f
#define DRIVE_DEAD_ZONE 0.25f
#define DRIVE_RANGE 0.75f

/* The trackside camera of a replay stands at the nearest of the points of the track no more than
 * 10 below the vehicle, within 150 of it; the views stay this many frames before the next choice
 * and after a player chooses one. */
#define CAMERA_POINT_MAX_DROP 10.0f
#define CAMERA_POINT_FAR 16777216.0f
#define CAMERA_POINT_RANGE_SQUARED 22500.0f
#define REPLAY_VIEW_FRAMES 60
#define REPLAY_CHOSEN_VIEW_FRAMES 450

/* The jolts of the weight on the spring of a vehicle: the gravity on it each frame, and the least
 * that jolts the vehicle. */
#define JOLT_GRAVITY 0.326666683f
#define JOLT_THRESHOLD 0.025f
#define JOLT_MAX_ONCE 2.5f
#define JOLT_MAX 5.0f

/* The drive after a season: how far ahead the route is read, the speed until the vehicle nears
 * the end, and where it brakes and stops. */
#define VICTORY_LOOK_AHEAD 15.0f
#define VICTORY_SPEED 27.0f
#define VICTORY_SLOW_DISTANCE 114.5f
#define VICTORY_END_DISTANCE 168.5f
#define VICTORY_BRAKE_DISTANCE 167.5f
#define VICTORY_STOP_DISTANCE 168.4f
#define VICTORY_STEER_SCALE 3.0f
#define VICTORY_CENTERING 0.2f

/* How far in from each side the lanes of a search are. */
#define TRACE_LANE_SHARE 0.2f

/* How far a computer looks ahead: 20, 3 more for each unit of speed, and at most 80. */
#define AI_LOOK_BASE 20.0f
#define AI_LOOK_PER_SPEED 3.0f
#define AI_LOOK_MAX 80.0f
#define AI_TARGET_FOLLOW 0.5f
#define AI_TARGET_PULL 0.4f
#define AI_LOOK_TIME 0.4f

/* The speeds over which a computer corrects its line and limits its steering. */
#define AI_CORRECT_SPEED 10.0f
#define AI_SIGN_DEAD_ZONE 0.1f

#if TGR_REGION == TGR_REGION_EU
/* The European release scales the velocity a computer keeps in the championship by this. */
#define AI_RETENTION_SCALE 0.9988f
#endif

/* A computer keeps at least 3 from the side of a track wider than 10, and to the middle 60% of a
 * narrower one. */
#define AI_WIDE_HALF_WIDTH 5.0f
#define AI_SIDE_MARGIN 3.0f
#define AI_NARROW_SHARE 0.4f

/* The frames a stuck computer drives the other way, the frames it tries first, and the speed
 * under which it is stuck. */
#define AI_UNSTICK_FRAMES 60
#define AI_STUCK_CHECK_FRAMES 31
#define AI_STUCK_SPEED 1.0f
#define AI_REVERSE_FLIP_FRAMES 151
#define AI_REVERSE_RESET_FRAMES 271
#define AI_REVERSE_RESET_TO 30

/* A computer moves aside for a vehicle up to 90 ahead and within 3 to the side, by up to 5. */
#define AI_PASS_RANGE 90.0f
#define AI_PASS_SIDE 3.0f
#define AI_PASS_PUSH 10.0f
#define AI_PASS_PUSH_MAX 5.0f

/* The speeds of a computer's vehicle: it keeps to the route over 3, losing up to 40% of its
 * sideways speed. */
#define AI_HOLD_SPEED 3.0f
#define AI_HOLD_SCALE 0.015625f
#define AI_HOLD_MAX 0.4f

/* How much of its velocity a computer's vehicle keeps each frame in the arcade, in snow and
 * otherwise. */
#define AI_ARCADE_SNOW_RETENTION 0.99f
#define AI_ARCADE_RETENTION 0.999f

/* The angular speed above which a computer's vehicle on the ground is slowed to 1. */
#define AI_MAX_SPIN 1.0f

void ClearPerFrameVariables(void) {
    s32 i;
    u8 *byte;
    u8 *end;

    for (i = 0; g_aPerFrameVariables[i].start != NULL; i++) {
        byte = g_aPerFrameVariables[i].start;
        end = byte + g_aPerFrameVariables[i].size;
        while (byte < end) {
            *byte++ = 0;
        }
    }
}

void AddVehicleJolt(s32 index, f32 amount) {
    if (JOLT_MAX_ONCE < amount) {
        amount = JOLT_MAX_ONCE;
    }
    /* The index of a computer's vehicle may pass the end of the array. */
    g_afVehicleJolts[index] += amount;
    if (JOLT_MAX < g_afVehicleJolts[index]) {
        g_afVehicleJolts[index] = JOLT_MAX;
    }
}

/* The Japanese and European bodies of its callers still come from the assembly. */
#if TGR_REGION == TGR_REGION_US
/* Whether a viewport follows the vehicle. */
static s32 IsVehicleViewed(Vehicle *vehicle) {
    return vehicle->index == g_aViewportRects[0].vehicleIndex ||
           (g_dwNumViewports >= 2 && vehicle->index == g_aViewportRects[1].vehicleIndex);
}
#endif

#if TGR_REGION == TGR_REGION_US
/* Stand the trackside camera at the nearest point of the track, or switch away from it when there
 * is none. The USA release compiles the routine into its caller. */
static void ChooseReplayCamera(Vehicle *vehicle);
#endif

/* A point is always chosen when the range test passes, which the compiler cannot prove. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
void ChooseReplayCamera(Vehicle *vehicle) {
    CameraFrame *trackside = &vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE];
    Vec3f *point = g_LoadedTrack.cameraPoints;
    Vec3f chosen;
    f32 best = CAMERA_POINT_FAR;
    f32 dx, dy, dz, distance;
    s32 i;

    for (i = 0; i < g_LoadedTrack.cameraPointCount; i++, point++) {
        dx = vehicle->frame.position.x - point->x;
        dy = vehicle->frame.position.y - point->y;
        dz = vehicle->frame.position.z - point->z;
        if (!(CAMERA_POINT_MAX_DROP < dz)) {
            distance = dx * dx + dy * dy + dz * dz;
            if (distance < best) {
                best = distance;
                chosen.x = point->x;
                chosen.y = point->y;
                chosen.z = point->z;
            }
        }
    }
    if (best < CAMERA_POINT_RANGE_SQUARED) {
        if (vehicle->activeCamera != trackside || trackside->position.x != chosen.x ||
            trackside->position.y != chosen.y || trackside->position.z != chosen.z) {
            vehicle->cameraChanged = 1;
        }
        trackside->position.x = chosen.x;
        trackside->position.y = chosen.y;
        trackside->position.z = chosen.z;
        vehicle->activeCamera = trackside;
    } else if (vehicle->activeCamera == trackside) {
        vehicle->cameraChanged = 1;
        if (g_bReplayCockpitNext || g_dwReplayCameraHoldTimer) {
            vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_COCKPIT];
        } else {
            vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_CHASE];
        }
        g_dwReplayCameraSwitchTimer = REPLAY_VIEW_FRAMES;
        g_bReplayCockpitNext = !g_bReplayCockpitNext;
    }
}
#pragma GCC diagnostic pop

/* The Japanese and European bodies still come from the assembly. */
#if TGR_REGION == TGR_REGION_US
/* Choose the view a replay, a race of the computer, or a finished racer shows, and take the view a
 * player asks for. */
static void ChooseVehicleView(Vehicle *vehicle) {
    s32 choose = FALSE;
    s32 hold = FALSE;

    if ((vehicle->racer->status & RACER_FINISHED) && IsVehicleViewed(vehicle)) {
        if (g_dwReplayCameraSwitchTimer == 0) {
            choose = TRUE;
        }
    } else if ((g_dwGameMode == GAME_MODE_DEMO || g_dwGameMode == GAME_MODE_AFTER_SEASON ||
                gInstantReplayActive) &&
               IsVehicleViewed(vehicle)) {
        if (g_dwReplayCameraSwitchTimer != 0) {
            g_dwReplayCameraSwitchTimer--;
            hold = TRUE;
        } else {
            choose = TRUE;
        }
    } else if (IsVehicleViewed(vehicle)) {
        g_dwReplayCameraSwitchTimer = 0;
    }
    if (choose) {
        ChooseReplayCamera(vehicle);
        hold = TRUE;
    }
    if (hold && (vehicle->input->buttons & INPUT_VIEWS)) {
        g_dwReplayCameraSwitchTimer = REPLAY_CHOSEN_VIEW_FRAMES;
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
}

/* Read the steering, the stick and the buttons into the controls of a vehicle. */
static void ReadVehicleControls(Vehicle *vehicle) {
    f32 steering = vehicle->input->steering;
    f32 stickY;
    u32 buttons;

    if (STEER_DEAD_ZONE < steering) {
        vehicle->steering = (steering - STEER_DEAD_ZONE) / STEER_RANGE;
        vehicle->steering = vehicle->steering * -vehicle->steering;
    } else if (steering < -STEER_DEAD_ZONE) {
        vehicle->steering = (steering + STEER_DEAD_ZONE) / STEER_RANGE;
        vehicle->steering = vehicle->steering * vehicle->steering;
    } else {
        vehicle->steering = 0.0f;
    }

    ChooseVehicleView(vehicle);

    stickY = vehicle->input->stickY;
    if (DRIVE_DEAD_ZONE < stickY) {
        vehicle->accelerator = (stickY - DRIVE_DEAD_ZONE) / DRIVE_RANGE;
        vehicle->accelerator = vehicle->accelerator * vehicle->accelerator;
    } else if (stickY < -DRIVE_DEAD_ZONE) {
        vehicle->accelerator = (stickY + DRIVE_DEAD_ZONE) / DRIVE_RANGE;
        vehicle->accelerator = vehicle->accelerator * -vehicle->accelerator;
    } else {
        vehicle->accelerator = 0.0f;
    }
    if (vehicle->flyMode) {
        vehicle->rollInput = 0.0f;
    }

    buttons = vehicle->input->buttons;
    if (buttons & INPUT_Z) {
        if (!vehicle->flyMode) {
            vehicle->accelerator = 1.0f;
        }
    } else if (vehicle->flyMode) {
        /* Without Z, the stick pitches a flying vehicle. */
        vehicle->pitchInput = vehicle->accelerator;
        vehicle->accelerator = 0.0f;
    }
    if (vehicle->input->buttons & INPUT_UP) {
        vehicle->accelerator = 1.0f;
    }
    if (vehicle->input->buttons & INPUT_DOWN) {
        vehicle->accelerator = -1.0f;
    }
    if (vehicle->input->buttons & INPUT_RIGHT) {
        if (vehicle->flyMode) {
            vehicle->rollInput = 1.0f;
        } else {
            vehicle->steering = -1.0f;
        }
    }
    if (vehicle->input->buttons & INPUT_LEFT) {
        if (vehicle->flyMode) {
            vehicle->rollInput = -1.0f;
        } else {
            vehicle->steering = 1.0f;
        }
    }
}

/* Drive the vehicle of the race after a season along the middle of the route to a stop. */
static void DriveAfterSeason(Vehicle *vehicle) {
    Vec3f ahead;
    Vec3f velocity;

    Vec3fMulAddInPlace(&vehicle->frame.position, &vehicle->frame.forward, VICTORY_LOOK_AHEAD);
    ComputeTrackPosition(vehicle);
    ahead.x = vehicle->trackForward.x;
    ahead.y = vehicle->trackForward.y;
    ahead.z = vehicle->trackForward.z;
    Vec3fMulAddInPlace(&vehicle->frame.position, &vehicle->frame.forward, -VICTORY_LOOK_AHEAD);
    ComputeTrackPosition(vehicle);

    vehicle->input->buttons &= ~(INPUT_VIEWS | INPUT_RECORDED_ACTIONS);
    if (vehicle->distance < VICTORY_SLOW_DISTANCE) {
        Vec3fScale(&velocity, &vehicle->trackForward, VICTORY_SPEED);
    } else {
        Vec3fScale(
            &velocity, &vehicle->trackForward, (VICTORY_END_DISTANCE - vehicle->distance) * 0.5f);
        if (VICTORY_BRAKE_DISTANCE < vehicle->distance) {
            vehicle->input->buttons |= INPUT_BRAKE;
            if (VICTORY_STOP_DISTANCE < vehicle->distance) {
                velocity.x = velocity.y = velocity.z = 0.0f;
            }
        } else {
            vehicle->input->buttons |= INPUT_EBRAKE;
        }
    }
    vehicle->steerAngle = Vec3fDot(&ahead, &vehicle->frame.side) * VICTORY_STEER_SCALE;
    velocity.x -= vehicle->trackOffset.x * VICTORY_CENTERING;
    velocity.y -= vehicle->trackOffset.y * VICTORY_CENTERING;
    SetVehicleVelocity(vehicle, velocity.x, velocity.y, velocity.z);
}

/* Swing the weight on the spring at the front left corner of a vehicle, and jolt the vehicle when
 * it swings hard. */
static void SwingJoltWeight(Vehicle *vehicle) {
    Vec3f corner;
    Vec3f predicted;
    Vec3f previousOffset;
    f32 length;
    f32 jolt;

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
    /* The root of a distance, not of its square. */
    jolt = sqrtf(Vec3fDistance(&vehicle->joltOffset, &previousOffset)) /
           (Vec3fLength(&vehicle->joltVelocity) + 1.0f);
    if (JOLT_THRESHOLD < jolt) {
        AddVehicleJolt(vehicle->index, jolt + jolt);
    }
    vehicle->joltPosition.x = corner.x;
    vehicle->joltPosition.y = corner.y;
    vehicle->joltPosition.z = corner.z;
    /* The new position less the predicted one, not the reverse. */
    Vec3fSubtract(&vehicle->joltVelocity, &vehicle->joltPosition, &predicted);
    vehicle->joltVelocity.z -= JOLT_GRAVITY;
}

void ProcessVehicleInputAndPhysics(Vehicle *vehicle) {
    if (g_dwTrackMirrored) {
        vehicle->input->steering = -vehicle->input->steering;
    }
    ReadVehicleControls(vehicle);
    if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        DriveAfterSeason(vehicle);
    }
    UpdateVehiclePhysicsFrame(vehicle);
    if (!vehicle->flyMode) {
        SwingJoltWeight(vehicle);
    }
    RespawnVehicle(vehicle);
}
#endif

void UpdatePhysicsStep(Vehicle *vehicle) {
    ProcessVehicleInputAndPhysics(vehicle);
}

/* The flat crossing test is handed the positions themselves and reads their x and y. */
s32 TraceTrackSplineRecursive(Vehicle *vehicle, s32 depth, s32 pointIndex, TrackSection *section) {
    TrackPoint *point;
    TrackSection *child;
    Vec2f middle;
    s32 found = FALSE;
    s32 last;
    s32 i;

    if (section == NULL) {
        section = g_LoadedTrack.rootSection;
        pointIndex = 0;
    }
    if (depth == 0) {
        g_dwBestTraceDepth = 0;
        g_dwTraceUnusedB = 0;
        g_dwTraceUnusedA = 0;
        g_bTraceBlocked = FALSE;
    } else {
        if (depth > TRACE_DEPTH || (section->flags & TRACK_SECTION_ALTERNATE)) {
            return FALSE;
        }
        last = depth - 1;
        point = &section->points[pointIndex];
        Vec3fLerp(&g_avTraceRightLane[last], &point->left, &point->right, TRACE_LANE_SHARE);
        g_avTraceCenterLine[last].x = point->position.x;
        g_avTraceCenterLine[last].y = point->position.y;
        g_avTraceCenterLine[last].z = point->position.z;
        Vec3fLerp(&g_avTraceLeftLane[last], &point->right, &point->left, TRACE_LANE_SHARE);
        Vec3fMidpoint(
            &g_avTraceLeftHalf[last], &g_avTraceLeftLane[last], &g_avTraceCenterLine[last]);
        Vec3fMidpoint(
            &g_avTraceRightHalf[last], &g_avTraceRightLane[last], &g_avTraceCenterLine[last]);
        middle.x = (point->right.x + point->left.x) * 0.5f;
        middle.y = (point->right.y + point->left.y) * 0.5f;
        for (i = 1; i < last; i++) {
            if (!TestLineSegmentIntersection((Vec2f *)&vehicle->frame.position,
                                             &middle,
                                             (Vec2f *)&g_avTraceLeftLane[i],
                                             (Vec2f *)&g_avTraceRightLane[i])) {
                g_bTraceBlocked = TRUE;
                goto next;
            }
        }
        if (g_dwBestTraceDepth < depth) {
            /* Store the line as the deepest so far, and nudge the computer away from the side the
             * line passes close to. */
            g_dwBestTraceDepth = depth;
            g_pBestTraceSection = section;
            g_dwBestTracePoint = pointIndex;
            if (depth >= 3) {
                if (TestLineSegmentIntersection((Vec2f *)&vehicle->frame.position,
                                                (Vec2f *)&g_avTraceCenterLine[2],
                                                (Vec2f *)&g_avTraceLeftHalf[1],
                                                (Vec2f *)&g_avTraceLeftLane[1])) {
                    g_bAiNudgeLeft = FALSE;
                    g_bAiNudgeRight = TRUE;
                } else if (TestLineSegmentIntersection((Vec2f *)&vehicle->frame.position,
                                                       (Vec2f *)&g_avTraceCenterLine[2],
                                                       (Vec2f *)&g_avTraceRightHalf[1],
                                                       (Vec2f *)&g_avTraceRightLane[1])) {
                    g_bAiNudgeLeft = TRUE;
                    g_bAiNudgeRight = FALSE;
                } else {
                    g_bAiNudgeLeft = FALSE;
                    g_bAiNudgeRight = FALSE;
                }
            } else {
                g_bAiNudgeLeft = FALSE;
                g_bAiNudgeRight = FALSE;
            }
            g_vTraceHalfwayPoint.x = g_avTraceCenterLine[depth >> 1].x;
            g_vTraceHalfwayPoint.y = g_avTraceCenterLine[depth >> 1].y;
            g_vTraceHalfwayPoint.z = g_avTraceCenterLine[depth >> 1].z;
            g_vTraceFarPoint.x = g_avTraceCenterLine[depth - 1].x;
            g_vTraceFarPoint.y = g_avTraceCenterLine[depth - 1].y;
            g_vTraceFarPoint.z = g_avTraceCenterLine[depth - 1].z;
            memcpy(g_avBestLeftLane, g_avTraceLeftLane, depth * sizeof(Vec3f));
            memcpy(g_avBestCenterLine, g_avTraceCenterLine, depth * sizeof(Vec3f));
            memcpy(g_avBestRightLane, g_avTraceRightLane, depth * sizeof(Vec3f));
            g_dwBestTraceLength = depth;
            found = TRUE;
        }
    }

next:
    pointIndex++;
    if (pointIndex == section->pointCount) {
        child = section->child;
        if (child == NULL) {
            child = g_LoadedTrack.rootSection;
        }
        for (; child != NULL; child = child->next) {
            found |= TraceTrackSplineRecursive(vehicle, depth + 1, 0, child);
        }
    } else {
        found |= TraceTrackSplineRecursive(vehicle, depth + 1, pointIndex, section);
    }
    return found;
}

void ComputeTrackPosition(Vehicle *vehicle) {
    f32 span;
    Vec3f fromTangent;
    Vec3f toTangent;
    TrackSection *fromSection;
    TrackSection *toSection;
    s32 from;
    s32 to;
    Vec3f fromMiddle;
    Vec3f toMiddle;
    Vec3f flat;
    Vec3f fromOffset;
    Vec3f toOffset;
    f32 fromAlong;
    f32 toAlong;
    f32 length;
    f32 t;
    f32 t2;
    f32 t3;

    /* The tangents first hold the direction back along the route and the offset of the vehicle,
     * and the point is read again after each call. */
    span = vehicle->section->points[vehicle->pointIndex].distance -
           vehicle->section->points[vehicle->pointIndex + 1].distance;
    Vec3fSubtract(&fromTangent,
                  &vehicle->section->points[vehicle->pointIndex].left,
                  &vehicle->section->points[vehicle->pointIndex].position);
    Vec3fSubtract(&toTangent,
                  &vehicle->section->points[vehicle->pointIndex + 1].left,
                  &vehicle->section->points[vehicle->pointIndex + 1].position);
    Vec3fMidpoint(&toTangent, &fromTangent, &toTangent);
    /* The left of the route turned a quarter: back along it. */
    fromTangent.x = -toTangent.y;
    fromTangent.y = toTangent.x;
    fromTangent.z = 0.0f;
    Vec3fMidpoint(&toTangent,
                  &vehicle->section->points[vehicle->pointIndex].position,
                  &vehicle->section->points[vehicle->pointIndex + 1].position);
    Vec3fSubtract(&toTangent, &vehicle->frame.position, &toTangent);

    if (Vec3fDot(&fromTangent, &toTangent) < 0.0f) {
        fromSection = vehicle->section;
        from = vehicle->pointIndex;
        if (from + 1 == fromSection->pointCount) {
            toSection = fromSection->child;
            while (toSection->flags & TRACK_SECTION_ALTERNATE) {
                toSection = toSection->next;
            }
            to = 0;
        } else {
            toSection = fromSection;
            to = from + 1;
        }
    } else {
        /* The point before the first of a section is read from before its array. */
        fromSection = vehicle->section;
        from = vehicle->pointIndex - 1;
        toSection = vehicle->section;
        to = vehicle->pointIndex;
    }

    Vec3fDirection(
        &fromTangent, &fromSection->points[from].position, &fromSection->points[from + 1].position);
    Vec3fDirection(
        &toTangent, &toSection->points[to].position, &toSection->points[to + 1].position);
    Vec3fScaleInPlace(&fromTangent, span * 0.5f);
    Vec3fScaleInPlace(&toTangent, span * 0.5f);
    Vec3fMidpoint(
        &fromMiddle, &fromSection->points[from].position, &fromSection->points[from + 1].position);
    Vec3fMidpoint(&toMiddle, &toSection->points[to].position, &toSection->points[to + 1].position);
    flat.x = vehicle->frame.position.x;
    flat.y = vehicle->frame.position.y;
    flat.z = 0.0f;
    Vec3fSubtract(&fromOffset, &flat, &fromMiddle);
    Vec3fSubtract(&toOffset, &flat, &toMiddle);
    fromAlong = Vec3fDot(&fromOffset, &fromTangent);
    toAlong = -Vec3fDot(&toOffset, &toTangent);
    length = Vec3fDistance(&fromMiddle, &toMiddle);
    t = length * fromAlong / (fromAlong + toAlong) / length;
    t2 = t * t;
    t3 = t2 * t;

    /* A Hermite curve between the middles of the two segments. */
    Vec3fScale(&vehicle->trackPoint, &fromMiddle, t3 + t3 - 3.0f * t2 + 1.0f);
    Vec3fMulAddInPlace(&vehicle->trackPoint, &toMiddle, -2.0f * t3 + 3.0f * t2);
    Vec3fMulAddInPlace(&vehicle->trackPoint, &fromTangent, t3 - (t2 + t2) + t);
    Vec3fMulAddInPlace(&vehicle->trackPoint, &toTangent, t3 - t2);
    Vec3fLerp(&vehicle->trackForward, &toTangent, &fromTangent, t);
    Vec3fNormalize(&vehicle->trackForward);
    Vec3fCross(&vehicle->trackSide, &vehicle->trackUp, &vehicle->trackForward);
    Vec3fNormalize(&vehicle->trackSide);
    Vec3fCross(&vehicle->trackUp, &vehicle->trackForward, &vehicle->trackSide);
    Vec3fSubtract(&vehicle->trackOffset, &vehicle->frame.position, &vehicle->trackPoint);
    vehicle->trackSideOffset = Vec3fDot(&vehicle->trackSide, &vehicle->trackOffset);
    if (vehicle->trackSideOffset < 0.0f) {
        vehicle->trackSideDistance = -vehicle->trackSideOffset;
    } else {
        vehicle->trackSideDistance = vehicle->trackSideOffset;
    }
}

/* 1 above the dead zone of the computer's tests, -1 below it, and 0 within. */
static s32 AiSign(f32 value) {
    if (AI_SIGN_DEAD_ZONE < value) {
        return 1;
    }
    if (value < -AI_SIGN_DEAD_ZONE) {
        return -1;
    }
    return 0;
}

/* The point of the route a computer looks ahead to, farther the faster it goes. */
static void FindAiLookPoint(Vehicle *vehicle, Vec3f *out) {
    TrackSection *section = vehicle->section;
    s32 index = vehicle->pointIndex;
    TrackPoint *point;
    f32 look = Vec3fLength(&vehicle->placedVelocity) * AI_LOOK_PER_SPEED + AI_LOOK_BASE;
    f32 step;

    /* The original stores the point of the vehicle first and overwrites it after the walk. */
    out->x = vehicle->section->points[vehicle->pointIndex].position.x;
    out->y = vehicle->section->points[vehicle->pointIndex].position.y;
    out->z = vehicle->section->points[vehicle->pointIndex].position.z;
    if (AI_LOOK_MAX < look) {
        look = AI_LOOK_MAX;
    }
    point = &section->points[index];
    for (;;) {
        step = point[0].distance - point[1].distance;
        index++;
        point++;
        look -= step;
        if (index == section->pointCount) {
            section = section->child;
            index = 0;
            while (section->flags & TRACK_SECTION_ALTERNATE) {
                section = section->next;
            }
            point = section->points;
        }
        if (look < 0.0f) {
            break;
        }
    }
    out->x = point->position.x;
    out->y = point->position.y;
    out->z = point->position.z;
}

/* Set the axes of the route at the point of a computer's vehicle and its offset from the middle a
 * moment ahead. Return the offset to the left. */
static f32 MeasureAiRoutePosition(Vehicle *vehicle) {
    /* The point is read again from the vehicle at each use. */
    Vec3fDirection(&vehicle->trackForward,
                   &vehicle->section->points[vehicle->pointIndex].position,
                   &vehicle->section->points[vehicle->pointIndex + 1].position);
    Vec3fSubtract(&vehicle->trackSide,
                  &vehicle->section->points[vehicle->pointIndex].left,
                  &vehicle->section->points[vehicle->pointIndex].right);
    Vec3fCross(&vehicle->trackUp, &vehicle->trackForward, &vehicle->trackSide);
    Vec3fNormalize(&vehicle->trackUp);
    Vec3fCross(&vehicle->trackSide, &vehicle->trackUp, &vehicle->trackForward);
    Vec3fNormalize(&vehicle->trackSide);
    Vec3fSubtract(&vehicle->trackOffset,
                  &vehicle->frame.position,
                  &vehicle->section->points[vehicle->pointIndex].position);
    Vec3fMulAddInPlace(&vehicle->trackOffset, &vehicle->placedVelocity, AI_LOOK_TIME);
    return Vec3fDot(&vehicle->trackSide, &vehicle->trackOffset);
}

/* Correct a computer that strays from the route or skids across it. Return how hard it corrects. */
static f32 CorrectAiLine(
    Vehicle *vehicle, f32 sideOffset, f32 sideDistance, f32 heading, f32 halfWidth, Vec3f *across) {
    f32 limit;
    f32 steer = 0.0f;
    s32 velocitySign;
    s32 wheelSign;
    s32 forwardSign;

    if (AI_WIDE_HALF_WIDTH < halfWidth) {
        limit = halfWidth - AI_SIDE_MARGIN;
    } else {
        limit = halfWidth * AI_NARROW_SHARE;
    }

    if (limit < sideDistance) {
        /* These four compare in double precision, and each computes the share past the limit. */
        if (0.0f < sideOffset) {
            if (heading > -0.05) {
                g_bAiNudgeLeft = FALSE;
                g_bAiNudgeRight = TRUE;
                steer = heading * -0.2f * ((sideDistance - limit) / sideDistance) + 0.03f;
            } else if (heading < -0.15) {
                g_bAiNudgeLeft = TRUE;
                g_bAiNudgeRight = FALSE;
                steer = heading * -0.3f * ((sideDistance - limit) / sideDistance) + 0.1f;
            }
        } else {
            if (heading < 0.05) {
                g_bAiNudgeLeft = TRUE;
                g_bAiNudgeRight = FALSE;
                steer = heading * 0.2f * ((sideDistance - limit) / sideDistance) + 0.03f;
            } else if (heading > 0.15) {
                g_bAiNudgeLeft = FALSE;
                g_bAiNudgeRight = TRUE;
                steer = heading * 0.3f * ((sideDistance - limit) / sideDistance) + 0.1f;
            }
        }
    } else if (AI_CORRECT_SPEED < Vec3fLength(&vehicle->placedVelocity)) {
        velocitySign = AiSign(Vec3fDot(&vehicle->placedVelocity, &vehicle->trackSide));
        wheelSign = AiSign(Vec3fDot(&vehicle->wheelFrames[0].forward, across));
        forwardSign = AiSign(Vec3fDot(&vehicle->frame.forward, across));
        if (velocitySign != 0 || forwardSign != 0) {
            if (velocitySign != 0 && velocitySign + forwardSign == 0) {
                if (velocitySign == 1 && wheelSign == 1) {
                    g_bAiNudgeRight = FALSE;
                    g_bAiNudgeLeft = TRUE;
                } else if (velocitySign == -1 && wheelSign == -1) {
                    g_bAiNudgeRight = TRUE;
                    g_bAiNudgeLeft = FALSE;
                }
                steer = 0.1f;
            } else if (velocitySign != 0 && velocitySign == forwardSign) {
                if (velocitySign == 1 && wheelSign == 1) {
                    g_bAiNudgeRight = TRUE;
                    g_bAiNudgeLeft = FALSE;
                } else if (velocitySign == -1 && wheelSign == -1) {
                    g_bAiNudgeRight = FALSE;
                    g_bAiNudgeLeft = TRUE;
                }
                steer = 0.4f;
            } else if (wheelSign != 0 || forwardSign != 0) {
                if (wheelSign == 0 && velocitySign == 0) {
                    vehicle->input->buttons &= ~INPUT_ACCELERATE;
                } else if (wheelSign == forwardSign) {
                    if (wheelSign == 1) {
                        g_bAiNudgeRight = TRUE;
                        g_bAiNudgeLeft = FALSE;
                    } else if (wheelSign == -1) {
                        g_bAiNudgeRight = FALSE;
                        g_bAiNudgeLeft = TRUE;
                    }
                    steer = 0.5f;
                    vehicle->input->buttons &= ~INPUT_ACCELERATE;
                } else if (wheelSign == 0 || wheelSign + velocitySign != 0) {
                    vehicle->input->buttons &= ~INPUT_ACCELERATE;
                }
            }
        }
    }
    return steer;
}

/* Read how hard the line ahead turns against the velocity of a computer's vehicle, braking for a
 * hard turn. Return how much more it should steer. */
static f32 ReadAiLineAhead(Vehicle *vehicle) {
    Vec3f toLeft;
    Vec3f ahead;
    Vec3f normal;
    f32 climb;
    f32 lateral;
    f32 excess;
    s32 i = 0;

    while (i < g_dwBestTraceDepth) {
        Vec3fSubtract(&toLeft, &g_avBestLeftLane[i], &g_avBestCenterLine[i]);
        /* The point after the last is the first of the right lane, which follows in memory. */
        Vec3fSubtract(&ahead, &g_avBestCenterLine[i + 1], &g_avBestCenterLine[i]);
        Vec3fCross(&normal, &ahead, &toLeft);
        Vec3fNormalize(&normal);
        climb = Vec3fDot(&vehicle->placedVelocity, &normal);
        lateral = Vec3fDot(&vehicle->placedVelocity, &toLeft) * 0.03f;
        i++;
        if (climb < lateral) {
            excess = lateral;
        } else if (lateral < -climb) {
            excess = -lateral;
        } else {
            excess = climb;
        }
        if ((f32)i * 6.0f < excess) {
            vehicle->input->buttons &= ~INPUT_ACCELERATE;
            vehicle->input->buttons |= INPUT_BRAKE;
            return 2.0f;
        }
        if ((f32)i * 4.5f < excess) {
            vehicle->input->buttons &= ~INPUT_ACCELERATE;
            return 2.0f;
        }
        if ((f32)i * 3.0f < excess) {
            return 1.3f;
        }
    }
    return 1.0f;
}

/* Shape the steering of a computer, easing it to the side it is nudged to. */
static f32 ShapeAiSteering(f32 side, f32 steer) {
    if (steer < 0.01f) {
        steer = 0.01f;
    }
    if (side < 0.0f) {
        if (g_bAiNudgeLeft) {
            if (side < -(0.2f / steer)) {
                side -= steer * side;
            } else {
                side += 0.2f;
            }
        } else if (g_bAiNudgeRight) {
            if (side < -(0.2f / steer)) {
                side += steer * side;
            } else {
                side -= 0.2f;
            }
        }
        side = 1.0f + side;
        return side * side * side * side - 1.0f;
    }
    if (g_bAiNudgeLeft) {
        if (0.2f / steer < side) {
            side += steer * side;
        } else {
            side += 0.2f;
        }
    } else if (g_bAiNudgeRight) {
        if (0.2f / steer < side) {
            side -= steer * side;
        } else {
            side -= 0.2f;
        }
    }
    side = 1.0f - side;
    return 1.0f - side * side * side * side;
}

/* Drive a computer forward with the steering, or back out of where it is stuck, and brake when it
 * rolls the wrong way. */
static void ApplyAiDrive(Vehicle *vehicle, f32 side, f32 forwardSpeed) {
    f32 speed = Vec3fLength(&vehicle->placedVelocity);
    s32 reverse;

    if (0.0f <= vehicle->accelerator) {
        if (vehicle->forcedReverseFrames != 0) {
            vehicle->forcedReverseFrames--;
            reverse = TRUE;
        } else if (forwardSpeed < -1.0f) {
            vehicle->input->buttons |= INPUT_BRAKE;
            return;
        } else {
            reverse = FALSE;
        }
    } else if (vehicle->forcedForwardFrames != 0) {
        vehicle->forcedForwardFrames--;
        reverse = FALSE;
    } else if (1.0f < forwardSpeed) {
        vehicle->input->buttons |= INPUT_BRAKE;
        return;
    } else {
        reverse = TRUE;
    }

    if (!reverse) {
        vehicle->input->steering = -side;
        vehicle->forwardFrames++;
        if (vehicle->forwardFrames >= AI_STUCK_CHECK_FRAMES && speed < AI_STUCK_SPEED) {
            vehicle->forcedReverseFrames = AI_UNSTICK_FRAMES;
            vehicle->reverseFrames = 0;
            vehicle->forwardFrames = 0;
        }
        return;
    }
    vehicle->input->buttons |= INPUT_ACCELERATE;
    if (side < 0.0f) {
        vehicle->input->steering = -1.0f;
    } else {
        vehicle->input->steering = 1.0f;
    }
    vehicle->input->buttons |= INPUT_REVERSE;
    if (vehicle->reverseFrames >= AI_REVERSE_FLIP_FRAMES) {
        if (vehicle->reverseFrames >= AI_REVERSE_RESET_FRAMES) {
            vehicle->reverseFrames = AI_REVERSE_RESET_TO;
        } else {
            vehicle->input->steering = vehicle->input->steering * -1.0;
        }
    }
    vehicle->reverseFrames++;
    if (vehicle->reverseFrames >= AI_STUCK_CHECK_FRAMES && speed < AI_STUCK_SPEED) {
        vehicle->forcedForwardFrames = AI_UNSTICK_FRAMES;
        vehicle->reverseFrames = 0;
        vehicle->forwardFrames = 0;
    }
}

/* Move a computer aside for the nearest vehicle ahead of it. Return the offset to the left it
 * aims for. */
static f32 PassAiVehicles(Vehicle *vehicle, f32 sideOffset) {
    Vehicle *other;
    f32 best = AI_PASS_RANGE;
    f32 push = 0.0f;
    f32 lapLength;
    f32 gap;
    f32 sideGap;
    s32 found = -1;
    s32 i;

    for (i = 0; i < g_dwNumVehicles; i++) {
        other = g_aRacerSlots[i].vehicle;
        if (other == NULL || other == vehicle) {
            continue;
        }
        lapLength = g_LoadedTrack.rootSection->points[0].distance;
        gap = other->distance - vehicle->distance;
        while (lapLength < gap) {
            gap -= lapLength;
        }
        while (gap < -lapLength) {
            gap += lapLength;
        }
        if (0.0f < gap && gap < best) {
            best = gap;
            sideGap = other->trackSideOffset - sideOffset;
            if (sideGap < AI_PASS_SIDE) {
                push = (1.0f - gap * (1.0f / AI_PASS_RANGE)) * -AI_PASS_PUSH;
                if (push < -AI_PASS_PUSH_MAX) {
                    push = -AI_PASS_PUSH_MAX;
                }
                found = i;
            } else if (-AI_PASS_SIDE < sideGap) {
                /* Always true here: a gap of at least 3 is above -3. */
                push = (1.0f - gap * (1.0f / AI_PASS_RANGE)) * AI_PASS_PUSH;
                if (AI_PASS_PUSH_MAX < push) {
                    push = AI_PASS_PUSH_MAX;
                }
                found = i;
            }
        }
    }
    if (found != -1) {
        sideOffset += push;
        vehicle->trackSideDistance = sideOffset < 0.0f ? -sideOffset : sideOffset;
    }
    return sideOffset;
}

/* Hold a computer's vehicle to the route by taking away its speed across it, and slow it by the
 * mode. */
static void HoldAiToRoute(Vehicle *vehicle, f32 sideOffset, f32 halfWidth, f32 forwardSpeed) {
    Vec3f *velocity = &vehicle->body.state.velocity;
    f32 hold;
    f32 along;
    f32 across;
    f32 upward;
    s16 weather;

    if (vehicle->trackSideDistance < halfWidth + 1.0f) {
        hold = (forwardSpeed - AI_HOLD_SPEED) * AI_HOLD_SCALE;
        if (hold < 0.0f) {
            hold *= -0.25f;
        }
        if (AI_HOLD_MAX < hold) {
            hold = AI_HOLD_MAX;
        }
        along = Vec3fDot(velocity, &vehicle->trackForward);
        /* The original computes the speed across the route and overwrites it unread. */
        across = Vec3fDot(velocity, &vehicle->trackSide);
        upward = Vec3fDot(velocity, &vehicle->trackUp);
        across = -(sideOffset * hold);
        Vec3fScale(velocity, &vehicle->trackForward, along);
        Vec3fMulAddInPlace(velocity, &vehicle->trackSide, across);
        Vec3fMulAddInPlace(velocity, &vehicle->trackUp, upward);
    }
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
#if TGR_REGION == TGR_REGION_EU
        Vec3fScaleInPlace(
            velocity,
            g_aafAiVelocityRetention[g_pActiveProfile->season][g_pActiveProfile->round]
                                    [vehicle->racer->straggler] *
                AI_RETENTION_SCALE);
#else
        Vec3fScaleInPlace(
            velocity,
            g_aafAiVelocityRetention[g_pActiveProfile->season][g_pActiveProfile->round]
                                    [vehicle->racer->straggler]);
#endif
    } else if (g_dwGameMode == GAME_MODE_ARCADE) {
        weather = g_dwCurrentWeather - 1;
        if (weather >= ARCADE_WEATHERS || weather < 0) {
            weather = 0;
        }
        if (weather == WEATHER_SNOW - 1) {
            Vec3fScaleInPlace(velocity, AI_ARCADE_SNOW_RETENTION);
        } else {
            Vec3fScaleInPlace(velocity, AI_ARCADE_RETENTION);
        }
    }
}

/* Plan the frame of a computer's vehicle. */
static void PlanAiDrive(Vehicle *vehicle) {
    Vec3f target;
    Vec3f direction;
    Vec3f up;
    Vec3f across;
    Vec3f unread;
    Vec3f edge;
    TrackPoint *point;
    f32 forwardSpeed;
    f32 side;
    f32 sideOffset;
    f32 sideDistance;
    f32 heading;
    f32 halfWidth;
    f32 steer;
    f32 multiplier;

    if (vehicle->input->buttons & INPUT_VIEW_TRACKSIDE) {
        vehicle->cameraChanged = 1;
        PlaceTrackCamera(vehicle);
    }
    AddTimingMarker(0, 255, 255, 255, 255);
    FindAiLookPoint(vehicle, &target);
    if (TraceTrackSplineRecursive(vehicle, 0, vehicle->pointIndex, vehicle->section)) {
        Vec3fMidpoint(&g_vTraceFarPoint, &g_vTraceFarPoint, &g_vTraceHalfwayPoint);
        if (vehicle->aiTarget.x == 0.0f && vehicle->aiTarget.y == 0.0f &&
            vehicle->aiTarget.z == 0.0f) {
            vehicle->aiTarget.x = g_vTraceFarPoint.x;
            vehicle->aiTarget.y = g_vTraceFarPoint.y;
            vehicle->aiTarget.z = g_vTraceFarPoint.z;
        } else {
            Vec3fLerp(&vehicle->aiTarget,
                      &vehicle->aiTarget,
                      &g_vTraceFarPoint,
                      AI_TARGET_FOLLOW + g_fFrameDeltaTime);
        }
    }
    Vec3fLerp(&vehicle->aiTarget, &vehicle->aiTarget, &target, AI_TARGET_PULL);
    forwardSpeed = Vec3fDot(&vehicle->placedVelocity, &vehicle->frame.forward);
    Vec3fDirection(&direction, &vehicle->frame.position, &vehicle->aiTarget);
    up = g_vUpAxis;
    Vec3fCross(&across, &up, &direction);
    /* The original computes this product and never reads it. */
    Vec3fCross(&unread, &direction, &across);
    side = Vec3fDot(&vehicle->frame.side, &direction);
    vehicle->accelerator = Vec3fDot(&vehicle->frame.forward, &direction);
    vehicle->input->buttons |= INPUT_ACCELERATE;

    sideOffset = MeasureAiRoutePosition(vehicle);
    heading = Vec3fDot(&vehicle->frame.forward, &vehicle->trackSide);
    g_bAiNudgeLeft = FALSE;
    g_bAiNudgeRight = FALSE;
    sideDistance = sideOffset < 0.0f ? -sideOffset : sideOffset;
    point = &vehicle->section->points[vehicle->pointIndex];
    Vec3fSubtract(&edge, &point->left, &point->position);
    halfWidth = Vec3fDot(&vehicle->trackSide, &edge);
    steer = CorrectAiLine(vehicle, sideOffset, sideDistance, heading, halfWidth, &across);

    multiplier = ReadAiLineAhead(vehicle);
    if (AI_CORRECT_SPEED < Vec3fLength(&vehicle->placedVelocity)) {
        side *= multiplier;
        if (1.0f < side) {
            vehicle->input->buttons &= ~INPUT_ACCELERATE;
            side = 1.0f;
        } else if (side < -1.0f) {
            side = -1.0f;
            vehicle->input->buttons &= ~INPUT_ACCELERATE;
        }
    }
    side = ShapeAiSteering(side, steer);
    ApplyAiDrive(vehicle, side, forwardSpeed);

    ComputeTrackPosition(vehicle);
    sideOffset = PassAiVehicles(vehicle, sideOffset);
    if (AI_HOLD_SPEED < forwardSpeed) {
        HoldAiToRoute(vehicle, sideOffset, halfWidth, forwardSpeed);
    }
}

void UpdateAiVehicleBehavior(Vehicle *vehicle) {
    Vec3f *spin = &vehicle->body.state.angularVelocity;
    f32 length;

    if (!(vehicle->racer->status & RACER_HELD)) {
        PlanAiDrive(vehicle);
    }
    AddTimingMarker(0, 0, 130, 0, 255);
    length = Vec3fLength(spin);
    if (AI_MAX_SPIN < length && vehicle->wheelBodies[0].contactFrames) {
        Vec3fScaleInPlace(spin, 1.0f / length);
    }
    UpdateVehiclePhysicsFrame(vehicle);
    RespawnVehicle(vehicle);
}

void DriveAiVehicle(Vehicle *vehicle) {
    UpdateAiVehicleBehavior(vehicle);
}
