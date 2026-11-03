#pragma once

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
    /** Assignment is disabled because the active context has a stable address. */
    RaceInputContext &operator=(const RaceInputContext &) = delete;

private:
    std::unique_ptr<RaceInput> m_input;
};

} // namespace coastline
