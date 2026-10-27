#include "platform/win/season.h"

#include <cstdio>
#include <cstring>

#include <zlib.h>

#include "game.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/frontend.h"
#include "platform/win/settings.h"
#include "profile.h"
#include "seasons.h"
#include "trackselect.h"

Season g_SeasonTable[SEASONS] = {
    {0xdf, 2, 10, {1, 8}, 0x0081, {{2, 0}, {4, 0}, {0, 0}, {0, 0}}},
    {0xe0, 3, 15, {0, 7}, 0x0050, {{0, 0}, {2, 0}, {4, 2}, {0, 0}}},
    {0xe1, 4, 20, {4, 6}, 0x000c, {{4, 1}, {0, 1}, {2, 2}, {1, 0}}},
    {0xe2, 4, 20, {3, 2}, 0x0020, {{4, 3}, {1, 1}, {2, 4}, {0, 2}}},
    {0xe3, 4, 20, {2, 5}, 0x0020, {{2, 3}, {0, 4}, {10, 3}, {1, 2}}},
    {0xe4, 4, 20, {5, 5}, 0x0020, {{6, 1}, {2, 2}, {1, 4}, {10, 2}}},
};

PlayerProfile g_SeasonProgress;
PlayerProfile g_aSeasonSaveBuffers[2];
PlayerProfile *g_pPrimarySeasonSave;
PlayerProfile *g_pSecondarySeasonSave;
int g_dwSeasonYear;
int g_dwSeasonRound;
int g_bSeasonClass;
const char g_szSeasonSaveMagic[4] = {'R', 'S', 'e', 'a'};
const char g_szAutoSaveBrf[] = "AutoSave.brf";
char g_szSeasonSaveSlotName[SEASON_SAVE_SLOT_NAME_SIZE];
SeasonSaveSettings g_LoadedSeasonSettings;
char g_szSecondarySeasonSaveSlotName[SEASON_SAVE_SLOT_NAME_SIZE];
int g_dwSaveSlotPlayerIndex;
u8 g_abSaveFileReadBuffer[SEASON_SAVE_PAYLOAD_SIZE];

static inline void ReadSeasonSaveTrailer(FILE *pFile) {
    fseek(pFile, 0, SEEK_END);
    long nSettingsOffset = ftell(pFile) - sizeof(SeasonSaveSettings) - SEASON_SAVE_SLOT_NAME_SIZE;
    fseek(pFile, nSettingsOffset, SEEK_SET);
    // The original accepts short reads in the settings and slot label.
    fread(&g_LoadedSeasonSettings.transmission, 4, 1, pFile);
    fread(&g_LoadedSeasonSettings.tires, 4, 1, pFile);
    fread(&g_LoadedSeasonSettings.suspension, 4, 1, pFile);
    fread(&g_LoadedSeasonSettings.car, 4, 1, pFile);
    fread(&g_LoadedSeasonSettings.handling, 4, 1, pFile);
    fseek(pFile, 0, SEEK_END);
    long nLabelOffset = ftell(pFile) - SEASON_SAVE_SLOT_NAME_SIZE;
    fseek(pFile, nLabelOffset, SEEK_SET);
    fread(g_szSeasonSaveSlotName, 1, sizeof(g_szSeasonSaveSlotName), pFile);
    memcpy(g_szSecondarySeasonSaveSlotName,
           g_szSeasonSaveSlotName,
           sizeof(g_szSecondarySeasonSaveSlotName));
}

static inline void ApplySeasonProfilePreservingOtherPlayer(void) {
    PlayerProfile *pOtherProfile =
        g_dwSaveSlotPlayerIndex == 0 ? g_pSecondarySeasonSave : g_pPrimarySeasonSave;
    int nHandling = pOtherProfile->handling;
    int nTransmission = pOtherProfile->transmission;
    int nTires = pOtherProfile->tires;
    int nSuspension = pOtherProfile->suspension;
    int nControllerLayout = pOtherProfile->controllerLayout;
    memcpy(g_pPrimarySeasonSave, g_abSaveFileReadBuffer, sizeof(*g_pPrimarySeasonSave));
    memcpy(g_pSecondarySeasonSave, g_abSaveFileReadBuffer, sizeof(*g_pSecondarySeasonSave));
    pOtherProfile->handling = nHandling;
    pOtherProfile->transmission = nTransmission;
    pOtherProfile->tires = nTires;
    pOtherProfile->suspension = nSuspension;
    pOtherProfile->controllerLayout = nControllerLayout;
}

