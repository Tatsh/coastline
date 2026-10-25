#pragma once

/**
 * @file
 * The ghost cars of the PC port: the frames of their recordings.
 */

#include <cstddef>

/** The size of a frame of the recording of a ghost car. */
#define GHOST_FRAME_SIZE 24

/** The number of entries of g_adwGhostFrameCount. */
#define GHOST_RECORDING_COUNT 8

/**
 * The number of frames of the recordings of the ghost cars.
 *
 * @ghidraAddress BRD3D: 0x10b502e8
 * @ghidraAddress BRGlide: 0x10b73648
 */
extern unsigned g_adwGhostFrameCount[GHOST_RECORDING_COUNT];

/**
 * Whether a recording of a ghost car is being made.
 *
 * @ghidraAddress BRD3D: 0x11750308
 * @ghidraAddress BRGlide: 0x11773668
 */
extern int g_bGhostRecordingActive;

/**
 * Set the number of frames of the second recording from its size.
 *
 * @param dwSize The size of the recording in bytes.
 * @ghidraAddress BRD3D: 0x1006ae00
 * @ghidraAddress BRGlide: 0x10063db0
 */
void SetGhostFrameCountFromSize(unsigned dwSize);

/**
 * Clear the number of frames of the recordings, only the first in time attack and the demonstration
 * and all of them otherwise, and stop recording.
 *
 * @ghidraAddress BRD3D: 0x1006aa50
 * @ghidraAddress BRGlide: 0x10063a00
 */
void ResetGhostRecording(void);

/**
 * Fill memory with a byte.
 *
 * @param pDest The memory.
 * @param nCount The bytes to fill.
 * @param nByte The byte, in the low 8 bits.
 * @ghidraAddress BRD3D: 0x10060780
 * @ghidraAddress BRGlide: 0x100597f0
 */
void FillMemoryPatternBytes(void *pDest, size_t nCount, int nByte);
