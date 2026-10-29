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

/** A texture of a glTF document, as the texels of the console. */
struct GltfImage {
    std::string uri;                  /*!< The URI the document gives the image. */
    int width = 0;                    /*!< The width in texels. */
    int height = 0;                   /*!< The height in texels. */
    bool intensity = false;           /*!< Whether the texels are 8-bit intensities, not CI4. */
    std::vector<std::uint8_t> texels; /*!< The texels row by row, two to a byte for CI4. */
    std::array<std::uint8_t, 32> palette{}; /*!< The 16 colours of a CI4 image, RGBA 5551. */
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
 * are retained in the buffer for CarModelFromGltf().
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
 * Build the file of the model of a car from its glTF, the inverse of CarModelToGltf(). The
 * vertices come from the accessors of the document.
 *
 * @param json The document.
 * @param buffer The bytes of its buffer.
 * @return The file, or a description of why the glTF does not describe a car.
 */
std::expected<std::vector<std::uint8_t>, std::string>
CarModelFromGltf(const nlohmann::ordered_json &json, std::span<const std::uint8_t> buffer);

} // namespace coastline
