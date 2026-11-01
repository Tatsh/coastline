#pragma once

#include "common/fileio.h"
#include "common/track.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Storage for the loaded track geometry, with TRACK_MAX_SIZE bytes of capacity.
 *
 * @ghidraAddress BRD3D: 0x106c8e78
 * @ghidraAddress BRGlide: 0x106eff08
 */
extern u8 g_abTrackGeometryBuffer[TRACK_MAX_SIZE];

/**
 * Read TRACK_HEADER_SIZE bytes, swap header fields, and relocate embedded track pointers.
 *
 * @param track Writable storage for the header and the track data referenced by its offsets.
 * @param file The open track file positioned at its header; checked reads handle failures.
 * @ghidraAddress BRD3D: 0x10038510
 * @ghidraAddress BRGlide: 0x10031b80
 */
void ReadAndRelocateTrackHeader(TrackHeader *track, GameFile *file);

/**
 * Replace the active high-resolution texture map and select texture quality from video memory.
 *
 * @param path The null-terminated map path. A missing file produces an empty map and defaults.
 * @ghidraAddress BRD3D: 0x10037990
 * @ghidraAddress BRGlide: 0x10031030
 */
void LoadTextureMapFile(const char *path);

/**
 * Load a track's backend-specific high-resolution texture map.
 *
 * @param track A valid index in g_apszTrackFilenames.
 * @ghidraAddress BRD3D: 0x10037a90
 * @ghidraAddress BRGlide: 0x10031140
 */
void LoadTrackTextureMap(int track);

/**
 * Swap and relocate track payloads, prepare textures, and convert instance and route data.
 *
 * @param track The loaded track with a relocated header and big-endian payload arrays.
 * @ghidraAddress BRD3D: 0x10037e10
 * @ghidraAddress BRGlide: 0x100314d0
 */
void ByteSwapTrackData(TrackHeader *track);

/**
 * Replace the surface count with the maximum grid triangle index plus one, omitting entry zero.
 *
 * @param track The track whose grid starts and triangle indices already use host byte order.
 * @ghidraAddress BRD3D: 0x10037fa0
 * @ghidraAddress BRGlide: 0x10031660
 */
void ComputeTrackMaxCellIndex(TrackHeader *track);

/**
 * Swap the coordinates and distance of one route point in place.
 *
 * @param point A route point whose numeric fields are in the opposite byte order.
 * @ghidraAddress BRD3D: 0x10038380
 * @ghidraAddress BRGlide: 0x10031a40
 */
void ByteSwapTerrainVertex(TrackPoint *point);

/**
 * Swap instance fields and relocate the model pointer in place.
 *
 * @param instance A loaded track instance with unswapped fields.
 * @ghidraAddress BRD3D: 0x10038010
 * @ghidraAddress BRGlide: 0x100316d0
 */
void ByteSwapTrackInstance(TrackInstance *instance);

/**
 * Swap and relocate every loaded instance in a track.
 *
 * @param track The relocated header whose instance array remains in file byte order.
 * @ghidraAddress BRD3D: 0x10037fe0
 * @ghidraAddress BRGlide: 0x100316a0
 */
void ByteSwapTrackInstances(TrackHeader *track);

/**
 * Swap special animation records, relocating and converting moving-object path coordinates.
 *
 * @param track The relocated track whose special animation records remain in file byte order.
 * @ghidraAddress BRD3D: 0x10038b20
 * @ghidraAddress BRGlide: 0x10032190
 */
void ByteSwapTrackAnimationTable(TrackHeader *track);

/**
 * Relocate section-list entries and convert each route section in place.
 *
 * @param track The relocated track header with an unconverted section pointer array.
 * @ghidraAddress BRD3D: 0x10038250
 * @ghidraAddress BRGlide: 0x10031910
 */
void ByteSwapTrackSegmentList(TrackHeader *track);

/**
 * Swap route-section fields and points and relocate section links.
 *
 * @param section A section loaded in file byte order.
 * @ghidraAddress BRD3D: 0x100382a0
 * @ghidraAddress BRGlide: 0x10031960
 */
void ByteSwapTrackSegment(TrackSection *section);

/**
 * Swap every track camera point's three coordinates in place.
 *
 * @param track The relocated header whose camera point array remains in file byte order.
 * @ghidraAddress BRD3D: 0x10038410
 * @ghidraAddress BRGlide: 0x10031a80
 */
void ByteSwapTrackVec3Array(TrackHeader *track);

/**
 * Copy initial images and palettes for two-frame animated track textures into writable buffers.
 *
 * @param textures The relocated texture descriptors, backed by the active track texture segment.
 * @param count The number of descriptors in @p textures.
 * @ghidraAddress BRD3D: 0x10038450
 * @ghidraAddress BRGlide: 0x10031ac0
 */
void CopyTextureAndPaletteFromSegment(ModelTexture *textures, int count);

#ifdef __cplusplus
}
#endif
