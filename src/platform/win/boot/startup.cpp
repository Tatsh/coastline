#include <stdlib.h>
#include <string.h>

#include <d3dtypes.h>
#include <windows.h>

#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/fileio.h"
#include "platform/win/input.h"
#include "platform/win/settings.h"

/* The size of the buffer of a line of BossRally.ini. */
#define INI_LINE_SIZE 256

DWORD g_dwTotalPhysicalMemory;

BOOL FocusExistingBossRallyInstance(void) {
    HWND hWindow;
    HWND hForeground;
    DWORD dwForegroundThread;

    hWindow = FindWindowA("BossRally", "Boss Rally");
    if (hWindow == NULL) {
        return TRUE;
    }
    hForeground = GetForegroundWindow();
    dwForegroundThread = GetWindowThreadProcessId(hForeground, NULL);
    /* The other instance is left as it is when it already has the focus. */
    if (GetWindowThreadProcessId(hWindow, NULL) != dwForegroundThread || IsIconic(hWindow)) {
        hWindow = GetLastActivePopup(hWindow);
        if (IsIconic(hWindow)) {
            ShowWindow(hWindow, SW_RESTORE);
        }
        BringWindowToTop(hWindow);
        SetForegroundWindow(hWindow);
    }
    OutputDebugStringA("CheckPrevious App - Another instance of this app is already running.\n");
    return FALSE;
}

void RefreshTotalPhysicalMemory(void) {
    MEMORYSTATUS status;

    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    g_dwTotalPhysicalMemory = status.dwTotalPhys;
}

/* The Glide library reads a different set of keys. */
#if TGR_RENDERER == TGR_RENDERER_D3D
/* The value of a line of BossRally.ini that sets a key, the key given with its equals sign. */
static inline const char *GetIniValue(const char *pszLine, const char *pszKey) {
    size_t nLength = strlen(pszKey);

    return strncmp(pszLine, pszKey, nLength) == 0 ? pszLine + nLength : NULL;
}

/* The value that follows a key anywhere on the command line. */
static inline const char *GetCommandLineValue(const char *pszCmdLine, const char *pszKey) {
    const char *pszFound = strstr(pszCmdLine, pszKey);

    return pszFound != NULL ? pszFound + strlen(pszKey) : NULL;
}

/* Copy a directory of BossRally.ini, without the line feed that ends the line. */
static inline void CopyIniDirectory(char *pszDirectory, const char *pszValue) {
    strcpy(pszDirectory, pszValue);
    pszDirectory[strlen(pszDirectory) - 1] = '\0';
}

static inline void SelectInputBindings(void) {
    switch (g_dwInputDeviceType) {
    case 1:
        g_pActiveInputBindings = g_aInputBindingsDevice1;
        break;
    case 2:
        g_pActiveInputBindings = g_aInputBindingsDevice2;
        break;
    case 3:
        g_pActiveInputBindings = g_aInputBindingsDevice3;
        break;
    default:
        g_pActiveInputBindings = g_aInputBindingsKeyboard;
        break;
    }
}

