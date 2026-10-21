#ifndef COASTLINE_REPLAY_H
#define COASTLINE_REPLAY_H

/**
 * @file
 * The ghost recording a race replays.
 */

#include <PR/ultratypes.h>

/**
 * The recorded input of the ghost. A Controller Pak or the cartridge fills it.
 *
 * @ghidraAddress USA: 0x80307f00
 */
extern u8 g_abGhostRecording[];

/** The bytes of g_abGhostRecording in use. @ghidraAddress USA: 0x80270784 */
extern u32 g_dwGhostRecordingSize;

/**
 * Compress interleaved data with run lengths, each channel on its own.
 *
 * @param dst The buffer to write.
 * @param dstSize The size of dst, in bytes.
 * @param src The interleaved data.
 * @param srcSize The size of src, in bytes.
 * @param channels The number of interleaved channels.
 * @return The size of the compressed data, or -1 when it does not fit in dst.
 * @ghidraAddress USA: 0x8021c878
 */
s32 CompressRleChannels(u8 *dst, s32 dstSize, u8 *src, s32 srcSize, s32 channels);

/**
 * Expand the data CompressRleChannels() wrote, interleaving its channels again.
 *
 * @param dst The buffer to write.
 * @param dstSize The size of dst, which is not checked.
 * @param src The compressed data.
 * @param channels The number of interleaved channels.
 * @return The size of the expanded data, in bytes.
 * @ghidraAddress USA: 0x8021cb4c
 */
s32 DecompressInterleavedRle(u8 *dst, s32 dstSize, u8 *src, s32 channels);

#endif
