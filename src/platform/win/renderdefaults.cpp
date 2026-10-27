#include "platform/win/boot/app.h"
#include "platform/win/d3d/renderer.h"
#include "platform/win/displaylist.h"
#include "platform/win/netstream.h"
#include "platform/win/render.h"

int g_dwMaxTexHeightHighQuality = 64;

int g_dwMaxTexHeightLowQuality = 32;

int g_dwMaxTexWidthHighQuality = 64;

int g_dwMaxTexWidthLowQuality = 32;

int g_dwNullStubEnable = 1;

int g_dwScreenBitDepth = 16;

int g_dwVehicleStateSendCounter = -1;

float g_fDepthOffset = 0.46f;

float g_fDepthScale = 0.46f;
