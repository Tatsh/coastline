#include "vehicle.h"

#include <PR/gu.h>
#include <PR/os_libc.h>

#include "camera.h"
#include "game.h"
#include "mathutil.h"
#include "racehud.h"
#include "trackselect.h"
#include "tricache.h"

/* The rows of a frame matrix whose last column FinishFrameMatrix() sets. */
#define MATRIX_ROWS 4

/* A vehicle that has not crossed out of its point of the route keeps it while this near. */
#define POINT_KEEP_DISTANCE 64.0f

/* The farthest a section may be along the route from a vehicle to be searched, and the farthest
 * the distance may jump in a frame. */
#define ROUTE_DISTANCE_WINDOW 1000.0f

/* The cells of the grid on tracks 3 and 8 where a vehicle is put in one cell without the checks. */
#define GRID_PATCH_MIN_X 56
#define GRID_PATCH_END_X 59
#define GRID_PATCH_MIN_Y 23
#define GRID_PATCH_END_Y 28
#define GRID_PATCH_X 57
#define GRID_PATCH_Y 25

/* The size of a cell of the grid, and the cells across the track. */
#define GRID_CELL 32.0f
#define GRID_CELLS 64

/* The warning of a vehicle facing against the track: the frames before it, its flash and its
 * time on the screen. */
#define WRONG_WAY_FRAMES 32
#define WRONG_WAY_FLASH 0x10
#define WRONG_WAY_TIME 0.25f

/* How far above a point CastRayAgainstTrack() still finds the ground, how near it lists the
 * models near the ground, and how far above that it takes a ceiling. */
#define RAY_HEADROOM 1.5f
#define RAY_NEARBY_DISTANCE 5.0f
#define RAY_CEILING_DEPTH -1.0f

/* The rates of a flying vehicle. */
#define FLY_SPEED 100.0f
#define FLY_TURN_RATE 3.141592741f
#define FLY_TURN_SCALE 0.4f

/* The input bit that toggles reverse in first gear, and the one that rights a flying vehicle. */
#define INPUT_REVERSE_TOGGLE (1 << 17)
#define INPUT_FLY_LEVEL 0x10

/* The share of the drive the front wheels take with front-wheel drive, and the rear ones. */
#define FRONT_DRIVE_SHARE 2.0f
#define REAR_DRIVE_SHARE 0.0f

/* The drive of a rear wheel below which the brakes act on the rear wheels as well. */
#define REAR_BRAKE_DRIVE_LIMIT 1.0e-4f

/* The scale from the speed of a vehicle to the speed the display shows. */
#define SPEED_TO_DISPLAY 2.24f

/* A quarter of a turn, and the field of view of the cameras. */
#define HALF_PI 1.570796371f
#define DEFAULT_CAMERA_FOV 0.5235987902f

static const char sWrongWay[] = "%yyWRONG WAY";

void FinishFrameMatrix(f32 m[4][4]) {
    m[2][3] = 0.0f;
    m[1][3] = 0.0f;
    m[0][3] = 0.0f;
    m[3][3] = 1.0f;
}

/* The direction along the track at a point, across from its left edge to its right. */
static void TrackPointDirection(Vec3f *out, TrackPoint *point) {
    out->x = point->left.y - point->right.y;
    out->y = point->right.x - point->left.x;
    out->z = 0.0f;
}

