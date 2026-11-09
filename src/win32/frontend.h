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

#include "common/assetnames.h"
#include "common/frontendstate.h"
#include "win32/glide/rgb565image.h"
#include "win32/menufont.h"

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

    /**
     * Read descriptions from the last 128 bytes of matching saved files.
     *
     * @param pszPattern The file search pattern.
     * @return Zero when the first search fails, otherwise one.
     * @ghidraAddress BRD3D: 0x1005cf20
     * @ghidraAddress BRGlide: 0x10055d40
     */
    virtual int ScanSaveFiles(const char *pszPattern);

    /**
     * @param index The slot index, 0..99, without bounds checking.
     * @return The borrowed terminated name, valid until the slot changes or the list is destroyed.
     */
    const char *GetName(int index) const {
        return aszNames[index];
    }
    /**
     * Copy a terminated name into a slot without checking the destination capacity.
     * @param index The slot index, 0..99.
     * @param text A non-null terminated string of at most SAVE_FILE_NAME_SIZE - 1 bytes.
     */
    void SetName(int index, const char *text) {
        std::strcpy(aszNames[index], text);
    }
    /**
     * Format one integer into a slot without checking the destination capacity.
     * @param index The slot index, 0..99.
     * @param format A non-null terminated printf format accepting one int; the result including
     * its terminator must fit SAVE_FILE_NAME_SIZE bytes.
     * @param value The integer supplied to the format.
     */
    void SetFormattedName(int index, const char *format, int value) {
        std::sprintf(aszNames[index], format, value);
    }

private:
    char aszNames[SAVE_FILE_NAME_COUNT][SAVE_FILE_NAME_SIZE]; /*!< The names, empty at the start. */

    /**
     * Count matches after the first, stopping after 100 files.
     *
     * @param pszPattern The file search pattern.
     * @return Minus one when the first search fails, otherwise the count after the first match.
     * @ghidraAddress BRD3D: 0x1005d0a0
     * @ghidraAddress BRGlide: 0x10055ed0
     */
    virtual int CountMatchingFiles(const char *pszPattern);

    /**
     * Replace pattern markers with zeroes and derive a filename from matching list entries.
     *
     * @param pszPattern The mutable pattern, with X or x marking the numeric suffix.
     * @param ppszName The destination filename buffer.
     * @ghidraAddress BRD3D: 0x1005d110
     * @ghidraAddress BRGlide: 0x10055f40
     */
    virtual void GenerateNextSaveFileName(char *pszPattern, char **ppszName);

    /**
     * Write 100-byte records from the active screen's first file list.
     *
     * @param nUnused Not read.
     * @return One, including after reported file errors.
     * @ghidraAddress BRD3D: 0x1005cc20
     * @ghidraAddress BRGlide: 0x10055a40
     */
    virtual int WriteSeasonDescFile(int nUnused);

    /**
     * Load and uppercase a season description into the active screen's first file list.
     *
     * @param nSlot The signed slot index.
     * @return One, including when the file does not open.
     * @ghidraAddress BRD3D: 0x1005ccd0
     * @ghidraAddress BRGlide: 0x10055af0
     */
    virtual int LoadSeasonDescription(short nSlot);

    /**
     * Set a root screen file-list entry using the numeric part of its filename.
     *
     * @param pszFileName The saved filename.
     * @param pszDescription The description copied into the entry.
     * @return One.
     * @ghidraAddress BRD3D: 0x1005ce30
     * @ghidraAddress BRGlide: 0x10055c50
     */
    virtual int SetEntryFromSaveFileName(const char *pszFileName, const char *pszDescription);
};

class MenuScreen;
class MenuWidget;
/** Common widget representation used by menu action callbacks. */
using MenuActionWidget = MenuWidget;
/** Common widget representation used by network menu callbacks. */
using NetworkMenuWidget = MenuWidget;
class MenuItemList;

/** Byte payload associated with a menu-list slot. */
struct MenuItemData {
    /** Number of payload bytes last copied into the slot. */
    unsigned dwSize;
    /** Allocated payload storage, or null before attachment; native removal does not release it. */
    unsigned char *pData;
};

/**
 * Menu list with 100 font and payload slots, optional scrolling, and selection callbacks.
 * Native removal and destruction do not release item-data allocations.
 * Direct accessors do not validate indices or synchronise related layout fields.
 */
class MenuItemList {
public:
    /**
     * Initialise an empty list with null callbacks, disabled scrolling, and no images.
     * Initialise layout before drawing or adding items.
     * @ghidraAddress BRD3D: 0x1005b7f0
     * @ghidraAddress BRGlide: 0x10054610
     */
    MenuItemList();
    /**
     * Destroy font storage without releasing attached payload allocations.
     * @ghidraAddress BRD3D: 0x1005b8f0
     * @ghidraAddress BRGlide: 0x10054710
     */
    virtual ~MenuItemList();

    /**
     * Append a text row, removing the oldest row first when all 100 slots are used.
     * Optional automatic scrolling can fail after insertion without removing the new row.
     * @param text A terminated label fitting MenuFont storage, or null to reject insertion.
     * @param flags Font flag bits combined with the slot's existing flags.
     * @param style The font style, with 3 selecting alternate-font measurement.
     * @param bounds Non-null borrowed text bounds; left and right are copied, with rows 19 pixels
     * apart from the list's origin.
     * @param full Whether to copy the complete label rather than its first ten bytes.
     * @return One normally; zero for null text or an unavailable automatic-scroll target.
     * @ghidraAddress BRD3D: 0x1005bc10
     * @ghidraAddress BRGlide: 0x10054a30
     */
    virtual int
    AddItem(const char *text, unsigned flags, unsigned char style, const RECT *bounds, bool full);
    /**
     * Copy list bounds and configure visible rows, background, and enabled scrollbar geometry.
     * Scrollbar markers must be assigned before this call when scrolling is required.
     * @param flags The list flag bits to store.
     * @param bounds Non-null borrowed list bounds in menu pixels, copied during the call.
     * @param visibleRows The visible row count stored as an unsigned 16-bit value.
     * @param firstVisibleRow The first visible slot stored as a signed 16-bit value.
     * @param backgroundImage The image identifier stored as a signed 16-bit value.
     * @return One.
     * @ghidraAddress BRD3D: 0x1005b910
     * @ghidraAddress BRGlide: 0x10054730
     */
    virtual int Initialize(unsigned flags,
                           const RECT *bounds,
                           int visibleRows,
                           int firstVisibleRow,
                           int backgroundImage);

    /**
     * Draw the list and process input when text editing and modal dialogs do not block input.
     * @param selection The selection passed to the drawing operation.
     * @return The activated row, possibly changed by its callback, or -1 when no row is activated.
     * @ghidraAddress BRD3D: 0x1005cb40
     * @ghidraAddress BRGlide: 0x10055960
     */
    virtual int UpdateAndRenderMenuWidget(int selection);
    /**
     * Measure a row, process active text editing, and notify its completion or cancellation.
     * @param nIndex The row index, 0..99, without bounds checking.
     * @return Zero when the caret is inactive; one when text input was processed.
     * @ghidraAddress BRD3D: 0x1005bb80
     * @ghidraAddress BRGlide: 0x100549a0
     */
    virtual int UpdateItemAndNotify(int nIndex);

    /**
     * Copy bytes into slot payload storage, allocating only when the slot has no buffer.
     * Existing storage is reused without resizing, and allocation failure is unchecked.
     * @param data Non-null readable storage of at least size bytes, borrowed during copying.
     * @param size The byte count; existing slot storage must have sufficient capacity.
     * @param index The slot index, 0..99, or -1 for the last item; empty lists select slot 0.
     * @return One.
     * @ghidraAddress BRD3D: 0x1005c200
     * @ghidraAddress BRGlide: 0x10055020
     */
    virtual int SetItemData(const void *data, unsigned size, int index);

    /**
     * Clear a row and shift later rows towards the start without releasing payload allocations.
     * @param index An existing row index in a nonempty list.
     * @return One.
     * @ghidraAddress BRD3D: 0x1005c000
     * @ghidraAddress BRGlide: 0x10054e20
     */
    virtual int RemoveItem(int index);

    /**
     * @param index The font slot index, 0..99.
     * @return A mutable slot reference valid until destruction; row removal can replace its value.
     */
    MenuFont &GetFont(int index) {
        return fonts[index];
    }
    /** @return The stored row count, normally 0..100. */
    unsigned short GetItemCount() const {
        return wItemCount;
    }
    /**
     * @param index The payload slot index, 0..99.
     * @return The borrowed payload pointer, or null; subsequent slot changes can replace it.
     */
    const void *GetItemData(int index) const {
        return itemData[index].pData;
    }
    /**
     * @param index The payload slot index, 0..99.
     * @return The mutable borrowed payload pointer, or null; slot changes can replace it.
     */
    void *GetItemData(int index) {
        return itemData[index].pData;
    }
    /** @return The stored list flag bits. */
    unsigned GetFlags() const {
        return dwFlags;
    }
    /**
     * Replace list flags without rebuilding layout.
     * @param value The list flag bits to store.
     * @return The stored flag bits.
     */
    unsigned SetFlags(unsigned value) {
        return dwFlags = value;
    }
    /**
     * Store vertical scrollbar state without rebuilding layout.
     * @param value The marker, with nonzero enabling the scrollbar during initialisation.
     * @return The stored marker.
     */
    int SetVerticalScrollbar(int value) {
        return bVerticalScrollbar = value;
    }
    /**
     * Replace the row-activation callback without invoking it.
     * @param callback The callback, or null to disable notifications. Arguments are this list and
     * a mutable activated-row index borrowed during the call; the integer return is discarded.
     */
    void SetSelectionChangedCallback(int (*callback)(MenuItemList *, int *)) {
        pfnSelectionChanged = callback;
    }
    /**
     * Replace the text-edit completion callback without invoking it.
     * @param callback The callback, or null to disable notifications. Arguments are this list and
     * the edited row, or -1 for cancellation; the integer return is discarded.
     */
    void SetItemChangedCallback(int (*callback)(MenuItemList *, int)) {
        pfnItemChanged = callback;
    }
    /** @return The stored minimum vertical thumb position in menu pixels. */
    float GetMinScrollThumbY() const {
        return fMinScrollThumbY;
    }
    /** @return The stored maximum vertical thumb position in menu pixels. */
    float GetMaxScrollThumbY() const {
        return fMaxScrollThumbY;
    }
    /** @return The stored vertical thumb position in menu pixels. */
    float GetScrollThumbY() const {
        return fScrollThumbY;
    }
    /**
     * Store a vertical thumb position without clamping or updating its rectangle.
     * @param value The vertical position in menu pixels.
     */
    void SetScrollThumbY(float value) {
        fScrollThumbY = value;
    }
    /**
     * Change the thumb rectangle's top without updating its stored floating-point position.
     * @param value The top menu-pixel coordinate.
     */
    void SetScrollThumbTop(int value) {
        rcScrollThumb.top = value;
    }
    /**
     * Change the thumb rectangle's bottom without resizing other layout fields.
     * @param value The bottom menu-pixel coordinate.
     */
    void SetScrollThumbBottom(int value) {
        rcScrollThumb.bottom = value;
    }

private:
    /**
     * @ghidraAddress BRD3D: 0x1005c270
     * @ghidraAddress BRGlide: 0x10055090
     */
    virtual int BlitSurfaceById(unsigned short image, float x, float y);
    /**
     * @ghidraAddress BRD3D: 0x1005c2c0
     * @ghidraAddress BRGlide: 0x100550e0
     */
    virtual int Draw(int selection);
    /**
     * @ghidraAddress BRD3D: 0x10042af0
     * @ghidraAddress BRGlide: 0x100385e0
     */
    virtual int ReturnOne();

