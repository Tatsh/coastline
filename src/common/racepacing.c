#include "common/racepacing.h"

unsigned g_dwLastFrameTimestamp;
volatile unsigned g_dwRaceAccumulatedTime;
int g_nRacePacingInterval;
unsigned g_dwNextRaceFrameTime;
int g_nRaceFramesWithoutRender;
unsigned g_dwLastRaceRenderTime;
const unsigned g_adwRaceFrameIntervals[3] = {33, 33, 34};
