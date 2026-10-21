#ifndef COASTLINE_REPLAY_H
#define COASTLINE_REPLAY_H

/**
 * @file
 * The ghost recording a race replays.
 */

#include <PR/ultratypes.h>

#include "vehicle.h"

/**
 * The recorded input of the ghost. A Controller Pak or the cartridge fills it.
 *
 * @ghidraAddress USA: 0x80307f00
 */
extern u8 g_abGhostRecording[];

/** The bytes of g_abGhostRecording in use. @ghidraAddress USA: 0x80270784 */
extern u32 g_dwGhostRecordingSize;

/** The cartridge offset of the ghost recording, which the race after the splash screens replays. */
#define ROM_GHOST_RECORDING 0xAD400

/** The bytes a recording of input starts with: the track, the car and its setup. */
#define RECORDING_HEADER_SIZE 8

/** The size of the buffer a player's input is recorded into. */
#define RECORDING_CAPACITY 0xDE5C

/**
 * The buffer the recording a race replays is copied to. The link script places it, below the
 * frame buffers.
 */
extern u8 g_abPlaybackRecording[];

/**
 * The buffers the input of each local player is recorded into, 0xF000 bytes apart. The link script
 * places them, below and over the frame buffers.
 */
extern u8 g_aabInputRecordings[][0xF000];

/**
 * The player whose recording the instant replay plays, and the length of that recording.
 *
 * @ghidraAddress USA: 0x80315e3c
 */
extern s32 g_dwBestReplayLength;

/** @ghidraAddress USA: 0x80315e40 */
extern s32 g_dwBestReplayPlayer;

/**
 * The recordings the instant replay gives each local player, and their lengths.
 *
 * @ghidraAddress USA: 0x80315e48
 */
extern u8 *g_apReplayGhostPtrs[4];

/** @ghidraAddress USA: 0x80315e58 */
extern s32 g_adwReplayGhostSizes[4];

/**
 * Restore the state of the race from a snapshot SaveLastLapSnapshot() took.
 *
 * @param buffer The buffer holding the snapshot.
 * @ghidraAddress USA: 0x8022af08
 */
void RestoreRaceSnapshot(void *buffer);

/**
 * At the start of the last lap, give each local player a buffer to record the lap into and take a
 * snapshot of the race, which the instant replay starts from.
 *
 * @param vehicle The vehicle that reached the lap.
 * @ghidraAddress USA: 0x8022af90
 */
void SaveLastLapSnapshot(Vehicle *vehicle);

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
