#pragma once

/**
 * @file
 * The display lists of the front end of the Nintendo 64 release on the renderer of the PC release.
 */

/**
 * @param host An address on the host.
 * @return Whether the data at the address is the big-endian copy of the segment of the cartridge.
 */
bool CartridgeDataIsBigEndian(const void *host);

namespace coastline {

/**
 * Make each display list the game submits ready for the renderer of the PC release first, for the
 * front end of the Nintendo 64 release: matrices to floats, vertices to the converted vertices of
 * the PC release, and the indices of triangles halved.
 */
void RenderN64DisplayLists();

} // namespace coastline
