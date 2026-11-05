#include "common/driving.h"

#include "common/analyzer.h"
#include "common/camera.h"
#include "common/frametime.h"
#include "common/game.h"
#include "common/input.h"
#include "common/racerslotaccess.h"
#include "common/render.h"
#include "common/system.h"
#include "common/timing.h"
#include "common/trackmenuitem.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"
#include "common/vehiclephysics.h"
#include "common/vehiclereset.h"
#include "common/weather.h"

#if !TGR_BOSS_RALLY
#include <PR/gu.h>
#include <PR/os_libc.h>

#else
#include "common/screen.h"
#endif

#if TGR_BOSS_RALLY
#define AddTimingMarker(...) NullStub()
#endif

#if TGR_BOSS_RALLY
typedef long double AiReal;
#define AI_INTERMEDIATE(value) ((long double)(value))
#define AI_LESS(a, b) (!((a) >= (b)))
#define AI_GREATER(a, b) AI_LESS((b), (a))
#define AI_ZERO(value) (!((value) < 0.0f) && !((value) > 0.0f))
#define AI_ACTIVE_PROFILE VEHICLE_GET(&g_aVehicles[0], Profile)
#else
typedef f32 AiReal;
#define AI_INTERMEDIATE(value) (value)
#define AI_LESS(a, b) ((a) < (b))
#define AI_GREATER(a, b) ((a) > (b))
#define AI_ZERO(value) ((value) == 0.0f)
#define AI_ACTIVE_PROFILE (g_aVehicles[0].profile)
#endif /* TGR_BOSS_RALLY */

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
/* The European release scales the velocity a computer retains in the championship by this. */
#define AI_RETENTION_SCALE 0.9988f
#endif

/* A computer stays at least 3 from the side of a track wider than 10, and in the middle 60% of a
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

/* The speeds of a computer's vehicle above which it follows the route, losing up to 40% of its
 * sideways speed. */
#define AI_HOLD_SPEED 3.0f
#define AI_HOLD_SCALE 0.015625f
#define AI_HOLD_MAX 0.4f

/* How much of its velocity a computer's vehicle retains each frame in the arcade, in snow and
 * otherwise. */
#define AI_ARCADE_SNOW_RETENTION 0.99f
#define AI_ARCADE_RETENTION 0.999f

/* The angular speed above which a computer's vehicle on the ground is slowed to 1. */
#define AI_MAX_SPIN 1.0f

