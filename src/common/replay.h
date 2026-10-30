#ifndef COASTLINE_REPLAY_H
#define COASTLINE_REPLAY_H

/**
 * @file
 * The ghost recording a race replays.
 */

#include <PR/ultratypes.h>

#include "common/input.h"
#include "common/recordingformat.h"
#include "common/rle.h"
#include "common/vehicle.h"
#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "common/vehiclemodel.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The recorded input of the ghost. A Controller Pak or the cartridge fills it.
 *
 * @ghidraAddress USA: 0x80307f00
 * @ghidraAddress EUR: 0x80309bf0
 * @ghidraAddress JPN: 0x80308630
 */
extern u8 g_abGhostRecording[];

/**
 * The bytes of g_abGhostRecording in use.
 *
 * @ghidraAddress USA: 0x80270784
 * @ghidraAddress EUR: 0x80272754
 * @ghidraAddress JPN: 0x80272204
 */
extern u32 g_dwGhostRecordingSize;

extern u8 _ghostRecordingSegmentRomStart[];
extern u8 _creditsRecordingSegmentRomStart[];
extern u8 _endingRecordingSegmentRomStart[];

/** The cartridge offset of the ghost recording, which the race after the splash screens replays. */
#define ROM_GHOST_RECORDING ((u32)_ghostRecordingSegmentRomStart)

/** The size of the buffer a player's input is recorded into. */
#define RECORDING_CAPACITY 0xDE5C

/**
 * The buffer the recording a race replays is copied to. The link script places it, below the
 * frame buffers.
 *
 * @ghidraAddress BRD3D: 0x117554f0
 * @ghidraAddress BRGlide: 0x11778850
 */
extern u8 g_abPlaybackRecording[];

/**
 * The buffers the input of each local player is recorded into, 0xF000 bytes apart. The link script
 * places them, below and over the frame buffers.
 *
 * @ghidraAddress BRD3D: 0x117644f0
 * @ghidraAddress BRGlide: 0x11787850
 */
extern u8 g_aabInputRecordings[][0xF000];

#if !TGR_BOSS_RALLY
/**
 * The player whose recording the instant replay plays, and the length of that recording.
 *
 * @ghidraAddress USA: 0x80315e3c
 * @ghidraAddress EUR: 0x80317b2c
 * @ghidraAddress JPN: 0x8031656c
 * @ghidraAddress BRD3D: 0x106909c8
 * @ghidraAddress BRGlide: 0x105ccb70
 */
extern s32 g_dwBestReplayLength;
#endif /* !TGR_BOSS_RALLY */

/**
 * @ghidraAddress USA: 0x80315e40
 * @ghidraAddress EUR: 0x80317b30
 * @ghidraAddress JPN: 0x80316570
 * @ghidraAddress BRD3D: 0x10680668
 * @ghidraAddress BRGlide: 0x105bc810
 */
extern s32 g_dwBestReplayPlayer;

#if TGR_PLATFORM == TGR_PLATFORM_N64
/**
 * The recordings the instant replay gives each local player, and their lengths.
 *
 * @ghidraAddress USA: 0x80315e48
 * @ghidraAddress EUR: 0x80317b38
 * @ghidraAddress JPN: 0x80316578
 */
extern u8 *g_apReplayGhostPtrs[];

/**
 * @ghidraAddress USA: 0x80315e58
 * @ghidraAddress EUR: 0x80317b48
 * @ghidraAddress JPN: 0x80316588
 */
extern s32 g_adwReplayGhostSizes[];
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

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
 * Last-lap buffers share vehicle-model storage and descend through slots by local-player index.
 */
#if TGR_PLATFORM == TGR_PLATFORM_SDL
/* The symbols of the SDL game have no leading underscore on every system. A cast gives the storage
 * of g_aVehicleModels the type of the buffers. */
#define g_aLastLapBuffers ((LastLapBuffer *)g_aVehicleModels)
#elif TGR_BOSS_RALLY
extern LastLapBuffer g_aLastLapBuffers[] __asm__("_g_aVehicleModels");
#else
extern LastLapBuffer g_aLastLapBuffers[];
#endif

/** The slot of g_aLastLapBuffers of the first player; the others count down from it. */
#if TGR_BOSS_RALLY
#define LAST_LAP_FIRST_SLOT 15
#else
#define LAST_LAP_FIRST_SLOT 3
#endif

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
 * @ghidraAddress EUR: 0x8028ddc0
 * @ghidraAddress JPN: 0x8028d7f0
 * @ghidraAddress BRD3D: 0x100b39b0
 */
extern SaveField g_aRaceSnapshotFields[];

/**
 * The random seed and the clocks a pause keeps.
 *
 * @ghidraAddress USA: 0x8028bbc4
 * @ghidraAddress EUR: 0x8028de78
 * @ghidraAddress JPN: 0x8028d8a8
 * @ghidraAddress BRD3D: 0x100b3a68
 * @ghidraAddress BRGlide: 0x100b3270
 */
extern SaveField g_aPausedClockFields[];

/** The bytes of the paused clock state. */
#define PAUSED_CLOCK_STATE_SIZE 0x40

