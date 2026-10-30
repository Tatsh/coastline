#pragma once

/**
 * @file
 * The credits of the Nintendo 64 release over a race of the recording of the credits of the PC
 * release: the captions of the race of the options menu, read from the main segment of a release.
 */

#include <filesystem>

namespace coastline {

class MenuText;
class SceneRenderer;

/**
 * Read the captions of a release and start them from the first, as the race of the credits starts.
 *
 * @param release The directory of the import of the release, such as n64/usa.
 * @return Whether the release has the captions, logged when it does not.
 */
bool StartConsoleCredits(const std::filesystem::path &release);

/**
 * Advance the captions by a step of the race, as DrawCreditCaption() of the console advances them
 * once a frame, while the race of the credits runs.
 *
 * @param seconds The time of the step.
 */
void StepConsoleCredits(float seconds);

/**
 * Draw the caption that shows, as DrawCreditCaption() of the console draws it, while the race of
 * the credits runs.
 *
 * @param renderer The renderer of the frame.
 * @param text The fonts of the release.
 */
void DrawConsoleCredits(SceneRenderer &renderer, const MenuText &text);

} // namespace coastline
