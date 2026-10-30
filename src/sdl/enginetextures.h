#pragma once

/**
 * @file
 * The textures the race engine creates through the hooks of the renderer of the PC release, stored
 * as texels in memory for the SDL game to draw.
 */

#include <cstdint>
#include <vector>

namespace coastline {

/** A texture of the race engine, as 8-bit RGBA texels. */
struct EngineTexture {
    int width = 0;                  /*!< The width in texels. */
    int height = 0;                 /*!< The height in texels. */
    std::vector<std::uint8_t> rgba; /*!< The texels, row by row from the top. */
    unsigned version = 0;           /*!< Counts the loads of new texels into the texture. */
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
 * @return The entry of the texture, from 1.
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
 * @param entry The entry of the texture.
 * @param colors The colours of its texels, each a big-endian RGBA5551 half, row by row.
 */
void LoadEngineTexture(unsigned entry, const unsigned char *colors);

/**
 * @param entry The entry of a texture.
 * @return The texture, or null for an entry that was not created.
 */
const EngineTexture *FindEngineTexture(unsigned entry);

/** Drop every texture, as the renderer of the PC release shuts its textures down. */
void ClearEngineTextures();

} // namespace coastline
