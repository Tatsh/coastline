#include "platform/win/ghost.h"

#include <cstring>

#include "game.h"

unsigned g_adwGhostFrameCount[GHOST_RECORDING_COUNT];
int g_bGhostRecordingActive;

void FillMemoryPatternBytes(void *pDest, size_t nCount, int nByte) {
    memset(pDest, nByte, nCount);
}

void ResetGhostRecording(void) {
    int nRecordings = GHOST_RECORDING_COUNT;
    if (g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_DEMO) {
        nRecordings = 1;
    }
    memset(g_adwGhostFrameCount, 0, nRecordings * sizeof(g_adwGhostFrameCount[0]));
    g_bGhostRecordingActive = false;
}

void SetGhostFrameCountFromSize(unsigned dwSize) {
    g_adwGhostFrameCount[1] = dwSize / GHOST_FRAME_SIZE;
}

void MatrixToPositionQuaternion(QuaternionPose *pPose, f32 m[4][4]) {
    MatrixToQuaternion(m, &pPose->orientation);
    pPose->position.x = m[3][0];
    pPose->position.y = m[3][1];
    pPose->position.z = m[3][2];
}
