#pragma once

/**
 * @file
 * The model of a car of a cartridge as the paint shop of the Nintendo 64 release paints it: the
 * bytes of the model as the console loads it, built again from its glTF document in the import,
 * with the decals and the palettes painted in place.
 */

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/types.h"

namespace coastline {

/** The decals of a car, each the texture of the material of the same number. */
inline constexpr int DECAL_SLOTS = 10;

/** The materials of a car whose palettes the colour of the car paints. */
inline constexpr int CAR_MATERIALS = 12;

/** The material whose palette is the paint, saved with the decals. */
inline constexpr int CAR_MATERIAL_PAINT = 2;

/** The material whose palette is the headlights. */
inline constexpr int CAR_MATERIAL_HEADLIGHTS = 10;

/** The material whose palette is the rear lights. */
inline constexpr int CAR_MATERIAL_REAR_LIGHTS = 11;

/** A texture of the model: where its texels and its palette are in the bytes, and its size. */
struct DecalTexture {
    std::size_t texels = 0;  /*!< The offset of the 4-bit texels. */
    std::size_t palette = 0; /*!< The offset of the 16 colours, each a big-endian RGBA5551 half. */
    int width = 0;           /*!< The width in texels. */
    int height = 0;          /*!< The height in texels. */
};

/** The model of a car of a cartridge, painted in place. */
class DecalCar {
public:
    /**
     * Build the bytes of a car of the import again from its glTF document.
     *
     * @param directory The directory of the car, with model.gltf.
     * @return The car, or nothing with the reason logged.
     */
    static std::optional<DecalCar> Load(const std::filesystem::path &directory);

    /**
     * @param material The material, from 0 to CAR_MATERIALS - 1.
     * @return The texture of the material.
     */
    DecalTexture Texture(int material) const;

    /**
     * @param material The material, from 0 to CAR_MATERIALS - 1.
     * @return The texels of the texture of the material, in the order the console loads them.
     */
    u8 *Texels(int material);

    /** @return The end of the bytes of the model, past which no texel is read. */
    const u8 *End() const;

    /**
     * @param slot The decal, from 0 to DECAL_SLOTS - 1.
     * @return The mask of the decal, a bit a texel with a set bit protecting it, or null where
     * every texel can be painted.
     */
    const u8 *Mask(int slot) const;

    /**
     * @param material The material, from 0 to CAR_MATERIALS - 1.
     * @param entry The colour, from 0 to 15.
     * @return The RGBA5551 colour.
     */
    u16 PaletteColor(int material, int entry) const;

    /**
     * @param material The material, from 0 to CAR_MATERIALS - 1.
     * @param entry The colour, from 0 to 15.
     * @param color The RGBA5551 colour.
     */
    void SetPaletteColor(int material, int entry, u16 color);

    /**
     * Write the images of the car as they are painted, and a glTF document of those images, one
     * for one with the images of the car of the import.
     *
     * @param directory The directory to write the document and the images to.
     * @return The document, or nothing with the reason logged.
     */
    std::optional<std::filesystem::path> WriteImages(const std::filesystem::path &directory) const;

    /** @return The directory of the car in the import. */
    const std::filesystem::path &Directory() const;

private:
    DecalCar() = default;
    std::filesystem::path m_directory;
    nlohmann::ordered_json m_images;
    std::vector<u8> m_bytes;
};

/**
 * The byte of a texel of a 4-bit texture as the console loads it in one block, the 64-bit words of
 * each odd row swapped.
 *
 * @param x The column.
 * @param y The row.
 * @param width The width of the texture in texels.
 * @return The offset of the byte from the first texel.
 */
inline int TexelOffset(int x, int y, int width) {
    return (((y & 1) << 3 ^ x) >> 1) + y * (width >> 1);
}

} // namespace coastline
