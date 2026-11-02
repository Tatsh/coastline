#pragma once

#include "importer.h"

namespace coastline {

/**
 * Resolve tracks from CUE, ISO, BIN, numbered track files, CloneCD, DiscJuggler CDI, Nero NRG,
 * Alcohol MDS, BlindWrite 4/5/6, Pinnacle PDI, or an unencrypted ISZ image.
 * Returned tracks and readers retain shared temporary storage for decoded images.
 *
 * @param disc Disc image or metadata file. Adjacent CUE sheets precede BIN layout discovery.
 * @param progress Decoding progress, or empty. Return false to cancel image decoding.
 * @param sources Optional output, replaced on success by original metadata and data paths.
 * Temporary decoded files are excluded. BIN discovery also records its directory.
 * @return Track extents, or a message when metadata is unreadable or a data track cannot be
 * located. Container offsets and split-file extents exclude pregaps. Unknown BIN layouts with
 * uncertain audio boundaries return only their data track.
 */
std::expected<std::vector<CueTrack>, std::string>
ReadDiscTracks(const std::filesystem::path &disc,
               const ImportProgress &progress = {},
               std::vector<std::filesystem::path> *sources = nullptr);

} // namespace coastline
