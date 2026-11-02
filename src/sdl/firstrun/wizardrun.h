#pragma once

/**
 * @file
 * The setup wizard in the window of the game, driven by SDL or by a script of run-script.
 */

#include <filesystem>
#include <span>
#include <string>

#include <SDL3/SDL.h>

#include "config.h"
#include "gpu.h"
#include "paths.h"
#include "runscript.h"

namespace coastline {

/** How a run of the setup wizard ended. */
struct SetupRun {
    bool finished = false; /*!< Whether the data was imported and Finish was pressed. */
    /** Whether renderer, font, or portable-configuration setup failed. */
    bool failed = true;
    bool portable = false; /*!< Whether setup switched to fully local storage. */
    int frames = 0;        /*!< The frames the wizard drew, for the frames of a script after it. */
};

/**
 * Show the setup wizard until the user finishes or quits it. Its import deletes the packs
 * default, boss, eur, and jpn under assets of the data directory and imports the chosen files into
 * the default pack. The wizard hides the cursor of the system and draws its own, and shows the
 * cursor again when it ends. Disc, ROM, search, and last-browsed paths are restored from the
 * SetupWizard section of @p config and updated on exit, including cancellation.
 *
 * @param window The window of the game.
 * @param paths The directories of the game, updated after a successful portable installation.
 * @param config The settings of the game.
 * @param script The run-script input, or an empty script. Scripted key presses, text input, and
 * mouse movements operate wizard controls. Mouse coordinates on the 320 by 240 screen scale to
 * the 640 by 400 grid.
 * @param scriptKeys The scancode of each key of @p script, in order.
 * @param captures The directory the frames of the script are saved to.
 * @param fixedTime Whether each frame steps the blink of the cursor by a 60th of a second.
 * @param reimportReason Why existing assets require another import, or empty for first-run setup.
 * @param portableDirectory Portable destination, or empty to disable the installation checkbox.
 * @return How the wizard ended.
 */
SetupRun RunSetupWizard(GpuWindow &window,
                        GamePaths &paths,
                        GameConfig &config,
                        const Script &script,
                        std::span<const SDL_Scancode> scriptKeys,
                        const std::filesystem::path &captures,
                        bool fixedTime,
                        const std::string &reimportReason = {},
                        const std::filesystem::path &portableDirectory = {});

} // namespace coastline
