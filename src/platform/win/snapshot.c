#include "platform/win/snapshot.h"

#include <stdbool.h>

int g_bFrameSnapshotsInitialized;

void InvalidateFrameSnapshotSlots(void) {
    if (g_bFrameSnapshotsInitialized) {
        g_bFrameSnapshotsInitialized = false;
    }
}
