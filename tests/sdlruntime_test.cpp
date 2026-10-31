#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <utility>

#include <SDL3/SDL.h>

#include "common/racephases.h"
#include "common/session.h"
#include "common/settings.h"
#include "common/vehicledeform.h"
#include "runtimebuffers.h"

namespace {

void Check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "Failed. " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

template <typename T, std::size_t Size>
void CheckAndChange(T (&values)[Size]) {
    Check(std::ranges::all_of(values, [](T value) { return value == 0; }),
          "a fresh runtime clears every array element");
    std::fill_n(values, Size, T{1});
}

} // namespace

int main() {
    using coastline::EngineRuntime;
    Check(SDL_Init(0), "SDL properties start");
    for (int run = 0; run < 100; ++run) {
        Check(EngineRuntime::Active() == nullptr, "the previous runtime is detached");
        {
            EngineRuntime runtime;
            Check(EngineRuntime::Active() == &runtime, "the runtime is active in its scope");
            Check(GetSdlFrontEndState() == &runtime.FrontEnd() &&
                      GetSdlSkidMarkState() == &runtime.SkidMarks() &&
                      GetSdlSnapshotUsage() == &runtime.SnapshotUsage() &&
                      GetSdlRaceStarted() == &runtime.RaceStarted() &&
                      GetSdlDentLimit() == &runtime.DentLimit(),
                  "common code accesses the runtime's storage");
            const std::array<std::pair<int *, int>, 15> settings{{
                {&g_dwChosenCar, 0},
                {&g_dwInNetworkCarSetup, 0},
                {&g_dwFrontEndAtRootMenu, 0},
                {&g_dwSelectedTransmission, 0},
                {&g_dwTireMenuIndex, 1},
                {&g_dwSuspensionMenuIndex, 1},
                {&g_dwCarMenuIndex, 1},
                {&g_dwTotalLapsSetting, 3},
                {&g_dwAccumUnlockedCarMask, 0},
                {&g_dwAccumUnlockedTrackMask, 0},
                {&g_dwHandlingTypeMenuIndex, 4},
                {&g_dwRunBenchmark, 0},
                {&g_dwNetworkPlayMode, 0},
                {&g_dwNullStubEnable, 1},
                {&g_bRaceStartReached, 0},
            }};
            for (const auto &[value, expected] : settings) {
                Check(*value == expected, "a fresh runtime restores each initial setting");
            }
            for (const auto &[value, expected] : settings) {
                *value = expected + 77;
            }
            Check(g_wVehicleDeformMaxDent == 0x100, "the dent limit matches the PC release");
            g_wVehicleDeformMaxDent = 7;
            CheckAndChange(g_szConfigFilePath);
            CheckAndChange(g_szSeasonSaveFileName);
            CheckAndChange(g_szTimeAttackSaveFileName);
            CheckAndChange(g_abFrameSnapshotInUse);
            CheckAndChange(g_anSkidMarkHead);
            CheckAndChange(g_anSkidMarkTail);
            static_assert(sizeof(g_szConfigFilePath) == 4096);
            static_assert(sizeof(g_szSeasonSaveFileName) == FRONT_END_SAVE_PATH_SIZE);
            static_assert(sizeof(g_abFrameSnapshotInUse) == FRAME_SNAPSHOT_COUNT * sizeof(int));
            static_assert(sizeof(g_anSkidMarkHead) == SKID_MARK_TRACK_COUNT * sizeof(int));
        }
        Check(EngineRuntime::Active() == nullptr, "destruction detaches the runtime");
    }
    SDL_Quit();
    return EXIT_SUCCESS;
}
