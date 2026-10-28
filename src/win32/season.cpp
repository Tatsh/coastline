#include "win32/season.h"

#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <zlib.h>

#include "game.h"
#include "profile.h"
#include "race.h"
#include "seasons.h"
#include "trackselect.h"
#include "weather.h"
#include "win32/boot/rallyconfig.h"
#include "win32/buildinfo.h"
#include "win32/configfile.h"
#if TGR_PLATFORM != TGR_PLATFORM_SDL
#include "win32/frontend.h"
#else
#include "win32/frontendstate.h"
#endif
#include "win32/ghost.h"
#include "win32/settings.h"

void ProcessSeasonResults(void) {
    if (g_dwGameMode != GAME_MODE_CHAMPIONSHIP || !g_dwSavedResultsValid) {
        return;
    }
    for (int i = 0; i < static_cast<int>(g_dwNumLocalPlayers); ++i) {
        PlayerProfile *profile = g_aVehicles[i].GetProfile();
        int points = 0;
        for (int race = 0; race < SEASON_RACES; ++race) {
            points += profile->seasonRacePoints[profile->season][race];
        }
        ++profile->round;
        if (profile->round == g_SeasonTable[profile->season].raceCount) {
            profile->round = 0;
            if (points >= g_SeasonTable[profile->season].pointsNeeded) {
                profile->unlocks |= g_SeasonTable[profile->season].unlocks;
                ++profile->season;
                if (profile->season == SEASONS) {
                    bool perfect = true;
                    for (int race = 0; race < g_SeasonTable[SEASONS - 1].raceCount; ++race) {
                        if (profile->seasonPlaces[SEASONS - 1][race] != 0) {
                            perfect = false;
                            break;
                        }
                    }
                    if (perfect) {
                        unsigned mine = (profile->championshipFlags & 1) ? TRACK_MIRROR_STRIP_MINE :
                                                                           TRACK_STRIP_MINE;
                        profile->unlockedTracks |= 1u << mine;
                    }
                    switch (profile->championshipFlags) {
                    case 0:
                        profile->unlocks |= 0x0400;
                        break;
                    case 1:
                        profile->unlocks |= 0x2200;
                        break;
                    case 2:
                        profile->unlocks |= 0x0800;
                        break;
                    case 3:
                        profile->unlocks |= 0x5000;
                        break;
                    case 4:
                        profile->unlocks |= 0x8000;
                        break;
                    }
                    profile->season = 0;
                    profile->championshipFlags =
                        std::bit_cast<s32>(static_cast<u32>(profile->championshipFlags) + 1u);
                }
            }
        }
        unsigned trackBase = (profile->championshipFlags & 1) ? TRACK_MIRRORED : 0;
        unsigned track = g_SeasonTable[profile->season].races[profile->round][0];
        unsigned weather = g_SeasonTable[profile->season].races[profile->round][1];
        profile->unlockedTracks |= 1u << ((track + trackBase) & 31u);
        profile->unlockedWeathers |= 1u << (weather & 31u);
    }
}

void ExitRaceAndProcessSeasonResults(void) {
    PrintWineDiagnostic("coastline: Leaving replay and processing race results.\n");
    std::memset(&g_SeasonResumeState, 0, sizeof(g_SeasonResumeState));
    g_SeasonResumeState.state = g_dwGameMode;
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        PlayerProfile *profile = g_pPrimarySeasonSave;
        g_SeasonResumeState.season = profile->season;
        g_SeasonResumeState.round = profile->round;
        g_SeasonResumeState.year = profile->championshipFlags;
        g_SeasonResumeState.track = g_dwSelectedTrackId;
        g_SeasonResumeState.weather = g_dwCurrentWeather;
        Vehicle &vehicle = g_aVehicles[0];
        SeasonRacePlayerResult &result = g_SeasonResumeState.player;
        result.resultValue = vehicle.GetLap();
        unsigned position =
            profile->seasonPlaces[g_SeasonResumeState.season][g_SeasonResumeState.round];
        result.position = position;
        result.points = static_cast<s8>(g_abRacePositionPoints[position]);
        result.raceTime = vehicle.GetTotalTime();
        std::memcpy(result.lapTimes, vehicle.GetLapTimes(), sizeof(result.lapTimes));
        std::memcpy(&g_SeasonProgressPrev, profile, sizeof(g_SeasonProgressPrev));
        g_dwNumLocalPlayers = 1;
        ProcessSeasonResults();
        PrintWineDiagnostic("coastline: Championship results processed.\n");
        std::memcpy(&g_SeasonProgress, g_pPrimarySeasonSave, sizeof(g_SeasonProgress));
        SetTrackFromSeason();
    }
    ShutdownGameSession();
}

void ShutdownGameSessionToSeasonMenu(void) {
    g_SeasonResumeState.state = SEASON_RESUME_STATE_RESTART;
    ShutdownGameSession();
}

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
int g_dwSeasonYear;
int g_dwSeasonRound;
s8 g_bSeasonClass;
SeasonResumeState g_SeasonResumeState;