s32 UpdateVehicleTrackPosition(Vehicle *vehicle) {
    TrackSection *section;
    TrackPoint *point;
    Vec3f direction, offset;
    f32 x = vehicle->frame.position.x;
    f32 y = vehicle->frame.position.y;
    f32 z = vehicle->frame.position.z;
    f32 lapLength, intoLap, best, distance, dx, dy, dz, change;
    s32 checked = TRUE;
    s32 bestPoint = -1;
    s32 bestSection = 0;
    s32 i, p;
    u8 gridX = vehicle->gridX;
    u8 gridY = vehicle->gridY;

    if (g_dwSelectedTrackId == 3 || g_dwSelectedTrackId == 8) {
        if (gridX >= GRID_PATCH_MIN_X && gridX < GRID_PATCH_END_X && gridY >= GRID_PATCH_MIN_Y &&
            gridY < GRID_PATCH_END_Y) {
            gridX = GRID_PATCH_X;
            gridY = GRID_PATCH_Y;
            checked = FALSE;
        }
    }
    intoLap = vehicle->distance -
              g_LoadedTrack.rootSection->points[0].distance * (f32)vehicle->distanceLaps;

    /* Keep the point while the vehicle has not crossed out of it and stays near it. */
    if (checked) {
        section = vehicle->section;
        i = vehicle->pointIndex;
        if (!SegmentCrossesLine2D(&section->points[i].left,
                                  &section->points[i].right,
                                  (Vec2f *)&vehicle->previousPosition,
                                  (Vec2f *)&vehicle->frame.position) &&
            !SegmentCrossesLine2D(&section->points[i + 1].left,
                                  &section->points[i + 1].right,
                                  (Vec2f *)&vehicle->previousPosition,
                                  (Vec2f *)&vehicle->frame.position) &&
            Vec3fDistance(&vehicle->frame.position, &section->points[i].position) <
                POINT_KEEP_DISTANCE) {
            p = i;
            goto found;
        }
    }

    /* The nearest point ahead of the vehicle, of the sections of its cell near it on the route. */
    best = g_LoadedTrack.maxX - g_LoadedTrack.minX;
    best *= best;
    for (i = 0; i < g_LoadedTrack.sectionCount; i++) {
        section = g_LoadedTrack.sections[i];
        if (vehicle->index >= (s32)g_dwNumLocalPlayers && g_dwGameMode != GAME_MODE_TIME_ATTACK &&
            (section->flags & TRACK_SECTION_ALTERNATE)) {
            continue;
        }
        if (gridX < section->gridMinX || section->gridMaxX < gridX || gridY < section->gridMinY ||
            section->gridMaxY < gridY) {
            continue;
        }
        if (checked) {
            lapLength = g_LoadedTrack.rootSection->points[0].distance;
            if (ROUTE_DISTANCE_WINDOW < lapLength - section->points[0].distance - intoLap) {
                continue;
            }
            if (ROUTE_DISTANCE_WINDOW <
                intoLap - (lapLength - section->points[section->pointCount].distance)) {
                continue;
            }
        }
        for (p = 0; p < section->pointCount; p++) {
            point = &section->points[p];
            dx = point->position.x - x;
            dy = point->position.y - y;
            dz = point->position.z - z;
            distance = dx * dx + dy * dy + dz * dz;
            if (distance < best) {
                TrackPointDirection(&direction, point);
                Vec3fSubtract(&offset, &vehicle->frame.position, &point->position);
                if (0.0f <= Vec3fDot(&direction, &offset)) {
                    best = distance;
                    bestPoint = p;
                    bestSection = i;
                }
            }
        }
    }
    if (bestPoint == -1) {
        return 0;
    }
    section = g_LoadedTrack.sections[bestSection];
    p = bestPoint;

found:
    /* The distance moves by how far along the track the vehicle is from the point. */
    point = &section->points[p];
    TrackPointDirection(&direction, point);
    Vec3fNormalize(&direction);
    Vec3fSubtract(&offset, &vehicle->frame.position, &point->position);
    change = Vec3fDot(&direction, &offset) +
             (g_LoadedTrack.rootSection->points[0].distance * (f32)(vehicle->distanceLaps + 1) -
              point->distance);
    change -= vehicle->distance;
    if (!checked || (-ROUTE_DISTANCE_WINDOW < change && change < ROUTE_DISTANCE_WINDOW)) {
        vehicle->distance = vehicle->distance + change;
    }
    vehicle->section = section;
    vehicle->pointIndex = p;
    vehicle->trackDirection = direction;
    return 1;
}

