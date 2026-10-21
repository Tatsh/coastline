#include "vehicle.h"

#include <PR/gu.h>
#include <PR/os_libc.h>

#include "camera.h"
#include "game.h"
#include "mathutil.h"
#include "physics.h"
#include "race.h"
#include "timing.h"
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
#define GRID_CELL 32
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

/* The share of the drive the rear wheels take with rear-wheel drive, and the front ones. */
#define REAR_DRIVE_SHARE 2.0f
#define FRONT_DRIVE_SHARE 0.0f

/* The drive of a front wheel below which the brakes act on the front wheels as well. */
#define FRONT_BRAKE_DRIVE_LIMIT 1.0e-4f

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
    s32 i, p, count;
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
        if (!SegmentCrossesLine2D(&vehicle->section->points[vehicle->pointIndex].left,
                                  &vehicle->section->points[vehicle->pointIndex].right,
                                  &vehicle->previousPosition,
                                  &vehicle->frame.position) &&
            !SegmentCrossesLine2D(&vehicle->section->points[vehicle->pointIndex + 1].left,
                                  &vehicle->section->points[vehicle->pointIndex + 1].right,
                                  &vehicle->previousPosition,
                                  &vehicle->frame.position) &&
            Vec3fDistance(&vehicle->frame.position,
                          &vehicle->section->points[vehicle->pointIndex].position) <
                POINT_KEEP_DISTANCE) {
            section = vehicle->section;
            p = vehicle->pointIndex;
            goto found;
        }
    }

    /* The nearest point ahead of the vehicle, of the sections of its cell near it on the route. */
    best = g_LoadedTrack.maxX - g_LoadedTrack.minX;
    best *= best;
    for (i = 0; i < g_LoadedTrack.sectionCount; i++) {
        if (vehicle->index >= (s32)g_dwNumLocalPlayers && g_dwGameMode != GAME_MODE_TIME_ATTACK &&
            (g_LoadedTrack.sections[i]->flags & TRACK_SECTION_ALTERNATE)) {
            continue;
        }
        section = g_LoadedTrack.sections[i];
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
        count = section->pointCount;
        for (p = 0; p < count; p++) {
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
    vehicle->trackDirection.x = direction.x;
    vehicle->trackDirection.y = direction.y;
    vehicle->trackDirection.z = direction.z;
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
    if (vehicle->messageTime != 0) {
        vehicle->messageTime -= g_fFrameDeltaTime;
        if (vehicle->messageTime <= 0.0f) {
            vehicle->messageTime = 0.0f;
            vehicle->message = NULL;
        }
    } else if (vehicle->subMessageTime != 0) {
        vehicle->subMessageTime -= g_fFrameDeltaTime;
        if (vehicle->subMessageTime <= 0.0f) {
            vehicle->subMessageTime = 0.0f;
            vehicle->subMessage = NULL;
        }
    }
}

void CheckWrongWayWarning(Vehicle *vehicle) {
    if (g_LoadedTrack.rootSection != NULL) {
        if (vehicle->lap < g_dwTotalLaps && !vehicle->flyMode &&
            (f64)Vec3fDot(&vehicle->trackDirection, &vehicle->frame.forward) < 0.0) {
            vehicle->wrongWayFrames++;
            if (vehicle->wrongWayFrames >= WRONG_WAY_FRAMES &&
                (vehicle->wrongWayFrames & WRONG_WAY_FLASH) == WRONG_WAY_FLASH) {
                if (vehicle->message == NULL) {
                    vehicle->message = sWrongWay;
                    vehicle->subMessage = NULL;
                    vehicle->messageTime = WRONG_WAY_TIME;
                }
            } else if (vehicle->message == sWrongWay) {
                vehicle->subMessage = NULL;
                vehicle->message = NULL;
            }
        } else {
            vehicle->wrongWayFrames = 0;
        }
    }
}

/* The original tests the unsigned cells against zero as well. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
void UpdateVehicleGridPosition(Vehicle *vehicle) {
    if (g_LoadedTrack.rootSection == NULL) {
        return;
    }
    vehicle->gridX = FloatToInt(vehicle->frame.position.x / GRID_CELL);
    vehicle->gridY = FloatToInt(vehicle->frame.position.y / GRID_CELL);
    if (vehicle->gridX < 0) {
        vehicle->gridX = 0;
    } else if (vehicle->gridX >= GRID_CELLS) {
        vehicle->gridX = GRID_CELLS - 1;
    }
    if (vehicle->gridY < 0) {
        vehicle->gridY = 0;
    } else if (vehicle->gridY >= GRID_CELLS) {
        vehicle->gridY = GRID_CELLS - 1;
    }
    UpdateVehicleTrackPosition(vehicle);
}
#pragma GCC diagnostic pop

s32 CastRayAgainstTrack(Vec3f *outPoint,
                        Vec3f *outNormal,
                        Vec3f *origin,
                        u16 *groundInstances,
                        s32 *groundInstanceCount,
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
                    groundNormal.x = triangle->normal.x;
                    groundNormal.y = triangle->normal.y;
                    groundNormal.z = triangle->normal.z;
                }
                if (groundCount < RAY_INSTANCES) {
                    groundInstances[groundCount++] = groundInstances[0];
                }
                groundInstances[0] = g_LoadedTrack.surfaces[triangle->index].instance + 1;
                if (below < RAY_NEARBY_DISTANCE) {
                    for (j = g_LoadedTrack.triangleNearbyInstances[triangle->index];
                         g_LoadedTrack.nearbyInstances[j] != 0;
                         j++) {
                        if (nearby < RAY_INSTANCES) {
                            nearbyInstances[nearby++] = nearbyInstances[0];
                        }
                        nearbyInstances[0] = g_LoadedTrack.nearbyInstances[j];
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
                    ceilingNormal.x = triangle->normal.x;
                    ceilingNormal.y = triangle->normal.y;
                    ceilingNormal.z = triangle->normal.z;
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
            outNormal->x = groundNormal.x;
            outNormal->y = groundNormal.y;
            outNormal->z = groundNormal.z;
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
            outNormal->x = ceilingNormal.x;
            outNormal->y = ceilingNormal.y;
            outNormal->z = ceilingNormal.z;
        }
    }
    *groundInstanceCount = 1;
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
                                                  &vehicle->groundInstanceCount,
                                                  vehicle->nearbyInstances,
                                                  &vehicle->nearbyInstanceCount,
                                                  &vehicle->groundDistance,
                                                  &vehicle->groundSurface);
        wheels[1]->steerAngle = 0.0f;
        wheels[0]->steerAngle = wheels[1]->steerAngle;
        UpdateVehicleEngineAndGearbox(vehicle);
        wheels[3]->steerAngle = vehicle->steerAngle;
        wheels[2]->steerAngle = vehicle->steerAngle;
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
            if (vehicle->rearWheelDrive) {
                wheels[0]->drive = -vehicle->throttle * REAR_DRIVE_SHARE;
                wheels[1]->drive = -vehicle->throttle * REAR_DRIVE_SHARE;
                wheels[2]->drive = -vehicle->throttle * FRONT_DRIVE_SHARE;
                wheels[3]->drive = -vehicle->throttle * FRONT_DRIVE_SHARE;
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
            if (drive < FRONT_BRAKE_DRIVE_LIMIT) {
                wheels[2]->brake = -vehicle->brake;
                wheels[3]->brake = -vehicle->brake;
            }
        }
        SimulateVehiclePhysicsSubstep(vehicle);
    }

    TransformAllWheelMatrices(vehicle);
    /* The speed changes while the first front wheel is on the ground. */
    if (vehicle->wheelBodies[1].contactFrames) {
        velocity = &vehicle->body.state.velocity;
        vehicle->speedKph = sqrtf(velocity->z * velocity->z +
                                  (velocity->x * velocity->x + velocity->y * velocity->y)) *
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
    vehicle->frame.position.x = x;
    vehicle->frame.position.y = y;
    vehicle->frame.position.z = z;
    vehicle->sprayOrigin.x = x;
    vehicle->sprayOrigin.y = y;
    vehicle->sprayOrigin.z = z;
    vehicle->body.state.position.x = x;
    vehicle->body.state.position.y = y;
    vehicle->body.state.position.z = z;
    vehicle->body.stepStates[1].position.x = x;
    vehicle->body.stepStates[1].position.y = y;
    vehicle->body.stepStates[1].position.z = z;
    vehicle->body.stepStates[0].position.x = x;
    vehicle->body.stepStates[0].position.y = y;
    vehicle->body.stepStates[0].position.z = z;
    QuaternionToMatrix(vehicle->body.orientation, &vehicle->body.state);
}

/* Copy the orientation of the body into both of its step states, a coordinate at a time. */
#define COPY_ORIENTATION_TO_STEPS(vehicle)                                                         \
    (vehicle)->body.stepStates[1].orientation.w = (vehicle)->body.state.orientation.w;             \
    (vehicle)->body.stepStates[0].orientation.w = (vehicle)->body.state.orientation.w;             \
    (vehicle)->body.stepStates[1].orientation.x = (vehicle)->body.state.orientation.x;             \
    (vehicle)->body.stepStates[0].orientation.x = (vehicle)->body.state.orientation.x;             \
    (vehicle)->body.stepStates[1].orientation.y = (vehicle)->body.state.orientation.y;             \
    (vehicle)->body.stepStates[0].orientation.y = (vehicle)->body.state.orientation.y;             \
    (vehicle)->body.stepStates[1].orientation.z = (vehicle)->body.state.orientation.z;             \
    (vehicle)->body.stepStates[0].orientation.z = (vehicle)->body.state.orientation.z

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
    COPY_ORIENTATION_TO_STEPS(vehicle);
    QuaternionToMatrix(vehicle->body.orientation, &vehicle->body.state);
}

