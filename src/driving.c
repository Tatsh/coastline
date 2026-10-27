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
