#pragma once

#include <filesystem>

namespace coastline {

/**
 * Run the routines of the platform the race engine calls against the imported data, without a
 * window or a playback device: the input of a scripted keyboard, a sound effect mixed by EAR, the
 * music of the modules and of the CD, a track, a car, and a model of BossRally.pod. Each result is
 * written to the log.
 *
 * @param defaults The directory of the default assets.
 * @return EXIT_SUCCESS when every check passed.
 */
int CheckEngine(const std::filesystem::path &defaults);

} // namespace coastline
