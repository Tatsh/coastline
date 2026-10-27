#pragma once

#include "track.h"

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
 * @ghidraAddress BRD3D: 0x10038410
 */
void ByteSwapTrackVec3Array(TrackHeader *track);

/**
 * @ghidraAddress BRD3D: 0x10038450
 */
void CopyTextureAndPaletteFromSegment(ModelTexture *textures, int count);
