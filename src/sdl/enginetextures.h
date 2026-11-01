#pragma once

/**
 * @file
 * The textures the race engine creates through the hooks of the renderer of the PC release, stored
 * as texels in memory for the SDL game to draw.
 */

#include <cstdint>
#include <vector>

#include "common/renderbackend.h"
#include "common/vehicleview.h"

namespace coastline {

/** A texture of the race engine, as 8-bit RGBA texels. */
struct EngineTexture {
    int width = 0;                  /*!< The width in texels. */
    int height = 0;                 /*!< The height in texels. */
    std::vector<std::uint8_t> rgba; /*!< The texels, row by row from the top. */
    unsigned version = 0;           /*!< Counts the loads of new texels into the texture. */
};

/** Decoded textures for one active race engine. One instance exists at a time. */
class EngineTextures {
public:
    /** Register an empty texture collection for the sole active race engine. */
    EngineTextures();
    /** Unregister the collection and release its decoded texels. */
    ~EngineTextures();
    /** Copying is disabled because texture hooks address the active collection. */
    EngineTextures(const EngineTextures &) = delete;
    /** Assignment is disabled because the active collection has a stable address. */
    EngineTextures &operator=(const EngineTextures &) = delete;

    /** @return The active texture collection, or null outside its lifetime. */
    static EngineTextures *Active();

    /** @return Renderer settings and texture hooks for the race engine. */
    SdlRenderBackendState &Backend() {
        return m_backend;
    }
    /** @return Sky images and reflection descriptors for the race engine. */
    SdlVehicleReflections &Reflections() {
        return m_reflections;
    }

    /** @copydoc coastline::CreateEngineTexture */
    unsigned Create(const unsigned char *image,
                    const unsigned char *palette,
                    int width,
                    int height,
                    int lineBytes);
    /** @copydoc coastline::LoadEngineTexture */
    void Load(unsigned entry, const unsigned char *colors);
    /** @copydoc coastline::FindEngineTexture */
    const EngineTexture *Find(unsigned entry) const;
    /** Discard decoded textures when the race engine shuts its texture subsystem down. */
    void Clear();

private:
    std::vector<EngineTexture> m_textures;
    SdlRenderBackendState m_backend{};
    SdlVehicleReflections m_reflections{};
};

/**
 * Create a texture from an 8-bit colour-indexed image of the console, as the hook
 * g_pfnCreateTextureFromN64Image() of the PC release creates one.
 *
 * @param image The indices of the texels.
 * @param palette The 256 colours, each a big-endian RGBA5551 half.
 * @param width The width in texels.
 * @param height The height in texels.
 * @param lineBytes The bytes of a row of the image.
 * @return The texture entry, from 1, or zero without an active EngineTextures instance.
 */
unsigned CreateEngineTexture(
    const unsigned char *image, const unsigned char *palette, int width, int height, int lineBytes);

/**
 * Decode an 8-bit colour-indexed image of the console to its colours, each a big-endian
 * RGBA5551 half, as the hook g_pfnDecodeN64TextureToPixelBuffer() of the PC release decodes one.
 *
 * @param destination Receives the colours, row by row.
 * @param image The indices of the texels.
 * @param palette The 256 colours, each a big-endian RGBA5551 half.
 * @param width The width in texels.
 * @param height The height in texels.
 * @param lineBytes The bytes of a row of the image.
 * @return The bytes written.
 */
int DecodeEngineTexture(void *destination,
                        const unsigned char *image,
                        const unsigned char *palette,
                        int width,
                        int height,
                        int lineBytes);

/**
 * Load the decoded colours of an image into a texture, as a command 0xdd of a display list loads
 * them.
 *
 * @param entry The texture entry, from 1. Invalid entries are ignored.
 * @param colors The colours of its texels, each a big-endian RGBA5551 half, row by row.
 */
void LoadEngineTexture(unsigned entry, const unsigned char *colors);

/**
 * @param entry The entry of a texture.
 * @return The texture, or null for an invalid entry or without an active collection.
 * The pointer remains valid until another texture is created or the collection is cleared.
 */
const EngineTexture *FindEngineTexture(unsigned entry);

/** Drop every texture, as the renderer of the PC release shuts its textures down. */
void ClearEngineTextures();

} // namespace coastline
