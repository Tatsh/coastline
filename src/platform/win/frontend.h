#pragma once

/**
 * @file
 * The front end of the PC port: its menus and their surfaces, the cheats, the selections it
 * restores, and the window procedure of the menus.
 */

#include <windows.h>
// The headers below need the types of windows.h.
#include <cstdio>

#include <ddraw.h>

#include "platform/win/menufont.h"

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
    char aszNames[SAVE_FILE_NAME_COUNT][SAVE_FILE_NAME_SIZE]; /*!< The names, empty at the start. */

public:
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

private:
    /**
     * Read descriptions from the last 128 bytes of matching saved files.
     *
     * @param pszPattern The file search pattern.
     * @return Zero when the first search fails, otherwise one.
     * @ghidraAddress BRD3D: 0x1005cf20
     */
    virtual int ScanSaveFiles(const char *pszPattern);

    /**
     * Count matches after the first, stopping after 100 files.
     *
     * @param pszPattern The file search pattern.
     * @return Minus one when the first search fails, otherwise the count after the first match.
     * @ghidraAddress BRD3D: 0x1005d0a0
     */
    virtual int CountMatchingFiles(const char *pszPattern);

    /**
     * Replace pattern markers with zeroes and derive a filename from matching list entries.
     *
     * @param pszPattern The mutable pattern, with X or x marking the numeric suffix.
     * @param ppszName The destination filename buffer.
     * @ghidraAddress BRD3D: 0x1005d110
     */
    virtual void GenerateNextSaveFileName(char *pszPattern, char **ppszName);

    /**
     * Write 100-byte records from the active screen's first file list.
     *
     * @param nUnused Not read.
     * @return One, including after reported file errors.
     * @ghidraAddress BRD3D: 0x1005cc20
     */
    virtual int WriteSeasonDescFile(int nUnused);

    /**
     * Load and uppercase a season description into the active screen's first file list.
     *
     * @param nSlot The signed slot index.
     * @return One, including when the file does not open.
     * @ghidraAddress BRD3D: 0x1005ccd0
     */
    virtual int LoadSeasonDescription(short nSlot);

    /**
     * Set a root screen file-list entry using the numeric part of its filename.
     *
     * @param pszFileName The saved filename.
     * @param pszDescription The description copied into the entry.
     * @return One.
     * @ghidraAddress BRD3D: 0x1005ce30
     */
    virtual int SetEntryFromSaveFileName(const char *pszFileName, const char *pszDescription);

public:
    const char *GetName(int index) const {
        return aszNames[index];
    }
    void SetName(int index, const char *text) {
        std::strcpy(aszNames[index], text);
    }
    void SetFormattedName(int index, const char *format, int value) {
        std::sprintf(aszNames[index], format, value);
    }
};

class MenuScreen;
class MenuWidget;
using MenuActionWidget = MenuWidget;
using NetworkMenuWidget = MenuWidget;
class MenuItemList;

struct MenuItemData {
    unsigned dwSize;
    unsigned char *pData;
};

class MenuItemList {
public:
    /**
     * @ghidraAddress BRD3D: 0x1005b7f0
     */
    MenuItemList();
    /**
     * @ghidraAddress BRD3D: 0x1005b8f0
     */
    virtual ~MenuItemList();

private:
    /**
     * @ghidraAddress BRD3D: 0x1005c270
     */
    virtual int BlitSurfaceById(unsigned short image, float x, float y);
    /**
     * @ghidraAddress BRD3D: 0x1005c2c0
     */
    virtual int Draw(int selection);
    /**
     * @ghidraAddress BRD3D: 0x10042af0
     * @stub
     */
    virtual int ReturnOne();

public:
    /**
     * @ghidraAddress BRD3D: 0x1005bc10
     */
    virtual int
    AddItem(const char *text, unsigned flags, unsigned char style, const RECT *bounds, bool full);
    /**
     * @ghidraAddress BRD3D: 0x1005b910
     */
    virtual int Initialize(unsigned flags,
                           const RECT *bounds,
                           int visibleRows,
                           int firstVisibleRow,
                           int backgroundImage);

private:
    /**
     * @ghidraAddress BRD3D: 0x1005c510
     */
    virtual int UpdateHoverInRect(const RECT *bounds);
    /**
     * @ghidraAddress BRD3D: 0x1005c590
     */
    virtual int ProcessInput(int *selection);

public:
    /**
     * @ghidraAddress BRD3D: 0x1005cb40
     */
    virtual int UpdateAndRenderMenuWidget(int selection);
    /**
     * @ghidraAddress BRD3D: 0x1005bb80
     * @ghidraAddress BRGlide: 0x100549a0
     */
    virtual int UpdateItemAndNotify(int nIndex);

    /**
     * @ghidraAddress BRD3D: 0x1005c200
     */
    virtual int SetItemData(const void *data, unsigned size, int index);

    /**
     * @ghidraAddress BRD3D: 0x1005c000
     */
    virtual int RemoveItem(int index);

    MenuFont &GetFont(int index) {
        return fonts[index];
    }
    unsigned short GetItemCount() const {
        return wItemCount;
    }
    const void *GetItemData(int index) const {
        return itemData[index].pData;
    }
    unsigned GetFlags() const {
        return dwFlags;
    }
    unsigned SetFlags(unsigned value) {
        return dwFlags = value;
    }
    int SetVerticalScrollbar(int value) {
        return bVerticalScrollbar = value;
    }
    void SetSelectionChangedCallback(int (*callback)(MenuItemList *, int *)) {
        pfnSelectionChanged = callback;
    }
    float GetMinScrollThumbY() const {
        return fMinScrollThumbY;
    }
    float GetMaxScrollThumbY() const {
        return fMaxScrollThumbY;
    }
    float GetScrollThumbY() const {
        return fScrollThumbY;
    }
    void SetScrollThumbY(float value) {
        fScrollThumbY = value;
    }
    void SetScrollThumbTop(int value) {
        rcScrollThumb.top = value;
    }
    void SetScrollThumbBottom(int value) {
        rcScrollThumb.bottom = value;
    }

private:
    int (*pfnSelectionChanged)(MenuItemList *, int *);
    void (*pfnScrollUp)(void);
    void (*pfnScrollDown)(void);
    void (*pfnScrollDrag)(void);
    int (*pfnItemChanged)(MenuItemList *, int);
    unsigned dwFlags;
    float fX;
    float fY;
    float fBackgroundX;
    float fBackgroundY;
    MenuFont fonts[100];
    MenuItemData itemData[100];
    unsigned short wItemCount;
    short nFirstVisibleRow;
    unsigned short wVisibleRowCount;
    short nScrollUpImage;
    short nScrollDownImage;
    short nBackgroundImage;
    short nScrollThumbImage;
    char abUnknown1a93a[2]; /* +0x1a93a */
    RECT rcBounds;
    RECT rcScrollUp;
    RECT rcScrollDown;
    RECT rcHorizontalScrollLeft;
    RECT rcHorizontalScrollRight;
    RECT rcScrollThumb;
    int bScrollUpHovered;
    int bScrollDownHovered;
    char abUnknown1a9a4[8]; /* +0x1a9a4 */
    float fScrollThumbX;
    float fScrollThumbY;
    int bScrollDragging;
    int bHorizontalScrollbar;
    int bVerticalScrollbar;
    float fMinScrollThumbX;
    float fMaxScrollThumbX;
    float fMinScrollThumbY;
    float fMaxScrollThumbY;
    float fScrollThumbTravel;
};

/** An item of a panel of the front end. */
class MenuWidget {
public:
    /**
     * @ghidraAddress BRD3D: 0x100476c0
     */
    MenuWidget();
    /**
     * @ghidraAddress BRD3D: 0x100478c0
     * @ghidraAddress BRGlide: 0x10040d10
     */
    virtual ~MenuWidget();
    /**
     * @ghidraAddress BRD3D: 0x100480a0
     * @ghidraAddress BRGlide: 0x100414f0
     */
    virtual int UpdateBlinkTimer();

private:
    /**
     * @ghidraAddress BRD3D: 0x10048010
     * @ghidraAddress BRGlide: 0x10041460
     */
    virtual int Draw();

public:
    /**
     * Run a frame of the item.
     *
     * @return Whether the item still runs.
     * @ghidraAddress BRD3D: 0x10048180
     * @ghidraAddress BRGlide: 0x100415d0
     */
    virtual bool Update();

private:
    /**
     * @ghidraAddress BRD3D: 0x10047a10
     */
    virtual int DrawCurrentImage();

public:
    /**
     * @ghidraAddress BRD3D: 0x100479d0
     */
    virtual int BlitSurfaceById(int nImage, int x, int y);

private:
    /**
     * @ghidraAddress BRD3D: 0x10047980
     */
    virtual int DrawSpriteRegion(const RECT *pRect);
    /**
     * @ghidraAddress BRD3D: 0x10047930
     */
    virtual int DrawSprite();
    /**
     * @ghidraAddress BRD3D: 0x10047a60
     */
    virtual bool UpdateHoverState();
    /**
     * @ghidraAddress BRD3D: 0x10047cb0
     */
    virtual int InitSlideAnimation(int nDuration);
    /**
     * @ghidraAddress BRD3D: 0x10047ce0
     */
    virtual long double ComputeSlideOffset(int nTicks);
    /**
     * @ghidraAddress BRD3D: 0x10047d10
     */
    virtual int StartSlideAnimation();
    /**
     * @ghidraAddress BRD3D: 0x10047d30
     */
    virtual int UpdateSlideAnimation();

public:
    /**
     * @ghidraAddress BRD3D: 0x10047eb0
     */
    virtual void
    SetText(const char *pszText, unsigned dwTextFlags, unsigned char bStyle, const RECT *pRect);
    /**
     * @ghidraAddress BRD3D: 0x10047fb0
     */
    virtual void Initialize(MenuScreen *pOwner,
                            float x,
                            float y,
                            unsigned dwItemFlags,
                            unsigned dwSecondary,
                            unsigned dwTertiary,
                            int nMode,
                            short nImage);

private:
    /**
     * @ghidraAddress BRD3D: 0x10048060
     */
    virtual bool IsBlockedByModalDialog();

