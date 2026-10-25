#include "platform/win/boot/app.h"

void DispatchGameStateFrame(void) {
    g_apfnGameStateHandlers[g_dwGameState]();
}
