#include "driving.h"

#include "camera.h"
#include "frametime.h"
#include "game.h"
#include "input.h"
#include "racerslotaccess.h"
#include "render.h"
#include "system.h"
#include "timing.h"
#include "trackmenuitem.h"
#include "vehicleaccess.h"
#include "vehiclecount.h"
#include "vehiclephysics.h"
#include "vehiclereset.h"
#include "weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/gu.h>
#include <PR/os_libc.h>

#include "racemenu.h"
#else
#include "screen.h"
#endif

#if !(TGR_PLATFORM == TGR_PLATFORM_N64)
#define AddTimingMarker(...) NullStub()
#endif

#if TGR_PC
typedef long double AiReal;
#define AI_INTERMEDIATE(value) ((long double)(value))
#define AI_LESS(a, b) (!((a) >= (b)))
#define AI_ZERO(value) (!((value) < 0.0f) && !((value) > 0.0f))
#define AI_ACTIVE_PROFILE VEHICLE_GET(&g_aVehicles[0], Profile)
#else
typedef f32 AiReal;
#define AI_INTERMEDIATE(value) (value)
#define AI_LESS(a, b) ((a) < (b))
#define AI_ZERO(value) ((value) == 0.0f)
#define AI_ACTIVE_PROFILE g_pActiveProfile
#endif /* TGR_PC */

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

/* 1 above the dead zone of the computer's tests, -1 below it, and 0 within. */
static s32 AiSign(AiReal value) {
    if (AI_SIGN_DEAD_ZONE < value) {
        return 1;
    }
    if (AI_LESS(value, -AI_SIGN_DEAD_ZONE)) {
        return -1;
    }
    return 0;
}

