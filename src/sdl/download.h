#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "importer.h"

namespace coastline {

/**
 * Download an HTTPS URL to a file with the HTTP client of the operating system, following
 * redirects.
 * @param url The HTTPS URL.
 * @param output The file to write.
 * @param progress Progress and cancellation callback, or empty.
 * @return An error on failure or cancellation, or an empty optional when complete.
 */
std::optional<std::string> DownloadFile(const std::string &url,
                                        const std::filesystem::path &output,
                                        const ImportProgress &progress = {});

} // namespace coastline
