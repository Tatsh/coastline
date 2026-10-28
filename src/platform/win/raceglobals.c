#include "platform/win/settings.h"
#include "race.h"
#include "vehicle.h"

int g_dwChosenWeather;
int g_bNetMaxPlayers;
int g_wNetAllowedCarsMask;
int g_bNetTireSetting;
int g_bNetSuspensionSetting;

ViewportRect g_aViewportRects[4];

u8 g_abPlayerColorsRgb[20][3] = {
    {0, 127, 0},     {255, 175, 0},   {0, 0, 175},     {200, 0, 0},     {160, 0, 140},
    {255, 255, 255}, {0, 48, 0},      {0, 0, 0},       {240, 120, 0},   {96, 48, 16},
    {128, 255, 140}, {128, 128, 128}, {128, 184, 255}, {88, 0, 72},     {208, 200, 128},
    {128, 128, 128}, {200, 0, 100},   {0, 190, 160},   {255, 148, 160}, {0, 0, 64},
};

u8 g_abRacePositionPoints[20] = {9, 6, 5, 3, 2, 1};
