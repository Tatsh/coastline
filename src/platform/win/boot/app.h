#pragma once

#include <cstddef>

#include <windows.h>

/**
 * Exit code RallyMain() returns to the launcher.
 *
 * @ghidraAddress BRD3D: 0x118abe2c
 * @ghidraAddress BRGlide: 0x105ccbc0
 */
extern int g_dwAppExitCode;

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
 * The amount of physical memory of the machine, in bytes.
 *
 * @ghidraAddress BRD3D: 0x1022b348
 * @ghidraAddress BRGlide: 0x10226e78
 */
extern size_t g_dwTotalPhysicalMemory;

/**
 * The module of the game, set by InitializeApplication().
 *
 * @ghidraAddress BRD3D: 0x118abe20
 */
extern HINSTANCE g_hAppInstance;

/**
 * The main window.
 *
 * @ghidraAddress BRD3D: 0x10680584
 */
extern HWND g_hMainWindow;

/**
 * The keyboard accelerators of the main window.
 *
 * @ghidraAddress BRD3D: 0x118abe24
 */
extern HACCEL g_hAccelTable;

/**
 * Name of the window class of the main window.
 *
 * @ghidraAddress BRD3D: 0x100bd594
 */
extern const char *g_pszMainWindowClassName;

/**
 * Whether the application is active. A frame runs only while it, g_dwActivateState, and not
 * g_dwActivateMinimized are set.
 *
 * @ghidraAddress BRD3D: 0x10680598
 */
extern bool g_dwAppActive;

/**
 * Whether the main window is activated.
 *
 * @ghidraAddress BRD3D: 0x1068059c
 */
extern bool g_dwActivateState;

/**
 * Whether the main window is minimised.
 *
 * @ghidraAddress BRD3D: 0x106805a0
 */
extern bool g_dwActivateMinimized;

/** The DirectDraw renderer of a window. The layout is not recovered yet. */
class DirectDrawRenderer {
public:
    /**
     * Release the surfaces and the device of the renderer.
     *
     * @ghidraAddress BRD3D: 0x1000a1c0
     */
    void Cleanup();
};

/**
 * The renderer of the main window.
 *
 * @ghidraAddress BRD3D: 0x1027736c
 */
extern DirectDrawRenderer *g_pMainRenderWindow;

/**
 * Destroy the main window and its renderer.
 *
 * @return 0.
 * @ghidraAddress BRD3D: 0x10079c40
 */
int DestroyMainWindow(void);

/**
 * Run a frame of the current state of the game.
 *
 * @ghidraAddress BRD3D: 0x1002f8f0
 */
void DispatchGameStateFrame(void);

/**
 * Do nothing.
 *
 * @ghidraAddress BRD3D: 0x10008b80
 */
void NullStub(void);

/**
 * Initialise DirectPlay.
 *
 * @ghidraAddress BRD3D: 0x1003bd80
 */
void InitializeDirectPlay(void);

/**
 * Query the installed DirectX version.
 *
 * @param pdwVersion Receives the version, 0x600 for DirectX 6.
 * @param pdwRevision Receives the revision.
 * @ghidraAddress BRD3D: 0x10030210
 * @ghidraAddress BRGlide: 0x1001d8a0
 */
void GetDXVersion(unsigned *pdwVersion, unsigned *pdwRevision);

/**
 * Bring an instance of the game that is already running to the front.
 *
 * @return Whether this instance is the only one and may start.
 * @ghidraAddress BRD3D: 0x10007b10
 * @ghidraAddress BRGlide: 0x10007e80
 */
bool FocusExistingBossRallyInstance(void);

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
 * Start the application: refuse DirectX before version 6 and a second instance, then create the
 * main window. Only the Direct3D library has it.
 *
 * @return Whether the application started.
 * @ghidraAddress BRD3D: 0x100796b0
 */
bool InitializeApplication(void);

/**
 * Create the main window.
 *
 * @return Whether the window was created.
 * @ghidraAddress BRD3D: 0x100798f0
 * @ghidraAddress BRGlide: 0x10019670
 */
bool CreateMainWindow(void);

/**
 * Run the message loop until the game quits.
 *
 * @ghidraAddress BRD3D: 0x10079740
 * @ghidraAddress BRGlide: 0x10019730
 * @stub{BRGlide}
 */
void RunMessageLoop(void);

/**
 * Destroy the main window. Only the Direct3D library has it.
 *
 * @ghidraAddress BRD3D: 0x10079720
 */
void ShutdownMainWindow(void);
