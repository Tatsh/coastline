#pragma once

#include "decodeddisc.h"

namespace coastline {

/**
 * Decode a Pinnacle InstantCopy PDI image to temporary storage.
 * Support cooked data, raw data and audio, scrambled data, and interleaved subchannels.
 * @param image PDI image with a 304-byte container header.
 * @param progress Decoding progress, or empty. Return false to discard temporary data and cancel.
 * @return Headerless ISO or BIN with shared cleanup storage, or a diagnostic on failure.
 */
std::expected<DecodedDisc, std::string> DecodePdi(const std::filesystem::path &image,
                                                  const ImportProgress &progress = {});

} // namespace coastline
