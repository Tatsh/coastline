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
#include "vehicletrackpos.h"

/* The rows of a frame matrix whose last column FinishFrameMatrix() sets. */
#define MATRIX_ROWS 4

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
