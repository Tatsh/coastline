#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "freecamera.h"

namespace {

bool Close(float a, float b) {
    return std::abs(a - b) < 0.0001f;
}

} // namespace

int main() {
    constexpr std::array<float, 16> identity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    coastline::FreeCamera camera;
    if (camera.View() != identity) {
        return EXIT_FAILURE;
    }
    camera.Move(0.1f, {10.0f, 20.0f, 30.0f}, {});
    const auto moved = camera.View();
    if (!Close(moved[12], -2.0f) || !Close(moved[13], -3.0f) || !Close(moved[14], 1.0f)) {
        return EXIT_FAILURE;
    }
    camera.Reset();
    camera.Move(0.0f, {}, {-1.5707963f, 0.0f});
    camera.Move(0.1f, {10.0f, 0.0f, 0.0f}, {});
    const auto rotated = camera.View();
    if (!Close(rotated[12], 0.0f) || !Close(rotated[14], 1.0f) || !Close(rotated[2], -1.0f)) {
        return EXIT_FAILURE;
    }
    camera.Reset();
    camera.Move(10.0f, {10.0f, 0.0f, 0.0f}, {0.0f, 100.0f});
    const auto limited = camera.View();
    for (const auto value : limited) {
        if (!std::isfinite(value)) {
            return EXIT_FAILURE;
        }
    }
    if (!Close(limited[14], 1.0f)) {
        return EXIT_FAILURE;
    }
    camera.Reset();
    if (camera.View() != identity) {
        return EXIT_FAILURE;
    }
    std::cout << "Free camera movement, orientation, elapsed-time limit, and reset pass.\n";
    return EXIT_SUCCESS;
}
