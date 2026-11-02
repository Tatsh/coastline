#pragma once

#include "importer.h"

namespace coastline {

/**
 * Resolve disc tracks from CUE, ISO, BIN, numbered track files, or CloneCD metadata.
 *
 * @param disc Disc image or metadata file. Adjacent CUE sheets precede BIN layout discovery.
 * @return Track extents, or a message when metadata is unreadable or a data track cannot be
 * located. Unknown BIN layouts with uncertain audio boundaries return only their data track.
 */
std::expected<std::vector<CueTrack>, std::string> ReadDiscTracks(const std::filesystem::path &disc);

} // namespace coastline
