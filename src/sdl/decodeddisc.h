#pragma once

#include "importer.h"

namespace coastline {

/** A decoded disc image whose temporary files remain available through shared storage. */
struct DecodedDisc {
    std::filesystem::path file; /*!< Headerless ISO or raw-sector BIN image. */
    /** Attempts to delete temporary files when the final shared storage reference is released. */
    std::shared_ptr<const void> storage;
};

/**
 * Create private temporary storage for an ISO or raw-sector image.
 * @param raw Whether the image uses raw CD sectors rather than cooked data sectors.
 * @param bytes Required disk space in bytes.
 * @return Destination path for a new image and shared cleanup storage, or a diagnostic on failure.
 */
std::expected<DecodedDisc, std::string> CreateDecodedDisc(bool raw, std::uint64_t bytes);

} // namespace coastline
