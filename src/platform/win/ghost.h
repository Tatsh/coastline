#pragma once

/**
 * @file
 * The ghost cars of the PC port: the frames of their recordings.
 */

#include <cstddef>

#include "rigidbody.h"
#include "vecmath.h"
#include "vehicle.h"

/** The size of a frame of the recording of a ghost car. */
#define GHOST_FRAME_SIZE 24

/** The number of entries of g_adwGhostFrameCount. */
#define GHOST_RECORDING_COUNT 8

/** The maximum number of frames in each recording. */
#define GHOST_MAX_FRAMES 65536

/** Vehicle pose and interpolated controls before compact frame encoding. */
struct KeyframeState {
    Quaternion orientation;
    Vec3f position;
    Vec3f linearVelocity;
    Vec3f angularVelocity;
    float fBodyRollAngle;
    float fSteerAngle;
    float afWheelAngle[4];
    float afWheelContact[4];
    float afWheelSurface[4];
    float fGearControl;
    float fBraking;
    float fReverse;
    float fProgress;
    float fEngineRpm;
    float afLight[2];
    float afEffectFlags[6];
};

/**
 * Encode a pose and controls into a compact ghost frame, preserving its final two bytes.
 *
 * @param pBuffer Destination frame.
 * @param pState Pose and controls.
 * @ghidraAddress BRD3D: 0x10006bd0
 */
void PackKeyframeToCompactBuffer(u8 *pBuffer, const KeyframeState *pState);

/**
 * Decode a compact ghost frame into pose and controls. Velocities and progress are not written.
 *
 * @param pState Destination pose and controls.
 * @param pBuffer Encoded frame.
 * @ghidraAddress BRD3D: 0x10007730
 */
void DeserializeKeyframeState(KeyframeState *pState, const u8 *pBuffer);

/**
 * Interpolate poses and controls, reversing the target quaternion for distant scalar signs.
 *
 * @param pOut Destination frame.
 * @param fShare Interpolation share, clamped from zero to ten.
 * @param pFrom Initial frame.
 * @param pTo Target frame.
 * @ghidraAddress BRD3D: 0x100079e0
 */
void InterpolateKeyframeState(KeyframeState *pOut,
                              float fShare,
                              KeyframeState *pFrom,
                              KeyframeState *pTo);

/** The metadata stored before the frames of a recording. */
struct GhostRecordingInfo {
    s8 track;            /*!< The recorded track, or -1 without a valid recording. */
    s8 car;              /*!< The recorded car. */
    s8 steeringMode;     /*!< The recorded handling option. */
    s8 transmission;     /*!< The recorded transmission option. */
    s8 tires;            /*!< The recorded tyre option. */
    s8 suspension;       /*!< The recorded suspension option. */
    u8 controllerLayout; /*!< The recorded controller layout. */
    s8 weather;          /*!< The recorded weather option. */
    u32 reserved[2];     /*!< Zero for a new recording, or all bits set after a failed load. */
};

/**
 * The metadata of the ghost recording.
 *
 * @ghidraAddress BRD3D: 0x10680738
 */
extern GhostRecordingInfo g_GhostRecordingInfo;

/**
 * The number of metadata bytes written before the frames.
 *
 * @ghidraAddress BRD3D: 0x10680730
 */
extern unsigned g_dwGhostRecordingSize;

/**
 * The encoded frames of all ghost recordings.
 *
 * @ghidraAddress BRD3D: 0x10b50308
 */
extern u8 g_aGhostKeyframes[GHOST_RECORDING_COUNT][GHOST_MAX_FRAMES * GHOST_FRAME_SIZE];

/**
 * The best race time of the ghost, in seconds.
 *
 * @ghidraAddress BRD3D: 0x10aa28c8
 */
extern float g_fGhostBestRaceTime;

/**
 * The best lap time of the ghost, in seconds.
 *
 * @ghidraAddress BRD3D: 0x10aa28cc
 */
extern float g_fGhostBestLapTime;

/**
 * The lap-count option read from a ghost file.
 *
 * @ghidraAddress BRD3D: 0x10ad098c
 */
extern int g_dwLoadedGhostLapCount;

/**
 * The four-byte signature of a ghost file.
 *
 * @ghidraAddress BRD3D: 0x100b5d9c
 */
extern const char g_szGhostSaveMagic[4];

/**
 * The zero marker preceding the length in a current ghost file.
 *
 * @ghidraAddress BRD3D: 0x1008fc1c
 */
extern const unsigned g_dwGhostFileHeader;

/**
 * Get the encoded frames of the first recording.
 *
 * @return The recording buffer.
 * @ghidraAddress BRD3D: 0x1006ab90
 */
u8 *GetGhostKeyframeBuffer(void);

/**
 * Get the encoded frames used for ghost playback.
 *
 * @return The second recording buffer.
 * @ghidraAddress BRD3D: 0x1006adf0
 */
u8 *GetGhostPlaybackKeyframeBuffer(void);

/**
 * Get the size of the encoded frames of the first recording.
 *
 * @return The size in bytes.
 * @ghidraAddress BRD3D: 0x1006aba0
 */
unsigned GetGhostKeyframeDataSize(void);

