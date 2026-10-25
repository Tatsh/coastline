#pragma once

#include <windows.h>
// The headers below need the types of windows.h.
#include <ddraw.h>

/** The number of names of a SaveFileList. */
#define SAVE_FILE_NAME_COUNT 100
/** The size of a name of a SaveFileList. */
#define SAVE_FILE_NAME_SIZE 0x104
/** The number of widgets of a MenuPanel. */
#define MENU_PANEL_WIDGET_COUNT 200
/** The number of panels of a MenuScreen. */
#define MENU_SCREEN_PANEL_COUNT 20

/** The names of the saved files a screen of the front end lists. */
class SaveFileList {
public:
    char aszNames[SAVE_FILE_NAME_COUNT][SAVE_FILE_NAME_SIZE]; /*!< The names, empty at the start. */

    /**
     * Clear every name.
     *
     * @ghidraAddress BRD3D: 0x1005cb90
     * @ghidraAddress BRGlide: 0x100559b0
     */
    SaveFileList();

    /**
     * Do nothing.
     *
     * @ghidraAddress BRD3D: 0x1005cc10
     * @ghidraAddress BRGlide: 0x10055a30
     */
    virtual ~SaveFileList();
};

/** An item of a panel of the front end. The layout is not recovered past the position. */
class MenuWidget {
public:
    /**
     * @ghidraAddress BRD3D: 0x100478c0
     * @ghidraAddress BRGlide: 0x10040d10
     */
    virtual ~MenuWidget();
    /**
     * @ghidraAddress BRD3D: 0x100480a0
     * @ghidraAddress BRGlide: 0x100414f0
     */
    virtual void UpdateBlinkTimer();
    /**
     * @ghidraAddress BRD3D: 0x10048010
     * @ghidraAddress BRGlide: 0x10041460
     */
    virtual void Draw();
    /**
     * Run a frame of the item.
     *
     * @return Whether the item still runs.
     * @ghidraAddress BRD3D: 0x10048180
     * @ghidraAddress BRGlide: 0x100415d0
     */
    virtual bool Update();

    char abUnknown4[0x38]; /* +0x04 */
    float fX;              /*!< The left of the item on the screen. */
    float fY;              /*!< The top of the item on the screen. */
};

/** The index in apWidgets of the mouse cursor of the first panel of the root screen. */
#define MENU_CURSOR_WIDGET_INDEX (MENU_PANEL_WIDGET_COUNT - 1)

/** A panel of a screen of the front end. The layout is not recovered past the widgets. */
class MenuPanel {
public:
    char abUnknown4[0x14];                          /* +0x04 */
    MenuWidget *apWidgets[MENU_PANEL_WIDGET_COUNT]; /*!< The items of the panel. */
    char abUnknown338[0x10];                        /* +0x338 */

    /**
     * @ghidraAddress BRD3D: 0x100484e0
     * @ghidraAddress BRGlide: 0x10041930
     */
    virtual ~MenuPanel();
    /**
     * Run a frame of every item of the panel.
     *
     * @return Whether the panel still runs.
     * @ghidraAddress BRD3D: 0x10048530
     * @ghidraAddress BRGlide: 0x10041980
     */
    virtual bool RunFrame();
};

/** A screen of the front end menus. */
class MenuScreen {
public:
    /**
     * Create a running screen with two lists of saved files.
     *
     * @ghidraAddress BRD3D: 0x10048710
     * @ghidraAddress BRGlide: 0x10041b60
     */
    MenuScreen();
    /**
     * Free the lists of saved files.
     *
     * @ghidraAddress BRD3D: 0x10048870
     * @ghidraAddress BRGlide: 0x10041cc0
     */
    virtual ~MenuScreen();
    /**
     * Start a frame of the screen.
     *
     * @return Always true.
     * @ghidraAddress BRD3D: 0x100488b0
     * @ghidraAddress BRGlide: 0x10041d00
     */
    virtual bool BeginFrame();
    /**
     * Finish a frame of the screen.
     *
     * @return Whether the frame finished.
     * @ghidraAddress BRD3D: 0x100488c0
     * @ghidraAddress BRGlide: 0x10041d10
     */
    virtual bool EndFrame();
    /**
     * Run a frame of the screen, and shut the front end down once the screen stops running.
     *
     * @return Whether the screen still runs.
     * @ghidraAddress BRD3D: 0x100489a0
     * @ghidraAddress BRGlide: 0x10041dd0
     */
    virtual bool RunFrame();
    /**
     * Make the screen the one that shows.
     *
     * @return The result of the activation. Its meaning is not recovered yet.
     * @ghidraAddress BRD3D: 0x1005ae70
     * @ghidraAddress BRGlide: 0x10053d20
     */
    virtual int ActivateMenuScreen();
    /**
     * Flip the frame to the display and wait for the flip to finish. The Glide library flips its
     * own way.
     *
     * @ghidraAddress BRD3D: 0x10048960
     * @ghidraAddress BRGlide: 0x10041db0
     * @stub{BRGlide}
     */
    virtual void FlipAndWait();
    /**
     * Free the cached screens of the front end.
     *
     * @param bPartial Whether to free only the cached screens. When false, the menu images, the
     * root screen, and the scratch buffer are freed as well, and no screen shows afterwards.
     * @ghidraAddress BRD3D: 0x10048b20
     * @ghidraAddress BRGlide: 0x10041f50
     */
    virtual void ShutdownFrontEnd(int bPartial);
    /**
     * Delete every widget of every panel and the panels, and put the hover back on the first item.
     * The pointers to the panels and their count are retained.
     *
     * @ghidraAddress BRD3D: 0x10048aa0
     * @ghidraAddress BRGlide: 0x10041ed0
     */
    virtual void DestroyPanels();
    /**
     * Clear the back buffer of the display. The Glide library does nothing here.
     *
     * @return 0, or E_FAIL when the renderer has no device or viewport.
     * @ghidraAddress BRD3D: 0x1005afa0
     */
    virtual HRESULT ClearBackBuffer();

