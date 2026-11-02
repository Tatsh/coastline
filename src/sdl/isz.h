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
 * Decode an unencrypted ISZ image, including split segments, to temporary storage.
 * Validate metadata, compressed stream lengths, and checksums when supplied by the header.
 *
 * @param image First segment of an ISZ image.
 * @param progress Decoding progress, or empty. Return false to cancel and discard temporary data.
 * @return A decoded image with shared storage, or a diagnostic on invalid input or cancellation.
 */
std::expected<DecodedDisc, std::string> DecodeIsz(const std::filesystem::path &image,
                                                  const ImportProgress &progress = {});

} // namespace coastline
