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

static const char sWrongWay[] = "%yyWRONG WAY";

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

void InstallVehicleModel(Vehicle *vehicle, s32 slot, s32 car) {
    LoadCar(slot, car, NULL);
    SetVehicleModelSlot(vehicle, slot);
#if TGR_REGION == TGR_REGION_JP
    ExtractVehicleBaseColor(vehicle, vehicle->model);
#endif
}

#if TGR_REGION == TGR_REGION_EU
/* The European release has a second, identical copy of InstallVehicleModel(). */
void InstallVehicleModelCopy(Vehicle *vehicle, s32 slot, s32 car) {
    LoadCar(slot, car, NULL);
    SetVehicleModelSlot(vehicle, slot);
}
#endif

#if TGR_REGION == TGR_REGION_JP
void LoadAndActivateTrackWithPaintColor(Vehicle *vehicle, s32 slot, s32 car) {
    LoadCar(slot, car, NULL);
    SetVehicleModelSlot(vehicle, slot);
    SetMaterialColor(
        vehicle->model, vehicle->color[0] >> 3, vehicle->color[1] >> 3, vehicle->color[2] >> 3);
}
#endif

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

void BindPlayerInputToVehicle(Vehicle *vehicle) {
    vehicle->input = &g_aPlayerInputState[vehicle - g_aVehicles];
    guMtxIdent(&vehicle->sprayMtx);
}

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
#define SIGN_OF(x) ((x) == 0.0f ? 0.0f : (0.0f < (x) ? 1.0f : -1.0f))

#define ABS_OF(x) ((x) < 0.0f ? -(x) : (x))

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
    if (ABS_OF(stick) < 0.001f) {
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
    returning = SIGN_OF(current) != SIGN_OF(target);
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
        if (SIGN_OF(target) != SIGN_OF(current)) {
            target = 0.0f;
        }
    } else {
        vehicle->steerReturn = 0;
        current = vehicle->steerAngle;
    }
    if (ABS_OF(current - target) < rate) {
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
        amount = ABS_OF(stick);
        root = sqrtf(amount);
        vehicle->steerAngle =
            -(sqrtf(sqrtf(sqrtf(amount) * amount) * root) * SIGN_OF(stick)) * 10.0f * PI_F / 180.0f;
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
        gap = ABS_OF(g_aVehicles[vehicle->index ^ 1].distance - vehicle->distance) * 0.5f;
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
    if (MAX_RPM_CHANGE < ABS_OF(change)) {
        change = SIGN_OF(change) * MAX_RPM_CHANGE;
    }
    vehicle->brake = 0.0f;
    vehicle->engineRpm = previous + change;
    if (vehicle->input->buttons & INPUT_BRAKE) {
        vehicle->brake = -140000.0f;
    }
}
#pragma GCC diagnostic pop
