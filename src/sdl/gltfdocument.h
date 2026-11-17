#pragma once

/**
 * @file
 * The reader of glTF and GLB model documents.
 */

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace coastline {

/** A glTF or GLB document with buffers joined and buffer-view offsets adjusted. */
class GltfDocument {
public:
    /**
     * A document from its parts.
     *
     * @param path The selected document, after .gltf precedence is resolved.
     * @param json The parsed document with consolidated buffer references.
     * @param buffer The decoded buffers in order, each starting on a 4-byte boundary.
     */
    GltfDocument(std::filesystem::path path,
                 nlohmann::ordered_json json,
                 std::vector<std::uint8_t> buffer);

    /**
     * Report the path of the document.
     *
     * @return The selected document, after .gltf precedence is resolved.
     */
    const std::filesystem::path &Path() const;

    /**
     * Report the parsed document.
     *
     * @return The document with consolidated buffer references.
     */
    const nlohmann::ordered_json &Json() const;

    /**
     * Report the buffer of the document.
     *
     * @return The decoded buffers in order, each starting on a 4-byte boundary.
     */
    const std::vector<std::uint8_t> &Buffer() const;

    /**
     * Read the encoded bytes of an image, from the file of its URI relative to the document or
     * from its buffer view.
     *
     * @param index The zero-based entry in the document's images array.
     * @return The encoded image from a URI or buffer view, or an error.
     */
    std::expected<std::vector<std::uint8_t>, std::string> Image(std::size_t index) const;

private:
    std::filesystem::path m_path;
    nlohmann::ordered_json m_json;
    std::vector<std::uint8_t> m_buffer;
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
