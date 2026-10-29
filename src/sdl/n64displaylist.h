#pragma once

/**
 * @file
 * The display lists of the front end of the Nintendo 64 release, drawn as the RSP and the RDP of
 * the console draw them.
 */

#include <filesystem>

/**
 * @param host An address on the host.
 * @return Whether the data at the address is the big-endian copy of the segment of the cartridge.
 */
bool CartridgeDataIsBigEndian(const void *host);

namespace coastline {

/**
 * Set where the textures of the front end are read from and written to as PNG files, named after
 * the hash of their pixels. A file there replaces the texture, at any size.
 *
 * @param directory The directory.
 * @param dump Whether each texture not in the directory is written to it when first drawn.
 */
void SetN64TextureDirectory(std::filesystem::path directory, bool dump);

/**
 * Make each display list the game submits ready for the renderer of the PC release first, for the
 * front end of the Nintendo 64 release: matrices to floats, vertices to the converted vertices of
 * the PC release, and the indices of triangles halved.
 */
void RenderN64DisplayLists();

} // namespace coastline
