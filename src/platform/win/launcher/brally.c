#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <process.h>
#include <windows.h>

#include "ini.h"
#include "platform/win/diagnostics.h"
#include "platform/win/fileio.h"
#include "registry.h"

typedef int(__cdecl *RallyMainProc)(HINSTANCE, HINSTANCE, LPSTR, int);

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

// @ghidraAddress BRally: 0x004017b0
static int LoadRendererDll(const char *path) {
    g_hRendererDll = LoadLibraryA(path);
    if (!g_hRendererDll) {
        return 0;
    }
#ifdef ENABLE_PATCHES
    // GetProcAddress() returns a generic function pointer, cast through void(void) to
    // RallyMainProc.
    g_pfnRallyMain = (RallyMainProc)(void (*)(void))GetProcAddress(g_hRendererDll, "RallyMain");
#else
    g_pfnRallyMain = (RallyMainProc)GetProcAddress(g_hRendererDll, "RallyMain");
#endif /* ENABLE_PATCHES */
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
    InitializeApplicationDiagnostics();
    WriteApplicationDiagnostic("BRally entered WinMain; requesting desktop window.\n");
    HWND desktop = GetDesktopWindow();
    WriteApplicationDiagnostic("Desktop window query completed; loading launcher configuration.\n");
    ReadInstallDirectoryFromRegistry();
    strcpy(g_szIniFilePath, g_szInstallDir);
    strcat(g_szIniFilePath, "BossRally.ini");
    if (!CHK_FileExists(g_szIniFilePath)) {
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
    WriteApplicationDiagnostic("Loading renderer DLL.\n");
    if (!LoadRendererDll(rendererPath)) {
        WriteApplicationDiagnostic("Renderer DLL load failed.\n");
        char message[1024];
#ifdef ENABLE_PATCHES
        snprintf(message, sizeof(message), "Error: failed to load %s.", rendererPath);
#else
        sprintf(message, "Error: failed to load %s.", rendererPath);
#endif /* ENABLE_PATCHES */
        MessageBoxA(desktop, message, "Boss Rally", MB_ICONHAND);
        exit(1);
    }
    WriteApplicationDiagnostic("Calling renderer entry point.\n");
    int result = g_pfnRallyMain(instance, previousInstance, commandLine, showCommand);
    WriteApplicationDiagnostic("Renderer returned; unloading DLL.\n");
    UnloadRendererDll();
    ShutdownApplicationDiagnostics();
    return result;
}
