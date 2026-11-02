#include "disctrackreader.h"

#include <algorithm>
#include <limits>

namespace coastline {

DiscTrackReader::DiscTrackReader(const CueTrack &track)
    : m_storage(track.storage), m_extents(track.extents), m_count(track.sectorCount),
      m_stride(track.sectorSize) {
    if (m_stride < 2048 || m_stride > 2448 || m_count == 0) {
        return;
    }
    if (m_extents.empty()) {
        if (track.firstSector > std::numeric_limits<std::uint64_t>::max() / m_stride) {
            return;
        }
        m_extents.push_back({track.file, track.firstSector * m_stride, m_count});
    }
    std::uint64_t total = 0;
    for (const auto &extent : m_extents) {
        std::error_code error;
        const auto size = std::filesystem::file_size(extent.file, error);
        if (error || extent.sectorCount == 0 || extent.byteOffset > size ||
            size > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()) ||
            extent.sectorCount > (size - extent.byteOffset) / m_stride ||
            extent.sectorCount > m_count - total) {
            return;
        }
        total += extent.sectorCount;
    }
    m_valid = total == m_count;
}

bool DiscTrackReader::Read(std::uint64_t sector,
                           std::uint32_t count,
                           std::span<std::uint8_t> output) {
    if (!m_valid || sector > m_count || count > m_count - sector ||
        static_cast<std::uint64_t>(count) * m_stride != output.size()) {
        return false;
    }
    for (const auto &extent : m_extents) {
        if (count == 0) {
            return true;
        }
        if (sector >= extent.sectorCount) {
            sector -= extent.sectorCount;
            continue;
        }
        const auto part = std::min<std::uint64_t>(count, extent.sectorCount - sector);
        const auto bytes = static_cast<std::size_t>(part * m_stride);
        if (m_open != extent.file) {
            m_file.close();
            m_file.clear();
            m_file.open(extent.file, std::ios::binary);
            m_open = extent.file;
        }
        m_file.clear();
        m_file.seekg(static_cast<std::streamoff>(extent.byteOffset + sector * m_stride));
        if (!m_file.read(reinterpret_cast<char *>(output.data()),
                         static_cast<std::streamsize>(bytes))) {
            return false;
        }
        output = output.subspan(bytes);
        count -= static_cast<std::uint32_t>(part);
        sector = 0;
    }
    return count == 0;
}

} // namespace coastline
