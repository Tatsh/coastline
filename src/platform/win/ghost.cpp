#include "platform/win/ghost.h"

#include <cstdio>
#include <cstring>

#include <zlib.h>

#include "game.h"
#include "instantreplay.h"
#include "pause.h"
#include "platform/win/frontend.h"
#include "platform/win/input.h"
#include "platform/win/netstream.h"
#include "platform/win/season.h"

unsigned g_adwGhostFrameCount[GHOST_RECORDING_COUNT];
int g_bGhostRecordingActive;
int g_adwGhostPlaybackFrame[GHOST_RECORDING_COUNT];
GhostRecordingInfo g_GhostRecordingInfo;
unsigned g_dwGhostRecordingSize;
u8 g_aGhostKeyframes[GHOST_RECORDING_COUNT][GHOST_MAX_FRAMES * GHOST_FRAME_SIZE];
float g_fGhostBestRaceTime;
float g_fGhostBestLapTime;
int g_dwLoadedGhostLapCount;
const char g_szGhostSaveMagic[4] = {'R', 'G', 'h', 'o'};
const unsigned g_dwGhostFileHeader = 0;

struct CompactGhostFrame {
    short anOrientation[4];
    unsigned nPositionXAndFlags;
    unsigned nPositionYAndFlags;
    short nPositionZ;
    signed char nSteering;
    unsigned char bWheelOffsetAndLight;
    unsigned char abWheelContactAndSurface[2];
    unsigned char abReserved[2];
};

static inline bool IsKeyframeFlagSet(float value) {
    return value > 0.0f || value < 0.0f;
}

void PackKeyframeToCompactBuffer(u8 *pBuffer, const KeyframeState *pState) {
    CompactGhostFrame frame;
    frame.anOrientation[0] =
        static_cast<short>((ConvertFloatToFixedAngle(pState->orientation.w) & 0xfffe) |
                           IsKeyframeFlagSet(pState->afEffectFlags[1]));
    frame.anOrientation[1] =
        static_cast<short>((ConvertFloatToFixedAngle(pState->orientation.x) & 0xfffe) |
                           IsKeyframeFlagSet(pState->afEffectFlags[2]));
    frame.anOrientation[2] =
        static_cast<short>((ConvertFloatToFixedAngle(pState->orientation.y) & 0xfffe) |
                           IsKeyframeFlagSet(pState->afEffectFlags[3]));
    frame.anOrientation[3] =
        static_cast<short>((ConvertFloatToFixedAngle(pState->orientation.z) & 0xfffe) |
                           IsKeyframeFlagSet(pState->afEffectFlags[4]));
    frame.nPositionXAndFlags =
        (ConvertFloatToFixedAngleNoClamp(pState->position.x) & 0xfffff8) |
        IsKeyframeFlagSet(pState->fGearControl) |
        (IsKeyframeFlagSet(pState->afEffectFlags[0]) << 1) |
        (IsKeyframeFlagSet(pState->afEffectFlags[5]) << 2) |
        (static_cast<unsigned>(FloatToByteIndex(pState->afWheelAngle[0])) << 24);
    frame.nPositionYAndFlags =
        (ConvertFloatToFixedAngleNoClamp(pState->position.y) & 0xfffffc) |
        (IsKeyframeFlagSet(pState->fBraking) << 1) | IsKeyframeFlagSet(pState->fReverse) |
        (static_cast<unsigned>((FloatToSectorIndex(pState->fSector) & 0x3f) |
                               (QuantizeFloatToLightIndex(pState->afLight[1]) << 6))
         << 24);
    frame.nPositionZ = static_cast<short>(ConvertFloatToFixedAngle16B(pState->position.z));
    frame.nSteering = static_cast<signed char>(ConvertFloatToSignedByte(pState->fSteering));
    frame.bWheelOffsetAndLight =
        static_cast<unsigned char>((FloatToSignedByteIndex(pState->fWheelOffset) & 0x3f) |
                                   (QuantizeFloatToLightIndex(pState->afLight[0]) << 6));
    for (int pair = 0; pair < 2; ++pair) {
        const int first = pair * 2;
        const int second = first + 1;
        frame.abWheelContactAndSurface[pair] = static_cast<unsigned char>(
            (IsKeyframeFlagSet(pState->afWheelContact[first]) << 7) |
            ((static_cast<int>(pState->afWheelSurface[first]) & 7) << 4) |
            (IsKeyframeFlagSet(pState->afWheelContact[second]) << 3) |
            (static_cast<int>(pState->afWheelSurface[second]) & 7));
    }
    memcpy(pBuffer, &frame, GHOST_FRAME_SIZE - sizeof(frame.abReserved));
}

