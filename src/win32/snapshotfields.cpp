#include <cstddef>

#include "frametime.h"
#include "physics.h"
#include "playerinput.h"
#include "race.h"
#include "racerslot.h"
#include "replay.h"
#include "soundeffect.h"
#include "soundsource.h"
#include "system.h"
#include "vecmath.h"
#include "vehicle.h"
#include "win32/airplanestate.h"
#include "win32/sound.h"

namespace {
/* Native code accesses these bytes only through the snapshot field list. */
// @ghidraAddress BRD3D: 0x118ac8a0
u8 g_abUnusedSnapshotState[0xb4];
} // namespace

SaveField g_aRaceSnapshotFields[] = {
    {g_aVehicles, sizeof(g_aVehicles[0]) * 3},
    {g_aRacerSlots, sizeof(g_aRacerSlots)},
    {&g_dwRaceFinishOrderCounter, sizeof(g_dwRaceFinishOrderCounter)},
    {&g_dwRandomSeed, sizeof(g_dwRandomSeed)},
    {&g_aPlayerInputState[0].heldButtons, sizeof(g_aPlayerInputState[0].heldButtons)},
    {&g_aPlayerInputState[1].heldButtons, sizeof(g_aPlayerInputState[1].heldButtons)},
    {&g_aPlayerInputState[2].heldButtons, sizeof(g_aPlayerInputState[2].heldButtons)},
    {&g_aPlayerInputState[0].controllerLayout, sizeof(g_aPlayerInputState[0].controllerLayout)},
    {&g_aPlayerInputState[1].controllerLayout, sizeof(g_aPlayerInputState[1].controllerLayout)},
    {&g_aPlayerInputState[2].controllerLayout, sizeof(g_aPlayerInputState[2].controllerLayout)},
    {&g_qwTimestampBase, sizeof(g_qwTimestampBase)},
    {&g_dwFrameTimestamp, sizeof(g_dwFrameTimestamp)},
    {&g_dwPrevElapsedTimeMs, sizeof(g_dwPrevElapsedTimeMs)},
    {&g_dwElapsedTimeMs, sizeof(g_dwElapsedTimeMs)},
    {&g_fFrameDeltaTime, sizeof(g_fFrameDeltaTime)},
    {&g_dwFrameDeltaMs, sizeof(g_dwFrameDeltaMs)},
    {g_aSoundChannelRequested, sizeof(g_aSoundChannelRequested)},
    {g_abUnusedSnapshotState, sizeof(g_abUnusedSnapshotState)},
    {&g_AirplaneState, sizeof(g_AirplaneState)},
    {&g_dwChannel3PlayingSfxId, sizeof(g_dwChannel3PlayingSfxId)},
    {&g_ClosestSoundSource, sizeof(g_ClosestSoundSource)},
    {&g_bTrackCollisionFrameToggle, sizeof(g_bTrackCollisionFrameToggle)},
    {nullptr, 0},
};
