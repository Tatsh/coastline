#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "importer.h"

namespace coastline {

/**
 * Convert optional disc intro media with an external FFmpeg process.
 * Windows and macOS download one temporary tool copy when FFmpeg is absent. Linux uses PATH.
 * @param directory The extracted disc assets, including optional brally.avi and splash.img.
 * @param progress Progress and cancellation callback, or empty.
 * @return An error on conversion or cancellation, or an empty optional when complete or media
 * is absent.
 */
std::optional<std::string> ConvertIntroMedia(const std::filesystem::path &directory,
                                             const ImportProgress &progress = {});

} // namespace coastline