    /**
     * @ghidraAddress BRD3D: 0x1005c510
     * @ghidraAddress BRGlide: 0x10055330
     */
    virtual int UpdateHoverInRect(const RECT *bounds);
    /**
     * @ghidraAddress BRD3D: 0x1005c590
     * @ghidraAddress BRGlide: 0x100553b0
     */
    virtual int ProcessInput(int *selection);

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
    RECT rcBounds;
    RECT rcScrollUp;
    RECT rcScrollDown;
    RECT rcHorizontalScrollLeft;
    RECT rcHorizontalScrollRight;
    RECT rcScrollThumb;
    int bScrollUpHovered;
    int bScrollDownHovered;
    [[maybe_unused]] char abUnknown1a9a4[8]; /* +0x1a9a4 */
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

/**
 * Front-end panel widget with text, sprites, animations, and input callbacks.
 * Direct setters do not validate values or rebuild related layout state.
 * Referenced screens, rectangle arrays, and callbacks must remain valid while used by the widget.
 */
class MenuWidget {
public:
    /**
     * Initialise inactive callbacks, default flags and position, and empty animation/link storage.
     * Image rectangles and text bounds require separate initialisation before their use.
     * @ghidraAddress BRD3D: 0x100476c0
     * @ghidraAddress BRGlide: 0x10040b10
     */
    MenuWidget();
    /**
     * Destroy embedded fonts and list storage without releasing borrowed screens or rectangles.
     * @ghidraAddress BRD3D: 0x100478c0
     * @ghidraAddress BRGlide: 0x10040d10
     */
    virtual ~MenuWidget();
    /**
     * Advance enabled image animation using elapsed milliseconds and mark widgets for redraw.
     * Custom frame durations control frame advancement; other animations request redraw after
     * more than 60 ms.
     * @return One.
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
     * @ghidraAddress BRGlide: 0x10040e60
     */
    virtual int DrawCurrentImage();

public:
    /**
     * Blit a menu sprite's stored source rectangle at the supplied destination.
     * @param nImage A valid menu sprite and image identifier.
     * @param x The horizontal destination in menu pixels.
     * @param y The vertical destination in menu pixels.
     * @return One, without propagating the underlying blit result.
     * @ghidraAddress BRD3D: 0x100479d0
     * @ghidraAddress BRGlide: 0x10040e20
     */
    virtual int BlitSurfaceById(int nImage, int x, int y);

private:
    /**
     * @ghidraAddress BRD3D: 0x10047980
     * @ghidraAddress BRGlide: 0x10040dd0
     */
    virtual int DrawSpriteRegion(const RECT *pRect);
    /**
     * @ghidraAddress BRD3D: 0x10047930
     * @ghidraAddress BRGlide: 0x10040d80
     */
    virtual int DrawSprite();
    /**
     * @ghidraAddress BRD3D: 0x10047a60
     * @ghidraAddress BRGlide: 0x10040eb0
     */
    virtual bool UpdateHoverState();
    /**
     * @ghidraAddress BRD3D: 0x10047cb0
     * @ghidraAddress BRGlide: 0x10041100
     */
    virtual int InitSlideAnimation(int nDuration);
    /**
     * @ghidraAddress BRD3D: 0x10047ce0
     * @ghidraAddress BRGlide: 0x10041130
     */
    virtual long double ComputeSlideOffset(int nTicks);
    /**
     * @ghidraAddress BRD3D: 0x10047d10
     * @ghidraAddress BRGlide: 0x10041160
     */
    virtual int StartSlideAnimation();
    /**
     * @ghidraAddress BRD3D: 0x10047d30
     * @ghidraAddress BRGlide: 0x10041180
     */
    virtual int UpdateSlideAnimation();

public:
    /**
     * Copy text into font slot zero, measure it, and update widget bounds and text dimensions.
     * @param pszText A non-null terminated label fitting MenuFont text storage, copied during
     * the call.
     * @param dwTextFlags Font flags combined with existing flags; bit zero centres measured text.
     * @param bStyle The font style, with 3 selecting alternate-font measurement.
     * @param pRect Non-null borrowed bounds supplying horizontal menu-pixel limits.
     * @ghidraAddress BRD3D: 0x10047eb0
     * @ghidraAddress BRGlide: 0x10041300
     */
    virtual void
    SetText(const char *pszText, unsigned dwTextFlags, unsigned char bStyle, const RECT *pRect);
    /**
     * Assign the parent screen and position, combine flag groups, and set the first sprite.
     * @param pOwner The borrowed parent screen, valid while the widget uses screen operations.
     * @param x The horizontal position in menu pixels.
     * @param y The vertical position in menu pixels.
     * @param dwItemFlags The primary flag bits combined with existing flags.
     * @param dwSecondary The secondary flag bits combined with existing flags.
     * @param dwTertiary The tertiary flag bits combined with existing flags.
     * @param nMode The image-animation mode, with zero disabling timed image updates.
     * @param nImage The initial sprite identifier, also assigned to animation slot zero.
     * @ghidraAddress BRD3D: 0x10047fb0
     * @ghidraAddress BRGlide: 0x10041400
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
     * @ghidraAddress BRGlide: 0x100414b0
     */
    virtual bool IsBlockedByModalDialog();

public:
    /** @return The borrowed parent screen, or null before assignment. */
    MenuScreen *GetScreen() const {
        return pScreen;
    }
    /**
     * @param index The font slot index, 0..2.
     * @return A mutable reference to embedded font storage, valid until widget destruction.
     */
    MenuFont &GetFont(int index) {
        return fonts[index];
    }
    /** @return A mutable reference to the embedded item list, valid until widget destruction. */
    MenuItemList &GetItemList() {
        return itemList;
    }
    /** @return The stored primary widget flag bits. */
    unsigned GetFlags() const {
        return dwFlags;
    }
    /**
     * Replace primary widget flags without updating layout.
     * @param value The primary widget flag bits.
     */
    void SetFlags(unsigned value) {
        dwFlags = value;
    }
    /** @return The horizontal position in menu pixels. */
    float GetX() const {
        return fX;
    }
    /** @return The vertical position in menu pixels. */
    float GetY() const {
        return fY;
    }
    /**
     * Store a horizontal position without updating bounds or font coordinates.
     * @param value The horizontal position in menu pixels.
     */
    void SetX(float value) {
        fX = value;
    }
    /**
     * Store a vertical position without updating bounds or font coordinates.
     * @param value The vertical position in menu pixels.
     */
    void SetY(float value) {
        fY = value;
    }
    /**
     * Store the current sprite identifier without updating geometry.
     * @param value The sprite identifier required by the selected drawing mode.
     */
    void SetSpriteId(short value) {
        nSpriteId = value;
    }
    /** @return The stored current sprite identifier. */
    short GetSpriteId() const {
        return nSpriteId;
    }
    /**
     * Store a sprite identifier in one animation slot.
     * @param index The animation slot index, 0..49.
     * @param value The sprite identifier used when the frame is selected.
     */
    void SetAnimationSpriteId(int index, short value) {
        anSpriteIds[index] = value;
    }
    /**
     * Store the image-animation mode without resetting timers.
     * @param value The mode, with zero disabling timed image updates.
     */
    void SetImageMode(int value) {
        nImageMode = value;
    }
    /**
     * Replace the borrowed source-rectangle array used for custom animation frames.
     * @param rects An array with a valid rectangle for every selected frame, valid while attached.
     */
    void SetImageRects(const RECT *rects) {
        pImageRects = rects;
    }
    /**
     * Select custom frame timings and per-frame source rectangles without initialising either.
     * Positive frame durations must end with a zero or negative duration within the 50 slots.
     * @param value Whether valid frame durations, sprites, and source rectangles are configured.
     */
    void SetCustomFrameDurations(bool value) {
        bCustomFrameDurations = value;
    }
    /**
     * Store a custom frame duration without changing animation progress.
     * @param index The duration slot index, 0..49.
     * @param value The duration in milliseconds; a zero or negative next duration wraps to frame
     * zero.
     */
    void SetFrameDuration(int index, int value) {
        anFrameDurations[index] = value;
    }
    /** @return The stored linked-widget count, normally 0..25. */
    short GetLinkedWidgetCount() const {
        return nLinkedWidgetCount;
    }
    /**
     * @param index The link slot index, 0..24.
     * @return The linked widget index in the parent screen's current panel.
     */
    short GetLinkedWidgetIndex(int index) const {
        return anLinkedWidgetIndices[index];
    }
    /**
     * Store link count without initialising the corresponding link indices.
     * @param value The number of valid link slots, 0..25.
     */
    void SetLinkedWidgetCount(short value) {
        nLinkedWidgetCount = value;
    }
    /**
     * Assign one linked widget index without changing link count.
     * @param index The link slot index, 0..24.
     * @param value A valid widget index in the parent screen's current panel.
     */
    void SetLinkedWidgetIndex(int index, short value) {
        anLinkedWidgetIndices[index] = value;
    }
    /** @return The stored left bound in menu pixels. */
    LONG GetBoundsLeft() const {
        return rcBounds.left;
    }
    /** @return The stored top bound in menu pixels. */
    LONG GetBoundsTop() const {
        return rcBounds.top;
    }
    /**
     * Store the left bound without updating other layout fields.
     * @param value The left menu-pixel coordinate.
     */
    void SetBoundsLeft(LONG value) {
        rcBounds.left = value;
    }
    /**
     * Store the top bound without updating other layout fields.
     * @param value The top menu-pixel coordinate.
     */
    void SetBoundsTop(LONG value) {
        rcBounds.top = value;
    }
    /**
     * Store the right bound without updating other layout fields.
     * @param value The right menu-pixel coordinate.
     */
    void SetBoundsRight(LONG value) {
        rcBounds.right = value;
    }
    /**
     * Store the bottom bound without updating other layout fields.
     * @param value The bottom menu-pixel coordinate.
     */
    void SetBoundsBottom(LONG value) {
        rcBounds.bottom = value;
    }
    /**
     * Install a widget-update callback without invoking it.
     * @param callback The callback receiving this widget, or null. Return -2 to skip the remaining
     * update while continuing, -1 to end the widget, or another value to continue normally.
     */
    void SetUpdateCallback(int (*callback)(MenuWidget *)) {
        pfnUpdate = callback;
    }
    /** @return The installed widget-update callback, or null. */
    auto GetUpdateCallback() const {
        return pfnUpdate;
    }
    /**
     * Install an activation callback without invoking it.
     * @param callback The callback receiving this widget, or null; zero ends the widget update.
     */
    void SetActionCallback(int (*callback)(MenuActionWidget *)) {
        pfnAction = callback;
    }
    /** @return The installed activation callback, or null. */
    auto GetActionCallback() const {
        return pfnAction;
    }
    /**
     * Install an idle callback without invoking it.
     * @param callback The callback receiving this widget, or null; the return value is discarded.
     */
    void SetIdleActionCallback(int (*callback)(MenuWidget *)) {
        pfnIdleAction = callback;
    }
    /**
     * Install a text-edit completion callback without invoking it.
     * @param callback The callback receiving this widget, or null; the return value is discarded.
     */
    void SetTextEditCompleteCallback(int (*callback)(MenuWidget *)) {
        pfnTextEditComplete = callback;
    }
    /** @return The installed text-edit completion callback, or null. */
    auto GetTextEditCompleteCallback() const {
        return pfnTextEditComplete;
    }
    /**
     * Install a callback run by the panel before the widget update.
     * @param callback The callback receiving this widget, or null; zero aborts panel processing.
     */
    void SetBeforeUpdateCallback(int (*callback)(MenuWidget *)) {
        pfnBeforeUpdate = callback;
    }
    /** @return The installed callback for panel processing before widget updates, or null. */
    auto GetBeforeUpdateCallback() const {
        return pfnBeforeUpdate;
    }
    /**
     * Install a callback run by the panel after the widget update.
     * @param callback The callback receiving this widget, or null; zero aborts panel processing.
     */
    void SetAfterUpdateCallback(int (*callback)(MenuWidget *)) {
        pfnAfterUpdate = callback;
    }
    /** @return The installed callback for panel processing after widget updates, or null. */
    auto GetAfterUpdateCallback() const {
        return pfnAfterUpdate;
    }

private:
    inline MenuWidget *LinkedMenuWidget(int index);
    inline bool UpdateMenuSlideAxis(int enabled,
                                    signed char direction,
                                    const float &saved,
                                    const float &target,
                                    float &current);

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
    const RECT *pImageRects;
};

/** Callbacks for PC music-track selection and persistent music and sound-effect volume. */
/** Callbacks for volume, CD track selection, and navigation in the PC audio-options menu. */
class AudioOptionsUI {
public:
    /**
     * Update the volume-channel image and its first animation frame.
     *
     * @param pWidget The non-null borrowed channel-image widget to update.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003e950
     * @ghidraAddress BRGlide: 0x10037f70
     */
    static int UpdateVolumeChannelImage(MenuWidget *pWidget);

    /**
     * Position the volume indicator for the selected music or sound-effect channel.
     * @param pWidget The non-null borrowed indicator widget to reposition.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003ea40
     * @ghidraAddress BRGlide: 0x10038060
     */
    static int UpdateVolumeLevelIndicatorPos(MenuWidget *pWidget);

    /**
     * Draw and process the CD track list, accepting nonnegative activated-row results.
     * @param pWidget The non-null borrowed widget with the track list.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003ee20
     * @ghidraAddress BRGlide: 0x10038350
     */
    static int UpdateCdTrackSelection(MenuWidget *pWidget);

    /**
     * Select the CD track two greater than the activated list index.
     * @param pList The unused list pointer, possibly null.
     * @param pTrack A non-null borrowed selected-row pointer, read without modification.
     * @return One.
     * @ghidraAddress BRD3D: 0x1004e810
     * @ghidraAddress BRGlide: 0x100476c0
     */
    static int OnMusicTrackChanged(MenuItemList *pList, int *pTrack);

    /**
     * Select the sound-effect volume channel, cycle its 0..9 level using mouse edges, and apply it.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x10042cf0
     * @ghidraAddress BRGlide: 0x1003c240
     */
    static int OnCycleSfxVolume(MenuActionWidget *pWidget);

    /**
     * Select the music volume channel, cycle its 0..9 level using mouse edges, and apply it.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x10042d60
     * @ghidraAddress BRGlide: 0x1003c2b0
     */
    static int OnCycleMusicVolume(MenuActionWidget *pWidget);

    /**
     * Destroy the audio-options screen, return to cached Options, back up choices, and save config.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x100466c0
     * @ghidraAddress BRGlide: 0x1003fb10
     */
    static int OnBackAndSave(MenuActionWidget *pItem);
};

/** Callbacks for car, handling, equipment, and navigation choices in the PC car-setup menus. */
class CarSetupUI {
public:
    /**
     * Stop the setup screen, back up choices, save config, and attempt an automatic season save.
     * Championship mode also selects three laps.
     * @param pWidget A non-null borrowed widget of the setup screen.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10043760
     * @ghidraAddress BRGlide: 0x1003ccb0
     */
    static int OnAcceptAndSave(MenuActionWidget *pWidget);

    /**
     * Position the steering indicator for the selected handling choice.
     *
     * @param pWidget The non-null borrowed steering-indicator widget to reposition.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003e920
     * @ghidraAddress BRGlide: 0x10037f40
     */
    static int UpdateSteeringIndicatorPos(MenuWidget *pWidget);

    /**
     * Cycle the handling menu index through 0..7 using mouse edges and update the chosen handling.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x10042c80
     * @ghidraAddress BRGlide: 0x1003c1d0
     */
    static int OnCycleHandlingType(MenuWidget *pWidget);

    /**
     * Cycle the tyre menu index through 0..2 using mouse edges and resolve the tyre type.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x10042dc0
     * @ghidraAddress BRGlide: 0x1003c310
     */
    static int OnCycleTireType(MenuWidget *pWidget);

    /**
     * Cycle the suspension menu index through 0..2 using mouse edges and resolve its type.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x10042e20
     * @ghidraAddress BRGlide: 0x1003c370
     */
    static int OnCycleSuspensionType(MenuWidget *pWidget);

    /**
     * Cycle the transmission menu index through 0..1 using mouse edges and resolve its type.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x10042e80
     * @ghidraAddress BRGlide: 0x1003c3d0
     */
    static int OnCycleTransmissionType(MenuWidget *pWidget);

    /**
     * Select the sprite for the current transmission choice.
     * @param pWidget The non-null borrowed transmission-image widget.
     * @return One.
     * @ghidraAddress BRD3D: 0x10040870
     * @ghidraAddress BRGlide: 0x10039db0
     */
    static int UpdateTransmissionImage(MenuWidget *pWidget);

    /**
     * Select the sprite for the current tyre choice.
     * @param pWidget The non-null borrowed tyre-image widget.
     * @return One.
     * @ghidraAddress BRD3D: 0x10040890
     * @ghidraAddress BRGlide: 0x10039dd0
     */
    static int UpdateTiresImage(MenuWidget *pWidget);

    /**
     * Select the sprite for the current suspension choice.
     * @param pWidget The non-null borrowed suspension-image widget.
     * @return One.
     * @ghidraAddress BRD3D: 0x100408b0
     * @ghidraAddress BRGlide: 0x10039df0
     */
    static int UpdateShocksImage(MenuWidget *pWidget);