void UpdateAiVehicleBehavior(Vehicle *vehicle) {
    f32 length;
    AiReal rawLength;

    if (!(RACER_SLOT_GET(VEHICLE_GET(vehicle, Racer), Status) & RACER_HELD)) {
        Vec3f direction;
        AiReal side;
        s32 index;
        f32 forwardSpeed;
        AiReal look;
        TrackSection *section;
        f32 sideOffset;
        Vec3f target;
        f32 steer;
        AiReal limit;
        s32 level;
        AiReal lateral;
        f32 speed;
        s32 forwardSign;
        f32 signDot;
        Vec3f unread;
        Vec3f across;
        s32 found;
        Vec3f up;
        f32 best;
        f32 push;
        f32 lapLength;
        f32 heading;
        f32 sideDistance;
        Vec3f edge;
        f32 halfWidth;
        AiReal gap;
        AiReal sideGap;
        AiReal hold;
        s32 velocitySign;
#if !TGR_BOSS_RALLY
        int stackPaddingForLook[7];
#endif
        f32 multiplier;
        Vec3f ahead;
        Vec3f toLeft;
        Vec3f normal;
        f32 climb;
        Vehicle *other;
        f32 relativeVelocity[3];
        s16 weather;
        AiReal rawHalfWidth;
        AiReal highPush;
        AiReal updated;

        if (VEHICLE_GET(vehicle, Input)->buttons & INPUT_VIEW_TRACKSIDE) {
            VEHICLE_SET(vehicle, CameraChanged, 1);
            PlaceTrackCamera(vehicle);
        }
        AddTimingMarker(0, 255, 255, 255, 255);
        section = VEHICLE_GET(vehicle, Section);
        index = VEHICLE_GET(vehicle, PointIndex);
        look =
            Vec3fLength(&VEHICLE_GET(vehicle, PlacedVelocity)) * AI_LOOK_PER_SPEED + AI_LOOK_BASE;
        /* The original stores the point of the vehicle first and overwrites it after the walk. */
        {
            f32 *position =
                &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position.x;

            target.x = position[0];
            target.y = position[1];
            target.z = position[2];
        }
        if (AI_LOOK_MAX < look) {
            look = AI_LOOK_MAX;
        }
        for (;;) {
            look -= AI_INTERMEDIATE(section->points[index].distance) -
                    section->points[index + 1].distance;
            index++;
            if (index == section->pointCount) {
                section = section->child;
                while (section->flags & TRACK_SECTION_ALTERNATE) {
                    section = section->next;
                }
                index = 0;
            }
            if (AI_LESS(look, 0)) {
                break;
            }
        }
        target.x = section->points[index].position.x;
        target.y = section->points[index].position.y;
        target.z = section->points[index].position.z;
        if (TraceTrackSplineRecursive(
                vehicle, 0, VEHICLE_GET(vehicle, PointIndex), VEHICLE_GET(vehicle, Section))) {
            Vec3fMidpoint(&g_vTraceFarPoint, &g_vTraceFarPoint, &g_vTraceHalfwayPoint);
            if (!AI_ZERO(VEHICLE_GET(vehicle, AiTarget).x) ||
                !AI_ZERO(VEHICLE_GET(vehicle, AiTarget).y) ||
                !AI_ZERO(VEHICLE_GET(vehicle, AiTarget).z)) {
                Vec3fLerp(&VEHICLE_GET(vehicle, AiTarget),
                          &VEHICLE_GET(vehicle, AiTarget),
                          &g_vTraceFarPoint,
                          AI_TARGET_FOLLOW + AI_INTERMEDIATE(g_fFrameDeltaTime));
            } else {
                VEHICLE_GET(vehicle, AiTarget).x = g_vTraceFarPoint.x;
                VEHICLE_GET(vehicle, AiTarget).y = g_vTraceFarPoint.y;
                VEHICLE_GET(vehicle, AiTarget).z = g_vTraceFarPoint.z;
            }
        }
        Vec3fLerp(&VEHICLE_GET(vehicle, AiTarget),
                  &VEHICLE_GET(vehicle, AiTarget),
                  &target,
                  AI_TARGET_PULL);
        forwardSpeed =
            Vec3fDot(&VEHICLE_GET(vehicle, PlacedVelocity), &VEHICLE_GET(vehicle, Frame).forward);
        Vec3fDirection(
            &direction, &VEHICLE_GET(vehicle, Frame).position, &VEHICLE_GET(vehicle, AiTarget));
#if TGR_BOSS_RALLY
        up.x = 0.0f;
        up.y = 0.0f;
        up.z = 1.0f;
#else
        up = g_vUpAxis;
#endif
        Vec3fCross(&across, &up, &direction);
        /* The original computes this product and never reads it. */
        Vec3fCross(&unread, &direction, &across);
        side = (f32)Vec3fDot(&VEHICLE_GET(vehicle, Frame).side, &direction);
        VEHICLE_SET(
            vehicle, Accelerator, Vec3fDot(&VEHICLE_GET(vehicle, Frame).forward, &direction));
        VEHICLE_GET(vehicle, Input)->buttons |= INPUT_ACCELERATE;

        /* The point is read again from the vehicle at each use. */
        Vec3fDirection(
            &VEHICLE_GET(vehicle, TrackForward),
            &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position,
            &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex) + 1].position);
        Vec3fSubtract(
            &VEHICLE_GET(vehicle, TrackSide),
            &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].left,
            &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].right);
        Vec3fCross(&VEHICLE_GET(vehicle, TrackUp),
                   &VEHICLE_GET(vehicle, TrackForward),
                   &VEHICLE_GET(vehicle, TrackSide));
        Vec3fNormalize(&VEHICLE_GET(vehicle, TrackUp));
        Vec3fCross(&VEHICLE_GET(vehicle, TrackSide),
                   &VEHICLE_GET(vehicle, TrackUp),
                   &VEHICLE_GET(vehicle, TrackForward));
        Vec3fNormalize(&VEHICLE_GET(vehicle, TrackSide));
        Vec3fSubtract(
            &VEHICLE_GET(vehicle, TrackOffset),
            &VEHICLE_GET(vehicle, Frame).position,
            &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position);
        Vec3fMulAddInPlace(&VEHICLE_GET(vehicle, TrackOffset),
                           &VEHICLE_GET(vehicle, PlacedVelocity),
                           AI_LOOK_TIME);
        sideOffset = Vec3fDot(&VEHICLE_GET(vehicle, TrackSide), &VEHICLE_GET(vehicle, TrackOffset));
        steer = 0.0f;
        heading = Vec3fDot(&VEHICLE_GET(vehicle, Frame).forward, &VEHICLE_GET(vehicle, TrackSide));
        g_bAiNudgeLeft = FALSE;
        g_bAiNudgeRight = FALSE;
        sideDistance = AI_LESS(sideOffset, 0.0f) ? -sideOffset : sideOffset;
        Vec3fSubtract(
            &edge,
            &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].left,
            &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position);
        rawHalfWidth = Vec3fDot(&VEHICLE_GET(vehicle, TrackSide), &edge);
        halfWidth = rawHalfWidth;
        if (AI_WIDE_HALF_WIDTH < rawHalfWidth) {
            limit = AI_INTERMEDIATE(halfWidth) - AI_SIDE_MARGIN;
        } else {
            limit = AI_INTERMEDIATE(halfWidth) * AI_NARROW_SHARE;
        }

        if (limit < sideDistance) {
            /* These four compare in double precision, and each computes the share past the limit.
             */
            if (0.0f < sideOffset) {
                if (heading > -0.05) {
                    g_bAiNudgeLeft = FALSE;
                    g_bAiNudgeRight = TRUE;
                    steer = AI_INTERMEDIATE(heading) * -0.2f *
                                ((AI_INTERMEDIATE(sideDistance) - limit) / sideDistance) +
                            0.03f;
                } else if (AI_LESS(heading, -0.15)) {
                    g_bAiNudgeLeft = TRUE;
                    g_bAiNudgeRight = FALSE;
                    steer = AI_INTERMEDIATE(heading) * -0.3f *
                                ((AI_INTERMEDIATE(sideDistance) - limit) / sideDistance) +
                            0.1f;
                }
            } else {
                if (AI_LESS(heading, 0.05)) {
                    g_bAiNudgeLeft = TRUE;
                    g_bAiNudgeRight = FALSE;
                    steer = AI_INTERMEDIATE(heading) * 0.2f *
                                ((AI_INTERMEDIATE(sideDistance) - limit) / sideDistance) +
                            0.03f;
                } else if (heading > 0.15) {
                    g_bAiNudgeLeft = FALSE;
                    g_bAiNudgeRight = TRUE;
                    steer = AI_INTERMEDIATE(heading) * 0.3f *
                                ((AI_INTERMEDIATE(sideDistance) - limit) / sideDistance) +
                            0.1f;
                }
            }
        } else if (AI_CORRECT_SPEED < Vec3fLength(&VEHICLE_GET(vehicle, PlacedVelocity))) {
            signDot =
                Vec3fDot(&VEHICLE_GET(vehicle, PlacedVelocity), &VEHICLE_GET(vehicle, TrackSide));
            if (AI_SIGN_DEAD_ZONE < signDot) {
                velocitySign = 1;
            } else if (AI_LESS(signDot, -AI_SIGN_DEAD_ZONE)) {
                velocitySign = -1;
            } else {
                velocitySign = 0;
            }
            signDot = Vec3fDot(&VEHICLE_GET(vehicle, WheelFrames)[0].forward, &across);
            if (AI_SIGN_DEAD_ZONE < signDot) {
                index = 1;
            } else if (AI_LESS(signDot, -AI_SIGN_DEAD_ZONE)) {
                index = -1;
            } else {
                index = 0;
            }
            signDot = Vec3fDot(&VEHICLE_GET(vehicle, Frame).forward, &across);
            if (AI_SIGN_DEAD_ZONE < signDot) {
                forwardSign = 1;
            } else if (AI_LESS(signDot, -AI_SIGN_DEAD_ZONE)) {
                forwardSign = -1;
            } else {
                forwardSign = 0;
            }
            if (velocitySign != 0 || forwardSign != 0) {
                if (velocitySign != 0 && velocitySign + forwardSign == 0) {
                    if (velocitySign == 1 && index == 1) {
                        g_bAiNudgeRight = FALSE;
                        g_bAiNudgeLeft = TRUE;
                    } else if (velocitySign == -1 && index == -1) {
                        g_bAiNudgeRight = TRUE;
                        g_bAiNudgeLeft = FALSE;
                    }
                    steer = 0.1;
                } else if (velocitySign != 0 && velocitySign == forwardSign) {
                    if (velocitySign == 1 && index == 1) {
                        g_bAiNudgeRight = TRUE;
                        g_bAiNudgeLeft = FALSE;
                    } else if (velocitySign == -1 && index == -1) {
                        g_bAiNudgeRight = FALSE;
                        g_bAiNudgeLeft = TRUE;
                    }
                    steer = 0.4f;
                } else if (index != 0 || forwardSign != 0) {
                    if (index == 0 && velocitySign == 0) {
                        VEHICLE_GET(vehicle, Input)->buttons &= ~INPUT_ACCELERATE;
                    } else if (index == forwardSign) {
                        if (index == 1) {
                            g_bAiNudgeRight = TRUE;
                            g_bAiNudgeLeft = FALSE;
                        } else if (index == -1) {
                            g_bAiNudgeRight = FALSE;
                            g_bAiNudgeLeft = TRUE;
                        }
                        steer = 0.5;
                        VEHICLE_GET(vehicle, Input)->buttons &= ~INPUT_ACCELERATE;
                    } else if (index == 0 || index + velocitySign != 0) {
                        VEHICLE_GET(vehicle, Input)->buttons &= ~INPUT_ACCELERATE;
                    }
                }
            }
        }

        multiplier = 1.0f;
        for (level = 0; level < g_dwBestTraceDepth; level++) {
            Vec3fSubtract(&toLeft, &g_avBestLeftLane[level], &g_avBestCenterLine[level]);
#if TGR_BOSS_RALLY
            if (level + 1 == TRACE_DEPTH) {
                Vec3fSubtract(&ahead, &g_avTraceCenterLine[0], &g_avBestCenterLine[level]);
            } else {
                Vec3fSubtract(&ahead, &g_avBestCenterLine[level + 1], &g_avBestCenterLine[level]);
            }
#else
            Vec3fSubtract(&ahead, &g_avBestCenterLine[level + 1], &g_avBestCenterLine[level]);
#endif
            Vec3fCross(&normal, &ahead, &toLeft);
            Vec3fNormalize(&normal);
            climb = Vec3fDot(&VEHICLE_GET(vehicle, PlacedVelocity), &normal);
            lateral = Vec3fDot(&VEHICLE_GET(vehicle, PlacedVelocity), &toLeft) * 0.03f;
            if (climb < lateral) {
                climb = lateral;
            } else if (AI_LESS(lateral, -climb)) {
                climb = -lateral;
            }
            if (AI_GREATER(climb, (level + 1) * 6.0f)) {
                multiplier = 2.0f;
                VEHICLE_GET(vehicle, Input)->buttons &= ~INPUT_ACCELERATE;
                VEHICLE_GET(vehicle, Input)->buttons |= INPUT_BRAKE;
                break;
            }
            if (AI_GREATER(climb, (level + 1) * 4.5f)) {
                multiplier = 2.0f;
                VEHICLE_GET(vehicle, Input)->buttons &= ~INPUT_ACCELERATE;
                break;
            }
            if (AI_GREATER(climb, (level + 1) * 3.0f)) {
                multiplier = 1.3f;
                break;
            }
        }
        if (AI_CORRECT_SPEED < Vec3fLength(&VEHICLE_GET(vehicle, PlacedVelocity))) {
            side *= multiplier;
            if (1.0f < side) {
                VEHICLE_GET(vehicle, Input)->buttons &= ~INPUT_ACCELERATE;
                side = 1.0f;
            } else if (AI_LESS(side, -1.0f)) {
                side = -1.0f;
                VEHICLE_GET(vehicle, Input)->buttons &= ~INPUT_ACCELERATE;
            }
        }
        if (AI_LESS(steer, 0.01f)) {
            steer = 0.01f;
        }
        if (AI_LESS(side, 0.0f)) {
            if (g_bAiNudgeLeft) {
                if (AI_LESS(side, -(AI_INTERMEDIATE(0.2f) / steer))) {
                    side -= steer * side;
                } else {
                    side = side + 0.2f;
                }
            } else if (g_bAiNudgeRight) {
                if (AI_LESS(side, -(AI_INTERMEDIATE(0.2f) / steer))) {
#if TGR_BOSS_RALLY
                    side *= AI_INTERMEDIATE(steer) + 1.0f;
#else
                    side += steer * side;
#endif
                } else {
                    side = side - 0.2f;
                }
            }
            side = 1.0f + side;
            side = (f32)(side * side * side * side - 1.0f);
        } else {
            if (g_bAiNudgeLeft) {
                if (AI_INTERMEDIATE(0.2f) / steer < side) {
#if TGR_BOSS_RALLY
                    side *= AI_INTERMEDIATE(steer) + 1.0f;
#else
                    side += steer * side;
#endif
                } else {
                    side = side + 0.2f;
                }
            } else if (g_bAiNudgeRight) {
                if (AI_INTERMEDIATE(0.2f) / steer < side) {
                    side = side - steer * side;
                } else {
                    side = side - 0.2f;
                }
            }
            side = 1.0f - side;
            side = (f32)(1.0f - side * side * side * side);
        }
        speed = Vec3fLength(&VEHICLE_GET(vehicle, PlacedVelocity));
        if (0.0f <= VEHICLE_GET(vehicle, Accelerator)) {
            if (VEHICLE_GET(vehicle, ForcedReverseFrames) != 0) {
                VEHICLE_SET(
                    vehicle, ForcedReverseFrames, VEHICLE_GET(vehicle, ForcedReverseFrames) - 1);
                goto aiReverse;
            }
            if (AI_LESS(forwardSpeed, -1.0f)) {
                VEHICLE_GET(vehicle, Input)->buttons |= INPUT_BRAKE;
            } else {
            aiForward:
                VEHICLE_GET(vehicle, Input)->steering = -side;
                VEHICLE_SET(vehicle, ForwardFrames, VEHICLE_GET(vehicle, ForwardFrames) + 1);
                if (VEHICLE_GET(vehicle, ForwardFrames) >= AI_STUCK_CHECK_FRAMES &&
                    AI_LESS(speed, AI_STUCK_SPEED)) {
                    VEHICLE_SET(vehicle, ForcedReverseFrames, AI_UNSTICK_FRAMES);
                    VEHICLE_SET(vehicle, ReverseFrames, 0);
                    VEHICLE_SET(vehicle, ForwardFrames, 0);
                }
            }
        } else {
            if (VEHICLE_GET(vehicle, ForcedForwardFrames) != 0) {
                VEHICLE_SET(
                    vehicle, ForcedForwardFrames, VEHICLE_GET(vehicle, ForcedForwardFrames) - 1);
                goto aiForward;
            }
            if (1.0f < forwardSpeed) {
                VEHICLE_GET(vehicle, Input)->buttons |= INPUT_BRAKE;
            } else {
            aiReverse:
                VEHICLE_GET(vehicle, Input)->buttons |= INPUT_ACCELERATE;
                if (AI_LESS(side, 0.0f)) {
                    VEHICLE_GET(vehicle, Input)->steering = -1.0f;
                } else {
                    VEHICLE_GET(vehicle, Input)->steering = 1.0f;
                }
                VEHICLE_GET(vehicle, Input)->buttons |= INPUT_REVERSE;
                if (VEHICLE_GET(vehicle, ReverseFrames) >= AI_REVERSE_FLIP_FRAMES) {
                    if (VEHICLE_GET(vehicle, ReverseFrames) >= AI_REVERSE_RESET_FRAMES) {
                        VEHICLE_SET(vehicle, ReverseFrames, AI_REVERSE_RESET_TO);
                    } else {
                        VEHICLE_GET(vehicle, Input)->steering =
                            VEHICLE_GET(vehicle, Input)->steering * -1.0;
                    }
                }
                VEHICLE_SET(vehicle, ReverseFrames, VEHICLE_GET(vehicle, ReverseFrames) + 1);
                if (VEHICLE_GET(vehicle, ReverseFrames) >= AI_STUCK_CHECK_FRAMES &&
                    AI_LESS(speed, AI_STUCK_SPEED)) {
                    VEHICLE_SET(vehicle, ForcedForwardFrames, AI_UNSTICK_FRAMES);
                    VEHICLE_SET(vehicle, ReverseFrames, 0);
                    VEHICLE_SET(vehicle, ForwardFrames, 0);
                }
            }
        }

        ComputeTrackPosition(vehicle);
        best = AI_PASS_RANGE;
        push = 0.0f;
        found = -1;
        for (level = 0; level < g_dwNumVehicles; level++) {
            other = RACER_SLOT_GET(&(g_aRacerSlots[level]), Vehicle);
            if (other == NULL || other == vehicle) {
                continue;
            }
            lapLength = g_LoadedTrack.rootSection->points[0].distance;
            gap = AI_INTERMEDIATE(VEHICLE_GET(other, Distance)) - VEHICLE_GET(vehicle, Distance);
            while (lapLength < gap) {
                gap -= lapLength;
            }
            while (AI_LESS(gap, -lapLength)) {
                gap += lapLength;
            }
            if (0.0f < gap && AI_LESS(gap, best)) {
                best = gap;
                sideGap = AI_INTERMEDIATE(VEHICLE_GET(other, TrackSideOffset)) - sideOffset;
                if (AI_LESS(sideGap, AI_PASS_SIDE)) {
                    highPush = (1.0f - gap * (1.0f / AI_PASS_RANGE)) * -AI_PASS_PUSH;
                    push = highPush;
                    if (AI_LESS(highPush, -AI_PASS_PUSH_MAX)) {
                        push = -AI_PASS_PUSH_MAX;
                    }
                    found = level;
                } else if (-AI_PASS_SIDE < sideGap) {
                    /* Always true here: a gap of at least 3 is above -3. */
                    highPush = (1.0f - gap * (1.0f / AI_PASS_RANGE)) * AI_PASS_PUSH;
                    push = highPush;
                    if (AI_PASS_PUSH_MAX < highPush) {
                        push = AI_PASS_PUSH_MAX;
                    }
                    found = level;
                }
            }
        }
        if (found != -1) {
            updated = AI_INTERMEDIATE(sideOffset) + push;
            sideOffset = updated;
            VEHICLE_SET(
                vehicle, TrackSideDistance, AI_LESS(updated, 0.0f) ? -sideOffset : sideOffset);
        }
        if (AI_HOLD_SPEED < forwardSpeed) {
            if (AI_LESS(VEHICLE_GET(vehicle, TrackSideDistance),
                        AI_INTERMEDIATE(halfWidth) + 1.0f)) {
                hold = (AI_INTERMEDIATE(forwardSpeed) - AI_HOLD_SPEED) * AI_HOLD_SCALE;
                if (AI_LESS(hold, 0.0f)) {
                    hold *= -0.25f;
                }
                if (AI_HOLD_MAX < hold) {
                    hold = AI_HOLD_MAX;
                }
                relativeVelocity[0] = Vec3fDot(&VEHICLE_GET(vehicle, Body).state.velocity,
                                               &VEHICLE_GET(vehicle, TrackForward));
                /* The original computes the speed across the route and overwrites it unread. */
                relativeVelocity[1] = Vec3fDot(&VEHICLE_GET(vehicle, Body).state.velocity,
                                               &VEHICLE_GET(vehicle, TrackSide));
                relativeVelocity[2] = Vec3fDot(&VEHICLE_GET(vehicle, Body).state.velocity,
                                               &VEHICLE_GET(vehicle, TrackUp));
                relativeVelocity[1] = -(sideOffset * hold);
                Vec3fScale(&VEHICLE_GET(vehicle, Body).state.velocity,
                           &VEHICLE_GET(vehicle, TrackForward),
                           relativeVelocity[0]);
                Vec3fMulAddInPlace(&VEHICLE_GET(vehicle, Body).state.velocity,
                                   &VEHICLE_GET(vehicle, TrackSide),
                                   relativeVelocity[1]);
                Vec3fMulAddInPlace(&VEHICLE_GET(vehicle, Body).state.velocity,
                                   &VEHICLE_GET(vehicle, TrackUp),
                                   relativeVelocity[2]);
            }
            if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
#if TGR_REGION == TGR_REGION_EU
                Vec3fScaleInPlace(
                    &VEHICLE_GET(vehicle, Body).state.velocity,
                    g_aafAiVelocityRetention[AI_ACTIVE_PROFILE->season][AI_ACTIVE_PROFILE->round]
                                            [vehicle->racer->straggler] *
                        AI_RETENTION_SCALE);
#else
                Vec3fScaleInPlace(
                    &VEHICLE_GET(vehicle, Body).state.velocity,
                    g_aafAiVelocityRetention[AI_ACTIVE_PROFILE->season][AI_ACTIVE_PROFILE->round]
                                            [RACER_SLOT_GET(VEHICLE_GET(vehicle, Racer),
                                                            Straggler)]);
#endif /* TGR_REGION == TGR_REGION_EU */
            } else if (g_dwGameMode == GAME_MODE_ARCADE
#if TGR_BOSS_RALLY
                       || g_dwGameMode == GAME_MODE_NETWORK
#endif
            ) {
                weather = g_dwCurrentWeather - 1;
                if (weather >= ARCADE_WEATHERS || weather < 0) {
                    weather = 0;
                }
                if (weather == WEATHER_SNOW - 1) {
                    Vec3fScaleInPlace(&VEHICLE_GET(vehicle, Body).state.velocity,
                                      AI_ARCADE_SNOW_RETENTION);
                } else {
                    Vec3fScaleInPlace(&VEHICLE_GET(vehicle, Body).state.velocity,
                                      AI_ARCADE_RETENTION);
                }
            }
        }
    }
    AddTimingMarker(0, 0, 130, 0, 255);
    rawLength = Vec3fLength(&VEHICLE_GET(vehicle, Body).state.angularVelocity);
    length = rawLength;
    if (AI_MAX_SPIN < rawLength && VEHICLE_GET(vehicle, WheelBodies)[0].contactFrames) {
        Vec3fScaleInPlace(&VEHICLE_GET(vehicle, Body).state.angularVelocity,
                          AI_INTERMEDIATE(1.0f) / length);
    }
    UpdateVehiclePhysicsFrame(vehicle);
    RespawnVehicle(vehicle);
}

#if !TGR_BOSS_RALLY
void DriveAiVehicle(Vehicle *vehicle) {
    UpdateAiVehicleBehavior(vehicle);
}
#endif
