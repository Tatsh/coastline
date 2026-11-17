#pragma once

/**
 * @file
 * The front-end choices and race buffers of the active race engine runtime.
 */

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
    /**
     * Assignment is disabled because active runtime storage has a stable address.
     *
     * @return This runtime; the operator is deleted and never called.
     */
    EngineRuntime &operator=(const EngineRuntime &) = delete;

    /**
     * Get the registered runtime the engine's C interface reads.
     *
     * @return The active runtime, or null outside its lifetime.
     */
    static EngineRuntime *Active();

    /**
     * Get the menu selections the race reads when it starts.
     *
     * @return Mutable front-end choices for this runtime.
     */
    SdlFrontEndState &FrontEnd() {
        return m_frontEnd;
    }
    /**
     * Get the storage of the skid marks the race places on the track.
     *
     * @return The skid-mark pool used by this runtime's race.
     */
    SdlSkidMarkState &SkidMarks() {
        return m_skidMarks;
    }
    /**
     * Get the use count of each frame-snapshot slot.
     *
     * @return Mutable usage counters for the frame-snapshot pool.
     */
    SnapshotSlots &SnapshotUsage() {
        return m_snapshotUsage;
    }
    /**
     * Get the flag the engine sets once a race has started.
     *
     * @return The race-start flag shared with the engine's C interface.
     */
    int &RaceStarted() {
        return m_raceStarted;
    }
    /**
     * Get the largest distance a dent may move a vertex of a car.
     *
     * @return The engine's maximum dent displacement, initially 0x100.
     */
    s16 &DentLimit() {
        return m_dentLimit;
    }

private:
    SdlFrontEndState m_frontEnd{};   // The menu selections of this runtime.
    SdlSkidMarkState m_skidMarks{};  // The skid-mark pool of the race.
    SnapshotSlots m_snapshotUsage{}; // The use count of each frame-snapshot slot.
    int m_raceStarted = 0;           // Non-zero once the race has started.
    s16 m_dentLimit = 0x100;         // The largest dent displacement of a car vertex.
};

} // namespace coastline
