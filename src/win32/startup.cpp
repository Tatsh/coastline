#include <cstdlib>
#include <cstring>

#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "sdl/d3dcompat.h"
#include "sdl/win32api.h"
#else
#include <d3dtypes.h>
#include <windows.h>
#endif

#include "common/fileio.h"
#include "common/settings.h"
#include "win32/app.h"
#include "win32/input.h"
#include "win32/paintshop.h"
#include "win32/podarchive.h"
#include "win32/rallyconfig.h"
#include "win32/render.h"
#include "win32/sound.h"

// The size of the buffer of a line of BossRally.ini.
#define INI_LINE_SIZE 256

size_t g_dwTotalPhysicalMemory;

#if TGR_PLATFORM == TGR_PLATFORM_SDL
bool FocusExistingBossRallyInstance(void) {
    // Each SDL game has its own window, and instances do not look for each other.
    return true;
}

void RefreshTotalPhysicalMemory(void) {
    g_dwTotalPhysicalMemory = static_cast<size_t>(SDL_GetSystemRAM()) * 1024 * 1024;
}
#else
bool FocusExistingBossRallyInstance(void) {
    HWND hWindow;
    HWND hForeground;
    DWORD dwForegroundThread;

    hWindow = FindWindowA("BossRally", "Boss Rally");
    if (hWindow == nullptr) {
        return true;
    }
    hForeground = GetForegroundWindow();
    dwForegroundThread = GetWindowThreadProcessId(hForeground, nullptr);
    // The other instance is left as it is when it already has the focus.
    if (GetWindowThreadProcessId(hWindow, nullptr) != dwForegroundThread || IsIconic(hWindow)) {
        hWindow = GetLastActivePopup(hWindow);
        if (IsIconic(hWindow)) {
            ShowWindow(hWindow, SW_RESTORE);
        }
        BringWindowToTop(hWindow);
        SetForegroundWindow(hWindow);
    }
    OutputDebugStringA("CheckPrevious App - Another instance of this app is already running.\n");
    return false;
}

void RefreshTotalPhysicalMemory(void) {
    MEMORYSTATUS status;

    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    g_dwTotalPhysicalMemory = status.dwTotalPhys;
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */

// The value of a line of BossRally.ini that sets a key, the key given with its equals sign.
static inline const char *GetIniValue(const char *pszLine, const char *pszKey) {
    size_t nLength = strlen(pszKey);

    return strncmp(pszLine, pszKey, nLength) == 0 ? pszLine + nLength : nullptr;
}

// The value that follows a key anywhere on the command line.
static inline const char *GetCommandLineValue(const char *pszCmdLine, const char *pszKey) {
    const char *pszFound = strstr(pszCmdLine, pszKey);

    return pszFound != nullptr ? pszFound + strlen(pszKey) : nullptr;
}

// Copy a directory of BossRally.ini, without the line feed that ends the line.
static inline void CopyIniDirectory(char *pszDirectory, const char *pszValue) {
    strcpy(pszDirectory, pszValue);
    pszDirectory[strlen(pszDirectory) - 1] = '\0';
}

static inline void ApplyIniLine(const char *pszLine) {
    const char *pszValue;

    if ((pszValue = GetIniValue(pszLine, "NetworkPlay=")) != nullptr) {
        g_dwNetworkPlayMode = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "chosenTrack=")) != nullptr) {
        g_dwSelectedTrackId = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "chosenCar=")) != nullptr) {
        g_dwChosenCar = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "chosenWeather=")) != nullptr) {
        g_dwChosenWeather = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "gameMode=")) != nullptr) {
        g_dwGameMode = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "ReadJoystick=")) != nullptr) {
        g_GlobalRallyConfig.dwInputDeviceType = atoi(pszValue);
        g_GlobalRallyConfig.SelectActiveInputBindings();
    } else if ((pszValue = GetIniValue(pszLine, "HandlingType=")) != nullptr) {
        g_dwHandlingType = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "SuspensionType=")) != nullptr) {
        g_dwSuspensionType = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "TireType=")) != nullptr) {
        g_dwTireType = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "TransmissionType=")) != nullptr) {
        g_dwTransmissionType = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "TrackDir=")) != nullptr) {
        CopyIniDirectory(g_szTracks, pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "CarDir=")) != nullptr) {
        CopyIniDirectory(g_szCarsDirPath, pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "SFXDir=")) != nullptr) {
        CopyIniDirectory(g_szSfxDirPath, pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "Interpolate=")) != nullptr) {
        g_bInterpolateFrames = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "SpeedSensitive=")) != nullptr) {
        g_bSpeedSensitiveSteering = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "D3DDrawCarShadow=")) != nullptr) {
        g_bDisableCarShadow = atoi(pszValue) == 0;
    } else if ((pszValue = GetIniValue(pszLine, "RunBenchmark=")) != nullptr) {
        g_dwRunBenchmark = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "PlayMusic=")) != nullptr) {
        g_dwPlayMusicMode = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "PlaySFX=")) != nullptr) {
        g_bSoundEnabledIni = atoi(pszValue);
#if TGR_BRD3D
    } else if ((pszValue = GetIniValue(pszLine, "D3DAlphaCompare=")) != nullptr) {
        g_bD3DAlphaCompare = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "D3DAlwaysSquareTextures=")) != nullptr) {
        g_dwD3DAlwaysSquareTextures = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "D3DClearZBuffer=")) != nullptr) {
        g_dwD3DClearZBuffer = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "D3DWaitCanFlip=")) != nullptr) {
        g_bD3DWaitCanFlip = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "D3DWaitFlipDone=")) != nullptr) {
        g_bD3DWaitFlipDone = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "D3DInvSrcAlpha=")) != nullptr) {
        g_dwD3DDestBlendMode = atoi(pszValue) != 0 ? D3DBLEND_INVSRCALPHA : D3DBLEND_ONE;
#endif /* TGR_BRD3D */
    }
}

