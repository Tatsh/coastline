#pragma once

#include "platform/win/fileio.h"
#include "track.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress BRD3D: 0x106c8e78
 * @ghidraAddress BRGlide: 0x106eff08
 */
extern u8 g_abTrackGeometryBuffer[TRACK_MAX_SIZE];

/**
 * @ghidraAddress BRD3D: 0x10038510
 * @ghidraAddress BRGlide: 0x10031b80
 */
void ReadAndRelocateTrackHeader(TrackHeader *track, GameFile *file);

/**
 * @ghidraAddress BRD3D: 0x10037990
 * @ghidraAddress BRGlide: 0x10031030
 */
void LoadTextureMapFile(const char *path);

/**
 * @ghidraAddress BRD3D: 0x10037a90
 * @ghidraAddress BRGlide: 0x10031140
 */
void LoadTrackTextureMap(int track);

/**
 * @ghidraAddress BRD3D: 0x10037e10
 * @ghidraAddress BRGlide: 0x100314d0
 */
void ByteSwapTrackData(TrackHeader *track);

/**
 * @ghidraAddress BRD3D: 0x10037fa0
 * @ghidraAddress BRGlide: 0x10031660
 */
void ComputeTrackMaxCellIndex(TrackHeader *track);

/**
 * @ghidraAddress BRD3D: 0x10038380
 * @ghidraAddress BRGlide: 0x10031a40
 */
void ByteSwapTerrainVertex(TrackPoint *point);

/**
 * @ghidraAddress BRD3D: 0x10038010
 * @ghidraAddress BRGlide: 0x100316d0
 */
void ByteSwapTrackInstance(TrackInstance *instance);

/**
 * @ghidraAddress BRD3D: 0x10037fe0
 * @ghidraAddress BRGlide: 0x100316a0
 */
void ByteSwapTrackInstances(TrackHeader *track);

/**
 * @ghidraAddress BRD3D: 0x10038b20
 * @ghidraAddress BRGlide: 0x10032190
 */
void ByteSwapTrackAnimationTable(TrackHeader *track);

/**
 * @ghidraAddress BRD3D: 0x10038250
 * @ghidraAddress BRGlide: 0x10031910
 */
void ByteSwapTrackSegmentList(TrackHeader *track);

/**
 * @ghidraAddress BRD3D: 0x100382a0
 * @ghidraAddress BRGlide: 0x10031960
 */
void ByteSwapTrackSegment(TrackSection *section);

/**
 * @ghidraAddress BRD3D: 0x10038410
 * @ghidraAddress BRGlide: 0x10031a80
 */
void ByteSwapTrackVec3Array(TrackHeader *track);

/**
 * @ghidraAddress BRD3D: 0x10038450
 * @ghidraAddress BRGlide: 0x10031ac0
 */
void CopyTextureAndPaletteFromSegment(ModelTexture *textures, int count);

#ifdef __cplusplus
}
#endif