    void (*pfnBuildMenu)(MenuScreen *pScreen); /*!< Builds the items of the screen. */
    int dwFlags;                  /*!< Options of the screen. Bit 0x10 stops EndFrame(). */
    int nUnknownC;                /*!< Set as the screen shows. Its meaning is not recovered yet. */
    unsigned short wPanelCount;   /*!< The number of entries of apPanels in use. */
    unsigned short wCurrentPanel; /*!< The index of the panel that runs. */
    MenuPanel *apPanels[MENU_SCREEN_PANEL_COUNT]; /*!< The panels of the screen. */
    MenuPanel *pCurrentPanel;                     /*!< The panel that runs. */
    int bRunning; /*!< Whether the screen runs and reads window messages. */
    /** Whether each panel of apPanels runs its items. */
    int anPanelActive[MENU_SCREEN_PANEL_COUNT];
    unsigned short wUnknownBC; /*!< Cleared at the start. Its meaning is not recovered yet. */
    char abUnknownBE[2];       /* +0xbe */
    SaveFileList *apFiles[2];  /*!< The two lists of saved files of the screen. */
};

/**
 * Set up the front end. The paths of the 145 menu images are allocated and filled, the resources
 * are reset, the root screen is created as the active one, the scratch buffer is allocated, and
 * the paths of the saved season and ghost and the rectangles of the sprites are set.
 *
 * @return Whether the root screen was created.
 * @ghidraAddress BRD3D: 0x1005d440
 * @ghidraAddress BRGlide: 0x10056260
 */
bool InitializeFrontEndResources(void);

/**
 * The screen of the front end that is showing.
 *
 * @ghidraAddress BRD3D: 0x10aa2904
 * @ghidraAddress BRGlide: 0x10ac5c5c
 */
extern MenuScreen *g_pActiveMenuScreen;

/**
 * The list of sessions of the screen to join or create a network game, while it exists.
 *
 * @ghidraAddress BRD3D: 0x10aa29d4
 * @ghidraAddress BRGlide: 0x10ac5d2c
 */
extern MenuWidget *g_pJoinCreateGameMenu;

/**
 * A buffer of the front end, passed to FrontEndWindowProc() and not read by it.
 *
 * @ghidraAddress BRD3D: 0x10aa2900
 * @ghidraAddress BRGlide: 0x10ac5c58
 */
extern void *g_pFrontEndScratchBuffer;

/** The number of entries of g_aMenuImages. */
#define MENU_IMAGE_COUNT 145
/** The size of the path of a menu image. */
#define MENU_IMAGE_PATH_SIZE 0x104

/** An image of the menus and the file it is loaded from. */
struct MenuImage {
#if TGR_RENDERER == TGR_RENDERER_D3D
    DDSURFACEDESC desc;  /*!< The description of the surface of the image. */
    char abUnknown6c[4]; /* +0x6c */
#else
    char abUnknown0[4]; /* +0x00 */
#endif
    char *pszPath; /*!< The path of the file of the image, from new. */
};

/**
 * The images of the menus.
 *
 * @ghidraAddress BRD3D: 0x10a9e360
 * @ghidraAddress BRGlide: 0x10ac53e8
 */
extern MenuImage g_aMenuImages[MENU_IMAGE_COUNT];

/**
 * The number of menu images loaded.
 *
 * @ghidraAddress BRD3D: 0x10aa28d4
 * @ghidraAddress BRGlide: 0x10ac5c2c
 */
extern unsigned short g_wMenuImageLoadedCount;