void MatrixToQuaternion(f32 m[4][4], Quaternion *q) {
    if (0.0f <= m[0][0]) {
        if (0.0f <= m[1][1] + m[2][2]) {
            q->w = 1.0f + m[0][0] + m[1][1] + m[2][2];
            q->x = m[1][2] - m[2][1];
            q->y = m[2][0] - m[0][2];
            q->z = m[0][1] - m[1][0];
        } else {
            q->w = m[1][2] - m[2][1];
            q->x = 1.0f + m[0][0] - m[1][1] - m[2][2];
            q->y = m[0][1] + m[1][0];
            q->z = m[2][0] + m[0][2];
        }
    } else if (m[2][2] <= m[1][1]) {
        q->w = m[2][0] - m[0][2];
        q->x = m[0][1] + m[1][0];
        q->y = 1.0f - m[0][0] + m[1][1] - m[2][2];
        q->z = m[1][2] + m[2][1];
    } else {
        q->w = m[0][1] - m[1][0];
        q->x = m[2][0] + m[0][2];
        q->y = m[1][2] + m[2][1];
        q->z = 1.0f - m[0][0] - m[1][1] + m[2][2];
    }
    QuaternionNormalize(q);
}

void SetVehicleFrameMatrix(Vehicle *vehicle, f32 (*m)[4]) {
    memcpy(vehicle->matrix, m, sizeof(vehicle->matrix));
    MatrixToQuaternion(m, &vehicle->body.state.orientation);
    COPY_ORIENTATION_TO_STEPS(vehicle);
    QuaternionToMatrix(vehicle->body.orientation, &vehicle->body.state);
}