    /**
     * Cycle selectable cars using mouse edges and resolve the chosen car identifier.
     * An active DirectPlay interface also receives a selection notice.
     * @param pItem The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x10042b30
     * @ghidraAddress BRGlide: 0x1003c080
     */
    static int OnCycleCar(MenuActionWidget *pItem);

    /**
     * Refresh the car-name label, or the placeholder before a ghost selection is loaded, and
     * process active text editing.
     * @param pWidget The non-null borrowed label widget with writable font slot zero.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003f760
     * @ghidraAddress BRGlide: 0x10038ca0
     */
    static int UpdateCarNameLabel(MenuWidget *pWidget);

    /**
     * Refresh the primary transmission label and process active text editing.
     * @param pWidget The non-null borrowed label widget with writable font slot zero.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003f860
     * @ghidraAddress BRGlide: 0x10038da0
     */
    static int UpdateTransmissionLabel(MenuWidget *pWidget);

    /**
     * Refresh the primary label for the current suspension choice and process active text editing.
     * @param pWidget The non-null borrowed label widget with writable font slot zero.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003f990
     * @ghidraAddress BRGlide: 0x10038ed0
     */
    static int UpdateShocksLabel(MenuWidget *pWidget);

    /**
     * Refresh the primary label for the current tyre choice and process active text editing.
     * @param pWidget The non-null borrowed label widget with writable font slot zero.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003f7f0
     * @ghidraAddress BRGlide: 0x10038d30
     */
    static int UpdateTiresLabel(MenuWidget *pWidget);

    /**
     * Select the current car sprite unless the ghost-load screen has no loaded selection.
     * @param pWidget The non-null borrowed car-image widget.
     * @return -2 before a ghost selection to skip the remaining widget update, or one after
     * updating the sprite.
     * @ghidraAddress BRD3D: 0x1003f720
     * @ghidraAddress BRGlide: 0x10038c60
     */
    static int UpdateCarImage(MenuWidget *pWidget);

    /**
     * Destroy the active setup screen, clear its cache, and restore the cached main menu.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x100464e0
     * @ghidraAddress BRGlide: 0x1003f980
     */
    static int OnBackToRaceSetup(MenuActionWidget *pItem);

    /**
     * Open Options and install its return callback for car setup.
     * @param pItem The unused widget pointer forwarded to ShowOptionsMenu(), possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x100458c0
     * @ghidraAddress BRGlide: 0x1003ed50
     */
    static int OnOpenOptions(MenuActionWidget *pItem);

    /**
     * Open season saving and install its return callback for car setup.
     * @param pItem The unused widget pointer forwarded to ShowSaveSeasonScreen(), possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x100458e0
     * @ghidraAddress BRGlide: 0x1003ed70
     */
    static int OnOpenSaveSeason(MenuActionWidget *pItem);

    /**
     * Destroy the active setup screen, clear setup references, and restore season progress.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046cd0
     * @ghidraAddress BRGlide: 0x10040120
     */
    static int OnBackToSeasonProgressInfo(MenuActionWidget *pItem);

    /**
     * Destroy the active setup screen, clear setup references, and restore cached Time Attack.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046d20
     * @ghidraAddress BRGlide: 0x10040170
     */
    static int OnBackToTimeAttack(MenuActionWidget *pItem);
};

/** Callbacks for starting a championship season and returning to the root menu. */
/** New-season and return callbacks for the championship menu. */
class ChampionshipSeasonUI {
public:
    /**
     * Reset championship choices and open pre-race information for a new season.
     *
     * @param pWidget The unused widget forwarded to pre-race setup, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x10045aa0
     * @ghidraAddress BRGlide: 0x1003ef40
     */
    static int OnStartNewSeason(MenuWidget *pWidget);

    /**
     * Destroy the active season menu, clear its cache, and return to the root screen.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046c90
     * @ghidraAddress BRGlide: 0x100400e0
     */
    static int OnBack(MenuActionWidget *pItem);
};

/** Text-entry and network-provider presentation callbacks for the connection menu. */
class ConnectionUI {
public:
    /**
     * Begin text editing and toggle the primary caret when no menu text edit is active.
     * @param pWidget A non-null borrowed widget with primary text storage.
     * @return One, including when editing was already active.
     * @ghidraAddress BRD3D: 0x10042ac0
     * @ghidraAddress BRGlide: 0x1003c020
     */
    static int OnToggleTextEdit(MenuWidget *pWidget);

    /**
     * Select the sprite for the current network service provider.
     * @param pWidget The non-null borrowed provider-image widget.
     * @return One.
     * @ghidraAddress BRD3D: 0x10040930
     * @ghidraAddress BRGlide: 0x10039e70
     */
    static int UpdateServiceProviderImage(MenuWidget *pWidget);

    /**
     * Refresh and measure the provider label, then process its text-input update.
     * @param pWidget The non-null borrowed label widget with primary text storage.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003fc40
     * @ghidraAddress BRGlide: 0x10039180
     */
    static int UpdateServiceProviderLabel(MenuWidget *pWidget);
};

/** Text-entry and return callbacks for creating a network game. */
class CreateGameMenuUI {
public:
    /**
     * Begin text editing and toggle the primary caret when no menu text edit is active.
     * @param pWidget A non-null borrowed widget with primary text storage.
     * @return One, including when editing was already active.
     * @ghidraAddress BRD3D: 0x10042b00
     * @ghidraAddress BRGlide: 0x1003c050
     */
    static int OnToggleTextEdit(MenuWidget *pWidget);

    /**
     * Destroy game creation, restore session selection, and enable joining an existing session.
     * Local non-lobby providers are disconnected and reinitialised for session discovery.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x100444c0
     * @ghidraAddress BRGlide: 0x1003da10
     */
    static int OnBack(MenuActionWidget *pItem);
};

/** State-update callbacks for the network game-creation screen. */
class CreateGameUI {
public:
    /**
     * Enable game creation after text has been entered.
     *
     * @param pWidget A non-null borrowed name-entry widget with terminated primary text.
     * @return One; empty text preserves the Continue widget's previous enabled state.
     * @ghidraAddress BRD3D: 0x1003f280
     * @ghidraAddress BRGlide: 0x100387c0
     */
    static int EnableContinueIfNameEntered(MenuWidget *pWidget);
};

/** Device-list and driver-selection callbacks for the Direct3D backend. */
class D3DDeviceSelectUI {
public:
    /**
     * Draw and process the device list, updating the selected device on row activation.
     * @param pWidget A non-null borrowed widget with the device list.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003ed10
     */
    static int UpdateDeviceSelection(MenuWidget *pWidget);

    /**
     * Resolve the selected row's device GUID, update config, and return to video options.
     * Device-list payload and device lookup must be valid.
     * @param pWidget A non-null active-screen widget, invalidated by the return transition.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x1003ecb0
     */
    static int OnSetDriver(MenuWidget *pWidget);
};

/** Display-mode list and configuration callbacks for the video-options screen. */
class DisplayModeSelectUI {
public:
    /**
     * Draw and process the mode list, updating the selected mode on row activation.
     * @param pWidget A non-null borrowed widget with the display-mode list.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003edf0
     * @ghidraAddress BRGlide: 0x10038320
     */
    static int UpdateModeSelection(MenuWidget *pWidget);

    /**
     * Apply the chosen display mode to the configuration and return to the video options.
     *
     * @param pWidget A non-null active-screen widget, invalidated by the return transition.
     * Selected list payload and display-mode references must be valid.
     * @return 0.
     * @ghidraAddress BRD3D: 0x1003ed40
     * @ghidraAddress BRGlide: 0x100382d0
     */
    static int OnSetMode(MenuWidget *pWidget);
};

/** Configuration, image, and label callbacks for PC graphics and force-feedback options. */
class GameOptionsUI {
public:
    /**
     * Draw the force feedback choice, displaying off when the device lacks support.
     *
     * @param pWidget The non-null borrowed force-feedback image widget.
     * @return One.
     * @ghidraAddress BRD3D: 0x10040950
     * @ghidraAddress BRGlide: 0x10039e90
     */
    static int UpdateForceFeedbackImage(MenuWidget *pWidget);

    /**
     * Cycle the force-feedback choice using mouse edges and update its config value.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x10043590
     * @ghidraAddress BRGlide: 0x1003cae0
     */
    static int OnToggleForceFeedback(MenuWidget *pWidget);

    /**
     * Cycle the skid-mark choice using mouse edges and update its config value.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x100435f0
     * @ghidraAddress BRGlide: 0x1003cb40
     */
    static int OnToggleSkidMarks(MenuWidget *pWidget);

    /**
     * Cycle the car-shadow choice using mouse edges and update its config value.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x10043650
     * @ghidraAddress BRGlide: 0x1003cba0
     */
    static int OnToggleCarShadow(MenuWidget *pWidget);

    /**
     * Cycle the specular-lighting choice using mouse edges and update its config value.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x100436b0
     * @ghidraAddress BRGlide: 0x1003cc00
     */
    static int OnToggleSpecularLighting(MenuWidget *pWidget);

    /**
     * Select the sprite for the current car-shadow choice.
     * @param pWidget The non-null borrowed car-shadow image widget.
     * @return One.
     * @ghidraAddress BRD3D: 0x100409b0
     * @ghidraAddress BRGlide: 0x10039ef0
     */
    static int UpdateCarShadowImage(MenuWidget *pWidget);

    /**
     * Select the sprite for the current specular-lighting choice.
     * @param pWidget The non-null borrowed specular-lighting image widget.
     * @return One.
     * @ghidraAddress BRD3D: 0x100409d0
     * @ghidraAddress BRGlide: 0x10039f10
     */
    static int UpdateSpecularLightingImage(MenuWidget *pWidget);

    /**
     * Select the sprite for the current skid-mark choice.
     * @param pWidget The non-null borrowed skid-mark image widget.
     * @return One.
     * @ghidraAddress BRD3D: 0x10040990
     * @ghidraAddress BRGlide: 0x10039ed0
     */
    static int UpdateSkidMarksImage(MenuWidget *pWidget);

    /**
     * Refresh and measure the car-shadow label, then process its text-input update.
     * @param pWidget The non-null borrowed label widget with primary text storage.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003fda0
     * @ghidraAddress BRGlide: 0x100392e0
     */
    static int UpdateCarShadowLabel(MenuWidget *pWidget);

    /**
     * Refresh and measure the skid-mark label, then process its text-input update.
     * @param pWidget The non-null borrowed label widget with primary text storage.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003fd30
     * @ghidraAddress BRGlide: 0x10039270
     */
    static int UpdateSkidMarksLabel(MenuWidget *pWidget);

    /**
     * Refresh and measure the specular-lighting label, then process its text-input update.
     * @param pWidget The non-null borrowed label widget with primary text storage.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003fe10
     * @ghidraAddress BRGlide: 0x10039350
     */
    static int UpdateSpecularLightingLabel(MenuWidget *pWidget);

    /**
     * Refresh the force-feedback label, displaying Off when feedback is unavailable.
     * The primary font is measured and its text-input update is processed.
     * @param pWidget The non-null borrowed label widget with primary text storage.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003fcb0
     * @ghidraAddress BRGlide: 0x100391f0
     */
    static int UpdateForceFeedbackLabel(MenuWidget *pWidget);

    /**
     * Destroy the active game-options screen, clear its cache, and return to cached Options.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046710
     * @ghidraAddress BRGlide: 0x1003fb60
     */
    static int OnBack(MenuActionWidget *pItem);
};

/** Input-device selection, default bindings, and label callbacks for PC controls. */
class InputOptionsUI {
public:
    /**
     * Reload default bindings when the selected device index is in 0..3.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One, including for an out-of-range device index.
     * @ghidraAddress BRD3D: 0x100406c0
     * @ghidraAddress BRGlide: 0x10039c00
     */
    static int OnRestoreDefaults(MenuWidget *pWidget);

    /**
     * Cycle among device indices 0, 2, and 3 using mouse edges, then select and back up bindings.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x10043400
     * @ghidraAddress BRGlide: 0x1003c950
     */
    static int OnCycleInputDevice(MenuWidget *pWidget);

    /**
     * Select the sprite for the current input device.
     * @param pWidget The non-null borrowed input-device image widget.
     * @return One.
     * @ghidraAddress BRD3D: 0x10041870
     * @ghidraAddress BRGlide: 0x1003add0
     */
    static int UpdateInputDeviceImage(MenuWidget *pWidget);

    /**
     * Refresh and measure the input-device label, then process its text-input update.
     * @param pWidget The non-null borrowed label widget with primary text storage.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003ffd0
     * @ghidraAddress BRGlide: 0x10039510
     */
    static int UpdateInputDeviceLabel(MenuWidget *pWidget);

    /**
     * Destroy the active input-options screen, clear its cache, and return to cached Options.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x100465a0
     * @ghidraAddress BRGlide: 0x1003fa40
     */
    static int OnBack(MenuActionWidget *pItem);
};

/** Session-list and return callbacks for joining or creating a network game. */
class JoinCreateGameUI {
public:
    /**
     * Draw and process the session list, updating the selected session on row activation.
     * @param pWidget A non-null borrowed widget with the session list.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003eb90
     * @ghidraAddress BRGlide: 0x10038180
     */
    static int UpdateSessionSelection(MenuWidget *pWidget);

    /**
     * Destroy session selection, clear join/create references, restore connection, and disconnect.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10044ae0
     * @ghidraAddress BRGlide: 0x1003e030
     */
    static int OnBack(MenuActionWidget *pItem);
};

/** Action selection, key capture, and binding-warning callbacks for PC input configuration. */
class KeyConfigUI {
public:
    /**
     * Draw and process the action list, updating the selected action on row activation.
     * @param pWidget A non-null borrowed widget with the action list.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003ec80
     * @ghidraAddress BRGlide: 0x100382a0
     */
    static int UpdateActionSelection(MenuWidget *pWidget);

    /**
     * Refresh the binding label or capture prompt, measure it, and process its text-input update.
     * Invalid device indices preserve prior label text.
     * @param pWidget A non-null borrowed label widget with primary text storage.
     * @return One.
     * @ghidraAddress BRD3D: 0x100400e0
     * @ghidraAddress BRGlide: 0x10039620
     */
    static int UpdateBindingLabel(MenuWidget *pWidget);

