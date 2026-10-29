#include "win32/app.h"
#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "sdl/renderhost.h"
#else
#include "win32/d3d/renderer.h"
#endif
#include "win32/displaylist.h"
#include "win32/netstream.h"
#include "win32/render.h"

int g_dwMaxTexHeightHighQuality = 64;

int g_dwMaxTexHeightLowQuality = 32;

int g_dwMaxTexWidthHighQuality = 64;

int g_dwMaxTexWidthLowQuality = 32;

int g_dwNullStubEnable = 1;

int g_dwScreenBitDepth = 16;

int g_dwVehicleStateSendCounter = -1;

float g_fDepthOffset = 0.46f;

float g_fDepthScale = 0.46f;