bool LoadSeasonSaveFile(int nMode, bool bAllowFailure) {
    FILE *pFile = nullptr;
    if (nMode == SEASON_LOAD_CURRENT) {
        memcpy(g_abSaveFileReadBuffer, &g_SeasonProgress, sizeof(g_SeasonProgress));
    } else {
        pFile = fopen(g_szSeasonSaveFileName, "rb");
        if (!pFile) {
            return bAllowFailure;
        }
        u32 dwChecksum;
        if (fread(g_abSaveFileReadBuffer, 1, sizeof(g_szSeasonSaveMagic), pFile) !=
                sizeof(g_szSeasonSaveMagic) ||
            memcmp(g_abSaveFileReadBuffer, g_szSeasonSaveMagic, sizeof(g_szSeasonSaveMagic)) != 0 ||
            fread(&dwChecksum, 1, sizeof(dwChecksum), pFile) != sizeof(dwChecksum) ||
            fread(g_abSaveFileReadBuffer, 1, sizeof(g_abSaveFileReadBuffer), pFile) !=
                sizeof(g_abSaveFileReadBuffer) ||
            adler32(adler32(0, nullptr, 0),
                    g_abSaveFileReadBuffer,
                    sizeof(g_abSaveFileReadBuffer)) != dwChecksum) {
            fclose(pFile);
            return bAllowFailure;
        }
    }
    if (nMode == SEASON_LOAD_FILE) {
        if (!ResetSeasonSaveBuffers()) {
            return false;
        }
        if (!g_pPrimarySeasonSave || !g_pSecondarySeasonSave) {
            return false;
        }
        memcpy(g_pPrimarySeasonSave, g_abSaveFileReadBuffer, sizeof(*g_pPrimarySeasonSave));
        memcpy(g_pSecondarySeasonSave, g_abSaveFileReadBuffer, sizeof(*g_pSecondarySeasonSave));
        ReadSeasonSaveTrailer(pFile);
    } else {
        ApplySeasonProfilePreservingOtherPlayer();
    }
    if (nMode != SEASON_LOAD_CURRENT) {
        fclose(pFile);
    }
    return true;
}

bool SaveSeasonSaveFile(void) {
    u32 dwChecksum = adler32(0, nullptr, 0);
    if (g_pPrimarySeasonSave) {
        u8 abProfileBytes[SEASON_SAVE_PAYLOAD_SIZE];
        // The saved payload extends beyond the 332-byte profile into adjacent memory.
        memcpy(abProfileBytes, g_pPrimarySeasonSave, sizeof(abProfileBytes));
        dwChecksum = adler32(dwChecksum, abProfileBytes, sizeof(abProfileBytes));
    }
    FILE *pFile = fopen(g_szSeasonSaveFileName, "wb");
    if (!pFile) {
        return false;
    }
    if (fwrite(g_szSeasonSaveMagic, 1, sizeof(g_szSeasonSaveMagic), pFile) !=
            sizeof(g_szSeasonSaveMagic) ||
        fwrite(&dwChecksum, 1, sizeof(dwChecksum), pFile) != sizeof(dwChecksum) ||
        fwrite(g_pPrimarySeasonSave, 1, SEASON_SAVE_PAYLOAD_SIZE, pFile) !=
            SEASON_SAVE_PAYLOAD_SIZE) {
        fclose(pFile);
        return false;
    }
    // The original ignores failures in the five settings writes and the final close.
    fwrite(&g_dwSelectedTransmission, 4, 1, pFile);
    fwrite(&g_dwTireMenuIndex, 4, 1, pFile);
    fwrite(&g_dwSuspensionMenuIndex, 4, 1, pFile);
    fwrite(&g_dwCarMenuIndex, 4, 1, pFile);
    fwrite(&g_dwHandlingTypeMenuIndex, 4, 1, pFile);
    if (fwrite(g_szSeasonSaveSlotName, 1, sizeof(g_szSeasonSaveSlotName), pFile) !=
        sizeof(g_szSeasonSaveSlotName)) {
        fclose(pFile);
        return false;
    }
    fclose(pFile);
    return true;
}

void SaveAutoSaveSeason(void) {
    if (!g_pPrimarySeasonSave) {
        return;
    }
    strcpy(g_szSeasonSaveFileName, g_szAutoSaveBrf);
    if (!g_pPrimarySeasonSave->season && !g_pPrimarySeasonSave->round) {
        memset(g_pPrimarySeasonSave->seasonPlaces, 0, sizeof(g_pPrimarySeasonSave->seasonPlaces));
        memset(g_pPrimarySeasonSave->seasonRacePoints,
               0,
               sizeof(g_pPrimarySeasonSave->seasonRacePoints));
        memset(g_pPrimarySeasonSave->seasonTimes, 0, sizeof(g_pPrimarySeasonSave->seasonTimes));
    }
    SaveSeasonSaveFile();
}

int ResetSeasonSaveBuffers(void) {
    if (!g_pPrimarySeasonSave) {
        g_pPrimarySeasonSave = &g_aSeasonSaveBuffers[0];
    }
    memset(g_pPrimarySeasonSave, 0, sizeof(*g_pPrimarySeasonSave));
    if (!g_pSecondarySeasonSave) {
        g_pSecondarySeasonSave = &g_aSeasonSaveBuffers[1];
    }
    memset(g_pSecondarySeasonSave, 0, sizeof(*g_pSecondarySeasonSave));
    return 1;
}

void SyncSeasonProgress(void) {
    g_SeasonProgress.transmission = g_dwTransmissionType;
    g_SeasonProgress.tires = g_dwTireType;
    g_SeasonProgress.suspension = g_dwSuspensionType;
    g_SeasonProgress.championshipFlags = g_dwSeasonYear;
    g_SeasonProgress.season = g_bSeasonClass;
    g_SeasonProgress.round = g_dwSeasonRound;
    g_SeasonProgress.controllerLayout = g_GlobalRallyConfig.dwInputDeviceType;
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        const u8 *pRace = g_SeasonTable[g_SeasonProgress.season].races[g_SeasonProgress.round];
        g_dwSelectedTrackId = pRace[0];
        g_dwChosenWeather = pRace[1];
    }
    g_dwAccumUnlockedCarMask |= g_SeasonProgress.unlocks;
    g_dwAccumUnlockedTrackMask |= g_SeasonProgress.unlockedTracks;
}