    /**
     * Update the duplicate-binding warning and highlight an affected selected action.
     * The primary font is measured and its text-input update is processed.
     * @param pWidget A non-null borrowed warning widget with primary text storage.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003f8d0
     * @ghidraAddress BRGlide: 0x10038e10
     */
    static int UpdateDuplicateBindingWarning(MenuWidget *pWidget);

    /**
     * Poll keyboard, joystick, or mouse capture, update the selected binding, and detect
     * duplicates.
     * A pending release marker ends capture when no keyboard code is detected. Mouse captures
     * first wait for all raw mouse buttons to be released.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One, including while capture is inactive or the selected device is invalid.
     * @ghidraAddress BRD3D: 0x10040450
     * @ghidraAddress BRGlide: 0x10039990
     */
    static int PollKeyCapture(MenuWidget *pWidget);

    /**
     * Enable key capture, clear pending character input, and reset keyboard edge state if needed.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One, including when capture is already active.
     * @ghidraAddress BRD3D: 0x10040680
     * @ghidraAddress BRGlide: 0x10039bc0
     */
    static int BeginKeyCapture(MenuWidget *pWidget);

    /**
     * Destroy key configuration, return to cached input options, and release the joystick device.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046560
     * @ghidraAddress BRGlide: 0x1003fa00
     */
    static int OnBack(MenuActionWidget *pItem);
};

/** Saved-file list selection callbacks. */
class LoadFileListUI {
public:
    /**
     * Draw and process the save-slot list, updating the selected slot on row activation.
     * @param pWidget A non-null borrowed widget with the save-slot list.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003eae0
     * @ghidraAddress BRGlide: 0x10038220
     */
    static int UpdateSlotSelection(MenuWidget *pWidget);
};

/** Championship save loading, continuation, and return callbacks. */
class LoadSeasonUI {
public:
    /**
     * Enable continuing after a championship slot has been opened.
     * Otherwise disable the widget and select its fallback sprite and font style.
     *
     * @param pWidget A non-null borrowed Continue widget whose enabled state is updated.
     * @return One.
     * @ghidraAddress BRD3D: 0x10041890
     * @ghidraAddress BRGlide: 0x1003adf0
     */
    static int UpdateContinueButtonState(MenuWidget *pWidget);

    /**
     * Attempt to load the automatic season save and open pre-race information in championship mode.
     * Install the return callback to the season-loading menu.
     * @param pWidget The unused widget forwarded to pre-race setup, possibly null.
     * @return One without reporting the automatic-load result.
     * @ghidraAddress BRD3D: 0x100450c0
     * @ghidraAddress BRGlide: 0x1003e610
     */
    static int OnLoadAutoSave(MenuWidget *pWidget);

    /**
     * Open pre-race information for the loaded season and install the return callback to loading.
     * @param pItem The unused widget forwarded to pre-race setup, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x10045090
     * @ghidraAddress BRGlide: 0x1003e5e0
     */
    static int OnContinueSeason(MenuActionWidget *pItem);

    /**
     * Destroy season loading, clear its selection and labels, and restore the cached season menu.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046e10
     * @ghidraAddress BRGlide: 0x10040260
     */
    static int OnBack(MenuActionWidget *pItem);
};

/** Saved time-attack selection, car setup, and return callbacks. */
class LoadTimeAttackMenuUI {
public:
    /**
     * Enable a time attack item after a saved slot has been selected.
     * Otherwise disable the widget and select its fallback sprite and font style.
     *
     * @param pWidget A non-null borrowed widget whose enabled state is updated.
     * @return One.
     * @ghidraAddress BRD3D: 0x100418f0
     * @ghidraAddress BRGlide: 0x1003ae50
     */
    static int UpdateItemEnabledState(MenuWidget *pWidget);

    /**
     * Open car setup for the loaded time attack and install its return callback.
     * @param pWidget The unused widget forwarded to ShowCarSetupScreen(), possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x10046380
     * @ghidraAddress BRGlide: 0x1003f820
     */
    static int OnCarSetup(MenuWidget *pWidget);

    /**
     * Load a numbered TimeAttack slot, restore its race selections and profile, and synchronise
     * progress.
     * @param pList The unused list pointer, possibly null.
     * @param pnSlot A non-null readable slot index, normally 0..99, used in the saved filename.
     * @return Zero when the loaded ghost reports an invalid track; one after restoring selections.
     * @ghidraAddress BRD3D: 0x10042880
     * @ghidraAddress BRGlide: 0x1003bde0
     */
    static int OnSlotSelected(MenuItemList *pList, int *pnSlot);

    /**
     * Destroy ghost loading, clear its cache, and restore the cached Time Attack menu.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10044de0
     * @ghidraAddress BRGlide: 0x1003e330
     */
    static int OnBack(MenuActionWidget *pItem);
};

/** Root-menu callbacks for entering the credits sequence. */
class MainMenuUI {
public:
    /**
     * Select the credits or cheat-enabled outro session and stop the front end for playback.
     * @param pWidget A non-null borrowed widget of the active screen to stop.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10041970
     * @ghidraAddress BRGlide: 0x1003aed0
     */
    static int OnPlayCredits(MenuWidget *pWidget);
};

/** Modem-number entry, provider selection, and connection-screen return callbacks. */
class ModemConnectionUI {
public:
    /**
     * Select the modem provider and restore the cached join or create screen.
     *
     * @param pWidget A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10043f50
     * @ghidraAddress BRGlide: 0x1003d4a0
     */
    static int OnAccept(MenuWidget *pWidget);

    /**
     * Enable continuing after a modem number has been entered.
     *
     * @param pWidget A non-null borrowed number-entry widget with terminated primary text.
     * @return One; empty text preserves the Continue widget's previous enabled state.
     * @ghidraAddress BRD3D: 0x1003f020
     * @ghidraAddress BRGlide: 0x10038550
     */
    static int EnableContinueIfNumberEntered(MenuWidget *pWidget);

    /**
     * Destroy modem entry, clear its references, and restore the cached connection screen.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10044b40
     * @ghidraAddress BRGlide: 0x1003e090
     */
    static int OnBack(MenuActionWidget *pItem);
};

/** Provider selection, hover animation, and return callbacks for multiplayer connections. */
class MultiplayerConnectionUI {
public:
    /**
     * Select IPX and attempt to open the session join/create screen.
     * @param pWidget The unused widget forwarded to ShowJoinCreateGameScreen(), possibly null.
     * @return One without propagating the screen-opening result.
     * @ghidraAddress BRD3D: 0x10044010
     * @ghidraAddress BRGlide: 0x1003d560
     */
    static int OnSelectIpx(MenuWidget *pWidget);

    /**
     * Select IPX and update the provider widget's hover and animation appearance.
     * @param pWidget A non-null borrowed provider widget to validate.
     * @return One.
     * @ghidraAddress BRD3D: 0x10044030
     * @ghidraAddress BRGlide: 0x1003d580
     */
    static int UpdateIpxItem(MenuWidget *pWidget);

    /**
     * Select TCP/IP and attempt to open the session join/create screen.
     * @param pWidget The unused widget forwarded to ShowJoinCreateGameScreen(), possibly null.
     * @return One without propagating the screen-opening result.
     * @ghidraAddress BRD3D: 0x10044050
     * @ghidraAddress BRGlide: 0x1003d5a0
     */
    static int OnSelectTcpIp(MenuWidget *pWidget);

    /**
     * Select TCP/IP and update the provider widget's hover and animation appearance.
     * @param pWidget A non-null borrowed provider widget to validate.
     * @return One.
     * @ghidraAddress BRD3D: 0x10044070
     * @ghidraAddress BRGlide: 0x1003d5c0
     */
    static int UpdateTcpIpItem(MenuWidget *pWidget);

    /**
     * Select the modem provider and attempt to open the session join/create screen.
     * @param pWidget The unused widget forwarded to ShowJoinCreateGameScreen(), possibly null.
     * @return One without propagating the screen-opening result.
     * @ghidraAddress BRD3D: 0x10044090
     * @ghidraAddress BRGlide: 0x1003d5e0
     */
    static int OnSelectModem(MenuWidget *pWidget);

    /**
     * Select the modem provider and update its widget's hover and animation appearance.
     * @param pWidget A non-null borrowed provider widget to validate.
     * @return One.
     * @ghidraAddress BRD3D: 0x100440b0
     * @ghidraAddress BRGlide: 0x1003d600
     */
    static int UpdateModemItem(MenuWidget *pWidget);

    /**
     * Destroy provider selection, clear its cache, and restore the cached multiplayer-name screen.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x100463c0
     * @ghidraAddress BRGlide: 0x1003f860
     */
    static int OnBack(MenuActionWidget *pItem);
};

/** Multiplayer race-setup callbacks using the shared session and cached connection screens. */
/** Car-class selection and host/client exit callbacks for multiplayer setup. */
class MultiplayerGameSetupUI {
public:
    /**
     * Exit an active network game launched through a lobby.
     *
     * @param pWidget A non-null borrowed widget of the active screen to stop before exit.
     * @return One when application shutdown is not requested.
     * @ghidraAddress BRD3D: 0x100437d0
     * @ghidraAddress BRGlide: 0x1003cd20
     */
    static int OnEscape(MenuWidget *pWidget);

    /**
     * Cycle car classes 0..4 using mouse edges and apply the selected car-group preset.
     * An active DirectPlay interface also receives a selection notice.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x10044600
     * @ghidraAddress BRGlide: 0x1003db50
     */
    static int OnCycleCarClass(MenuWidget *pWidget);

    /**
     * Return a client to session selection, disconnect, and restore provider discovery when needed.
     * Lobby-launched sessions exit the application instead.
     * @param pItem A non-null active-screen widget, invalidated by screen destruction or shutdown.
     * @return Zero when control returns, ending the current widget update.
     * @ghidraAddress BRD3D: 0x10044970
     * @ghidraAddress BRGlide: 0x1003dec0
     */
    static int OnBackAsClient(MenuActionWidget *pItem);

    /**
     * Return a host to game creation, disconnect, and restore provider discovery when needed.
     * Lobby-launched sessions exit the application instead.
     * @param pItem A non-null active-screen widget, invalidated by screen destruction or shutdown.
     * @return Zero when control returns, ending the current widget update.
     * @ghidraAddress BRD3D: 0x10044a30
     * @ghidraAddress BRGlide: 0x1003df80
     */
    static int OnBackAsHost(MenuActionWidget *pItem);
};

/** Sprite-update callbacks for multiplayer car-group choices. */
class MultiplayerSetupUI {
public:
    /**
     * Select the car-group sprite, using the default image for groups outside 1..4.
     * @param pWidget A non-null borrowed car-group image widget.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003f5e0
     * @ghidraAddress BRGlide: 0x10038b20
     */
    static int UpdateCarGroupImage(MenuWidget *pWidget);

    /**
     * Select the secondary car-group sprite, using -1 for groups outside 1..4.
     * @param pWidget A non-null borrowed secondary-image widget.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003f680
     * @ghidraAddress BRGlide: 0x10038bc0
     */
    static int UpdateCarGroupSubImage(MenuWidget *pWidget);
};

/**
 * Network car-setup callbacks for chat, player selection, readiness, and session transitions.
 * Shared DirectPlay state and required menu references must be initialised before use.
 */
class NetworkCarSetupUI {
public:
    /**
     * Draw and process the chat list without storing a newly activated row index.
     * @param pWidget A non-null borrowed widget with the chat list.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003ebc0
     * @ghidraAddress BRGlide: 0x100381b0
     */
    static int PollChatListSelection(MenuWidget *pWidget);

    /**
     * Copy primary text into the shared chat buffer, send it, and clear both text buffers.
     * @param pWidget A non-null borrowed entry widget with terminated primary text of at most
     * 200 bytes, copied into the 201-byte chat buffer without a capacity check.
     * @return One without propagating the chat-send result.
     * @ghidraAddress BRD3D: 0x1003f170
     * @ghidraAddress BRGlide: 0x100386b0
     */
    static int SendChatMessage(MenuWidget *pWidget);

    /**
     * Send a removal request for the selected player unless the selection is the local player.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x100437b0
     * @ghidraAddress BRGlide: 0x1003cd00
     */
    static int OnBootPlayer(MenuWidget *pWidget);

    /**
     * Query session readiness and send a start request for a ready host or toggle client readiness.
     * Missing peers and unready host selections produce chat feedback.
     * Session-query and network-send failures are not propagated to the caller.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One, including when no session description is available.
     * @ghidraAddress BRD3D: 0x10043a00
     * @ghidraAddress BRGlide: 0x1003cf50
     */
    static int OnRefreshSessionDesc(MenuWidget *pWidget);

    /**
     * Process network escape, lobby return, host readiness, or the pending race-start transition.
     * Lobby-launched active streams exit the application; accepted race starts save menu choices.
     * @param pWidget A non-null active-screen widget, potentially invalidated by a menu transition.
     * @return Zero after a menu or race transition; one when continuing the current setup screen.
     * @ghidraAddress BRD3D: 0x10043810
     * @ghidraAddress BRGlide: 0x1003cd60
     */
    static int OnEscape(MenuWidget *pWidget);

    /**
     * Process player-list selection and read the selected row's application data as its player ID.
     * @param pWidget A non-null borrowed player-list widget with a valid selected row index.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003ebe0
     * @ghidraAddress BRGlide: 0x100381d0
     */
    static int UpdateSelectedPlayer(MenuWidget *pWidget);
};

/** Return callbacks that destroy the active Options screen and restore a cached destination. */
class OptionsMenuUI {
public:
    /**
     * Destroy Options, clear its cache, and restore the root menu.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046520
     * @ghidraAddress BRGlide: 0x1003f9c0
     */
    static int OnBack(MenuActionWidget *pItem);

    /**
     * Destroy Options, clear its cache, and restore cached car setup.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046bb0
     * @ghidraAddress BRGlide: 0x10040000
     */
    static int OnBackToCarSetup(MenuActionWidget *pItem);

    /**
     * Destroy Options, clear its cache, and restore cached pre-race information.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046750
     * @ghidraAddress BRGlide: 0x1003fba0
     */
    static int OnBackToPreRaceInfo(MenuActionWidget *pItem);

    /**
     * Destroy Options, clear its cache, and restore cached season progress.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046830
     * @ghidraAddress BRGlide: 0x1003fc80
     */
    static int OnBackToSeasonProgressInfo(MenuActionWidget *pItem);

    /**
     * Destroy Options, clear its cache, and restore cached race-results information.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046910
     * @ghidraAddress BRGlide: 0x1003fd60
     */
    static int OnBackToRaceResultsInfo(MenuActionWidget *pItem);

