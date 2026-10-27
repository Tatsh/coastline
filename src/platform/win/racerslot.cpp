#include "racerslot.h"

#include <cstdlib>

#include "platform/win/ghost.h"
#include "platform/win/settings.h"

void RacerSlot::UpdateRemoteVehicleFrame(void) {
    if (vehicle == nullptr) {
        return;
    }
    if (g_dwNetworkPlayMode != 0 && !ApplyRemoteVehicleState(vehicle, vehicle->index)) {
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
