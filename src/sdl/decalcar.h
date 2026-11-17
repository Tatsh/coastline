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
#include "gltfdocument.h"

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
     * Look up the texture of a material in the texture table of the model, or the layer of a
     * numbered panel.
     *
     * @param material The console material or zero-based numbered image-panel index.
     * @return Texture offsets and dimensions, or an empty texture for an invalid index.
     */
    DecalTexture Texture(int material) const;
    /**
     * Test whether a material or panel can be painted.
     *
     * @param material The body material or numbered image-panel index.
     * @return Whether the material has non-empty indexed texels and a palette.
     */
    bool HasMaterial(int material) const;
    /**
     * Select the note of the decals of the car by its directory: TOP GEAR DECALS for console
     * cars, or COASTLINE MT, COASTLINE CP, COASTLINE HE, or COASTLINE BB for the Milk Truck, the
     * Cupra, the Helmet, and the Beach Ball.
     *
     * @return The Controller Pak note identifier associated with this car's decal format.
     */
    std::string DecalNote() const;
    /**
     * Report the layout of the car, chosen by Load() from the directory name.
     *
     * @return Whether numbered image layers replace the original body-panel layout.
     */
    bool ImagePanels() const;
    /**
     * Count the panels of the paint shop, DECAL_SLOTS for console cars.
     *
     * @return The number of editable panels, including absent console body materials.
     */
    int PanelCount() const;
    /**
     * Composite a numbered panel over its imported RGBA pixels.
     *
     * @param panel The zero-based numbered image-panel index.
     * @return Top-down RGBA8 pixels, or an empty vector for a non-image or invalid panel.
     */
    std::vector<u8> PanelPixels(int panel) const;
    /**
     * Build the header of a note of numbered panels, the bytes CPN and version 1 followed by the
     * big-endian FNV-1a hash of the sizes and pixels of the imported images.
     *
     * @return The versioned image-layout fingerprint, or an empty header for console decals.
     */
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
     * Locate the editable texels of a material or panel.
     *
     * @param material The console material or zero-based numbered image-panel index.
     * @return Mutable packed 4-bit texels, or null for an absent or invalid material.
     * Console rows use TexelOffset(); numbered panels use linear rows through PixelOffset().
     */
    u8 *Texels(int material);

    /**
     * Locate the end of the editable storage, the bound of a pointer from Texels().
     *
     * @return One byte past the editable storage, valid until the car is destroyed.
     */
    const u8 *End() const;

    /**
     * Copy the editable storage for a later RestoreEditableData().
     *
     * @return A copy of all editable texels and palettes for paint history.
     */
    std::vector<u8> CopyEditableData() const;

    /**
     * Restore editable texels and palettes without invalidating texture pointers.
     *
     * @param data A snapshot from CopyEditableData() for this car.
     * @return Whether the snapshot has the expected size and was restored.
     */
    bool RestoreEditableData(std::span<const u8> data);

    /**
     * Locate the protection mask of a console decal in the mask table of the model.
     *
     * @param slot The console decal, from 0 to DECAL_SLOTS - 1.
     * @return The mask of the decal, a bit a texel with a set bit protecting it, or null where
     * every texel can be painted. Numbered image panels always return null.
     */
    const u8 *Mask(int slot) const;

    /**
     * Read one palette entry of a material.
     *
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

    /**
     * Build the image-override document of WriteImages() in memory, with each image in a buffer
     * view.
     *
     * @return The document, or std::nullopt when encoding or decoding an image fails.
     */
    std::optional<GltfDocument> ImageDocument() const;

    /**
     * Report the directory passed to Load().
     *
     * @return The directory of the car in the import.
     */
    const std::filesystem::path &Directory() const;

private:
    // Create an empty car for Load() to fill.
    DecalCar() = default;
    // Replace the model bytes with one shared 16-colour palette and a 4-bit layer for each of 3
    // to 20 imported images, decoding each image to RGBA8 and hashing it into the note key.
    // Fails unless every image decodes within 80 by 125 pixels and 2048 layer bytes, and the
    // storage with a note header fits in 0x3e00 bytes.
    bool LoadImagePanels();
    // Encode one imported image with the current paint. Numbered panels encode as RGBA8 PNG,
    // painted console materials as 4-bit indexed PNG, and other images retain their imported bytes.
    // Returns std::nullopt when decoding or encoding fails.
    std::optional<std::vector<std::uint8_t>> ImageBytes(std::size_t index) const;
    std::filesystem::path m_directory; // From Load().
    // The images array of the document, each with a uri of image-N.png in place of its view.
    nlohmann::ordered_json m_images;
    // The encoded bytes of each imported image, in document order.
    std::vector<std::vector<std::uint8_t>> m_imageBytes;
    // The texture each image maps to in m_bytes, or a numbered panel layer, in image order.
    std::vector<DecalTexture> m_imageTextures;
    // Whether each console image shares texels with a decal and writes rows without the swap.
    std::vector<bool> m_imageDecals;
    // The editable storage, the model bytes of a console car or the panel palette and layers.
    std::vector<u8> m_bytes;
    bool m_imagePanels = false; // Whether the car uses numbered image panels.
    // The imported RGBA8 pixels beneath each numbered panel, top-down.
    std::vector<std::vector<u8>> m_basePixels;
    // The FNV-1a hash of the sizes and pixels of the numbered panels, for NoteHeader().
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