void SetVehicleVelocity(Vehicle *vehicle, f32 x, f32 y, f32 z) {
    Vec3f *placed = &vehicle->placedVelocity;

    vehicle->body.state.velocity.x = x;
    vehicle->body.state.velocity.y = y;
    vehicle->body.state.velocity.z = z;
    vehicle->body.stepStates[1].velocity.x = x;
    vehicle->body.stepStates[1].velocity.y = y;
    vehicle->body.stepStates[1].velocity.z = z;
    vehicle->body.stepStates[0].velocity.x = x;
    vehicle->body.stepStates[0].velocity.y = y;
    vehicle->body.stepStates[0].velocity.z = z;
    placed->x = x;
    placed->y = y;
    placed->z = z;
}

void RotateVehicleByEuler(Vehicle *vehicle, f32 yaw, f32 pitch, f32 roll) {
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
    QuaternionMultiply(&vehicle->body.state.orientation, &vehicle->body.state.orientation, &turn);
    sine = sinf(pitch);
    turn.w = cosf(pitch);
    turn.x = 0.0f;
    turn.y = sine;
    turn.z = 0.0f;
    QuaternionMultiply(&vehicle->body.state.orientation, &vehicle->body.state.orientation, &turn);
    sine = sinf(roll);
    turn.w = cosf(roll);
    turn.x = sine;
    turn.y = 0.0f;
    turn.z = 0.0f;
    QuaternionMultiply(&vehicle->body.state.orientation, &vehicle->body.state.orientation, &turn);
    QuaternionNormalize(&vehicle->body.state.orientation);
    COPY_ORIENTATION_TO_STEPS(vehicle);
}

void SetVehicleTurnRate(Vehicle *vehicle, f32 x, f32 y, f32 z) {
    vehicle->body.state.angularVelocity.x = x;
    vehicle->body.state.angularVelocity.y = y;
    vehicle->body.state.angularVelocity.z = z;
    vehicle->body.stepStates[1].angularVelocity.x = x;
    vehicle->body.stepStates[1].angularVelocity.y = y;
    vehicle->body.stepStates[1].angularVelocity.z = z;
    vehicle->body.stepStates[0].angularVelocity.x = x;
    vehicle->body.stepStates[0].angularVelocity.y = y;
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
    LoadOrCopyVehicleModel(slot, car, NULL);
    SetVehicleModelSlot(vehicle, slot);
}

void LoadVehicleModel(VehicleModelLoad *load, s32 slot, s32 car, s32 buffer) {
    g_aCarModels[car].modelSize = DmaReadAssetSize(g_aCarModels[car].modelRom);
    InitDmaRequest(&load->job, &g_aDmaWorkBuffers[buffer * DMA_WORK_BUFFER_SIZE]);
    DecompressFromRom(&g_aVehicleModels[slot], g_aCarModels[car].modelRom, &load->job);
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
    s32 car;

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
    vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_COCKPIT];
    vehicle->model = NULL;
    FinishFrameMatrix(vehicle->wheelMatrices[0]);
    FinishFrameMatrix(vehicle->wheelMatrices[1]);
    FinishFrameMatrix(vehicle->wheelMatrices[2]);
    FinishFrameMatrix(vehicle->wheelMatrices[3]);
    SetVehicleVelocity(vehicle, 0.0f, 0.0f, 0.0f);
    vehicle->section = NULL;
    vehicle->pointIndex = 0;
    vehicle->previousCamera = NULL;

    /* The gearbox and engine of the car. */
    car = vehicle->carModel;
    memcpy(vehicle->gearRatios, g_aCarPhysicsSpecs[car].gearRatios, sizeof(vehicle->gearRatios));
    vehicle->torqueCurve[0] = g_aCarPhysicsSpecs[car].torqueCurve[0];
    vehicle->torqueCurve[1] = g_aCarPhysicsSpecs[car].torqueCurve[1];
    vehicle->torqueCurve[2] = g_aCarPhysicsSpecs[car].torqueCurve[2];
    vehicle->torqueCurve[3] = g_aCarPhysicsSpecs[car].torqueCurve[3];
    vehicle->finalDrive = g_aCarPhysicsSpecs[car].finalDrive;
    vehicle->gearCount = g_aCarPhysicsSpecs[car].gearCount;
    vehicle->transmission = vehicle->chosenTransmission;
    vehicle->rearWheelDrive = g_aCarPhysicsSpecs[car].rearWheelDrive;
    vehicle->body.boxSize.x = g_aCarPhysicsSpecs[car].boxSize.x;
    vehicle->body.boxSize.y = g_aCarPhysicsSpecs[car].boxSize.y;
    vehicle->body.boxSize.z = g_aCarPhysicsSpecs[car].boxSize.z;
    vehicle->body.boxCenterZ = g_aCarPhysicsSpecs[car].boxCenterZ;
    vehicle->tableIndex = g_aCarPhysicsSpecs[car].tableIndex;
}

void BindPlayerInputToVehicle(Vehicle *vehicle) {
    vehicle->input = &g_aPlayerInputState[vehicle - g_aVehicles];
    guMtxIdent(&vehicle->sprayMtx);
}