    /**
     * Destroy Options, clear its cache, and restore the cached race-results summary.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x100469f0
     * @ghidraAddress BRGlide: 0x1003fe40
     */
    static int OnBackToRaceResultsSummary(MenuActionWidget *pItem);

    /**
     * Destroy Options, clear its cache, and restore cached race-results totals.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046ad0
     * @ghidraAddress BRGlide: 0x1003ff20
     */
    static int OnBackToRaceResultsTotals(MenuActionWidget *pItem);
};

/** Drawing callbacks for the overwrite-confirmation prompt. */
class OverwritePromptUI {
public:
    /**
     * Draw a segmented prompt surround sized from the primary font's measured width.
     * @param pWidget A non-null borrowed prompt widget with a nonnegative measured text width.
     * @return One without propagating underlying sprite-blit results.
     * @ghidraAddress BRD3D: 0x1003e7a0
     * @ghidraAddress BRGlide: 0x10037dc0
     */
    static int DrawPromptBox(MenuWidget *pWidget);
};

/** Player-name editing, continuation, and root-menu return callbacks. */
class PlayerNameEntryUI {
public:
    /**
     * Enable continuing after a player name has been entered.
     *
     * @param pWidget A non-null borrowed name-entry widget with terminated primary text.
     * @return One; empty text preserves the Continue widget's previous enabled state.
     * @ghidraAddress BRD3D: 0x1003ef60
     * @ghidraAddress BRGlide: 0x10038490
     */
    static int EnableContinueIfNameEntered(MenuWidget *pWidget);

    /**
     * Begin text editing and toggle the primary caret when no menu text edit is active.
     * @param pWidget A non-null borrowed widget with primary text storage.
     * @return One, including when editing was already active.
     * @ghidraAddress BRD3D: 0x10042a90
     * @ghidraAddress BRGlide: 0x1003bff0
     */
    static int OnToggleTextEdit(MenuWidget *pWidget);

    /**
     * Destroy player-name entry, clear its references, and restore the root menu.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10043fc0
     * @ghidraAddress BRGlide: 0x1003d510
     */
    static int OnBack(MenuActionWidget *pItem);
};

/** Confirmation callbacks for abandoning a championship from pre-race information. */
/** Confirmation callbacks for abandoning a championship season before a race. */
class PreRaceExitSeasonConfirmUI {
public:
    /**
     * Destroy confirmation and cached pre-race screens, clear the pre-race cache and screen
     * pointer, and restore the root menu.
     * @param pWidget A non-null confirmation widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046f60
     * @ghidraAddress BRGlide: 0x100403b0
     */
    static int OnYes(MenuWidget *pWidget);

    /**
     * Restore cached pre-race information without destroying the confirmation screen.
     * @param pWidget The unused widget pointer, possibly null.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046fc0
     * @ghidraAddress BRGlide: 0x10040410
     */
    static int OnNo(MenuWidget *pWidget);
};

/** Championship pre-race records, options, saving, and exit-confirmation callbacks. */
class PreRaceInfoUI {
public:
    /**
     * Refresh and measure the Back label according to championship and resume state.
     * @param pWidget A non-null borrowed label widget with primary text storage.
     * @return One.
     * @ghidraAddress BRD3D: 0x1003e840
     * @ghidraAddress BRGlide: 0x10037e60
     */
    static int UpdateBackButtonText(MenuWidget *pWidget);

    /**
     * Open Options and install its return callback to pre-race information.
     * @param pWidget The unused widget forwarded to ShowOptionsMenu(), possibly null.
     * @return One without propagating the screen-opening result.
     * @ghidraAddress BRD3D: 0x10045780
     * @ghidraAddress BRGlide: 0x1003ec10
     */
    static int OnOptions(MenuWidget *pWidget);

    /**
     * Open season saving and install its return callback to pre-race information.
     * @param pWidget The unused widget forwarded to ShowSaveSeasonScreen(), possibly null.
     * @return One without propagating the screen-opening result.
     * @ghidraAddress BRD3D: 0x100457a0
     * @ghidraAddress BRGlide: 0x1003ec30
     */
    static int OnSaveSeason(MenuWidget *pWidget);

    /**
     * Destroy pre-race information, clear its references, and restore cached season loading.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046dc0
     * @ghidraAddress BRGlide: 0x10040210
     */
    static int OnBackToLoadSeason(MenuActionWidget *pItem);

    /**
     * Destroy pre-race information, clear its references, and restore the cached season menu.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046d70
     * @ghidraAddress BRGlide: 0x100401c0
     */
    static int OnBackToChampionshipSeason(MenuActionWidget *pItem);

    /**
     * Activate the previously built pre-race exit-confirmation screen.
     * @param pItem The unused widget pointer, possibly null.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x10046f50
     * @ghidraAddress BRGlide: 0x100403a0
     */
    static int OnBack(MenuActionWidget *pItem);

    /**
     * Draw the season's best race time for the saved round's track, or a placeholder when unloaded.
     * @param pWidget A non-null borrowed label widget with primary text storage.
     * @return One after formatting and drawing the time.
     * @ghidraAddress BRD3D: 0x10040c00
     * @ghidraAddress BRGlide: 0x1003a140
     */
    static int UpdateBestTimeLabel(MenuWidget *pWidget);

    /**
     * Draw the season's best lap time for the saved round's track, or a placeholder when unloaded.
     * @param pWidget A non-null borrowed label widget with primary text storage.
     * @return One after formatting and drawing the time.
     * @ghidraAddress BRD3D: 0x10040d70
     * @ghidraAddress BRGlide: 0x1003a2b0
     */
    static int UpdateBestLapLabel(MenuWidget *pWidget);
};

/** Application-exit confirmation and root-menu return callbacks. */
class QuitGameConfirmUI {
public:
    /**
     * Back up choices, save config, shut down the front end and DirectPlay, and exit the
     * application.
     * @param pWidget A non-null active-screen widget used during shutdown.
     * @return Does not return after application shutdown; the native routine has an unreachable
     * return of one.
     * @ghidraAddress BRD3D: 0x10043710
     * @ghidraAddress BRGlide: 0x1003cc60
     */
    static int OnConfirmQuit(MenuWidget *pWidget);

    /**
     * Destroy quit confirmation, clear its cache, and restore the root menu.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x100464a0
     * @ghidraAddress BRGlide: 0x1003f940
     */
    static int OnBack(MenuActionWidget *pItem);
};

/** Lap selection, timing labels, and submenu callbacks for race-results information. */
class RaceResultsInfoUI {
public:
    /**
     * Select the first lap's recorded time for results display.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x100412c0
     * @ghidraAddress BRGlide: 0x1003a820
     */
    static int OnSelectLap1(MenuWidget *pWidget);

    /**
     * Select the second lap's recorded time for results display.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x100412d0
     * @ghidraAddress BRGlide: 0x1003a830
     */
    static int OnSelectLap2(MenuWidget *pWidget);

    /**
     * Select the third lap's recorded time for results display.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x100412e0
     * @ghidraAddress BRGlide: 0x1003a840
     */
    static int OnSelectLap3(MenuWidget *pWidget);

    /**
     * Select the complete race time for results display.
     * @param pWidget The unused widget pointer, possibly null.
     * @return One.
     * @ghidraAddress BRD3D: 0x100412f0
     * @ghidraAddress BRGlide: 0x1003a850
     */
    static int OnSelectTotalTime(MenuWidget *pWidget);

    /**
     * Open Options and install its return callback to race-results information.
     * @param pItem The unused widget forwarded to ShowOptionsMenu(), possibly null.
     * @return One without propagating the screen-opening result.
     * @ghidraAddress BRD3D: 0x10045800
     * @ghidraAddress BRGlide: 0x1003ec90
     */
    static int OnOpenOptions(MenuActionWidget *pItem);

    /**
     * Open season saving and install its return callback to race-results information.
     * @param pItem The unused widget forwarded to ShowSaveSeasonScreen(), possibly null.
     * @return One without propagating the screen-opening result.
     * @ghidraAddress BRD3D: 0x10045820
     * @ghidraAddress BRGlide: 0x1003ecb0
     */
    static int OnOpenSaveSeason(MenuActionWidget *pItem);

    /**
     * Format and draw the selected lap or complete race time using the primary font.
     * @param pWidget A non-null borrowed label widget; the selected lap must be in 0..3.
     * @return One after drawing a time or the missing-time placeholder.
     * @ghidraAddress BRD3D: 0x10040ee0
     * @ghidraAddress BRGlide: 0x1003a420
     */
    static int UpdateSelectedTimeLabel(MenuWidget *pWidget);
};

/** Submenu and return callbacks for the championship race-results summary. */
class RaceResultsSummaryUI {
public:
    /**
     * Open Options and install its return callback to the results summary.
     * @param pItem The unused widget forwarded to ShowOptionsMenu(), possibly null.
     * @return One without propagating the screen-opening result.
     * @ghidraAddress BRD3D: 0x10045840
     * @ghidraAddress BRGlide: 0x1003ecd0
     */
    static int OnOpenOptions(MenuActionWidget *pItem);

    /**
     * Open season saving and install its return callback to the results summary.
     * @param pItem The unused widget forwarded to ShowSaveSeasonScreen(), possibly null.
     * @return One without propagating the screen-opening result.
     * @ghidraAddress BRD3D: 0x10045860
     * @ghidraAddress BRGlide: 0x1003ecf0
     */
    static int OnOpenSaveSeason(MenuActionWidget *pItem);

    /**
     * Destroy the results summary, clear its cache, and restore cached race-results information.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x100470a0
     * @ghidraAddress BRGlide: 0x100404f0
     */
    static int OnBack(MenuActionWidget *pItem);
};

/** Season points, progress labels, submenu callbacks, and continuation from results totals. */
class RaceResultsTotalsUI {
public:
    /**
     * Draw the total points of the race results.
     *
     * @param pWidget A non-null borrowed label widget with primary text storage.
     * @return One after formatting and drawing the numeric label.
     * @ghidraAddress BRD3D: 0x10041710
     * @ghidraAddress BRGlide: 0x1003ac70
     */
    static int UpdateTotalPointsLabel(MenuWidget *pWidget);

    /**
     * Draw the number of completed rounds in the race results.
     *
     * @param pWidget A non-null borrowed label widget with primary text storage.
     * @return One after formatting and drawing the numeric label.
     * @ghidraAddress BRD3D: 0x10041670
     * @ghidraAddress BRGlide: 0x1003abd0
     */
    static int UpdateRoundsCompletedLabel(MenuWidget *pWidget);

    /**
     * Draw the remaining points for the season stored in the resume state.
     *
     * @param pWidget A non-null borrowed label widget with primary text storage.
     * @return One after drawing the remaining points, clamped to zero when already sufficient.
     * @ghidraAddress BRD3D: 0x100417b0
     * @ghidraAddress BRGlide: 0x1003ad10
     */
    static int UpdatePointsNeededLabel(MenuWidget *pWidget);

    /**
     * Destroy results totals, clear its cache, and restore the cached results summary.
     * @param pItem A non-null active-screen widget, invalidated when its panels are destroyed.
     * @return Zero to end the current widget update.
     * @ghidraAddress BRD3D: 0x100470e0
     * @ghidraAddress BRGlide: 0x10040530
     */
    static int OnBack(MenuActionWidget *pItem);

    /**
     * Open Options and install its return callback to results totals.
     * @param pItem The unused widget forwarded to ShowOptionsMenu(), possibly null.
     * @return One without propagating the screen-opening result.
     * @ghidraAddress BRD3D: 0x10045880
     * @ghidraAddress BRGlide: 0x1003ed10
     */
    static int OnOpenOptions(MenuActionWidget *pItem);

    /**
     * Open season saving and install its return callback to results totals.
     * @param pItem The unused widget forwarded to ShowSaveSeasonScreen(), possibly null.
     * @return One without propagating the screen-opening result.
     * @ghidraAddress BRD3D: 0x100458a0
     * @ghidraAddress BRGlide: 0x1003ed30
     */
    static int OnOpenSaveSeason(MenuActionWidget *pItem);