void UpdateRaceTimers(Vehicle *vehicle) {
    if (vehicle->racer->status & RACER_CLOCK_STOPPED) {
        return;
    }
    if (g_dwGameMode == GAME_MODE_PRACTICE) {
        vehicle->lapTime = vehicle->lapTime + g_fFrameDeltaTime;
        return;
    }
    vehicle->totalTime = vehicle->totalTime + g_fFrameDeltaTime;
    vehicle->lapTime = vehicle->lapTime + g_fFrameDeltaTime;
    if (g_dwGameMode == GAME_MODE_ARCADE) {
        vehicle->timeLeft = vehicle->timeLeft - g_fFrameDeltaTime;
        if (vehicle->timeLeft < 0.0f) {
            vehicle->timeLeft = 0.0f;
        }
    }
}

void UpdateHudMessageTimers(Vehicle *vehicle) {
    if (vehicle->messageTime != 0.0f) {
        vehicle->messageTime = vehicle->messageTime - g_fFrameDeltaTime;
        if (vehicle->messageTime <= 0.0f) {
            vehicle->messageTime = 0.0f;
            vehicle->message = NULL;
        }
    } else if (vehicle->subMessageTime != 0.0f) {
        vehicle->subMessageTime = vehicle->subMessageTime - g_fFrameDeltaTime;
        if (vehicle->subMessageTime <= 0.0f) {
            vehicle->subMessageTime = 0.0f;
            vehicle->subMessage = NULL;
        }
    }
}

void CheckWrongWayWarning(Vehicle *vehicle) {
    if (g_LoadedTrack.rootSection == NULL) {
        return;
    }
    if (vehicle->lap >= g_dwTotalLaps || vehicle->flyMode ||
        !((f64)Vec3fDot(&vehicle->trackDirection, &vehicle->frame.forward) < 0.0)) {
        vehicle->wrongWayFrames = 0;
        return;
    }
    vehicle->wrongWayFrames++;
    if (vehicle->wrongWayFrames >= WRONG_WAY_FRAMES &&
        (vehicle->wrongWayFrames & WRONG_WAY_FLASH) == WRONG_WAY_FLASH) {
        if (vehicle->message == NULL) {
            vehicle->message = sWrongWay;
            vehicle->subMessage = NULL;
            vehicle->messageTime = WRONG_WAY_TIME;
        }
        return;
    }
    if (vehicle->message == sWrongWay) {
        vehicle->subMessage = NULL;
        vehicle->message = NULL;
    }
}

void UpdateVehicleGridPosition(Vehicle *vehicle) {
    if (g_LoadedTrack.rootSection == NULL) {
        return;
    }
    vehicle->gridX = FloatToInt(vehicle->frame.position.x / GRID_CELL);
    vehicle->gridY = FloatToInt(vehicle->frame.position.y / GRID_CELL);
    if (vehicle->gridX >= GRID_CELLS) {
        vehicle->gridX = GRID_CELLS - 1;
    }
    if (vehicle->gridY >= GRID_CELLS) {
        vehicle->gridY = GRID_CELLS - 1;
    }
    UpdateVehicleTrackPosition(vehicle);
}

