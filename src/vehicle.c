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
#include "vehiclestatus.h"

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