/* The point of the route a computer looks ahead to, farther the faster it goes. */
static void FindAiLookPoint(Vehicle *vehicle, Vec3f *out) {
    TrackSection *section = VEHICLE_GET(vehicle, Section);
    s32 index = VEHICLE_GET(vehicle, PointIndex);
    TrackPoint *point;
    AiReal look =
        Vec3fLength(&VEHICLE_GET(vehicle, PlacedVelocity)) * AI_LOOK_PER_SPEED + AI_LOOK_BASE;
    AiReal step;

    /* The original stores the point of the vehicle first and overwrites it after the walk. */
    out->x = VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position.x;
    out->y = VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position.y;
    out->z = VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position.z;
    if (AI_LOOK_MAX < look) {
        look = AI_LOOK_MAX;
    }
    point = &section->points[index];
    for (;;) {
        step = AI_INTERMEDIATE(point[0].distance) - point[1].distance;
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
        if (AI_LESS(look, 0.0f)) {
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
    Vec3fDirection(
        &VEHICLE_GET(vehicle, TrackForward),
        &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position,
        &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex) + 1].position);
    Vec3fSubtract(&VEHICLE_GET(vehicle, TrackSide),
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
    Vec3fMulAddInPlace(
        &VEHICLE_GET(vehicle, TrackOffset), &VEHICLE_GET(vehicle, PlacedVelocity), AI_LOOK_TIME);
    return Vec3fDot(&VEHICLE_GET(vehicle, TrackSide), &VEHICLE_GET(vehicle, TrackOffset));
}

/* Correct a computer that strays from the route or skids across it. Return how hard it corrects. */
static f32 CorrectAiLine(Vehicle *vehicle,
                         f32 sideOffset,
                         f32 sideDistance,
                         f32 heading,
                         AiReal rawHalfWidth,
                         Vec3f *across) {
    f32 halfWidth = rawHalfWidth;
    AiReal limit;
    f32 steer = 0.0f;
    s32 velocitySign;
    s32 wheelSign;
    s32 forwardSign;

    if (AI_WIDE_HALF_WIDTH < rawHalfWidth) {
        limit = AI_INTERMEDIATE(halfWidth) - AI_SIDE_MARGIN;
    } else {
        limit = AI_INTERMEDIATE(halfWidth) * AI_NARROW_SHARE;
    }

    if (limit < sideDistance) {
        /* These four compare in double precision, and each computes the share past the limit. */
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
        velocitySign = AiSign(
            Vec3fDot(&VEHICLE_GET(vehicle, PlacedVelocity), &VEHICLE_GET(vehicle, TrackSide)));
        wheelSign = AiSign(Vec3fDot(&VEHICLE_GET(vehicle, WheelFrames)[0].forward, across));
        forwardSign = AiSign(Vec3fDot(&VEHICLE_GET(vehicle, Frame).forward, across));
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
                    VEHICLE_GET(vehicle, Input)->buttons &= ~INPUT_ACCELERATE;
                } else if (wheelSign == forwardSign) {
                    if (wheelSign == 1) {
                        g_bAiNudgeRight = TRUE;
                        g_bAiNudgeLeft = FALSE;
                    } else if (wheelSign == -1) {
                        g_bAiNudgeRight = FALSE;
                        g_bAiNudgeLeft = TRUE;
                    }
                    steer = 0.5f;
                    VEHICLE_GET(vehicle, Input)->buttons &= ~INPUT_ACCELERATE;
                } else if (wheelSign == 0 || wheelSign + velocitySign != 0) {
                    VEHICLE_GET(vehicle, Input)->buttons &= ~INPUT_ACCELERATE;
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
    AiReal lateral;
    f32 excess;
    s32 i = 0;

    while (i < g_dwBestTraceDepth) {
        Vec3fSubtract(&toLeft, &g_avBestLeftLane[i], &g_avBestCenterLine[i]);
        /* The boundary read follows the original data layout of each platform. */
        if (i + 1 == TRACE_DEPTH) {
#if TGR_PC
            Vec3fSubtract(&ahead, &g_avTraceCenterLine[0], &g_avBestCenterLine[i]);
#else
            Vec3fSubtract(&ahead, &g_avBestRightLane[0], &g_avBestCenterLine[i]);
#endif
        } else {
            Vec3fSubtract(&ahead, &g_avBestCenterLine[i + 1], &g_avBestCenterLine[i]);
        }
        Vec3fCross(&normal, &ahead, &toLeft);
        Vec3fNormalize(&normal);
        climb = Vec3fDot(&VEHICLE_GET(vehicle, PlacedVelocity), &normal);
        lateral = Vec3fDot(&VEHICLE_GET(vehicle, PlacedVelocity), &toLeft) * 0.03f;
        i++;
        if (climb < lateral) {
            excess = lateral;
        } else if (AI_LESS(lateral, -climb)) {
            excess = -lateral;
        } else {
            excess = climb;
        }
        if ((f32)i * 6.0f < excess) {
            VEHICLE_GET(vehicle, Input)->buttons &= ~INPUT_ACCELERATE;
            VEHICLE_GET(vehicle, Input)->buttons |= INPUT_BRAKE;
            return 2.0f;
        }
        if (AI_LESS((f32)i * 4.5f, excess)) {
            VEHICLE_GET(vehicle, Input)->buttons &= ~INPUT_ACCELERATE;
            return 2.0f;
        }
        if (AI_LESS((f32)i * 3.0f, excess)) {
            return 1.3f;
        }
    }
    return 1.0f;
}

/* Shape the steering of a computer, easing it to the side it is nudged to. */
static f32 ShapeAiSteering(AiReal side, f32 steer) {
    f32 threshold;

    if (AI_LESS(steer, 0.01f)) {
        steer = 0.01f;
    }
    threshold = AI_INTERMEDIATE(0.2f) / steer;
    if (AI_LESS(side, 0.0f)) {
        if (g_bAiNudgeLeft) {
            if (AI_LESS(side, -threshold)) {
                side -= steer * side;
            } else {
                side += 0.2f;
            }
        } else if (g_bAiNudgeRight) {
            if (AI_LESS(side, -threshold)) {
#if TGR_PC
                side *= AI_INTERMEDIATE(steer) + 1.0f;
#else
                side += steer * side;
#endif
            } else {
                side -= 0.2f;
            }
        }
        side = 1.0f + side;
        return side * side * side * side - 1.0f;
    }
    if (g_bAiNudgeLeft) {
        if (threshold < side) {
#if TGR_PC
            side *= AI_INTERMEDIATE(steer) + 1.0f;
#else
            side += steer * side;
#endif
        } else {
            side += 0.2f;
        }
    } else if (g_bAiNudgeRight) {
        if (threshold < side) {
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
    f32 speed = Vec3fLength(&VEHICLE_GET(vehicle, PlacedVelocity));
    s32 reverse;

    if (0.0f <= VEHICLE_GET(vehicle, Accelerator)) {
        if (VEHICLE_GET(vehicle, ForcedReverseFrames) != 0) {
            VEHICLE_SET(
                vehicle, ForcedReverseFrames, VEHICLE_GET(vehicle, ForcedReverseFrames) - 1);
            reverse = TRUE;
        } else if (AI_LESS(forwardSpeed, -1.0f)) {
            VEHICLE_GET(vehicle, Input)->buttons |= INPUT_BRAKE;
            return;
        } else {
            reverse = FALSE;
        }
    } else if (VEHICLE_GET(vehicle, ForcedForwardFrames) != 0) {
        VEHICLE_SET(vehicle, ForcedForwardFrames, VEHICLE_GET(vehicle, ForcedForwardFrames) - 1);
        reverse = FALSE;
    } else if (1.0f < forwardSpeed) {
        VEHICLE_GET(vehicle, Input)->buttons |= INPUT_BRAKE;
        return;
    } else {
        reverse = TRUE;
    }

    if (!reverse) {
        VEHICLE_GET(vehicle, Input)->steering = -side;
        VEHICLE_SET(vehicle, ForwardFrames, VEHICLE_GET(vehicle, ForwardFrames) + 1);
        if (VEHICLE_GET(vehicle, ForwardFrames) >= AI_STUCK_CHECK_FRAMES &&
            AI_LESS(speed, AI_STUCK_SPEED)) {
            VEHICLE_SET(vehicle, ForcedReverseFrames, AI_UNSTICK_FRAMES);
            VEHICLE_SET(vehicle, ReverseFrames, 0);
            VEHICLE_SET(vehicle, ForwardFrames, 0);
        }
        return;
    }
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
            VEHICLE_GET(vehicle, Input)->steering = VEHICLE_GET(vehicle, Input)->steering * -1.0;
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

/* Move a computer aside for the nearest vehicle ahead of it. Return the offset to the left it
 * aims for. */
static f32 PassAiVehicles(Vehicle *vehicle, f32 sideOffset) {
    Vehicle *other;
    f32 best = AI_PASS_RANGE;
    f32 push = 0.0f;
    f32 lapLength;
    AiReal gap;
    AiReal sideGap;
    AiReal highPush, updated;
    s32 found = -1;
    s32 i;

    for (i = 0; i < g_dwNumVehicles; i++) {
        other = RACER_SLOT_GET(&(g_aRacerSlots[i]), Vehicle);
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
                found = i;
            } else if (-AI_PASS_SIDE < sideGap) {
                /* Always true here: a gap of at least 3 is above -3. */
                highPush = (1.0f - gap * (1.0f / AI_PASS_RANGE)) * AI_PASS_PUSH;
                push = highPush;
                if (AI_PASS_PUSH_MAX < highPush) {
                    push = AI_PASS_PUSH_MAX;
                }
                found = i;
            }
        }
    }
    if (found != -1) {
        updated = AI_INTERMEDIATE(sideOffset) + push;
        sideOffset = updated;
        VEHICLE_SET(vehicle, TrackSideDistance, AI_LESS(updated, 0.0f) ? -sideOffset : sideOffset);
    }
    return sideOffset;
}

/* Constrain a computer's vehicle to the route by removing its speed across it, and slow it by the
 * mode. */
static void HoldAiToRoute(Vehicle *vehicle, f32 sideOffset, f32 halfWidth, f32 forwardSpeed) {
    Vec3f *velocity = &VEHICLE_GET(vehicle, Body).state.velocity;
    AiReal hold;
    f32 along;
    f32 across;
    f32 upward;
    s16 weather;

    if (AI_LESS(VEHICLE_GET(vehicle, TrackSideDistance), AI_INTERMEDIATE(halfWidth) + 1.0f)) {
        hold = (AI_INTERMEDIATE(forwardSpeed) - AI_HOLD_SPEED) * AI_HOLD_SCALE;
        if (AI_LESS(hold, 0.0f)) {
            hold *= -0.25f;
        }
        if (AI_HOLD_MAX < hold) {
            hold = AI_HOLD_MAX;
        }
        along = Vec3fDot(velocity, &VEHICLE_GET(vehicle, TrackForward));
        /* The original computes the speed across the route and overwrites it unread. */
        across = Vec3fDot(velocity, &VEHICLE_GET(vehicle, TrackSide));
        upward = Vec3fDot(velocity, &VEHICLE_GET(vehicle, TrackUp));
        across = -(sideOffset * hold);
        Vec3fScale(velocity, &VEHICLE_GET(vehicle, TrackForward), along);
        Vec3fMulAddInPlace(velocity, &VEHICLE_GET(vehicle, TrackSide), across);
        Vec3fMulAddInPlace(velocity, &VEHICLE_GET(vehicle, TrackUp), upward);
    }
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
#if TGR_REGION == TGR_REGION_EU
        Vec3fScaleInPlace(
            velocity,
            g_aafAiVelocityRetention[AI_ACTIVE_PROFILE->season][AI_ACTIVE_PROFILE->round]
                                    [vehicle->racer->straggler] *
                AI_RETENTION_SCALE);
#else
        Vec3fScaleInPlace(
            velocity,
            g_aafAiVelocityRetention[AI_ACTIVE_PROFILE->season][AI_ACTIVE_PROFILE->round]
                                    [RACER_SLOT_GET(VEHICLE_GET(vehicle, Racer), Straggler)]);
#endif /* TGR_REGION == TGR_REGION_EU */
    } else if (g_dwGameMode == GAME_MODE_ARCADE
#if TGR_PC
               || g_dwGameMode == GAME_MODE_NETWORK
#endif
    ) {
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
    AiReal side;
    AiReal rawHalfWidth;
    f32 sideOffset;
    f32 sideDistance;
    f32 heading;
    f32 halfWidth;
    f32 steer;
    f32 multiplier;

    if (VEHICLE_GET(vehicle, Input)->buttons & INPUT_VIEW_TRACKSIDE) {
        VEHICLE_SET(vehicle, CameraChanged, 1);
        PlaceTrackCamera(vehicle);
    }
    AddTimingMarker(0, 255, 255, 255, 255);
    FindAiLookPoint(vehicle, &target);
    if (TraceTrackSplineRecursive(
            vehicle, 0, VEHICLE_GET(vehicle, PointIndex), VEHICLE_GET(vehicle, Section))) {
        Vec3fMidpoint(&g_vTraceFarPoint, &g_vTraceFarPoint, &g_vTraceHalfwayPoint);
        if (AI_ZERO(VEHICLE_GET(vehicle, AiTarget).x) &&
            AI_ZERO(VEHICLE_GET(vehicle, AiTarget).y) &&
            AI_ZERO(VEHICLE_GET(vehicle, AiTarget).z)) {
            VEHICLE_GET(vehicle, AiTarget).x = g_vTraceFarPoint.x;
            VEHICLE_GET(vehicle, AiTarget).y = g_vTraceFarPoint.y;
            VEHICLE_GET(vehicle, AiTarget).z = g_vTraceFarPoint.z;
        } else {
            Vec3fLerp(&VEHICLE_GET(vehicle, AiTarget),
                      &VEHICLE_GET(vehicle, AiTarget),
                      &g_vTraceFarPoint,
                      AI_TARGET_FOLLOW + AI_INTERMEDIATE(g_fFrameDeltaTime));
        }
    }
    Vec3fLerp(
        &VEHICLE_GET(vehicle, AiTarget), &VEHICLE_GET(vehicle, AiTarget), &target, AI_TARGET_PULL);
    forwardSpeed =
        Vec3fDot(&VEHICLE_GET(vehicle, PlacedVelocity), &VEHICLE_GET(vehicle, Frame).forward);
    Vec3fDirection(
        &direction, &VEHICLE_GET(vehicle, Frame).position, &VEHICLE_GET(vehicle, AiTarget));
#if TGR_PC
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
    VEHICLE_SET(vehicle, Accelerator, Vec3fDot(&VEHICLE_GET(vehicle, Frame).forward, &direction));
    VEHICLE_GET(vehicle, Input)->buttons |= INPUT_ACCELERATE;

    sideOffset = MeasureAiRoutePosition(vehicle);
    heading = Vec3fDot(&VEHICLE_GET(vehicle, Frame).forward, &VEHICLE_GET(vehicle, TrackSide));
    g_bAiNudgeLeft = FALSE;
    g_bAiNudgeRight = FALSE;
    sideDistance = AI_LESS(sideOffset, 0.0f) ? -sideOffset : sideOffset;
    point = &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)];
    Vec3fSubtract(&edge, &point->left, &point->position);
    rawHalfWidth = Vec3fDot(&VEHICLE_GET(vehicle, TrackSide), &edge);
    halfWidth = rawHalfWidth;
    steer = CorrectAiLine(vehicle, sideOffset, sideDistance, heading, rawHalfWidth, &across);

    multiplier = ReadAiLineAhead(vehicle);
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
    side = ShapeAiSteering(side, steer);
    ApplyAiDrive(vehicle, side, forwardSpeed);

    ComputeTrackPosition(vehicle);
    sideOffset = PassAiVehicles(vehicle, sideOffset);
    if (AI_HOLD_SPEED < forwardSpeed) {
        HoldAiToRoute(vehicle, sideOffset, halfWidth, forwardSpeed);
    }
}

void UpdateAiVehicleBehavior(Vehicle *vehicle) {
    Vec3f *spin = &VEHICLE_GET(vehicle, Body).state.angularVelocity;
    f32 length;
    AiReal rawLength;

    if (!(RACER_SLOT_GET(VEHICLE_GET(vehicle, Racer), Status) & RACER_HELD)) {
        PlanAiDrive(vehicle);
    }
    AddTimingMarker(0, 0, 130, 0, 255);
    rawLength = Vec3fLength(spin);
    length = rawLength;
    if (AI_MAX_SPIN < rawLength && VEHICLE_GET(vehicle, WheelBodies)[0].contactFrames) {
        Vec3fScaleInPlace(spin, AI_INTERMEDIATE(1.0f) / length);
    }
    UpdateVehiclePhysicsFrame(vehicle);
    RespawnVehicle(vehicle);
}

void DriveAiVehicle(Vehicle *vehicle) {
    UpdateAiVehicleBehavior(vehicle);
}
