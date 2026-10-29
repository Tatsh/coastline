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
 * @return The offset in the USA cartridge of the music module of the track of the race, or 0
 * while a race loads, before the instant replay, and when the music of the PC release plays
 * instead.
 */
std::uint32_t PcGameMusicModule();

/**
 * Start the renderer, DirectInput, and the keyboard of the PC release, which
 * OpenBossRallyPodAndInit() starts for the race, for the front end of the Nintendo 64 release,
 * which draws and reads the controllers without it.
 */
void StartPcGameForFrontEnd();

/**
 * Run the race the front end of the Nintendo 64 release chose, with the game code of the PC
 * release, until the race returns to the front end, and then restore the screen of the front end.
 * The game mode, the track, the car, and the weather are those the front end set.
 */
void RunPcRaceFromFrontEnd();

/**
 * Save the frames of the scenes that end after this call as a PNG image, for code that presents
 * its frames itself.
 *
 * @param capture The image, or null for no image.
 */
void SetPcGameCapture(const std::filesystem::path *capture);

} // namespace coastline
