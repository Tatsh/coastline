#pragma once

#include "decodeddisc.h"

namespace coastline {

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