/**
 * Where a pause keeps them.
 *
 * @ghidraAddress USA: 0x8034e570
 * @ghidraAddress EUR: 0x80326800
 * @ghidraAddress JPN: 0x80325430
 * @ghidraAddress BRD3D: 0x10af9848
 * @ghidraAddress BRGlide: 0x10b1cba8
 */
extern u8 g_abPausedClockState[PAUSED_CLOCK_STATE_SIZE];

#if !TGR_BOSS_RALLY && (TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU)
/**
 * Copy the variables of a list one after another into a buffer, and report when they overflow it.
 *
 * @param fields The list of variables.
 * @param buffer The buffer.
 * @param capacity The size of the buffer in bytes.
 * @param offset The bytes past the start of each variable to copy from.
 * @return The bytes written.
 * @ghidraAddress EUR: 0x8022c144
 * @ghidraAddress JPN: 0x8022b8fc
 */
s32 SerializeSaveData(SaveField *fields, u8 *buffer, s32 capacity, s32 offset);

/**
 * Copy the variables of a list back from a buffer SerializeSaveData() filled.
 *
 * @param fields The list of variables.
 * @param buffer The buffer.
 * @param offset The bytes past the start of each variable to copy to.
 * @return The bytes read.
 * @ghidraAddress EUR: 0x8022c1f8
 * @ghidraAddress JPN: 0x8022b9b0
 */
s32 DeserializeSaveData(SaveField *fields, u8 *buffer, s32 offset);
#else
/**
 * Copy the variables of a list one after another into a buffer, and report when they overflow it.
 *
 * @param fields The list of variables.
 * @param buffer The buffer.
 * @param capacity The bytes the buffer holds.
 * @ghidraAddress USA: 0x8022adcc
 * @ghidraAddress BRD3D: 0x10067880
 * @ghidraAddress BRGlide: 0x100608f0
 */
void SerializeSaveData(SaveField *fields, u8 *buffer, s32 capacity);

/**
 * Copy the variables of a list back from a buffer SerializeSaveData() filled.
 *
 * @param fields The list of variables.
 * @param buffer The buffer.
 * @ghidraAddress USA: 0x8022ae70
 * @ghidraAddress BRD3D: 0x10067900
 * @ghidraAddress BRGlide: 0x10060970
 */
void DeserializeSaveData(SaveField *fields, u8 *buffer);
#endif /*                                                                                          \
        * !TGR_BOSS_RALLY && (TGR_REGION == TGR_REGION_JP || TGR_REGION ==                         \
        * TGR_REGION_EU)                                                                           \
        */

/**
 * Take a snapshot of the race into the buffer of a last lap.
 *
 * @param buffer The buffer of the last lap.
 * @ghidraAddress USA: 0x8022aed8
 * @ghidraAddress EUR: 0x8022c274
 * @ghidraAddress JPN: 0x8022ba2c
 * @ghidraAddress BRD3D: 0x10067940
 * @ghidraAddress BRGlide: 0x100609b0
 */
void SaveRaceSnapshot(LastLapBuffer *buffer);

/**
 * Restore the state of the race from a snapshot SaveLastLapSnapshot() took.
 *
 * @param buffer The buffer of the last lap holding the snapshot.
 * @ghidraAddress USA: 0x8022af08
 * @ghidraAddress EUR: 0x8022c2ac
 * @ghidraAddress JPN: 0x8022ba64
 * @ghidraAddress BRD3D: 0x10067960
 * @ghidraAddress BRGlide: 0x100609d0
 */
void RestoreRaceSnapshot(LastLapBuffer *buffer);

#if TGR_PLATFORM == TGR_PLATFORM_N64
/**
 * At the start of the last lap, give each local player a buffer to record the lap into and take a
 * snapshot of the race, which the instant replay starts from.
 *
 * @param vehicle The vehicle that reached the lap.
 * @ghidraAddress USA: 0x8022af90
 * @ghidraAddress EUR: 0x8022c4a0
 * @ghidraAddress JPN: 0x8022bc58
 */
void SaveLastLapSnapshot(Vehicle *vehicle);
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
/**
 * The frames RestoreReplayKeyframeState() has run.
 *
 * @ghidraAddress EUR: 0x8028dec0
 * @ghidraAddress JPN: 0x8028d8f0
 */
extern s32 g_dwReplayKeyframeFrameCount;

/**
 * The variables a keyframe of the recording of the demo restores, in the order of the keyframe.
 *
 * @ghidraAddress EUR: 0x8028e2cc
 * @ghidraAddress JPN: 0x8028dcfc
 */
extern SaveField g_aReplayKeyframeDescTable1[];

/**
 * @ghidraAddress EUR: 0x8028e22c
 * @ghidraAddress JPN: 0x8028dc5c
 */
extern SaveField g_aReplayKeyframeDescTable2[];

/**
 * @ghidraAddress EUR: 0x8028dec4
 * @ghidraAddress JPN: 0x8028d8f4
 */
extern SaveField g_aReplayKeyframeDescTable3[];

/**
 * In the demo, restore the race from a keyframe when the recording of the first vehicle reaches
 * one, then place the trackside camera of the first vehicle anew.
 *
 * @ghidraAddress EUR: 0x8022c340
 * @ghidraAddress JPN: 0x8022baf8
 */
void RestoreReplayKeyframeState(void);
#endif /* TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_REPLAY_H */
