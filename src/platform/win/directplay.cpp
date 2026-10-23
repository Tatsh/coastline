#include "platform/win/directplay.h"

#include <windows.h>

#include "platform/win/boot/app.h"

void InitializeDirectPlayForDesktopWindow(void) {
    // The window is looked up and discarded.
    GetDesktopWindow();
    InitializeDirectPlay();
}
