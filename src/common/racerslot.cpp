#include "common/racerslot.h"

#include <cstdlib>

#include "common/ghost.h"
#include "common/settings.h"
#include "common/vehicleaccess.h"

RacerSlot g_aRacerSlots[RACER_SLOTS];

void RacerSlot::UpdateRemoteVehicleFrame(void) {
    if (vehicle == nullptr) {
        return;
    }
    if (g_dwNetworkPlayMode != 0 &&
        !ApplyRemoteVehicleState(vehicle, VEHICLE_GET(vehicle, Index))) {
        return;
    }
    vehicle->TransformAllWheelMatrices();
    vehicle->UpdateCameraSystem();
}

void RacerSlot::InitRacerSlot(void) {
    ::InitRacerSlot(this);
}

void RacerSlot::FreeEnemyCarColorPanels(void) {
    for (s32 index = 0; index < colorPanelCount; index++) {
        free(colorPanels[index]);
        colorPanels[index] = nullptr;
    }
    free(colorPanels);
    colorPanels = nullptr;
    colorPanelCount = 0;
}