void ExitRaceSessionToMenu(void) {
    memset(&g_SeasonResumeState, 0, sizeof(g_SeasonResumeState));
    g_SeasonResumeState.state = 8;
    ShutdownGameSession();
}
SeasonRaceResult g_LastRaceResultCopy;
float g_afLastRaceLapTimes[12];
u8 g_abSeasonRoundPlace[SEASON_RACES];
u16 g_awSeasonRoundPoints[SEASON_RACES];
char g_szSeasonYearLabel[SEASON_YEAR_LABEL_SIZE];
char g_szSeasonRoundLabel[SEASON_ROUND_LABEL_SIZE];
int g_dwSeasonSavedRound;
int g_dwLastRaceResultField18;
int g_dwSeasonTotalPoints;
PlayerProfile g_SeasonProgressPrev;
int g_dwUnusedSeasonMenuValue;
int g_dwRaceResultsSelectedLap;
int g_dwLoadSeasonSlotValid;
int g_dwSeasonLoaded;
int g_dwSeasonFailed;
int g_dwSeasonRoundQualified;
const char g_szSeasonSaveMagic[4] = {'R', 'S', 'e', 'a'};
const char g_szAutoSaveBrf[] = "AutoSave.brf";
char g_szSeasonSaveSlotName[SEASON_SAVE_SLOT_NAME_SIZE];
SeasonSaveSettings g_LoadedSeasonSettings;
char g_szSecondarySeasonSaveSlotName[SEASON_SAVE_SLOT_NAME_SIZE];
int g_dwSaveSlotPlayerIndex;
u8 g_abSaveFileReadBuffer[SEASON_SAVE_PAYLOAD_SIZE];

static inline void UpdateSeasonLabels(void) {
    sprintf(g_szSeasonYearLabel, "%d", g_dwSeasonYear + 1);
    sprintf(g_szSeasonRoundLabel, "%d", g_dwSeasonRound + 1);
}

static inline int CurrentSeasonPoints(void) {
    int nPoints = 0;
    for (int nRound = 0; nRound < SEASON_RACES; ++nRound) {
        nPoints += g_SeasonProgress.seasonRacePoints[g_bSeasonClass][nRound];
    }
    return nPoints;
}

void ApplyLoadedSeasonSettings(bool bUpdatePoints) {
    g_bSeasonClass = g_SeasonProgress.season;
    g_dwTransmissionType = g_SeasonProgress.transmission;
    g_dwSuspensionType = g_SeasonProgress.suspension;
    g_dwTireType = g_SeasonProgress.tires;
    g_dwSeasonYear = g_SeasonProgress.championshipFlags;
    g_dwSeasonRound = g_SeasonProgress.round;
    g_GlobalRallyConfig.dwInputDeviceType = g_SeasonProgress.controllerLayout;
    g_GlobalRallyConfig.SelectActiveInputBindings();
    UpdateSeasonLabels();
    g_dwSeasonSavedRound = g_dwSeasonRound;
    if (bUpdatePoints) {
        g_dwSeasonTotalPoints = CurrentSeasonPoints();
    }
    g_dwAccumUnlockedCarMask |= g_SeasonProgress.unlocks;
    g_dwAccumUnlockedTrackMask |= g_SeasonProgress.unlockedTracks;
}

static inline void ClearCurrentSeasonRecords(PlayerProfile *pProfile) {
    memset(pProfile->seasonPlaces[pProfile->season],
           0,
           sizeof(pProfile->seasonPlaces[pProfile->season]));
    memset(pProfile->seasonRacePoints[pProfile->season],
           0,
           sizeof(pProfile->seasonRacePoints[pProfile->season]));
    memset(pProfile->seasonTimes[pProfile->season],
           0,
           sizeof(pProfile->seasonTimes[pProfile->season]));
}

