#pragma once

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
    std::shared_ptr<const void> m_storage;
    std::vector<DiscTrackExtent> m_extents;
    std::uint64_t m_count = 0;
    std::uint32_t m_stride = 0;
    std::ifstream m_file;
    std::filesystem::path m_open;
    bool m_valid = false;
};

} // namespace coastline
