#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "importer.h"

namespace coastline {

/**
 * Convert optional disc intro media. Linux uses libav directly. Windows and macOS use a temporary
 * FFmpeg executable when one is absent from PATH.
 * @param directory The extracted disc assets, including optional brally.avi and splash.img.
 * @param progress Progress and cancellation callback, or empty.
 * @return An error on conversion or cancellation, or an empty optional when complete or media
 * is absent.
 */
std::optional<std::string> ConvertIntroMedia(const std::filesystem::path &directory,
                                             const ImportProgress &progress = {});

} // namespace coastline
