#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <process.h>
#include <windows.h>

#include "ini.h"

typedef int(__cdecl *RallyMainProc)(HINSTANCE, HINSTANCE, LPSTR, int);

// @ghidraAddress BRally: 0x004032a8
static char g_szInstallDir[260];
// @ghidraAddress BRally: 0x004033b0
static char g_szIniFilePath[1024];
// @ghidraAddress BRally: 0x00403bb0
static char g_szSpawnArgBuffer[32768];
// @ghidraAddress BRally: 0x0040bbb0
static const char *g_apszSpawnArgv[128];
// @ghidraAddress BRally: 0x0040bdb0
static RallyMainProc g_pfnRallyMain;
// @ghidraAddress BRally: 0x0040bdb4
static unsigned g_dwSpawnArgCount;
// @ghidraAddress BRally: 0x0040bdb8
static unsigned g_dwSpawnArgBufferUsed;
// @ghidraAddress BRally: 0x0040bdbc
static HMODULE g_hRendererDll;

// @ghidraAddress BRally: 0x004016d0
static void ReadInstallDirectoryFromRegistry(void) {
    HKEY key;
    if (RegOpenKeyExA(
            HKEY_LOCAL_MACHINE, "SOFTWARE\\SouthPeak Interactive\\Boss Rally", 0, KEY_READ, &key) ==
        ERROR_SUCCESS) {
        DWORD bytes = 260;
        LSTATUS result =
            RegQueryValueExA(key, "Directory", nullptr, nullptr, (BYTE *)g_szInstallDir, &bytes);
        RegCloseKey(key);
        if (result == ERROR_SUCCESS) {
            if (g_szInstallDir[strlen(g_szInstallDir) - 1] != '\\') {
                strcat(g_szInstallDir, "\\");
            }
            return;
        }
    }
    strcpy(g_szInstallDir, "c:\\");
}

// @ghidraAddress BRally: 0x004017b0
static int LoadRendererDll(const char *path) {
    g_hRendererDll = LoadLibraryA(path);
    if (!g_hRendererDll) {
        return 0;
    }
    g_pfnRallyMain = (RallyMainProc)GetProcAddress(g_hRendererDll, "RallyMain");
    return g_pfnRallyMain != nullptr;
}

// @ghidraAddress BRally: 0x004017e0
static BOOL UnloadRendererDll(void) {
    if (g_hRendererDll) {
        BOOL result = FreeLibrary(g_hRendererDll);
        g_hRendererDll = nullptr;
        return result;
    }
    return TRUE;
}

// @ghidraAddress BRally: 0x00401a70
static void AppendSpawnArgument(const char *argument) {
    char *destination = g_szSpawnArgBuffer + g_dwSpawnArgBufferUsed;
    strcpy(destination, argument);
    g_apszSpawnArgv[g_dwSpawnArgCount++] = destination;
    g_apszSpawnArgv[g_dwSpawnArgCount] = nullptr;
    g_dwSpawnArgBufferUsed += strlen(argument) + 1;
}

/** @ghidraAddress BRally: 0x00401810 */
int WINAPI WinMain(HINSTANCE instance,
                   HINSTANCE previousInstance,
                   LPSTR commandLine,
                   int showCommand) {
    HWND desktop = GetDesktopWindow();
    ReadInstallDirectoryFromRegistry();
    strcpy(g_szIniFilePath, g_szInstallDir);
    strcat(g_szIniFilePath, "BossRally.ini");
    if (!FileExists(g_szIniFilePath)) {
        AppendSpawnArgument("SetVideo.exe");
        if (_spawnv(_P_WAIT, "SetVideo.exe", g_apszSpawnArgv)) {
            MessageBoxA(desktop, "User canceled SetVideo.exe", "Boss Rally", MB_OK);
            return 0;
        }
    }
    char rendererPath[1024];
    rendererPath[0] = '\0';
    LauncherIniFile *file = ReadIniFile(g_szIniFilePath);
    LauncherIniSectionIterator *section = FindIniSection(file, "[Video]");
    char *driver = GetIniSectionValue(section, "Driver");
    if (driver) {
        if (!strcmp(driver, "D3D")) {
            driver = "BRD3D.dll";
        } else if (!strcmp(driver, "Glide")) {
            driver = "BRGlide.dll";
        }
        strcpy(rendererPath, driver);
    }
    FreeIniSectionIterator(section);
    FreeIniFile(file);
    if (!rendererPath[0]) {
        return 0;
    }
    if (!LoadRendererDll(rendererPath)) {
        char message[1024];
        sprintf(message, "Error: failed to load %s.", rendererPath);
        MessageBoxA(desktop, message, "Boss Rally", MB_ICONHAND);
        exit(1);
    }
    int result = g_pfnRallyMain(instance, previousInstance, commandLine, showCommand);
    UnloadRendererDll();
    return result;
}