void DeserializeKeyframeState(KeyframeState *pState, const u8 *pBuffer) {
    CompactGhostFrame frame;
    memcpy(&frame, pBuffer, sizeof(frame));
    pState->orientation.w = ConvertFixedAngleToFloat(frame.anOrientation[0]);
    pState->orientation.x = ConvertFixedAngleToFloat(frame.anOrientation[1]);
    pState->orientation.y = ConvertFixedAngleToFloat(frame.anOrientation[2]);
    pState->orientation.z = ConvertFixedAngleToFloat(frame.anOrientation[3]);
    pState->position.x = ConvertUintToScaledFloat(frame.nPositionXAndFlags & 0xffffff);
    pState->position.y = ConvertUintToScaledFloat(frame.nPositionYAndFlags & 0xffffff);
    pState->position.z = FixedToRadians(frame.nPositionZ);
    pState->fSteering = ConvertCharToScaledFloat(frame.nSteering);
    pState->fWheelOffset = SignedByteToFloat(frame.bWheelOffsetAndLight & 0x3f);
    pState->afLight[0] = GetLightLevelScale(frame.bWheelOffsetAndLight >> 6);
    pState->afWheelAngle[0] = ByteToFloat(frame.nPositionXAndFlags >> 24);
    pState->afWheelAngle[1] = pState->afWheelAngle[0];
    float rearAngle = pState->afWheelAngle[0] + 35.0f;
    if (rearAngle >= 360.0f) {
        rearAngle -= 360.0f;
    }
    pState->afWheelAngle[2] = rearAngle;
    pState->afWheelAngle[3] = rearAngle;
    for (int pair = 0; pair < 2; ++pair) {
        const unsigned flags = frame.abWheelContactAndSurface[pair];
        pState->afWheelContact[pair * 2] = static_cast<float>(flags >> 7);
        pState->afWheelSurface[pair * 2] = static_cast<float>((flags >> 4) & 7);
        pState->afWheelContact[pair * 2 + 1] = static_cast<float>((flags >> 3) & 1);
        pState->afWheelSurface[pair * 2 + 1] = static_cast<float>(flags & 7);
    }
    pState->fGearControl = (frame.nPositionXAndFlags & 1) ? 128.0f : 0.0f;
    pState->fBraking = (frame.nPositionYAndFlags & 2) ? 1.0f : 0.0f;
    pState->fReverse = (frame.nPositionYAndFlags & 1) ? 1.0f : 0.0f;
    const unsigned sectorAndLight = frame.nPositionYAndFlags >> 24;
    pState->fSector = IndexToWorldCoord(sectorAndLight & 0x3f);
    pState->afLight[1] = GetLightLevelScale(sectorAndLight >> 6);
    pState->afEffectFlags[0] = (frame.nPositionXAndFlags & 2) ? 128.0f : 0.0f;
    for (int i = 0; i < 4; ++i) {
        pState->afEffectFlags[i + 1] = (frame.anOrientation[i] & 1) ? 128.0f : 0.0f;
    }
    pState->afEffectFlags[5] = (frame.nPositionXAndFlags & 4) ? 128.0f : 0.0f;
}

static inline float BlendKeyframeValue(float from, float to, float share) {
    return from + (to - from) * share;
}