/* The step of the physics of a frame, in seconds. */
#define PHYSICS_STEP (1.0f / 30.0f)

/* How far below where the physics hangs them the wheels are drawn. */
#define WHEEL_DRAW_DROP 0.254f

/* The front wheels are drawn steered twice as far as they steer, in degrees. */
#define TWO_RAD_TO_DEG 114.5915527f

#define PI_F 3.141592741f
#define TWO_PI_F 6.283185482f

/* The button layout that steers straight with the stick. */
#define STICK_STEERING_LAYOUT 4

/* The middle of the stick that does not steer. */
#define STICK_DEAD_ZONE 0.07f

/* The revolutions of the engine: idle, the least and the most, the change of the automatic
 * gearbox, and the most they change a frame. */
#define IDLE_RPM 800.0f
#define MIN_RPM 900.0f
#define MAX_RPM 8000.0f
#define SHIFT_DOWN_RPM 4000.0f
#define SHIFT_UP_RPM 6000.0f
#define MAX_RPM_CHANGE 400.0f

/* The sign of a value: 0, 1 or -1. */
static inline f32 SignOf(f32 x) {
    return x == 0.0f ? 0.0f : (0.0f < x ? 1.0f : -1.0f);
}

static inline f32 AbsOf(f32 x) {
    return x < 0.0f ? -x : x;
}

/* A coordinate of a vector by its axis. */
static inline f32 *VectorAxis(Vec3f *v, s32 axis) {
    /* The original indexes the coordinates as an array rather than choosing among them. */
    return &(&v->x)[axis];
}

/* Keep a velocity at a damped one while the damping does not turn it back, and stop it if it does.
 */
static inline void KeepUnreversed(f32 *velocity, f32 damped) {
    *velocity = SignOf(damped) == SignOf(*velocity) ? damped : 0.0f;
}

void SimulateVehiclePhysicsSubstep(Vehicle *vehicle) {
    RigidBody *body = &vehicle->body;
    RigidBodyState *state = &body->state;
    s32 i;
    s16 axis;

    AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
    /* The first pass: the springs, the drive of the wheels and the grip. */
    body->forces = &vehicle->forces[VEHICLE_FORCE_SPRING_0];
    body->wheels[0]->forces = &vehicle->forces[VEHICLE_FORCE_WHEEL_0];
    body->wheels[1]->forces = &vehicle->forces[VEHICLE_FORCE_WHEEL_2];
    body->wheels[2]->forces = &vehicle->forces[VEHICLE_FORCE_WHEEL_1];
    body->wheels[3]->forces = &vehicle->forces[VEHICLE_FORCE_WHEEL_3];
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        body->wheels[i]->forces->vector.x = 0.0f;
        body->wheels[i]->forces->vector.y = 0.0f;
        body->wheels[i]->forces->vector.z = 0.0f;
    }
    ComputeSpringForces(body);
    if (vehicle->skipDrive) {
        vehicle->skipDrive = 0;
    } else {
        vehicle->frontLoad = 0.0f;
        vehicle->rearLoad = 0.0f;
        vehicle->rearSliding = 0;
        DriveWheel(body, body->wheels[0], &vehicle->rearLoad, &vehicle->rearSliding, PHYSICS_STEP);
        DriveWheel(body, body->wheels[1], &vehicle->rearLoad, &vehicle->rearSliding, PHYSICS_STEP);
        DriveWheel(
            body, body->wheels[2], &vehicle->frontLoad, &vehicle->frontSliding, PHYSICS_STEP);
        DriveWheel(
            body, body->wheels[3], &vehicle->frontLoad, &vehicle->frontSliding, PHYSICS_STEP);
        vehicle->skipDrive = 0;
    }
    body->force.x = 0.0f;
    body->force.y = 0.0f;
    body->force.z = 0.0f;
    body->torque.x = 0.0f;
    body->torque.y = 0.0f;
    body->torque.z = 0.0f;
    ComputeVehicleForces(body);
    ApplyRigidBodyImpulse(state, body, PHYSICS_STEP);
    ApplySideGrip(body,
                  PHYSICS_STEP,
                  &vehicle->rearLoad,
                  &vehicle->frontLoad,
                  &vehicle->rearSliding,
                  &vehicle->frontSliding);
    ComputeDeltaQuaternion(state);

    /* The second pass: the dampers and the drag, which stop the velocities, not turn them back. */
    body->forces = &vehicle->forces[VEHICLE_FORCE_DAMPER_0];
    body->wheels[0]->forces = NULL;
    body->wheels[2]->forces = NULL;
    body->wheels[1]->forces = NULL;
    body->wheels[3]->forces = NULL;
    ComputeDragForce(body, &vehicle->forces[VEHICLE_FORCE_DRAG]);
    ComputeDampingForces(body);
    body->force.x = 0.0f;
    body->force.y = 0.0f;
    body->force.z = 0.0f;
    body->torque.x = 0.0f;
    body->torque.y = 0.0f;
    body->torque.z = 0.0f;
    ComputeVehicleForces(body);
    memcpy(&body->stepStates[1], state, sizeof(RigidBodyState));
    ApplyRigidBodyImpulse(&body->stepStates[1], body, PHYSICS_STEP);
    for (axis = 0; axis < 3; axis++) {
        KeepUnreversed(VectorAxis(&state->angularVelocity, axis),
                       *VectorAxis(&body->stepStates[1].angularVelocity, axis));
        KeepUnreversed(VectorAxis(&state->velocity, axis),
                       *VectorAxis(&body->stepStates[1].velocity, axis));
    }

    /* Move it through the frame against the track and the others, and hang the wheels. */
    memcpy(&body->stepStates[0], state, sizeof(RigidBodyState));
    ComputeDeltaQuaternion(&body->stepStates[0]);
    ComputeDeltaQuaternion(&body->stepStates[0]);
    StepVehicleCollisions(body);
    memcpy(state, &body->stepStates[1], sizeof(RigidBodyState));
    UpdateWheelHeights(body);
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        QuaternionToMatrix(body->wheels[i]->orientation, &body->wheels[i]->state);
    }
    AddTimingMarker(0, 0, 0x80, 0, 0xFF);
}

