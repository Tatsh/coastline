#include "fade.h"
#include "game.h"
#include "race.h"
#include "racerender.h"
#include "render.h"
#include "shadowbuffers.h"
#include "sky.h"
#include "snapshot.h"
#include "system.h"
#include "timing.h"

namespace {
/** @ghidraAddress BRD3D: 0x100a6cf8 */
int g_dwHudClearFramesRemaining = 2;
/** @ghidraAddress BRD3D: 0x100a6cfc */
int g_dwHudLastViewportCount = -1;
} // namespace

void FrameSnapshot::RenderRacingScreen() {
    LoadParticleListHeads(&particles);
    UpdateFadeTransitions();
    StartLowResFrame(&viewport);
    NullStub();
    ResetShadowBuffers();
    NullStub();
    if (!g_dwHudClearFramesRemaining && !g_dwSkyboxDisabled) {
        if (viewport.width < 304) {
            DrawFilledRect(viewport.x + viewport.width, 8, 304 - viewport.width, 224, 0, 0, 0);
        }
    } else {
        ClearScreen(0, 0, 0);
        if (g_dwHudClearFramesRemaining) {
            g_dwHudClearFramesRemaining--;
        }
    }
    if (g_dwNumViewports != g_dwHudLastViewportCount) {
        g_dwHudClearFramesRemaining = 2;
        g_dwHudLastViewportCount = g_dwNumViewports;
    }
    for (int index = 0; index < g_dwNumViewports; index++) {
        RenderRaceView(&viewport, racers, vehicles, index);
    }
    ClearDepthBuffer();
    SetScissorRect(0, 0, g_dwScreenWidth, g_dwScreenHeight);
    if (g_dwRacePauseState) {
        DrawPauseMenu(&viewport, racers, vehicles);
    }
    if (g_dwGameMode == GAME_MODE_DEMO && g_dwRaceSessionType == 2) {
        DrawScrollingText(&viewport);
    } else if (g_dwGameMode == GAME_MODE_DEMO && g_dwRaceSessionType == 1 &&
               g_aCreditCaptions[g_dwRaceCaptionIndex].title != nullptr) {
        DrawCreditCaption();
    }
    ApplyScreenFade();
    if (g_dwHudDebugLevel > 1) {
        SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, TRUE);
        NullStub();
    }
    NullStub();
    StreamAnimatedTrackTextures();
    NullStub();
    FlushDisplayList();
}

void ProcessRacingScreen(int snapshotSlot) {
    FrameSnapshot &snapshot = snapshotSlot == FRAME_SNAPSHOT_COUNT ?
                                  g_InterpolatedFrameSnapshot :
                                  g_aFrameSnapshots[snapshotSlot];
    snapshot.RenderRacingScreen();
}
