#pragma once

/**
 * @file
 * Editable car textures reconstructed from imported glTF or GLB assets. Console body materials
 * use indexed texels and palettes; numbered panels composite indexed paint over imported RGBA.
 */

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
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

/** The byte offsets and dimensions of an indexed model texture or numbered paint layer. */
struct DecalTexture {
    std::size_t texels = 0;  /*!< The offset of the 4-bit texels. */
    std::size_t palette = 0; /*!< The offset of the 16 colours, each a big-endian RGBA5551 half. */
    int width = 0;           /*!< The width in texels. */
    int height = 0;          /*!< The height in texels. */
};

/** Editable console body materials or numbered image layers for an imported car. */
class DecalCar {
public:
    /**
     * Reconstruct editable textures from an imported car document and its images.
     *
     * @param directory The directory of the car, with model.gltf or model.glb.
     * @return The car, or std::nullopt for reported loading or validation failures.
     * @throws nlohmann::json::exception If editable image metadata has invalid fields or types.
     */
    static std::optional<DecalCar> Load(const std::filesystem::path &directory);

    /**
     * @param material The console material or zero-based numbered image-panel index.
     * @return Texture offsets and dimensions, or an empty texture for an invalid index.
     */
    DecalTexture Texture(int material) const;
    /**
     * @param material The body material or numbered image-panel index.
     * @return Whether the material has non-empty indexed texels and a palette.
     */
    bool HasMaterial(int material) const;
    /** @return The Controller Pak note identifier associated with this car's decal format. */
    std::string DecalNote() const;
    /** @return Whether numbered image layers replace the original body-panel layout. */
    bool ImagePanels() const;
    /** @return The number of editable panels, including absent console body materials. */
    int PanelCount() const;
    /**
     * Composite a numbered panel over its imported RGBA pixels.
     *
     * @param panel The zero-based numbered image-panel index.
     * @return Top-down RGBA8 pixels, or an empty vector for a non-image or invalid panel.
     */
    std::vector<u8> PanelPixels(int panel) const;
    /** @return The versioned image-layout fingerprint, or an empty header for console decals. */
    std::vector<u8> NoteHeader() const;
    /**
     * Validate the save header, complete panel payloads, and palette before loading.
     *
     * @param note The saved note bytes.
     * @return Whether saved data matches this model and includes every editable panel and palette.
     */
    bool ValidNote(std::span<const u8> note) const;
    /**
     * Locate a packed pixel in linear image layers or console body-panel rows.
     *
     * @param x The pixel column within the texture.
     * @param y The pixel row within the texture.
     * @param width The texture width, in pixels.
     * @return The byte offset from the start of the panel's packed 4-bit texels.
     */
    std::size_t PixelOffset(int x, int y, int width) const;

    /**
     * @param material The console material or zero-based numbered image-panel index.
     * @return Mutable packed 4-bit texels, or null for an absent or invalid material.
     * Console rows use TexelOffset(); numbered panels use linear rows through PixelOffset().
     */
    u8 *Texels(int material);

    /** @return One byte past the editable storage, valid until the car is destroyed. */
    const u8 *End() const;

    /**
     * @param slot The console decal, from 0 to DECAL_SLOTS - 1.
     * @return The mask of the decal, a bit a texel with a set bit protecting it, or null where
     * every texel can be painted. Numbered image panels always return null.
     */
    const u8 *Mask(int slot) const;

    /**
     * @param material The console material or zero-based numbered image-panel index.
     * @param entry The colour, from 0 to 15.
     * @return The RGBA5551 colour, or zero for an absent material or invalid entry.
     */
    u16 PaletteColor(int material, int entry) const;

    /**
     * Replace one material palette entry without changing its indexed texels.
     * Absent materials and invalid entries are ignored. Numbered panels share one palette.
     *
     * @param material The console material or zero-based numbered image-panel index.
     * @param entry The colour, from 0 to 15.
     * @param color The RGBA5551 colour.
     */
    void SetPaletteColor(int material, int entry, u16 color);

    /**
     * Write painted images and an image-override glTF document in imported image order.
     * Unpainted console images retain their original encoded bytes.
     * Errors from the final document write are not checked.
     *
     * @param directory The directory to write the document and the images to.
     * @return The document path, or std::nullopt when writing or decoding an image fails.
     */
    std::optional<std::filesystem::path> WriteImages(const std::filesystem::path &directory) const;

    /** @return The directory of the car in the import. */
    const std::filesystem::path &Directory() const;

private:
    DecalCar() = default;
    bool LoadImagePanels();
    std::filesystem::path m_directory;
    nlohmann::ordered_json m_images;
    std::vector<std::vector<std::uint8_t>> m_imageBytes;
    std::vector<DecalTexture> m_imageTextures;
    std::vector<bool> m_imageDecals;
    std::vector<u8> m_bytes;
    bool m_imagePanels = false;
    std::vector<std::vector<u8>> m_basePixels;
    std::uint32_t m_imageKey = 2166136261u;
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
