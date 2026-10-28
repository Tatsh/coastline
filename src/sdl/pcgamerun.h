#pragma once

#include <filesystem>

#include "tlrenderer.h"

namespace coastline {

/**
 * Prepare the game code of the PC release as RallyMain() of BRD3D.dll does before its loop, with
 * the renderer of the SDL game as its device.
 *
 * @param renderer The renderer the device draws with.
 * @param data The data directory.
 * @param state The state directory.
 * @return Whether the game is ready.
 */
bool StartPcGame(TlRenderer &renderer,
                 const std::filesystem::path &data,
                 const std::filesystem::path &state);

/**
 * Run one frame of the state machine of the game, as the message loop of the PC release does when
 * no message waits.
 *
 * @param capture The PNG image to save the frame as, or null for no image.
 * @return Whether the game continues.
 */
bool RunPcGameFrame(const std::filesystem::path *capture = nullptr);

} // namespace coastline
