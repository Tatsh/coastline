#pragma once

namespace coastline {

/** @return The entries of the texture map LoadTrackTextureMap() read last. */
int LoadedTextureMapEntries();

/** @return The vertices converted into the pool since ResetModelDataPool(). */
unsigned ConvertedVertexCount();

} // namespace coastline
