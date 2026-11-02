#pragma once

#include <array>

namespace coastline {

/** A movable camera relative to the normal eye space of menus and logos. */
class FreeCamera {
public:
    /** Restore the position and orientation of the normal scene camera. */
    void Reset();
    /**
     * Move independently of simulation time.
     * @param seconds Elapsed time, clamped to zero through 0.1 seconds.
     * @param movement Forward, right, and upward speeds in scene units per second.
     * @param rotation Yaw and pitch changes in radians.
     */
    void
    Move(float seconds, const std::array<float, 3> &movement, const std::array<float, 2> &rotation);
    /** @return The column-major eye-space view transform, identity after Reset(). */
    std::array<float, 16> View() const;

private:
    std::array<float, 3> m_position{};
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
};

} // namespace coastline
