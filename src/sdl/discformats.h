#pragma once

#include <algorithm>
#include <array>
#include <string_view>

namespace coastline {

/** Disc images, track metadata, and companion files accepted by the importer. */
inline constexpr std::array<std::string_view, 18> DISC_IMAGE_EXTENSIONS{".b5i",
                                                                        ".b5t",
                                                                        ".b6i",
                                                                        ".b6t",
                                                                        ".bin",
                                                                        ".bwi",
                                                                        ".bws",
                                                                        ".bwt",
                                                                        ".ccd",
                                                                        ".cdi",
                                                                        ".cue",
                                                                        ".img",
                                                                        ".iso",
                                                                        ".isz",
                                                                        ".mdf",
                                                                        ".mds",
                                                                        ".nrg",
                                                                        ".sub"};

/**
 * Check a lower-case filename extension, including its initial dot.
 *
 * @param extension Normalised extension to test.
 * @return Whether the importer recognises the extension as a disc source.
 */
constexpr bool IsDiscImageExtension(std::string_view extension) {
    return std::ranges::contains(DISC_IMAGE_EXTENSIONS, extension);
}

} // namespace coastline