s32 CastRayAgainstTrack(Vec3f *outPoint,
                        Vec3f *outNormal,
                        Vec3f *origin,
                        u16 *groundInstances,
                        s32 *done,
                        u16 *nearbyInstances,
                        s32 *nearbyCount,
                        f32 *groundDistance,
                        s32 *groundSurface) {
    TrackTriangle *triangle, *end;
    Vec3f ceilingNormal = {0.0f, 0.0f, 1.0f};
    Vec3f groundNormal = {0.0f, 0.0f, 1.0f};
    Vec3f line, direction, toVertex, hit;
    f32 nearest, nearestCeiling, groundZ, ceilingZ, denominator, below;
    s32 slot, groundHits = 0, groundCount = 0, nearby = 0, j;
    s16 ceilingTriangle = 0;
    u16 ceilingInstance = 0, instance;

    nearest = g_LoadedTrack.maxZ - g_LoadedTrack.minZ;
    nearest = nearest * nearest + 1.0f;
    nearestCeiling = nearest;
    groundZ = origin->z;
    ceilingZ = origin->z;
    direction.x = 0.0f;
    direction.y = 0.0f;
    direction.z = 1.0f;
    line.x = origin->x;
    line.y = origin->y;
    line.z = 1.0f;
    *groundSurface = 0;

    slot = BuildTrackTriangleCache(line.x, line.y);
    triangle = g_aTriCacheTriangles[slot];
    end = triangle + g_awTriCacheTriCount[slot];
    for (; triangle != end; triangle++) {
        /* The vertical line meets the plane of each triangle facing up. */
        if (triangle->normal.z < 0.0f) {
            continue;
        }
        denominator = Vec3fDot(&direction, &triangle->normal);
        if (denominator == 0.0f) {
            continue;
        }
        Vec3fSubtract(&toVertex, triangle->vertices[0], &line);
        Vec3fMulAdd(&hit, &line, &direction, Vec3fDot(&toVertex, &triangle->normal) / denominator);
        if (!PointInTriangle3D(&hit,
                               triangle->vertices[0],
                               triangle->vertices[1],
                               triangle->vertices[2],
                               &triangle->normal)) {
            continue;
        }

        /* The ground: the triangles below the point and a little above it, the nearest first. */
        below = origin->z + RAY_HEADROOM - hit.z;
        if (0.0f <= below) {
            groundHits++;
            if (below < nearest) {
                *groundSurface = triangle->index;
                nearest = below;
                groundZ = hit.z;
                if (triangle->normal.z < 0.0f) {
                    Vec3fNegate(&groundNormal, &triangle->normal);
                } else {
                    groundNormal = triangle->normal;
                }
                if (groundCount < RAY_INSTANCES) {
                    groundInstances[groundCount++] = groundInstances[0];
                }
                groundInstances[0] = g_LoadedTrack.surfaces[triangle->index].instance + 1;
                if (below < RAY_NEARBY_DISTANCE) {
                    j = g_LoadedTrack.triangleNearbyInstances[triangle->index];
                    for (instance = g_LoadedTrack.nearbyInstances[j]; instance != 0;
                         instance = g_LoadedTrack.nearbyInstances[++j]) {
                        if (nearby < RAY_INSTANCES) {
                            nearbyInstances[nearby++] = nearbyInstances[0];
                        }
                        nearbyInstances[0] = instance;
                    }
                }
            } else if (groundCount < RAY_INSTANCES) {
                groundInstances[groundCount++] =
                    g_LoadedTrack.surfaces[triangle->index].instance + 1;
                if (below < RAY_NEARBY_DISTANCE) {
                    j = g_LoadedTrack.triangleNearbyInstances[triangle->index];
                    for (instance = g_LoadedTrack.nearbyInstances[j]; instance != 0;
                         instance = g_LoadedTrack.nearbyInstances[++j]) {
                        if (nearby < RAY_INSTANCES) {
                            nearbyInstances[nearby++] = instance;
                        }
                    }
                }
            }
        }

        /* The ceiling: the nearest triangle just above the headroom. */
        below -= RAY_HEADROOM;
        if (below <= 0.0f && below < nearestCeiling) {
            ceilingInstance = g_LoadedTrack.surfaces[triangle->index].instance + 1;
            if (RAY_CEILING_DEPTH < below) {
                ceilingTriangle = triangle->index;
                nearestCeiling = below;
                ceilingZ = hit.z;
                if (triangle->normal.z < 0.0f) {
                    Vec3fNegate(&ceilingNormal, &triangle->normal);
                } else {
                    ceilingNormal = triangle->normal;
                }
            }
        }
    }

    if (groundHits != 0) {
        if (outPoint != NULL) {
            outPoint->z = groundZ;
            *groundDistance = nearest - RAY_HEADROOM;
        }
        if (outNormal != NULL) {
            *outNormal = groundNormal;
        }
    } else {
        /* Without ground, the ceiling stands in for it. */
        j = g_LoadedTrack.triangleNearbyInstances[(u16)ceilingTriangle];
        for (instance = g_LoadedTrack.nearbyInstances[j]; instance != 0;
             instance = g_LoadedTrack.nearbyInstances[++j]) {
            if (nearby >= RAY_INSTANCES) {
                nearbyInstances[RAY_INSTANCES - 1] = instance;
                break;
            }
            nearbyInstances[nearby++] = instance;
        }
        if (groundCount < RAY_INSTANCES) {
            groundInstances[groundCount] = ceilingInstance;
        } else {
            groundInstances[RAY_INSTANCES - 1] = ceilingInstance;
        }
        if (outPoint != NULL) {
            outPoint->z = ceilingZ;
        }
        if (outNormal != NULL) {
            *outNormal = ceilingNormal;
        }
    }
    *done = 1;
    *nearbyCount = nearby;
    return groundHits;
}

