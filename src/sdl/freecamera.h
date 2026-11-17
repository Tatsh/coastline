#pragma once

/**
 * @file
 * The free camera the user flies through every screen of the game.
 */

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
    /**
     * Build the view transform of the camera from its position, yaw, and pitch.
     *
     * @return The column-major eye-space view transform, identity after Reset().
     */
    std::array<float, 16> View() const;

private:
    std::array<float, 3> m_position{}; // In eye-space scene units.
    float m_yaw = 0.0f;                // The turn about the vertical axis in radians.
    float m_pitch = 0.0f;              // In radians, clamped to plus or minus 1.55.
};

} // namespace coastline