/* Place the matrix of a wheel at the wheel, in the frame of the vehicle. */
static inline void PlaceWheel(f32 out[4][4], RigidBody *body, RigidBody *wheel) {
    /* The original transforms the translation rows in place, as points. */
    Mtx4x4TransformPoint((Vec3f *)out[3], body->orientation, (Vec3f *)wheel->orientation[3]);
}

/* The turn of a front wheel: its roll, then its steering. */
static inline void SteerWheel(f32 out[4][4], RigidBody *wheel) {
    f32 roll[4][4], steer[4][4];

    guRotateF(roll, wheel->rollAngle, 0.0f, 1.0f, 0.0f);
    guRotateF(steer, wheel->steerAngle * TWO_RAD_TO_DEG, 0.0f, 0.0f, 1.0f);
    guMtxCatF(roll, steer, out);
}

void TransformAllWheelMatrices(Vehicle *vehicle) {
    RigidBody *body = &vehicle->body;
    f32 turn[4][4];
    s32 i;

    guRotateF(turn, body->rollAngle, 1.0f, 0.0f, 0.0f);
    guMtxCatF(turn, body->orientation, vehicle->matrix);
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        vehicle->wheelBodies[i].orientation[3][2] =
            vehicle->wheelBodies[i].orientation[3][2] + WHEEL_DRAW_DROP;
    }
    /* The rear wheels by their roll, then the front ones by their roll and steering. */
    guRotateF(turn, vehicle->wheelBodies[0].rollAngle, 0.0f, 1.0f, 0.0f);
    guMtxCatF(turn, body->orientation, vehicle->wheelMatrices[2]);
    PlaceWheel(vehicle->wheelMatrices[2], body, &vehicle->wheelBodies[0]);
    guRotateF(turn, vehicle->wheelBodies[2].rollAngle, 0.0f, 1.0f, 0.0f);
    guMtxCatF(turn, body->orientation, vehicle->wheelMatrices[3]);
    PlaceWheel(vehicle->wheelMatrices[3], body, &vehicle->wheelBodies[2]);
    SteerWheel(turn, &vehicle->wheelBodies[1]);
    guMtxCatF(turn, body->orientation, vehicle->wheelMatrices[1]);
    PlaceWheel(vehicle->wheelMatrices[1], body, &vehicle->wheelBodies[1]);
    SteerWheel(turn, &vehicle->wheelBodies[3]);
    guMtxCatF(turn, body->orientation, vehicle->wheelMatrices[0]);
    PlaceWheel(vehicle->wheelMatrices[0], body, &vehicle->wheelBodies[3]);
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        vehicle->wheelBodies[i].orientation[3][2] =
            vehicle->wheelBodies[i].orientation[3][2] - WHEEL_DRAW_DROP;
    }
}

