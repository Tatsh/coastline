#include <cstring>
#include <initializer_list>

#include "race.h"
#include "settings.h"
#include "snapshot.h"
#include "timer.h"
#include "vehiclecount.h"

namespace {
inline float Blend(float from, float to, long double share) {
    return (static_cast<long double>(to) - from) * share + from;
}

inline void BlendVector(Vec3f &out, const Vec3f &from, const Vec3f &to, long double share) {
    out.x = Blend(from.x, to.x, share);
    out.y = Blend(from.y, to.y, share);
    out.z = Blend(from.z, to.z, share);
}

inline void
BlendCamera(CameraFrame &out, const CameraFrame &from, const CameraFrame &to, long double share) {
    BlendVector(out.forward, from.forward, to.forward, share);
    BlendVector(out.side, from.side, to.side, share);
    BlendVector(out.up, from.up, to.up, share);
    BlendVector(out.position, from.position, to.position, share);
    out.forwardW = out.sideW = out.upW = 0.0f;
    out.positionW = 1.0f;
    out.fov = Blend(from.fov, to.fov, share);
}
} // namespace

inline void
Vehicle::InterpolateSnapshotState(const Vehicle &from, const Vehicle &to, long double share) {
    std::memcpy(this, &from, sizeof(*this));
    const auto repairCamera = [&](CameraFrame *camera) -> CameraFrame * {
        for (int i : {0, 5, 1, 2, 3}) {
            if (camera == &from.cameras[i]) {
                return &cameras[i];
            }
        }
        return nullptr;
    };
    if (from.activeCamera != nullptr) {
        activeCamera = repairCamera(from.activeCamera);
    }
    if (from.previousCamera != nullptr) {
        previousCamera = repairCamera(from.previousCamera);
    }
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 3; ++column) {
            matrix[row][column] = Blend(from.matrix[row][column], to.matrix[row][column], share);
        }
    }
    for (int wheel = 0; wheel < 4; ++wheel) {
        for (int column = 0; column < 3; ++column) {
            wheelMatrices[wheel][3][column] = Blend(
                from.wheelMatrices[wheel][3][column], to.wheelMatrices[wheel][3][column], share);
        }
    }
    /* Camera slot 4 is copied without interpolation or pointer repair. */
    for (int camera : {0, 1, 2, 3, 5}) {
        BlendCamera(cameras[camera], from.cameras[camera], to.cameras[camera], share);
    }
}

void FrameSnapshot::InterpolateFrom(const FrameSnapshot &from,
                                    const FrameSnapshot &to,
                                    long double share) {
    for (int i = 0; i < g_dwNumVehicles; ++i) {
        std::memcpy(&racers[i], &from.racers[i], sizeof(racers[i]));
        if (const auto vehicle = from.racers[i].GetVehicle()) {
            racers[i].SetVehicle(&vehicles[vehicle - from.vehicles]);
        }
    }
    for (int i = 0; i < (g_dwNumActiveVehicles == 0 ? 1 : g_dwNumActiveVehicles); ++i) {
        vehicles[i].InterpolateSnapshotState(from.vehicles[i], to.vehicles[i], share);
    }
    std::memcpy(&viewport, &from.viewport, sizeof(viewport));
    unreconstructed2c0e0 = from.unreconstructed2c0e0;
    std::memcpy(&particles, &from.particles, sizeof(particles));
}

int InterpolateFrameSnapshotsAndRender(int forceRender) {
    if (g_dwFrameSnapshotCurrentSlot < 0 || g_dwFrameSnapshotPreviousSlot < 0) {
        return 0;
    }
    const auto sequenceDelta =
        g_nLastInterpolatedSnapshot < 0 ?
            0 :
            static_cast<int>(g_aFrameSnapshots[g_dwFrameSnapshotCurrentSlot].GetSequence() -
                             g_aFrameSnapshots[g_nLastInterpolatedSnapshot].GetSequence());
    g_nLastInterpolatedSnapshot = g_dwFrameSnapshotCurrentSlot;
    g_dwFrameSnapshotBlendFromSlot = g_dwFrameSnapshotPreviousSlot;
    const auto &currentCamera = g_aFrameSnapshots[g_dwFrameSnapshotCurrentSlot].GetPrimaryCamera();
    const auto &previousCamera =
        g_aFrameSnapshots[g_dwFrameSnapshotPreviousSlot].GetPrimaryCamera();
    const long double x =
        static_cast<long double>(currentCamera.position.x) - previousCamera.position.x;
    const long double y =
        static_cast<long double>(currentCamera.position.y) - previousCamera.position.y;
    const long double z =
        static_cast<long double>(currentCamera.position.z) - previousCamera.position.z;
    const bool cameraJump = (x * x + y * y) + z * z > 100.0f;
    const auto now = GetHighResolutionTime();
    bool render = true;
    long double share;
    if (cameraJump || forceRender != 0) {
        share = 1.0f;
    } else if (g_dwFrameSnapshotBlendStartTime != 0) {
        share = static_cast<long double>(now - g_dwFrameSnapshotBlendStartTime) * 0.030000001f +
                g_fFrameSnapshotBlendCarry - sequenceDelta;
        if (sequenceDelta == 0 && share > 1.0f) {
            render = false;
        }
        if (!(share >= 0.0f)) {
            share = 0.0f;
        }
        if (share > 1.0f) {
            share = 1.0f;
        }
    } else {
        share = 0.0f;
    }
    if (!g_bInterpolateFrames) {
        share = 1.0f;
        render = true;
    }
    if (!render) {
        return 0;
    }
    for (auto &inUse : g_abFrameSnapshotInUse) {
        inUse = 0;
    }
    g_dwFrameSnapshotBlendStartTime = now;
    g_fFrameSnapshotBlendCarry = share;
    g_abFrameSnapshotInUse[g_nLastInterpolatedSnapshot] = 1;
    g_abFrameSnapshotInUse[g_dwFrameSnapshotBlendFromSlot] = 1;
    g_InterpolatedFrameSnapshot.InterpolateFrom(g_aFrameSnapshots[g_dwFrameSnapshotBlendFromSlot],
                                                g_aFrameSnapshots[g_nLastInterpolatedSnapshot],
                                                share);
    ProcessRacingScreen(FRAME_SNAPSHOT_COUNT);
    g_dwInterpolatedFrameCount = static_cast<unsigned>(g_dwInterpolatedFrameCount) + 1;
    return 1;
}