void InterpolateKeyframeState(KeyframeState *pOut,
                              float fShare,
                              KeyframeState *pFrom,
                              KeyframeState *pTo) {
    if (!(fShare > 0.0f)) {
        fShare = 0.0f;
    } else if (!(fShare < 10.0f)) {
        // The original upper limit is ten rather than the unit interpolation range.
        fShare = 10.0f;
    }
    const float fromW = pFrom->orientation.w;
    const float toW = pTo->orientation.w;
    const bool reverse = (fromW >= 0.0f && toW < 0.0f && fromW - toW >= 1.0f) ||
                         (toW >= 0.0f && fromW < 0.0f && toW - fromW >= 1.0f);
    const float sign = reverse ? -1.0f : 1.0f;
    pOut->orientation.w = BlendKeyframeValue(fromW, toW * sign, fShare);
    pOut->orientation.x =
        BlendKeyframeValue(pFrom->orientation.x, pTo->orientation.x * sign, fShare);
    pOut->orientation.y =
        BlendKeyframeValue(pFrom->orientation.y, pTo->orientation.y * sign, fShare);
    pOut->orientation.z =
        BlendKeyframeValue(pFrom->orientation.z, pTo->orientation.z * sign, fShare);
    Vec3fLerp(&pOut->position, &pTo->position, &pFrom->position, fShare);
    Vec3fLerp(&pOut->linearVelocity, &pTo->linearVelocity, &pFrom->linearVelocity, fShare);
    Vec3fLerp(&pOut->angularVelocity, &pTo->angularVelocity, &pFrom->angularVelocity, fShare);
    pOut->fSteering = BlendKeyframeValue(pFrom->fSteering, pTo->fSteering, fShare);
    pOut->fWheelOffset = BlendKeyframeValue(pFrom->fWheelOffset, pTo->fWheelOffset, fShare);
    for (int i = 0; i < 4; ++i) {
        pOut->afWheelAngle[i] =
            BlendKeyframeValue(pFrom->afWheelAngle[i], pTo->afWheelAngle[i], fShare);
        pOut->afWheelContact[i] =
            BlendKeyframeValue(pFrom->afWheelContact[i], pTo->afWheelContact[i], fShare);
        pOut->afWheelSurface[i] =
            BlendKeyframeValue(pFrom->afWheelSurface[i], pTo->afWheelSurface[i], fShare);
    }
    pOut->fGearControl = BlendKeyframeValue(pFrom->fGearControl, pTo->fGearControl, fShare);
    pOut->fBraking = BlendKeyframeValue(pFrom->fBraking, pTo->fBraking, fShare);
    pOut->fReverse = BlendKeyframeValue(pFrom->fReverse, pTo->fReverse, fShare);
    pOut->fProgress = BlendKeyframeValue(pFrom->fProgress, pTo->fProgress, fShare);
    pOut->fSector = BlendKeyframeValue(pFrom->fSector, pTo->fSector, fShare);
    for (int i = 0; i < 2; ++i) {
        pOut->afLight[i] = BlendKeyframeValue(pFrom->afLight[i], pTo->afLight[i], fShare);
    }
    for (int i = 0; i < 5; ++i) {
        pOut->afEffectFlags[i] =
            BlendKeyframeValue(pFrom->afEffectFlags[i], pTo->afEffectFlags[i], fShare);
    }
    pOut->afEffectFlags[5] = pTo->afEffectFlags[5];
}

u8 *GetGhostKeyframeBuffer(void) {
    return g_aGhostKeyframes[0];
}

u8 *GetGhostPlaybackKeyframeBuffer(void) {
    return g_aGhostKeyframes[1];
}

unsigned GetGhostKeyframeDataSize(void) {
    return g_adwGhostFrameCount[0] * GHOST_FRAME_SIZE;
}

static inline u32 GhostChecksum(unsigned nSize, bool bIncludeTimes, const u8 *pFrames) {
    u32 dwChecksum = adler32(0, nullptr, 0);
    u8 abWord[4];
    if (bIncludeTimes) {
        memcpy(abWord, &g_fGhostBestLapTime, sizeof(abWord));
        dwChecksum = adler32(dwChecksum, abWord, sizeof(abWord));
        memcpy(abWord, &g_fGhostBestRaceTime, sizeof(abWord));
        dwChecksum = adler32(dwChecksum, abWord, sizeof(abWord));
    }
    u8 abInfo[sizeof(GhostRecordingInfo)];
    memcpy(abInfo, &g_GhostRecordingInfo, sizeof(abInfo));
    dwChecksum = adler32(dwChecksum, abInfo, sizeof(abInfo));
    return adler32(dwChecksum, pFrames, nSize);
}

static inline void ReadGhostSaveTrailer(FILE *pFile) {
    fseek(pFile, 0, SEEK_END);
    long nSettingsOffset = ftell(pFile) - 24 - SEASON_SAVE_SLOT_NAME_SIZE;
    fseek(pFile, nSettingsOffset, SEEK_SET);
    // The original accepts short reads in the settings and slot label.
    fread(&g_LoadedSeasonSettings.transmission, 4, 1, pFile);
    fread(&g_LoadedSeasonSettings.tires, 4, 1, pFile);
    fread(&g_LoadedSeasonSettings.suspension, 4, 1, pFile);
    fread(&g_LoadedSeasonSettings.car, 4, 1, pFile);
    fread(&g_LoadedSeasonSettings.handling, 4, 1, pFile);
    fread(&g_dwLoadedGhostLapCount, 4, 1, pFile);
    fseek(pFile, 0, SEEK_END);
    long nLabelOffset = ftell(pFile) - SEASON_SAVE_SLOT_NAME_SIZE;
    fseek(pFile, nLabelOffset, SEEK_SET);
    fread(g_szSeasonSaveSlotName, 1, sizeof(g_szSeasonSaveSlotName), pFile);
    memcpy(g_szSecondarySeasonSaveSlotName,
           g_szSeasonSaveSlotName,
           sizeof(g_szSecondarySeasonSaveSlotName));
}

