/* Host check of racers.c and snapshot.c: the route walk, gates and laps, the sort, and the
 * snapshot round trip, on a straight route along x with gates at x = 0 and 50. */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "audiomix.h"
#include "racers.c"
#include "snapshot.c"
#include "trackmenuitem.h"

TrackHeader g_LoadedTrack;
RacerSlot g_aRacerSlots[RACER_SLOTS];
s32 g_dwNumVehicles, g_dwNumActiveVehicles;
u32 g_dwNumLocalPlayers, g_dwCurrentWeather, g_dwSelectedTrackId;
s32 g_dwGameMode;
Vehicle g_aVehicles[MAX_VEHICLES];
PlayerProfile *g_pActiveProfile;
u8 g_abPlayerColorsRgb[RACER_SLOTS][3];
MenuItem *g_apTrackMenuItems[10];
s32 gInstantReplayActive, g_dwTotalLaps, g_dwRaceFinishOrderCounter;
u32 g_dwRacePauseState;
f32 g_fFrameDeltaTime;
MixerChannel g_aSoundEffectChannels[SOUND_EFFECT_CHANNELS];
Vec3f g_vTrackRoutePoint;
TrackSection *g_pTrackRouteSection;
s32 g_dwTrackRoutePointIndex, g_dwTrackRouteGates, g_dwTrackRouteLaps;
const char *g_apszRaceProgressStrings[RACER_SLOTS] = {"1st", "2nd"};
SaveField g_aRaceSnapshotFields[3], g_aPausedClockFields[1];
u8 g_abPausedClockState[PAUSED_CLOCK_STATE_SIZE];
LastLapBuffer g_aLastLapBuffers[4];

void qsort(void *base, unsigned long count, unsigned long size, CompareFunc compare);
void QuickSort(void *base, u32 count, u32 size, CompareFunc compare) {
    qsort(base, count, size, compare);
}
s32 FloatToInt(float value) {
    return (s32)value;
}
void FormatLapTime(char *out, f32 seconds) {
    sprintf(out, "%d", (int)seconds);
}
void PlayBeepSound(void) {
}
void osSyncPrintf(const char *format, ...) {
}
void InitVehiclePhysics(Vehicle *vehicle) {
}
void ResetVehicleEffects(Vehicle *vehicle) {
}
void SetVehiclePosition(Vehicle *vehicle, f32 x, f32 y, f32 z) {
}
void SetVehicleFrameMatrix(Vehicle *vehicle, f32 (*m)[4]) {
}
void SetVehicleVelocity(Vehicle *vehicle, f32 x, f32 y, f32 z) {
}
void SetMaterialColor(VehicleModel *model, s32 red, s32 green, s32 blue) {
}
void UpdateRaceTimers(Vehicle *vehicle) {
}
void UpdateHudMessageTimers(Vehicle *vehicle) {
}
void CheckWrongWayWarning(Vehicle *vehicle) {
}
void UpdateVehicleGridPosition(Vehicle *vehicle) {
}
void DriveAiVehicle(Vehicle *vehicle) {
}
void DeformVehicleMeshByImpact(Vehicle *vehicle, Vec3f *direction, s16 amount) {
}

#define POINTS 40
#define NEAR(a, b) (fabsf((a) - (b)) < 0.01f)

static void Gate(s32 index, f32 x) {
    g_LoadedTrack.gates[index].ends[0].x = x;
    g_LoadedTrack.gates[index].ends[0].y = 10.0f;
    g_LoadedTrack.gates[index].ends[1].x = x;
    g_LoadedTrack.gates[index].ends[1].y = -10.0f;
}

static void Move(RacerSlot *slot, f32 from, f32 to) {
    slot->previousPosition.x = from;
    slot->position.x = to;
    UpdateRaceProgress(slot);
}

int main(void) {
    static u8 sectionBuffer[sizeof(TrackSection) + (POINTS + 1) * sizeof(TrackPoint)];
    TrackSection *section = (TrackSection *)sectionBuffer;
    RacerSlot *slot = &g_aRacerSlots[5];
    s32 values[3] = {1, 2, 3};
    s32 i;

    section->pointCount = POINTS;
    for (i = 0; i <= POINTS; i++) {
        section->points[i].position.x = i * 5.0f;
        section->points[i].distance = 200.0f - i * 5.0f;
    }
    g_LoadedTrack.rootSection = section;
    Gate(0, 0.0f);
    Gate(1, 50.0f);
    g_LoadedTrack.gateCount = 2;

    /* 62 along the route is in the segment from 60 to 65, past the gate at 50. */
    FindTrackRoutePoint(section, 62.0f);
    assert(NEAR(g_vTrackRoutePoint.x, 62.0f) && g_dwTrackRoutePointIndex == 12);
    assert(g_dwTrackRouteGates == 1 && g_dwTrackRouteLaps == 0);
    AdvanceAlongTrackRoute(section, 12, 0.6f, 4.0f);
    assert(NEAR(g_vTrackRoutePoint.x, 66.0f) && g_dwTrackRoutePointIndex == 13);

    /* A racer crosses gate 1, then gate 0, finishing its first lap; then it backs over gate 0. */
    g_dwTotalLaps = 3;
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    slot->index = 5;
    slot->lapTime = 12.345f;
    Move(slot, 49.0f, 51.0f);
    assert(slot->gate == 1 && slot->maxGate == 1 && slot->lap == 0);
    Move(slot, 1.0f, -1.0f);
    assert(slot->gate == 2 && slot->lap == 1 && slot->distanceLaps == 1);
    assert(NEAR(slot->bestLapTime, 12.34f) && NEAR(slot->lapTime, 0.005f));
    Move(slot, -1.0f, 1.0f);
    assert(slot->gate == 1 && slot->distanceLaps == 0 && slot->maxGate == 2);
    /* Crossing it again forward counts no new lap. */
    Move(slot, 1.0f, -1.0f);
    assert(slot->gate == 2 && slot->lap == 1 && slot->distanceLaps == 1);

    /* The furthest racer that has not finished takes the leading place. */
    g_dwNumVehicles = 3;
    g_aRacerSlots[0].distance = 10.0f;
    g_aRacerSlots[1].distance = 30.0f;
    g_aRacerSlots[2].distance = 20.0f;
    SortRacersByPosition();
    assert(g_aRacerSlots[1].racePosition == 0 && g_aRacerSlots[2].racePosition == 1 &&
           g_aRacerSlots[0].racePosition == 2);

    /* A snapshot gives back the variables it saved. */
    g_aRaceSnapshotFields[0].data = values;
    g_aRaceSnapshotFields[0].size = sizeof(values);
    SaveRaceSnapshot(&g_aLastLapBuffers[3]);
    values[0] = values[1] = values[2] = 0;
    RestoreRaceSnapshot(&g_aLastLapBuffers[3]);
    assert(values[0] == 1 && values[2] == 3);

    puts("racers ok");
    return 0;
}
