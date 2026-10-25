#include <windows.h>

#include "platform/win/boot/app.h"

void UpdateAppActiveStateFromActivate(WPARAM wParam) {
    g_dwAppActive = wParam;
    g_dwActivateState = LOWORD(wParam);
    g_dwActivateMinimized = HIWORD(wParam);
    // The original calls NullStub() when the window goes inactive or is minimised.
    if (g_dwActivateState == WA_INACTIVE || g_dwActivateMinimized) {
        NullStub();
    }
}
