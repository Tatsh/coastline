#pragma once

#include "common/frontendstate.h"
#include "common/snapshot.h"
#include "common/trackskidmarks.h"

namespace coastline {

/** Front-end choices and race buffers for one active engine runtime. */
class EngineRuntime {
public:
    /** Usage counters indexed by the race engine's frame-snapshot slot. */
    using SnapshotSlots = int[FRAME_SNAPSHOT_COUNT];

    /** Register the sole active runtime and initialise its front-end defaults. */
    EngineRuntime();
    /** Unregister the active runtime before its race storage is destroyed. */
    ~EngineRuntime();
    /** Copying is disabled because only one runtime can be active. */
    EngineRuntime(const EngineRuntime &) = delete;
    /** Assignment is disabled because active runtime storage has a stable address. */
    EngineRuntime &operator=(const EngineRuntime &) = delete;

    /** @return The active runtime, or null outside its lifetime. */
    static EngineRuntime *Active();

    /** @return Mutable front-end choices for this runtime. */
    SdlFrontEndState &FrontEnd() {
        return m_frontEnd;
    }
    /** @return The skid-mark pool used by this runtime's race. */
    SdlSkidMarkState &SkidMarks() {
        return m_skidMarks;
    }
    /** @return Mutable usage counters for the frame-snapshot pool. */
    SnapshotSlots &SnapshotUsage() {
        return m_snapshotUsage;
    }
    /** @return The race-start flag shared with the engine's C interface. */
    int &RaceStarted() {
        return m_raceStarted;
    }
    /** @return The engine's maximum dent displacement, initially 0x100. */
    s16 &DentLimit() {
        return m_dentLimit;
    }

private:
    SdlFrontEndState m_frontEnd{};
    SdlSkidMarkState m_skidMarks{};
    SnapshotSlots m_snapshotUsage{};
    int m_raceStarted = 0;
    s16 m_dentLimit = 0x100;
};

} // namespace coastline