/* The gain of SteerByStick() is read unset for an unknown steering mode, as in the original. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wuninitialized"

/* Steer the front wheels towards the stick, faster back towards the middle. */
static inline void SteerByStick(Vehicle *vehicle, f32 stick) {
    Vec3f local;
    f32 forward, kph, rate, lock, gain, most, least, target, current;
    s32 down, up;
    s16 share = 1, returning;

    /* The middle of the stick does not steer. */
    if (0.0f < stick) {
        stick = stick - STICK_DEAD_ZONE;
        if (stick < 0.0f) {
            stick = 0.0f;
        }
    } else {
        stick = stick + STICK_DEAD_ZONE;
        if (0.0f < stick) {
            stick = 0.0f;
        }
    }
    kph = vehicle->speedKph;
    /* The original computes the speed of the body and discards it. */
    sqrtf(vehicle->body.state.velocity.z * vehicle->body.state.velocity.z +
          (vehicle->body.state.velocity.x * vehicle->body.state.velocity.x +
           vehicle->body.state.velocity.y * vehicle->body.state.velocity.y));
    Mtx4fInverseRotate(&local, vehicle->body.orientation, &vehicle->body.state.velocity);
    forward = local.x;
    if (forward < 10.0f) {
        forward = 10.0f;
    }
    if (70.0f < forward) {
        forward = 70.0f;
    }
    if (kph < 140.0f) {
        kph = 140.0f;
    }
    if (340.0f < kph) {
        kph = 340.0f;
    }
    kph = kph - 140.0f;
    /* The original leaves the gain unset for a mode beyond the three. */
    switch (vehicle->steeringMode) {
    case 0:
        g_fSteerLock = 14.0f;
        g_fSteerRate = 0.01f;
        g_fSteerGain = 6.0f;
        g_fSteerLockLoss = 10.0f;
        gain = g_fSteerGain;
        g_wSteerGainSpeedShare = 0;
        share = g_wSteerGainSpeedShare;
        break;
    case 1:
        g_fSteerLock = 14.0f;
        g_fSteerRate = 0.01f;
        g_fSteerGain = 6.0f;
        g_fSteerLockLoss = 10.0f;
        gain = 3.0f;
        g_wSteerGainSpeedShare = 0;
        share = g_wSteerGainSpeedShare;
        break;
    case 2:
        g_fSteerLock = 17.0f;
        g_fSteerRate = 0.1f;
        g_fSteerGain = 16.0f;
        g_fSteerLockLoss = 14.0f;
        gain = g_fSteerGain;
        g_wSteerGainSpeedShare = 0;
        share = g_wSteerGainSpeedShare;
        break;
    }
    rate = g_fSteerRate;

    /* The lock narrows with the speed; a hard push takes all of it, a soft one the gain. */
    lock = g_fSteerLock - forward / 90.0f * g_fSteerLockLoss;
    if (AbsOf(stick) < 0.001f) {
        most = lock * PI_F / 180.0f;
        least = -lock * PI_F / 180.0f;
        target = 0.0f;
    } else if (stick < -0.75f) {
        most = lock * PI_F / 180.0f;
        least = -lock * PI_F / 180.0f;
        target = most;
    } else if (0.75f < stick) {
        least = -lock * PI_F / 180.0f;
        most = lock * PI_F / 180.0f;
        target = least;
    } else {
        gain = gain - (f32)share * (gain / 2.0f) * (kph / 200.0f);
        target = -stick * (gain * PI_F / 180.0f);
        most = lock * PI_F / 180.0f;
        least = -lock * PI_F / 180.0f;
    }
    if (most < target) {
        target = most;
    }
    if (target < least) {
        target = least;
    }

    /* Back towards the middle, or across it, the wheels turn a radian a frame. */
    current = vehicle->steerAngle;
    returning = SignOf(current) != SignOf(target);
    if (!returning) {
        returning = (0.0f < current && target < current) || (current < 0.0f && current < target);
    }
    if (current == 0.0f) {
        returning = 0;
        vehicle->steerReturn = 0;
        current = vehicle->steerAngle;
    }
    down = target < current;
    up = current < target;
    if (down && vehicle->steerReturn < 0) {
        returning = 1;
    }
    if (up && vehicle->steerReturn > 0) {
        returning = 1;
    }
    if (returning) {
        vehicle->steerReturn = down ? -1 : 1;
        rate = 1.0f;
        current = vehicle->steerAngle;
        if (SignOf(target) != SignOf(current)) {
            target = 0.0f;
        }
    } else {
        vehicle->steerReturn = 0;
        current = vehicle->steerAngle;
    }
    if (AbsOf(current - target) < rate) {
        vehicle->steerAngle = target;
    } else if (target < current) {
        vehicle->steerAngle = current - rate;
    } else {
        vehicle->steerAngle = current + rate;
    }
}

void UpdateVehicleEngineAndGearbox(Vehicle *vehicle) {
    PlayerInputState *input = vehicle->input;
    f32 stick = input->steering;
    f32 amount, root, previous, rpm, extra, gap, torque, change;
    u32 buttons;
    s32 gear;

    if (input->controllerLayout == STICK_STEERING_LAYOUT) {
        /* Steer straight from the stick, by a curve of it. */
        amount = AbsOf(stick);
        root = sqrtf(amount);
        vehicle->steerAngle =
            -(sqrtf(sqrtf(sqrtf(amount) * amount) * root) * SignOf(stick)) * 10.0f * PI_F / 180.0f;
    } else {
        SteerByStick(vehicle, stick);
    }

    previous = vehicle->engineRpm;
    if (vehicle->engineRpm < IDLE_RPM) {
        vehicle->engineRpm = IDLE_RPM;
    }
    if (vehicle->transmission) {
        /* The automatic gearbox changes down and up by the revolutions, not up in reverse. */
        if (vehicle->gear >= 2 && vehicle->engineRpm < SHIFT_DOWN_RPM) {
            vehicle->gear--;
        } else if (vehicle->gear < vehicle->gearCount && SHIFT_UP_RPM < vehicle->engineRpm &&
                   !(vehicle->input->buttons & INPUT_REVERSE)) {
            vehicle->gear++;
        }
    } else if (vehicle->gear > 0 && (vehicle->input->buttons & INPUT_GEAR_DOWN)) {
        TransferFlags(vehicle->input, INPUT_GEAR_DOWN);
        vehicle->gear--;
    } else if (vehicle->gear < vehicle->gearCount && (vehicle->input->buttons & INPUT_GEAR_UP) &&
               !(vehicle->input->buttons & INPUT_REVERSE)) {
        TransferFlags(vehicle->input, INPUT_GEAR_UP);
        vehicle->gear++;
    }
    buttons = vehicle->input->buttons;

    /* The torque of the engine, with a push for the second in arcade by its gap to the first. */
    extra = 0.0f;
    rpm = vehicle->engineRpm;
    if (g_dwGameMode == GAME_MODE_ARCADE && vehicle->racePosition == 1) {
        gap = AbsOf(g_aVehicles[vehicle->index ^ 1].distance - vehicle->distance) * 0.5f;
        if (gap < 18.0f) {
            extra = 0.0f;
        } else {
            extra = gap - 18.0f;
            if (30.0f < extra) {
                extra = 30.0f;
            }
        }
    }
    torque = vehicle->torqueCurve[0] * rpm * rpm * rpm + vehicle->torqueCurve[1] * rpm * rpm +
             vehicle->torqueCurve[2] * rpm + vehicle->torqueCurve[3] + extra;
    if (!vehicle->transmission) {
        torque = (f32)((f64)torque * 1.03);
    }
    if (!(buttons & INPUT_ACCELERATE)) {
        torque = 0.0f;
    }
    vehicle->throttle = torque * 7.0f;
    if (vehicle->racer->status & RACER_HELD) {
        vehicle->gear = 0;
        gear = 0;
    } else {
        gear = vehicle->gear;
        if (gear == 0) {
            vehicle->gear = 1;
            gear = 1;
        }
    }
    if (gear) {
        /* In gear the engine turns with the first front wheel. */
        vehicle->engineRpm =
            vehicle->gearRatios[gear] *
            (-(vehicle->wheelBodies[1].spinRate / TWO_PI_F) * 60.0f * vehicle->finalDrive);
    } else {
        if (vehicle->input->buttons & INPUT_ACCELERATE) {
            vehicle->engineRpm = vehicle->engineRpm + 300.0f;
        } else {
            vehicle->engineRpm = vehicle->engineRpm - 200.0f;
        }
        vehicle->throttle = 0.0f;
    }

    /* The revolutions stay in range and change no more than a step a frame. */
    if (vehicle->engineRpm < 0.0f) {
        vehicle->engineRpm = -vehicle->engineRpm;
    }
    if (vehicle->engineRpm < MIN_RPM) {
        vehicle->engineRpm = MIN_RPM;
    }
    if (MAX_RPM < vehicle->engineRpm) {
        vehicle->engineRpm = MAX_RPM;
    }
    change = vehicle->engineRpm - previous;
    if (MAX_RPM_CHANGE < AbsOf(change)) {
        change = SignOf(change) * MAX_RPM_CHANGE;
    }
    vehicle->brake = 0.0f;
    vehicle->engineRpm = previous + change;
    if (vehicle->input->buttons & INPUT_BRAKE) {
        vehicle->brake = -140000.0f;
    }
}
#pragma GCC diagnostic pop

