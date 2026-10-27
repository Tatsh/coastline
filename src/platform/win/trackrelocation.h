#pragma once

#include "platform/win/fileio.h"
#include "track.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress BRD3D: 0x106c8e78 */
extern u8 g_abTrackGeometryBuffer[TRACK_MAX_SIZE];

/** @ghidraAddress BRD3D: 0x106c56ec */
extern unsigned g_dwTrackDataRelocationDelta;

/** @ghidraAddress BRD3D: 0x10038510 */
void ReadAndRelocateTrackHeader(TrackHeader *track, GameFile *file);

/**
 * @ghidraAddress BRD3D: 0x10037990
 */
void LoadTextureMapFile(const char *path);

/**
 * @ghidraAddress BRD3D: 0x10037a90
 */
void LoadTrackTextureMap(int track);

/** @ghidraAddress BRD3D: 0x10037e10 */
void ByteSwapTrackData(TrackHeader *track);

/**
 * @ghidraAddress BRD3D: 0x10037fa0
 */
void ComputeTrackMaxCellIndex(TrackHeader *track);

/**
 * @ghidraAddress BRD3D: 0x10038380
 */
void ByteSwapTerrainVertex(TrackPoint *point);

/**
 * @ghidraAddress BRD3D: 0x10038010
 */
void ByteSwapTrackInstance(TrackInstance *instance);

/**
 * @ghidraAddress BRD3D: 0x10037fe0
 */
void ByteSwapTrackInstances(TrackHeader *track);

/**
 * @ghidraAddress BRD3D: 0x10038b20
 */
void ByteSwapTrackAnimationTable(TrackHeader *track);

/**
 * @ghidraAddress BRD3D: 0x10038250
 */
void ByteSwapTrackSegmentList(TrackHeader *track);

/**
 * @ghidraAddress BRD3D: 0x100382a0
 */
void ByteSwapTrackSegment(TrackSection *section);

/**
 * @ghidraAddress BRD3D: 0x10038410
 */
void ByteSwapTrackVec3Array(TrackHeader *track);

/**
 * @ghidraAddress BRD3D: 0x10038450
 */
void CopyTextureAndPaletteFromSegment(ModelTexture *textures, int count);

#ifdef __cplusplus
}
#endif