    MenuWidget *LinkedMenuWidget(int index);
    bool UpdateMenuSlideAxis(int enabled,
                             signed char direction,
                             const float &saved,
                             const float &target,
                             float &current);

public:
    MenuScreen *GetScreen() const {
        return pScreen;
    }
    MenuFont &GetFont(int index) {
        return fonts[index];
    }
    MenuItemList &GetItemList() {
        return itemList;
    }
    unsigned GetFlags() const {
        return dwFlags;
    }
    void SetFlags(unsigned value) {
        dwFlags = value;
    }
    float GetX() const {
        return fX;
    }
    float GetY() const {
        return fY;
    }
    void SetX(float value) {
        fX = value;
    }
    void SetY(float value) {
        fY = value;
    }
    void SetSpriteId(short value) {
        nSpriteId = value;
    }
    short GetSpriteId() const {
        return nSpriteId;
    }
    void SetAnimationSpriteId(int index, short value) {
        anSpriteIds[index] = value;
    }
    void SetImageMode(int value) {
        nImageMode = value;
    }
    void SetImageRects(const RECT *rects) {
        pImageRects = rects;
    }
    void SetCustomFrameDurations(bool value) {
        bCustomFrameDurations = value;
    }
    void SetFrameDuration(int index, int value) {
        anFrameDurations[index] = value;
    }
    short GetLinkedWidgetCount() const {
        return nLinkedWidgetCount;
    }
    void SetLinkedWidgetCount(short value) {
        nLinkedWidgetCount = value;
    }
    void SetLinkedWidgetIndex(int index, short value) {
        anLinkedWidgetIndices[index] = value;
    }
    LONG GetBoundsLeft() const {
        return rcBounds.left;
    }
    LONG GetBoundsTop() const {
        return rcBounds.top;
    }
    void SetBoundsLeft(LONG value) {
        rcBounds.left = value;
    }
    void SetBoundsTop(LONG value) {
        rcBounds.top = value;
    }
    void SetBoundsRight(LONG value) {
        rcBounds.right = value;
    }
    void SetBoundsBottom(LONG value) {
        rcBounds.bottom = value;
    }
    void SetUpdateCallback(int (*callback)(MenuWidget *)) {
        pfnUpdate = callback;
    }
    void SetActionCallback(int (*callback)(MenuActionWidget *)) {
        pfnAction = callback;
    }
    auto GetActionCallback() const {
        return pfnAction;
    }
    void SetIdleActionCallback(int (*callback)(MenuWidget *)) {
        pfnIdleAction = callback;
    }
    void SetTextEditCompleteCallback(int (*callback)(MenuWidget *)) {
        pfnTextEditComplete = callback;
    }
    auto GetTextEditCompleteCallback() const {
        return pfnTextEditComplete;
    }
    void SetBeforeUpdateCallback(int (*callback)(MenuWidget *)) {
        pfnBeforeUpdate = callback;
    }
    void SetAfterUpdateCallback(int (*callback)(MenuWidget *)) {
        pfnAfterUpdate = callback;
    }

private:
    int (*pfnUpdate)(MenuWidget *);
    int (*pfnAction)(MenuActionWidget *);
    int (*pfnIdleAction)(MenuWidget *);
    int (*pfnTextEditComplete)(MenuWidget *);
    int (*pfnBeforeUpdate)(MenuWidget *);
    int (*pfnAfterUpdate)(MenuWidget *);
    unsigned dwFlags;
    [[maybe_unused]] char abUnknown20[4]; /* +0x20 */
    unsigned dwSecondaryFlags;
    unsigned dwTertiaryFlags;
    char abUnknown2c[4]; /* +0x2c */
    float fSavedX;
    float fSavedY;
    float fSavedZ;
    float fX;
    float fY;
    float fZ;
    short nWidth;
    short nHeight;
    [[maybe_unused]] char abUnknown4c[4]; /* +0x4c */
    RECT rcBounds;
    [[maybe_unused]] char abUnknown60[0xc8]; /* +0x60 */
    short nAnimationFrame;
    char abUnknown12a[10000];                 /* +0x12a */
    [[maybe_unused]] char abUnknown283a[2];   /* +0x283a */
    [[maybe_unused]] char abUnknown283c[200]; /* +0x283c */
    [[maybe_unused]] char abUnknown2904[100]; /* +0x2904 */
    int nImageMode;
    int bCustomFrameDurations;
    int nAnimationLastTime;
    int nAnimationElapsed;
    int anFrameDurations[50];
    short anSpriteIds[50];
    [[maybe_unused]] char abUnknown2aa4[10]; /* +0x2aa4 */
    [[maybe_unused]] char abUnknown2aae[6];  /* +0x2aae */
    short nLinkedWidgetCount;
    short anLinkedWidgetIndices[25];
    MenuScreen *pScreen;
    [[maybe_unused]] char abUnknown2aec[4]; /* +0x2aec */
    char abUnknown2af0[100];                /* +0x2af0 */
    [[maybe_unused]] char abUnknown2b54[4]; /* +0x2b54 */
    [[maybe_unused]] char abUnknown2b58[4]; /* +0x2b58 */
    MenuFont fonts[3];
    int bSlideX;
    int bSlideY;
    signed char nSlideDirectionX;
    signed char nSlideDirectionY;
    [[maybe_unused]] char abUnknown380e[2]; /* +0x380e */
    float fSlideTargetX;
    float fSlideTargetY;
    int bSliding;
    float fSlideStart;
    float fSlideEnd;
    float fSlideStep;
    int nSlideLastTime;
    int nSlideElapsed;
    [[maybe_unused]] char abUnknown3830[8]; /* +0x3830 */
    MenuItemList itemList;
    short nSpriteId;
    [[maybe_unused]] char abUnknown1e20e[2]; /* +0x1e20e */
    const RECT *pImageRects;
};

/**
 * @ghidraAddress BRD3D: 0x10043760
 * @ghidraAddress BRGlide: 0x1003ccb0
 */
