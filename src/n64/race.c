#include "common/race.h"

#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_libc.h>

#include "carselect.h"
#include "common/audio.h"
#include "common/audiomix.h"
#include "common/camera.h"
#include "common/driving.h"
#include "common/effects.h"
#include "common/fade.h"
#include "common/game.h"
#include "common/hud.h"
#include "common/input.h"
#include "common/physics.h"
#include "common/racecontrols.h"
#include "common/racefinish.h"
#include "common/raceinit.h"
#include "common/raceload.h"
#include "common/racephases.h"
#include "common/racerender.h"
#include "common/racespecials.h"
#include "common/render.h"
#include "common/replay.h"
#include "common/rumble.h"
#include "common/sky.h"
#include "common/splash.h"
#include "common/system.h"
#include "common/text.h"
#include "common/timing.h"
#include "common/track.h"
#include "common/trackrender.h"
#include "common/trackselect.h"
#include "common/vecmath.h"
#include "common/vehicle.h"
#include "common/vehiclerender.h"
#include "common/visgrid.h"
#include "common/weather.h"
#include "mainmenu.h"
#include "options.h"
#include "racemenu.h"

void LoadAndPlayTrackBgm(void) {
    ResetMusicPlaybackState();
    DecompressFromRom(g_abMenuMusicModule, g_adwTrackMusicRom[g_dwSelectedTrackId], NULL);
    InitMusicPlayback(g_abMenuMusicModule, g_abMusicModuleWorkBuffer);
}

void StartAttractDemo(void) {
    g_dwGameMode = GAME_MODE_DEMO;
    g_dwRaceSessionType = RACE_SESSION_ATTRACT;
    SetCurrentScreenCallback(ProcessRacingScreen);
}

void StartCreditsRace(void) {
    g_dwGameMode = GAME_MODE_DEMO;
    if (g_dwMultiplayerMode) {
        g_dwRaceSessionType = RACE_SESSION_SCROLLING_CREDITS;
    } else {
        g_dwRaceSessionType = RACE_SESSION_CREDIT_CAPTIONS;
    }
    SetCurrentScreenCallback(ProcessRacingScreen);
}

/* Add a line of the bonus of a finished championship, after a blank line when others precede it. */
#define ADD_BONUS_LINE(line)                                                                       \
    {                                                                                              \
        if (count) {                                                                               \
            g_apszSeasonResultMsgLines[count++] = "";                                              \
        }                                                                                          \
        g_apszSeasonResultMsgLines[count++] = line;                                                \
    }

void ProcessSeasonResults(void) {
    s32 count, i, race, base;

    count = 0;
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP && g_dwSavedResultsValid) {
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            g_aVehicles[i].profile->round++;
            if (g_aVehicles[i].profile->round ==
                g_SeasonTable[g_aVehicles[i].profile->season].raceCount) {
                g_aVehicles[i].profile->round = 0;
                if (g_aVehicles[i].profile->seasonPoints[g_aVehicles[i].profile->season] >=
                    g_SeasonTable[g_aVehicles[i].profile->season].pointsNeeded) {
                    g_aVehicles[i].profile->unlocks |=
                        g_SeasonTable[g_aVehicles[i].profile->season].unlocks;
                    g_aVehicles[i].profile->season++;
                    if (g_aVehicles[i].profile->season == SEASONS) {
                        g_apszSeasonResultMsgLines[count++] = "ALL SEASONS COMPLETED!";
                        /* Winning every race of the last season adds the strip mine. */
                        for (race = 0; race < g_SeasonTable[SEASONS - 1].raceCount; race++) {
                            if (g_aVehicles[i].profile->seasonPlaces[SEASONS - 1][race] != 0) {
                                goto cars;
                            }
                        }
                        if (g_aVehicles[i].profile->championshipFlags & 1) {
                            if (!(g_aVehicles[i].profile->unlockedTracks &
                                  (1 << TRACK_MIRROR_STRIP_MINE))) {
                                ADD_BONUS_LINE("You Won all races this season!");
                                g_apszSeasonResultMsgLines[count++] =
                                    "Bonus: Mirror Mine Track Added!";
                                g_aVehicles[i].profile->unlockedTracks |=
                                    1 << TRACK_MIRROR_STRIP_MINE;
                            }
                        } else if (!(g_aVehicles[i].profile->unlockedTracks &
                                     (1 << TRACK_STRIP_MINE))) {
                            ADD_BONUS_LINE("You Won all races this season!");
                            g_apszSeasonResultMsgLines[count++] = "Bonus: Mine Track Added!";
                            g_aVehicles[i].profile->unlockedTracks |= 1 << TRACK_STRIP_MINE;
                        }
                    cars:
                        /* Each championship of the years unlocks a car. */
                        if (g_aVehicles[i].profile->championshipFlags == 0) {
                            ADD_BONUS_LINE("Bonus: Milk Truck Added!");
                            g_aVehicles[i].profile->unlocks |= 0x400;
                        } else if (g_aVehicles[i].profile->championshipFlags == 1) {
#if TGR_REGION == TGR_REGION_JP
                            ADD_BONUS_LINE("Bonus: Type-MN Car Added!");
#else
                            ADD_BONUS_LINE("Bonus: Helmet Car Added!");
#endif
                            g_aVehicles[i].profile->unlocks |= 0x200;
                        } else if (g_aVehicles[i].profile->championshipFlags == 2) {
                            ADD_BONUS_LINE("Bonus: Cupra Car Added!");
                            g_aVehicles[i].profile->unlocks |= 0x800;
                        } else if (g_aVehicles[i].profile->championshipFlags == 3) {
                            ADD_BONUS_LINE("Bonus: Beach Ball Car Added!");
                            g_aVehicles[i].profile->unlocks |= 0x1000;
                        } else if (g_aVehicles[i].profile->championshipFlags == 4) {
                            ADD_BONUS_LINE("Bonus: Mirrored Cars Added!");
                            g_aVehicles[i].profile->unlocks |= 0x8000;
                        }
                        g_aVehicles[i].profile->season = 0;
                        g_aVehicles[i].profile->championshipFlags++;
                    } else {
#if TGR_REGION == TGR_REGION_JP
                        g_apszSeasonResultMsgLines[count++] = "SEASON COMPLETE";
#else
                        g_apszSeasonResultMsgLines[count++] = "SEASON COMPLETED!";
                        g_apszSeasonResultMsgLines[count++] = "";
                        g_apszSeasonResultMsgLines[count++] = "PROGRESS TO THE NEXT SEASON";
#endif
                    }
                } else {
#if TGR_REGION == TGR_REGION_JP
                    g_apszSeasonResultMsgLines[count++] = "SEASON TRY AGAIN";
#else
                    g_apszSeasonResultMsgLines[count++] = "Sorry, you don't have enough points to";
                    g_apszSeasonResultMsgLines[count++] =
                        "pass this season.  You'll have to try again.";
#endif
                }
                g_aVehicles[i].profile->seasonPoints[g_aVehicles[i].profile->season] = 0;
            }
            /* The track and the weather of the next race become available. */
            base = g_aVehicles[i].profile->championshipFlags & 1 ? TRACK_MIRRORED : 0;
            g_aVehicles[i].profile->unlockedTracks |=
                1 << (g_SeasonTable[g_aVehicles[i].profile->season]
                          .races[g_aVehicles[i].profile->round][0] +
                      base);
            g_aVehicles[i].profile->unlockedWeathers |=
                1 << g_SeasonTable[g_aVehicles[i].profile->season]
                         .races[g_aVehicles[i].profile->round][1];
        }
    }
    g_dwSeasonResultMsgLineCount = count;
}
