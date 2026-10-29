#include "common/racepacing.h"

DWORD g_dwLastFrameTimestamp;
volatile DWORD g_dwRaceAccumulatedTime;
int g_nRacePacingInterval;
DWORD g_dwNextRaceFrameTime;
int g_nRaceFramesWithoutRender;
DWORD g_dwLastRaceRenderTime;
const DWORD g_adwRaceFrameIntervals[3] = {33, 33, 34};