void UpdateVehiclePhysicsFrame(Vehicle *vehicle) {
    RigidBody **wheels = vehicle->body.wheels;
    Vec3f step;
    Vec3f *velocity;
    f32 drive;
    s32 i;

    if (vehicle->flyMode) {
        /* Fly along the vehicle and turn with the controls. */
        Vec3fScale(
            &step, &vehicle->frame.forward, vehicle->accelerator * g_fFrameDeltaTime * FLY_SPEED);
        Vec3fAddInPlace(&vehicle->frame.position, &step);
        SetVehiclePosition(vehicle,
                           vehicle->frame.position.x,
                           vehicle->frame.position.y,
                           vehicle->frame.position.z);
        SetVehicleVelocity(vehicle, 0.0f, 0.0f, 0.0f);
        SetVehicleTurnRate(vehicle, 0.0f, 0.0f, 0.0f);
        vehicle->engineRpm = 0.0f;
        RotateVehicleByEuler(
            vehicle,
            vehicle->steering * FLY_TURN_RATE * FLY_TURN_SCALE * g_fFrameDeltaTime,
            vehicle->pitchInput * FLY_TURN_RATE * FLY_TURN_SCALE * g_fFrameDeltaTime,
            vehicle->rollInput * FLY_TURN_RATE * FLY_TURN_SCALE * g_fFrameDeltaTime);
        if (vehicle->input->buttons & INPUT_FLY_LEVEL) {
            vehicle->frame.up.x = 0.0f;
            vehicle->frame.up.y = 0.0f;
            vehicle->frame.up.z = 1.0f;
            Vec3fCross(&vehicle->frame.side, &vehicle->frame.up, &vehicle->frame.forward);
            Vec3fCross(&vehicle->frame.up, &vehicle->frame.forward, &vehicle->frame.side);
            TransferFlags(vehicle->input, INPUT_FLY_LEVEL);
        }
    } else {
        /* Put the vehicle on the ground, then drive and brake its wheels and step its physics. */
        vehicle->groundHits = CastRayAgainstTrack(&vehicle->frame.position,
                                                  &vehicle->frame.up,
                                                  &vehicle->frame.position,
                                                  vehicle->groundInstances,
                                                  &vehicle->rayDone,
                                                  vehicle->nearbyInstances,
                                                  &vehicle->nearbyInstanceCount,
                                                  &vehicle->groundDistance,
                                                  &vehicle->groundSurface);
        wheels[1]->friction = 0.0f;
        wheels[0]->friction = wheels[1]->friction;
        UpdateVehicleEngineAndGearbox(vehicle);
        wheels[3]->friction = vehicle->rearWheelFriction;
        wheels[2]->friction = vehicle->rearWheelFriction;
        for (i = 0; i < VEHICLE_WHEELS; i++) {
            wheels[i]->drive = 0.0f;
        }
        for (i = 0; i < VEHICLE_WHEELS; i++) {
            wheels[i]->brake = 0.0f;
        }
        if (0.0f < vehicle->throttle) {
            if ((vehicle->input->buttons & INPUT_REVERSE_TOGGLE) && vehicle->gear == 1) {
                vehicle->throttle = -vehicle->throttle;
            }
            if (vehicle->frontWheelDrive) {
                wheels[0]->drive = -vehicle->throttle * FRONT_DRIVE_SHARE;
                wheels[1]->drive = -vehicle->throttle * FRONT_DRIVE_SHARE;
                wheels[2]->drive = -vehicle->throttle * REAR_DRIVE_SHARE;
                wheels[3]->drive = -vehicle->throttle * REAR_DRIVE_SHARE;
            } else {
                for (i = 0; i < VEHICLE_WHEELS; i++) {
                    wheels[i]->drive = -vehicle->throttle;
                }
            }
        }
        if (vehicle->brake < 0.0f) {
            wheels[0]->brake = -vehicle->brake;
            wheels[1]->brake = -vehicle->brake;
            wheels[0]->drive = 0.0f;
            wheels[1]->drive = 0.0f;
            drive = wheels[2]->drive < 0.0f ? -wheels[2]->drive : wheels[2]->drive;
            if (drive < REAR_BRAKE_DRIVE_LIMIT) {
                wheels[2]->brake = -vehicle->brake;
                wheels[3]->brake = -vehicle->brake;
            }
        }
        SimulateVehiclePhysicsSubstep(vehicle);
    }

    TransformAllWheelMatrices(vehicle);
    if (vehicle->physicsActive) {
        velocity = &vehicle->body.state.velocity;
        vehicle->speedKph = sqrtf(velocity->x * velocity->x + velocity->y * velocity->y +
                                  velocity->z * velocity->z) *
                            SPEED_TO_DISPLAY;
    }
    for (i = 0; i < g_dwNumViewports; i++) {
        if (vehicle->index == g_aViewportRects[i].vehicleIndex) {
            UpdateCameraSystem(vehicle);
            return;
        }
    }
}