/* Set up the body of a wheel at rest at a place under the vehicle. */
static inline void InitWheelBody(RigidBody *wheel, f32 x, f32 y) {
    wheel->ground = NULL;
    wheel->kind = RIGID_BODY_NO_ROTATION;
    wheel->mass = 0.0f;
    wheel->size.x = 0.0f;
    wheel->size.y = 0.0f;
    wheel->size.z = 0.0f;
    wheel->suspension = 0.0f;
    wheel->spinRate = 0.0f;
    InitRigidBody(wheel);
    wheel->state.position.x = x;
    wheel->state.position.y = y;
    wheel->state.position.z = -0.1f;
    wheel->state.velocity.x = wheel->state.velocity.y = wheel->state.velocity.z = 0.0f;
    wheel->state.orientation.x = wheel->state.orientation.y = wheel->state.orientation.z = 0.0f;
    wheel->state.orientation.w = 1.0f;
    wheel->state.angularVelocity.x = wheel->state.angularVelocity.y =
        wheel->state.angularVelocity.z = 0.0f;
    QuaternionToMatrix(wheel->orientation, &wheel->state);
}

/* A force at a point of the body, which the physics sets each step. */
static inline void InitBodyForce(RigidBodyForce *force, f32 x, f32 y) {
    SetRigidBodyForce(force, 0.0f, 0.0f, 0.0f, x, y, 0.0f, RIGID_BODY_FORCE_BODY);
}

