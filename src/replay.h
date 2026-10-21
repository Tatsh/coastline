#ifndef COASTLINE_REPLAY_H
#define COASTLINE_REPLAY_H

/**
 * @file
 * The ghost recording a race replays.
 */

#include <PR/ultratypes.h>

#include "input.h"
#include "vehicle.h"

/**
 * The recorded input of the ghost. A Controller Pak or the cartridge fills it.
 *
 * @ghidraAddress USA: 0x80307f00
 * @ghidraAddress JPN: 0x80308630
 * @ghidraAddress EUR: 0x80309bf0
 */
extern u8 g_abGhostRecording[];

/**
 * The bytes of g_abGhostRecording in use.
 *
 * @ghidraAddress USA: 0x80270784
 * @ghidraAddress JPN: 0x80272204
 * @ghidraAddress EUR: 0x80272754
 */
extern u32 g_dwGhostRecordingSize;

extern u8 _ghostRecordingSegmentRomStart[];
extern u8 _creditsRecordingSegmentRomStart[];
extern u8 _endingRecordingSegmentRomStart[];

/** The cartridge offset of the ghost recording, which the race after the splash screens replays. */
#define ROM_GHOST_RECORDING ((u32)_ghostRecordingSegmentRomStart)

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
 * @ghidraAddress JPN: 0x8031656c
 * @ghidraAddress EUR: 0x80317b2c
 */
extern s32 g_dwBestReplayLength;

/**
 * @ghidraAddress USA: 0x80315e40
 * @ghidraAddress JPN: 0x80316570
 * @ghidraAddress EUR: 0x80317b30
 */
extern s32 g_dwBestReplayPlayer;

/**
 * The recordings the instant replay gives each local player, and their lengths.
 *
 * @ghidraAddress USA: 0x80315e48
 * @ghidraAddress EUR: 0x80314f88
 */
extern u8 *g_apReplayGhostPtrs[4];

/**
 * @ghidraAddress USA: 0x80315e58
 * @ghidraAddress EUR: 0x80314f98
 */
extern s32 g_adwReplayGhostSizes[4];

/** The bytes of the recording of each local player's input over the last lap. */
#define LAST_LAP_RECORDING_SIZE 0x3840

/**
 * The last lap of a player: the input of each local player over it, and a snapshot of the race at
 * its start, which the instant replay plays from.
 */
typedef struct {
    u8 recordings[INPUT_RECORDERS][LAST_LAP_RECORDING_SIZE]; /*!< The input of each local player. */
    u8 snapshot[sizeof(VehicleModel) - INPUT_RECORDERS * LAST_LAP_RECORDING_SIZE]; /*!< The race at
                                                                                       lap start. */
} LastLapBuffer;

/**
 * The buffers of the last laps. They are the memory of g_aVehicleModels, whose last slots hold
 * the last lap of each player, the first player in the last slot. The link script places them.
 */
extern LastLapBuffer g_aLastLapBuffers[];

/** The slot of g_aLastLapBuffers of the first player; the others count down from it. */
#define LAST_LAP_FIRST_SLOT 3

/** A block of variables a snapshot saves, in a list ending in a NULL block. */
typedef struct {
    void *data; /*!< The first byte of the block, or NULL at the end of the list. */
    u32 size;   /*!< The length of the block in bytes. */
} SaveField;

/**
 * The variables a snapshot of the race saves: the vehicles, the racers, the clocks, the sounds and
 * more.
 *
 * @ghidraAddress USA: 0x8028bb0c
 * @ghidraAddress JPN: 0x8028d7f0
 * @ghidraAddress EUR: 0x8028ddc0
 */
extern SaveField g_aRaceSnapshotFields[];

/**
 * The random seed and the clocks a pause keeps.
 *
 * @ghidraAddress USA: 0x8028bbc4
 * @ghidraAddress JPN: 0x8028d8a8
 * @ghidraAddress EUR: 0x8028de78
 */
extern SaveField g_aPausedClockFields[];

/** The bytes of the paused clock state. */
#define PAUSED_CLOCK_STATE_SIZE 0x40

/** Where a pause keeps them. @ghidraAddress USA: 0x8034e570 */
extern u8 g_abPausedClockState[PAUSED_CLOCK_STATE_SIZE];

/**
 * Copy the variables of a list one after another into a buffer, and report when they overflow it.
 *
 * @param fields The list of variables.
 * @param buffer The buffer.
 * @param capacity The bytes the buffer holds.
 * @ghidraAddress USA: 0x8022adcc
 * @ghidraAddress JPN: 0x8022b8fc (stub)
 * @ghidraAddress EUR: 0x8022c144 (stub)
 */
void SerializeSaveData(SaveField *fields, u8 *buffer, s32 capacity);

/**
 * Copy the variables of a list back from a buffer SerializeSaveData() filled.
 *
 * @param fields The list of variables.
 * @param buffer The buffer.
 * @ghidraAddress USA: 0x8022ae70
 * @ghidraAddress JPN: 0x8022b9b0 (stub)
 * @ghidraAddress EUR: 0x8022c1f8 (stub)
 */
void DeserializeSaveData(SaveField *fields, u8 *buffer);

/**
 * Take a snapshot of the race into the buffer of a last lap.
 *
 * @param buffer The buffer of the last lap.
 * @ghidraAddress USA: 0x8022aed8
 * @ghidraAddress JPN: 0x8022ba2c (stub)
 * @ghidraAddress EUR: 0x8022c274 (stub)
 */
void SaveRaceSnapshot(LastLapBuffer *buffer);

/**
 * Restore the state of the race from a snapshot SaveLastLapSnapshot() took.
 *
 * @param buffer The buffer of the last lap holding the snapshot.
 * @ghidraAddress USA: 0x8022af08
 * @ghidraAddress JPN: 0x8022ba64 (stub)
 * @ghidraAddress EUR: 0x8022c2ac (stub)
 */
void RestoreRaceSnapshot(LastLapBuffer *buffer);

/**
 * At the start of the last lap, give each local player a buffer to record the lap into and take a
 * snapshot of the race, which the instant replay starts from.
 *
 * @param vehicle The vehicle that reached the lap.
 * @ghidraAddress USA: 0x8022af90
 * @ghidraAddress JPN: 0x8022bc58
 * @ghidraAddress EUR: 0x8022c4a0
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
 * @ghidraAddress JPN: 0x8021d348
 * @ghidraAddress EUR: 0x8021da78
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
 * @ghidraAddress JPN: 0x8021d61c
 * @ghidraAddress EUR: 0x8021dd4c
 */
s32 DecompressInterleavedRle(u8 *dst, s32 dstSize, u8 *src, s32 channels);

#endif
