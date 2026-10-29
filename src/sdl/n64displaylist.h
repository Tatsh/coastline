#pragma once

/**
 * @file
 * The display lists of the front end of the Nintendo 64 release, drawn as the RSP and the RDP of
 * the console draw them.
 */

#include <cstdint>
#include <filesystem>
#include <string>

/**
 * @param host An address on the host.
 * @return Whether the data at the address is the big-endian copy of the segment of the cartridge.
 */
bool CartridgeDataIsBigEndian(const void *host);

namespace coastline {

/**
 * Set the texture packs of the front end. A pack is a directory of PNG files: the pictures of the
 * cartridge, named after where each starts on the cartridge, and the other textures, named after
 * their size and the hash of their pixels. A file of the chosen pack, or else of the pack default,
 * replaces the texture, at any size.
 *
 * @param directory The directory of the packs.
 * @param pack The chosen pack.
 * @param dump Whether each other texture no pack has is written to the pack default when first
 * drawn.
 */
void SetN64TextureDirectory(std::filesystem::path directory, std::string pack, bool dump);

/**
 * Make each display list the game submits ready for the renderer of the PC release first, for the
 * front end of the Nintendo 64 release: matrices to floats, vertices to the converted vertices of
 * the PC release, and the indices of triangles halved.
 */
void RenderN64DisplayLists();

} // namespace coastline