static inline bool ReadGhostPayload(FILE *pFile, unsigned *pnLayout, unsigned *pnSize) {
    if (fread(g_abSaveFileReadBuffer, 1, sizeof(g_szGhostSaveMagic), pFile) !=
            sizeof(g_szGhostSaveMagic) ||
        memcmp(g_abSaveFileReadBuffer, g_szGhostSaveMagic, sizeof(g_szGhostSaveMagic)) != 0 ||
        fread(pnLayout, 4, 1, pFile) != 1) {
        return false;
    }
    unsigned nRemaining = *pnLayout;
    if (*pnLayout < 12) {
        if (*pnLayout != 0 || fread(&nRemaining, 1, 4, pFile) != 4) {
            return false;
        }
    }
    u32 dwChecksum;
    nRemaining -= 4;
    if (fread(&dwChecksum, 1, 4, pFile) != 4) {
        return false;
    }
    if (*pnLayout < 12) {
        nRemaining -= 4;
        if (fread(&g_fGhostBestLapTime, 1, 4, pFile) != 4) {
            return false;
        }
        nRemaining -= 4;
        if (fread(&g_fGhostBestRaceTime, 1, 4, pFile) != 4) {
            return false;
        }
    }
    nRemaining -= sizeof(GhostRecordingInfo);
    if (fread(&g_GhostRecordingInfo, 1, sizeof(g_GhostRecordingInfo), pFile) !=
            sizeof(g_GhostRecordingInfo) ||
        fread(GetGhostPlaybackKeyframeBuffer(), 1, nRemaining, pFile) != nRemaining) {
        return false;
    }
    *pnSize = nRemaining;
    return GhostChecksum(nRemaining, *pnLayout < 12, GetGhostPlaybackKeyframeBuffer()) ==
           dwChecksum;
}

bool ReadGhostFileData(const char *pszPath, bool bAllowFailure) {
    FILE *pFile = fopen(pszPath, "rb");
    unsigned nLayout;
    unsigned nSize;
    if (!pFile || !ReadGhostPayload(pFile, &nLayout, &nSize)) {
        if (pFile) {
            fclose(pFile);
        }
        g_fGhostBestRaceTime = 0;
        g_fGhostBestLapTime = 0;
        memset(&g_GhostRecordingInfo, 0xff, sizeof(g_GhostRecordingInfo));
        return bAllowFailure;
    }
    g_dwGhostRecordingSize = sizeof(GhostRecordingInfo);
    unsigned nFrames = SetGhostFrameCountFromSize(nSize);
    if (nLayout >= 12) {
        g_fGhostBestLapTime = 0;
        g_fGhostBestRaceTime = (static_cast<s32>(nFrames) - 204) * (1.0f / 30.0f);
    }
    g_pPrimarySeasonSave->unlockedTracks |= 1u
                                            << (static_cast<u8>(g_GhostRecordingInfo.track) & 31);
    g_pPrimarySeasonSave->unlocks |= 1u << (g_GhostRecordingInfo.car & 31);
    ReadGhostSaveTrailer(pFile);
    fclose(pFile);
    return true;
}

bool LoadGhostFile(bool bAllowFailure) {
    return ReadGhostFileData(g_szTimeAttackSaveFileName, bAllowFailure);
}

