#include "platform/win/season.h"

#include <cstring>

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
