#include "racerslot.h"

#include <cstdlib>

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
