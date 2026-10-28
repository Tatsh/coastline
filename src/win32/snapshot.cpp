#include "win32/snapshot.h"

namespace {
/** @ghidraAddress BRD3D: 0x104afd00 */
int g_dwFrameSnapshotWriteSlot;
} // namespace

FrameSnapshot g_aFrameSnapshots[FRAME_SNAPSHOT_COUNT];
FrameSnapshot g_InterpolatedFrameSnapshot;

int g_bFrameSnapshotsInitialized;
int g_bFrameTimerReset = true;

void InitFrameSnapshotSlots(void) {
    int i;

    if (g_bFrameSnapshotsInitialized != 0) {
        return;
    }
    g_abFrameSnapshotInUse[0] = 0;
    g_dwFrameSnapshotBlendFromSlot = -1;
    g_abFrameSnapshotInUse[1] = 0;
    g_nLastInterpolatedSnapshot = -1;
    g_abFrameSnapshotInUse[2] = 0;
    g_dwFrameSnapshotWriteSlot = -1;
    g_abFrameSnapshotInUse[3] = 0;
    g_dwFrameSnapshotCurrentSlot = -1;
    g_dwFrameSnapshotPreviousSlot = -1;
    g_bFrameTimerReset = true;
    g_fFrameSnapshotBlendCarry = 0.0f;
    g_dwFrameSnapshotBlendStartTime = 0;
    g_dwInterpolatedFrameCount = 0;
    g_abFrameSnapshotInUse[4] = 0;
    for (i = 0; i < FRAME_SNAPSHOT_COUNT; i++) {
        g_aFrameSnapshots[i].SetSequence(0);
    }
    g_bFrameSnapshotsInitialized = true;
}

int AcquireNextFrameSnapshotSlot(void) {
    int writeSlot = g_dwFrameSnapshotWriteSlot;
    int oldestSlot = -1;
    unsigned oldestSequence = 0xffffffffu;
    unsigned nextSequence;
    int i;

    for (i = 0; i < FRAME_SNAPSHOT_COUNT; i++) {
        if (g_abFrameSnapshotInUse[i] == 0 && i != writeSlot &&
            g_aFrameSnapshots[i].GetSequence() <= oldestSequence) {
            oldestSlot = i;
            oldestSequence = g_aFrameSnapshots[i].GetSequence();
        }
    }
    nextSequence = writeSlot < 0 ? 0 : g_aFrameSnapshots[writeSlot].GetSequence();
    g_dwFrameSnapshotPreviousSlot = g_dwFrameSnapshotCurrentSlot;
    g_dwFrameSnapshotCurrentSlot = writeSlot;
    g_dwFrameSnapshotWriteSlot = oldestSlot;
    /* An exhausted pool still writes through the index of -1. */
    g_aFrameSnapshots[oldestSlot].SetSequence(nextSequence + 1);
    return oldestSlot;
}

void InvalidateFrameSnapshotSlots(void) {
    if (g_bFrameSnapshotsInitialized) {
        g_bFrameSnapshotsInitialized = false;
    }
}

int g_dwInterpolatedFrameCount;

int GetInterpolatedFrameCount(void) {
    return g_dwInterpolatedFrameCount;
}