/**
 * A word InitializeFrontEndResources() clears and no routine reads.
 *
 * @ghidraAddress BRD3D: 0x10aa29f8
 * @ghidraAddress BRGlide: 0x10ac5d50
 */
extern unsigned short g_wUnusedFrontEndWord0;

/**
 * A word InitializeFrontEndResources() clears and no routine reads.
 *
 * @ghidraAddress BRD3D: 0x10aa29fc
 * @ghidraAddress BRGlide: 0x10ac5d54
 */
extern unsigned short g_wUnusedFrontEndWord1;

/** A resource of the front end. Its layout is not recovered yet. */
struct FrontEndResourceSlot {
    int anValues[3]; /*!< Values that start at -1 but the third, which starts at 0. */
};

/** The number of entries of g_aFrontEndResourceSlots. */
#define FRONT_END_RESOURCE_SLOT_COUNT 60
/** The number of entries of g_adwFrontEndResourceFlags. */
#define FRONT_END_RESOURCE_FLAG_COUNT 100

/**
 * The resources of the front end.
 *
 * @ghidraAddress BRD3D: 0x10a9d778
 * @ghidraAddress BRGlide: 0x10ac4800
 */
extern FrontEndResourceSlot g_aFrontEndResourceSlots[FRONT_END_RESOURCE_SLOT_COUNT];

/**
 * Flags of the resources of the front end, -1 at the start. Their meaning is not recovered yet.
 *
 * @ghidraAddress BRD3D: 0x10a9e1d0
 * @ghidraAddress BRGlide: 0x10ac5258
 */
extern int g_adwFrontEndResourceFlags[FRONT_END_RESOURCE_FLAG_COUNT];

/**
 * The screen the front end starts at.
 *
 * @ghidraAddress BRD3D: 0x10aa2908
 * @ghidraAddress BRGlide: 0x10ac5c60
 */
extern MenuScreen *g_pRootMenuScreen;

/**
 * The path of the file of the saved season.
 *
 * @ghidraAddress BRD3D: 0x11782cd0
 * @ghidraAddress BRGlide: 0x117a6030
 */
extern char g_szSeasonSaveFileName[];

/**
 * The path of the file of the saved ghost of a time attack.
 *
 * @ghidraAddress BRD3D: 0x11782bc8
 * @ghidraAddress BRGlide: 0x117a5f28
 */
extern char g_szTimeAttackSaveFileName[];

/**
 * Build the items of the root screen of the front end.
 *
 * @param pScreen The screen.
 * @ghidraAddress BRD3D: 0x100491b0
 * @ghidraAddress BRGlide: 0x100425e0
 */
void BuildFrontEndRootMenuUI(MenuScreen *pScreen);

/** The number of entries of g_aMenuGlyphRects. */
#define MENU_GLYPH_RECT_COUNT 68
/** The number of entries of g_aMenuAltGlyphRects. */
#define MENU_ALT_GLYPH_RECT_COUNT 20
/** The number of entries of g_aMenuLargeIconRects. */
#define MENU_LARGE_ICON_RECT_COUNT 15
/** The number of entries of g_aMenuLargeIconRects3Col. */
#define MENU_LARGE_ICON_3_COLUMN_RECT_COUNT 9

/**
 * The 16 by 16 glyphs of the menus, eight to a row.
 *
 * @ghidraAddress BRD3D: 0x10a9d180
 * @ghidraAddress BRGlide: 0x10ac4208
 */
extern RECT g_aMenuGlyphRects[MENU_GLYPH_RECT_COUNT];

/**
 * The 39 by 44 glyphs of the menus, five to a row.
 *
 * @ghidraAddress BRD3D: 0x10a9d638
 * @ghidraAddress BRGlide: 0x10ac46c0
 */
extern RECT g_aMenuAltGlyphRects[MENU_ALT_GLYPH_RECT_COUNT];

/**
 * The 128 by 128 icons of the menus, five to a row.
 *
 * @ghidraAddress BRD3D: 0x10a9da50
 * @ghidraAddress BRGlide: 0x10ac4ad8
 */
extern RECT g_aMenuLargeIconRects[MENU_LARGE_ICON_RECT_COUNT];

/**
 * The 128 by 128 icons of the menus, three to a row. The table follows g_aMenuLargeIconRects.
 *
 * @ghidraAddress BRD3D: 0x10a9db40
 * @ghidraAddress BRGlide: 0x10ac4bc8
 */
extern RECT g_aMenuLargeIconRects3Col[MENU_LARGE_ICON_3_COLUMN_RECT_COUNT];

/**
 * Set the rectangles of the sprites of the menus.
 *
 * @ghidraAddress BRD3D: 0x1005f800
 * @ghidraAddress BRGlide: 0x10058540
 */