bool SaveGhostFile(void) {
    u32 dwChecksum = GhostChecksum(GetGhostKeyframeDataSize(), true, GetGhostKeyframeBuffer());
    unsigned nPayloadSize = GetGhostKeyframeDataSize() + g_dwGhostRecordingSize + 12;
    FILE *pFile = fopen(g_szTimeAttackSaveFileName, "wb");
    if (!pFile) {
        return false;
    }
    if (fwrite(g_szGhostSaveMagic, 1, sizeof(g_szGhostSaveMagic), pFile) !=
            sizeof(g_szGhostSaveMagic) ||
        fwrite(&g_dwGhostFileHeader, 4, 1, pFile) != 1 || fwrite(&nPayloadSize, 1, 4, pFile) != 4 ||
        fwrite(&dwChecksum, 1, 4, pFile) != 4 || fwrite(&g_fGhostBestLapTime, 4, 1, pFile) != 1 ||
        fwrite(&g_fGhostBestRaceTime, 4, 1, pFile) != 1 ||
        fwrite(&g_GhostRecordingInfo, 1, g_dwGhostRecordingSize, pFile) != g_dwGhostRecordingSize ||
        fwrite(GetGhostKeyframeBuffer(), 1, GetGhostKeyframeDataSize(), pFile) !=
            GetGhostKeyframeDataSize()) {
        fclose(pFile);
        return false;
    }
    // Failures in the six settings writes and final close do not affect success.
    fwrite(&g_dwSelectedTransmission, 4, 1, pFile);
    fwrite(&g_dwTireMenuIndex, 4, 1, pFile);
    fwrite(&g_dwSuspensionMenuIndex, 4, 1, pFile);
    fwrite(&g_dwCarMenuIndex, 4, 1, pFile);
    fwrite(&g_dwHandlingTypeMenuIndex, 4, 1, pFile);
    fwrite(&g_dwTotalLapsSetting, 4, 1, pFile);
    if (fwrite(g_szSeasonSaveSlotName, 1, sizeof(g_szSeasonSaveSlotName), pFile) !=
        sizeof(g_szSeasonSaveSlotName)) {
        fclose(pFile);
        return false;
    }
    fclose(pFile);
    return true;
}

void FillMemoryPatternBytes(void *pDest, size_t nCount, int nByte) {
    memset(pDest, nByte, nCount);
}

void StartGhostRecording(void) {
    g_bGhostRecordingActive = true;
}

int IsGhostRecordingActive(void) {
    return g_bGhostRecordingActive;
}

void AdvanceGhostRecordFrame(void) {
    if (!g_bGhostRecordingActive || g_dwRacePauseState) {
        return;
    }
    const bool singleRecording =
        g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_DEMO;
    const int count = singleRecording ? 1 : GHOST_RECORDING_COUNT;
    for (int i = 0; i < count; ++i) {
        if (static_cast<int>(g_adwGhostFrameCount[i]) < GHOST_MAX_FRAMES) {
            ++g_adwGhostFrameCount[i];
        }
    }
    if (singleRecording &&
        g_adwGhostPlaybackFrame[1] < static_cast<int>(g_adwGhostFrameCount[1] - 1)) {
        ++g_adwGhostPlaybackFrame[1];
    }
}

void ClearGhostPlaybackState(void) {
    memset(g_adwGhostPlaybackFrame, 0, sizeof(g_adwGhostPlaybackFrame));
}

void UpdateReplayScrubControls(void) {
    int step = 0;
    if (g_dwInputButtonMask & 0x200000) {
        gInstantReplayActive = 3;
        step = 1;
    } else if (g_dwInputButtonMask & 0x400000) {
        gInstantReplayActive = 3;
        step = -1;
    }
    if (g_dwInputButtonMask & 0x800000) {
        gInstantReplayActive = 3;
        step = 10;
    } else if (g_dwInputButtonMask & 0x1000000) {
        gInstantReplayActive = 3;
        step = -10;
    }
    if (g_dwInputButtonMask & 0x100000) {
        gInstantReplayActive = 1;
    }
    if (gInstantReplayActive == 1) {
        step = 1;
    } else if (gInstantReplayActive == 3 && step == 0) {
        gInstantReplayActive = 2;
    }
    for (int i = 0; i < GHOST_RECORDING_COUNT; ++i) {
        g_adwGhostPlaybackFrame[i] += step;
        if (g_adwGhostPlaybackFrame[i] < 0) {
            g_adwGhostPlaybackFrame[i] = 0;
        } else if (g_adwGhostPlaybackFrame[i] > static_cast<int>(g_adwGhostFrameCount[i] - 1)) {
            // Empty recordings can finish at frame -1 after the upper-bound comparison.
            g_adwGhostPlaybackFrame[i] = static_cast<int>(g_adwGhostFrameCount[i] - 1);
        }
    }
}

void ResetGhostRecording(void) {
    int nRecordings = GHOST_RECORDING_COUNT;
    if (g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_DEMO) {
        nRecordings = 1;
    }
    memset(g_adwGhostFrameCount, 0, nRecordings * sizeof(g_adwGhostFrameCount[0]));
    g_bGhostRecordingActive = false;
}

unsigned SetGhostFrameCountFromSize(unsigned dwSize) {
    g_adwGhostFrameCount[1] = dwSize / GHOST_FRAME_SIZE;
    return g_adwGhostFrameCount[1];
}

void MatrixToPositionQuaternion(QuaternionPose *pPose, f32 m[4][4]) {
    MatrixToQuaternion(m, &pPose->orientation);
    pPose->position.x = m[3][0];
    pPose->position.y = m[3][1];
    pPose->position.z = m[3][2];
}
