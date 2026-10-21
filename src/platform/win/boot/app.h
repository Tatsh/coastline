#pragma once

#include <windows.h>

/**
 * Exit code RallyMain() returns to the launcher.
 *
 * @ghidraAddress BRD3D: 0x118abe2c
 * @ghidraAddress BRGlide: 0x105ccbc0
 */
extern DWORD g_dwAppExitCode;

/**
 * Directory the game is installed in, read from the registry, with a trailing separator.
 *
 * @ghidraAddress BRD3D: 0x10b501e0
 * @ghidraAddress BRGlide: 0x10b73540
 */
extern char g_szInstallDirectory[];

/**
 * Path of BossRally.cfg, built from g_szInstallDirectory.
 *
 * @ghidraAddress BRD3D: 0x10b4fbe8
 * @ghidraAddress BRGlide: 0x10b72f48
 */
extern char g_szConfigFilePath[];

/**
 * Arguments of WinMain() as the launcher passed them to RallyMain(). Only the Glide library
 * stores them.
 *
 * @ghidraAddress BRGlide: 0x105bc730
 */
extern HINSTANCE g_hRallyMainInstance;

/** @ghidraAddress BRGlide: 0x105bc734 */
extern HINSTANCE g_hRallyMainPrevInstance;

/** @ghidraAddress BRGlide: 0x105bc738 */
extern LPSTR g_pszRallyMainCmdLine;

/** @ghidraAddress BRGlide: 0x105bc73c */
extern int g_nRallyMainCmdShow;

/**
 * Query the installed DirectX version.
 *
 * @param pdwVersion Receives the version, 0x600 for DirectX 6.
 * @param pdwRevision Receives the revision.
 * @ghidraAddress BRD3D: 0x10030210
 * @ghidraAddress BRGlide: 0x1001d8a0
 */
void GetDXVersion(DWORD *pdwVersion, DWORD *pdwRevision);

/**
 * Bring an instance of the game that is already running to the front.
 *
 * @return Whether this instance is the only one and may start.
 * @ghidraAddress BRD3D: 0x10007b10
 * @ghidraAddress BRGlide: 0x10007e80
 */
BOOL FocusExistingBossRallyInstance(void);

/**
 * Read the amount of physical memory the machine has.
 *
 * @ghidraAddress BRD3D: 0x10007ba0
 * @ghidraAddress BRGlide: 0x10007f10
 */
void RefreshTotalPhysicalMemory(void);

/**
 * Read the install directory from the registry into g_szInstallDirectory.
 *
 * @ghidraAddress BRD3D: 0x1006a8a0
 * @ghidraAddress BRGlide: 0x10063860
 */
void ReadInstallDirectoryFromRegistry(void);

/**
 * Create the main window and the Direct3D device. Only the Direct3D library has it.
 *
 * @return Whether the application started.
 * @ghidraAddress BRD3D: 0x100796b0
 */
BOOL InitializeApplication(void);

/**
 * Create the main window.
 *
 * @return Whether the window was created.
 * @ghidraAddress BRD3D: 0x100798f0
 * @ghidraAddress BRGlide: 0x10019670
 */
BOOL CreateMainWindow(void);

/**
 * Run the message loop until the game quits.
 *
 * @ghidraAddress BRD3D: 0x10079740
 * @ghidraAddress BRGlide: 0x10019730
 */
void RunMessageLoop(void);

/**
 * Destroy the main window. Only the Direct3D library has it.
 *
 * @ghidraAddress BRD3D: 0x10079720
 */
void ShutdownMainWindow(void);