void SetVehiclePosition(Vehicle *vehicle, f32 x, f32 y, f32 z) {
    vehicle->frame.position.y = y;
    vehicle->placedPosition.y = y;
    vehicle->body.state.position.y = y;
    vehicle->body.stepStates[1].position.y = y;
    vehicle->body.stepStates[0].position.y = y;
    vehicle->frame.position.z = z;
    vehicle->placedPosition.z = z;
    vehicle->body.state.position.z = z;
    vehicle->body.stepStates[1].position.z = z;
    vehicle->body.stepStates[0].position.z = z;
    vehicle->frame.position.x = x;
    vehicle->placedPosition.x = x;
    vehicle->body.state.position.x = x;
    vehicle->body.stepStates[1].position.x = x;
    vehicle->body.stepStates[0].position.x = x;
    QuaternionToMatrix(vehicle->body.orientation, &vehicle->body.state);
}

void InitVehicleOrientationFromAngle(Vehicle *vehicle, f32 angle) {
    f32 forwardX = cosf(angle);
    f32 forwardY = sinf(angle);
    f32 sideX = cosf(angle + HALF_PI);
    f32 sideY = sinf(angle + HALF_PI);
    f32 half = angle * 0.5f;

    vehicle->frame.forward.x = forwardX;
    vehicle->frame.forward.y = forwardY;
    vehicle->frame.forward.z = 0.0f;
    vehicle->frame.side.x = sideX;
    vehicle->frame.side.y = sideY;
    vehicle->frame.side.z = 0.0f;
    vehicle->frame.up.x = 0.0f;
    vehicle->frame.up.y = 0.0f;
    vehicle->frame.up.z = 1.0f;
    vehicle->body.state.orientation.w = cosf(half);
    vehicle->body.state.orientation.x = 0.0f;
    vehicle->body.state.orientation.y = 0.0f;
    vehicle->body.state.orientation.z = sinf(half);
    vehicle->body.stepStates[1].orientation = vehicle->body.state.orientation;
    vehicle->body.stepStates[0].orientation = vehicle->body.state.orientation;
    QuaternionToMatrix(vehicle->body.orientation, &vehicle->body.state);
}

