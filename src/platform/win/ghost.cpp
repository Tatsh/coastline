#include "platform/win/ghost.h"

#include <cstdio>
#include <cstring>

#include <zlib.h>

#include "game.h"
#include "platform/win/frontend.h"
#include "platform/win/season.h"

unsigned g_adwGhostFrameCount[GHOST_RECORDING_COUNT];
int g_bGhostRecordingActive;
GhostRecordingInfo g_GhostRecordingInfo;
unsigned g_dwGhostRecordingSize;
u8 g_aGhostKeyframes[GHOST_RECORDING_COUNT][GHOST_MAX_FRAMES * GHOST_FRAME_SIZE];
float g_fGhostBestRaceTime;
float g_fGhostBestLapTime;
int g_dwLoadedGhostLapCount;
const char g_szGhostSaveMagic[4] = {'R', 'G', 'h', 'o'};
const unsigned g_dwGhostFileHeader = 0;

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
