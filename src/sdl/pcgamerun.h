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
 * @param pcMusic Whether the music is the CD audio of the PC release, from FLAC files, rather
 * than the modules of the cartridge.
 * @return Whether the game is ready.
 */
bool StartPcGame(TlRenderer &renderer,
                 const std::filesystem::path &data,
                 const std::filesystem::path &state,
                 bool pcMusic);

/**
 * Run one frame of the state machine of the game, as the message loop of the PC release does when
 * no message waits.
 *
 * @param capture The PNG image to save the frame as, or null for no image.
 * @return Whether the game continues.
 */
bool RunPcGameFrame(const std::filesystem::path *capture = nullptr);

/**
 * @return The offset in the USA cartridge of the music module for the state of the game, the
 * module of the track in a race and the module of the menus elsewhere, or 0 when the music of the
 * PC release plays instead.
 */
std::uint32_t PcGameMusicModule();

} // namespace coastline