static inline void ApplyCommandLine(const char *pszCmdLine) {
    const char *pszValue;
    char *pszEnd;

    if ((pszValue = GetCommandLineValue(pszCmdLine, "NetworkPlay=")) != nullptr) {
        g_dwNetworkPlayMode = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "szPlayerName=")) != nullptr) {
        // The name ends at the first space or line feed.
        strcpy(g_GlobalRallyConfig.szPlayerName, pszValue);
        if ((pszEnd = strchr(g_GlobalRallyConfig.szPlayerName, ' ')) != nullptr) {
            *pszEnd = '\0';
        }
        if ((pszEnd = strchr(g_GlobalRallyConfig.szPlayerName, '\n')) != nullptr) {
            *pszEnd = '\0';
        }
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "chosenTrack=")) != nullptr) {
        g_dwSelectedTrackId = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "chosenCar=")) != nullptr) {
        g_dwChosenCar = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "chosenWeather=")) != nullptr) {
        g_dwChosenWeather = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "gameMode=")) != nullptr) {
        g_dwGameMode = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "ReadJoystick=")) != nullptr) {
        g_GlobalRallyConfig.dwInputDeviceType = atoi(pszValue);
        g_GlobalRallyConfig.SelectActiveInputBindings();
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "HandlingType=")) != nullptr) {
        g_dwHandlingType = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "SuspensionType=")) != nullptr) {
        g_dwSuspensionType = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "TireType=")) != nullptr) {
        g_dwTireType = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "TransmissionType=")) != nullptr) {
        g_dwTransmissionType = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "cPlayers=")) != nullptr) {
        g_bNetMaxPlayers = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "bcar=")) != nullptr) {
        g_wNetAllowedCarsMask = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "btire=")) != nullptr) {
        g_bNetTireSetting = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "bsuspension=")) != nullptr) {
        g_bNetSuspensionSetting = atoi(pszValue);
    }
}

void LoadBossRallyIniSettings(const char *pszCmdLine) {
    char szLine[INI_LINE_SIZE];
    GameFile *pFile;

    strcpy(g_szBossRallyIniPath, g_szInstallDirectory);
    strcat(g_szBossRallyIniPath, "BossRally.ini");
    if (CHK_FileExists(g_szBossRallyIniPath)) {
        pFile = CHK_FReadOpen(g_szBossRallyIniPath);
        while (FileReadLine(szLine, sizeof(szLine), pFile) != nullptr) {
            ApplyIniLine(szLine);
        }
        CHK_FClose(pFile);
    }
    if (pszCmdLine != nullptr && pszCmdLine[0] != '\0') {
        ApplyCommandLine(pszCmdLine);
    }
}

#if TGR_RENDERER == TGR_RENDERER_GLIDE
// The value of the second argument of ShowGlideSplashImage() for the splash image.
#define SPLASH_IMAGE_ARGUMENT 0x2ac7e58b
#endif
// The music track OpenBossRallyPodAndInit() selects.
#define FIRST_MUSIC_TRACK 2

bool OpenBossRallyPodAndInit(void) {
    g_BossRallyPodArchive.SetFilePath("BossRally.pod");
    g_BossRallyPodArchive.OpenPodFile();
    InitializeRenderSubsystems(RENDER_BACKEND_MODE);
#if TGR_RENDERER == TGR_RENDERER_GLIDE
    ShowGlideSplashImage("splash.img", SPLASH_IMAGE_ARGUMENT);
#endif
    LoadPaintDamageBitmaps();
    InitializeDirectInput();
    InitializeKeyboardDevice();
    if (g_dwPlayMusicMode) {
        RouteMusicInitByFlag(g_hMainWindow);
        ApplyAudioVolumeSettings();
        SetCurrentMusicTrack(FIRST_MUSIC_TRACK);
    }
    if (!g_bSoundEnabledIni) {
        return false;
    }
    return InitializeDirectSound();
}