void MatrixToQuaternion(f32 m[4][4], Quaternion *q) {
    if (0.0f <= m[0][0]) {
        if (0.0f <= m[2][2] + m[1][1]) {
            q->w = m[2][2] + (1.0f + m[0][0] + m[1][1]);
            q->x = m[1][2] - m[2][1];
            q->y = m[2][0] - m[0][2];
            q->z = m[0][1] - m[1][0];
        } else {
            q->w = m[1][2] - m[2][1];
            q->x = 1.0f + m[0][0] - m[1][1] - m[2][2];
            q->y = m[1][0] + m[0][1];
            q->z = m[0][2] + m[2][0];
        }
    } else if (m[2][2] <= m[1][1]) {
        q->w = m[2][0] - m[0][2];
        q->x = m[1][0] + m[0][1];
        q->y = 1.0f - m[0][0] + m[1][1] - m[2][2];
        q->z = m[2][1] + m[1][2];
    } else {
        q->w = m[0][1] - m[1][0];
        q->x = m[0][2] + m[2][0];
        q->y = m[2][1] + m[1][2];
        q->z = m[2][2] + (1.0f - m[0][0] - m[1][1]);
    }
    QuaternionNormalize(q);
}

void SetVehicleFrameMatrix(Vehicle *vehicle, f32 m[4][4]) {
    memcpy(vehicle->matrix, m, sizeof(vehicle->matrix));
    MatrixToQuaternion(m, &vehicle->body.state.orientation);
    vehicle->body.stepStates[1].orientation = vehicle->body.state.orientation;
    vehicle->body.stepStates[0].orientation = vehicle->body.state.orientation;
    QuaternionToMatrix(vehicle->body.orientation, &vehicle->body.state);
}

void SetVehicleVelocity(Vehicle *vehicle, f32 x, f32 y, f32 z) {
    vehicle->body.state.velocity.x = x;
    vehicle->body.state.velocity.y = y;
    vehicle->body.stepStates[1].velocity.x = x;
    vehicle->body.stepStates[1].velocity.y = y;
    vehicle->body.state.velocity.z = z;
    vehicle->body.stepStates[0].velocity.x = x;
    vehicle->body.stepStates[0].velocity.y = y;
    vehicle->body.stepStates[1].velocity.z = z;
    vehicle->body.stepStates[0].velocity.z = z;
    vehicle->placedVelocity.x = x;
    vehicle->placedVelocity.y = y;
    vehicle->placedVelocity.z = z;
}

void RotateVehicleByEuler(Vehicle *vehicle, f32 yaw, f32 pitch, f32 roll) {
    Quaternion *orientation = &vehicle->body.state.orientation;
    Quaternion turn;
    f32 sine;

    yaw *= 0.5f;
    pitch *= 0.5f;
    roll *= 0.5f;
    sine = sinf(yaw);
    turn.w = cosf(yaw);
    turn.x = 0.0f;
    turn.y = 0.0f;
    turn.z = sine;
    QuaternionMultiply(orientation, orientation, &turn);
    sine = sinf(pitch);
    turn.w = cosf(pitch);
    turn.x = 0.0f;
    turn.y = sine;
    turn.z = 0.0f;
    QuaternionMultiply(orientation, orientation, &turn);
    sine = sinf(roll);
    turn.w = cosf(roll);
    turn.x = sine;
    turn.y = 0.0f;
    turn.z = 0.0f;
    QuaternionMultiply(orientation, orientation, &turn);
    QuaternionNormalize(orientation);
    vehicle->body.stepStates[1].orientation = *orientation;
    vehicle->body.stepStates[0].orientation = *orientation;
}

void SetVehicleTurnRate(Vehicle *vehicle, f32 x, f32 y, f32 z) {
    vehicle->body.state.angularVelocity.x = x;
    vehicle->body.state.angularVelocity.y = y;
    vehicle->body.stepStates[1].angularVelocity.x = x;
    vehicle->body.stepStates[1].angularVelocity.y = y;
    vehicle->body.state.angularVelocity.z = z;
    vehicle->body.stepStates[0].angularVelocity.x = x;
    vehicle->body.stepStates[0].angularVelocity.y = y;
    vehicle->body.stepStates[1].angularVelocity.z = z;
    vehicle->body.stepStates[0].angularVelocity.z = z;
}

void InitVehicleColorAndPhysics(Vehicle *vehicle) {
    SetMaterialColor(
        vehicle->model, vehicle->color[0] >> 3, vehicle->color[1] >> 3, vehicle->color[2] >> 3);
    InitVehiclePhysics(vehicle);
}

