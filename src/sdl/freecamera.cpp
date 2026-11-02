#include "freecamera.h"

#include <algorithm>
#include <cmath>

namespace coastline {

void FreeCamera::Reset() {
    m_position = {};
    m_yaw = 0.0f;
    m_pitch = 0.0f;
}

void FreeCamera::Move(float seconds,
                      const std::array<float, 3> &movement,
                      const std::array<float, 2> &rotation) {
    m_yaw = std::remainder(m_yaw + rotation[0], 6.2831853f);
    m_pitch = std::clamp(m_pitch + rotation[1], -1.55f, 1.55f);
    const float elapsed = std::clamp(seconds, 0.0f, 0.1f);
    const float yawSin = std::sin(m_yaw);
    const float yawCos = std::cos(m_yaw);
    const float pitchCos = std::cos(m_pitch);
    m_position[0] += elapsed * (-movement[0] * yawSin * pitchCos + movement[1] * yawCos);
    m_position[1] += elapsed * (movement[0] * std::sin(m_pitch) + movement[2]);
    m_position[2] += elapsed * (-movement[0] * yawCos * pitchCos - movement[1] * yawSin);
}

std::array<float, 16> FreeCamera::View() const {
    const float yawSin = std::sin(m_yaw);
    const float yawCos = std::cos(m_yaw);
    const float pitchSin = std::sin(m_pitch);
    const float pitchCos = std::cos(m_pitch);
    const std::array<float, 3> side{yawCos, 0.0f, -yawSin};
    const std::array<float, 3> up{yawSin * pitchSin, pitchCos, yawCos * pitchSin};
    const std::array<float, 3> back{yawSin * pitchCos, -pitchSin, yawCos * pitchCos};
    auto offset = [&](const auto &axis) {
        return -(axis[0] * m_position[0] + axis[1] * m_position[1] + axis[2] * m_position[2]);
    };
    return {side[0],
            up[0],
            back[0],
            0,
            side[1],
            up[1],
            back[1],
            0,
            side[2],
            up[2],
            back[2],
            0,
            offset(side),
            offset(up),
            offset(back),
            1};
}

} // namespace coastline
