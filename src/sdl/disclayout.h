#pragma once

#include "importer.h"

namespace coastline {

/**
 * Resolve tracks from CUE, ISO, BIN, numbered track files, CloneCD, DiscJuggler CDI, Nero NRG,
 * or Alcohol MDS.
 *
 * @param disc Disc image or metadata file. Adjacent CUE sheets precede BIN layout discovery.
 * @return Track extents, or a message when metadata is unreadable or a data track cannot be
 * located. Container offsets and split MDS extents exclude pregaps. Unknown BIN layouts with
 * uncertain audio boundaries return only their data track.
 */
std::expected<std::vector<CueTrack>, std::string> ReadDiscTracks(const std::filesystem::path &disc);

} // namespace coastline
