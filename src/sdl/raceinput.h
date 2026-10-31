#pragma once

#include <memory>

namespace coastline {

class RaceInput;

/** Input history and haptic resources for one active input configuration. */
class RaceInputContext {
public:
    RaceInputContext();
    ~RaceInputContext();
    RaceInputContext(const RaceInputContext &) = delete;
    RaceInputContext &operator=(const RaceInputContext &) = delete;

private:
    std::unique_ptr<RaceInput> m_input;
};

} // namespace coastline
