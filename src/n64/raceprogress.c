#include <PR/os_libc.h>

#include "common/game.h"
#include "common/instantreplay.h"
#include "common/laptime.h"
#include "common/mathutil.h"
#include "common/profile.h"
#include "common/race.h"
#include "common/racers.h"
#include "common/soundeffect.h"
#include "common/trackmenuitem.h"
#include "common/trackselect.h"
#include "common/vecmath.h"
#include "common/vehiclecount.h"
#include "common/weather.h"

void UpdateRaceProgress(RacerSlot *slot) {
    Vehicle *vehicle;
    Vehicle *c;
    char *lapMessage;
    /* These locals preserve the original stack layout under IDO. */
    int stackPad74;
    int stackPad70;
    int currentGate;
    int nextGate;
    float completedLapTime;
    float d;
    int stackPad58;
    float paceGap;
    int stackPad50;
    int i;
    int stackPad48;
    short w;

    if (g_LoadedTrack.gateCount == 0) {
        return;
    }
    vehicle = slot->vehicle;
    if (vehicle != 0) {
        slot->position.x = vehicle->frame.position.x;
        slot->position.y = vehicle->frame.position.y;
        slot->position.z = vehicle->frame.position.z;
        slot->previousPosition.x = vehicle->previousPosition.x;
        slot->previousPosition.y = vehicle->previousPosition.y;
        slot->previousPosition.z = vehicle->previousPosition.z;
        slot->maxGate = vehicle->maxGate;
        slot->gate = vehicle->gate;
        slot->lap = vehicle->lap;
        slot->distanceLaps = vehicle->distanceLaps;
        slot->lapTime = vehicle->lapTime;
        slot->bestLapTime = vehicle->bestLapTime;
        slot->distance = vehicle->distance;
    }
    if (slot->gate < 0) {
        currentGate = (g_LoadedTrack.gateCount - (-1 - slot->gate) % g_LoadedTrack.gateCount - 1) %
                      g_LoadedTrack.gateCount;
    } else {
        currentGate = slot->gate % g_LoadedTrack.gateCount;
    }
    nextGate = (currentGate + 1) % g_LoadedTrack.gateCount;
    completedLapTime = FloatToInt(slot->lapTime * 100.0f) / 100.0f;
    if (TestLineSegmentIntersection(&g_LoadedTrack.gates[currentGate].ends[1],
                                    &g_LoadedTrack.gates[currentGate].ends[0],
                                    (Vec2f *)&slot->previousPosition,
                                    (Vec2f *)&slot->position)) {
        osSyncPrintf("%d went backwards, from gate %d to gate %d\n",
                     slot->index,
                     slot->gate,
                     slot->gate - 1);
        if (currentGate == 0) {
            slot->distanceLaps--;
        }
        osSyncPrintf("Hmm %d == %d %% %d && %d < 0 ? %d\n",
                     currentGate,
                     slot->maxGate,
                     g_LoadedTrack.gateCount,
                     slot->gate,
                     (slot->gate + g_LoadedTrack.gateCount * 2) % g_LoadedTrack.gateCount ==
                             slot->maxGate % g_LoadedTrack.gateCount &&
                         slot->gate < 0);
        if (currentGate == slot->maxGate % g_LoadedTrack.gateCount && slot->gate < 0) {
            slot->gate += g_LoadedTrack.gateCount;
            slot->distanceLaps++;
            if (g_LoadedTrack.rootSection != 0) {
                slot->distance += g_LoadedTrack.rootSection->points[0].distance;
            }
            osSyncPrintf("lapping %d forward to -1\n", slot->index);
        }
        slot->gate--;
        goto tail;
    }
    if (!TestLineSegmentIntersection(&g_LoadedTrack.gates[nextGate].ends[1],
                                     &g_LoadedTrack.gates[nextGate].ends[0],
                                     (Vec2f *)&slot->previousPosition,
                                     (Vec2f *)&slot->position)) {
        goto tail;
    }
    osSyncPrintf(
        "%d went forward, from gate %d to gate %d\n", slot->index, slot->gate, slot->gate + 1);
    slot->gate++;
    if (slot->maxGate < slot->gate) {
        slot->maxGate = slot->gate;
        osSyncPrintf("moved ahead one gate, getting %f seconds\n",
                     g_LoadedTrack.gates[nextGate].seconds);
        if (nextGate == 0) {
            if (slot->lap < g_dwTotalLaps) {
                lapMessage = 0;
                if (slot->bestLapTime == 0.0f || completedLapTime < slot->bestLapTime) {
                    slot->bestLapTime = completedLapTime;
                    if (slot->index < (s32)g_dwNumLocalPlayers) {
                        slot->vehicle->bestLap = slot->lap;
                        if (g_aVehicles[0].profile->lapRecords[g_dwSelectedTrackId] == 0.0f ||
                            slot->bestLapTime <
                                g_aVehicles[0].profile->lapRecords[g_dwSelectedTrackId]) {
                            g_aVehicles[0].profile->lapRecords[g_dwSelectedTrackId] =
                                slot->bestLapTime;
                            lapMessage = "%ryNEW LAP RECORD!";
                        } else if (slot->lap != 0) {
                            lapMessage = "%ryBEST LAP!";
                        }
                    }
                }
                if (vehicle != 0) {
                    osSyncPrintf("veh->lapTimeFinal[%d]=%f\n", slot->lap, completedLapTime);
                    vehicle->lapTimes[slot->lap] = completedLapTime;
                }
                slot->lapTime -= completedLapTime;
                slot->lap++;
                slot->distanceLaps = slot->lap;
                if (g_dwGameMode == 3) {
                    if (slot->lap == 1) {
                        osSyncPrintf("lapping %d back to 0 (a)\n", slot->index);
                        slot->lap--;
                        slot->distanceLaps--;
                        slot->maxGate -= g_LoadedTrack.gateCount;
                        slot->gate -= g_LoadedTrack.gateCount;
                        if (g_LoadedTrack.rootSection != 0) {
                            slot->distance -= g_LoadedTrack.rootSection->points[0].distance;
                        }
                    }
                } else if (vehicle != 0) {
                    osSyncPrintf("******* LAP #%d ********\n", slot->lap);
                    sprintf(vehicle->messageText, "%%ryLAP %d", slot->lap + 1);
                    vehicle->message = vehicle->messageText;
                    vehicle->messageTime = 1.0f;
                    if (lapMessage != 0) {
                        vehicle->subMessage = lapMessage;
                        vehicle->subMessageTime = 1.0f;
                    }
                }
            }
            if (slot->lap == g_dwTotalLaps) {
                slot->status |= 2;
                if (vehicle != 0) {
                    vehicle->opacity = 0.9f;
                    vehicle->totalTime = vehicle->totalTime - slot->lapTime;
                    slot->lapTime = 0.0f;
                    vehicle->racePosition = g_dwRaceFinishOrderCounter;
                    vehicle->message = g_apszRaceProgressStrings[g_dwRaceFinishOrderCounter];
                    vehicle->messageTime = 5.0f;
                    if (slot->index < (s32)g_dwNumLocalPlayers && g_dwTotalLaps == 3) {
                        if (g_aVehicles[0].profile->raceRecords[g_dwSelectedTrackId] == 0.0f ||
                            vehicle->totalTime <
                                g_aVehicles[0].profile->raceRecords[g_dwSelectedTrackId]) {
                            g_aVehicles[0].profile->raceRecords[g_dwSelectedTrackId] =
                                vehicle->totalTime;
                            vehicle->messageTime = 1.5f;
                            vehicle->subMessage = "%ryNEW COURSE RECORD!";
                            vehicle->subMessageTime = 3.5f;
                        }
                    }
                }
                g_dwRaceFinishOrderCounter++;
                goto tail;
            }
        }
        if (g_dwGameMode == 1 && slot->index < (s32)g_dwNumLocalPlayers &&
            0.0f < slot->vehicle->timeLeft) {
            if (gInstantReplayActive == 0) {
                PlayBeepSound();
            }
            d = g_aVehicles[0].distance - g_aVehicles[1].distance;
            if (d < 0) {
                d = -d;
            }
            paceGap = slot->vehicle->odometer / (slot->vehicle->totalTime * 2.24f);
            if (paceGap != 0.0f) {
                paceGap = d / paceGap;
            } else {
                paceGap = 1000.0f;
            }
            for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
                c = g_aRacerSlots[i].vehicle;
                c->message = "%ryCheck Point!";
                c->messageTime = 0.4f;
                w = g_dwCurrentWeather - 1;
                if (w >= 3 || w < 0) {
                    w = 0;
                }
                c->timeLeft += ((TrackMenuItem *)g_apTrackMenuItems[g_dwSelectedTrackId])
                                   ->arcadeTimes[g_aVehicles[0].tableIndex][w]
                                   .gateBonuses[nextGate];
                if (g_dwNumLocalPlayers == 1) {
                    sprintf(c->messageText, "%%ry");
                    FormatLapTime(c->messageText + 3, c->timeLeft);
                    sprintf(c->messageText + strlen(c->messageText), " Left");
                    c->subMessage = c->messageText;
                    c->subMessageTime = 0.9f;
                } else if (c->totalTime != 0.0f) {
                    sprintf(c->messageText, "%%ry");
                    FormatLapTime(c->messageText + 3, paceGap);
                    if (c->racePosition != 0) {
                        if (120.0f < paceGap) {
                            c->subMessage = "%ryYou're way behind!";
                        }
                        sprintf(c->messageText + strlen(c->messageText), " Behind");
                        c->subMessage = c->messageText;
                    } else {
                        sprintf(c->messageText + strlen(c->messageText), " Ahead");
                        c->subMessage = c->messageText;
                    }
                    c->subMessageTime = 0.9f;
                }
                c->maxGate = slot->maxGate;
            }
        }
        goto tail;
    }
    if (nextGate != 0) {
        goto tail;
    }
    if (slot->lap < ++slot->distanceLaps) {
        lapMessage = 0;
        if (slot->bestLapTime == 0.0f || completedLapTime < slot->bestLapTime) {
            slot->bestLapTime = completedLapTime;
            if (slot->index < (s32)g_dwNumLocalPlayers) {
                slot->vehicle->bestLap = slot->lap;
                if (g_aVehicles[0].profile->lapRecords[g_dwSelectedTrackId] == 0.0f ||
                    slot->bestLapTime < g_aVehicles[0].profile->lapRecords[g_dwSelectedTrackId]) {
                    g_aVehicles[0].profile->lapRecords[g_dwSelectedTrackId] = completedLapTime;
                    lapMessage = "%ryNEW LAP RECORD!";
                } else if (slot->lap != 0) {
                    lapMessage = "%ryBEST LAP!";
                }
            }
        }
        if (vehicle != 0) {
            osSyncPrintf("veh->lapTimeFinal[%d]=%f\n", slot->lap, completedLapTime);
            vehicle->lapTimes[slot->lap] = completedLapTime;
        }
        slot->lapTime -= completedLapTime;
        slot->lap = slot->distanceLaps;
        if (vehicle != 0) {
            osSyncPrintf("******* LAP #%d ********\n", slot->lap);
            sprintf(vehicle->messageText, "%%ryLAP %d", slot->lap + 1);
            vehicle->message = vehicle->messageText;
            vehicle->messageTime = 1.0f;
            if (lapMessage != 0) {
                vehicle->subMessage = lapMessage;
                vehicle->subMessageTime = 1.0f;
            }
        }
    }
    osSyncPrintf("granting technically-earned lap %d/%d to %d\n",
                 slot->distanceLaps,
                 slot->lap,
                 slot->index);
    if (g_dwGameMode == 3 && slot->lap == 1) {
        osSyncPrintf("lapping %d back to 0 (b)\n", slot->index);
        slot->lap--;
        slot->distanceLaps--;
        slot->maxGate -= g_LoadedTrack.gateCount;
        slot->gate -= g_LoadedTrack.gateCount;
        if (0 != g_LoadedTrack.rootSection) {
            slot->distance -= g_LoadedTrack.rootSection->points[0].distance;
        }
    }
tail:
    if (vehicle != 0) {
        vehicle->maxGate = slot->maxGate;
        vehicle->gate = slot->gate;
        vehicle->lap = slot->lap;
        vehicle->distanceLaps = slot->distanceLaps;
        vehicle->lapTime = slot->lapTime;
        vehicle->bestLapTime = slot->bestLapTime;
        vehicle->distance = slot->distance;
    }
}
