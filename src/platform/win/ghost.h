#pragma once

/**
 * @file
 * The ghost cars of the PC port: the frames of their recordings.
 */

/** The size of a frame of the recording of a ghost car. */
#define GHOST_FRAME_SIZE 24

/**
 * The number of frames of the recordings of the ghost cars. Only the second is written here, and
 * the number of entries is not recovered yet.
 *
 * @ghidraAddress BRD3D: 0x10b502e8
 * @ghidraAddress BRGlide: 0x10b73648
 */
extern unsigned g_adwGhostFrameCount[];

/**
 * Set the number of frames of the second recording from its size.
 *
 * @param dwSize The size of the recording in bytes.
 * @ghidraAddress BRD3D: 0x1006ae00
 * @ghidraAddress BRGlide: 0x10063db0
 */
void SetGhostFrameCountFromSize(unsigned dwSize);