static inline void ApplyIniLine(const char *pszLine) {
    const char *pszValue;

    if ((pszValue = GetIniValue(pszLine, "NetworkPlay=")) != NULL) {
        g_dwNetworkPlayMode = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "chosenTrack=")) != NULL) {
        g_dwSelectedTrackId = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "chosenCar=")) != NULL) {
        g_dwChosenCar = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "chosenWeather=")) != NULL) {
        g_dwChosenWeather = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "gameMode=")) != NULL) {
        g_dwGameMode = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "ReadJoystick=")) != NULL) {
        g_dwInputDeviceType = atoi(pszValue);
        SelectInputBindings();
    } else if ((pszValue = GetIniValue(pszLine, "HandlingType=")) != NULL) {
        g_dwHandlingType = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "SuspensionType=")) != NULL) {
        g_dwSuspensionType = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "TireType=")) != NULL) {
        g_dwTireType = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "TransmissionType=")) != NULL) {
        g_dwTransmissionType = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "TrackDir=")) != NULL) {
        CopyIniDirectory(g_szTracks, pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "CarDir=")) != NULL) {
        CopyIniDirectory(g_szCarsDirPath, pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "SFXDir=")) != NULL) {
        CopyIniDirectory(g_szSfxDirPath, pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "Interpolate=")) != NULL) {
        g_bInterpolateFrames = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "SpeedSensitive=")) != NULL) {
        g_bAutomaticGearboxIni = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "D3DDrawCarShadow=")) != NULL) {
        g_bDisableCarShadow = atoi(pszValue) == 0;
    } else if ((pszValue = GetIniValue(pszLine, "RunBenchmark=")) != NULL) {
        g_dwRunBenchmark = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "PlayMusic=")) != NULL) {
        g_dwPlayMusicMode = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "PlaySFX=")) != NULL) {
        g_bSoundEnabledIni = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "D3DAlphaCompare=")) != NULL) {
        g_bD3DAlphaCompare = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "D3DAlwaysSquareTextures=")) != NULL) {
        g_dwD3DAlwaysSquareTextures = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "D3DClearZBuffer=")) != NULL) {
        g_dwD3DClearZBuffer = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "D3DWaitCanFlip=")) != NULL) {
        g_bD3DWaitCanFlip = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "D3DWaitFlipDone=")) != NULL) {
        g_bD3DWaitFlipDone = atoi(pszValue);
    } else if ((pszValue = GetIniValue(pszLine, "D3DInvSrcAlpha=")) != NULL) {
        g_dwD3DDestBlendMode = atoi(pszValue) != 0 ? D3DBLEND_INVSRCALPHA : D3DBLEND_ONE;
    }
}

static inline void ApplyCommandLine(const char *pszCmdLine) {
    const char *pszValue;
    char *pszEnd;

    if ((pszValue = GetCommandLineValue(pszCmdLine, "NetworkPlay=")) != NULL) {
        g_dwNetworkPlayMode = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "szPlayerName=")) != NULL) {
        /* The name ends at the first space or line feed. */
        strcpy(g_szPlayerName, pszValue);
        if ((pszEnd = strchr(g_szPlayerName, ' ')) != NULL) {
            *pszEnd = '\0';
        }
        if ((pszEnd = strchr(g_szPlayerName, '\n')) != NULL) {
            *pszEnd = '\0';
        }
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "chosenTrack=")) != NULL) {
        g_dwSelectedTrackId = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "chosenCar=")) != NULL) {
        g_dwChosenCar = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "chosenWeather=")) != NULL) {
        g_dwChosenWeather = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "gameMode=")) != NULL) {
        g_dwGameMode = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "ReadJoystick=")) != NULL) {
        g_dwInputDeviceType = atoi(pszValue);
        SelectInputBindings();
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "HandlingType=")) != NULL) {
        g_dwHandlingType = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "SuspensionType=")) != NULL) {
        g_dwSuspensionType = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "TireType=")) != NULL) {
        g_dwTireType = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "TransmissionType=")) != NULL) {
        g_dwTransmissionType = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "cPlayers=")) != NULL) {
        g_bNetMaxPlayers = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "bcar=")) != NULL) {
        g_wNetAllowedCarsMask = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "btire=")) != NULL) {
        g_bNetTireSetting = atoi(pszValue);
    }
    if ((pszValue = GetCommandLineValue(pszCmdLine, "bsuspension=")) != NULL) {
        g_bNetSuspensionSetting = atoi(pszValue);
    }
}

void LoadBossRallyIniSettings(LPSTR pszCmdLine) {
    char szLine[INI_LINE_SIZE];
    GameFile *pFile;

    strcpy(g_szBossRallyIniPath, g_szInstallDirectory);
    strcat(g_szBossRallyIniPath, "BossRally.ini");
    if (FileExists(g_szBossRallyIniPath)) {
        pFile = FileOpenForReading(g_szBossRallyIniPath);
        while (FileReadLine(szLine, sizeof(szLine), pFile) != NULL) {
            ApplyIniLine(szLine);
        }
        FileClose(pFile);
    }
    if (pszCmdLine != NULL && pszCmdLine[0] != '\0') {
        ApplyCommandLine(pszCmdLine);
    }
}
#endif
