#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace coastline {

/**
 * Visit CRC-checked ROM entries in a ZIP archive without extracting archive paths.
 *
 * @param archive The ZIP archive.
 * @param visit Process unpacked entry bytes. Return false to stop.
 * @param progress Report entry progress, or empty. Return false to stop.
 * @return A failure or cancellation message, or nothing on success.
 */
std::optional<std::string>
VisitZipRoms(const std::filesystem::path &archive,
             const std::function<bool(std::vector<std::uint8_t> &)> &visit,
             const std::function<bool(const std::string &, double)> &progress = {});

} // namespace coastline
