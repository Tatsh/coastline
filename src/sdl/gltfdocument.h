#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace coastline {

/** A glTF or GLB document with buffers joined and buffer-view offsets adjusted. */
struct GltfDocument {
    std::filesystem::path path;
    nlohmann::ordered_json json;
    std::vector<std::uint8_t> buffer;

    /** @return The encoded image from a URI or buffer view, or an error. */
    std::expected<std::vector<std::uint8_t>, std::string> Image(std::size_t index) const;
};

/**
 * Read a model or image-override document. A sibling .gltf takes precedence over .glb, including
 * when the .gltf is invalid. Embedded images remain in their buffer views.
 *
 * @param path The document path, with either extension.
 * @return The document and its buffers, or an error.
 */
std::expected<GltfDocument, std::string> ReadGltfDocument(const std::filesystem::path &path);

} // namespace coastline
