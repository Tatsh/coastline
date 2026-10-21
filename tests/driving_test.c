/* Host check of driving.c: jolts, the per-frame clear, the track projection and the search for a
 * straight line, on a straight track along x. */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "driving.c"

TrackHeader g_LoadedTrack;
ViewportRect g_aViewportRects[2];
s32 g_dwNumViewports, g_dwTrackMirrored, gInstantReplayActive;
s32 g_dwReplayCameraSwitchTimer, g_dwReplayCameraHoldTimer;
u32 g_dwGameMode;
RacerSlot g_aRacerSlots[4];
s32 g_dwNumVehicles;
PlayerProfile *g_pActiveProfile;
u32 g_dwCurrentWeather;
f32 g_fFrameDeltaTime;
PerFrameVariable g_aPerFrameVariables[2];
Vec3f g_vUpAxis = {0, 0, 1};
f32 g_aafAiVelocityRetention[SEASONS][SEASON_RACES][2];
s32 g_bReplayCockpitNext;
f32 g_afVehicleJolts[4];
s32 g_dwBestTraceDepth, g_dwBestTracePoint, g_dwBestTraceLength, g_dwTraceUnusedA, g_dwTraceUnusedB;
s32 g_bTraceBlocked, g_bAiNudgeLeft, g_bAiNudgeRight;
TrackSection *g_pBestTraceSection;
Vec3f g_avBestLeftLane[TRACE_DEPTH], g_avBestCenterLine[TRACE_DEPTH],
    g_avBestRightLane[TRACE_DEPTH];
Vec3f g_avTraceLeftLane[TRACE_DEPTH], g_avTraceLeftHalf[TRACE_DEPTH],
    g_avTraceCenterLine[TRACE_DEPTH];
Vec3f g_avTraceRightHalf[TRACE_DEPTH], g_avTraceRightLane[TRACE_DEPTH];
Vec3f g_vTraceFarPoint, g_vTraceHalfwayPoint;

void TransferFlags(PlayerInputState *state, u32 mask) {
    state->buttons &= ~mask;
}
void PlaceTrackCamera(Vehicle *vehicle) {
}
void SetVehicleVelocity(Vehicle *vehicle, f32 x, f32 y, f32 z) {
}
void UpdateVehiclePhysicsFrame(Vehicle *vehicle) {
}
void RespawnVehicle(Vehicle *vehicle) {
}
void AddTimingMarker(s32 bar, s32 red, s32 green, s32 blue, s32 alpha) {
}

#define POINTS 20
#define NEAR(a, b) (fabsf((a) - (b)) < 0.01f)

int main(void) {
    static u8 block[16];
    static u8 sectionBuffer[sizeof(TrackSection) + (POINTS + 1) * sizeof(TrackPoint)];
    static Vehicle vehicleBuffer;
    TrackSection *section = (TrackSection *)sectionBuffer;
    Vehicle *vehicle = &vehicleBuffer;
    s32 i;

    AddVehicleJolt(1, 3.0f);
    assert(g_afVehicleJolts[1] == 2.5f);
    AddVehicleJolt(1, 3.0f);
    assert(g_afVehicleJolts[1] == 5.0f);

    for (i = 0; i < (s32)sizeof(block); i++) {
        block[i] = 0xAA;
    }
    g_aPerFrameVariables[0].start = block;
    g_aPerFrameVariables[0].size = 12;
    ClearPerFrameVariables();
    assert(block[0] == 0 && block[11] == 0 && block[12] == 0xAA);

    /* A straight route along x, 10 wide, its distances falling along it. */
    section->pointCount = POINTS;
    for (i = 0; i <= POINTS; i++) {
        TrackPoint *p = &section->points[i];
        p->position.x = p->left.x = p->right.x = i * 5.0f;
        p->left.y = 5.0f;
        p->right.y = -5.0f;
        p->distance = 100.0f - i * 5.0f;
    }
    g_LoadedTrack.rootSection = section;
    vehicle->section = section;
    vehicle->pointIndex = 2;
    vehicle->frame.position.x = 12.0f;
    vehicle->frame.position.y = 1.0f;
    vehicle->trackUp.z = 1.0f;

    ComputeTrackPosition(vehicle);
    assert(NEAR(vehicle->trackPoint.y, 0.0f) && NEAR(vehicle->trackPoint.z, 0.0f));
    assert(NEAR(vehicle->trackForward.x, 1.0f) && NEAR(vehicle->trackSide.y, 1.0f));
    assert(NEAR(vehicle->trackSideOffset, 1.0f) && NEAR(vehicle->trackSideDistance, 1.0f));

    assert(TraceTrackSplineRecursive(vehicle, 0, 2, section));
    assert(g_dwBestTraceDepth == TRACE_DEPTH && g_dwBestTraceLength == TRACE_DEPTH);
    assert(!g_bTraceBlocked);
    assert(NEAR(g_vTraceFarPoint.x, 50.0f) && NEAR(g_avBestRightLane[0].y, -3.0f));
    assert(NEAR(g_avBestLeftLane[0].y, 3.0f) && NEAR(g_vTraceHalfwayPoint.x, 35.0f));

    /* A vehicle off to the side of a bend cannot see through it: a wall of points crossing the
     * line turns the search back. */
    vehicle->frame.position.y = 40.0f;
    TraceTrackSplineRecursive(vehicle, 0, 2, section);
    assert(g_bTraceBlocked);

    puts("driving ok");
    return 0;
}
