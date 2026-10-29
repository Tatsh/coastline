#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace coastline {

/** A texture of a glTF document, as the RDP samples it. */
struct GltfImage {
    std::string uri;                /*!< The URI the document gives the image. */
    int width = 0;                  /*!< The width in texels. */
    int height = 0;                 /*!< The height in texels. */
    std::vector<std::uint8_t> rgba; /*!< The texels as 8-bit RGBA, row by row from the top. */
};

/** A glTF document, the bytes of its one buffer, and its images. */
struct GltfModel {
    nlohmann::ordered_json json;      /*!< The document. */
    std::vector<std::uint8_t> buffer; /*!< The bytes of the buffer the document refers to. */
    std::vector<GltfImage> images;    /*!< The images the document refers to. */
};

/**
 * Describe the model of a car as glTF. The document has a scene for each level of detail, with a
 * mesh for each display list of the body and the wheels at their places. Every vertex the display
 * lists load appears once, in the order of the file, and the bytes of the file outside the vertices
 * are retained in the buffer for ModelFromGltf().
 *
 * @param bytes The file, a header of @p imageOffset bytes followed by the image of the model as the
 * console loads it at 0x803c8000.
 * @param imageOffset The size of the header before the image, 0x8000 for a .rca file of the PC
 * release and 0 for a model of the cartridge.
 * @param bufferUri The URI the document gives its buffer.
 * @return The glTF, or a description of why the file is not a model of a car.
 */
std::expected<GltfModel, std::string> CarModelToGltf(std::span<const std::uint8_t> bytes,
                                                     std::size_t imageOffset,
                                                     std::string_view bufferUri);

/**
 * Describe a track as glTF. The document has one scene, with a node for each placed model at its
 * place, and the sky as a node outside the scene. The bytes of the file outside the vertices are
 * retained in the buffer for ModelFromGltf(), the textures of the bank after the geometry among
 * them.
 *
 * @param bytes The file, the image of the geometry as the console loads it at 0x80025c00, followed
 * by the textures that stream from it.
 * @param bufferUri The URI the document gives its buffer.
 * @return The glTF, or a description of why the file is not a track.
 */
std::expected<GltfModel, std::string> TrackToGltf(std::span<const std::uint8_t> bytes,
                                                  std::string_view bufferUri);

/**
 * Describe a model of the front end, such as a logo of the splash screens, as glTF. The document
 * has one scene, with a node for each part at its place. Each key of each animation of the
 * vertices is a morph target of every mesh, and the animation blends from each key to the next,
 * with the flags and the span of time the game plays it by in its extras. The bytes of the file
 * outside the vertices are retained in the buffer for ModelFromGltf().
 *
 * @param bytes The model as it inflates from the cartridge, its offsets from its first byte.
 * @param bufferUri The URI the document gives its buffer.
 * @return The glTF, or a description of why the file is not a model.
 */
std::expected<GltfModel, std::string> FrontEndModelToGltf(std::span<const std::uint8_t> bytes,
                                                          std::string_view bufferUri);

/**
 * Build the file of a model or a track from its glTF, the inverse of CarModelToGltf(),
 * TrackToGltf(), and FrontEndModelToGltf(). The vertices come from the accessors of the document.
 *
 * @param json The document.
 * @param buffer The bytes of its buffer.
 * @return The file, or a description of why the glTF does not describe a model of the console.
 */
std::expected<std::vector<std::uint8_t>, std::string>
ModelFromGltf(const nlohmann::ordered_json &json, std::span<const std::uint8_t> buffer);

} // namespace coastline