    /**
     * Restore championship settings and points from progress, then select the next screen.
     *
     * @param pItem A non-null totals-screen widget, invalidated when its panels are destroyed.
     * @return Zero.
     * @ghidraAddress BRD3D: 0x10047290
     * @ghidraAddress BRGlide: 0x100406e0
     */
    static int OnContinue(MenuActionWidget *pItem);
};

/** Callbacks for selecting a track, weather, and lap count. */
class RaceSetupUI {
public:
    /**
     * @ghidraAddress BRD3D: 0x100408d0
     * @ghidraAddress BRGlide: 0x10039e10
     */
    static int UpdateLapCountLabel(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x10040730
     * @ghidraAddress BRGlide: 0x10039c70
     */
    static int UpdateTrackImage(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x100407e0
     * @ghidraAddress BRGlide: 0x10039d20
     */
    static int UpdateWeatherImage(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x10042ee0
     * @ghidraAddress BRGlide: 0x1003c430
     */
    static int OnCycleTrack(MenuActionWidget *pItem);

    /**
     * @ghidraAddress BRD3D: 0x100430b0
     * @ghidraAddress BRGlide: 0x1003c600
     */
    static int OnCycleLapCount(MenuActionWidget *pItem);

    /**
     * @ghidraAddress BRD3D: 0x10043180
     * @ghidraAddress BRGlide: 0x1003c6d0
     */
    static int OnCycleWeather(MenuActionWidget *pItem);

    /**
     * @ghidraAddress BRD3D: 0x1003fa00
     * @ghidraAddress BRGlide: 0x10038f40
     */
    static int UpdateTrackNameLabel(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x1003fe80
     * @ghidraAddress BRGlide: 0x100393c0
     */
    static int UpdateWeatherLabel(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x10046450
     * @ghidraAddress BRGlide: 0x1003f8f0
     */
    static int OnBack(MenuActionWidget *pItem);

    /**
     * @ghidraAddress BRD3D: 0x10044cb0
     * @ghidraAddress BRGlide: 0x1003e200
     */
    static int OnBackToTimeAttack(MenuActionWidget *pItem);
};

/** Callbacks for confirming an exit from the championship results. */
class ResultsSummaryExitSeasonConfirmUI {
public:
    /**
     * @ghidraAddress BRD3D: 0x10046fd0
     * @ghidraAddress BRGlide: 0x10040420
     */
    static int OnYes(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x10047050
     * @ghidraAddress BRGlide: 0x100404a0
     */
    static int OnNo(MenuWidget *pWidget);
};

/** Callbacks for selecting and naming a championship save slot. */
class SaveSeasonUI {
public:
    /**
     * Process championship save text confirmation or cancellation.
     *
     * @ghidraAddress BRD3D: 0x10047210
     * @ghidraAddress BRGlide: 0x10040660
     */
    static int HandleTextEntry(MenuWidget *pWidget);

    /**
     * Start naming a new championship slot or activate confirmation for an existing file.
     *
     * @ghidraAddress BRD3D: 0x10042020
     * @ghidraAddress BRGlide: 0x1003b580
     */
    static int OnSlotSelected(MenuItemList *pList, int *pnSlot);

    /**
     * @ghidraAddress BRD3D: 0x1003eb10
     * @ghidraAddress BRGlide: 0x10038100
     */
    static int UpdateSlotSelection(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x10041df0
     * @ghidraAddress BRGlide: 0x1003b350
     */
    static int OnSlotNameEntered(MenuItemList *pList, int nStatus);

    /**
     * @ghidraAddress BRD3D: 0x100450f0
     * @ghidraAddress BRGlide: 0x1003e640
     */
    static int InvokeBackHandler(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x100418d0
     * @ghidraAddress BRGlide: 0x1003ae30
     */
    static int UpdateItemEnabledState(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x10046bf0
     * @ghidraAddress BRGlide: 0x10040040
     */
    static int OnBackToCarSetup(MenuActionWidget *pItem);

    /**
     * @ghidraAddress BRD3D: 0x10046790
     * @ghidraAddress BRGlide: 0x1003fbe0
     */
    static int OnBackToPreRaceInfo(MenuActionWidget *pItem);

    /**
     * @ghidraAddress BRD3D: 0x10046870
     * @ghidraAddress BRGlide: 0x1003fcc0
     */
    static int OnBackToSeasonProgressInfo(MenuActionWidget *pItem);

    /**
     * @ghidraAddress BRD3D: 0x10046eb0
     * @ghidraAddress BRGlide: 0x10040300
     */
    static int OnBack(MenuActionWidget *pItem);

    /**
     * @ghidraAddress BRD3D: 0x10046950
     * @ghidraAddress BRGlide: 0x1003fda0
     */
    static int OnBackToRaceResultsInfo(MenuActionWidget *pItem);

    /**
     * @ghidraAddress BRD3D: 0x10046a30
     * @ghidraAddress BRGlide: 0x1003fe80
     */
    static int OnBackToRaceResultsSummary(MenuActionWidget *pItem);

    /**
     * @ghidraAddress BRD3D: 0x10046b10
     * @ghidraAddress BRGlide: 0x1003ff60
     */
    static int OnBackToRaceResultsTotals(MenuActionWidget *pItem);
};

/** Callbacks for the time attack save menu. */
class SaveTimeAttackMenuUI {
public:
    /**
     * Process time attack save text confirmation or cancellation.
     *
     * @ghidraAddress BRD3D: 0x10047250
     * @ghidraAddress BRGlide: 0x100406a0
     */
    static int HandleTextEntry(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x10042740
     * @ghidraAddress BRGlide: 0x1003bca0
     */
    static int OnSlotSelected(MenuItemList *pList, int *pnSlot);

    /**
     * @ghidraAddress BRD3D: 0x10042560
     * @ghidraAddress BRGlide: 0x1003bac0
     */
    static int OnSlotNameEntered(MenuItemList *pList, int status);

    /**
     * @ghidraAddress BRD3D: 0x10044f00
     * @ghidraAddress BRGlide: 0x1003e450
     */
    static int OnBack(MenuActionWidget *pItem);
};

/** Callbacks for the time attack save-slot screen. */
class SaveTimeAttackUI {
public:
    /**
     * @ghidraAddress BRD3D: 0x1003ec30
     * @ghidraAddress BRGlide: 0x10038250
     */
    static int UpdateSlotSelection(MenuWidget *pWidget);
};

/** Callbacks for the championship completion screen. */
class SeasonCongratsUI {
public:
    /**
     * @ghidraAddress BRD3D: 0x100471f0
     * @ghidraAddress BRGlide: 0x10040640
     */
    static int PollAnyKey(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x1003f440
     * @ghidraAddress BRGlide: 0x10038980
     */
    static int UpdatePrimaryBannerImage(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x1003f540
     * @ghidraAddress BRGlide: 0x10038a80
     */
    static int UpdateSecondaryBannerImage(MenuWidget *pWidget);
};

/** Callbacks for the selected championship file's labels. */
class SeasonFileUI {
public:
    /**
     * @ghidraAddress BRD3D: 0x10040a50
     * @ghidraAddress BRGlide: 0x10039f90
     */
    static int UpdateYearLabel(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x10040ac0
     * @ghidraAddress BRGlide: 0x1003a000
     */
    static int UpdateRoundLabel(MenuWidget *pWidget);
};

/** Callbacks for championship progress and actions. */
class SeasonProgressInfoUI {
public:
    /**
     * Draw the season progress round with its label and two separating spaces.
     *
     * @ghidraAddress BRD3D: 0x10040b30
     * @ghidraAddress BRGlide: 0x1003a070
     */
    static int UpdateRoundLabel(MenuWidget *pWidget);

    /**
     * Draw the remaining points required to advance, with negative results clamped to zero.
     *
     * @ghidraAddress BRD3D: 0x100415a0
     * @ghidraAddress BRGlide: 0x1003ab00
     */
    static int UpdatePointsNeededLabel(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x10045050
     * @ghidraAddress BRGlide: 0x1003e5a0
     */
    static int OnCarSetup(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x10047340
     * @ghidraAddress BRGlide: 0x10040790
     */
    static int OnResetRound(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x100457c0
     * @ghidraAddress BRGlide: 0x1003ec50
     */
    static int OnOptions(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x100457e0
     * @ghidraAddress BRGlide: 0x1003ec70
     */
    static int OnSaveSeason(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x10047060
     * @ghidraAddress BRGlide: 0x100404b0
     */
    static int OnBack(MenuActionWidget *pItem);
};

/** Callbacks for championship standings and totals. */
class SeasonUI {
public:
    /**
     * @ghidraAddress BRD3D: 0x1003ea90
     * @ghidraAddress BRGlide: 0x100380b0
     */
    static int UpdateRoundPointsLabel(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x1003e8d0
     * @ghidraAddress BRGlide: 0x10037ef0
     */
    static int UpdateRoundPlaceLabel(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x10041300
     * @ghidraAddress BRGlide: 0x1003a860
     */
    static int UpdateSeasonNameLabel(MenuWidget *pWidget);

    /**
     * Draw the year with the translated suffix for its ordinal.
     *
     * @ghidraAddress BRD3D: 0x100413b0
     * @ghidraAddress BRGlide: 0x1003a910
     */
    static int UpdateYearOrdinalLabel(MenuWidget *pWidget);

    /**
     * Draw the sum of the selected season's four race scores, or zero before loading.
     *
     * @ghidraAddress BRD3D: 0x100414b0
     * @ghidraAddress BRGlide: 0x1003aa10
     */
    static int UpdateTotalPointsLabel(MenuWidget *pWidget);
};

/** Callbacks for the time attack menu. */
class TimeAttackMenuUI {
public:
    /**
     * @ghidraAddress BRD3D: 0x10044c70
     * @ghidraAddress BRGlide: 0x1003e1c0
     */
    static int OnBack(MenuActionWidget *pItem);
};

/** Callbacks for the selected time attack record. */
class TimeAttackUI {
public:
    /**
     * Update the selected time attack track image after a saved slot has been selected.
     *
     * @ghidraAddress BRD3D: 0x100407a0
     * @ghidraAddress BRGlide: 0x10039ce0
     */
    static int UpdateTrackImage(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x10041040
     * @ghidraAddress BRGlide: 0x1003a580
     */
    static int UpdateRaceTimeLabel(MenuWidget *pWidget);

    /**
     * @ghidraAddress BRD3D: 0x10041180
     * @ghidraAddress BRGlide: 0x1003a6d0
     */
    static int UpdateBestLapLabel(MenuWidget *pWidget);
};

/** Callbacks for the video options screen. */
class VideoOptionsUI {
public:
    /**
     * @ghidraAddress BRD3D: 0x100465e0
     * @ghidraAddress BRGlide: 0x1003fa80
     */
    static int OnBack(MenuActionWidget *pItem);
};

#ifdef __cplusplus
extern "C" {
#endif

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
 * @ghidraAddress BRD3D: 0x1003c9b0
 * @ghidraAddress BRGlide: 0x10036040
 */
void RefreshSessionListSlots(void);

/**
 * @ghidraAddress BRD3D: 0x1003d030
 * @ghidraAddress BRGlide: 0x100366c0
 */
int GetSelectedSessionGuid(GUID *guid);

#if TGR_BRD3D

#endif

/**
 * @ghidraAddress BRD3D: 0x10aa2a30
 * @ghidraAddress BRGlide: 0x10ac5d88
 */
extern int g_dwSelectedDisplayModeIndex;

/**
 * The entry of the list of display modes in use.
 *
 * @ghidraAddress BRD3D: 0x10aa2864
 * @ghidraAddress BRGlide: 0x10ac5bbc
 */
extern int g_dwActiveDisplayModeIndex;

/**
 * Open cached input options, building the screen on its first visit.
 *
 * @param pWidget Not read.
 * @ghidraAddress BRD3D: 0x100452c0
 * @ghidraAddress BRGlide: 0x1003e810
 */
int ShowInputOptionsMenu(MenuWidget *pWidget);

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
 * @ghidraAddress BRD3D: 0x1003f210
 * @ghidraAddress BRGlide: 0x10038750
 */
int OnSessionNameEdited(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003eef0
 * @ghidraAddress BRGlide: 0x10038420
 */
int OnPlayerNameEdited(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003ef90
 * @ghidraAddress BRGlide: 0x100384c0
 */
int OnModemPhoneNumberEdited(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003eb60
 * @ghidraAddress BRGlide: 0x10038150
 */
int SeasonRoundList_UpdateRoundSelection(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1003f110
 * @ghidraAddress BRGlide: 0x10038650
 */
int OnChatMessageEdited(MenuWidget *pWidget);

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
 * Build input options on the supplied screen.
 *
 * @return 1.
 * @ghidraAddress BRD3D: 0x1004c4a0
 * @ghidraAddress BRGlide: 0x100458d0
 */
int BuildInputOptionsUI(MenuScreen *pScreen);

/**
 * Open cached video options, building the screen on its first visit.
 *
 * @param pWidget Not read.
 * @ghidraAddress BRD3D: 0x10045390
 * @ghidraAddress BRGlide: 0x1003e8e0
 */
int ShowVideoOptionsMenu(MenuWidget *pWidget);

/**
 * @param pWidget Not read.
 * @ghidraAddress BRD3D: 0x100455e0
 * @ghidraAddress BRGlide: 0x1003ea70
 */
int ShowAudioOptionsMenu(MenuWidget *pWidget);

/**
 * @param pWidget Not read.
 * @ghidraAddress BRD3D: 0x100456b0
 * @ghidraAddress BRGlide: 0x1003eb40
 */
int ShowGameOptionsMenu(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1004dfc0
 * @ghidraAddress BRGlide: 0x10046e70
 */
int BuildAudioOptionsUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x1003e980
 * @ghidraAddress BRGlide: 0x10037fa0
 */
int DrawGameplaySliderA(MenuWidget *pWidget);
/**
 * @ghidraAddress BRD3D: 0x1003e9e0
 * @ghidraAddress BRGlide: 0x10038000
 */
int DrawGameplaySliderB(MenuWidget *pWidget);
/** @ghidraAddress BRD3D: 0x100ab548 */
extern RECT g_rcMenuAudioListBox;

/**
 * @ghidraAddress BRD3D: 0x1004e830
 * @ghidraAddress BRGlide: 0x100476e0
 */
int BuildGameOptionsUI(MenuScreen *pScreen);

/**
 * Build video options on the supplied screen.
 *
 * @return 1.
 * @ghidraAddress BRD3D: 0x1004d1f0
 * @ghidraAddress BRGlide: 0x10046620
 */
int BuildVideoOptionsUI(MenuScreen *pScreen);

/**
 * Open cached key configuration, building the screen on its first visit.
 *
 * @param pWidget Not read.
 * @ghidraAddress BRD3D: 0x100434c0
 * @ghidraAddress BRGlide: 0x1003ca10
 */
int ShowKeyConfigScreen(MenuWidget *pWidget);

/**
 * Build key configuration on the supplied screen.
 *
 * @return 1.
 * @ghidraAddress BRD3D: 0x1004cac0
 * @ghidraAddress BRGlide: 0x10045ef0
 */
int BuildKeyConfigUI(MenuScreen *pScreen);

/**
 * Open cached modem connection options, building the screen on its first visit.
 *
 * @param pWidget Not read.
 * @ghidraAddress BRD3D: 0x10043da0
 * @ghidraAddress BRGlide: 0x1003d2f0
 */
int ShowModemDialUpScreen(MenuWidget *pWidget);

/**
 * Reset time attack choices and restore race setup after a season.
 *
 * @param pWidget Not read.
 * @ghidraAddress BRD3D: 0x10046260
 * @ghidraAddress BRGlide: 0x1003f700
 */
int ReturnToMainMenuFromSeason(MenuWidget *pWidget);

/**
 * Build modem connection options on the supplied screen.
 *
 * @ghidraAddress BRD3D: 0x10056a10
 * @ghidraAddress BRGlide: 0x1004f8c0
 */
int BuildModemConnectionUI(MenuScreen *pScreen);

/**
 * Open display mode selection and populate its settings lists after successful creation.
 *
 * @param pWidget The item chosen, unread.
 * @return 0 when the screen cannot be created, 1 otherwise.
 * @ghidraAddress BRD3D: 0x10045520
 * @ghidraAddress BRGlide: 0x1003e9b0
 */
int ShowDisplayModeSelectMenu(MenuWidget *pWidget);

/**
 * Build display mode selection on the supplied screen.
 *
 * @return 1.
 * @ghidraAddress BRD3D: 0x1004db00
 * @ghidraAddress BRGlide: 0x100469b0
 */
int BuildDisplayModeSelectUI(MenuScreen *pScreen);

/**
 * Populate the video settings lists. The Glide library has no list of display devices.
 *
 * @return Whether the lists were populated.
 * @ghidraAddress BRD3D: 0x1007ac00
 * @ghidraAddress BRGlide: 0x10058f90
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

/**
 * @ghidraAddress BRD3D: 0x100ab528
 * @ghidraAddress BRGlide: 0x100aacc8
 */
extern RECT g_rcMenuDeviceListBox;

/** @ghidraAddress BRD3D: 0x100ab538 */
extern RECT g_rcMenuLoadSeasonListBox;

/**
 * Populate available display modes for the selected device. The Glide library lists the modes of
 * BuildGlideDisplayModeList().
 *
 * @return Whether a mode was listed.
 * @ghidraAddress BRD3D: 0x1007a940
 * @ghidraAddress BRGlide: 0x10058e20
 */
int PopulateDisplayModeList(void);

/**
 * Open Direct3D device selection and populate its settings lists.
 *
 * @param pWidget The item chosen, unread.
 * @return 0 when the screen cannot be created, 1 otherwise.
 * @ghidraAddress BRD3D: 0x10045460
 */
int ShowD3DDeviceSelectMenu(MenuWidget *pWidget);

/**
 * Build Direct3D device selection on the supplied screen.
 *
 * @return 1.
 * @ghidraAddress BRD3D: 0x1004d640
 */
int BuildD3DDeviceSelectUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x1003ee50
 * @ghidraAddress BRGlide: 0x10038380
 */
int ProcessMenuTextEditInput(MenuWidget *pWidget, short nFont);

/** A keyboard configuration row and its label resource. */
struct KeyConfigEntry {
    unsigned nStringId; /*!< The label resource identifier. */
    int nControl;       /*!< The control being configured. */
};

/**
 * @ghidraAddress BRD3D: 0x100ab330
 * @ghidraAddress BRGlide: 0x100aaad0
 */
extern KeyConfigEntry g_aKeyConfigEntries[KEY_CONFIG_ENTRY_COUNT];

/**
 * @ghidraAddress BRD3D: 0x100ac6a0
 * @ghidraAddress BRGlide: 0x100abe40
 */
extern int g_dwCapturedKeyBindingValue;

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
 * @param pWidget The item chosen, unread.
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x10045110
 * @ghidraAddress BRGlide: 0x1003e660
 */
int ShowCarSetupScreen(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1004a580
 * @ghidraAddress BRGlide: 0x100439b0
 */
int BuildCarSetupUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x100ac304
 * @ghidraAddress BRGlide: 0x100abaa4
 */
extern int g_bRaceSetupShowCarSetup;

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
 * @ghidraAddress BRD3D: 0x100558a0
 * @ghidraAddress BRGlide: 0x1004e750
 */
int BuildMultiplayerConnectionUI(MenuScreen *pScreen);

/**
 * Open cached connection settings, building the screen on its first visit.
 *
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x10043cd0
 * @ghidraAddress BRGlide: 0x1003d220
 */
int ShowMultiplayerConnectionScreen(MenuWidget *pWidget = nullptr);

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
#ifdef __cplusplus
}
#endif

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
 * @ghidraAddress BRD3D: 0x100ac598
 * @ghidraAddress BRGlide: 0x100abd38
 */
extern const signed char g_abTransmissionImageIds[2];

/**
 * @ghidraAddress BRD3D: 0x100ac59c
 * @ghidraAddress BRGlide: 0x100abd3c
 */
extern const signed char g_abTireImageIds[3];

/**
 * @ghidraAddress BRD3D: 0x100ac5a0
 * @ghidraAddress BRGlide: 0x100abd40
 */
extern const signed char g_abSuspensionImageIds[3];

/**
 * @ghidraAddress BRD3D: 0x100ac628
 * @ghidraAddress BRGlide: 0x100abdc8
 */
extern const signed char g_abInputDeviceImageIds[4];

/**
 * @ghidraAddress BRD3D: 0x100ac62c
 * @ghidraAddress BRGlide: 0x100abdcc
 */
extern const signed char g_abServiceProviderImageIds[4];

/**
 * @ghidraAddress BRD3D: 0x100ac634
 * @ghidraAddress BRGlide: 0x100abdd4
 */
extern const signed char g_abCarShadowImageIds[2];

/**
 * @ghidraAddress BRD3D: 0x100ac638
 * @ghidraAddress BRGlide: 0x100abdd8
 */
extern const signed char g_abSpecularLightingImageIds[2];

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
     * @ghidraAddress BRGlide: 0x100418c0
     */
    MenuPanel();

    /**
     * @ghidraAddress BRD3D: 0x100484f0
     * @ghidraAddress BRGlide: 0x10041940
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

    /** Set the callback invoked before panel widgets run. */
    void SetBeforeWidgetsCallback(int (*callback)(void)) {
        pfnBeforeWidgets = callback;
    }
    /** Set the callback invoked after panel widgets run. */
    void SetAfterWidgetsCallback(int (*callback)(void)) {
        pfnAfterWidgets = callback;
    }
    /** Replace the panel flags. */
    int SetFlags(int value) {
        return nFlags = value;
    }
    /** @return The number of widgets in the panel. */
    unsigned short GetWidgetCount() const {
        return wWidgetCount;
    }
    /** Replace the number of widgets in the panel. */
    unsigned short SetWidgetCount(unsigned short value) {
        return wWidgetCount = value;
    }
    /** @return The widget at @p index. */
    MenuWidget *GetWidget(int index) const {
        return apWidgets[index];
    }
    /** Replace the widget at @p index. */
    MenuWidget *SetWidget(int index, MenuWidget *widget) {
        return apWidgets[index] = widget;
    }
    /** @return The panel's horizontal position. */
    float GetX() const {
        return fX;
    }
    /** Set the panel's horizontal position. */
    float SetX(float value) {
        return fX = value;
    }
    /** @return The panel's vertical position. */
    float GetY() const {
        return fY;
    }
    /** Set the panel's vertical position. */
    float SetY(float value) {
        return fY = value;
    }
    /** Associate the panel with @p screen. */
    MenuScreen *SetScreen(MenuScreen *screen) {
        return pScreen = screen;
    }
    /** @return The number of navigable widgets. */
    unsigned short GetNavigationCount() const {
        return wNavigationCount;
    }
    /** Replace the number of navigable widgets. */
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
     * @ghidraAddress BRD3D: 0x10043fa0
     * @ghidraAddress BRGlide: 0x1003d4f0
     */
    static int OnReturnToMainMenu(MenuWidget *pWidget);

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
     * Flip the frame to the display and wait for the flip to finish. The Glide library writes
     * g_pMenuFrameSurface to the frame buffer and shows it with g_pfnPresentFrame instead.
     *
     * @ghidraAddress BRD3D: 0x10048960
     * @ghidraAddress BRGlide: 0x10041db0
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
     * @ghidraAddress BRGlide: 0x10053e60
     */
    virtual HRESULT ClearBackBuffer();

    /** @return The callback that builds the screen. */
    auto GetBuildCallback() const {
        return pfnBuildMenu;
    }
    /** Set the callback that builds the screen. */
    void SetBuildCallback(int (*callback)(MenuScreen *)) {
        pfnBuildMenu = callback;
    }
    /** @return Whether the screen has been built. */
    bool IsBuilt() const {
        return bBuilt != 0;
    }
    /** Set whether the screen has been built. */
    void SetBuilt(bool value) {
        bBuilt = value;
    }
    /** @return The number of panels in the screen. */
    unsigned short GetPanelCount() const {
        return wPanelCount;
    }
    /** Replace the number of panels in the screen. */
    unsigned short SetPanelCount(unsigned short value) {
        return wPanelCount = value;
    }
    /** Set the index of the current panel. */
    unsigned short SetCurrentPanelIndex(unsigned short value) {
        return wCurrentPanel = value;
    }
    /** @return The panel at @p index. */
    MenuPanel *GetPanel(int index) const {
        return apPanels[index];
    }
    /** Replace the panel at @p index. */
    MenuPanel *SetPanel(int index, MenuPanel *panel) {
        return apPanels[index] = panel;
    }
    /** @return The current panel. */
    MenuPanel *GetCurrentPanel() const {
        return pCurrentPanel;
    }
    /** @return Whether the screen is running. */
    bool IsRunning() const {
        return bRunning != 0;
    }
    /** Set whether the screen is running. */
    void SetRunning(bool value) {
        bRunning = value;
    }
    /** @return Whether the panel at @p index is active. */
    int GetPanelActive(int index) const {
        return anPanelActive[index];
    }
    /** Set whether the panel at @p index is active. */
    int SetPanelActive(int index, int value) {
        return anPanelActive[index] = value;
    }
    /** @return The save-file list at @p index. */
    SaveFileList *GetFileList(int index) const {
        return apFiles[index];
    }
    /** @return The widget selected when panel navigation resets. */
    unsigned short GetPanelResetWidgetIndex() const {
        return wPanelResetWidgetIndex;
    }
    /** Reset panel activation and select @p widgetIndex. */
    inline void ResetPanelActivation(unsigned short widgetIndex);

private:
    int (*pfnBuildMenu)(MenuScreen *pScreen); /*!< Builds the items of the screen. */
    int dwFlags;                  /*!< Options of the screen. Bit 0x10 stops EndFrame(). */
    int bBuilt;                   /*!< Whether pfnBuildMenu built the items of the screen. */
    unsigned short wPanelCount;   /*!< The number of entries of apPanels in use. */
    unsigned short wCurrentPanel; /*!< The index of the panel that runs. */
    MenuPanel *apPanels[MENU_SCREEN_PANEL_COUNT]; /*!< The panels of the screen. */
    MenuPanel *pCurrentPanel;                     /*!< The panel that runs. */
    int bRunning; /*!< Whether the screen runs and reads window messages. */
    /** Whether each panel of apPanels runs its items. */
    int anPanelActive[MENU_SCREEN_PANEL_COUNT];
    unsigned short wPanelResetWidgetIndex;
    SaveFileList *apFiles[2]; /*!< The two lists of saved files of the screen. */
};

#ifdef __cplusplus
extern "C" {
#endif

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
#if TGR_BRD3D
    IDirectDrawSurface *pSurface; /*!< The surface the image is loaded into. */
    DDSURFACEDESC desc;           /*!< The description of pSurface. */
#else
    Rgb565Image *pImage; /*!< The image loaded from the file. */
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
 * The number of entries of g_aMenuImages loaded.
 *
 * @ghidraAddress BRD3D: 0x10aa28d4
 * @ghidraAddress BRGlide: 0x10ac5c2c
 */
extern unsigned short g_wMenuImageLoadedCount;

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
 * @ghidraAddress BRGlide: 0x100aabe8
 */
extern RECT g_rcMenuCenterPanel;
/**
 * @ghidraAddress BRD3D: 0x100ab508
 * @ghidraAddress BRGlide: 0x100aaca8
 */
extern RECT g_rcMenuTitleBar;
/**
 * @ghidraAddress BRD3D: 0x100ab4d8
 * @ghidraAddress BRGlide: 0x100aac78
 */
extern RECT g_rcMenuListBox;
/**
 * @ghidraAddress BRD3D: 0x100ab4b8
 * @ghidraAddress BRGlide: 0x100aac58
 */
extern RECT g_rcMenuLeftColumn;
/**
 * @ghidraAddress BRD3D: 0x100ab478
 * @ghidraAddress BRGlide: 0x100aac18
 */
extern RECT g_rcMenuRightColumnWide;
/**
 * @ghidraAddress BRD3D: 0x100ab3f8
 * @ghidraAddress BRGlide: 0x100aab98
 */
extern RECT g_rcMenuNetworkPlayers;
/**
 * @ghidraAddress BRD3D: 0x100ab488
 * @ghidraAddress BRGlide: 0x100aac28
 */
extern RECT g_rcMenuRightColumn;
/**
 * @ghidraAddress BRD3D: 0x100ab468
 * @ghidraAddress BRGlide: 0x100aac08
 */
extern RECT g_rcMenuRightLabel;
/**
 * @ghidraAddress BRD3D: 0x100ad300
 * @ghidraAddress BRGlide: 0x100acad8
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
 * @ghidraAddress BRD3D: 0x100ac3b0
 * @ghidraAddress BRGlide: 0x100abb50
 */
extern int g_adwWeatherLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x100ac3a8
 * @ghidraAddress BRGlide: 0x100abb48
 */
extern int g_adwTransmissionLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x100ac358
 * @ghidraAddress BRGlide: 0x100abaf8
 */
extern int g_adwSuspensionLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x100ac3e0
 * @ghidraAddress BRGlide: 0x100abb80
 */
extern int g_adwInputDeviceLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x100ac3f0
 * @ghidraAddress BRGlide: 0x100abb90
 */
extern int g_adwServiceProviderLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x100ac408
 * @ghidraAddress BRGlide: 0x100abba8
 */
extern int g_adwCarShadowLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x100ac418
 * @ghidraAddress BRGlide: 0x100abbb8
 */
extern int g_adwSkidMarksLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x100ac410
 * @ghidraAddress BRGlide: 0x100abbb0
 */
extern int g_adwSpecularLightingLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x100ac400
 * @ghidraAddress BRGlide: 0x100abba0
 */
extern int g_adwForceFeedbackLabelStrIds[];

/**
 * @ghidraAddress BRD3D: 0x100446d0
 * @ghidraAddress BRGlide: 0x1003dc20
 */
int ShowNetworkCarSetupScreen(MenuWidget *pWidget = nullptr);

/**
 * @ghidraAddress BRD3D: 0x10058750
 * @ghidraAddress BRGlide: 0x10051600
 */
int BuildNetworkCarSetupUI(MenuScreen *pScreen);

/**
 * Accept the edited text of a connection field. The linker folds the routine with the other
 * routines that return 1.
 *
 * @param pWidget Not read.
 * @return 1.
 * @ghidraAddress BRD3D: 0x10042af0
 * @ghidraAddress BRGlide: 0x100385e0
 */
int CompleteConnectionTextEdit(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x100ac348
 */
extern const int g_adwTireLabelStrIds[3];

/**
 * @ghidraAddress BRD3D: 0x100ac5a8
 */
extern const int g_adwCarImageIds[32];

/**
 * @ghidraAddress BRD3D: 0x1003e100
 * @ghidraAddress BRGlide: 0x100377a0
 */
int IsGameCdInDrive(int nDrive);

/**
 * @ghidraAddress BRD3D: 0x10045a00
 * @ghidraAddress BRGlide: 0x1003ee90
 */
int FindGameCdDrive(void);

/**
 * @param pWidget The item chosen, unread.
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x10045900
 * @ghidraAddress BRGlide: 0x1003ed90
 */
int ShowChampionshipSeasonMenu(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1004f2b0
 * @ghidraAddress BRGlide: 0x10048160
 */
int BuildChampionshipSeasonUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x1005a6e0
 * @ghidraAddress BRGlide: 0x10053590
 */
int BuildSaveTimeAttackMenuUI(MenuScreen *pScreen);

/**
 * @param pWidget Not read.
 * @ghidraAddress BRD3D: 0x10044d00
 * @ghidraAddress BRGlide: 0x1003e250
 */
int ShowLoadGhostMenu(MenuWidget *pWidget);

/**
 * Host a session and show the race setup of the network game.
 *
 * @param pWidget The widget of the action. Not read, and the direct callers pass null.
 * @return 0 when the screen cannot be allocated, otherwise 1.
 * @ghidraAddress BRD3D: 0x100443e0
 * @ghidraAddress BRGlide: 0x1003d930
 */
int ShowHostRaceSetupScreen(MenuWidget *pWidget);
/**
 * Show the screen that creates a network game.
 *
 * @param pWidget The widget of the action. Not read, and the direct callers pass null.
 * @return 0 when the screen cannot be allocated, otherwise 1.
 * @ghidraAddress BRD3D: 0x100440d0
 * @ghidraAddress BRGlide: 0x1003d620
 */
int ShowCreateGameScreen(MenuWidget *pWidget);

/**
 * @param pWidget The item chosen, or null from the lobby; unread.
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x10043bf0
 * @ghidraAddress BRGlide: 0x1003d140
 */
int ShowMultiplayerNameScreen(MenuWidget *pWidget);

/**
 * @param pWidget The item of the chosen service provider, or null from the lobby; unread.
 * @return 0 when the screen cannot be allocated, otherwise 1.
 * @ghidraAddress BRD3D: 0x10043e70
 * @ghidraAddress BRGlide: 0x1003d3c0
 */
int ShowJoinCreateGameScreen(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10045dc0
 * @ghidraAddress BRGlide: 0x1003f260
 */
int ShowSeasonProgressInfoScreen(MenuWidget *pWidget = nullptr);

/**
 * @param pWidget The item chosen, unread.
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x10044f50
 * @ghidraAddress BRGlide: 0x1003e4a0
 */
int ShowRaceSetupMenu(MenuWidget *pWidget);

/**
 * @param pWidget The item chosen, unread.
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x10046170
 * @ghidraAddress BRGlide: 0x1003f610
 */
int ShowQuitGameConfirmMenu(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x10059760
 * @ghidraAddress BRGlide: 0x10052610
 */
int BuildTimeAttackMenuUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10059bb0
 * @ghidraAddress BRGlide: 0x10052a60
 */
int BuildLoadTimeAttackMenuUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10057c10
 * @ghidraAddress BRGlide: 0x10050ac0
 */
int BuildMultiplayerGameSetupUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x100563e0
 * @ghidraAddress BRGlide: 0x1004f290
 */
int BuildPlayerNameEntryUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10056ff0
 * @ghidraAddress BRGlide: 0x1004fea0
 */
int BuildJoinCreateGameUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x100575f0
 * @ghidraAddress BRGlide: 0x100504a0
 */
int BuildCreateGameMenuUI(MenuScreen *pScreen);

/**
 * @param pWidget Not read.
 * @ghidraAddress BRD3D: 0x10044280
 * @ghidraAddress BRGlide: 0x1003d7d0
 */
int JoinSessionAndShowRaceSetup(MenuWidget *pWidget = nullptr);

/**
 * @ghidraAddress BRD3D: 0x10052030
 * @ghidraAddress BRGlide: 0x1004aee0
 */
int BuildSeasonProgressInfoUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x1004b430
 * @ghidraAddress BRGlide: 0x10044860
 */
int BuildRaceSetupUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10049c20
 * @ghidraAddress BRGlide: 0x10043050
 */
int BuildQuitGameConfirmUI(MenuScreen *pScreen);

/**
 * @param pWidget The item chosen, unread.
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x100451e0
 * @ghidraAddress BRGlide: 0x1003e730
 */
int ShowOptionsMenu(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1004bdc0
 * @ghidraAddress BRGlide: 0x100451f0
 */
int BuildOptionsMenuUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x100419d0
 * @ghidraAddress BRGlide: 0x1003af30
 */
void SetActiveMenuItemText(const char *pszText);

/**
 * @ghidraAddress BRD3D: 0x100ab558
 * @ghidraAddress BRGlide: 0x100aacf8
 */
extern const RECT g_rcMenuRootTitleBar;

/**
 * @ghidraAddress BRD3D: 0x10aa29c8
 * @ghidraAddress BRGlide: 0x10ac5d20
 */
extern MenuWidget *g_pOptionsSubMenu;

/**
 * @ghidraAddress BRD3D: 0x100ab3f4
 * @ghidraAddress BRGlide: 0x100aab94
 */
extern int g_dwFileListSelectedSlot;

/**
 * Build the items of the root screen of the front end.
 *
 * @param pScreen The screen.
 * @return 1.
 * @ghidraAddress BRD3D: 0x100491b0
 * @ghidraAddress BRGlide: 0x100425e0
 */
int BuildFrontEndRootMenuUI(MenuScreen *pScreen);

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
#ifdef __cplusplus
}
#endif

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

#if TGR_BRD3D
/**
 * The DirectDraw interface the menu images are loaded with, while the menus are set up.
 *
 * @ghidraAddress BRD3D: 0x10a9d070
 */
extern IDirectDraw2 *g_pMenuDirectDraw;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Load every menu image into a surface with a green colour key, showing the loading image while
 * the images load again.
 *
 * @return Whether every image loaded. The game exits when one does not.
 * @ghidraAddress BRD3D: 0x1005f690
 */
bool InitMenuDirectDrawSurfaces(void);
#ifdef __cplusplus
}
#endif

#endif /* TGR_BRD3D */

#if TGR_RENDERER == TGR_RENDERER_GLIDE
/**
 * The frame the menus are drawn into, the size of the frame buffer.
 *
 * @ghidraAddress BRGlide: 0x10ac5d84
 */
extern Rgb565Image *g_pMenuFrameSurface;

/**
 * Create g_pMenuFrameSurface, show the loading screen unless this is the first call, and load
 * every menu image with its colour key.
 *
 * @return Whether every image loaded. The game exits when one does not.
 * @ghidraAddress BRGlide: 0x100583c0
 */
bool InitMenuSurfaces(void);
#endif

#ifdef __cplusplus
extern "C" {
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
#ifdef __cplusplus
}
#endif

#if TGR_BRD3D
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Choose the DirectDraw driver and the display mode of g_GlobalRallyConfig.
 *
 * @ghidraAddress BRD3D: 0x1003e2c0
 */
void SelectConfiguredDisplayMode(void);
#ifdef __cplusplus
}
#endif

#endif

/** The number of entries of g_adwInputDeviceMenuIndexToDevice. */
#define INPUT_DEVICE_MENU_CHOICES 4

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Show the screen of the results of a race.
 *
 * @ghidraAddress BRD3D: 0x10045ea0
 * @ghidraAddress BRGlide: 0x1003f340
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
 * @param pWidget The item chosen, or null after a race; unread.
 * @return 0 when the menu cannot be created, 1 otherwise.
 * @ghidraAddress BRD3D: 0x10044b90
 * @ghidraAddress BRGlide: 0x1003e0e0
 */
int ShowTimeAttackMenu(MenuWidget *pWidget);

/**
 * Show the menu to save the ghost car of a time attack, when it is not shown already.
 *
 * @param nUnused Not read.
 * @return 0 when the menu cannot be created, 1 otherwise.
 * @ghidraAddress BRD3D: 0x10044e20
 * @ghidraAddress BRGlide: 0x1003e370
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

#if TGR_BRD3D

#endif /* TGR_BRD3D */

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
 * menus. A lost surface restores the menu images and the part is not drawn. The Glide library
 * copies the part into g_pMenuFrameSurface with BlitRgb565ImageRegion().
 *
 * @param nDestX The left to draw at.
 * @param nDestY The top to draw at.
 * @param wImage The index in g_aMenuImages of the image.
 * @param pRect The part of the image.
 * @param dwFlags The flags of BltFast(). DDBLTFAST_WAIT is added.
 * @ghidraAddress BRD3D: 0x1005f5a0
 * @ghidraAddress BRGlide: 0x10058380
 */
void BlitSurfaceRegion(int nDestX, int nDestY, WORD wImage, const RECT *pRect, DWORD dwFlags);

/**
 * Release the surfaces of the loaded menu images. The Glide library frees its images and the
 * surface of the menu frame instead.
 *
 * @ghidraAddress BRD3D: 0x1005f530
 * @ghidraAddress BRGlide: 0x10058300
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
 * An unused value cleared with the menu selections.
 *
 * @ghidraAddress BRD3D: 0x10aa2a04
 * @ghidraAddress BRGlide: 0x10ac5d5c
 */
extern int g_dwUnusedWeatherMenuValue;

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
 * Give a message of the main window to the front end.
 *
 * @param pUnused Not read.
 * @param hWnd The main window.
 * @param nMessage The message.
 * @param wParam The first parameter of the message.
 * @param lParam The second parameter of the message.
 * @return The result of the message for the front end.
 * @ghidraAddress BRD3D: 0x10060060
 * @ghidraAddress BRGlide: 0x100590d0
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
 * @ghidraAddress BRD3D: 0x100ac420
 * @ghidraAddress BRGlide: 0x100abbc0
 */
extern int g_adwCarMenuIndexToCarId[32];

/**
 * @ghidraAddress BRD3D: 0x100ac4a0
 * @ghidraAddress BRGlide: 0x100abc40
 */
extern int g_adwTireMenuIndexToTireType[3];

/**
 * @ghidraAddress BRD3D: 0x100ac4b0
 * @ghidraAddress BRGlide: 0x100abc50
 */
extern int g_adwSuspensionMenuIndexToType[3];

/**
 * @ghidraAddress BRD3D: 0x100ac4c0
 * @ghidraAddress BRGlide: 0x100abc60
 */
extern int g_adwWeatherMenuIndexToWeather[5];

/**
 * @ghidraAddress BRD3D: 0x100ac4d8
 * @ghidraAddress BRGlide: 0x100abc78
 */
extern int g_adwTrackMenuIndexToTrackId[15];

/**
 * @ghidraAddress BRD3D: 0x100ac518
 * @ghidraAddress BRGlide: 0x100abcb8
 */
extern int g_adwTransmissionMenuIndexToType[2];

/**
 * Report a track's selection mask or a cheat override.
 *
 * @param nTrack The track menu index.
 * @return Zero when unavailable, otherwise a nonzero mask or one.
 * @ghidraAddress BRD3D: 0x1003f2b0
 * @ghidraAddress BRGlide: 0x100387f0
 */
unsigned IsTrackSelectable(int nTrack);

/**
 * @ghidraAddress BRD3D: 0x10052f50
 * @ghidraAddress BRGlide: 0x1004be00
 */
int BuildRaceResultsInfoUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10053cf0
 * @ghidraAddress BRGlide: 0x1004cba0
 */
int BuildRaceResultsSummaryUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10054b50
 * @ghidraAddress BRGlide: 0x1004da00
 */
int BuildRaceResultsTotalsUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x1004a260
 * @ghidraAddress BRGlide: 0x10043690
 */
int BuildResultsSummaryExitSeasonConfirmUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10051990
 * @ghidraAddress BRGlide: 0x1004a840
 */
int BuildSeasonCongratsUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10051d30
 * @ghidraAddress BRGlide: 0x1004abe0
 */
int BuildNotEnoughPointsUI(MenuScreen *pScreen);

/**
 * Show cached championship congratulations, building the screen on its first visit.
 *
 * @param pWidget The item chosen, unread.
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x10043260
 * @ghidraAddress BRGlide: 0x1003c7b0
 */
int ShowSeasonCongratsScreen(MenuWidget *pWidget);

/**
 * Show cached championship failure results, building the screen on its first visit.
 *
 * @param pWidget The item chosen, unread.
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x10043330
 * @ghidraAddress BRGlide: 0x1003c880
 */
int ShowNotEnoughPointsScreen(MenuWidget *pWidget);

/**
 * Show cached race totals, building the screen on its first visit.
 *
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x100460a0
 * @ghidraAddress BRGlide: 0x1003f540
 */
int ShowRaceResultsTotalsScreen(MenuWidget *pWidget = nullptr);

/**
 * Show cached race summary results and build an exit confirmation on the first visit.
 *
 * @return Whether both initial allocations succeeded, or the summary was already cached.
 * @ghidraAddress BRD3D: 0x10045f70
 * @ghidraAddress BRGlide: 0x1003f410
 */
int ShowRaceResultsSummaryScreen(MenuWidget *pWidget = nullptr);

/**
 * @ghidraAddress BRD3D: 0x100509f0
 * @ghidraAddress BRGlide: 0x100498a0
 */
int BuildPreRaceInfoUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10049f40
 * @ghidraAddress BRGlide: 0x10043370
 */
int BuildPreRaceExitSeasonConfirmUI(MenuScreen *pScreen);

/**
 * Show cached pre-race information and build an exit confirmation on the first visit.
 *
 * @param pWidget The item chosen, unread.
 * @return Whether both initial allocations succeeded, or the information screen was cached.
 * @ghidraAddress BRD3D: 0x10045c90
 * @ghidraAddress BRGlide: 0x1003f130
 */
int ShowPreRaceInfoScreen(MenuWidget *pWidget);

/**
 * @ghidraAddress BRD3D: 0x1004f700
 * @ghidraAddress BRGlide: 0x100485b0
 */
int BuildLoadSeasonUI(MenuScreen *pScreen);

/**
 * @ghidraAddress BRD3D: 0x10050060
 * @ghidraAddress BRGlide: 0x10048f10
 */
int BuildSaveSeasonUI(MenuScreen *pScreen);

/**
 * Show cached season loading, building the screen on its first visit.
 *
 * @param pWidget Not read.
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x10045af0
 * @ghidraAddress BRGlide: 0x1003ef90
 */
int ShowLoadSeasonMenu(MenuWidget *pWidget);

/**
 * Show cached season saving, building the screen on its first visit.
 *
 * @param pWidget The item chosen, unread.
 * @return Whether the screen was allocated or already cached.
 * @ghidraAddress BRD3D: 0x10045bc0
 * @ghidraAddress BRGlide: 0x1003f060
 */
int ShowSaveSeasonScreen(MenuWidget *pWidget);
#ifdef __cplusplus
}
#endif
