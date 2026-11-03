#pragma once

#include <expected>
#include <string>
#include <string_view>

#include "n64model.h"

namespace coastline {

/**
 * Generate a static menu miniature from a converted track during import.
 * Instance transforms are flattened, sky geometry is omitted, convex planar openings are capped,
 * exposed terrain edges extend to the base, and attribute-aware simplification reduces detail.
 * Average texture colours become vertex colours; the result does not require external images.
 * @param track The complete result of TrackToGltf(), including decoded images.
 * @param bufferUri The relative URI of the generated binary buffer.
 * @return A centred glTF miniature, or an error for invalid or empty geometry.
 */
std::expected<GltfModel, std::string> TrackPreviewToGltf(const GltfModel &track,
                                                         std::string_view bufferUri);

} // namespace coastline