/**
 * Read a current or legacy ghost file and validate its Adler checksum.
 *
 * @param pszPath The ghost file path.
 * @param bAllowFailure Whether a failed load returns success after clearing the metadata and times.
 * @return Whether loading succeeded or a permitted failure occurred.
 * @ghidraAddress BRD3D: 0x10070af0
 */
bool ReadGhostFileData(const char *pszPath, bool bAllowFailure);

/**
 * Read the ghost file selected in the front end.
 *
 * @param bAllowFailure Whether a failed load returns success.
 * @return The result of ReadGhostFileData().
 * @ghidraAddress BRD3D: 0x10070e60
 */
bool LoadGhostFile(bool bAllowFailure);

/**
 * Write the ghost recording, times, checksum, menu settings, and slot label to the selected file.
 *
 * @return Whether the file opened and all checked writes completed.
 * @ghidraAddress BRD3D: 0x10070e80
 */
bool SaveGhostFile(void);

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
 * @return The number of complete frames.
 * @ghidraAddress BRD3D: 0x1006ae00
 * @ghidraAddress BRGlide: 0x10063db0
 */
unsigned SetGhostFrameCountFromSize(unsigned dwSize);

/**
 * Clear the number of frames of the recordings, only the first in time attack and the demonstration
 * and all of them otherwise, and stop recording.
 *
 * @ghidraAddress BRD3D: 0x1006aa50
 * @ghidraAddress BRGlide: 0x10063a00
 */
void ResetGhostRecording(void);

/**
 * Start ghost recording.
 *
 * @ghidraAddress BRD3D: 0x1006aa90
 */
void StartGhostRecording(void);

/**
 * Read the ghost recording flag.
 *
 * @return Recording flag without boolean conversion.
 * @ghidraAddress BRD3D: 0x1006aaa0
 */
int IsGhostRecordingActive(void);

/**
 * Current playback frame of each recording.
 *
 * @ghidraAddress BRD3D: 0x11750310
 */
extern int g_adwGhostPlaybackFrame[GHOST_RECORDING_COUNT];

/**
 * Advance active recordings and the time attack or demonstration ghost playback.
 *
 * @ghidraAddress BRD3D: 0x1006ab20
 */
void AdvanceGhostRecordFrame(void);

/**
 * Reset every ghost playback frame to zero.
 *
 * @ghidraAddress BRD3D: 0x1006abb0
 */
void ClearGhostPlaybackState(void);

/**
 * Translate replay controls into playback mode and bounded frame changes.
 *
 * @ghidraAddress BRD3D: 0x1006ad10
 */
void UpdateReplayScrubControls(void);

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

/** The orientation and the position of a vehicle at the start of a key frame. */
struct QuaternionPose {
    Quaternion orientation; /*!< The orientation. */
    Vec3f position;         /*!< The position. */
};

/**
 * Get the orientation of a matrix as a quaternion and its translation as a position.
 *
 * @param pPose Receives the orientation and the position.
 * @param m The matrix.
 * @ghidraAddress BRD3D: 0x100695a0
 * @ghidraAddress BRGlide: 0x10062610
 */
void MatrixToPositionQuaternion(QuaternionPose *pPose, f32 m[4][4]);

/**
 * Convert a quaternion and position into a matrix, compensating for quaternion magnitude.
 *
 * @param matrix Destination matrix.
 * @param pPose Quaternion and position.
 * @ghidraAddress BRD3D: 0x100695d0
 */
void PositionQuaternionToMatrix4x4(f32 matrix[4][4], const QuaternionPose *pPose);

/**
 * Sample a vehicle pose, wheel motion, and replay effects.
 *
 * @param pState Destination frame.
 * @param pVehicle Vehicle to sample.
 * @ghidraAddress BRD3D: 0x100607b0
 */
void BuildVehicleKeyframe(KeyframeState *pState, Vehicle *pVehicle);

/**
 * @ghidraAddress BRD3D: 0x100609e0
 */
void RecordVehicleKeyframe(Vehicle *vehicle);

/**
 * @ghidraAddress BRD3D: 0x10060cc0
 * @ghidraAddress BRGlide: 0x10059d30
 */
unsigned ApplyRemoteVehicleState(Vehicle *vehicle, int slotIndex);

/**
 * Restore sampled motion and effects, then synchronise integration states.
 *
 * @param pVehicle Destination vehicle.
 * @param pState Frame to restore.
 * @ghidraAddress BRD3D: 0x10060a10
 */
void ApplyVehicleKeyframe(Vehicle *pVehicle, const KeyframeState *pState);

/**
 * Record one vehicle frame while recording is active and the race is not paused.
 *
 * @param pVehicle Vehicle to record.
 * @ghidraAddress BRD3D: 0x1006aab0
 */
void RecordGhostKeyframe(Vehicle *pVehicle);

/**
 * Restore a ghost frame and estimate velocity from the following frame when available.
 *
 * @param pVehicle Destination vehicle.
 * @param nRecording Recording slot.
 * @ghidraAddress BRD3D: 0x1006abd0
 */
void ApplyGhostKeyframeToVehicle(Vehicle *pVehicle, int nRecording);

/**
 * Restore the ghost frame from the recording slot matching a vehicle index.
 *
 * @param pVehicle Destination vehicle.
 * @ghidraAddress BRD3D: 0x1006acf0
 */
void ApplyGhostKeyframeForVehicle(Vehicle *pVehicle);
