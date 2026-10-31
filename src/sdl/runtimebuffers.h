#pragma once

#include "common/frontendstate.h"
#include "common/snapshot.h"
#include "common/trackskidmarks.h"

namespace coastline {

/** Front-end choices and race buffers for one active engine runtime. */
class EngineRuntime {
public:
    using SnapshotSlots = int[FRAME_SNAPSHOT_COUNT];

    EngineRuntime();
    ~EngineRuntime();
    EngineRuntime(const EngineRuntime &) = delete;
    EngineRuntime &operator=(const EngineRuntime &) = delete;

    /** @return The active runtime, or null outside its lifetime. */
    static EngineRuntime *Active();

    SdlFrontEndState &FrontEnd() {
        return m_frontEnd;
    }
    SdlSkidMarkState &SkidMarks() {
        return m_skidMarks;
    }
    SnapshotSlots &SnapshotUsage() {
        return m_snapshotUsage;
    }
    int &RaceStarted() {
        return m_raceStarted;
    }
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