void SetVehicleModelSlot(Vehicle *vehicle, s32 slot) {
    vehicle->model = &g_aVehicleModels[slot];
    InitVehicleColorAndPhysics(vehicle);
}

void InstallVehicleModel(Vehicle *vehicle, s32 slot, s32 car) {
    LoadOrCopyVehicleModel(slot, car, 0);
    SetVehicleModelSlot(vehicle, slot);
}

void LoadVehicleModel(VehicleModelLoad *load, s32 slot, s32 car, s32 buffer) {
    CarModel *model = &g_aCarModels[car];

    model->modelSize = DmaReadAssetSize(model->modelRom);
    InitDmaRequest(&load->job, &g_aDmaWorkBuffers[buffer * DMA_WORK_BUFFER_SIZE]);
    DecompressFromRom(&g_aVehicleModels[slot], model->modelRom, &load->job);
    load->slot = slot;
    load->car = car;
}

s32 IsVehicleModelLoaded(VehicleModelLoad *load) {
    return load->job.remaining == 0;
}

u32 PollVehicleModelDma(VehicleModelLoad *load) {
    if (load->job.remaining != 0) {
        DecompressFromRom(
            &g_aVehicleModels[load->slot], g_aCarModels[load->car].modelRom, &load->job);
        if (load->job.remaining == 0) {
            g_adwVehicleModelSlotCars[load->slot] = load->car;
            FixupVehicleModelPointers(&g_aVehicleModels[load->slot]);
        }
    }
    return load->job.remaining;
}

void ResetVehicleState(Vehicle *vehicle) {
    CarPhysicsSpec *spec;
    s32 i;

    /* The frames of the vehicle and its cameras, all but camera 4, and of its wheels. */
    FinishFrameMatrix(vehicle->matrix);
    FinishFrameMatrix((f32(*)[4]) & vehicle->cameras[VEHICLE_CAMERA_COCKPIT]);
    vehicle->cameras[VEHICLE_CAMERA_COCKPIT].fov = DEFAULT_CAMERA_FOV;
    FinishFrameMatrix((f32(*)[4]) & vehicle->cameras[VEHICLE_CAMERA_CHASE]);
    vehicle->cameras[VEHICLE_CAMERA_CHASE].fov = DEFAULT_CAMERA_FOV;
    FinishFrameMatrix((f32(*)[4]) & vehicle->cameras[VEHICLE_CAMERA_BUMPER]);
    vehicle->cameras[VEHICLE_CAMERA_BUMPER].fov = DEFAULT_CAMERA_FOV;
    FinishFrameMatrix((f32(*)[4]) & vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE]);
    vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].fov = DEFAULT_CAMERA_FOV;
    FinishFrameMatrix((f32(*)[4]) & vehicle->cameras[VEHICLE_CAMERA_MIRROR]);
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].fov = DEFAULT_CAMERA_FOV;
    vehicle->model = NULL;
    vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_COCKPIT];
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        FinishFrameMatrix(vehicle->wheelMatrices[i]);
    }
    SetVehicleVelocity(vehicle, 0.0f, 0.0f, 0.0f);
    vehicle->section = NULL;
    vehicle->pointIndex = 0;
    vehicle->previousCamera = NULL;

    /* The gearbox and engine of the car. */
    spec = &g_aCarPhysicsSpecs[vehicle->carModel];
    memcpy(vehicle->gearRatios, spec->gearRatios, sizeof(vehicle->gearRatios));
    for (i = 0; i < 4; i++) {
        vehicle->torqueCurve[i] = spec->torqueCurve[i];
    }
    vehicle->finalDrive = spec->finalDrive;
    vehicle->transmission = vehicle->chosenTransmission;
    vehicle->gearCount = spec->gearCount;
    vehicle->frontWheelDrive = spec->frontWheelDrive;
    for (i = 0; i < 4; i++) {
        vehicle->tuning[i] = spec->tuning[i];
    }
    vehicle->tableIndex = spec->tableIndex;
}

void BindPlayerInputToVehicle(Vehicle *vehicle) {
    vehicle->input = &g_aPlayerInputState[vehicle - g_aVehicles];
    guMtxIdent(&vehicle->transformMtx);
}
