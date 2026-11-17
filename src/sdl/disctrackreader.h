#pragma once

/**
 * @file
 * The reader of the stored sectors of a disc track split across image files.
 */

#include <fstream>
#include <span>

#include "importer.h"

namespace coastline {

/** Read bounded sectors across the ordered parts of a disc track. */
class DiscTrackReader {
public:
    /**
     * Validate track extents against their files without loading their contents.
     * Invalid tracks cause every subsequent Read() call to fail.
     *
     * @param track Track with sector stride, total length, and optional split-file extents.
     */
    explicit DiscTrackReader(const CueTrack &track);

    /**
     * Read complete stored sectors, including any headers and subchannel bytes.
     *
     * @param sector First sector relative to the start of the track, excluding its pregap.
     * @param count Number of sectors to read.
     * @param output Destination of exactly count times the stored sector stride bytes.
     * @return Whether the track is valid and every requested byte is read. Failure may partially
     * modify output. Requests outside the track or with a mismatched output size fail before
     * reading.
     */
    bool Read(std::uint64_t sector, std::uint32_t count, std::span<std::uint8_t> output);

private:
    // The shared temporary storage of a decoded image, retained while the reader exists.
    std::shared_ptr<const void> m_storage;
    // The parts of the track in order, or one part from the first sector of an unsplit track.
    std::vector<DiscTrackExtent> m_extents;
    std::uint64_t m_count = 0;    // The stored sectors of the track.
    std::uint32_t m_stride = 0;   // The stored bytes of a sector, from 2048 to 2448.
    std::ifstream m_file;         // The image file opened last, reused by the next Read().
    std::filesystem::path m_open; // The path of m_file, or empty before the first Read().
    bool m_valid = false;         // Whether the parts fit their files and cover the track.
};

} // namespace coastline
