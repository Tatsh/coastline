#include "runtimebuffers.h"

#include <cassert>

#include <SDL3/SDL.h>

#include "common/assetnames.h"
#include "common/racephases.h"
#include "common/vehicledeform.h"

namespace coastline {

namespace {
constexpr const char *ENGINE_RUNTIME_PROPERTY = "coastline.engine-runtime";
} // namespace

EngineRuntime::EngineRuntime() {
    assert(Active() == nullptr);
    m_frontEnd.tires = 1;
    m_frontEnd.suspension = 1;
    m_frontEnd.car = 1;
    m_frontEnd.laps = 3;
    m_frontEnd.handling = 4;
    m_frontEnd.frontEndEnabled = 1;
    SDL_SetPointerProperty(SDL_GetGlobalProperties(), ENGINE_RUNTIME_PROPERTY, this);
}

EngineRuntime::~EngineRuntime() {
    SDL_ClearProperty(SDL_GetGlobalProperties(), ENGINE_RUNTIME_PROPERTY);
}

EngineRuntime *EngineRuntime::Active() {
    return static_cast<EngineRuntime *>(
        SDL_GetPointerProperty(SDL_GetGlobalProperties(), ENGINE_RUNTIME_PROPERTY, nullptr));
}

} // namespace coastline

extern "C" {

const char g_szEmptyUiString[1] = {0};

SdlFrontEndState *GetSdlFrontEndState(void) {
    auto *runtime = coastline::EngineRuntime::Active();
    assert(runtime != nullptr);
    return &runtime->FrontEnd();
}

SdlSkidMarkState *GetSdlSkidMarkState(void) {
    auto *runtime = coastline::EngineRuntime::Active();
    assert(runtime != nullptr);
    return &runtime->SkidMarks();
}

int (*GetSdlSnapshotUsage(void)) [FRAME_SNAPSHOT_COUNT] {
    auto *runtime = coastline::EngineRuntime::Active();
    assert(runtime != nullptr);
    return &runtime->SnapshotUsage();
}

int *GetSdlRaceStarted(void) {
    auto *runtime = coastline::EngineRuntime::Active();
    assert(runtime != nullptr);
    return &runtime->RaceStarted();
}

s16 *GetSdlDentLimit(void) {
    auto *runtime = coastline::EngineRuntime::Active();
    assert(runtime != nullptr);
    return &runtime->DentLimit();
}
}