bool RestoreSeasonProgressState(void) {
    if (!g_SeasonResumeState.state) {
        memcpy(&g_LastRaceResultCopy, &g_SeasonResumeState, sizeof(g_LastRaceResultCopy));
    }
    g_dwChosenWeather = g_SeasonResumeState.weather;
    g_bSeasonClass = g_SeasonResumeState.season;
    g_dwSeasonYear = g_SeasonResumeState.year;
    g_dwSelectedTrackId = g_SeasonResumeState.track;
    g_dwLastRaceResultField18 = g_SeasonResumeState.player.resultValue;
    g_dwSeasonRound = g_SeasonResumeState.round;
    g_abSeasonRoundPlace[g_dwSeasonRound] =
        static_cast<unsigned>(g_SeasonResumeState.player.position) + 1;
    g_awSeasonRoundPoints[g_dwSeasonRound] = g_SeasonResumeState.player.points;
    g_fGhostBestRaceTime = g_SeasonResumeState.player.raceTime;
    UpdateSeasonLabels();
    memcpy(g_afLastRaceLapTimes, g_SeasonResumeState.player.lapTimes, sizeof(g_afLastRaceLapTimes));
    g_dwSeasonTotalPoints = CurrentSeasonPoints();
    if (g_SeasonProgress.round || g_dwGameMode == GAME_MODE_AFTER_SEASON ||
        g_SeasonResumeState.state) {
        return true;
    }
    u8 nPreviousClass = g_SeasonProgressPrev.season;
    if (g_SeasonProgress.season <= nPreviousClass &&
        (g_SeasonProgress.season != 0 || nPreviousClass == 0)) {
        g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
        g_dwSeasonRoundQualified = true;
        ClearCurrentSeasonRecords(&g_SeasonProgress);
        ClearCurrentSeasonRecords(g_pPrimarySeasonSave);
        ClearCurrentSeasonRecords(g_pSecondarySeasonSave);
        return true;
    }
    g_dwGameMode = GAME_MODE_AFTER_SEASON;
    if (!g_SeasonProgress.season && nPreviousClass > 0) {
        if (g_SeasonResumeState.year < 5) {
            g_dwSeasonFailed = true;
        }
        ApplyLoadedSeasonSettings(false);
    } else {
        ApplyLoadedSeasonSettings(true);
    }
    if (g_SeasonResumeState.season < 4 && g_SeasonResumeState.year < 1) {
        g_dwSeasonFailed = true;
    }
#if TGR_PLATFORM != TGR_PLATFORM_SDL
    // The SDL game has no menus of the PC release to close.
    g_pActiveMenuScreen->SetRunning(false);
    g_pActiveMenuScreen->ShutdownFrontEnd(false);
#endif
    return false;
}

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

// The menus of the PC release load seasons through these, and the SDL game has the menus of the
// Nintendo 64 release in their place.
#if TGR_PLATFORM != TGR_PLATFORM_SDL
static inline void ResetSeasonMenuForLoad(void) {
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    ResetMenuSelectionsToDefaults();
    g_dwSaveSlotPlayerIndex = 0;
    strcpy(g_szSeasonYearLabel, "");
    strcpy(g_szSeasonRoundLabel, "");
}

static inline bool LoadSeasonMenuFile(const char *pszFileName) {
    FILE *file = fopen(pszFileName, "r");
    if (!file) {
        return false;
    }
    fclose(file);
    strcpy(g_szSeasonSaveFileName, pszFileName);
    if (!LoadConfigData(CONFIG_LOAD_SEASON_FILE, true)) {
        ShowErrorMessage(7);
    }
    g_SeasonProgress = *g_pPrimarySeasonSave;
    g_dwLoadSeasonSlotValid = true;
    const int year = g_pPrimarySeasonSave->championshipFlags;
    const int round = g_pPrimarySeasonSave->round;
    g_dwSeasonYear = year;
    g_dwSeasonSavedRound = round;
    g_dwSeasonRound = round;
    g_bSeasonClass = g_pPrimarySeasonSave->season;
    g_dwSelectedTransmission = g_LoadedSeasonSettings.transmission;
    g_dwTireMenuIndex = g_LoadedSeasonSettings.tires;
    g_dwSuspensionMenuIndex = g_LoadedSeasonSettings.suspension;
    g_dwCarMenuIndex = g_LoadedSeasonSettings.car;
    g_dwHandlingTypeMenuIndex = g_LoadedSeasonSettings.handling;
    g_dwSeasonLoaded = true;
    BackupMenuSelections();
    ResolveRaceSelections();
    g_dwSeasonTotalPoints = CurrentSeasonPoints();
    memcpy(g_abSeasonRoundPlace,
           g_SeasonProgress.seasonPlaces[g_bSeasonClass],
           sizeof(g_abSeasonRoundPlace));
    for (int race = 0; race < g_dwSeasonRound; ++race) {
        ++g_abSeasonRoundPlace[race];
    }
    memcpy(g_awSeasonRoundPoints,
           g_SeasonProgress.seasonRacePoints[g_bSeasonClass],
           sizeof(g_awSeasonRoundPoints));
    sprintf(g_szSeasonYearLabel, "%d", year + 1);
    sprintf(g_szSeasonRoundLabel, "%d", round + 1);
    return true;
}

void LoadAutoSaveSeason(void) {
    ResetSeasonMenuForLoad();
    char fileName[260];
    strcpy(fileName, g_szAutoSaveBrf);
    LoadSeasonMenuFile(fileName);
}

int LoadSeasonUI_OnSlotSelected(MenuItemList *pUnused, const int *pnSlot) {
    g_dwLoadSeasonSlotValid = 0;
    ResetSeasonMenuForLoad();
    char fileName[260];
    char number[8];
    strcpy(fileName, "RallySeason");
    _itoa(*pnSlot, number, 10);
    strcat(fileName, number);
    strcat(fileName, ".brf");
    return LoadSeasonMenuFile(fileName);
}
#endif /* TGR_PLATFORM != TGR_PLATFORM_SDL */

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
        g_aVehicles[0].SetProfile(&g_aSeasonSaveBuffers[0]);
    }
    memset(g_pPrimarySeasonSave, 0, sizeof(*g_pPrimarySeasonSave));
    if (!g_pSecondarySeasonSave) {
        g_aVehicles[1].SetProfile(&g_aSeasonSaveBuffers[1]);
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