int CarSetupUI_OnAcceptAndSave(MenuActionWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10041a00
 * @ghidraAddress BRGlide: 0x1003af60
 */
int ToggleMenuItemTextEditA(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10042410
 * @ghidraAddress BRGlide: 0x1003b970
 */
int ToggleMenuItemTextEditB(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10041ac0
 * @ghidraAddress BRGlide: 0x1003b020
 */
int RestoreMenuItemTextA(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100424d0
 * @ghidraAddress BRGlide: 0x1003ba30
 */
int RestoreMenuItemTextB(MenuWidget *pWidget);

/**
 * Process championship save text confirmation or cancellation.
 *
 * @ghidraAddress BRD3D: 0x10047210
 * @ghidraAddress BRGlide: 0x10040660
 */
int SaveSeasonUI_HandleTextEntry(MenuWidget *pWidget);

/**
 * Process time attack save text confirmation or cancellation.
 *
 * @ghidraAddress BRD3D: 0x10047250
 * @ghidraAddress BRGlide: 0x100406a0
 */
int SaveTimeAttackMenuUI_HandleTextEntry(MenuWidget *pWidget);

/**
 * Exit an active network game launched through a lobby.
 *
 * @ghidraAddress BRD3D: 0x100437d0
 * @ghidraAddress BRGlide: 0x1003cd20
 */
int MultiplayerGameSetupUI_OnEscape(MenuWidget *pWidget);

/**
 * Reset championship choices and open pre-race information for a new season.
 *
 * @ghidraAddress BRD3D: 0x10045aa0
 * @ghidraAddress BRGlide: 0x1003ef40
 */
int ChampionshipSeasonUI_OnStartNewSeason(MenuWidget *pWidget);

/**
 * Destroy the active network setup screen and restore the cached race setup.
 *
 * @ghidraAddress BRD3D: 0x10046400
 * @ghidraAddress BRGlide: 0x1003f8a0
 */
int ReturnToMultiplayerLobby(MenuWidget *pWidget);

/**
 * Destroy display mode panels and restore the cached input configuration screen.
 *
 * @ghidraAddress BRD3D: 0x10046670
 * @ghidraAddress BRGlide: 0x1003fac0
 */
int ReturnToVideoOptionsFromResolution(MenuWidget *pWidget);

/**
 * Destroy device selection panels and restore the cached input configuration screen.
 *
 * @ghidraAddress BRD3D: 0x10046620
 */
int ReturnToVideoOptionsFromDeviceSelect(MenuWidget *pWidget);

/**
 * Select the modem provider and restore the cached join or create screen.
 *
 * @ghidraAddress BRD3D: 0x10043f50
 * @ghidraAddress BRGlide: 0x1003d4a0
 */
int ModemConnectionUI_OnAccept(MenuWidget *pWidget);

/**
 * Start naming a new championship slot or activate confirmation for an existing file.
 *
 * @ghidraAddress BRD3D: 0x10042020
 * @ghidraAddress BRGlide: 0x1003b580
 */
int SaveSeasonUI_OnSlotSelected(MenuItemList *pList, int *pnSlot);

/**
 * @ghidraAddress BRD3D: 0x1003eb10
 * @ghidraAddress BRGlide: 0x10038100
 */
int SaveSeasonUI_UpdateSlotSelection(MenuWidget *pWidget);

/** @ghidraAddress BRD3D: 0x1003ec30 */
int SaveTimeAttackUI_UpdateSlotSelection(MenuWidget *pWidget);

/** @ghidraAddress BRD3D: 0x1003c9b0 */
void RefreshSessionListSlots(void);

/** @ghidraAddress BRD3D: 0x1003d030 */
int GetSelectedSessionGuid(GUID *guid);

/**
 * @ghidraAddress BRD3D: 0x10041df0
 * @ghidraAddress BRGlide: 0x1003b350
 */
int SaveSeasonUI_OnSlotNameEntered(MenuItemList *pList, int nStatus);

/**
 * @ghidraAddress BRD3D: 0x1003eb90
 * @ghidraAddress BRGlide: 0x10038180
 */
int JoinCreateGameUI_UpdateSessionSelection(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003ec80
 * @ghidraAddress BRGlide: 0x100382a0
 */
int KeyConfigUI_UpdateActionSelection(MenuWidget *pWidget);

#if TGR_RENDERER == TGR_RENDERER_D3D
/**
 * @ghidraAddress BRD3D: 0x1003ed10
 */
int D3DDeviceSelectUI_UpdateDeviceSelection(MenuWidget *pWidget);
#endif

/**
 * @ghidraAddress BRD3D: 0x1003edf0
 * @ghidraAddress BRGlide: 0x10038320
 */
int DisplayModeSelectUI_UpdateModeSelection(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10aa2a30
 * @ghidraAddress BRGlide: 0x10ac5d88
 */
extern int g_dwSelectedDisplayModeIndex;

/**
 * Open cached input options, building the screen on its first visit.
 *
 * @ghidraAddress BRD3D: 0x100452c0
 * @ghidraAddress BRGlide: 0x1003e810
 */
int ShowInputOptionsMenu(void);

/**
 * Select saved-round browsing for season previews.
 *
 * @ghidraAddress BRD3D: 0x1003e8b0
 * @ghidraAddress BRGlide: 0x10037ed0
 */
int SeasonRoundList_SetBrowsingFlag(MenuWidget *pWidget = nullptr);

/**
 * Select the current round for season previews.
 *
 * @ghidraAddress BRD3D: 0x1003e8c0
 * @ghidraAddress BRGlide: 0x10037ee0
 */
int SeasonRoundList_ClearBrowsingFlag(MenuWidget *pWidget = nullptr);

/**
 * Enable game creation after text has been entered.
 *
 * @ghidraAddress BRD3D: 0x1003f280
 * @ghidraAddress BRGlide: 0x100387c0
 */
int CreateGameUI_EnableContinueIfNameEntered(MenuWidget *pWidget);

/** @ghidraAddress BRD3D: 0x1003f210 */
int OnSessionNameEdited(MenuWidget *pWidget);

/**
 * Enable continuing after a player name has been entered.
 *
 * @ghidraAddress BRD3D: 0x1003ef60
 * @ghidraAddress BRGlide: 0x10038490
 */
int PlayerNameEntryUI_EnableContinueIfNameEntered(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003eef0
 */
int OnPlayerNameEdited(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003ef90
 */
int OnModemPhoneNumberEdited(MenuWidget *pWidget);

/**
 * Enable continuing after a modem number has been entered.
 *
 * @ghidraAddress BRD3D: 0x1003f020
 * @ghidraAddress BRGlide: 0x10038550
 */
int ModemConnectionUI_EnableContinueIfNumberEntered(MenuWidget *pWidget);

/**
 * Position the steering indicator for the selected handling choice.
 *
 * @ghidraAddress BRD3D: 0x1003e920
 * @ghidraAddress BRGlide: 0x10037f40
 */
int CarSetupUI_UpdateSteeringIndicatorPos(MenuWidget *pWidget);

/**
 * Update the volume-channel image and its first animation frame.
 *
 * @ghidraAddress BRD3D: 0x1003e950
 * @ghidraAddress BRGlide: 0x10037f70
 */
int AudioOptionsUI_UpdateVolumeChannelImage(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003ea40
 * @ghidraAddress BRGlide: 0x10038060
 */
int AudioOptionsUI_UpdateVolumeLevelIndicatorPos(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003eae0
 * @ghidraAddress BRGlide: 0x10038220
 */
int LoadFileListUI_UpdateSlotSelection(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003eb60
 * @ghidraAddress BRGlide: 0x10038150
 */
int SeasonRoundList_UpdateRoundSelection(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003ea90
 * @ghidraAddress BRGlide: 0x100380b0
 */
int SeasonUI_UpdateRoundPointsLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003e8d0
 * @ghidraAddress BRGlide: 0x10037ef0
 */
int SeasonUI_UpdateRoundPlaceLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003ee20
 * @ghidraAddress BRGlide: 0x10038350
 */
int AudioOptionsUI_UpdateCdTrackSelection(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003ebc0
 * @ghidraAddress BRGlide: 0x100381b0
 */
int NetworkCarSetupUI_PollChatListSelection(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003f110
 * @ghidraAddress BRGlide: 0x10038650
 */
int OnChatMessageEdited(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003f170
 * @ghidraAddress BRGlide: 0x100386b0
 */
int NetworkCarSetupUI_SendChatMessage(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003f0b0
 * @ghidraAddress BRGlide: 0x100385f0
 */
int OnTcpIpAddressEdited(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003f050
 * @ghidraAddress BRGlide: 0x10038580
 */
int OnTcpIpPortEdited(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10aa29bc
 * @ghidraAddress BRGlide: 0x10ac5d14
 */
extern MenuWidget *g_pCreateGameContinueItem;

/**
 * Build input options on the supplied screen.
 *
 * @ghidraAddress BRD3D: 0x1004c4a0
 * @ghidraAddress BRGlide: 0x100458d0
 * @stub{BRGlide}
 */
void BuildInputOptionsUI(MenuScreen *pScreen);

/**
 * Open cached video options, building the screen on its first visit.
 *
 * @ghidraAddress BRD3D: 0x10045390
 * @ghidraAddress BRGlide: 0x1003e8e0
 */
int ShowVideoOptionsMenu(void);

/**
 * @ghidraAddress BRD3D: 0x100455e0
 */
int ShowAudioOptionsMenu(void);

/**
 * @ghidraAddress BRD3D: 0x100456b0
 */
int ShowGameOptionsMenu(void);

/**
 * @ghidraAddress BRD3D: 0x1004dfc0
 */
int BuildAudioOptionsUI(MenuScreen *pScreen);

/** @ghidraAddress BRD3D: 0x1003e980 */
int DrawGameplaySliderA(MenuWidget *pWidget);
/** @ghidraAddress BRD3D: 0x1003e9e0 */
int DrawGameplaySliderB(MenuWidget *pWidget);
/** @ghidraAddress BRD3D: 0x100ab548 */
extern RECT g_rcMenuAudioListBox;
/** @ghidraAddress BRD3D: 0x100b89c8 */
extern const char *g_apszBgmTrackNames[12];

/**
 * @ghidraAddress BRD3D: 0x1004e830
 */
int BuildGameOptionsUI(MenuScreen *pScreen);

/**
 * Build video options on the supplied screen.
 *
 * @ghidraAddress BRD3D: 0x1004d1f0
 * @ghidraAddress BRGlide: 0x10046620
 * @stub{BRGlide}
 */
void BuildVideoOptionsUI(MenuScreen *pScreen);

/**
 * Open cached key configuration, building the screen on its first visit.
 *
 * @ghidraAddress BRD3D: 0x100434c0
 * @ghidraAddress BRGlide: 0x1003ca10
 */
int ShowKeyConfigScreen(void);

/**
 * Build key configuration on the supplied screen.
 *
 * @ghidraAddress BRD3D: 0x1004cac0
 * @ghidraAddress BRGlide: 0x10045ef0
 * @stub
 */
void BuildKeyConfigUI(MenuScreen *pScreen);

/**
 * Open cached modem connection options, building the screen on its first visit.
 *
 * @ghidraAddress BRD3D: 0x10043da0
 * @ghidraAddress BRGlide: 0x1003d2f0
 */
int ShowModemDialUpScreen(void);

/**
 * Reset time attack choices and restore race setup after a season.
 *
 * @ghidraAddress BRD3D: 0x10046260
 * @ghidraAddress BRGlide: 0x1003f700
 */
int ReturnToMainMenuFromSeason(void);

/**
 * Build modem connection options on the supplied screen.
 *
 * @ghidraAddress BRD3D: 0x10056a10
 * @ghidraAddress BRGlide: 0x1004f8c0
 * @stub{BRGlide}
 */
int BuildModemConnectionUI(MenuScreen *pScreen);

/**
 * Open display mode selection and populate its settings lists after successful creation.
 *
 * @ghidraAddress BRD3D: 0x10045520
 */
int ShowDisplayModeSelectMenu(void);

/**
 * Build display mode selection on the supplied screen.
 *
 * @ghidraAddress BRD3D: 0x1004db00
 */
void BuildDisplayModeSelectUI(MenuScreen *pScreen);

/** @ghidraAddress BRD3D: 0x1003ecb0 */
int D3DDeviceSelectUI_OnSetDriver(MenuWidget *pWidget);
/** @ghidraAddress BRD3D: 0x1003ed40 */
int DisplayModeSelectUI_OnSetMode(MenuWidget *pWidget);

/**
 * Populate the video settings lists.
 *
 * @ghidraAddress BRD3D: 0x1007ac00
 */
int PopulateVideoSettingsLists(void);

/**
 * Populate available display devices.
 *
 * @ghidraAddress BRD3D: 0x1007a840
 */
int PopulateDisplayDeviceList(void);

/** @ghidraAddress BRD3D: 0x1007a710 */
int CompareDisplayModes(const void *left, const void *right);

/** @ghidraAddress BRD3D: 0x100ab528 */
extern RECT g_rcMenuDeviceListBox;

/**
 * Populate available display modes for the selected device.
 *
 * @ghidraAddress BRD3D: 0x1007a940
 */
int PopulateDisplayModeList(void);

/**
 * Open Direct3D device selection and populate its settings lists.
 *
 * @ghidraAddress BRD3D: 0x10045460
 * @ghidraAddress BRGlide: 0x1003e9b0
 * @stub{BRGlide}
 */
int ShowD3DDeviceSelectMenu(void);

/**
 * Build Direct3D device selection on the supplied screen.
 *
 * @ghidraAddress BRD3D: 0x1004d640
 */
void BuildD3DDeviceSelectUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10a9d078
 * @ghidraAddress BRGlide: 0x10ac4100
 */
extern char g_szMenuItemTextBackup[];

/**
 * @ghidraAddress BRD3D: 0x10aa28ec
 * @ghidraAddress BRGlide: 0x10ac5c44
 */
extern int g_dwTimeAttackSaveSlotNamed;

/**
 * @ghidraAddress BRD3D: 0x100408d0
 * @ghidraAddress BRGlide: 0x10039e10
 */
int RaceSetupUI_UpdateLapCountLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10040a50
 * @ghidraAddress BRGlide: 0x10039f90
 */
int SeasonFileUI_UpdateYearLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10040ac0
 * @ghidraAddress BRGlide: 0x1003a000
 */
int SeasonFileUI_UpdateRoundLabel(MenuWidget *pWidget);

/**
 * Draw the season progress round with its label and two separating spaces.
 *
 * @ghidraAddress BRD3D: 0x10040b30
 * @ghidraAddress BRGlide: 0x1003a070
 */
int SeasonProgressInfoUI_UpdateRoundLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10041300
 * @ghidraAddress BRGlide: 0x1003a860
 */
int SeasonUI_UpdateSeasonNameLabel(MenuWidget *pWidget);

/**
 * Draw the year with the translated suffix for its ordinal.
 *
 * @ghidraAddress BRD3D: 0x100413b0
 * @ghidraAddress BRGlide: 0x1003a910
 */
int SeasonUI_UpdateYearOrdinalLabel(MenuWidget *pWidget);

/**
 * Enable continuing after a championship slot has been opened.
 *
 * @ghidraAddress BRD3D: 0x10041890
 * @ghidraAddress BRGlide: 0x1003adf0
 */
int LoadSeasonUI_UpdateContinueButtonState(MenuWidget *pWidget);

/**
 * Enable a time attack item after a saved slot has been selected.
 *
 * @ghidraAddress BRD3D: 0x100418f0
 * @ghidraAddress BRGlide: 0x1003ae50
 */
int LoadTimeAttackMenuUI_UpdateItemEnabledState(MenuWidget *pWidget);

/**
 * Draw the sum of the selected season's four race scores, or zero before loading.
 *
 * @ghidraAddress BRD3D: 0x100414b0
 * @ghidraAddress BRGlide: 0x1003aa10
 */
int SeasonUI_UpdateTotalPointsLabel(MenuWidget *pWidget);

/**
 * Draw the total points of the race results.
 *
 * @ghidraAddress BRD3D: 0x10041710
 * @ghidraAddress BRGlide: 0x1003ac70
 */
int RaceResultsTotalsUI_UpdateTotalPointsLabel(MenuWidget *pWidget);

/**
 * Draw the number of completed rounds in the race results.
 *
 * @ghidraAddress BRD3D: 0x10041670
 * @ghidraAddress BRGlide: 0x1003abd0
 */
int RaceResultsTotalsUI_UpdateRoundsCompletedLabel(MenuWidget *pWidget);

/**
 * Draw the remaining points for the season stored in the resume state.
 *
 * @ghidraAddress BRD3D: 0x100417b0
 * @ghidraAddress BRGlide: 0x1003ad10
 */
int RaceResultsTotalsUI_UpdatePointsNeededLabel(MenuWidget *pWidget);

/**
 * Draw the remaining points required to advance, with negative results clamped to zero.
 *
 * @ghidraAddress BRD3D: 0x100415a0
 * @ghidraAddress BRGlide: 0x1003ab00
 */
int SeasonProgressInfoUI_UpdatePointsNeededLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10041970
 * @ghidraAddress BRGlide: 0x1003aed0
 */
int MainMenuUI_OnPlayCredits(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10046f60
 * @ghidraAddress BRGlide: 0x100403b0
 */
int PreRaceExitSeasonConfirmUI_OnYes(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10046fd0
 * @ghidraAddress BRGlide: 0x10040420
 */
int ResultsSummaryExitSeasonConfirmUI_OnYes(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100406c0
 * @ghidraAddress BRGlide: 0x10039c00
 */
int InputOptionsUI_OnRestoreDefaults(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10043400
 * @ghidraAddress BRGlide: 0x1003c950
 */
int InputOptionsUI_OnCycleInputDevice(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003ee50
 * @ghidraAddress BRGlide: 0x10038380
 */
int ProcessMenuTextEditInput(MenuWidget *pWidget, short nFont);

/** The number of controls displayed by key configuration. */
#define KEY_CONFIG_ENTRY_COUNT 21

struct KeyConfigEntry {
    unsigned nStringId;
    int nControl;
};

/**
 * @ghidraAddress BRD3D: 0x100ab330
 * @ghidraAddress BRGlide: 0x100aaad0
 */
extern KeyConfigEntry g_aKeyConfigEntries[KEY_CONFIG_ENTRY_COUNT];

/**
 * @ghidraAddress BRD3D: 0x10aa2840
 * @ghidraAddress BRGlide: 0x10ac5b98
 */
extern int g_dwKeyConfigSelectedAction;

/**
 * @ghidraAddress BRD3D: 0x100400e0
 * @ghidraAddress BRGlide: 0x10039620
 */
int KeyConfigUI_UpdateBindingLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac6a0
 * @ghidraAddress BRGlide: 0x100abe40
 */
extern int g_dwCapturedKeyBindingValue;

/**
 * @ghidraAddress BRD3D: 0x10aa2a38
 * @ghidraAddress BRGlide: 0x10ac5d90
 */
extern int g_dwKeyCaptureWaitRelease;

/**
 * @ghidraAddress BRD3D: 0x10aa2a3c
 * @ghidraAddress BRGlide: 0x10ac5d94
 */
extern int g_dwKeyCaptureComplete;

/**
 * @ghidraAddress BRD3D: 0x10aa2850
 * @ghidraAddress BRGlide: 0x10ac5ba8
 */
extern int g_dwKeyBindingDuplicatesFound;

/**
 * @ghidraAddress BRD3D: 0x1003f8d0
 * @ghidraAddress BRGlide: 0x10038e10
 */
int KeyConfigUI_UpdateDuplicateBindingWarning(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10a9d5c0
 * @ghidraAddress BRGlide: 0x10ac4648
 */
extern int g_adwKeyBindingDuplicateFlags[KEY_CONFIG_ENTRY_COUNT];

/**
 * @ghidraAddress BRD3D: 0x10040330
 * @ghidraAddress BRGlide: 0x10039870
 */
int DetectDuplicateKeyBindings(int nDevice);

/**
 * @ghidraAddress BRD3D: 0x10040450
 * @ghidraAddress BRGlide: 0x10039990
 */
int KeyConfigUI_PollKeyCapture(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10045110
 * @ghidraAddress BRGlide: 0x1003e660
 */
int ShowCarSetupScreen(void);

/**
 * @ghidraAddress BRD3D: 0x1004a580
 * @ghidraAddress BRGlide: 0x100439b0
 * @stub
 */
void BuildCarSetupUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10045050
 * @ghidraAddress BRGlide: 0x1003e5a0
 */
int SeasonProgressInfoUI_OnCarSetup(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10046380
 * @ghidraAddress BRGlide: 0x1003f820
 */
int LoadTimeAttackMenuUI_OnCarSetup(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac304
 * @ghidraAddress BRGlide: 0x100abaa4
 */
extern int g_bRaceSetupShowCarSetup;

/**
 * @ghidraAddress BRD3D: 0x100437b0
 * @ghidraAddress BRGlide: 0x1003cd00
 */
int NetworkCarSetupUI_OnBootPlayer(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100450c0
 * @ghidraAddress BRGlide: 0x1003e610
 */
int LoadSeasonUI_OnLoadAutoSave(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100471b0
 * @ghidraAddress BRGlide: 0x10040600
 */
int OnListDialogItemSelected(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10047120
 * @ghidraAddress BRGlide: 0x10040570
 */
int OnPressAnyKeyContinue(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100471f0
 * @ghidraAddress BRGlide: 0x10040640
 */
int SeasonCongratsUI_PollAnyKey(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003e840
 * @ghidraAddress BRGlide: 0x10037e60
 */
int PreRaceInfoUI_UpdateBackButtonText(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003f440
 * @ghidraAddress BRGlide: 0x10038980
 */
int SeasonCongratsUI_UpdatePrimaryBannerImage(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003f540
 * @ghidraAddress BRGlide: 0x10038a80
 */
int SeasonCongratsUI_UpdateSecondaryBannerImage(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003f5e0
 * @ghidraAddress BRGlide: 0x10038b20
 */
int MultiplayerSetupUI_UpdateCarGroupImage(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003f680
 * @ghidraAddress BRGlide: 0x10038bc0
 */
int MultiplayerSetupUI_UpdateCarGroupSubImage(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10040680
 * @ghidraAddress BRGlide: 0x10039bc0
 */
int KeyConfigUI_BeginKeyCapture(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10047360
 * @ghidraAddress BRGlide: 0x100407b0
 */
int ValidateEntityState(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100474b0
 * @ghidraAddress BRGlide: 0x10040af0
 */
int ValidateEntityStateAlwaysTrue(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100475f0
 * @ghidraAddress BRGlide: 0x10040a20
 */
int ProcessCheatInputAndValidateEntity(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10044010
 */
int MultiplayerConnectionUI_OnSelectIpx(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100558a0
 */
void BuildMultiplayerConnectionUI(MenuScreen *pScreen);

/**
 * Open cached connection settings, building the screen on its first visit.
 *
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x10043cd0
 */
int ShowMultiplayerConnectionScreen(MenuWidget *pWidget = nullptr);

/**
 * @ghidraAddress BRD3D: 0x10044030
 */
int MultiplayerConnectionUI_UpdateIpxItem(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10044050
 */
int MultiplayerConnectionUI_OnSelectTcpIp(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10044070
 */
int MultiplayerConnectionUI_UpdateTcpIpItem(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10044090
 */
int MultiplayerConnectionUI_OnSelectModem(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100440b0
 */
int MultiplayerConnectionUI_UpdateModemItem(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10042a90
 * @ghidraAddress BRGlide: 0x1003bff0
 */
int PlayerNameEntryUI_OnToggleTextEdit(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10042ac0
 * @ghidraAddress BRGlide: 0x1003c020
 */
int ConnectionUI_OnToggleTextEdit(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10042b00
 * @ghidraAddress BRGlide: 0x1003c050
 */
int CreateGameMenuUI_OnToggleTextEdit(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100409f0
 * @ghidraAddress BRGlide: 0x10039f30
 */
int RaceResultsPanel_PreUpdateSelectLastRace(void);

/**
 * @ghidraAddress BRD3D: 0x10040a20
 * @ghidraAddress BRGlide: 0x10039f60
 */
int RaceResultsPanel_PostUpdateRestoreSeasonProgress(void);

/**
 * @ghidraAddress BRD3D: 0x10047340
 * @ghidraAddress BRGlide: 0x10040790
 */
int SeasonProgressInfoUI_OnResetRound(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100450f0
 * @ghidraAddress BRGlide: 0x1003e640
 */
int SaveSeasonUI_InvokeBackHandler(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1004e810
 * @ghidraAddress BRGlide: 0x100476c0
 */
int AudioOptionsUI_OnMusicTrackChanged(MenuItemList *pList, int *pTrack);

/**
 * @ghidraAddress BRD3D: 0x10043710
 * @ghidraAddress BRGlide: 0x1003cc60
 */
int QuitGameConfirmUI_OnConfirmQuit(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10040730
 * @ghidraAddress BRGlide: 0x10039c70
 */
int RaceSetupUI_UpdateTrackImage(MenuWidget *pWidget);

/**
 * Update the selected time attack track image after a saved slot has been selected.
 *
 * @ghidraAddress BRD3D: 0x100407a0
 * @ghidraAddress BRGlide: 0x10039ce0
 */
int TimeAttackUI_UpdateTrackImage(MenuWidget *pWidget);

/** @ghidraAddress BRD3D: 0x10041040 */
int TimeAttackUI_UpdateRaceTimeLabel(MenuWidget *pWidget);

/** @ghidraAddress BRD3D: 0x10041180 */
int TimeAttackUI_UpdateBestLapLabel(MenuWidget *pWidget);

/**
 * Draw the force feedback choice, displaying off when the device lacks support.
 *
 * @ghidraAddress BRD3D: 0x10040950
 * @ghidraAddress BRGlide: 0x10039e90
 */
int GameOptionsUI_UpdateForceFeedbackImage(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac570
 * @ghidraAddress BRGlide: 0x100abd10
 */
extern const short g_awTimeAttackTrackImageIds[];

/**
 * @ghidraAddress BRD3D: 0x100ac630
 * @ghidraAddress BRGlide: 0x100abdd0
 */
extern const signed char g_abForceFeedbackImageIds[2];

/**
 * @ghidraAddress BRD3D: 0x100407e0
 * @ghidraAddress BRGlide: 0x10039d20
 */
int RaceSetupUI_UpdateWeatherImage(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac550
 * @ghidraAddress BRGlide: 0x100abcf0
 */
extern const short g_awTrackImageIds[];

/**
 * @ghidraAddress BRD3D: 0x100ac590
 * @ghidraAddress BRGlide: 0x100abd30
 */
extern const signed char g_abWeatherImageIds[];

/**
 * @ghidraAddress BRD3D: 0x10aa28a8
 * @ghidraAddress BRGlide: 0x10ac5c00
 */
extern unsigned char g_bSeasonRoundListBrowsing;

/**
 * @ghidraAddress BRD3D: 0x10aa28e8
 * @ghidraAddress BRGlide: 0x10ac5c40
 */
extern int g_dwTimeAttackLoadSelected;

/**
 * @ghidraAddress BRD3D: 0x10042c80
 * @ghidraAddress BRGlide: 0x1003c1d0
 */
int CarSetupUI_OnCycleHandlingType(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10042dc0
 * @ghidraAddress BRGlide: 0x1003c310
 */
int CarSetupUI_OnCycleTireType(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10042e20
 * @ghidraAddress BRGlide: 0x1003c370
 */
int CarSetupUI_OnCycleSuspensionType(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10042e80
 * @ghidraAddress BRGlide: 0x1003c3d0
 */
int CarSetupUI_OnCycleTransmissionType(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10043590
 * @ghidraAddress BRGlide: 0x1003cae0
 */
int GameOptionsUI_OnToggleForceFeedback(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100435f0
 * @ghidraAddress BRGlide: 0x1003cb40
 */
int GameOptionsUI_OnToggleSkidMarks(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10043650
 * @ghidraAddress BRGlide: 0x1003cba0
 */
int GameOptionsUI_OnToggleCarShadow(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100436b0
 * @ghidraAddress BRGlide: 0x1003cc00
 */
int GameOptionsUI_OnToggleSpecularLighting(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac530
 * @ghidraAddress BRGlide: 0x100abcd0
 */
extern const int g_adwForceFeedbackMenuIndexToValue[2];

/**
 * @ghidraAddress BRD3D: 0x100ac548
 * @ghidraAddress BRGlide: 0x100abce8
 */
extern const int g_adwSkidMarksMenuIndexToValue[2];

/**
 * @ghidraAddress BRD3D: 0x100ac538
 * @ghidraAddress BRGlide: 0x100abcd8
 */
extern const int g_adwCarShadowMenuIndexToValue[2];

/**
 * @ghidraAddress BRD3D: 0x100ac540
 * @ghidraAddress BRGlide: 0x100abce0
 */
extern const int g_adwSpecularLightingMenuIndexToValue[2];

/**
 * @ghidraAddress BRD3D: 0x100412c0
 * @ghidraAddress BRGlide: 0x1003a820
 */
int RaceResultsInfoUI_OnSelectLap1(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100412d0
 * @ghidraAddress BRGlide: 0x1003a830
 */
int RaceResultsInfoUI_OnSelectLap2(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100412e0
 * @ghidraAddress BRGlide: 0x1003a840
 */
int RaceResultsInfoUI_OnSelectLap3(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100412f0
 * @ghidraAddress BRGlide: 0x1003a850
 */
int RaceResultsInfoUI_OnSelectTotalTime(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100418d0
 * @ghidraAddress BRGlide: 0x1003ae30
 */
int SaveSeasonUI_UpdateItemEnabledState(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10046fc0
 * @ghidraAddress BRGlide: 0x10040410
 */
int PreRaceExitSeasonConfirmUI_OnNo(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10047050
 * @ghidraAddress BRGlide: 0x100404a0
 */
int ResultsSummaryExitSeasonConfirmUI_OnNo(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10043fa0
 * @ghidraAddress BRGlide: 0x1003d4f0
 */
int MenuScreen_OnReturnToMainMenu(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10045780
 * @ghidraAddress BRGlide: 0x1003ec10
 */
int PreRaceInfoUI_OnOptions(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100457a0
 * @ghidraAddress BRGlide: 0x1003ec30
 */
int PreRaceInfoUI_OnSaveSeason(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100457c0
 * @ghidraAddress BRGlide: 0x1003ec50
 */
int SeasonProgressInfoUI_OnOptions(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100457e0
 * @ghidraAddress BRGlide: 0x1003ec70
 */
int SeasonProgressInfoUI_OnSaveSeason(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10042cf0
 * @ghidraAddress BRGlide: 0x1003c240
 */
int AudioOptionsUI_OnCycleSfxVolume(MenuActionWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10042d60
 * @ghidraAddress BRGlide: 0x1003c2b0
 */
int AudioOptionsUI_OnCycleMusicVolume(MenuActionWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10040870
 * @ghidraAddress BRGlide: 0x10039db0
 */
int CarSetupUI_UpdateTransmissionImage(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac598
 * @ghidraAddress BRGlide: 0x100abd38
 */
extern const signed char g_abTransmissionImageIds[2];

/**
 * @ghidraAddress BRD3D: 0x10040890
 * @ghidraAddress BRGlide: 0x10039dd0
 */
int CarSetupUI_UpdateTiresImage(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac59c
 * @ghidraAddress BRGlide: 0x100abd3c
 */
extern const signed char g_abTireImageIds[3];

/**
 * @ghidraAddress BRD3D: 0x100408b0
 * @ghidraAddress BRGlide: 0x10039df0
 */
int CarSetupUI_UpdateShocksImage(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac5a0
 * @ghidraAddress BRGlide: 0x100abd40
 */
extern const signed char g_abSuspensionImageIds[3];

/**
 * @ghidraAddress BRD3D: 0x10041870
 * @ghidraAddress BRGlide: 0x1003add0
 */
int InputOptionsUI_UpdateInputDeviceImage(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac628
 * @ghidraAddress BRGlide: 0x100abdc8
 */
extern const signed char g_abInputDeviceImageIds[4];

/**
 * @ghidraAddress BRD3D: 0x10040930
 * @ghidraAddress BRGlide: 0x10039e70
 */
int ConnectionUI_UpdateServiceProviderImage(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac62c
 * @ghidraAddress BRGlide: 0x100abdcc
 */
extern const signed char g_abServiceProviderImageIds[4];

/**
 * @ghidraAddress BRD3D: 0x100409b0
 * @ghidraAddress BRGlide: 0x10039ef0
 */
int GameOptionsUI_UpdateCarShadowImage(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac634
 * @ghidraAddress BRGlide: 0x100abdd4
 */
extern const signed char g_abCarShadowImageIds[2];

/**
 * @ghidraAddress BRD3D: 0x100409d0
 * @ghidraAddress BRGlide: 0x10039f10
 */
int GameOptionsUI_UpdateSpecularLightingImage(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac638
 * @ghidraAddress BRGlide: 0x100abdd8
 */
extern const signed char g_abSpecularLightingImageIds[2];

/**
 * @ghidraAddress BRD3D: 0x10040990
 * @ghidraAddress BRGlide: 0x10039ed0
 */
int GameOptionsUI_UpdateSkidMarksImage(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac640
 * @ghidraAddress BRGlide: 0x100abde0
 */
extern const int g_awSkidMarksImageIds[2];

/**
 * @ghidraAddress BRD3D: 0x10aa28d8
 * @ghidraAddress BRGlide: 0x10ac5c30
 */
extern int g_dwMenuTextEditActive;

/** The index in apWidgets of the mouse cursor of the first panel of the root screen. */
#define MENU_CURSOR_WIDGET_INDEX (MENU_PANEL_WIDGET_COUNT - 1)

/** A panel of a screen of the front end. */
class MenuPanel {
    int (*pfnBeforeWidgets)(void);
    int (*pfnAfterWidgets)(void);
    int (*pfnUpdate)(void);
    int nFlags;
    unsigned short wWidgetCount;
    char abUnknown16[2];                            /* +0x16 */
    MenuWidget *apWidgets[MENU_PANEL_WIDGET_COUNT]; /*!< The items of the panel. */
    float fX;
    float fY;
    MenuScreen *pScreen;
    unsigned short wNavigationCount;
    unsigned short wSelectedIndex;

public:
    /**
     * Clear widget pointers and panel selections.
     *
     * @ghidraAddress BRD3D: 0x10048470
     */
    MenuPanel();

    /**
     * @ghidraAddress BRD3D: 0x100484f0
     */
    int RestoreSelection();

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

    void SetBeforeWidgetsCallback(int (*callback)(void)) {
        pfnBeforeWidgets = callback;
    }
    void SetAfterWidgetsCallback(int (*callback)(void)) {
        pfnAfterWidgets = callback;
    }
    int SetFlags(int value) {
        return nFlags = value;
    }
    unsigned short GetWidgetCount() const {
        return wWidgetCount;
    }
    unsigned short SetWidgetCount(unsigned short value) {
        return wWidgetCount = value;
    }
    MenuWidget *GetWidget(int index) const {
        return apWidgets[index];
    }
    MenuWidget *SetWidget(int index, MenuWidget *widget) {
        return apWidgets[index] = widget;
    }
    float GetX() const {
        return fX;
    }
    float SetX(float value) {
        return fX = value;
    }
    float GetY() const {
        return fY;
    }
    float SetY(float value) {
        return fY = value;
    }
    MenuScreen *SetScreen(MenuScreen *screen) {
        return pScreen = screen;
    }
    unsigned short GetNavigationCount() const {
        return wNavigationCount;
    }
    unsigned short SetNavigationCount(unsigned short value) {
        return wNavigationCount = value;
    }
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

private:
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

public:
    /**
     * Run a frame of the screen, and shut the front end down once the screen stops running.
     *
     * @return Whether the screen still runs.
     * @ghidraAddress BRD3D: 0x100489a0
     * @ghidraAddress BRGlide: 0x10041dd0
     */
    virtual bool RunFrame();
    /**
     * Make the screen the one that shows. The choices of the menus are restored, the screen is
     * built the first time with the menu images loaded, and the screen the game returns to after a
     * race, a time attack, or network play is entered.
     *
     * @return False when the progress of the season is not restored, and true otherwise.
     * @ghidraAddress BRD3D: 0x1005ae70
     * @ghidraAddress BRGlide: 0x10053d20
     */
    virtual bool ActivateMenuScreen();
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

    auto GetBuildCallback() const {
        return pfnBuildMenu;
    }
    void SetBuildCallback(void (*callback)(MenuScreen *)) {
        pfnBuildMenu = callback;
    }
    bool IsBuilt() const {
        return bBuilt != 0;
    }
    void SetBuilt(bool value) {
        bBuilt = value;
    }
    unsigned short GetPanelCount() const {
        return wPanelCount;
    }
    unsigned short SetPanelCount(unsigned short value) {
        return wPanelCount = value;
    }
    unsigned short SetCurrentPanelIndex(unsigned short value) {
        return wCurrentPanel = value;
    }
    MenuPanel *GetPanel(int index) const {
        return apPanels[index];
    }
    MenuPanel *SetPanel(int index, MenuPanel *panel) {
        return apPanels[index] = panel;
    }
    MenuPanel *GetCurrentPanel() const {
        return pCurrentPanel;
    }
    bool IsRunning() const {
        return bRunning != 0;
    }
    void SetRunning(bool value) {
        bRunning = value;
    }
    int GetPanelActive(int index) const {
        return anPanelActive[index];
    }
    int SetPanelActive(int index, int value) {
        return anPanelActive[index] = value;
    }
    SaveFileList *GetFileList(int index) const {
        return apFiles[index];
    }

private:
    void (*pfnBuildMenu)(MenuScreen *pScreen); /*!< Builds the items of the screen. */
    int dwFlags;                  /*!< Options of the screen. Bit 0x10 stops EndFrame(). */
    int bBuilt;                   /*!< Whether pfnBuildMenu built the items of the screen. */
    unsigned short wPanelCount;   /*!< The number of entries of apPanels in use. */
    unsigned short wCurrentPanel; /*!< The index of the panel that runs. */
    MenuPanel *apPanels[MENU_SCREEN_PANEL_COUNT]; /*!< The panels of the screen. */
    MenuPanel *pCurrentPanel;                     /*!< The panel that runs. */
    int bRunning; /*!< Whether the screen runs and reads window messages. */
    /** Whether each panel of apPanels runs its items. */
    int anPanelActive[MENU_SCREEN_PANEL_COUNT];
    unsigned short wUnknownBC; /*!< Cleared at the start. Its meaning is not recovered yet. */
    [[maybe_unused]] char abUnknownBE[2]; /* +0xbe */
    SaveFileList *apFiles[2];             /*!< The two lists of saved files of the screen. */
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
extern NetworkMenuWidget *g_pJoinCreateGameMenu;

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
    IDirectDrawSurface *pSurface; /*!< The surface the image is loaded into. */
    DDSURFACEDESC desc;           /*!< The description of pSurface. */
#else
    char abUnknown0[4]; /* +0x00 */
#endif
    char *pszPath; /*!< The path of the file of the image, from new. */
};

/** A resource of the front end. Its layout is not recovered yet. */
struct FrontEndResourceSlot {
    int anValues[3]; /*!< Values that start at -1 but the third, which starts at 0. */
};

/** The number of entries of g_aFrontEndResourceSlots. */
#define FRONT_END_RESOURCE_SLOT_COUNT 60
/** The number of entries of g_adwFrontEndResourceFlags. */
#define FRONT_END_RESOURCE_FLAG_COUNT 100

/**
 * The screen the front end starts at.
 *
 * @ghidraAddress BRD3D: 0x10aa2908
 * @ghidraAddress BRGlide: 0x10ac5c60
 */
extern MenuScreen *g_pRootMenuScreen;

/**
 * The cached screen of the setup of a network race.
 *
 * @ghidraAddress BRD3D: 0x10aa2950
 * @ghidraAddress BRGlide: 0x10ac5ca8
 */
extern MenuScreen *g_pCachedMenu_RaceSetupNetwork;

/**
 * The cached screen of the settings of the game.
 *
 * @ghidraAddress BRD3D: 0x10aa2984
 * @ghidraAddress BRGlide: 0x10ac5cdc
 */
extern MenuScreen *g_pCachedMenu_GameplaySettings;

/**
 * The cached screen of the championship.
 *
 * @ghidraAddress BRD3D: 0x10aa2988
 * @ghidraAddress BRGlide: 0x10ac5ce0
 */
extern MenuScreen *g_pCachedMenu_Championship;

/**
 * The item that continues from the name of the player.
 *
 * @ghidraAddress BRD3D: 0x10aa29a8
 * @ghidraAddress BRGlide: 0x10ac5d00
 */
extern MenuWidget *g_pPlayerNameContinueItem;

/**
 * The menu of the setup of a network game.
 *
 * @ghidraAddress BRD3D: 0x10aa29b8
 * @ghidraAddress BRGlide: 0x10ac5d10
 */
extern MenuWidget *g_pMultiplayerGameSetupMenu;

/**
 * @ghidraAddress BRD3D: 0x100ab448
 */
extern RECT g_rcMenuCenterPanel;
/**
 * @ghidraAddress BRD3D: 0x100ab508
 */
extern RECT g_rcMenuTitleBar;
/**
 * @ghidraAddress BRD3D: 0x100ab4d8
 */
extern RECT g_rcMenuListBox;
/**
 * @ghidraAddress BRD3D: 0x100ab4b8
 */
extern RECT g_rcMenuLeftColumn;
/**
 * @ghidraAddress BRD3D: 0x100ab478
 */
extern RECT g_rcMenuRightColumnWide;
/**
 * @ghidraAddress BRD3D: 0x100ab3f8
 */
extern RECT g_rcMenuNetworkPlayers;
/**
 * @ghidraAddress BRD3D: 0x100ab488
 */
extern RECT g_rcMenuRightColumn;
/**
 * @ghidraAddress BRD3D: 0x100ab468
 */
extern RECT g_rcMenuRightLabel;
/**
 * @ghidraAddress BRD3D: 0x100ad300
 */
extern const char g_szSpace[];
/**
 * @ghidraAddress BRD3D: 0x10aa2a70
 */
extern char g_szEmptyString[1];

/**
 * The menu to save a time attack.
 *
 * @ghidraAddress BRD3D: 0x10aa29c4
 * @ghidraAddress BRGlide: 0x10ac5d1c
 */
extern MenuWidget *g_pSaveTimeAttackMenu;

/**
 * The field of the name of a saved time attack.
 *
 * @ghidraAddress BRD3D: 0x10aa29d0
 * @ghidraAddress BRGlide: 0x10ac5d28
 */
extern MenuItemList *g_pSaveTimeAttackEditMenu;

/**
 * The menu to join a network session.
 *
 * @ghidraAddress BRD3D: 0x10aa29d8
 * @ghidraAddress BRGlide: 0x10ac5d30
 */
extern NetworkMenuWidget *g_pNetworkSessionJoinMenu;

/**
 * The menu of the setup of the car of network play.
 *
 * @ghidraAddress BRD3D: 0x10aa29e0
 * @ghidraAddress BRGlide: 0x10ac5d38
 */
extern NetworkMenuWidget *g_pNetworkCarSetupMenu;

/**
 * The list of the players of network play.
 *
 * @ghidraAddress BRD3D: 0x10aa29e4
 * @ghidraAddress BRGlide: 0x10ac5d3c
 */
extern MenuWidget *g_pNetworkPlayerListMenu;

/**
 * The item that continues from the modem screen.
 *
 * @ghidraAddress BRD3D: 0x10aa29e8
 * @ghidraAddress BRGlide: 0x10ac5d40
 */
extern MenuWidget *g_pModemContinueItem;

/**
 * The menu that chooses the display mode.
 *
 * @ghidraAddress BRD3D: 0x10aa29ec
 * @ghidraAddress BRGlide: 0x10ac5d44
 */
extern MenuWidget *g_pDisplayModeSelectMenu;

/**
 * The menu that chooses the Direct3D device.
 *
 * @ghidraAddress BRD3D: 0x10aa29f0
 * @ghidraAddress BRGlide: 0x10ac5d48
 */
extern MenuWidget *g_pD3DDeviceSelectMenu;

/**
 * @ghidraAddress BRD3D: 0x10042b30
 */
int CarSetupUI_OnCycleCar(MenuActionWidget *pItem);
/**
 * @ghidraAddress BRD3D: 0x10042ee0
 */
int RaceSetupUI_OnCycleTrack(MenuActionWidget *pItem);
/**
 * @ghidraAddress BRD3D: 0x100430b0
 */
int RaceSetupUI_OnCycleLapCount(MenuActionWidget *pItem);
/**
 * @ghidraAddress BRD3D: 0x10043180
 */
int RaceSetupUI_OnCycleWeather(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x100ac368
 * @ghidraAddress BRGlide: 0x100abb08
 */
extern int g_adwCarNameStringIds[];
/**
 * @ghidraAddress BRD3D: 0x100ac308
 * @ghidraAddress BRGlide: 0x100abaa8
 */
extern int g_adwTrackNameStringIds[];

/**
 * @ghidraAddress BRD3D: 0x1003f760
 * @ghidraAddress BRGlide: 0x10038ca0
 */
int CarSetupUI_UpdateCarNameLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003fa00
 * @ghidraAddress BRGlide: 0x10038f40
 */
int RaceSetupUI_UpdateTrackNameLabel(MenuWidget *pWidget);
/**
 * @ghidraAddress BRD3D: 0x100ac3b0
 * @ghidraAddress BRGlide: 0x100abb50
 */
extern int g_adwWeatherLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x1003fe80
 * @ghidraAddress BRGlide: 0x100393c0
 */
int RaceSetupUI_UpdateWeatherLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003f860
 * @ghidraAddress BRGlide: 0x10038da0
 */
int CarSetupUI_UpdateTransmissionLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac3a8
 * @ghidraAddress BRGlide: 0x100abb48
 */
extern int g_adwTransmissionLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x1003f990
 * @ghidraAddress BRGlide: 0x10038ed0
 */
int CarSetupUI_UpdateShocksLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac358
 * @ghidraAddress BRGlide: 0x100abaf8
 */
extern int g_adwSuspensionLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x1003ffd0
 * @ghidraAddress BRGlide: 0x10039510
 */
int InputOptionsUI_UpdateInputDeviceLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac3e0
 * @ghidraAddress BRGlide: 0x100abb80
 */
extern int g_adwInputDeviceLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x1003fc40
 * @ghidraAddress BRGlide: 0x10039180
 */
int ConnectionUI_UpdateServiceProviderLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac3f0
 * @ghidraAddress BRGlide: 0x100abb90
 */
extern int g_adwServiceProviderLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x1003fda0
 * @ghidraAddress BRGlide: 0x100392e0
 */
int GameOptionsUI_UpdateCarShadowLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac408
 * @ghidraAddress BRGlide: 0x100abba8
 */
extern int g_adwCarShadowLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x1003fd30
 * @ghidraAddress BRGlide: 0x10039270
 */
int GameOptionsUI_UpdateSkidMarksLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac418
 * @ghidraAddress BRGlide: 0x100abbb8
 */
extern int g_adwSkidMarksLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x1003fe10
 * @ghidraAddress BRGlide: 0x10039350
 */
int GameOptionsUI_UpdateSpecularLightingLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac410
 * @ghidraAddress BRGlide: 0x100abbb0
 */
extern int g_adwSpecularLightingLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x1003fcb0
 * @ghidraAddress BRGlide: 0x100391f0
 */
int GameOptionsUI_UpdateForceFeedbackLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac400
 * @ghidraAddress BRGlide: 0x100abba0
 */
extern int g_adwForceFeedbackLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x100446d0
 */
int ShowNetworkCarSetupScreen(MenuWidget *pWidget = nullptr);

/**
 * @ghidraAddress BRD3D: 0x10058750
 */
void BuildNetworkCarSetupUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10043a00
 */
int NetworkCarSetupUI_OnRefreshSessionDesc(MenuWidget *pWidget);
/**
 * @ghidraAddress BRD3D: 0x10043810
 */
int NetworkCarSetupUI_OnEscape(MenuWidget *pWidget);
/**
 * @ghidraAddress BRD3D: 0x1003ebe0
 */
int NetworkCarSetupUI_UpdateSelectedPlayer(MenuWidget *pWidget);
/**
 * @ghidraAddress BRD3D: 0x1003f7f0
 */
int CarSetupUI_UpdateTiresLabel(MenuWidget *pWidget);
/**
 * @ghidraAddress BRD3D: 0x100ac348
 */
extern const int g_adwTireLabelStrIds[3];
/**
 * @ghidraAddress BRD3D: 0x1003f720
 */
int CarSetupUI_UpdateCarImage(MenuWidget *pWidget);
/**
 * @ghidraAddress BRD3D: 0x100ac5a8
 */
extern const int g_adwCarImageIds[32];

/**
 * @ghidraAddress BRD3D: 0x1003e100
 */
int IsGameCdInDrive(int nDrive);

/**
 * @ghidraAddress BRD3D: 0x10045a00
 */
int FindGameCdDrive(void);

/**
 * @ghidraAddress BRD3D: 0x10045900
 */
int ShowChampionshipSeasonMenu(void);

/**
 * @ghidraAddress BRD3D: 0x1004f2b0
 */
void BuildChampionshipSeasonUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x1005a6e0
 * @stub
 */
void BuildSaveTimeAttackMenuUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10044d00
 */
int ShowLoadGhostMenu(void);

/**
 * @ghidraAddress BRD3D: 0x100443e0
 */
int ShowHostRaceSetupScreen(void);
/**
 * @ghidraAddress BRD3D: 0x100440d0
 */
int ShowCreateGameScreen(void);

/**
 * @ghidraAddress BRD3D: 0x10043bf0
 */
int ShowMultiplayerNameScreen(void);

/**
 * @ghidraAddress BRD3D: 0x10043e70
 */
int ShowJoinCreateGameScreen(void);

/**
 * @ghidraAddress BRD3D: 0x10045dc0
 */
int ShowSeasonProgressInfoScreen(MenuWidget *pWidget = nullptr);

/**
 * @ghidraAddress BRD3D: 0x10044f50
 */
int ShowRaceSetupMenu(void);

/**
 * @ghidraAddress BRD3D: 0x10046170
 */
int ShowQuitGameConfirmMenu(void);

/**
 * @ghidraAddress BRD3D: 0x10059760
 */
int BuildTimeAttackMenuUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10059bb0
 * @stub
 */
void BuildLoadTimeAttackMenuUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10057c10
 */
void BuildMultiplayerGameSetupUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10044600
 */
int MultiplayerGameSetupUI_OnCycleCarClass(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100563e0
 */
int BuildPlayerNameEntryUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10056ff0
 */
int BuildJoinCreateGameUI(MenuScreen *pScreen);

/** @ghidraAddress BRD3D: 0x100575f0 */
int BuildCreateGameMenuUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10044280
 */
int JoinSessionAndShowRaceSetup(void);

/**
 * @ghidraAddress BRD3D: 0x10052030
 */
void BuildSeasonProgressInfoUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x1004b430
 * @stub
 */
void BuildRaceSetupUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10049c20
 */
int BuildQuitGameConfirmUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10044970
 */
int MultiplayerGameSetupUI_OnBackAsClient(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10044a30
 */
int MultiplayerGameSetupUI_OnBackAsHost(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x100444c0
 */
int CreateGameMenuUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10044ae0
 */
int JoinCreateGameUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x100464e0
 */
int CarSetupUI_OnBackToRaceSetup(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10045090
 */
int LoadSeasonUI_OnContinueSeason(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x100458c0
 */
int CarSetupUI_OnOpenOptions(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x100458e0
 */
int CarSetupUI_OnOpenSaveSeason(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046dc0
 */
int PreRaceInfoUI_OnBackToLoadSeason(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046d70
 */
int PreRaceInfoUI_OnBackToChampionshipSeason(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046cd0
 */
int CarSetupUI_OnBackToSeasonProgressInfo(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046d20
 */
int CarSetupUI_OnBackToTimeAttack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10043fc0
 */
int PlayerNameEntryUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046450
 */
int RaceSetupUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10044cb0
 */
int RaceSetupUI_OnBackToTimeAttack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10044b40
 */
int ModemConnectionUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046e10
 */
int LoadSeasonUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10044f00
 */
int SaveTimeAttackMenuUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046560
 */
int KeyConfigUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x100466c0
 */
int AudioOptionsUI_OnBackAndSave(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046f50
 */
int PreRaceInfoUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046520
 */
int OptionsMenuUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046bb0
 */
int OptionsMenuUI_OnBackToCarSetup(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046750
 */
int OptionsMenuUI_OnBackToPreRaceInfo(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046830
 */
int OptionsMenuUI_OnBackToSeasonProgressInfo(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046bf0
 */
int SaveSeasonUI_OnBackToCarSetup(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046790
 */
int SaveSeasonUI_OnBackToPreRaceInfo(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046870
 */
int SaveSeasonUI_OnBackToSeasonProgressInfo(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046eb0
 */
int SaveSeasonUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046c90
 */
int ChampionshipSeasonUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046710
 */
int GameOptionsUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x100465a0
 */
int InputOptionsUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10044de0
 */
int LoadTimeAttackMenuUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x100463c0
 */
int MultiplayerConnectionUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x100464a0
 */
int QuitGameConfirmUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x100470e0
 */
int RaceResultsTotalsUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10047060
 */
int SeasonProgressInfoUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10044c70
 */
int TimeAttackMenuUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x100465e0
 */
int VideoOptionsUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10045800
 */
int RaceResultsInfoUI_OnOpenOptions(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10045820
 */
int RaceResultsInfoUI_OnOpenSaveSeason(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10045840
 */
int RaceResultsSummaryUI_OnOpenOptions(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10045860
 */
int RaceResultsSummaryUI_OnOpenSaveSeason(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10045880
 */
int RaceResultsTotalsUI_OnOpenOptions(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x100458a0
 */
int RaceResultsTotalsUI_OnOpenSaveSeason(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046910
 */
int OptionsMenuUI_OnBackToRaceResultsInfo(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046950
 */
int SaveSeasonUI_OnBackToRaceResultsInfo(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x100469f0
 */
int OptionsMenuUI_OnBackToRaceResultsSummary(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046a30
 */
int SaveSeasonUI_OnBackToRaceResultsSummary(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046ad0
 */
int OptionsMenuUI_OnBackToRaceResultsTotals(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x10046b10
 */
int SaveSeasonUI_OnBackToRaceResultsTotals(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x100470a0
 */
int RaceResultsSummaryUI_OnBack(MenuActionWidget *pItem);

/**
 * @ghidraAddress BRD3D: 0x100451e0
 */
int ShowOptionsMenu(void);

/**
 * @ghidraAddress BRD3D: 0x1004bdc0
 */
int BuildOptionsMenuUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x100419d0
 * @ghidraAddress BRGlide: 0x1003af30
 */
void SetActiveMenuItemText(const char *pszText);

/**
 * @ghidraAddress BRD3D: 0x10a9dbd0
 * @ghidraAddress BRGlide: 0x10ac4c58
 */
extern int g_dwRootMenuStringIndex;

/**
 * @ghidraAddress BRD3D: 0x100ab558
 * @ghidraAddress BRGlide: 0x100aacf8
 */
extern const RECT g_rcMenuRootTitleBar;

/**
 * @ghidraAddress BRD3D: 0x10aa29c8
 */
extern MenuWidget *g_pOptionsSubMenu;

/**
 * @ghidraAddress BRD3D: 0x100ab3f4
 */
extern int g_dwFileListSelectedSlot;

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
 * Set the rectangles of the sprites of the menus.
 *
 * @ghidraAddress BRD3D: 0x1005f800
 * @ghidraAddress BRGlide: 0x10058540
 */
void InitMenuSpriteRects(void);

/** A sprite of the menus, a part of a menu image. */
struct MenuSprite {
    int nImage;    /*!< The index in g_aMenuImages of the image. */
    RECT rc;       /*!< The part of the image. */
    DWORD dwFlags; /*!< The flags of BltFast(), DDBLTFAST_SRCCOLORKEY or 0. */
};

/**
 * The sprites of the menus. The first is the whole of the loading image.
 *
 * @ghidraAddress BRD3D: 0x100ab568
 * @ghidraAddress BRGlide: 0x100aad08
 */
extern MenuSprite g_aMenuSprites[];

/**
 * The number of times the menu images were loaded. Every load but the first shows the loading
 * image first.
 *
 * @ghidraAddress BRD3D: 0x10aa2a6c
 * @ghidraAddress BRGlide: 0x10ac5dc4
 */
extern int g_dwMenuSurfaceInitCount;

#if TGR_RENDERER == TGR_RENDERER_D3D
/**
 * The DirectDraw interface the menu images are loaded with, while the menus are set up.
 *
 * @ghidraAddress BRD3D: 0x10a9d070
 */
extern IDirectDraw2 *g_pMenuDirectDraw;

/**
 * Load every menu image into a surface with a green colour key, showing the loading image while
 * the images load again.
 *
 * @return Whether every image loaded. The game exits when one does not.
 * @ghidraAddress BRD3D: 0x1005f690
 */
bool InitMenuDirectDrawSurfaces(void);
#endif

#if TGR_RENDERER == TGR_RENDERER_GLIDE
/**
 * Load every menu image.
 *
 * @ghidraAddress BRGlide: 0x100583c0
 */
void InitMenuSurfaces(void);
#endif

/**
 * Copy adwMenuSelectionBackup of g_GlobalRallyConfig and its other settings back into the choices
 * of the menus. The input device of the configuration is taken from the choice of the menu before
 * the choice is restored, and a restored choice of 1 becomes 2. The cars and tracks the choices
 * unlocked are added to the selectable ones.
 *
 * @ghidraAddress BRD3D: 0x1003e3a0
 * @ghidraAddress BRGlide: 0x100379b0
 */
void RestoreMenuSelections(void);

#if TGR_RENDERER == TGR_RENDERER_D3D
/**
 * Choose the DirectDraw driver and the display mode of g_GlobalRallyConfig.
 *
 * @ghidraAddress BRD3D: 0x1003e2c0
 */
void SelectConfiguredDisplayMode(void);
#endif

/** The number of entries of g_adwInputDeviceMenuIndexToDevice. */
#define INPUT_DEVICE_MENU_CHOICES 4

/**
 * Show the screen of the results of a race.
 *
 * @ghidraAddress BRD3D: 0x10045ea0
 * @ghidraAddress BRGlide: 0x1003f340
 * @stub{BRGlide}
 */
int ShowRaceResultsScreen(void);

/**
 * Show the lobby of network play again after a race.
 *
 * @ghidraAddress BRD3D: 0x100447d0
 * @ghidraAddress BRGlide: 0x1003dd20
 */
int ReturnToNetworkLobbyScreen(void);

/**
 * Show the menu of a finished time attack, when it is not shown already.
 *
 * @param nUnused Not read.
 * @return 0 when the menu cannot be created, 1 otherwise.
 * @ghidraAddress BRD3D: 0x10044b90
 * @ghidraAddress BRGlide: 0x1003e0e0
 * @stub{BRGlide}
 */
int ShowTimeAttackMenu(int nUnused);

/**
 * Show the menu to save the ghost car of a time attack, when it is not shown already.
 *
 * @param nUnused Not read.
 * @return 0 when the menu cannot be created, 1 otherwise.
 * @ghidraAddress BRD3D: 0x10044e20
 * @ghidraAddress BRGlide: 0x1003e370
 * @stub{BRGlide}
 */
int ShowSaveGhostMenu(int nUnused);

/**
 * Enter the time attack mode, first showing the menus of a finished time attack when the first
 * vehicle has driven every lap.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10041930
 * @ghidraAddress BRGlide: 0x1003ae90
 */
int EnterTimeAttackMode(void);

/**
 * Show the lobby of network play.
 *
 * @ghidraAddress BRD3D: 0x100441a0
 * @ghidraAddress BRGlide: 0x1003d6f0
 */
void EnterNetworkLobbyScreen(void);

/**
 * The width of the surface of the front end. The Glide library sets 640 as the game is set up.
 *
 * @ghidraAddress BRD3D: 0x10690a0c
 * @ghidraAddress BRGlide: 0x105ccbb4
 */
extern int g_dwFrontEndSurfaceWidth;

/**
 * The height of the surface of the front end. The Glide library sets 480 as the game is set up.
 *
 * @ghidraAddress BRD3D: 0x10690a08
 * @ghidraAddress BRGlide: 0x105ccbb0
 */
extern int g_dwFrontEndSurfaceHeight;

#if TGR_RENDERER == TGR_RENDERER_D3D
/**
 * The bits of a pixel of the surface of the front end.
 *
 * @ghidraAddress BRD3D: 0x10690a00
 */
extern int g_dwFrontEndSurfaceBitDepth;

/**
 * The refresh rate of the surface of the front end, 0 for the default.
 *
 * @ghidraAddress BRD3D: 0x10690a04
 */
extern int g_dwFrontEndSurfaceRefreshRate;
#endif

/**
 * Whether the screen of the setup of the car of network play shows.
 *
 * @ghidraAddress BRD3D: 0x10aa285c
 * @ghidraAddress BRGlide: 0x10ac5bb4
 */
extern int g_dwInNetworkCarSetup;

/**
 * Whether ActivateMenuScreen() is entering the screen the game returns to.
 *
 * @ghidraAddress BRD3D: 0x10aa2878
 * @ghidraAddress BRGlide: 0x10ac5bd0
 */
extern int g_dwMenuScreenActivated;

/**
 * Whether the lobby was entered for a game launched from a lobby.
 *
 * @ghidraAddress BRD3D: 0x10aa2a50
 * @ghidraAddress BRGlide: 0x10ac5da8
 */
extern int g_dwLobbyLaunchHandled;

/**
 * Restore the surfaces of the loaded menu images and load their bitmaps into them again.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x1005f4e0
 * @ghidraAddress BRGlide: 0x10001320
 * @stub{BRGlide}
 */
bool RestoreMenuSurfaces(void);

/**
 * Release the surfaces of the menu images and exit the game.
 *
 * @param hWnd Not read.
 * @return False, though the routine does not return.
 * @ghidraAddress BRD3D: 0x1005f580
 * @ghidraAddress BRGlide: 0x10058360
 */
bool ReleaseMenuSurfacesAndFail(HWND hWnd);

/**
 * Draw a part of a menu image into the back buffer. The cursor is clipped to the screen of the
 * menus. A lost surface restores the menu images and the part is not drawn.
 *
 * @param nDestX The left to draw at.
 * @param nDestY The top to draw at.
 * @param wImage The index in g_aMenuImages of the image.
 * @param pRect The part of the image.
 * @param dwFlags The flags of BltFast(). DDBLTFAST_WAIT is added.
 * @ghidraAddress BRD3D: 0x1005f5a0
 * @ghidraAddress BRGlide: 0x10058380
 * @stub{BRGlide}
 */
void BlitSurfaceRegion(int nDestX, int nDestY, WORD wImage, const RECT *pRect, DWORD dwFlags);

/**
 * Release the surfaces of the loaded menu images. The Glide library frees its images and the
 * surface of the menu frame instead.
 *
 * @ghidraAddress BRD3D: 0x1005f530
 * @ghidraAddress BRGlide: 0x10058300
 * @stub{BRGlide}
 */
void ReleaseMenuSurfaces(void);

/**
 * The state of the network menus. A state of 2 or 3 delays ShutdownFrontEnd().
 *
 * @ghidraAddress BRD3D: 0x10aa2854
 * @ghidraAddress BRGlide: 0x10ac5bac
 */
extern int g_dwNetworkMenuState;

/**
 * Whether the cheat that unlocks every track was entered.
 *
 * @ghidraAddress BRD3D: 0x10aa28f8
 * @ghidraAddress BRGlide: 0x10ac5c50
 */
extern int g_dwCheatAllTracksUnlocked;

/**
 * Whether the cheat that unlocks the bonus tracks was entered.
 *
 * @ghidraAddress BRD3D: 0x10aa28fc
 * @ghidraAddress BRGlide: 0x10ac5c54
 */
extern int g_dwCheatBonusTracksUnlocked;

/**
 * Whether the cheat that unlocks the bonus cars was entered.
 *
 * @ghidraAddress BRD3D: 0x10aa28f4
 * @ghidraAddress BRGlide: 0x10ac5c4c
 */
extern int g_dwCheatBonusCarsUnlocked;

/**
 * Whether the cheat that shows the closing sequence was entered.
 *
 * @ghidraAddress BRD3D: 0x10aa2a40
 * @ghidraAddress BRGlide: 0x10ac5d98
 */
extern int g_dwCheatOutroSequenceEnabled;

/**
 * Unlock every track, play the sound of a cheat, and set the network menus to state 2.
 *
 * @ghidraAddress BRD3D: 0x10047500
 * @ghidraAddress BRGlide: 0x10040930
 */
void Cheat_Hazel_UnlockAllTracks(void);

/**
 * Unlock the bonus tracks, play the sound of a cheat, and set the network menus to state 2.
 *
 * @ghidraAddress BRD3D: 0x10047530
 * @ghidraAddress BRGlide: 0x10040960
 */
void Cheat_Brielle_UnlockBonusTracks(void);

/**
 * Stop the vehicles deforming, play the sound of this cheat, and set the network menus to state 3.
 *
 * @ghidraAddress BRD3D: 0x10047560
 * @ghidraAddress BRGlide: 0x10040990
 */
void Cheat_DisableVehicleDeformation(void);

/**
 * Show the closing sequence, play the sound of a cheat, and set the network menus to state 2.
 *
 * @ghidraAddress BRD3D: 0x10047590
 * @ghidraAddress BRGlide: 0x100409c0
 */
void Cheat_Sophia_EnableOutroSequence(void);

/**
 * Unlock the bonus cars, play the sound of a cheat, and set the network menus to state 2.
 *
 * @ghidraAddress BRD3D: 0x100475c0
 * @ghidraAddress BRGlide: 0x100409f0
 */
void Cheat_Lynette_UnlockBonusCars(void);

/**
 * Whether the cheat that unlocks every car was entered.
 *
 * @ghidraAddress BRD3D: 0x10aa28f0
 * @ghidraAddress BRGlide: 0x10ac5c48
 */
extern int g_dwCheatAllCarsUnlocked;

/**
 * Unlock every car, play the sound of a cheat, and set the network menus to state 2.
 *
 * @ghidraAddress BRD3D: 0x100474d0
 * @ghidraAddress BRGlide: 0x10040900
 */
void Cheat_Benjamin_UnlockAllCars(void);

/** A cheat and the word that enters it. */
struct CheatCode {
    void (*pfnApply)(void); /*!< The routine of the cheat. */
    const char *pszCode;    /*!< The word, in lowercase, or null for the end of the table. */
};

/** The number of entries of g_aCheatCodes, with the entry that ends it. */
#define CHEAT_CODE_COUNT 7

/**
 * The cheats, ended by an entry without a word.
 *
 * @ghidraAddress BRD3D: 0x100ac6a8
 * @ghidraAddress BRGlide: 0x100abe48
 */
extern CheatCode g_aCheatCodes[CHEAT_CODE_COUNT];

/** The number of entries of g_anCheatInputBuffer. */
#define CHEAT_INPUT_BUFFER_SIZE 32

/**
 * The last characters typed for the cheats, a ring that wraps at its end.
 *
 * @ghidraAddress BRD3D: 0x10a9e150
 * @ghidraAddress BRGlide: 0x10ac51d8
 */
extern int g_anCheatInputBuffer[CHEAT_INPUT_BUFFER_SIZE];

/**
 * The entry of g_anCheatInputBuffer the next character fills.
 *
 * @ghidraAddress BRD3D: 0x10aa2a48
 * @ghidraAddress BRGlide: 0x10ac5da0
 */
extern int g_dwCheatInputCount;

/**
 * The last character the window received, or 0 once it is read.
 *
 * @ghidraAddress BRD3D: 0x10aa33e4
 * @ghidraAddress BRGlide: 0x10ac6744
 */
extern int g_dwLastWmCharParam;

/**
 * Add the last character the window received to the characters of the cheats, in lowercase, and
 * apply each cheat whose word they end with.
 *
 * @ghidraAddress BRD3D: 0x10047610
 * @ghidraAddress BRGlide: 0x10040a40
 */
void ProcessCheatCodeInput(void);

/**
 * Apply each cheat whose word the characters of the cheats end with.
 *
 * @ghidraAddress BRD3D: 0x10047660
 * @ghidraAddress BRGlide: 0x10040a90
 * @stub{BRGlide}
 */
void CheckCheatCodeMatches(void);

/**
 * The index of the network session chosen in the list of sessions.
 *
 * @ghidraAddress BRD3D: 0x10aa2880
 * @ghidraAddress BRGlide: 0x10ac5bd8
 */
extern int g_dwSelectedSessionIndex;

/**
 * Copy the choices of the menus to adwMenuSelectionBackup of g_GlobalRallyConfig.
 *
 * @ghidraAddress BRD3D: 0x1003e310
 * @ghidraAddress BRGlide: 0x10037920
 */
void BackupMenuSelections(void);

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
 * Set the cars of the chosen class and choose the first of them, when the class changed since the
 * last call. An unknown class takes the cars of the first.
 *
 * @ghidraAddress BRD3D: 0x10044540
 * @ghidraAddress BRGlide: 0x1003da90
 */
void ApplyCarGroupPreset(void);

/**
 * Report whether a car can be chosen in the current mode of the game.
 *
 * @param nCar The car.
 * @return Nonzero when the car can be chosen.
 * @ghidraAddress BRD3D: 0x1003f320
 * @ghidraAddress BRGlide: 0x10038860
 */
int IsCarSelectable(int nCar);

/**
 * Reset the track, the weather, the car and its settings, and the input device, to the keyboard,
 * for a new game mode.
 *
 * @ghidraAddress BRD3D: 0x1003dfc0
 * @ghidraAddress BRGlide: 0x10037660
 */
void ResetGameModeConfig(void);

/** An error of the game that ShowErrorMessage() can show. */
struct ErrorMessage {
    int bFatal;         /*!< Whether the game exits after the message. */
    unsigned nStringId; /*!< The string of the message. */
};

/** The number of entries of g_ErrorMessageTable. */
#define ERROR_MESSAGE_COUNT 8

/**
 * The errors of the game, indexed by the code of the error.
 *
 * @ghidraAddress BRD3D: 0x100ac660
 * @ghidraAddress BRGlide: 0x100abe00
 */
extern ErrorMessage g_ErrorMessageTable[ERROR_MESSAGE_COUNT];

/**
 * Show an error of the game in a message box from its second character, and exit when the error is
 * fatal. A code above #ERROR_MESSAGE_COUNT shows nothing.
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

/** A key of text input and the character it types. */
struct KeyCharacter {
    DWORD dwKey; /*!< The key. */
    char chText; /*!< The character. */
};

/** The number of entries of g_KeyNameLookupTable. */
#define KEY_CHARACTER_COUNT 98

/**
 * The keys of text input, each typing the printable character of its own code.
 *
 * @ghidraAddress BRD3D: 0x100ace58
 * @ghidraAddress BRGlide: 0x100ac5f8
 */
extern KeyCharacter g_KeyNameLookupTable[KEY_CHARACTER_COUNT];

/**
 * The character a key of text input types.
 *
 * @param dwKey The key.
 * @return The character, or 0 when the key types none.
 * @ghidraAddress BRD3D: 0x1005b540
 * @ghidraAddress BRGlide: 0x10054360
 */
char LookupKeyCharacter(DWORD dwKey);

/**
 * The track mask used before loading a championship profile.
 *
 * @ghidraAddress BRD3D: 0x10a9d010
 */
extern unsigned g_dwUnlockedTrackMask;

/**
 * @ghidraAddress BRD3D: 0x100ac420
 */
extern int g_adwCarMenuIndexToCarId[32];

/**
 * @ghidraAddress BRD3D: 0x100ac4a0
 */
extern int g_adwTireMenuIndexToTireType[3];

/**
 * @ghidraAddress BRD3D: 0x100ac4b0
 */
extern int g_adwSuspensionMenuIndexToType[3];

/**
 * @ghidraAddress BRD3D: 0x100ac4c0
 */
extern int g_adwWeatherMenuIndexToWeather[5];

/**
 * @ghidraAddress BRD3D: 0x100ac4d8
 */
extern int g_adwTrackMenuIndexToTrackId[15];

/**
 * @ghidraAddress BRD3D: 0x100ac518
 */
extern int g_adwTransmissionMenuIndexToType[2];

/**
 * Report a track's selection mask or a cheat override.
 *
 * @param nTrack The track menu index.
 * @return Zero when unavailable, otherwise a nonzero mask or one.
 * @ghidraAddress BRD3D: 0x1003f2b0
 */
unsigned IsTrackSelectable(int nTrack);

/**
 * Restore menu choices, advance unavailable selections, and apply race settings.
 *
 * @ghidraAddress BRD3D: 0x1003e510
 */
void ResolveRaceSelections(void);

/**
 * Reset menu selections, championship profiles, and result display values.
 *
 * @ghidraAddress BRD3D: 0x1003e680
 */
void ResetMenuSelectionsToDefaults(void);

/**
 * @ghidraAddress BRD3D: 0x10052f50
 */
void BuildRaceResultsInfoUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10040ee0
 */
int RaceResultsInfoUI_UpdateSelectedTimeLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10053cf0
 */
void BuildRaceResultsSummaryUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10054b50
 */
void BuildRaceResultsTotalsUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x1004a260
 */
int BuildResultsSummaryExitSeasonConfirmUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10051990
 */
int BuildSeasonCongratsUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10051d30
 */
int BuildNotEnoughPointsUI(MenuScreen *pScreen);

/**
 * Show cached championship congratulations, building the screen on its first visit.
 *
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x10043260
 */
int ShowSeasonCongratsScreen(void);

/**
 * Show cached championship failure results, building the screen on its first visit.
 *
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x10043330
 */
int ShowNotEnoughPointsScreen(void);

/**
 * Show cached race totals, building the screen on its first visit.
 *
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x100460a0
 */
int ShowRaceResultsTotalsScreen(MenuWidget *pWidget = nullptr);

/**
 * Show cached race summary results and build an exit confirmation on the first visit.
 *
 * @return Whether both initial allocations succeeded, or the summary was already cached.
 * @ghidraAddress BRD3D: 0x10045f70
 */
int ShowRaceResultsSummaryScreen(MenuWidget *pWidget = nullptr);

/**
 * @ghidraAddress BRD3D: 0x100509f0
 */
void BuildPreRaceInfoUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10040c00
 */
int PreRaceInfoUI_UpdateBestTimeLabel(MenuWidget *pWidget);
/**
 * @ghidraAddress BRD3D: 0x10040d70
 */
int PreRaceInfoUI_UpdateBestLapLabel(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10049f40
 */
int BuildPreRaceExitSeasonConfirmUI(MenuScreen *pScreen);

/**
 * Show cached pre-race information and build an exit confirmation on the first visit.
 *
 * @return Whether both initial allocations succeeded, or the information screen was cached.
 * @ghidraAddress BRD3D: 0x10045c90
 */
int ShowPreRaceInfoScreen(void);

/**
 * @ghidraAddress BRD3D: 0x1004f700
 */
void BuildLoadSeasonUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10050060
 */
void BuildSaveSeasonUI(MenuScreen *pScreen);

/**
 * Show cached season loading, building the screen on its first visit.
 *
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x10045af0
 */
int ShowLoadSeasonMenu(void);

/**
 * Show cached season saving, building the screen on its first visit.
 *
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x10045bc0
 */
int ShowSaveSeasonScreen(void);

/**
 * Apply championship progress and switch from totals to the next championship screen.
 *
 * @param pItem The action item on the totals screen.
 * @return Zero.
 * @ghidraAddress BRD3D: 0x10047290
 */
int RaceResultsTotalsUI_OnContinue(MenuActionWidget *pItem);
