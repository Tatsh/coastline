#pragma once

/**
 * @file
 * The owner of the controller input and haptic resources of the races.
 */

#include <memory>

namespace coastline {

class RaceInput;

/** Input history and haptic resources for one active input configuration. */
class RaceInputContext {
public:
    /** Register input history and haptic resources for the sole active race input context. */
    RaceInputContext();
    /** Stop haptics and unregister the active race input context. */
    ~RaceInputContext();
    /** Copying is disabled because race input callbacks address one active context. */
    RaceInputContext(const RaceInputContext &) = delete;
    /**
     * Assignment is disabled because the active context has a stable address.
     *
     * @return This context; the operator is deleted and never called.
     */
    RaceInputContext &operator=(const RaceInputContext &) = delete;

private:
    std::unique_ptr<RaceInput> m_input; // The button, steering, and force-feedback state.
};

} // namespace coastline