void InitMenuSpriteRects(void);

/**
 * Copy the choices of the menus to adwMenuSelectionBackup of g_GlobalRallyConfig.
 *
 * @ghidraAddress BRD3D: 0x1003e310
 * @ghidraAddress BRGlide: 0x10037920
 */
void BackupMenuSelections(void);

/**
 * The number of frames of the front end, counted while the music is not chosen by hand.
 *
 * @ghidraAddress BRD3D: 0x10aa2a4c
 * @ghidraAddress BRGlide: 0x10ac5da4
 */
extern int g_dwFrontEndFrameCounter;

/**
 * The track of the CD the menus show, counted from the first music track.
 *
 * @ghidraAddress BRD3D: 0x10aa2a34
 * @ghidraAddress BRGlide: 0x10ac5d8c
 */
extern int g_dwSelectedCdTrack;

/**
 * Whether the list of network sessions is refreshing. No frame is flipped while it refreshes.
 *
 * @ghidraAddress BRD3D: 0x10aa2874
 * @ghidraAddress BRGlide: 0x10ac5bcc
 */
extern int g_dwSessionListRefreshing;

/**
 * Whether the root screen is the screen that shows.
 *
 * @ghidraAddress BRD3D: 0x10aa2868
 * @ghidraAddress BRGlide: 0x10ac5bc0
 */
extern int g_dwFrontEndAtRootMenu;

/**
 * The chosen track.
 *
 * @ghidraAddress BRD3D: 0x100ac648
 * @ghidraAddress BRGlide: 0x100abde8
 */
extern int g_dwTrackMenuIndex;

/**
 * The chosen weather.
 *
 * @ghidraAddress BRD3D: 0x10aa2a00
 * @ghidraAddress BRGlide: 0x10ac5d58
 */
extern int g_dwSelectedWeather;

/**
 * The chosen transmission.
 *
 * @ghidraAddress BRD3D: 0x10aa2a08
 * @ghidraAddress BRGlide: 0x10ac5d60
 */
extern int g_dwSelectedTransmission;

/**
 * The chosen tyres.
 *
 * @ghidraAddress BRD3D: 0x100ac64c
 * @ghidraAddress BRGlide: 0x100abdec
 */
extern int g_dwTireMenuIndex;

/**
 * The chosen suspension.
 *
 * @ghidraAddress BRD3D: 0x100ac650
 * @ghidraAddress BRGlide: 0x100abdf0
 */
extern int g_dwSuspensionMenuIndex;

/**
 * The chosen car.
 *
 * @ghidraAddress BRD3D: 0x100ac654
 * @ghidraAddress BRGlide: 0x100abdf4
 */
extern int g_dwCarMenuIndex;

/**
 * The chosen input device.
 *
 * @ghidraAddress BRD3D: 0x10aa2a0c
 * @ghidraAddress BRGlide: 0x10ac5d64
 */
extern int g_dwSelectedInputDevice;

/**
 * The chosen number of laps.
 *
 * @ghidraAddress BRD3D: 0x100ac658
 * @ghidraAddress BRGlide: 0x100abdf8
 */
extern int g_dwTotalLapsSetting;

/**
 * The cars unlocked so far, one bit to a car.
 *
 * @ghidraAddress BRD3D: 0x10aa2a10
 * @ghidraAddress BRGlide: 0x10ac5d68
 */
extern int g_dwAccumUnlockedCarMask;

/**
 * The tracks unlocked so far, one bit to a track.
 *
 * @ghidraAddress BRD3D: 0x10aa2a14
 * @ghidraAddress BRGlide: 0x10ac5d6c
 */
extern int g_dwAccumUnlockedTrackMask;

/**
 * The chosen handling.
 *
 * @ghidraAddress BRD3D: 0x100ac65c
 * @ghidraAddress BRGlide: 0x100abdfc
 */
extern int g_dwHandlingTypeMenuIndex;

/**
 * The chosen class of car.
 *
 * @ghidraAddress BRD3D: 0x10aa2a18
 * @ghidraAddress BRGlide: 0x10ac5d70
 */
extern int g_dwSelectedCarClass;

/**
 * Show an error of the game.
 *
 * @param nErrorCode The error.
 * @ghidraAddress BRD3D: 0x1003e260
 * @ghidraAddress BRGlide: 0x100378c0
 */
void ShowErrorMessage(int nErrorCode);

/**
 * Give a message of the main window to the front end.
 *
 * @param pUnused Not read.
 * @param hWnd The main window.
 * @param nMessage The message.
 * @param wParam The first parameter of the message.
 * @param lParam The second parameter of the message.
 * @return The result of the message for the front end.
 * @ghidraAddress BRD3D: 0x10060060
 */
LRESULT WINAPI
FrontEndWindowProc(void *pUnused, HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);