void InitVehiclePhysics(Vehicle *vehicle) {
    RigidBody *body = &vehicle->body;
    RigidBodyForce *forces = vehicle->forces;

    vehicle->skipDrive = 1;
    vehicle->gear = 0;
    vehicle->steerAngle = 0.0f;
    vehicle->engineRpm = 0.0f;
    if (vehicle->model != NULL) {
        body->kind = RIGID_BODY_BOX_1;
        body->size.y = 2.0f;
        body->mass = 1000.0f;
        body->size.x = 3.5f;
        body->size.z = 1.5f;
        InitRigidBody(body);
        body->state.position.x = 0.0f;
        body->state.position.y = 0.0f;
        body->state.position.z = 2.0f;
        body->state.velocity.x = 0.0f;
        body->state.velocity.y = 0.0f;
        body->state.velocity.z = 0.0f;
        body->state.orientation.x = 0.0f;
        body->state.orientation.y = 0.0f;
        body->state.orientation.z = 0.0f;
        body->state.angularVelocity.x = 0.0f;
        body->state.angularVelocity.y = 0.0f;
        body->state.angularVelocity.z = 0.0f;
        body->state.orientation.w = 1.0f;
        QuaternionToMatrix(body->orientation, &body->state);
        body->springRate = ((f32)vehicle->suspensionStiffness * 4.0f + 20.0f) * 16000.0f;
        body->damping = -3000.0f;

        /* The rear and the front wheel of the model, and those of the other side mirrored. */
        InitWheelBody(
            &vehicle->wheelBodies[0], vehicle->model->wheels[2].x, vehicle->model->wheels[2].y);
        InitWheelBody(
            &vehicle->wheelBodies[2], vehicle->model->wheels[2].x, -vehicle->model->wheels[2].y);
        InitWheelBody(
            &vehicle->wheelBodies[1], vehicle->model->wheels[1].x, vehicle->model->wheels[1].y);
        InitWheelBody(
            &vehicle->wheelBodies[3], vehicle->model->wheels[1].x, -vehicle->model->wheels[1].y);
        body->wheels[0] = &vehicle->wheelBodies[0];
        body->wheels[1] = &vehicle->wheelBodies[2];
        body->wheels[2] = &vehicle->wheelBodies[1];
        body->wheels[3] = &vehicle->wheelBodies[3];

        /* The springs over the wheels, then gravity. */
        InitBodyForce(&forces[VEHICLE_FORCE_SPRING_0], -1.5f, -1.0f);
        InitBodyForce(&forces[VEHICLE_FORCE_SPRING_2], -1.5f, 1.0f);
        InitBodyForce(&forces[VEHICLE_FORCE_SPRING_1], 1.5f, -1.0f);
        InitBodyForce(&forces[VEHICLE_FORCE_SPRING_3], 1.5f, 1.0f);
        SetRigidBodyForce(&forces[VEHICLE_FORCE_GRAVITY],
                          0.0f,
                          0.0f,
                          -15450.75f,
                          0.0f,
                          0.0f,
                          0.0f,
                          RIGID_BODY_FORCE_WORLD);
        SetRigidBodyForce(&forces[VEHICLE_FORCE_FRONT_WHEEL_GRAVITY],
                          0.0f,
                          0.0f,
                          -(10.5f * 9.81f),
                          0.0f,
                          0.0f,
                          0.0f,
                          RIGID_BODY_FORCE_WORLD);
        SetRigidBodyForce(&forces[VEHICLE_FORCE_REAR_WHEEL_GRAVITY],
                          0.0f,
                          0.0f,
                          -(10.5f * 9.81f),
                          0.0f,
                          0.0f,
                          0.0f,
                          RIGID_BODY_FORCE_WORLD);
        forces[VEHICLE_FORCE_SPRING_0].next = &forces[VEHICLE_FORCE_SPRING_2];
        forces[VEHICLE_FORCE_SPRING_2].next = &forces[VEHICLE_FORCE_SPRING_1];
        forces[VEHICLE_FORCE_SPRING_1].next = &forces[VEHICLE_FORCE_SPRING_3];
        forces[VEHICLE_FORCE_GRAVITY].next = NULL;
        forces[VEHICLE_FORCE_SPRING_3].next = &forces[VEHICLE_FORCE_GRAVITY];
        body->forces = &forces[VEHICLE_FORCE_SPRING_0];

        /* The drive of each wheel, then the gravity of its axle. */
        InitBodyForce(&forces[VEHICLE_FORCE_WHEEL_0], 0.0f, 0.0f);
        InitBodyForce(&forces[VEHICLE_FORCE_WHEEL_2], 0.0f, 0.0f);
        InitBodyForce(&forces[VEHICLE_FORCE_WHEEL_1], 0.0f, 0.0f);
        InitBodyForce(&forces[VEHICLE_FORCE_WHEEL_3], 0.0f, 0.0f);
        vehicle->wheelBodies[0].forces = &forces[VEHICLE_FORCE_WHEEL_0];
        vehicle->wheelBodies[2].forces = &forces[VEHICLE_FORCE_WHEEL_2];
        vehicle->wheelBodies[1].forces = &forces[VEHICLE_FORCE_WHEEL_1];
        forces[VEHICLE_FORCE_REAR_WHEEL_GRAVITY].next = NULL;
        forces[VEHICLE_FORCE_FRONT_WHEEL_GRAVITY].next = NULL;
        forces[VEHICLE_FORCE_WHEEL_0].next = &forces[VEHICLE_FORCE_REAR_WHEEL_GRAVITY];
        forces[VEHICLE_FORCE_WHEEL_2].next = &forces[VEHICLE_FORCE_REAR_WHEEL_GRAVITY];
        forces[VEHICLE_FORCE_WHEEL_1].next = &forces[VEHICLE_FORCE_FRONT_WHEEL_GRAVITY];
        forces[VEHICLE_FORCE_WHEEL_3].next = &forces[VEHICLE_FORCE_FRONT_WHEEL_GRAVITY];
        vehicle->wheelBodies[3].forces = &forces[VEHICLE_FORCE_WHEEL_3];

        /* The dampers over the wheels, then the drag. */
        InitBodyForce(&forces[VEHICLE_FORCE_DAMPER_0], -1.5f, -1.0f);
        InitBodyForce(&forces[VEHICLE_FORCE_DAMPER_2], -1.5f, 1.0f);
        InitBodyForce(&forces[VEHICLE_FORCE_DAMPER_1], 1.5f, -1.0f);
        InitBodyForce(&forces[VEHICLE_FORCE_DAMPER_3], 1.5f, 1.0f);
        SetRigidBodyForce(&forces[VEHICLE_FORCE_DRAG],
                          0.0f,
                          0.0f,
                          0.0f,
                          0.0f,
                          0.0f,
                          0.0f,
                          RIGID_BODY_FORCE_WORLD);
        forces[VEHICLE_FORCE_DAMPER_0].next = &forces[VEHICLE_FORCE_DAMPER_2];
        forces[VEHICLE_FORCE_DAMPER_2].next = &forces[VEHICLE_FORCE_DAMPER_1];
        forces[VEHICLE_FORCE_DRAG].next = NULL;
        vehicle->steerReturn = 0;
        vehicle->frontLoad = 0.0f;
        vehicle->rearLoad = 0.0f;
        forces[VEHICLE_FORCE_DAMPER_3].next = &forces[VEHICLE_FORCE_DRAG];
        body->tireType = vehicle->tireType;
        forces[VEHICLE_FORCE_DAMPER_1].next = &forces[VEHICLE_FORCE_DAMPER_3];
    }
    vehicle->rearSliding = 0;
    vehicle->frontSliding = 0;
}
