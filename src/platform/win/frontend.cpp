#include "platform/win/frontend.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include <direct.h>
#include <io.h>
#include <windows.h>

#pragma push_macro("errno")
#undef errno
#include <PR/os_cont.h>
#pragma pop_macro("errno")

#include "game.h"
#include "laps.h"
#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/boot/strings.h"
#include "platform/win/directplay.h"
#include "platform/win/ghost.h"
#include "platform/win/input.h"
#include "platform/win/season.h"
#include "platform/win/settings.h"
#include "platform/win/sound.h"
#include "platform/win/timer.h"
#include "profile.h"
#include "screen.h"
#include "seasons.h"
#include "trackmenuitem.h"
#include "trackselect.h"
#include "vehicle.h"
#if TGR_RENDERER == TGR_RENDERER_D3D
#include "platform/win/d3d/ddutil.h"
#include "platform/win/d3d/renderer.h"
#include "platform/win/render.h"
#endif

MenuScreen *g_pActiveMenuScreen;
int g_bAudioOptionsSfxSelected;
void *g_pFrontEndScratchBuffer;
MenuImage g_aMenuImages[MENU_IMAGE_COUNT];
unsigned short g_wMenuImageLoadedCount;
unsigned short g_wUnusedFrontEndWord0;
unsigned short g_wUnusedFrontEndWord1;
FrontEndResourceSlot g_aFrontEndResourceSlots[FRONT_END_RESOURCE_SLOT_COUNT];
int g_adwFrontEndResourceFlags[FRONT_END_RESOURCE_FLAG_COUNT];
MenuScreen *g_pRootMenuScreen;
int g_dwSaveSlotListIsSaveMode;
char g_szSeasonSaveFileName[MAX_PATH];
char g_szTimeAttackSaveFileName[MAX_PATH];
RECT g_aMenuGlyphRects[MENU_GLYPH_RECT_COUNT];
RECT g_aMenuAltGlyphRects[MENU_ALT_GLYPH_RECT_COUNT];
RECT g_aMenuLargeIconRects[MENU_LARGE_ICON_RECT_COUNT];
RECT g_aMenuLargeIconRects3Col[MENU_LARGE_ICON_3_COLUMN_RECT_COUNT];
int g_dwFrontEndFrameCounter;
int g_dwSelectedCdTrack;
int g_dwSessionListRefreshing;
int g_dwFrontEndAtRootMenu;
int g_dwTrackMenuIndex = 2;
int g_dwSelectedWeather;
int g_dwSelectedTransmission;
int g_dwTireMenuIndex = 1;
int g_dwSuspensionMenuIndex = 1;
int g_dwCarMenuIndex = 1;
int g_dwSelectedInputDevice;
int g_dwTotalLapsSetting = 3;
int g_dwAccumUnlockedCarMask;
int g_dwAccumUnlockedTrackMask;
int g_dwHandlingTypeMenuIndex = 4;
int g_dwSelectedCarClass;
int g_dwAppliedCarGroupPreset;
unsigned g_dwSelectableCarMask = 0x102;
const int g_adwInputDeviceMenuIndexToDevice[INPUT_DEVICE_MENU_CHOICES] = {0, 1, 2, 3};
int g_dwOptionForceFeedback;
int g_dwOptionCarShadow;
int g_dwOptionSpecularLighting;
int g_dwOptionSkidMarks;
unsigned short g_wUnlockedCarModeMask = 0x102;
int g_dwSelectableTrackMaskAlt = 0x24;
int g_dwNetworkMenuState;
int g_dwSelectedSessionIndex;
int g_dwSelectedDisplayModeIndex;
NetworkMenuWidget *g_pJoinCreateGameMenu;
MenuScreen *g_pCachedMenu_MainMenu;
MenuScreen *g_pCachedMenu_MainMenuAlternate;
MenuScreen *g_pCachedMenu_CarSetup;
MenuScreen *g_pCachedMenu_Options;
MenuScreen *g_pCachedMenu_TimeTrial;
MenuScreen *g_pCachedMenu_Unused2920;
MenuScreen *g_pCachedMenu_LoadSeason;
MenuScreen *g_pCachedMenu_SaveSeason;
MenuScreen *g_pCachedMenu_PreRaceInfo;
MenuScreen *g_pCachedMenu_SeasonProgressInfo;
MenuScreen *g_pCachedMenu_RaceResults;
MenuScreen *g_pCachedMenu_RaceResultsSummary;
MenuScreen *g_pCachedMenu_RaceResultsTotals;
MenuScreen *g_pRaceResultsSummaryMenuScreen;
MenuScreen *g_pCachedMenu_MultiplayerConnection;
MenuScreen *g_pCachedMenu_Unused2944;
MenuScreen *g_pCachedMenu_JoinCreateGame;
MenuScreen *g_pCachedMenu_CreateGame;
MenuScreen *g_pCachedMenu_RaceSetupNetwork;
MenuScreen *g_pCachedMenu_NetworkCarSetup;
MenuScreen *g_pCachedMenu_MultiplayerName;
MenuScreen *g_pCachedMenu_TimeAttack;
MenuScreen *g_pCachedMenu_Unused2960;
MenuScreen *g_pCachedMenu_LoadGhost;
MenuScreen *g_pCachedMenu_SaveGhost;
MenuScreen *g_pCachedMenu_SeasonCongrats;
MenuScreen *g_pCachedMenu_NotEnoughPoints;
MenuScreen *g_pPreRaceInfoMenuScreen;
MenuScreen *g_pCachedMenu_Credits;
MenuScreen *g_pCachedMenu_InputConfig;
MenuScreen *g_pCachedMenu_GameplaySettings;
MenuScreen *g_pCachedMenu_Championship;
MenuScreen *g_pCachedMenu_ModemDialUp;
MenuScreen *g_pCachedMenu_VideoSettings;
MenuScreen *g_pCachedMenu_AudioSettings;
MenuScreen *g_pCachedMenu_KeyConfig;
MenuWidget *g_pPlayerNameContinueItem;
MenuWidget *g_pRaceSetupMenu;
MenuWidget *g_pExitSeasonConfirmMenu;
MenuWidget *g_pCarSetupMenu;
MenuWidget *g_pMultiplayerGameSetupMenu;
MenuWidget *g_pSaveSeasonMenu;
int g_dwModalDialogBlocking;
int g_dwMenuTextEditActive;
unsigned short g_wMenuHoverIndexPrev;
int g_dwMenuHoverActive;
RECT g_rcMenuCenterPanel = {148, 110, 358, 260};
RECT g_rcMenuTitleBar = {100, 10, 410, 29};
RECT g_rcMenuListBox = {188, 130, 300, 225};
RECT g_rcMenuLeftColumn = {67, 0, 167, 0};
RECT g_rcMenuRightColumnMid = {330, 0, 447, 0};
RECT g_rcMenuRightListBox = {440, 128, 540, 204};
RECT g_rcMenuRightColumnWide = {478, 0, 578, 0};
RECT g_rcMenuSubPanel = {87, 61, 186, 132};
RECT g_rcMenuRightLabel = {330, 70, 447, 86};
const char g_szSpace[] = " ";
RECT g_rcHoverTopLeft = {0, 0, 200, 200};
RECT g_rcHoverBottomLeft = {0, 380, 200, 480};
MenuWidget *g_pSaveTimeAttackMenu;
MenuItemList *g_pSaveSeasonEditMenu;
MenuItemList *g_pSaveTimeAttackEditMenu;
NetworkMenuWidget *g_pNetworkSessionJoinMenu;
NetworkMenuWidget *g_pNetworkCarSetupMenu;
MenuWidget *g_pNetworkPlayerListMenu;
MenuWidget *g_pModemContinueItem;
MenuWidget *g_pDisplayModeSelectMenu;
MenuWidget *g_pD3DDeviceSelectMenu;
MenuWidget *g_pSaveSeasonBackHandlerMenu;
MenuWidget *g_pOptionsSubMenu;
int g_dwSaveSeasonSlotNamed;
int g_dwFileListSelectedSlot = -1;

// The size of the scratch buffer of the front end.
#define FRONT_END_SCRATCH_SIZE 0x400

// The files of the menu images, in the order of g_aMenuImages.
static const char *const s_apszMenuImageFiles[MENU_IMAGE_COUNT] = {
    "images\\work1a.bmp",    "images\\cursor.bmp",    "images\\type_gry.bmp",
    "images\\type_wit.bmp",  "images\\type_mid.bmp",  "images\\bignums.bmp",
    "images\\champ.bmp",     "images\\mp.bmp",        "images\\ta.bmp",
    "images\\op.bmp",        "images\\qr.bmp",        "images\\carmt.bmp",
    "images\\mshft.bmp",     "images\\softshok.bmp",  "images\\medshok.bmp",
    "images\\hardshok.bmp",  "images\\desrttrk.bmp",  "images\\coasttrk.bmp",
    "images\\trakc.bmp",     "images\\fog.bmp",       "images\\nite.bmp",
    "images\\rain.bmp",      "images\\snow.bmp",      "images\\sunweth.bmp",
    "images\\drytire.bmp",   "images\\medtire.bmp",   "images\\wettire.bmp",
    "images\\trakd.bmp",     "images\\trake.bmp",     "images\\ashft.bmp",
    "images\\carTR.bmp",     "images\\carCE.bmp",     "images\\carCU.bmp",
    "images\\carES.bmp",     "images\\carFH.bmp",     "images\\carIP.bmp",
    "images\\carLD.bmp",     "images\\carM3.bmp",     "images\\carMN.bmp",
    "images\\carNS.bmp",     "images\\carPJ.bmp",     "images\\carPS.bmp",
    "images\\carRS.bmp",     "images\\carSP.bmp",     "images\\carBB.bmp",
    "images\\arrowdd.bmp",   "images\\arrowdu.bmp",   "images\\arrowud.bmp",
    "images\\arrowuu.bmp",   "images\\joystk.bmp",    "images\\keybd.bmp",
    "images\\steerinp.bmp",  "images\\type_yel.bmp",  "images\\steerarr.bmp",
    "images\\steeradj.bmp",  "images\\spkr.bmp",      "images\\monitr.bmp",
    "images\\namebar.bmp",   "images\\slidebox.bmp",  "images\\boxtile2.bmp",
    "images\\rboxend.bmp",   "images\\lboxend.bmp",   "images\\trakc_.bmp",
    "images\\trakd_.bmp",    "images\\trake_.bmp",    "images\\desrttr_.bmp",
    "images\\coasttr_.bmp",  "images\\mpmodem.bmp",   "images\\mptcpip.bmp",
    "images\\mpipx.bmp",     "images\\mpserial.bmp",  "images\\seasn2a.bmp",
    "images\\seasn2b.bmp",   "images\\seasn3a.bmp",   "images\\seasn3b.bmp",
    "images\\seasn4a.bmp",   "images\\seasn4b.bmp",   "images\\seasn5a.bmp",
    "images\\bgdim.bmp",     "images\\congrat.bmp",   "images\\noadv1.bmp",
    "images\\noadv2.bmp",    "images\\but-main.bmp",  "images\\but-maind.bmp",
    "images\\but-op.bmp",    "images\\but-opd.bmp",   "images\\cars1a.bmp",
    "images\\cars2a.bmp",    "images\\cars2b.bmp",    "images\\cars3a.bmp",
    "images\\cars3b.bmp",    "images\\cars4a.bmp",    "images\\cars4b.bmp",
    "images\\cars5a.bmp",    "images\\cars5b.bmp",    "images\\chatbar2.bmp",
    "images\\mousinpt.bmp",  "images\\ffstick.bmp",   "images\\carwnoshad2.bmp",
    "images\\carwshad2.bmp", "images\\specoff.bmp",   "images\\specon.bmp",
    "images\\noffstik.bmp",  "images\\listbox.bmp",   "images\\engsound.bmp",
    "images\\music.bmp",     "images\\soundtik.bmp",  "images\\soundptr.bmp",
    "images\\trrwd.bmp",     "images\\pjrwd.bmp",     "images\\mnrwd.bmp",
    "images\\mirrwd.bmp",    "images\\bbrwd.bmp",     "images\\curwd.bmp",
    "images\\fbrwd.bmp",     "images\\mtrwd.bmp",     "images\\sndlevl2.bmp",
    "images\\sndlevl3.bmp",  "images\\trakraceb.bmp", "images\\trakracel.bmp",
    "images\\but-sav.bmp",   "images\\but-savd.bmp",  "images\\z-carMT.bmp",
    "images\\z-carTR.bmp",   "images\\z-carCE.bmp",   "images\\z-carCU.bmp",
    "images\\z-carES.bmp",   "images\\z-carFH.bmp",   "images\\z-carIP.bmp",
    "images\\z-carLD.bmp",   "images\\z-carM3.bmp",   "images\\z-carMN.bmp",
    "images\\z-carNS.bmp",   "images\\z-carPJ.bmp",   "images\\z-carPS.bmp",
    "images\\z-carRS.bmp",   "images\\z-carSP.bmp",   "images\\z-carBB.bmp",
    "images\\lightr.bmp",    "images\\lightg.bmp",    "images\\tire2on.bmp",
    "images\\tire2off.bmp",  "images\\listbox2.bmp",  "images\\trakQ.bmp",
    "images\\trakQ_.bmp"};

// The string of the format of the default names of the saved files.
#define SAVE_FILE_DEFAULT_NAME_STRING 0xbe
// The error a list of saved files that is not allocated shows.
#define SAVE_FILE_LIST_ERROR 6

SaveFileList::SaveFileList() {
    memset(aszNames, 0, sizeof(aszNames));
    for (char *pszName : aszNames) {
        strcpy(pszName, g_szEmptyUiString);
    }
}

SaveFileList::~SaveFileList() {
}

int SaveFileList::WriteSeasonDescFile(int nUnused) {
    FILE *file = fopen("seasondesc.dat", "wb");
    if (!file) {
        ShowErrorMessage(7);
    }
    char description[100];
    for (int slot = 0; slot < SAVE_FILE_NAME_COUNT; ++slot) {
        strcpy(description, g_pActiveMenuScreen->apFiles[0]->aszNames[slot]);
        // The unused tail of each record retains the preceding stack contents.
        if (fwrite(description, 1, sizeof(description), file) < sizeof(description)) {
            ShowErrorMessage(7);
        }
    }
    fclose(file);
    return 1;
}

int SaveFileList::LoadSeasonDescription(short nSlot) {
    char fileName[SAVE_FILE_NAME_SIZE];
    char number[4];
    strcpy(fileName, "RallySeason");
    _itoa(nSlot, number, 10);
    strcat(fileName, number);
    strcat(fileName, ".brf");
    FILE *file = fopen(fileName, "rb");
    if (file) {
        char description[SAVE_FILE_NAME_SIZE] = {};
        if (fread(description, 1, 128, file) < 128) {
            ShowErrorMessage(7);
        }
        strcpy(g_pActiveMenuScreen->apFiles[0]->aszNames[nSlot], _strupr(description));
        fclose(file);
    }
    return 1;
}

int SaveFileList::SetEntryFromSaveFileName(const char *pszFileName, const char *pszDescription) {
    char number[32];
    const bool season = g_dwSaveSlotListIsSaveMode != 0;
    const char *prefix = season ? "RallySeason" : "TimeAttack";
    strcpy(number, pszFileName + strlen(prefix));
    const int slot = atoi(number);
    strcpy(g_pRootMenuScreen->apFiles[season ? 0 : 1]->aszNames[slot], pszDescription);
    return 1;
}

int SaveFileList::ScanSaveFiles(const char *pszPattern) {
    _finddata32_t found;
    const intptr_t handle = _findfirst32(pszPattern, &found);
    if (handle == -1) {
        return 0;
    }
    char description[SAVE_FILE_NAME_SIZE] = {};
    for (int fileIndex = 0; fileIndex < SAVE_FILE_NAME_COUNT; ++fileIndex) {
        if (fileIndex != 0) {
            memset(description, 0, sizeof(description));
            if (_findnext32(handle, &found) != 0) {
                break;
            }
        }
        FILE *file = fopen(found.name, "rb");
        fseek(file, 0, SEEK_END);
        const long descriptionOffset = ftell(file) - 128;
        fseek(file, descriptionOffset, SEEK_SET);
        fread(description, 1, 128, file);
        if (fileIndex == 0) {
            fclose(file);
        }
        SetEntryFromSaveFileName(found.name, description);
        if (fileIndex != 0) {
            fclose(file);
        }
    }
    _findclose(handle);
    return 1;
}

int SaveFileList::CountMatchingFiles(const char *pszPattern) {
    _finddata32_t found;
    const intptr_t handle = _findfirst32(pszPattern, &found);
    if (handle == -1) {
        return -1;
    }
    int count = 0;
    while (count < SAVE_FILE_NAME_COUNT - 1 && _findnext32(handle, &found) == 0) {
        ++count;
    }
    _findclose(handle);
    return count;
}

void SaveFileList::GenerateNextSaveFileName(char *pszPattern, char **ppszName) {
    char candidate[SAVE_FILE_NAME_SIZE] = {};
    char number[SAVE_FILE_NAME_SIZE] = {};
    char extension[SAVE_FILE_NAME_SIZE] = {};
    char prefix[SAVE_FILE_NAME_SIZE] = {};
    strcpy(candidate, pszPattern);
    int prefixLength = 0;
    bool suffix = false;
    for (size_t index = 0; index < strlen(pszPattern); ++index) {
        if (pszPattern[index] == 'X' || pszPattern[index] == 'x') {
            pszPattern[index] = '0';
            suffix = true;
        } else if (!suffix) {
            if (pszPattern[index] == '.') {
                suffix = true;
            } else {
                prefix[index] = pszPattern[index];
                ++prefixLength;
            }
        }
    }
    const short numericStart = static_cast<short>(prefixLength);
    for (const char *name : aszNames) {
        // Each comparison uses the pattern, rather than the preceding candidate.
        if (strncmp(pszPattern, name, numericStart) == 0 && strcmp(pszPattern, name) < 0) {
            strcpy(candidate, name);
        }
    }
    char *dot = candidate + strlen(candidate) - 1;
    while (dot != candidate && *dot != '.') {
        --dot;
    }
    if (dot != candidate) {
        strcpy(extension, dot);
    }
    for (size_t index = numericStart; index < strlen(candidate); ++index) {
        if (candidate[index] < '0' || candidate[index] > '9') {
            break;
        }
        number[index - numericStart] = candidate[index];
    }
    const int next = static_cast<int>(static_cast<unsigned>(atoi(number)) + 1);
    memset(number, 0, sizeof(number));
    // Overlapping decimal writes preserve the original suffix conversion.
    _itoa(next & 0xff000000, number, 10);
    _itoa(next & 0xff0000, number + 1, 10);
    _itoa(next & 0xff00, number + 2, 10);
    _itoa(next & 0xff, number + 3, 10);
    strcpy(*ppszName, prefix);
    strcat(*ppszName, number);
    strcat(*ppszName, extension);
}

void MenuWidget::SetText(const char *pszText,
                         unsigned dwTextFlags,
                         unsigned char bStyle,
                         const RECT *pRect) {
    MenuFont &font = fonts[0];
    std::strcpy(font.szText, pszText);
    MenuFontVTable *pVTable = font.pVTable;
    font.dwFlags |= dwTextFlags;
    font.bStyle = bStyle;
    font.nInputMaxWidth = 0;
    font.nHeight = 0;
    font.nWidth = 0;
    font.rcBounds.left = pRect->left;
    font.rcBounds.right = pRect->right;
    font.fX = fX;
    font.fY = fY;
    font.fZ = 0.0f;
    font.bDrawCaret = 0;
    if (bStyle == 3) {
        pVTable->MeasureTextAltFont(&font);
    } else {
        pVTable->MeasureText(&font);
    }
    if (dwTextFlags & 1) {
        pVTable->ComputeCenteredX(&font);
    }
    rcBounds.top = ConvertMenuCoordinate(fY);
    rcBounds.left = pRect->left;
    rcBounds.right = pRect->right;
    rcBounds.bottom = static_cast<int>(static_cast<unsigned>(rcBounds.top) + font.nHeight);
    nWidth = font.nWidth;
    nHeight = font.nHeight;
}

int MenuWidget::BlitSurfaceById(int nImage, int x, int y) {
    const MenuSprite &sprite = g_aMenuSprites[nImage];
    BlitSurfaceRegion(x, y, static_cast<WORD>(nImage), &sprite.rc, sprite.dwFlags);
    return 1;
}

int MenuWidget::DrawSprite() {
    if (nSpriteId >= 0) {
        const MenuSprite &sprite = g_aMenuSprites[nSpriteId];
        DWORD flags = sprite.dwFlags;
        int image = sprite.nImage;
        int y = ConvertMenuCoordinate(fY);
        int x = ConvertMenuCoordinate(fX);
        BlitSurfaceRegion(x, y, static_cast<WORD>(image), &sprite.rc, flags);
    }
    return 1;
}

int MenuWidget::DrawSpriteRegion(const RECT *pRect) {
    DWORD flags = g_aMenuSprites[nSpriteId].dwFlags;
    WORD image = static_cast<WORD>(nSpriteId);
    int y = ConvertMenuCoordinate(fY);
    int x = ConvertMenuCoordinate(fX);
    BlitSurfaceRegion(x, y, image, pRect, flags);
    return 1;
}

int MenuWidget::DrawCurrentImage() {
    if (!bCustomFrameDurations) {
        DrawSprite();
    } else {
        nSpriteId = anSpriteIds[nAnimationFrame];
        DrawSpriteRegion(&pImageRects[nAnimationFrame]);
    }
    return 1;
}

int MenuWidget::Draw() {
    if (dwTertiaryFlags & 1) {
        if (dwFlags & 0x100000) {
            fonts[0].pVTable->DrawText(&fonts[0]);
        } else if (!(dwFlags & 0x200000)) {
            if (!DrawCurrentImage()) {
                return 0;
            }
        }
    }
    return 1;
}

static inline void PlayMenuActionSound(int sample) {
    PlaySoundChannelOneOnce(sample, 0x200020);
    g_dwNetworkMenuState = sample;
}

static inline MenuWidget *LinkedMenuWidget(MenuWidget *pWidget, int index) {
    return pWidget->pScreen->pCurrentPanel->apWidgets[pWidget->anLinkedWidgetIndices[index]];
}

bool MenuWidget::Update() {
    short previousFrame = nAnimationFrame;
    if ((dwFlags & 0x10) || IsBlockedByModalDialog()) {
        Draw();
        return true;
    }
    if (bSliding) {
        UpdateSlideAnimation();
    }
    UpdateBlinkTimer();
    if (pfnUpdate) {
        int result = pfnUpdate(this);
        if (result == -2) {
            return true;
        }
        if (result == -1) {
            return false;
        }
    }
    if (UpdateHoverState() && !g_dwMenuTextEditActive) {
        unsigned flags = dwFlags;
        if ((flags & 0x400000) && (g_pMouseDevice->abHeld[0] || g_pMouseDevice->abHeld[1])) {
            nSpriteId = anSpriteIds[1];
        }
        if (flags & 2) {
            if (pfnAction) {
                if (pfnAction == CarSetupUI_OnAcceptAndSave) {
                    PlayMenuActionSound(2);
                } else if (pfnAction != AudioOptionsUI_OnCycleSfxVolume) {
                    PlayMenuActionSound(1);
                }
                if (!pfnAction(this)) {
                    return false;
                }
                if (pfnAction == AudioOptionsUI_OnCycleSfxVolume) {
                    PlayMenuActionSound(1);
                }
                g_dwLastWmCharParam = 0;
            }
            dwFlags &= ~2u;
        } else if (pfnIdleAction) {
            pfnIdleAction(this);
        }
        if ((dwFlags & 0x10000) && nLinkedWidgetCount > 0) {
            for (int i = 0; i < nLinkedWidgetCount; i++) {
                MenuWidget *linked = LinkedMenuWidget(this, i);
                linked->dwFlags |= 0x20000;
                linked->nAnimationFrame = previousFrame;
                linked->nAnimationElapsed = 0;
                linked->nAnimationLastTime = 0;
                rcBounds.right =
                    static_cast<int>(static_cast<unsigned>(rcBounds.right) + linked->nWidth);
                linked->Update();
                LinkedMenuWidget(this, i)->dwFlags &= ~0x20000u;
            }
            Draw();
            return true;
        }
    } else {
        unsigned flags = dwFlags;
        if (flags & 0x400000) {
            nSpriteId = anSpriteIds[0];
        }
        if (!(flags & (4 | 0x20000))) {
            nAnimationFrame = 0;
            if ((flags & 0x100000) && !(flags & 0x10) && pfnIdleAction) {
                nSpriteId = 3;
                fonts[0].bStyle = 1;
                Draw();
                return true;
            }
        } else if (pfnIdleAction) {
            pfnIdleAction(this);
        }
    }
    Draw();
    return true;
}

int MenuItemList::UpdateItemAndNotify(int nIndex) {
    MenuFont *font = &fonts[nIndex];
    MenuFontVTable *vtable = font->pVTable;
    vtable->MeasureText(font);
    if (!font->bDrawCaret) {
        return 0;
    }
    signed char result = vtable->HandleTextInput(font);
    if (result <= 0) {
        g_dwMenuTextEditActive = 0;
        font->bDrawCaret = 0;
        if (pfnItemChanged) {
            pfnItemChanged(this, result < 0 ? -1 : nIndex);
        }
    }
    return 1;
}

MenuPanel::MenuPanel() {
    nFlags = 0;
    wWidgetCount = 0;
    fX = 0;
    fY = 0;
    pfnBeforeWidgets = nullptr;
    pfnAfterWidgets = nullptr;
    pfnUpdate = nullptr;
    for (int i = 0; i < MENU_PANEL_WIDGET_COUNT; ++i) {
        apWidgets[i] = nullptr;
    }
    pScreen = nullptr;
    wNavigationCount = 0;
    wSelectedIndex = 0;
}

int MenuPanel::RestoreSelection() {
    short index = static_cast<short>(g_wMenuHoverIndex);
    if (index >= static_cast<int>(wNavigationCount)) {
        g_wMenuHoverIndex = 0;
    } else if (index < 0) {
        g_wMenuHoverIndex = static_cast<unsigned short>(wNavigationCount - 1);
    }
    wSelectedIndex = g_wMenuHoverIndex;
    return 1;
}

static inline bool MenuBoundsIncludeCursor(const RECT &bounds, const MouseDevice *pMouse) {
    return bounds.left <= pMouse->nCursorX && bounds.right >= pMouse->nCursorX &&
           bounds.top <= pMouse->nCursorY && bounds.bottom >= pMouse->nCursorY;
}

bool MenuWidget::UpdateHoverState() {
    if (dwFlags & 8) {
        return false;
    }
    if (dwFlags & 0x10) {
        if (g_wMenuHoverIndex == g_wMenuHoverIndexPrev) {
            g_wMenuHoverIndex =
                static_cast<unsigned short>(g_wMenuHoverIndex + g_dwMouseScrollDirection);
            pScreen->pCurrentPanel->RestoreSelection();
        }
        g_wMenuHoverIndexPrev++;
        return false;
    }
    if (dwFlags & 0x80000) {
        if (!IsInputOrEventActive()) {
            dwFlags &= 0xfff7fffd;
        } else if ((dwFlags & 0x80000) && IsInputOrEventActive()) {
            dwFlags |= 0x22;
            return true;
        }
    }
    bool cursorInPanel = MenuBoundsIncludeCursor(g_rcMenuCenterPanel, g_pMousePosition) ||
                         MenuBoundsIncludeCursor(g_rcHoverTopLeft, g_pMousePosition) ||
                         MenuBoundsIncludeCursor(g_rcHoverBottomLeft, g_pMousePosition);
    bool selectedByIndex = !cursorInPanel && g_wMenuHoverIndex == g_wMenuHoverIndexPrev;
    g_wMenuHoverIndexPrev++;
    g_dwMenuHoverActive = cursorInPanel;
    if (!MenuBoundsIncludeCursor(rcBounds, g_pMousePosition) && !selectedByIndex) {
        dwFlags &= ~0x22u;
        return false;
    }
    if (selectedByIndex && (g_adwMouseButtonPressed[0] || g_adwMouseButtonPressed[1] ||
                            g_adwMouseButtonPressed[2] || g_adwMouseButtonPressed[3])) {
        return true;
    }
    if (dwFlags & 0x40000) {
        if (g_pMouseDevice->abHeld[0] || g_pMouseDevice->abHeld[1]) {
            dwFlags |= 0x80002;
        } else {
            dwFlags &= ~2u;
        }
    } else if ((!g_dwInNetworkCarSetup &&
                (g_adwKeyPressedEdge[DIK_RETURN] || g_adwKeyPressedEdge[DIK_NUMPADENTER])) ||
               IsInputOrEventActive()) {
        dwFlags |= 2;
    } else {
        dwFlags &= ~2u;
    }
    dwFlags |= 0x20;
    return true;
}

int MenuWidget::UpdateBlinkTimer() {
    if (nImageMode) {
        unsigned now = GetHighResolutionTime();
        unsigned delta = now - static_cast<unsigned>(nAnimationLastTime);
        nAnimationLastTime = static_cast<int>(now);
        nAnimationElapsed = static_cast<int>(static_cast<unsigned>(nAnimationElapsed) + delta);
        if (bCustomFrameDurations) {
            if (nAnimationElapsed > anFrameDurations[nAnimationFrame]) {
                nAnimationElapsed = 0;
                dwFlags |= 0x100;
                itemList.dwFlags |= 0x100;
                nAnimationFrame =
                    static_cast<short>(static_cast<unsigned short>(nAnimationFrame) + 1);
                if (anFrameDurations[nAnimationFrame] <= 0) {
                    nAnimationFrame = 0;
                }
            }
        } else if (nAnimationElapsed > 60) {
            nAnimationElapsed = 0;
            dwFlags |= 0x100;
            itemList.dwFlags |= 0x100;
        }
    }
    return 1;
}

int MenuWidget::InitSlideAnimation(int nDuration) {
    fSlideStep = (static_cast<long double>(fSlideEnd) - fSlideStart) / nDuration;
    fSavedX = fX;
    fSavedY = fY;
    fSavedZ = fZ;
    return 1;
}

long double MenuWidget::ComputeSlideOffset(int nTicks) {
    int square = static_cast<int>(static_cast<unsigned>(nTicks) * static_cast<unsigned>(nTicks));
    return static_cast<long double>(square) * fSlideStep * 0.5f * 0.001f;
}

int MenuWidget::StartSlideAnimation() {
    fX = fSavedX;
    fY = fSavedY;
    fZ = fSavedZ;
    bSliding = 1;
    return 1;
}

static inline bool UpdateMenuSlideAxis(MenuWidget *pWidget,
                                       int bEnabled,
                                       signed char direction,
                                       const float &saved,
                                       const float &target,
                                       float &current) {
    if (!bEnabled || direction == 0) {
        return true;
    }
    if (direction != 1 && direction != -1) {
        return false;
    }
    long double offset = pWidget->ComputeSlideOffset(pWidget->nSlideElapsed);
    long double position = direction == 1 ? saved + offset : saved - offset;
    current = position;
    bool done = direction == 1 ? position >= target : !(position > target);
    if (done) {
        current = target;
    }
    return done;
}

int MenuWidget::UpdateSlideAnimation() {
    if (bSliding) {
        unsigned now = GetHighResolutionTime();
        if (nSlideLastTime <= 0) {
            nSlideLastTime = static_cast<int>(now);
        }
        unsigned delta = now - static_cast<unsigned>(nSlideLastTime);
        nSlideLastTime = static_cast<int>(now);
        nSlideElapsed = static_cast<int>(static_cast<unsigned>(nSlideElapsed) + delta);
        bool doneX =
            UpdateMenuSlideAxis(this, bSlideX, nSlideDirectionX, fSavedX, fSlideTargetX, fX);
        bool doneY =
            UpdateMenuSlideAxis(this, bSlideY, nSlideDirectionY, fSavedY, fSlideTargetY, fY);
        if (doneX && doneY) {
            nSlideElapsed = 0;
            bSliding = 0;
        }
    }
    return 1;
}

void MenuWidget::Initialize(MenuScreen *pOwner,
                            float x,
                            float y,
                            unsigned dwItemFlags,
                            unsigned dwSecondary,
                            unsigned dwTertiary,
                            int nMode,
                            short nImage) {
    pScreen = pOwner;
    dwFlags |= dwItemFlags;
    dwSecondaryFlags |= dwSecondary;
    dwTertiaryFlags |= dwTertiary;
    nImageMode = nMode;
    fY = y;
    fX = x;
    anSpriteIds[0] = nImage;
    nSpriteId = nImage;
}

bool MenuWidget::IsBlockedByModalDialog() {
    if (g_pSaveSeasonMenu && g_pSaveSeasonMenu->pScreen->anPanelActive[1] == 1) {
        if (this != g_pSaveSeasonMenu) {
            g_dwModalDialogBlocking = 1;
            return true;
        }
        return false;
    }
    g_dwModalDialogBlocking = 0;
    return false;
}

MenuScreen::MenuScreen() {
    dwFlags = 0;
    bBuilt = false;
    wPanelCount = 0;
    wCurrentPanel = 0;
    pCurrentPanel = nullptr;
    bRunning = true;
    wUnknownBC = 0;
    for (SaveFileList *&pFiles : apFiles) {
        pFiles = new (std::nothrow) SaveFileList();
        if (pFiles == nullptr) {
            ShowErrorMessage(SAVE_FILE_LIST_ERROR);
        }
    }
    // A list that is not allocated is written to all the same, as in the original.
    for (int i = 0; i < SAVE_FILE_NAME_COUNT; i++) {
        sprintf(apFiles[0]->aszNames[i], GetStringResource(SAVE_FILE_DEFAULT_NAME_STRING), i);
        sprintf(apFiles[1]->aszNames[i], GetStringResource(SAVE_FILE_DEFAULT_NAME_STRING), i);
    }
    memset(anPanelActive, 0, sizeof(anPanelActive));
}

MenuScreen::~MenuScreen() {
    delete apFiles[0];
    apFiles[0] = nullptr;
    delete apFiles[1];
    apFiles[1] = nullptr;
}

// The string of the error shown when the cursor of the mouse is not reset.
#define MOUSE_RESET_ERROR_STRING 0xab

int g_dwMenuScreenActivated;
int g_dwLobbyLaunchHandled;

bool MenuScreen::ActivateMenuScreen() {
    RestoreMenuSelections();
    if (!bBuilt) {
#if TGR_RENDERER == TGR_RENDERER_D3D
        InitMenuDirectDrawSurfaces();
#else
        InitMenuSurfaces();
#endif
        // The original passes g_pFrontEndScratchBuffer as this, and the routine does not read it.
        if (!ResetMouseCursorToCenter(g_hMainWindow)) {
            ShowInfoMessageBox(g_hMainWindow, 0, GetStringResource(MOUSE_RESET_ERROR_STRING));
            exit(1);
        }
        pfnBuildMenu(this);
        bBuilt = true;
        bRunning = true;
    }
    if (g_SeasonResumeState.state == SEASON_RESUME_STATE_RESTART) {
        g_dwSeasonRound = 0;
        memset(g_szSeasonRoundLabel, 0, sizeof(g_szSeasonRoundLabel));
        g_SeasonProgress.round = 0;
    }
    if (g_dwGameMode == GAME_MODE_DEMO && g_dwRaceSessionType == RACE_SESSION_SCROLLING_CREDITS &&
        g_dwSeasonLoaded) {
        ShowRaceResultsScreen();
        g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    }
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        if (g_SeasonResumeState.state == 0 ||
            g_SeasonResumeState.state == SEASON_RESUME_STATE_RESTART) {
            if (!RestoreSeasonProgressState()) {
                return false;
            }
            ShowRaceResultsScreen();
            SaveAutoSaveSeason();
            g_dwSeasonLoaded = true;
        }
    } else if (g_dwGameMode == GAME_MODE_NETWORK) {
        g_dwMenuScreenActivated = true;
        ReturnToNetworkLobbyScreen();
        g_dwMenuScreenActivated = false;
    } else if (g_dwGameMode == GAME_MODE_TIME_ATTACK) {
        g_dwMenuScreenActivated = true;
        EnterTimeAttackMode();
        g_dwMenuScreenActivated = false;
    }
    if (g_dwLaunchedFromLobby && !g_dwLobbyLaunchHandled) {
        g_dwLobbyLaunchHandled = true;
        EnterNetworkLobbyScreen();
    }
    return true;
}

bool MenuScreen::BeginFrame() {
    ClearBackBuffer();
    return true;
}

// The value of g_dwPlayMusicMode while the music is chosen by hand.
#define PLAY_MUSIC_MODE_MANUAL 2
// The number of frames between two reads of the music track.
#define MUSIC_TRACK_POLL_FRAMES 120
// The track of the CD the first music track is.
#define FIRST_MUSIC_CD_TRACK 2
// The bit of dwFlags that stops EndFrame().
#define MENU_SCREEN_FLAG_NO_END_FRAME 0x10

bool MenuScreen::EndFrame() {
    MenuScreen *pActiveScreen;
    MenuWidget *pCursor;
    bool bReadTrack;

    if (dwFlags & MENU_SCREEN_FLAG_NO_END_FRAME) {
        return false;
    }
    bReadTrack = false;
    if (g_dwPlayMusicMode == PLAY_MUSIC_MODE_MANUAL) {
        bReadTrack = true;
    } else {
        if (g_dwFrontEndFrameCounter % MUSIC_TRACK_POLL_FRAMES == 0) {
            bReadTrack = true;
        }
        g_dwFrontEndFrameCounter++;
    }
    if (bReadTrack) {
        g_dwSelectedCdTrack = GetCurrentMusicTrack() - FIRST_MUSIC_CD_TRACK;
    }
    // The cursor belongs to the root screen, which is active while the cursor runs.
    pActiveScreen = g_pActiveMenuScreen;
    g_pActiveMenuScreen = g_pRootMenuScreen;
    pCursor = g_pRootMenuScreen->apPanels[0]->apWidgets[MENU_CURSOR_WIDGET_INDEX];
    pCursor->fX = static_cast<float>(g_pMouseDevice->nCursorX);
    pCursor->fY = static_cast<float>(g_pMouseDevice->nCursorY);
    pCursor->Update();
    g_pActiveMenuScreen = pActiveScreen;
    if (!g_dwSessionListRefreshing) {
        FlipAndWait();
    }
    return true;
}

// Save the configuration and shut the front end down as the screen stops running.
static inline void StopMenuScreen(MenuScreen *pScreen) {
    BackupMenuSelections();
    g_GlobalRallyConfig.SaveConfigToFile(g_szConfigFilePath);
    pScreen->wCurrentPanel = 0;
    pScreen->ShutdownFrontEnd(false);
}

bool MenuScreen::RunFrame() {
    MenuScreen *pActiveScreen;

    if (!bRunning) {
        StopMenuScreen(this);
        return false;
    }
    BeginFrame();
    pActiveScreen = g_pActiveMenuScreen;
    g_pActiveMenuScreen = g_pRootMenuScreen;
    // The original passes g_pFrontEndScratchBuffer as this, and the routine does not read it.
    UpdateMouseInput();
    g_pActiveMenuScreen = pActiveScreen;
    PollKeyboardState();
    g_dwFrontEndAtRootMenu = g_pActiveMenuScreen == g_pRootMenuScreen;
    wCurrentPanel = 0;
    for (int i = 0; i < wPanelCount; i++) {
        pCurrentPanel = apPanels[i];
        if (pCurrentPanel == nullptr) {
            return false;
        }
        wCurrentPanel = static_cast<unsigned short>(i);
        if (anPanelActive[i] && !pCurrentPanel->RunFrame()) {
            return false;
        }
    }
    EndFrame();
    if (!bRunning) {
        StopMenuScreen(this);
        return false;
    }
    return true;
}

#if TGR_RENDERER == TGR_RENDERER_D3D
// The red of the background of the menus, 64 of 255.
#define MENU_BACKGROUND_RED (64.0f / 255.0f)

HRESULT MenuScreen::ClearBackBuffer() {
    DirectDrawRenderer *pRenderer = g_pRenderHost->pRenderer;
    IDirect3DViewport2 *pViewport = pRenderer->pViewport;
    D3DMaterial *pMaterial;
    D3DRECT rect;

    if (pRenderer->pDirect3D2 == nullptr || pRenderer->pD3DDevice == nullptr ||
        pViewport == nullptr) {
        return E_FAIL;
    }
    rect.x1 = 0;
    rect.y1 = 0;
    rect.x2 = g_dwConfigFramebufferWidth;
    rect.y2 = g_dwConfigFramebufferHeight;
    pMaterial = pRenderer->pMaterial;
    pMaterial->desc.dcvDiffuse.r = MENU_BACKGROUND_RED;
    pMaterial->desc.dcvDiffuse.g = 0.0f;
    pMaterial->desc.dcvDiffuse.b = 0.0f;
    pMaterial->bDirty = 1;
    pMaterial->bDirty = 0;
    pMaterial->desc.dwRampSize = 1;
    pMaterial->pMaterial->SetMaterial(&pMaterial->desc);
    pRenderer->pViewport->SetBackground(pMaterial->hMaterial);
    pViewport->Clear(1, &rect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
    return 0;
}
#else
HRESULT MenuScreen::ClearBackBuffer() {
    return 0;
}
#endif

int g_dwMenuSurfaceInitCount;

bool ReleaseMenuSurfacesAndFail(HWND hWnd) {
    ReleaseMenuSurfaces();
    exit(0);
    return false;
}

#if TGR_RENDERER == TGR_RENDERER_D3D
IDirectDraw2 *g_pMenuDirectDraw;

// The image of the mouse cursor, which BlitSurfaceRegion() clips to the screen of the menus.
#define MENU_IMAGE_CURSOR 1
// The size of the screen of the menus.
#define MENU_SCREEN_WIDTH 640
#define MENU_SCREEN_HEIGHT 480
// The colour key of the menu images.
#define MENU_IMAGE_COLOR_KEY RGB(0, 0xff, 0)

bool RestoreMenuSurfaces(void) {
    for (int i = 0; i < g_wMenuImageLoadedCount; i++) {
        if (g_aMenuImages[i].pSurface->Restore() == DD_OK) {
            DDReLoadBitmap(g_aMenuImages[i].pSurface, g_aMenuImages[i].pszPath);
        }
    }
    return true;
}

void BlitSurfaceRegion(int nDestX, int nDestY, WORD wImage, const RECT *pRect, DWORD dwFlags) {
    RECT rc;
    HRESULT hr;

    dwFlags |= DDBLTFAST_WAIT;
    do {
        rc = *pRect;
        if (wImage == MENU_IMAGE_CURSOR) {
            if (rc.right + nDestX > MENU_SCREEN_WIDTH) {
                rc.right += MENU_SCREEN_WIDTH - (rc.right + nDestX);
                if (rc.right < rc.left) {
                    rc.right = rc.left;
                }
            }
            if (rc.bottom + nDestY > MENU_SCREEN_HEIGHT) {
                rc.bottom += MENU_SCREEN_HEIGHT - (rc.bottom + nDestY);
                if (rc.bottom < rc.top) {
                    rc.bottom = rc.top;
                }
            }
        }
        hr = g_pRenderHost->pRenderer->pBackBuffer->BltFast(
            nDestX, nDestY, g_aMenuImages[wImage].pSurface, &rc, dwFlags);
        if (hr == DDERR_SURFACELOST) {
            RestoreMenuSurfaces();
            return;
        }
    } while (hr == DDERR_WASSTILLDRAWING);
}

bool InitMenuDirectDrawSurfaces(void) {
    DDSURFACEDESC desc;
    char szMessage[0x100];

    g_pMenuDirectDraw = g_pRenderHost->pRenderer->pDirectDraw2;
    if (g_dwMenuSurfaceInitCount != 0) {
        g_aMenuImages[0].pSurface =
            DDLoadBitmap(g_pMenuDirectDraw, "images\\loading.bmp", 0, 0, &desc);
        if (g_aMenuImages[0].pszPath != nullptr && g_aMenuImages[0].pSurface == nullptr) {
            // The message is formatted and not shown.
            sprintf(szMessage, "DDraw_DoInit: loading.bmp failed to load!");
            return ReleaseMenuSurfacesAndFail(g_hMainWindow);
        }
        g_pActiveMenuScreen->ClearBackBuffer();
        BlitSurfaceRegion(0, 0, 0, &g_aMenuSprites[0].rc, g_aMenuSprites[0].dwFlags);
        g_pActiveMenuScreen->FlipAndWait();
        if (g_aMenuImages[0].pSurface != nullptr) {
            g_aMenuImages[0].pSurface->Release();
            g_aMenuImages[0].pSurface = nullptr;
        }
    }
    g_dwMenuSurfaceInitCount++;
    for (int i = 0; i < MENU_IMAGE_COUNT; i++) {
        MenuImage &image = g_aMenuImages[i];

        if (image.pszPath == nullptr) {
            continue;
        }
        image.pSurface = DDLoadBitmap(g_pMenuDirectDraw, image.pszPath, 0, 0, &desc);
        // The path is checked a second time, as in the original.
        if (image.pszPath != nullptr && image.pSurface == nullptr) {
            sprintf(szMessage, "DDraw_DoInit: Bitmap %d failed to load!", i);
            return ReleaseMenuSurfacesAndFail(g_hMainWindow);
        }
        g_wMenuImageLoadedCount++;
        image.desc = desc;
        DDSetColorKey(image.pSurface, MENU_IMAGE_COLOR_KEY);
    }
    return true;
}

void ReleaseMenuSurfaces(void) {
    if (g_pMenuDirectDraw == nullptr) {
        return;
    }
    for (int i = 0; i < g_wMenuImageLoadedCount; i++) {
        if (g_aMenuImages[i].pSurface != nullptr) {
            g_aMenuImages[i].pSurface->Release();
            g_aMenuImages[i].pSurface = nullptr;
        }
    }
}

void MenuScreen::FlipAndWait() {
    DirectDrawRenderer *pRenderer = g_pRenderHost->pRenderer;

    pRenderer->pPrimarySurface->Flip(pRenderer->pBackBuffer, DDFLIP_WAIT);
    while (pRenderer->pPrimarySurface->GetFlipStatus(DDGFS_ISFLIPDONE) != DD_OK) {
        Sleep(0);
    }
}
#endif

// The delays of the shutdown in milliseconds, in states 2 and 3 of g_dwNetworkMenuState.
#define SHUTDOWN_DELAY_STATE_2_MS 4570
#define SHUTDOWN_DELAY_STATE_3_MS 1540

static inline int ShowCachedMenuScreen(MenuScreen *&pScreen, void (*pfnBuild)(MenuScreen *)) {
    if (!pScreen) {
        pScreen = new (std::nothrow) MenuScreen();
        g_pActiveMenuScreen = pScreen;
        if (!pScreen) {
            return 0;
        }
        pScreen->pfnBuildMenu = pfnBuild;
        pScreen->pfnBuildMenu(pScreen);
        g_pActiveMenuScreen->bBuilt = true;
        g_pActiveMenuScreen->bRunning = true;
    } else {
        g_pActiveMenuScreen = pScreen;
    }
    return 1;
}

int ShowInputOptionsMenu(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_Credits, BuildInputOptionsUI);
}

int SeasonRoundList_SetBrowsingFlag(MenuWidget *) {
    g_bSeasonRoundListBrowsing = 1;
    return 1;
}

int SeasonRoundList_ClearBrowsingFlag(MenuWidget *) {
    g_bSeasonRoundListBrowsing = 0;
    return 1;
}

int CreateGameUI_EnableContinueIfNameEntered(MenuWidget *pWidget) {
    if (strlen(pWidget->fonts[0].szText)) {
        g_pCreateGameContinueItem->dwFlags &= ~0x10u;
    }
    return 1;
}

int PlayerNameEntryUI_EnableContinueIfNameEntered(MenuWidget *pWidget) {
    if (strlen(pWidget->fonts[0].szText)) {
        g_pPlayerNameContinueItem->dwFlags &= ~0x10u;
    }
    return 1;
}

int ModemConnectionUI_EnableContinueIfNumberEntered(MenuWidget *pWidget) {
    if (strlen(pWidget->fonts[0].szText)) {
        g_pModemContinueItem->dwFlags &= ~0x10u;
    }
    return 1;
}

int CarSetupUI_UpdateSteeringIndicatorPos(MenuWidget *pWidget) {
    int position = static_cast<int>(static_cast<unsigned>(g_dwHandlingTypeMenuIndex) * 11u + 61u);
    pWidget->fX = static_cast<float>(position);
    return 1;
}

int AudioOptionsUI_UpdateVolumeChannelImage(MenuWidget *pWidget) {
    short image = g_bAudioOptionsSfxSelected ? 0x68 : 0x69;
    pWidget->anSpriteIds[0] = image;
    pWidget->nSpriteId = image;
    return 1;
}

int AudioOptionsUI_UpdateVolumeLevelIndicatorPos(MenuWidget *pWidget) {
    unsigned volume = g_bAudioOptionsSfxSelected ? g_GlobalRallyConfig.dwSfxVolume :
                                                   g_GlobalRallyConfig.dwMusicVolume;
    int position = static_cast<int>(volume * 8u + 74u);
    pWidget->fX = static_cast<float>(position);
    return 1;
}

static inline void UpdateMenuListSelection(MenuWidget *pWidget, int &selection) {
    MenuItemList *list = &pWidget->itemList;
    int result = list->pVTable->PollSelection(list, selection);
    if (result >= 0) {
        selection = result;
    }
}

#if TGR_RENDERER == TGR_RENDERER_D3D
int D3DDeviceSelectUI_UpdateDeviceSelection(MenuWidget *pWidget) {
    UpdateMenuListSelection(pWidget, g_dwSelectedDisplayDeviceIndex);
    return 1;
}
#endif

int DisplayModeSelectUI_UpdateModeSelection(MenuWidget *pWidget) {
    UpdateMenuListSelection(pWidget, g_dwSelectedDisplayModeIndex);
    return 1;
}

int JoinCreateGameUI_UpdateSessionSelection(MenuWidget *pWidget) {
    UpdateMenuListSelection(pWidget, g_dwSelectedSessionIndex);
    return 1;
}

int KeyConfigUI_UpdateActionSelection(MenuWidget *pWidget) {
    UpdateMenuListSelection(pWidget, g_dwKeyConfigSelectedAction);
    return 1;
}

int SaveSeasonUI_UpdateSlotSelection(MenuWidget *pWidget) {
    MenuItemList *list = &pWidget->itemList;
    MenuItemListVTable *vtable = list->pVTable;
    int selection = vtable->PollSelection(list, g_dwFileListSelectedSlot);
    if (selection >= 0) {
        g_dwFileListSelectedSlot = selection;
    } else {
        selection = g_dwFileListSelectedSlot;
    }
    if (g_dwMenuTextEditActive && selection >= 0) {
        vtable->UpdateItemAndNotify(list, selection);
    }
    return 1;
}

int LoadFileListUI_UpdateSlotSelection(MenuWidget *pWidget) {
    UpdateMenuListSelection(pWidget, g_dwFileListSelectedSlot);
    return 1;
}

int SeasonRoundList_UpdateRoundSelection(MenuWidget *pWidget) {
    UpdateMenuListSelection(pWidget, g_dwSeasonSavedRound);
    return 1;
}

int AudioOptionsUI_UpdateCdTrackSelection(MenuWidget *pWidget) {
    MenuItemList *list = &pWidget->itemList;
    int track = g_dwSelectedCdTrack;
    if (track < 0 || track >= 12) {
        track = -1;
    }
    int result = list->pVTable->PollSelection(list, track);
    if (result >= 0) {
        g_dwSelectedCdTrack = result;
    }
    return 1;
}

int NetworkCarSetupUI_PollChatListSelection(MenuWidget *pWidget) {
    MenuItemList *list = &pWidget->itemList;
    list->pVTable->PollSelection(list, g_dwSelectedSessionIndex);
    return 1;
}

static inline int SynchronizeEditedMenuText(MenuWidget *pWidget, char *destination) {
    ProcessMenuTextEditInput(pWidget, 0);
    const char *text = pWidget->fonts[0].szText;
    if (_stricmp(destination, text)) {
        strcpy(destination, text);
    }
    return 1;
}

int OnChatMessageEdited(MenuWidget *pWidget) {
    return SynchronizeEditedMenuText(pWidget, g_szNetworkChatText);
}

int NetworkCarSetupUI_SendChatMessage(MenuWidget *pWidget) {
    DirectPlayInfo *info = g_pDirectPlayInfo;
    strcpy(g_szNetworkChatText, pWidget->fonts[0].szText);
    SendNetworkChatMessage(g_hMainWindow, info, false);
    strcpy(g_szNetworkChatText, g_szEmptyUiString);
    strcpy(pWidget->fonts[0].szText, g_szEmptyUiString);
    return 1;
}

int OnTcpIpAddressEdited(MenuWidget *pWidget) {
    return SynchronizeEditedMenuText(pWidget, g_GlobalRallyConfig.szTcpIpAddress);
}

int OnTcpIpPortEdited(MenuWidget *pWidget) {
    return SynchronizeEditedMenuText(pWidget, g_GlobalRallyConfig.szTcpIpPort);
}

int ShowVideoOptionsMenu(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_InputConfig, BuildVideoOptionsUI);
}

int ShowKeyConfigScreen(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_KeyConfig, BuildKeyConfigUI);
}

int ShowModemDialUpScreen(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_ModemDialUp, BuildModemConnectionUI);
}

int ShowDisplayModeSelectMenu(void) {
    if (!ShowCachedMenuScreen(g_pCachedMenu_AudioSettings, BuildDisplayModeSelectUI)) {
        return 0;
    }
    PopulateVideoSettingsLists();
    return 1;
}

int PopulateVideoSettingsLists(void) {
    if (!PopulateDisplayDeviceList()) {
        return 0;
    }
    return PopulateDisplayModeList() != 0;
}

#if TGR_RENDERER == TGR_RENDERER_D3D
int ShowD3DDeviceSelectMenu(void) {
    if (!ShowCachedMenuScreen(g_pCachedMenu_VideoSettings, BuildD3DDeviceSelectUI)) {
        return 0;
    }
    PopulateVideoSettingsLists();
    return 1;
}
#endif

int IsGameCdInDrive(int nDrive) {
    char szRoot[MAX_PATH];
    char szVolume[MAX_PATH];
    sprintf(szRoot, "%C:\\", nDrive + '@');
    if (_chdrive(nDrive) != 0 || GetDriveTypeA(nullptr) != DRIVE_CDROM || _chdir("\\") != 0) {
        return 0;
    }
    if (!GetVolumeInformationA(szRoot, szVolume, MAX_PATH, nullptr, nullptr, nullptr, nullptr, 0)) {
        return 0;
    }
    return strcmp(szVolume, "Boss Rally") == 0;
}

int FindGameCdDrive(void) {
    char szDirectory[MAX_PATH];
    int nFound = 0;
    const int nOriginalDrive = _getdrive();
    _getcwd(szDirectory, MAX_PATH);
    for (int nDrive = 3; nDrive <= 26; nDrive++) {
        if (_chdrive(nDrive) == 0 && GetDriveTypeA(nullptr) == DRIVE_CDROM && _chdir("\\") == 0 &&
            IsGameCdInDrive(nDrive)) {
            nFound = 1;
            break;
        }
    }
    _chdrive(nOriginalDrive);
    _chdir(szDirectory);
    return nFound;
}

int ShowChampionshipSeasonMenu(void) {
    if (!FindGameCdDrive()) {
        SetActiveMenuItemText(GetStringResource(13));
        return 0;
    }
    SetActiveMenuItemText(" ");
    return ShowCachedMenuScreen(g_pCachedMenu_TimeTrial, BuildChampionshipSeasonUI);
}

int ShowSaveGhostMenu(int) {
    g_fGhostBestLapTime = g_aVehicles[0].bestLapTime;
    g_fGhostBestRaceTime = g_aVehicles[0].totalTime;
    return ShowCachedMenuScreen(g_pCachedMenu_SaveGhost, BuildSaveTimeAttackMenuUI);
}

static inline void PostSelectionNotice(void) {
    SendNetworkChatMessage(g_hMainWindow, g_pDirectPlayInfo, true);
    strcpy(g_szNetworkChatText, "");
}

template <typename Available>
static inline void CycleAvailableMenuChoice(
    int &choice, int count, int step, Available available, bool skipWrappedStop) {
    choice += step;
    if (choice >= count) {
        choice = 0;
    } else if (choice < 0) {
        choice = count - 1;
    }
    const int first = choice;
    if (available(choice)) {
        return;
    }
    for (;;) {
        choice += step;
        bool wrapped = false;
        if (choice >= count) {
            choice = 0;
            wrapped = true;
        } else if (choice < 0) {
            choice = count - 1;
            wrapped = true;
        }
        /* Track cycling skips the stop comparison on the wrapped iteration. */
        if ((!skipWrappedStop || !wrapped) && choice == first) {
            return;
        }
        if (available(choice)) {
            return;
        }
    }
}

int CarSetupUI_OnCycleCar(MenuActionWidget *) {
    if (g_adwMouseButtonPressed[1]) {
        CycleAvailableMenuChoice(g_dwCarMenuIndex, 32, 1, IsCarSelectable, false);
    } else if (g_adwMouseButtonPressed[0]) {
        CycleAvailableMenuChoice(g_dwCarMenuIndex, 32, -1, IsCarSelectable, false);
    }
    g_dwChosenCar = g_adwCarMenuIndexToCarId[g_dwCarMenuIndex];
    if (g_DirectPlayInfo.pDirectPlay) {
        int car = g_dwCarMenuIndex;
        if (car > 15) {
            car -= 16;
        }
        sprintf(g_szNetworkChatText,
                GetStringResource(0xb7),
                GetStringResource(g_adwCarNameStringIds[car]));
        PostSelectionNotice();
    }
    return 1;
}

int RaceSetupUI_OnCycleTrack(MenuActionWidget *) {
    const int count = g_dwCheatBonusTracksUnlocked ? 15 : 12;
    if (g_adwMouseButtonPressed[1]) {
        CycleAvailableMenuChoice(g_dwTrackMenuIndex, count, 1, IsTrackSelectable, true);
    } else if (g_adwMouseButtonPressed[0]) {
        CycleAvailableMenuChoice(g_dwTrackMenuIndex, count, -1, IsTrackSelectable, true);
    }
    g_dwSelectedTrackId = g_adwTrackMenuIndexToTrackId[g_dwTrackMenuIndex];
    if (g_DirectPlayInfo.pDirectPlay) {
        sprintf(g_szNetworkChatText,
                GetStringResource(0xb8),
                GetStringResource(g_adwTrackNameStringIds[g_dwSelectedTrackId]));
        if (g_apTrackMenuItems[g_dwSelectedTrackId]->flags & 0x10) {
            strcat(g_szNetworkChatText, GetStringResource(0xb0));
        }
        PostSelectionNotice();
    }
    return 1;
}

int RaceSetupUI_OnCycleLapCount(MenuActionWidget *) {
    if (g_adwMouseButtonPressed[1]) {
        if (++g_dwTotalLaps > 12) {
            g_dwTotalLaps = 1;
        }
    } else if (g_adwMouseButtonPressed[0]) {
        if (--g_dwTotalLaps < 1) {
            g_dwTotalLaps = 12;
        }
    }
    g_dwTotalLapsSetting = g_dwTotalLaps;
    if (g_DirectPlayInfo.pDirectPlay) {
        char number[32];
        _itoa(g_dwTotalLaps, number, 10);
        sprintf(g_szNetworkChatText, GetStringResource(0xb9), number);
        PostSelectionNotice();
    }
    return 1;
}

int RaceSetupUI_OnCycleWeather(MenuActionWidget *) {
    if (g_adwMouseButtonPressed[1]) {
        if (++g_dwSelectedWeather > 4) {
            g_dwSelectedWeather = 0;
        }
    } else if (g_adwMouseButtonPressed[0]) {
        if (--g_dwSelectedWeather < 0) {
            g_dwSelectedWeather = 4;
        }
    }
    g_dwChosenWeather = g_adwWeatherMenuIndexToWeather[g_dwSelectedWeather];
    if (g_DirectPlayInfo.pDirectPlay) {
        sprintf(g_szNetworkChatText,
                GetStringResource(0xba),
                GetStringResource(g_adwWeatherLabelStrIds[g_dwChosenWeather]));
        PostSelectionNotice();
    }
    return 1;
}

int ShowTimeAttackMenu(int) {
    SetActiveMenuItemText(" ");
    return ShowCachedMenuScreen(g_pCachedMenu_TimeAttack, BuildTimeAttackMenuUI);
}

int ShowLoadGhostMenu(void) {
    g_fGhostBestRaceTime = 0.0f;
    g_fGhostBestLapTime = 0.0f;
    return ShowCachedMenuScreen(g_pCachedMenu_LoadGhost, BuildLoadTimeAttackMenuUI);
}

int ShowHostRaceSetupScreen(void) {
    g_dwLobbyIsHost = 1;
    g_dwNetworkPlayMode = 2;
    g_dwSessionEnumPending = 0;
    if (!ShowCachedMenuScreen(g_pCachedMenu_RaceSetupNetwork, BuildMultiplayerGameSetupUI)) {
        return 0;
    }
    g_pMultiplayerGameSetupMenu->pfnAction = MultiplayerGameSetupUI_OnBackAsHost;
    return 1;
}

int ShowNetworkCarSetupScreen(void) {
    if (!ShowCachedMenuScreen(g_pCachedMenu_NetworkCarSetup, BuildNetworkCarSetupUI)) {
        return 0;
    }
    g_dwGameMode = GAME_MODE_NETWORK;
    if (g_dwLobbyIsHost) {
        if (!g_dwNetworkCarSetupShown) {
            if (g_dwServiceProviderIndex == 2 || g_dwServiceProviderIndex == 3) {
                InitServiceProviderAndStartTimer();
            }
            HostNetworkSession();
            g_dwNetworkCarSetupShown = 1;
        }
        UpdateNetworkSessionSettings();
    }
    return 1;
}

int ShowMultiplayerNameScreen(void) {
    SetActiveMenuItemText(" ");
    ResolveRaceSelections();
    return ShowCachedMenuScreen(g_pCachedMenu_MultiplayerName, BuildPlayerNameEntryUI);
}

int ShowJoinCreateGameScreen(void) {
    if (!ShowCachedMenuScreen(g_pCachedMenu_JoinCreateGame, BuildJoinCreateGameUI)) {
        return 0;
    }
    if (!g_dwDirectPlaySessionActive && !g_dwLaunchedFromLobby &&
        (g_dwServiceProviderIndex == 0 || g_dwServiceProviderIndex == 1)) {
        InitializeNetworkServiceProvider();
    }
    return 1;
}

int ShowSeasonProgressInfoScreen(MenuWidget *) {
    g_dwSeasonSavedRound = g_dwSeasonRound;
    return ShowCachedMenuScreen(g_pCachedMenu_SeasonProgressInfo, BuildSeasonProgressInfoUI);
}

int ShowRaceSetupMenu(void) {
    bool wasCached;
    SetActiveMenuItemText(" ");
    g_dwGameMode = GAME_MODE_ARCADE;
    ResetMenuSelectionsToDefaults();
    wasCached = g_pCachedMenu_MainMenu != nullptr;
    if (!ShowCachedMenuScreen(g_pCachedMenu_MainMenu, BuildRaceSetupUI)) {
        return 0;
    }
    if (!wasCached) {
        NullStub();
        ResetGameModeConfig();
        ResolveRaceSelections();
    }
    return 1;
}

int ReturnToMainMenuFromSeason(void) {
    g_dwGameMode = GAME_MODE_TIME_ATTACK;
    ResetMenuSelectionsToDefaults();
    bool wasCached = g_pCachedMenu_MainMenu != nullptr;
    g_pPrimarySeasonSave = nullptr;
    g_LoadedSeasonSettings.car = 1;
    g_dwGameMode = GAME_MODE_TIME_ATTACK;
    g_bRaceSetupShowCarSetup = 1;
    g_GhostRecordingInfo.track = -1;
    if (!ShowCachedMenuScreen(g_pCachedMenu_MainMenu, BuildRaceSetupUI)) {
        return 0;
    }
    if (!wasCached) {
        NullStub();
        ResetGameModeConfig();
        ResolveRaceSelections();
        g_pRaceSetupMenu->pfnAction = RaceSetupUI_OnBackToTimeAttack;
    }
    return 1;
}

int ShowQuitGameConfirmMenu(void) {
    SetActiveMenuItemText(" ");
    PlaySoundChannelOneOnce(3, 0x200020);
    g_dwNetworkMenuState = 3;
    return ShowCachedMenuScreen(g_pCachedMenu_MainMenuAlternate, BuildQuitGameConfirmUI);
}

int ShowOptionsMenu(void) {
    SetActiveMenuItemText(" ");
    return ShowCachedMenuScreen(g_pCachedMenu_Options, BuildOptionsMenuUI);
}

static inline int ReturnFromOptionsMenu(MenuActionWidget *pItem, MenuScreen *&pTarget) {
    pItem->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    g_pCachedMenu_Options = nullptr;
    g_pActiveMenuScreen = pTarget;
    return 0;
}

static inline int ReturnFromSaveSeasonMenu(MenuActionWidget *pItem, MenuScreen *&pTarget) {
    pItem->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    g_pCachedMenu_SaveSeason = nullptr;
    g_pSaveSeasonMenu = nullptr;
    g_pSaveSeasonEditMenu = nullptr;
    g_dwSaveSeasonSlotNamed = 0;
    g_dwFileListSelectedSlot = -1;
    strcpy(g_szSeasonYearLabel, "");
    strcpy(g_szSeasonRoundLabel, "");
    g_pActiveMenuScreen = pTarget;
    return 0;
}

static inline int
ReturnFromCachedMenu(MenuActionWidget *pItem, MenuScreen *&pCached, MenuScreen *&pTarget) {
    MenuScreen *pDestination;
    pItem->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    pDestination = pTarget;
    pCached = nullptr;
    g_pActiveMenuScreen = pDestination;
    return 0;
}

int OptionsMenuUI_OnBack(MenuActionWidget *pItem) {
    return ReturnFromOptionsMenu(pItem, g_pRootMenuScreen);
}

int OptionsMenuUI_OnBackToCarSetup(MenuActionWidget *pItem) {
    return ReturnFromOptionsMenu(pItem, g_pCachedMenu_CarSetup);
}

int OptionsMenuUI_OnBackToPreRaceInfo(MenuActionWidget *pItem) {
    return ReturnFromOptionsMenu(pItem, g_pCachedMenu_PreRaceInfo);
}

int OptionsMenuUI_OnBackToSeasonProgressInfo(MenuActionWidget *pItem) {
    return ReturnFromOptionsMenu(pItem, g_pCachedMenu_SeasonProgressInfo);
}

int SaveSeasonUI_OnBackToCarSetup(MenuActionWidget *pItem) {
    return ReturnFromSaveSeasonMenu(pItem, g_pCachedMenu_CarSetup);
}

int SaveSeasonUI_OnBackToPreRaceInfo(MenuActionWidget *pItem) {
    return ReturnFromSaveSeasonMenu(pItem, g_pCachedMenu_PreRaceInfo);
}

int SaveSeasonUI_OnBackToSeasonProgressInfo(MenuActionWidget *pItem) {
    return ReturnFromSaveSeasonMenu(pItem, g_pCachedMenu_SeasonProgressInfo);
}

int SaveSeasonUI_OnBack(MenuActionWidget *pItem) {
    return ReturnFromSaveSeasonMenu(pItem, g_pCachedMenu_RaceResults);
}

int ChampionshipSeasonUI_OnBack(MenuActionWidget *pItem) {
    return ReturnFromCachedMenu(pItem, g_pCachedMenu_TimeTrial, g_pRootMenuScreen);
}

int GameOptionsUI_OnBack(MenuActionWidget *pItem) {
    return ReturnFromCachedMenu(pItem, g_pCachedMenu_Championship, g_pCachedMenu_Options);
}

int InputOptionsUI_OnBack(MenuActionWidget *pItem) {
    return ReturnFromCachedMenu(pItem, g_pCachedMenu_GameplaySettings, g_pCachedMenu_Options);
}

int LoadTimeAttackMenuUI_OnBack(MenuActionWidget *pItem) {
    return ReturnFromCachedMenu(pItem, g_pCachedMenu_LoadGhost, g_pCachedMenu_TimeAttack);
}

int MultiplayerConnectionUI_OnBack(MenuActionWidget *pItem) {
    return ReturnFromCachedMenu(
        pItem, g_pCachedMenu_MultiplayerConnection, g_pCachedMenu_MultiplayerName);
}

int QuitGameConfirmUI_OnBack(MenuActionWidget *pItem) {
    return ReturnFromCachedMenu(pItem, g_pCachedMenu_MainMenuAlternate, g_pRootMenuScreen);
}

int RaceResultsTotalsUI_OnBack(MenuActionWidget *pItem) {
    return ReturnFromCachedMenu(
        pItem, g_pCachedMenu_RaceResultsTotals, g_pCachedMenu_RaceResultsSummary);
}

int SeasonProgressInfoUI_OnBack(MenuActionWidget *pItem) {
    return ReturnFromCachedMenu(pItem, g_pCachedMenu_SeasonProgressInfo, g_pCachedMenu_PreRaceInfo);
}

int TimeAttackMenuUI_OnBack(MenuActionWidget *pItem) {
    return ReturnFromCachedMenu(pItem, g_pCachedMenu_TimeAttack, g_pRootMenuScreen);
}

int VideoOptionsUI_OnBack(MenuActionWidget *pItem) {
    return ReturnFromCachedMenu(pItem, g_pCachedMenu_InputConfig, g_pCachedMenu_Options);
}

int PreRaceInfoUI_OnBackToLoadSeason(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    pItem->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    pDestination = g_pCachedMenu_LoadSeason;
    g_pCachedMenu_PreRaceInfo = nullptr;
    g_pExitSeasonConfirmMenu = nullptr;
    g_pPreRaceInfoMenuScreen = nullptr;
    g_pActiveMenuScreen = pDestination;
    return 0;
}

int PreRaceInfoUI_OnBackToChampionshipSeason(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    pItem->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    pDestination = g_pCachedMenu_TimeTrial;
    g_pCachedMenu_PreRaceInfo = nullptr;
    g_pExitSeasonConfirmMenu = nullptr;
    g_pPreRaceInfoMenuScreen = nullptr;
    g_pActiveMenuScreen = pDestination;
    return 0;
}

int CarSetupUI_OnBackToSeasonProgressInfo(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    pItem->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    pDestination = g_pCachedMenu_SeasonProgressInfo;
    g_pCachedMenu_CarSetup = nullptr;
    g_pCarSetupMenu = nullptr;
    g_pActiveMenuScreen = pDestination;
    return 0;
}

int CarSetupUI_OnBackToTimeAttack(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    pItem->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    pDestination = g_pCachedMenu_TimeAttack;
    g_pCachedMenu_CarSetup = nullptr;
    g_pCarSetupMenu = nullptr;
    g_pActiveMenuScreen = pDestination;
    return 0;
}

int PlayerNameEntryUI_OnBack(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    pItem->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    pDestination = g_pRootMenuScreen;
    g_pCachedMenu_MultiplayerName = nullptr;
    g_pPlayerNameContinueItem = nullptr;
    g_pActiveMenuScreen = pDestination;
    return 0;
}

int RaceSetupUI_OnBack(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    pItem->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    pDestination = g_pRootMenuScreen;
    g_pCachedMenu_MainMenu = nullptr;
    g_pRaceSetupMenu = nullptr;
    g_pActiveMenuScreen = pDestination;
    return 0;
}

int RaceSetupUI_OnBackToTimeAttack(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    pItem->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    pDestination = g_pCachedMenu_TimeAttack;
    g_pCachedMenu_MainMenu = nullptr;
    g_pRaceSetupMenu = nullptr;
    g_pActiveMenuScreen = pDestination;
    return 0;
}

int ModemConnectionUI_OnBack(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    pItem->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    pDestination = g_pCachedMenu_MultiplayerConnection;
    g_pCachedMenu_ModemDialUp = nullptr;
    g_pModemContinueItem = nullptr;
    g_pActiveMenuScreen = pDestination;
    return 0;
}

int LoadSeasonUI_OnBack(MenuActionWidget *pItem) {
    pItem->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    g_pCachedMenu_LoadSeason = nullptr;
    g_dwLoadSeasonSlotValid = 0;
    g_dwFileListSelectedSlot = -1;
    strcpy(g_szSeasonYearLabel, "");
    strcpy(g_szSeasonRoundLabel, "");
    g_pActiveMenuScreen = g_pCachedMenu_TimeTrial;
    return 0;
}

int SaveTimeAttackMenuUI_OnBack(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    pItem->pScreen->DestroyPanels();
    delete g_pCachedMenu_SaveGhost;
    pDestination = g_pCachedMenu_TimeAttack;
    g_pCachedMenu_SaveGhost = nullptr;
    g_pActiveMenuScreen = pDestination;
    g_dwGameMode = GAME_MODE_TIME_ATTACK;
    return 0;
}

int KeyConfigUI_OnBack(MenuActionWidget *pItem) {
    ReturnFromCachedMenu(pItem, g_pCachedMenu_KeyConfig, g_pCachedMenu_Credits);
    ReleaseJoystickDevice();
    return 0;
}

int CarSetupUI_UpdateTransmissionImage(MenuWidget *pWidget) {
    pWidget->nSpriteId = g_abTransmissionImageIds[g_dwSelectedTransmission];
    return 1;
}

int CarSetupUI_UpdateTiresImage(MenuWidget *pWidget) {
    pWidget->nSpriteId = g_abTireImageIds[g_dwTireMenuIndex];
    return 1;
}

int CarSetupUI_UpdateShocksImage(MenuWidget *pWidget) {
    pWidget->nSpriteId = g_abSuspensionImageIds[g_dwSuspensionMenuIndex];
    return 1;
}

int InputOptionsUI_UpdateInputDeviceImage(MenuWidget *pWidget) {
    pWidget->nSpriteId = g_abInputDeviceImageIds[g_dwSelectedInputDevice];
    return 1;
}

int ConnectionUI_UpdateServiceProviderImage(MenuWidget *pWidget) {
    pWidget->nSpriteId = g_abServiceProviderImageIds[g_dwServiceProviderIndex];
    return 1;
}

int GameOptionsUI_UpdateCarShadowImage(MenuWidget *pWidget) {
    pWidget->nSpriteId = g_abCarShadowImageIds[g_dwOptionCarShadow];
    return 1;
}

int GameOptionsUI_UpdateSpecularLightingImage(MenuWidget *pWidget) {
    pWidget->nSpriteId = g_abSpecularLightingImageIds[g_dwOptionSpecularLighting];
    return 1;
}

int GameOptionsUI_UpdateSkidMarksImage(MenuWidget *pWidget) {
    pWidget->nSpriteId = static_cast<short>(g_awSkidMarksImageIds[g_dwOptionSkidMarks]);
    return 1;
}

int RaceResultsInfoUI_OnSelectLap1(MenuWidget *) {
    g_dwRaceResultsSelectedLap = 0;
    return 1;
}

int RaceResultsInfoUI_OnSelectLap2(MenuWidget *) {
    g_dwRaceResultsSelectedLap = 1;
    return 1;
}

int RaceResultsInfoUI_OnSelectLap3(MenuWidget *) {
    g_dwRaceResultsSelectedLap = 2;
    return 1;
}

int RaceResultsInfoUI_OnSelectTotalTime(MenuWidget *) {
    g_dwRaceResultsSelectedLap = 3;
    return 1;
}

int SaveSeasonUI_UpdateItemEnabledState(MenuWidget *pWidget) {
    if (g_dwSaveSeasonSlotNamed) {
        pWidget->dwFlags &= ~0x1010u;
    }
    return 1;
}

int PreRaceExitSeasonConfirmUI_OnNo(MenuWidget *) {
    g_pActiveMenuScreen = g_pCachedMenu_PreRaceInfo;
    return 0;
}

int ResultsSummaryExitSeasonConfirmUI_OnNo(MenuWidget *) {
    g_pActiveMenuScreen = g_pCachedMenu_RaceResultsTotals;
    return 0;
}

int MenuScreen_OnReturnToMainMenu(MenuWidget *pWidget) {
    pWidget->pScreen->ShutdownFrontEnd(1);
    g_pActiveMenuScreen = g_pCachedMenu_MainMenu;
    return 0;
}

int PreRaceInfoUI_OnOptions(MenuWidget *) {
    ShowOptionsMenu();
    g_pOptionsSubMenu->pfnAction = OptionsMenuUI_OnBackToPreRaceInfo;
    return 1;
}

int PreRaceInfoUI_OnSaveSeason(MenuWidget *) {
    ShowSaveSeasonScreen();
    g_pSaveSeasonBackHandlerMenu->pfnAction = SaveSeasonUI_OnBackToPreRaceInfo;
    return 1;
}

int SeasonProgressInfoUI_OnOptions(MenuWidget *) {
    ShowOptionsMenu();
    g_pOptionsSubMenu->pfnAction = OptionsMenuUI_OnBackToSeasonProgressInfo;
    return 1;
}

int SeasonProgressInfoUI_OnSaveSeason(MenuWidget *) {
    ShowSaveSeasonScreen();
    g_pSaveSeasonBackHandlerMenu->pfnAction = SaveSeasonUI_OnBackToSeasonProgressInfo;
    return 1;
}

void SetActiveMenuItemText(const char *pszText) {
    MenuWidget *pWidget = g_pActiveMenuScreen->apPanels[0]->apWidgets[g_dwRootMenuStringIndex];
    if (pWidget) {
        pWidget->SetText(pszText, 1, 1, &g_rcMenuRootTitleBar);
    }
}

static inline void CycleMenuSetting(int &value, int maximum) {
    if (g_adwMouseButtonPressed[1]) {
        value = static_cast<int>(static_cast<unsigned>(value) + 1);
        if (value > maximum) {
            value = 0;
        }
    } else if (g_adwMouseButtonPressed[0]) {
        value = static_cast<int>(static_cast<unsigned>(value) - 1);
        if (value < 0) {
            value = maximum;
        }
    }
}

int CarSetupUI_OnCycleHandlingType(MenuWidget *) {
    CycleMenuSetting(g_dwHandlingTypeMenuIndex, 7);
    g_dwHandlingType = g_dwHandlingTypeMenuIndex;
    return 1;
}

int CarSetupUI_OnCycleTireType(MenuWidget *) {
    CycleMenuSetting(g_dwTireMenuIndex, 2);
    g_dwTireType = g_adwTireMenuIndexToTireType[g_dwTireMenuIndex];
    return 1;
}

int CarSetupUI_OnCycleSuspensionType(MenuWidget *) {
    CycleMenuSetting(g_dwSuspensionMenuIndex, 2);
    g_dwSuspensionType = g_adwSuspensionMenuIndexToType[g_dwSuspensionMenuIndex];
    return 1;
}

int CarSetupUI_OnCycleTransmissionType(MenuWidget *) {
    CycleMenuSetting(g_dwSelectedTransmission, 1);
    g_dwTransmissionType = g_adwTransmissionMenuIndexToType[g_dwSelectedTransmission];
    return 1;
}

int GameOptionsUI_OnToggleForceFeedback(MenuWidget *) {
    CycleMenuSetting(g_dwOptionForceFeedback, 1);
    g_GlobalRallyConfig.bForceFeedbackEnabled =
        g_adwForceFeedbackMenuIndexToValue[g_dwOptionForceFeedback];
    return 1;
}

int GameOptionsUI_OnToggleSkidMarks(MenuWidget *) {
    CycleMenuSetting(g_dwOptionSkidMarks, 1);
    g_GlobalRallyConfig.bSkidMarksSelection = g_adwSkidMarksMenuIndexToValue[g_dwOptionSkidMarks];
    return 1;
}

int GameOptionsUI_OnToggleCarShadow(MenuWidget *) {
    CycleMenuSetting(g_dwOptionCarShadow, 1);
    g_GlobalRallyConfig.bCarShadowSelection = g_adwCarShadowMenuIndexToValue[g_dwOptionCarShadow];
    return 1;
}

int GameOptionsUI_OnToggleSpecularLighting(MenuWidget *) {
    CycleMenuSetting(g_dwOptionSpecularLighting, 1);
    g_GlobalRallyConfig.bSpecularLightingSelection =
        g_adwSpecularLightingMenuIndexToValue[g_dwOptionSpecularLighting];
    return 1;
}

static inline int ToggleSavedMenuText(MenuWidget *pWidget, MenuItemList *&list) {
    pWidget->pScreen->anPanelActive[1] = 0;
    MenuFont &font = list->fonts[g_dwFileListSelectedSlot];
    font.bDrawCaret = !font.bDrawCaret;
    MenuFont &selected = list->fonts[g_dwFileListSelectedSlot];
    g_dwMenuTextEditActive = selected.bDrawCaret;
    if (g_dwMenuTextEditActive) {
        strcpy(g_szMenuItemTextBackup, selected.szText);
        strcpy(selected.szText, g_szEmptyUiString);
    }
    return 1;
}

int ToggleMenuItemTextEditA(MenuWidget *pWidget) {
    return ToggleSavedMenuText(pWidget, g_pSaveSeasonEditMenu);
}

int ToggleMenuItemTextEditB(MenuWidget *pWidget) {
    return ToggleSavedMenuText(pWidget, g_pSaveTimeAttackEditMenu);
}

static inline int RestoreSavedMenuText(MenuWidget *pWidget, MenuItemList *&list, int &named) {
    pWidget->pScreen->anPanelActive[1] = 0;
    named = 0;
    if (g_dwMenuTextEditActive) {
        strcpy(list->fonts[g_dwFileListSelectedSlot].szText, g_szMenuItemTextBackup);
        strcpy(g_szMenuItemTextBackup, g_szEmptyUiString);
    }
    return 1;
}

int RestoreMenuItemTextA(MenuWidget *pWidget) {
    return RestoreSavedMenuText(pWidget, g_pSaveSeasonEditMenu, g_dwSaveSeasonSlotNamed);
}

int RestoreMenuItemTextB(MenuWidget *pWidget) {
    return RestoreSavedMenuText(pWidget, g_pSaveTimeAttackEditMenu, g_dwTimeAttackSaveSlotNamed);
}

static inline int HandleSavedMenuTextEntry(MenuWidget *pWidget,
                                           int (*toggle)(MenuWidget *),
                                           int (*restore)(MenuWidget *)) {
    if (g_adwKeyPressedEdge[DIK_Y]) {
        toggle(pWidget);
    } else if (CheckForUserInput()) {
        restore(pWidget);
    } else {
        return 1;
    }
    g_dwLastWmCharParam = 0;
    return -1;
}

int SaveSeasonUI_HandleTextEntry(MenuWidget *pWidget) {
    return HandleSavedMenuTextEntry(pWidget, ToggleMenuItemTextEditA, RestoreMenuItemTextA);
}

int SaveTimeAttackMenuUI_HandleTextEntry(MenuWidget *pWidget) {
    return HandleSavedMenuTextEntry(pWidget, ToggleMenuItemTextEditB, RestoreMenuItemTextB);
}

int MultiplayerGameSetupUI_OnEscape(MenuWidget *pWidget) {
    if (g_dwNetworkDataStreamActive && g_dwLaunchedFromLobby) {
        pWidget->pScreen->bRunning = 0;
        pWidget->pScreen->ShutdownFrontEnd(0);
        ShutdownApplication(0);
    }
    return 1;
}

int ChampionshipSeasonUI_OnStartNewSeason(MenuWidget *) {
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    ResetMenuSelectionsToDefaults();
    g_pPrimarySeasonSave = nullptr;
    ShowPreRaceInfoScreen();
    g_pExitSeasonConfirmMenu->pfnAction = PreRaceInfoUI_OnBackToChampionshipSeason;
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    ResolveRaceSelections();
    return 1;
}

int ReturnToMultiplayerLobby(MenuWidget *pWidget) {
    pWidget->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    MenuScreen *screen = g_pCachedMenu_RaceSetupNetwork;
    g_pCachedMenu_NetworkCarSetup = nullptr;
    g_pNetworkPlayerListMenu = nullptr;
    g_pNetworkCarSetupMenu = nullptr;
    g_dwInNetworkCarSetup = 0;
    g_pActiveMenuScreen = screen;
    return 0;
}

int ReturnToVideoOptionsFromResolution(MenuWidget *pWidget) {
    pWidget->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    MenuScreen *screen = g_pCachedMenu_InputConfig;
    g_pCachedMenu_AudioSettings = nullptr;
    g_pDisplayModeSelectMenu = nullptr;
    g_pActiveMenuScreen = screen;
    return 0;
}

int ReturnToVideoOptionsFromDeviceSelect(MenuWidget *pWidget) {
    pWidget->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    MenuScreen *screen = g_pCachedMenu_InputConfig;
    g_pCachedMenu_VideoSettings = nullptr;
    g_pD3DDeviceSelectMenu = nullptr;
    g_pActiveMenuScreen = screen;
    return 0;
}

int ModemConnectionUI_OnAccept(MenuWidget *pWidget) {
    g_dwServiceProviderIndex = 2;
    pWidget->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    MenuScreen *screen = g_pCachedMenu_JoinCreateGame;
    g_pCachedMenu_ModemDialUp = nullptr;
    g_pActiveMenuScreen = screen;
    return 0;
}

int SaveSeasonUI_OnSlotNameEntered(MenuItemList *pList, int nStatus) {
    if (!g_dwSeasonLoaded) {
        return 0;
    }
    if (nStatus == -1) {
        strcpy(pList->fonts[g_dwFileListSelectedSlot].szText, g_szMenuItemTextBackup);
        return 1;
    }
    char fileName[260];
    char number[4];
    strcpy(fileName, "RallySeason");
    _itoa(g_dwFileListSelectedSlot, number, 10);
    strcat(fileName, number);
    strcat(fileName, ".brf");
    strcpy(g_szSeasonSaveFileName, fileName);
    strcpy(g_pRootMenuScreen->apFiles[0]->aszNames[g_dwFileListSelectedSlot],
           pList->fonts[g_dwFileListSelectedSlot].szText);
    strcpy(g_szSeasonSaveSlotName, pList->fonts[g_dwFileListSelectedSlot].szText);
    if (!g_pPrimarySeasonSave->season && !g_pPrimarySeasonSave->round) {
        memset(g_pPrimarySeasonSave->seasonPlaces, 0, sizeof(g_pPrimarySeasonSave->seasonPlaces));
        memset(g_pPrimarySeasonSave->seasonRacePoints,
               0,
               sizeof(g_pPrimarySeasonSave->seasonRacePoints));
        memset(g_pPrimarySeasonSave->seasonTimes, 0, sizeof(g_pPrimarySeasonSave->seasonTimes));
    }
    // The slot is marked as named even when writing fails.
    SaveSeasonSaveFile();
    g_dwSaveSeasonSlotNamed = 1;
    return 1;
}

int SaveSeasonUI_OnSlotSelected(MenuItemList *pList, int *pnSlot) {
    int slot = *pnSlot;
    if (slot < 0) {
        return 0;
    }
    g_pSaveSeasonEditMenu = pList;
    g_dwFileListSelectedSlot = slot;
    char fileName[256];
    char number[4];
    strcpy(fileName, "RallySeason");
    _itoa(slot, number, 10);
    strcat(fileName, number);
    strcat(fileName, ".brf");
    FILE *file = fopen(fileName, "r");
    bool missing = file == nullptr;
    if (file) {
        fclose(file);
    }
    PollInputAndUpdateEdges();
    if (missing) {
        ToggleMenuItemTextEditA(g_pSaveSeasonMenu);
    } else {
        g_pSaveSeasonMenu->pScreen->anPanelActive[1] = 1;
    }
    return 1;
}

int PreRaceExitSeasonConfirmUI_OnYes(MenuWidget *pWidget) {
    pWidget->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    g_pActiveMenuScreen = nullptr;
    g_pPreRaceInfoMenuScreen = nullptr;
    if (g_pCachedMenu_PreRaceInfo) {
        delete g_pCachedMenu_PreRaceInfo;
        g_pCachedMenu_PreRaceInfo = nullptr;
    }
    g_pActiveMenuScreen = g_pCachedMenu_MainMenu;
    return 0;
}

static inline void ClearCachedMenuPanels(MenuScreen *&screen) {
    if (screen) {
        screen->DestroyPanels();
        screen = nullptr;
    }
}

int ResultsSummaryExitSeasonConfirmUI_OnYes(MenuWidget *pWidget) {
    ClearCachedMenuPanels(g_pCachedMenu_RaceResults);
    ClearCachedMenuPanels(g_pCachedMenu_RaceResultsSummary);
    ClearCachedMenuPanels(g_pCachedMenu_RaceResultsTotals);
    pWidget->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    g_pPreRaceInfoMenuScreen = nullptr;
    g_pActiveMenuScreen = g_pCachedMenu_MainMenu;
    return 0;
}

int MainMenuUI_OnPlayCredits(MenuWidget *pWidget) {
    SetActiveMenuItemText(" ");
    g_dwGameMode = GAME_MODE_DEMO;
    if (g_dwCheatOutroSequenceEnabled) {
        g_dwRaceSessionType = RACE_SESSION_SCROLLING_CREDITS;
        g_dwSeasonLoaded = 0;
    } else {
        g_dwRaceSessionType = RACE_SESSION_CREDIT_CAPTIONS;
    }
    pWidget->pScreen->bRunning = 0;
    pWidget->pScreen->ShutdownFrontEnd(0);
    return 0;
}

int ShowCarSetupScreen(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_CarSetup, BuildCarSetupUI);
}

int SeasonProgressInfoUI_OnCarSetup(MenuWidget *) {
    g_bRaceSetupShowCarSetup = 0;
    ShowCarSetupScreen();
    g_bRaceSetupShowCarSetup = 1;
    g_pCarSetupMenu->pfnAction = CarSetupUI_OnBackToSeasonProgressInfo;
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    return 1;
}

int LoadTimeAttackMenuUI_OnCarSetup(MenuWidget *) {
    g_bRaceSetupShowCarSetup = 0;
    ShowCarSetupScreen();
    g_bRaceSetupShowCarSetup = 1;
    g_pCarSetupMenu->pfnAction = CarSetupUI_OnBackToTimeAttack;
    g_dwGameMode = GAME_MODE_TIME_ATTACK;
    return 1;
}

int NetworkCarSetupUI_OnBootPlayer(MenuWidget *) {
    if (g_pDirectPlayInfo->dpidPlayer != g_dwNetCarSetupSelectedPlayerId) {
        SendNetworkMessage4(g_pDirectPlayInfo, g_dwNetCarSetupSelectedPlayerId);
    }
    return 1;
}

int LoadSeasonUI_OnLoadAutoSave(MenuWidget *) {
    LoadAutoSaveSeason();
    ShowPreRaceInfoScreen();
    g_pExitSeasonConfirmMenu->pfnAction = PreRaceInfoUI_OnBackToLoadSeason;
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    return 1;
}

int OnListDialogItemSelected(MenuWidget *pWidget) {
    ShowPreRaceInfoScreen();
    pWidget->pScreen->DestroyPanels();
    delete g_pCachedMenu_NotEnoughPoints;
    g_pCachedMenu_NotEnoughPoints = nullptr;
    return 0;
}

int OnPressAnyKeyContinue(MenuWidget *pWidget) {
    ShowPreRaceInfoScreen();
    if (g_SeasonProgress.championshipFlags > 0 && !g_SeasonProgress.season &&
        !g_SeasonProgress.round) {
        memset(g_SeasonProgress.seasonPlaces, 0, sizeof(g_SeasonProgress.seasonPlaces));
        memset(g_SeasonProgress.seasonRacePoints, 0, sizeof(g_SeasonProgress.seasonRacePoints));
        memset(g_SeasonProgress.seasonTimes, 0, sizeof(g_SeasonProgress.seasonTimes));
    }
    g_dwSeasonTotalPoints = 0;
    pWidget->pScreen->DestroyPanels();
    delete g_pCachedMenu_SeasonCongrats;
    g_pCachedMenu_SeasonCongrats = nullptr;
    return 0;
}

int SeasonCongratsUI_UpdatePrimaryBannerImage(MenuWidget *pWidget) {
    int flags = g_SeasonProgress.championshipFlags;
    if (flags > 0) {
        pWidget->nSpriteId = flags <= 5 ? 0x74 - flags : -1;
    }
    if (g_SeasonProgress.championshipFlags == 0) {
        unsigned season = g_SeasonProgress.season;
        if (season >= 1 && season <= 3) {
            pWidget->nSpriteId = 0x45 + season * 2;
        } else {
            pWidget->nSpriteId = season >= 4 && season <= 6 ? 0x4d : -1;
        }
    }
    return 1;
}

int SeasonCongratsUI_UpdateSecondaryBannerImage(MenuWidget *pWidget) {
    int flags = g_SeasonProgress.championshipFlags;
    if (flags > 0) {
        switch (flags) {
        case 2:
            pWidget->nSpriteId = 0x6d;
            break;
        case 3:
            pWidget->nSpriteId = 0x6e;
            break;
        case 4:
            pWidget->nSpriteId = 0x6c;
            break;
        default:
            pWidget->nSpriteId = -1;
            break;
        }
    }
    if (g_SeasonProgress.championshipFlags == 0) {
        unsigned season = g_SeasonProgress.season;
        pWidget->nSpriteId = season >= 1 && season <= 3 ? 0x46 + season * 2 : -1;
    }
    return 1;
}

int MultiplayerSetupUI_UpdateCarGroupImage(MenuWidget *pWidget) {
    unsigned group = static_cast<unsigned>(g_dwSelectedCarClass);
    pWidget->nSpriteId = group >= 1 && group <= 4 ? 0x55 + group * 2 : 0x56;
    return 1;
}

int MultiplayerSetupUI_UpdateCarGroupSubImage(MenuWidget *pWidget) {
    unsigned group = static_cast<unsigned>(g_dwSelectedCarClass);
    pWidget->nSpriteId = group >= 1 && group <= 4 ? 0x56 + group * 2 : -1;
    return 1;
}

int SeasonCongratsUI_PollAnyKey(MenuWidget *pWidget) {
    if (CheckForUserInput()) {
        OnPressAnyKeyContinue(pWidget);
        return -1;
    }
    return 1;
}

int KeyConfigUI_BeginKeyCapture(MenuWidget *) {
    if (!g_fCharInputSuppressed) {
        g_dwMenuTextEditActive = 1;
        g_fCharInputSuppressed = 1;
        g_dwLastWmCharParam = 0;
        ClearKeyboardStateBuffers();
        PollInputAndUpdateEdges();
    }
    return 1;
}

int ValidateEntityState(MenuWidget *pWidget) {
    unsigned flags = pWidget->dwFlags;
    if ((flags & (0x10 | 0x1000000)) || (pWidget->itemList.dwFlags & 0x1000000)) {
        return 0;
    }
    if (g_dwMenuHoverActive && (g_pMouseDevice->abHeld[0] || g_pMouseDevice->abHeld[1] ||
                                g_pMouseDevice->abHeld[2] || g_pMouseDevice->abHeld[3])) {
        pWidget->fonts[0].bStyle = 4;
        return 1;
    }
    if (flags & 0x100) {
        pWidget->nSpriteId =
            static_cast<short>(static_cast<unsigned short>(pWidget->nSpriteId) + 1);
        switch (pWidget->nSpriteId) {
        case 2:
            pWidget->fonts[0].bStyle = 0;
            break;
        case 3:
            pWidget->fonts[0].bStyle = 1;
            break;
        case 4:
            pWidget->fonts[0].bStyle = 2;
            break;
        case 52:
            pWidget->fonts[0].bStyle = 4;
            break;
        default:
            pWidget->nSpriteId = 2;
            break;
        }
        pWidget->dwFlags = flags & ~0x100u;
    }
    return 1;
}

int ValidateEntityStateAlwaysTrue(MenuWidget *pWidget) {
    ValidateEntityState(pWidget);
    return 1;
}

int ProcessCheatInputAndValidateEntity(MenuWidget *pWidget) {
    ProcessCheatCodeInput();
    ValidateEntityState(pWidget);
    return 1;
}

int MultiplayerConnectionUI_OnSelectIpx(MenuWidget *pWidget) {
    g_dwServiceProviderIndex = 0;
    ShowJoinCreateGameScreen();
    return 1;
}

int MultiplayerConnectionUI_UpdateIpxItem(MenuWidget *pWidget) {
    g_dwServiceProviderIndex = 0;
    ValidateEntityState(pWidget);
    return 1;
}

int MultiplayerConnectionUI_OnSelectTcpIp(MenuWidget *pWidget) {
    g_dwServiceProviderIndex = 1;
    ShowJoinCreateGameScreen();
    return 1;
}

int MultiplayerConnectionUI_UpdateTcpIpItem(MenuWidget *pWidget) {
    g_dwServiceProviderIndex = 1;
    ValidateEntityState(pWidget);
    return 1;
}

int MultiplayerConnectionUI_OnSelectModem(MenuWidget *pWidget) {
    g_dwServiceProviderIndex = 2;
    ShowJoinCreateGameScreen();
    return 1;
}

int MultiplayerConnectionUI_UpdateModemItem(MenuWidget *pWidget) {
    g_dwServiceProviderIndex = 2;
    ValidateEntityState(pWidget);
    return 1;
}

static inline int BeginMenuTextEdit(MenuWidget *pWidget) {
    if (!g_dwMenuTextEditActive) {
        g_dwMenuTextEditActive = 1;
        pWidget->fonts[0].bDrawCaret = !pWidget->fonts[0].bDrawCaret;
    }
    return 1;
}

int PlayerNameEntryUI_OnToggleTextEdit(MenuWidget *pWidget) {
    return BeginMenuTextEdit(pWidget);
}

int ConnectionUI_OnToggleTextEdit(MenuWidget *pWidget) {
    return BeginMenuTextEdit(pWidget);
}

int CreateGameMenuUI_OnToggleTextEdit(MenuWidget *pWidget) {
    return BeginMenuTextEdit(pWidget);
}

int RaceResultsPanel_PreUpdateSelectLastRace(void) {
    g_dwSeasonYear = g_LastRaceResultCopy.year;
    g_bSeasonClass = static_cast<s8>(g_LastRaceResultCopy.season);
    g_dwSeasonRound = g_LastRaceResultCopy.round;
    return 1;
}

int RaceResultsPanel_PostUpdateRestoreSeasonProgress(void) {
    g_dwSeasonYear = g_SeasonProgress.championshipFlags;
    g_bSeasonClass = static_cast<s8>(g_SeasonProgress.season);
    g_dwSeasonRound = g_SeasonProgress.round;
    return 1;
}

int SeasonProgressInfoUI_OnResetRound(MenuWidget *) {
    memset(g_szSeasonRoundLabel, 0, sizeof(g_szSeasonRoundLabel));
    g_dwSeasonRound = 0;
    g_SeasonProgress.round = 0;
    return 1;
}

int SaveSeasonUI_InvokeBackHandler(MenuWidget *pWidget) {
    g_pSaveSeasonBackHandlerMenu->pfnAction(pWidget);
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    return 0;
}

int AudioOptionsUI_OnMusicTrackChanged(MenuWidget *, int *pTrack) {
    SetCurrentMusicTrack(static_cast<int>(static_cast<unsigned>(*pTrack) + 2));
    return 1;
}

int DetectDuplicateKeyBindings(int nDevice) {
    int found = 0;
    for (int i = 0; i < KEY_CONFIG_ENTRY_COUNT; i++) {
        // Later rows clear duplicate marks assigned by earlier rows.
        g_adwKeyBindingDuplicateFlags[i] = 0;
        int flags =
            g_GlobalRallyConfig.GetInputBindingFlags(nDevice, g_aKeyConfigEntries[i].nControl);
        unsigned char code = static_cast<unsigned char>(
            g_GlobalRallyConfig.GetInputBindingKeyCode(nDevice, g_aKeyConfigEntries[i].nControl));
        for (int j = i + 1; j < KEY_CONFIG_ENTRY_COUNT; j++) {
            int control = g_aKeyConfigEntries[j].nControl;
            if (i < 12 && (control == 12 || control == 13 || control == 14)) {
                continue;
            }
            int otherFlags = g_GlobalRallyConfig.GetInputBindingFlags(nDevice, control);
            unsigned char otherCode = static_cast<unsigned char>(
                g_GlobalRallyConfig.GetInputBindingKeyCode(nDevice, control));
            if ((flags || code) && flags == otherFlags && code == otherCode) {
                found = 1;
                g_adwKeyBindingDuplicateFlags[j] = 1;
                g_adwKeyBindingDuplicateFlags[i] = 1;
            }
        }
    }
    return found;
}

int KeyConfigUI_PollKeyCapture(MenuWidget *) {
    if (!g_fCharInputSuppressed) {
        return 1;
    }
    int code = GetFirstPressedKeyIndex();
    g_dwCapturedKeyBindingValue = code;
    if (code == -1 && g_dwKeyCaptureWaitRelease) {
        g_dwLastWmCharParam = 0;
        g_dwMenuTextEditActive = 0;
        g_fCharInputSuppressed = 0;
        g_dwKeyCaptureWaitRelease = 0;
        return 1;
    }
    int device = g_dwSelectedInputDevice;
    if (static_cast<unsigned>(device) > 3) {
        return 1;
    }
    if (device == 0) {
        if (code >= 0) {
            g_dwKeyCaptureWaitRelease = 1;
            int flags = g_GlobalRallyConfig.GetInputBindingFlags(
                0, g_aKeyConfigEntries[g_dwKeyConfigSelectedAction].nControl);
            g_GlobalRallyConfig.SetInputBinding(
                0,
                g_aKeyConfigEntries[g_dwKeyConfigSelectedAction].nControl,
                flags,
                g_dwCapturedKeyBindingValue);
        }
    } else {
        unsigned flags = 0;
        if (code == -1) {
            if (device == 3) {
                code = GetPressedMouseButton();
                flags = 0x300;
            } else {
                code = GetPressedJoystickControl(&flags);
            }
        }
        if (code >= 0) {
            g_dwCapturedKeyBindingValue = code;
            if (device != 3) {
                g_dwKeyCaptureWaitRelease = 1;
            }
            g_GlobalRallyConfig.SetInputBinding(
                device, g_aKeyConfigEntries[g_dwKeyConfigSelectedAction].nControl, flags, code);
            if (device == 3) {
                g_dwKeyCaptureComplete = 1;
            }
        }
    }
    g_dwKeyBindingDuplicatesFound = DetectDuplicateKeyBindings(device);
    if (device == 3 && g_dwKeyCaptureComplete) {
        GetPressedMouseButton();
        for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
            if (g_adwMouseButtonRaw[i]) {
                return 1;
            }
        }
        g_dwKeyCaptureComplete = 0;
        g_dwKeyCaptureWaitRelease = 1;
    }
    return 1;
}

int KeyConfigUI_UpdateBindingLabel(MenuWidget *pWidget) {
    const char *text = nullptr;
    bool updateText = true;
    if (g_fCharInputSuppressed) {
        text = GetStringResource(0xb2);
    } else {
        int device = g_dwSelectedInputDevice;
        if (static_cast<unsigned>(device) <= 3) {
            unsigned char code =
                static_cast<unsigned char>(g_GlobalRallyConfig.GetInputBindingKeyCode(
                    device, g_aKeyConfigEntries[g_dwKeyConfigSelectedAction].nControl));
            if (device == 0) {
                text = g_aKeyboardControlNames[FindControlNameIndex(0, code)].szName;
            } else if (g_GlobalRallyConfig.GetInputBindingFlags(
                           device, g_aKeyConfigEntries[g_dwKeyConfigSelectedAction].nControl)) {
                int index = FindControlNameIndex(device, code);
                text = device == 3 ? g_aJoystickButtonNamesA[index].szName :
                                     g_aJoystickButtonNamesB[index].szName;
            } else if (code) {
                text = g_aKeyboardControlNames[FindControlNameIndex(0, code)].szName;
            } else {
                text = GetStringResource(0xb1);
            }
        } else {
            updateText = false;
        }
    }
    MenuFont *font = &pWidget->fonts[0];
    if (updateText) {
        strcpy(font->szText, text);
    }
    font->pVTable->MeasureText(font);
    ProcessMenuTextEditInput(pWidget, 0);
    return 1;
}

void PollInputAndUpdateEdges(void) {
    UpdateKeyboardEdgeState();
    UpdateMouseButtonEdges();
}

int ProcessMenuTextEditInput(MenuWidget *pWidget, short nFont) {
    MenuFont *font = &pWidget->fonts[nFont];
    MenuFontVTable *vtable = font->pVTable;
    vtable->MeasureText(font);
    if (!font->bDrawCaret) {
        vtable->DrawText(font);
        return 0;
    }
    signed char result = vtable->HandleTextInput(font);
    if (result <= 0 || (pWidget->dwFlags & 2)) {
        if (!g_dwInNetworkCarSetup) {
            g_dwMenuTextEditActive = 0;
            font->bDrawCaret = 0;
            pWidget->dwFlags &= ~2u;
        }
        PollInputAndUpdateEdges();
        if (pWidget->pfnTextEditComplete) {
            pWidget->pfnTextEditComplete(pWidget);
        }
    }
    vtable->DrawText(font);
    return 1;
}

int FindControlNameIndex(int nTable, int nCode) {
    InputControlName *names;
    int count;
    switch (nTable) {
    case 0:
        names = g_aKeyboardControlNames;
        count = KEYBOARD_CONTROL_NAME_COUNT;
        break;
    case 3:
        names = g_aJoystickButtonNamesA;
        count = JOYSTICK_NAME_COUNT_A;
        break;
    case 2:
    case 1:
        names = g_aJoystickButtonNamesB;
        count = JOYSTICK_NAME_COUNT_B;
        break;
    default:
        return 0;
    }
    for (int i = 0; i < count; i++) {
        if (names[i].nCode == nCode) {
            return i;
        }
    }
    return 0;
}

int InputOptionsUI_OnCycleInputDevice(MenuWidget *) {
    if (g_adwMouseButtonPressed[1]) {
        g_dwSelectedInputDevice =
            static_cast<int>(static_cast<unsigned>(g_dwSelectedInputDevice) + 1);
        if (g_dwSelectedInputDevice >= 4) {
            g_dwSelectedInputDevice = 0;
        }
        if (g_dwSelectedInputDevice == 1) {
            g_dwSelectedInputDevice = 2;
        }
    } else if (g_adwMouseButtonPressed[0]) {
        g_dwSelectedInputDevice =
            static_cast<int>(static_cast<unsigned>(g_dwSelectedInputDevice) - 1);
        if (g_dwSelectedInputDevice < 0) {
            g_dwSelectedInputDevice = 3;
        }
        if (g_dwSelectedInputDevice == 1) {
            g_dwSelectedInputDevice = 0;
        }
    }
    g_GlobalRallyConfig.adwMenuSelectionBackup[6] = g_dwSelectedInputDevice;
    g_GlobalRallyConfig.dwInputDeviceType =
        g_adwInputDeviceMenuIndexToDevice[g_dwSelectedInputDevice];
    g_GlobalRallyConfig.SelectActiveInputBindings();
    return 1;
}

int InputOptionsUI_OnRestoreDefaults(MenuWidget *) {
    if (static_cast<unsigned>(g_dwSelectedInputDevice) <= 3) {
        g_GlobalRallyConfig.LoadDeviceDefaultBindings(g_dwSelectedInputDevice);
    }
    return 1;
}

int QuitGameConfirmUI_OnConfirmQuit(MenuWidget *pWidget) {
#if TGR_RENDERER == TGR_RENDERER_D3D
    QueryAvailableTextureMemory();
#endif
    BackupMenuSelections();
    g_GlobalRallyConfig.SaveConfigToFile(g_szConfigFilePath);
    pWidget->pScreen->ShutdownFrontEnd(0);
    ShutdownDirectPlaySession(g_pDirectPlayInfo);
    ShutdownApplication(0);
}

int RaceSetupUI_UpdateTrackImage(MenuWidget *pWidget) {
    int track = g_dwTrackMenuIndex;
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        int round = g_bSeasonRoundListBrowsing ? g_dwSeasonSavedRound : g_dwSeasonRound;
        track = g_SeasonTable[g_bSeasonClass].races[round][0];
    }
    pWidget->nSpriteId = g_awTrackImageIds[track];
    return 1;
}

int TimeAttackUI_UpdateTrackImage(MenuWidget *pWidget) {
    if (g_pActiveMenuScreen == g_pCachedMenu_LoadGhost && !g_dwTimeAttackLoadSelected) {
        return -2;
    }
    pWidget->nSpriteId = g_awTimeAttackTrackImageIds[g_dwTrackMenuIndex];
    return 1;
}

int GameOptionsUI_UpdateForceFeedbackImage(MenuWidget *pWidget) {
    int option = g_bForceFeedbackAvailable ? g_dwOptionForceFeedback : 1;
    pWidget->nSpriteId = g_abForceFeedbackImageIds[option];
    return 1;
}

int RaceSetupUI_UpdateWeatherImage(MenuWidget *pWidget) {
    if (g_pActiveMenuScreen == g_pCachedMenu_LoadGhost && !g_dwTimeAttackLoadSelected) {
        return -2;
    }
    int weather = g_dwSelectedWeather;
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        int round = g_bSeasonRoundListBrowsing ? g_dwSeasonSavedRound : g_dwSeasonRound;
        weather = g_SeasonTable[g_bSeasonClass].races[round][1];
    }
    pWidget->nSpriteId = g_abWeatherImageIds[weather];
    return 1;
}

int SeasonUI_UpdateSeasonNameLabel(MenuWidget *pWidget) {
    int season = g_dwSeasonLoaded ? g_bSeasonClass : 0;
    if (strlen(GetStringResource(g_SeasonTable[season].nameStringId)) == 0) {
        return 0;
    }
    MenuFont *font = &pWidget->fonts[0];
    strcpy(font->szText, _strupr(GetStringResource(g_SeasonTable[season].nameStringId)));
    MenuFontVTable *vtable = font->pVTable;
    vtable->MeasureText(font);
    vtable->DrawText(font);
    return 1;
}

int SeasonUI_UpdateYearOrdinalLabel(MenuWidget *pWidget) {
    sprintf(g_szSeasonYearLabel, "%d", static_cast<int>(static_cast<unsigned>(g_dwSeasonYear) + 1));
    unsigned suffix = 0xb6;
    switch (g_dwSeasonYear) {
    case 0:
        suffix = 0xb3;
        break;
    case 1:
        suffix = 0xb4;
        break;
    case 2:
        suffix = 0xb5;
        break;
    }
    char text[128];
    sprintf(text, "%s%s", g_szSeasonYearLabel, GetStringResource(suffix));
    MenuFont *font = &pWidget->fonts[0];
    strcpy(font->szText, text);
    MenuFontVTable *vtable = font->pVTable;
    vtable->MeasureText(font);
    vtable->DrawText(font);
    return 1;
}

static inline int UpdateLoadedMenuItemState(MenuWidget *pWidget, int loaded) {
    if (loaded) {
        pWidget->dwFlags &= ~0x1010u;
    } else {
        pWidget->dwFlags |= 0x1010u;
        pWidget->nSpriteId = 2;
        pWidget->fonts[0].bStyle = 0;
    }
    return 1;
}

int LoadSeasonUI_UpdateContinueButtonState(MenuWidget *pWidget) {
    return UpdateLoadedMenuItemState(pWidget, g_dwLoadSeasonSlotValid);
}

int LoadTimeAttackMenuUI_UpdateItemEnabledState(MenuWidget *pWidget) {
    return UpdateLoadedMenuItemState(pWidget, g_dwTimeAttackLoadSelected);
}

static inline void DrawAlternateMenuLabel(MenuWidget *pWidget, const char *text) {
    MenuFont *font = &pWidget->fonts[0];
    strcpy(font->szText, text);
    MenuFontVTable *vtable = font->pVTable;
    vtable->MeasureTextAltFont(font);
    vtable->DrawTextAltFont(font);
}

static inline int DrawSeasonRoundValue(MenuWidget *pWidget, int value) {
    MenuFont *font = &pWidget->fonts[0];
    _itoa(value, font->szText, 10);
    MenuFontVTable *vtable = font->pVTable;
    vtable->MeasureTextAltFont(font);
    vtable->DrawTextAltFont(font);
    return 1;
}

int SeasonUI_UpdateRoundPointsLabel(MenuWidget *pWidget) {
    return DrawSeasonRoundValue(pWidget,
                                static_cast<short>(g_awSeasonRoundPoints[g_dwSeasonSavedRound]));
}

int SeasonUI_UpdateRoundPlaceLabel(MenuWidget *pWidget) {
    return DrawSeasonRoundValue(
        pWidget, static_cast<signed char>(g_abSeasonRoundPlace[g_dwSeasonSavedRound]));
}

int SeasonUI_UpdateTotalPointsLabel(MenuWidget *pWidget) {
    char text[32] = {};
    if (!g_dwSeasonLoaded) {
        strcpy(text, "0");
    } else {
        int points = 0;
        for (int round = 0; round < SEASON_RACES; round++) {
            points += g_SeasonProgress.seasonRacePoints[g_bSeasonClass][round];
        }
        _itoa(points, text, 10);
    }
    if (strlen(text) == 0) {
        return 0;
    }
    DrawAlternateMenuLabel(pWidget, _strupr(text));
    return 1;
}

static inline int DrawResultsNumericLabel(MenuWidget *pWidget, int value) {
    char text[32] = {};
    _itoa(value, text, 10);
    if (strlen(text) == 0) {
        return 0;
    }
    DrawAlternateMenuLabel(pWidget, _strupr(text));
    return 1;
}

int RaceResultsTotalsUI_UpdateTotalPointsLabel(MenuWidget *pWidget) {
    return DrawResultsNumericLabel(pWidget, g_dwSeasonTotalPoints);
}

int RaceResultsTotalsUI_UpdateRoundsCompletedLabel(MenuWidget *pWidget) {
    return DrawResultsNumericLabel(pWidget,
                                   static_cast<int>(static_cast<unsigned>(g_dwSeasonRound) + 1));
}

int RaceResultsTotalsUI_UpdatePointsNeededLabel(MenuWidget *pWidget) {
    int points = static_cast<int>(
        static_cast<unsigned>(g_SeasonTable[g_SeasonResumeState.season].pointsNeeded) -
        static_cast<unsigned>(g_dwSeasonTotalPoints));
    if (points < 0) {
        points = 0;
    }
    return DrawResultsNumericLabel(pWidget, points);
}

int SeasonProgressInfoUI_UpdatePointsNeededLabel(MenuWidget *pWidget) {
    char text[32] = {};
    int season = g_dwSeasonLoaded ? g_bSeasonClass : 0;
    int points = static_cast<int>(static_cast<unsigned>(g_SeasonTable[season].pointsNeeded) -
                                  static_cast<unsigned>(g_dwSeasonTotalPoints));
    if (points < 0) {
        points = 0;
    }
    _itoa(points, text, 10);
    if (strlen(text) == 0) {
        return 0;
    }
    DrawAlternateMenuLabel(pWidget, _strupr(text));
    return 1;
}

int SeasonFileUI_UpdateYearLabel(MenuWidget *pWidget) {
    sprintf(g_szSeasonYearLabel, "%d", static_cast<int>(static_cast<unsigned>(g_dwSeasonYear) + 1));
    DrawAlternateMenuLabel(pWidget, g_szSeasonYearLabel);
    return 1;
}

int SeasonFileUI_UpdateRoundLabel(MenuWidget *pWidget) {
    sprintf(
        g_szSeasonRoundLabel, "%d", static_cast<int>(static_cast<unsigned>(g_dwSeasonRound) + 1));
    DrawAlternateMenuLabel(pWidget, g_szSeasonRoundLabel);
    return 1;
}

int SeasonProgressInfoUI_UpdateRoundLabel(MenuWidget *pWidget) {
    sprintf(
        g_szSeasonRoundLabel, "%d", static_cast<int>(static_cast<unsigned>(g_dwSeasonRound) + 1));
    MenuFont *font = &pWidget->fonts[0];
    strcpy(font->szText, GetStringResource(0x37));
    strcat(font->szText, "  ");
    strcat(font->szText, g_szSeasonRoundLabel);
    MenuFontVTable *vtable = font->pVTable;
    vtable->MeasureText(font);
    vtable->DrawText(font);
    return 1;
}

int KeyConfigUI_UpdateDuplicateBindingWarning(MenuWidget *pWidget) {
    MenuFont *font = &pWidget->fonts[0];
    if (g_dwKeyBindingDuplicatesFound) {
        strcpy(font->szText, GetStringResource(0xaf));
        font->bStyle = g_adwKeyBindingDuplicateFlags[g_dwKeyConfigSelectedAction] ? 4 : 1;
    } else {
        strcpy(font->szText, " ");
    }
    MenuFontVTable *vtable = font->pVTable;
    vtable->MeasureText(font);
    ProcessMenuTextEditInput(pWidget, 0);
    return 1;
}

int PreRaceInfoUI_UpdateBackButtonText(MenuWidget *pWidget) {
    int stringId = g_dwGameMode == 0 && g_SeasonResumeState.state == 0 ? 0x51 : 0xc;
    strcpy(pWidget->fonts[0].szText, GetStringResource(stringId));
    MenuFont *font = &pWidget->fonts[0];
    font->pVTable->MeasureText(font);
    return 1;
}

static inline void DrawPrimaryMenuLabel(MenuWidget *pWidget, const char *text) {
    MenuFont *font = &pWidget->fonts[0];
    MenuFontVTable *vtable = font->pVTable;
    strcpy(font->szText, text);
    vtable->MeasureText(font);
    ProcessMenuTextEditInput(pWidget, 0);
}

int CarSetupUI_UpdateTransmissionLabel(MenuWidget *pWidget) {
    DrawPrimaryMenuLabel(pWidget,
                         GetStringResource(g_adwTransmissionLabelStrIds[g_dwSelectedTransmission]));
    return 1;
}

int CarSetupUI_UpdateShocksLabel(MenuWidget *pWidget) {
    DrawPrimaryMenuLabel(pWidget,
                         GetStringResource(g_adwSuspensionLabelStrIds[g_dwSuspensionMenuIndex]));
    return 1;
}

int InputOptionsUI_UpdateInputDeviceLabel(MenuWidget *pWidget) {
    DrawPrimaryMenuLabel(pWidget,
                         GetStringResource(g_adwInputDeviceLabelStrIds[g_dwSelectedInputDevice]));
    return 1;
}

int ConnectionUI_UpdateServiceProviderLabel(MenuWidget *pWidget) {
    DrawPrimaryMenuLabel(
        pWidget, GetStringResource(g_adwServiceProviderLabelStrIds[g_dwServiceProviderIndex]));
    return 1;
}

int GameOptionsUI_UpdateCarShadowLabel(MenuWidget *pWidget) {
    DrawPrimaryMenuLabel(pWidget,
                         GetStringResource(g_adwCarShadowLabelStrIds[g_dwOptionCarShadow]));
    return 1;
}

int GameOptionsUI_UpdateSkidMarksLabel(MenuWidget *pWidget) {
    DrawPrimaryMenuLabel(pWidget,
                         GetStringResource(g_adwSkidMarksLabelStrIds[g_dwOptionSkidMarks]));
    return 1;
}

int GameOptionsUI_UpdateSpecularLightingLabel(MenuWidget *pWidget) {
    DrawPrimaryMenuLabel(
        pWidget, GetStringResource(g_adwSpecularLightingLabelStrIds[g_dwOptionSpecularLighting]));
    return 1;
}

int GameOptionsUI_UpdateForceFeedbackLabel(MenuWidget *pWidget) {
    DrawPrimaryMenuLabel(
        pWidget,
        GetStringResource(g_bForceFeedbackAvailable ?
                              g_adwForceFeedbackLabelStrIds[g_dwOptionForceFeedback] :
                              0x74));
    return 1;
}

int CarSetupUI_UpdateCarNameLabel(MenuWidget *pWidget) {
    int stringId = 0x14;
    if (g_pActiveMenuScreen != g_pCachedMenu_LoadGhost || g_dwTimeAttackLoadSelected) {
        int car = g_dwCarMenuIndex;
        if (car > 15) {
            car -= 16;
        }
        stringId = g_adwCarNameStringIds[car];
    }
    DrawPrimaryMenuLabel(pWidget, GetStringResource(stringId));
    return 1;
}

int RaceSetupUI_UpdateTrackNameLabel(MenuWidget *pWidget) {
    if (g_pActiveMenuScreen == g_pCachedMenu_LoadGhost && !g_dwTimeAttackLoadSelected) {
        DrawPrimaryMenuLabel(pWidget, GetStringResource(0x1b));
        return 1;
    }
    char text[128];
    int track;
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        bool browsing = g_bSeasonRoundListBrowsing != 0;
        int round = browsing ? g_dwSeasonSavedRound : g_dwSeasonRound;
        track = g_SeasonTable[g_bSeasonClass].races[round][0];
        strcpy(text, GetStringResource(g_adwTrackNameStringIds[track]));
        round = browsing ? g_dwSeasonSavedRound : g_dwSeasonRound;
        track = g_SeasonTable[g_bSeasonClass].races[round][0];
    } else {
        strcpy(text, GetStringResource(g_adwTrackNameStringIds[g_dwTrackMenuIndex]));
        track = g_dwTrackMenuIndex;
    }
    if (g_apTrackMenuItems[track]->flags & 0x10) {
        float y = pWidget->fY;
        pWidget->fonts[0].fY = 130.0f;
        DrawPrimaryMenuLabel(pWidget, GetStringResource(0xb0));
        // The widget supplies the font's restored coordinate.
        pWidget->fonts[0].fY = y;
    }
    DrawPrimaryMenuLabel(pWidget, text);
    return 1;
}

int RaceSetupUI_UpdateWeatherLabel(MenuWidget *pWidget) {
    bool loadingGhost =
        g_pActiveMenuScreen == g_pCachedMenu_LoadGhost && !g_dwTimeAttackLoadSelected;
    MenuFont *font = &pWidget->fonts[0];
    int stringId;
    if (loadingGhost) {
        font->fY -= 8.0f;
        stringId = 0x1c;
    } else {
        int weather = g_dwSelectedWeather;
        if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
            int round = g_bSeasonRoundListBrowsing ? g_dwSeasonSavedRound : g_dwSeasonRound;
            weather = g_SeasonTable[g_bSeasonClass].races[round][1];
        }
        stringId = g_adwWeatherLabelStrIds[weather];
    }
    const char *text = GetStringResource(stringId);
    MenuFontVTable *vtable = font->pVTable;
    strcpy(font->szText, text);
    vtable->MeasureText(font);
    ProcessMenuTextEditInput(pWidget, 0);
    if (loadingGhost) {
        font->fY += 8.0f;
    }
    return 1;
}

int RaceSetupUI_UpdateLapCountLabel(MenuWidget *pWidget) {
    if (g_pActiveMenuScreen != g_pCachedMenu_LoadGhost || g_dwTimeAttackLoadSelected) {
        MenuFont *font = &pWidget->fonts[0];
        _itoa(g_dwTotalLaps, font->szText, 10);
        MenuFontVTable *vtable = font->pVTable;
        vtable->MeasureTextAltFont(font);
        vtable->DrawTextAltFont(font);
    }
    return 1;
}

int CarSetupUI_OnAcceptAndSave(MenuActionWidget *pWidget) {
    pWidget->pScreen->bRunning = 0;
    pWidget->pScreen->ShutdownFrontEnd(0);
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        g_dwTotalLaps = 3;
    }
    BackupMenuSelections();
    g_GlobalRallyConfig.SaveConfigToFile(g_szConfigFilePath);
    SaveAutoSaveSeason();
    return 0;
}

int AudioOptionsUI_OnCycleSfxVolume(MenuActionWidget *) {
    g_bAudioOptionsSfxSelected = 1;
    if (g_adwMouseButtonPressed[1]) {
        g_GlobalRallyConfig.dwSfxVolume =
            static_cast<int>(static_cast<unsigned>(g_GlobalRallyConfig.dwSfxVolume) + 1);
        if (g_GlobalRallyConfig.dwSfxVolume > 9) {
            g_GlobalRallyConfig.dwSfxVolume = 0;
        }
    } else if (g_adwMouseButtonPressed[0]) {
        g_GlobalRallyConfig.dwSfxVolume =
            static_cast<int>(static_cast<unsigned>(g_GlobalRallyConfig.dwSfxVolume) - 1);
        if (g_GlobalRallyConfig.dwSfxVolume < 0) {
            g_GlobalRallyConfig.dwSfxVolume = 9;
        }
    }
    ApplyAudioVolumeSettings();
    return 1;
}

int AudioOptionsUI_OnCycleMusicVolume(MenuActionWidget *) {
    g_bAudioOptionsSfxSelected = 0;
    if (g_adwMouseButtonPressed[1]) {
        g_GlobalRallyConfig.dwMusicVolume =
            static_cast<int>(static_cast<unsigned>(g_GlobalRallyConfig.dwMusicVolume) + 1);
        if (g_GlobalRallyConfig.dwMusicVolume > 9) {
            g_GlobalRallyConfig.dwMusicVolume = 0;
        }
    } else if (g_adwMouseButtonPressed[0]) {
        g_GlobalRallyConfig.dwMusicVolume =
            static_cast<int>(static_cast<unsigned>(g_GlobalRallyConfig.dwMusicVolume) - 1);
        if (g_GlobalRallyConfig.dwMusicVolume < 0) {
            g_GlobalRallyConfig.dwMusicVolume = 9;
        }
    }
    ApplyAudioVolumeSettings();
    return 1;
}

int AudioOptionsUI_OnBackAndSave(MenuActionWidget *pItem) {
    ReturnFromCachedMenu(pItem, g_pCachedMenu_GameplaySettings, g_pCachedMenu_Options);
    BackupMenuSelections();
    g_GlobalRallyConfig.SaveConfigToFile(g_szConfigFilePath);
    return 0;
}

int PreRaceInfoUI_OnBack(MenuActionWidget *) {
    g_pActiveMenuScreen = g_pPreRaceInfoMenuScreen;
    return 0;
}

int MultiplayerGameSetupUI_OnBackAsClient(MenuActionWidget *pItem) {
    if (g_dwLaunchedFromLobby) {
        pItem->pScreen->ShutdownFrontEnd(0);
        ShutdownApplication(0);
    }
    ReturnFromCachedMenu(pItem, g_pCachedMenu_RaceSetupNetwork, g_pCachedMenu_JoinCreateGame);
    if (g_pNetworkSessionJoinMenu) {
        g_pNetworkSessionJoinMenu->dwFlags &= ~0x10u;
    }
    DisconnectNetworkSession();
    g_dwSessionEnumPending = 1;
    if ((g_dwServiceProviderIndex == 0 || g_dwServiceProviderIndex == 1) &&
        !g_dwLaunchedFromLobby) {
        InitializeNetworkServiceProvider();
    }
    if ((g_dwServiceProviderIndex == 2 || g_dwServiceProviderIndex == 3) &&
        g_pNetworkSessionJoinMenu) {
        g_pNetworkSessionJoinMenu->dwFlags &= ~0x10u;
    }
    return 0;
}

int MultiplayerGameSetupUI_OnBackAsHost(MenuActionWidget *pItem) {
    if (g_dwLaunchedFromLobby) {
        pItem->pScreen->ShutdownFrontEnd(0);
        ShutdownApplication(0);
    }
    ReturnFromCachedMenu(pItem, g_pCachedMenu_RaceSetupNetwork, g_pCachedMenu_CreateGame);
    DisconnectNetworkSession();
    if ((g_dwServiceProviderIndex == 0 || g_dwServiceProviderIndex == 1) &&
        !g_dwLaunchedFromLobby) {
        InitializeNetworkServiceProvider();
    }
    if ((g_dwServiceProviderIndex == 2 || g_dwServiceProviderIndex == 3) &&
        g_pNetworkSessionJoinMenu) {
        g_pNetworkSessionJoinMenu->dwFlags &= ~0x10u;
        g_pNetworkSessionJoinMenu->fonts[0].bStyle = 0;
    }
    return 0;
}

int CreateGameMenuUI_OnBack(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    NetworkMenuWidget *pJoinMenu;
    pItem->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    pJoinMenu = g_pNetworkSessionJoinMenu;
    pDestination = g_pCachedMenu_JoinCreateGame;
    g_pCachedMenu_CreateGame = nullptr;
    g_pMultiplayerGameSetupMenu = nullptr;
    g_pActiveMenuScreen = pDestination;
    if (pJoinMenu) {
        pJoinMenu->dwFlags &= ~0x10u;
    }
    if ((g_dwServiceProviderIndex == 0 || g_dwServiceProviderIndex == 1) &&
        !g_dwLaunchedFromLobby) {
        DisconnectNetworkSession();
        g_dwSessionEnumPending = 1;
        InitializeNetworkServiceProvider();
    }
    return 0;
}

int JoinCreateGameUI_OnBack(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    pItem->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    pDestination = g_pCachedMenu_MultiplayerConnection;
    g_pCachedMenu_JoinCreateGame = nullptr;
    g_pMultiplayerGameSetupMenu = nullptr;
    g_pNetworkSessionJoinMenu = nullptr;
    g_pJoinCreateGameMenu = nullptr;
    g_dwSelectedSessionIndex = 0;
    g_pActiveMenuScreen = pDestination;
    DisconnectNetworkSession();
    return 0;
}

int CarSetupUI_OnBackToRaceSetup(MenuActionWidget *pItem) {
    return ReturnFromCachedMenu(pItem, g_pCachedMenu_CarSetup, g_pCachedMenu_MainMenu);
}

int LoadSeasonUI_OnContinueSeason(MenuActionWidget *) {
    ShowPreRaceInfoScreen();
    g_pExitSeasonConfirmMenu->pfnAction = PreRaceInfoUI_OnBackToLoadSeason;
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    return 1;
}

int CarSetupUI_OnOpenOptions(MenuActionWidget *) {
    ShowOptionsMenu();
    g_pOptionsSubMenu->pfnAction = OptionsMenuUI_OnBackToCarSetup;
    return 1;
}

int CarSetupUI_OnOpenSaveSeason(MenuActionWidget *) {
    ShowSaveSeasonScreen();
    g_pSaveSeasonBackHandlerMenu->pfnAction = SaveSeasonUI_OnBackToCarSetup;
    return 1;
}

int OptionsMenuUI_OnBackToRaceResultsInfo(MenuActionWidget *pItem) {
    return ReturnFromOptionsMenu(pItem, g_pCachedMenu_RaceResults);
}

int RaceResultsInfoUI_OnOpenOptions(MenuActionWidget *) {
    ShowOptionsMenu();
    g_pOptionsSubMenu->pfnAction = OptionsMenuUI_OnBackToRaceResultsInfo;
    return 1;
}

int SaveSeasonUI_OnBackToRaceResultsInfo(MenuActionWidget *pItem) {
    return ReturnFromSaveSeasonMenu(pItem, g_pCachedMenu_RaceResults);
}

int RaceResultsInfoUI_OnOpenSaveSeason(MenuActionWidget *) {
    ShowSaveSeasonScreen();
    g_pSaveSeasonBackHandlerMenu->pfnAction = SaveSeasonUI_OnBackToRaceResultsInfo;
    return 1;
}

int OptionsMenuUI_OnBackToRaceResultsSummary(MenuActionWidget *pItem) {
    return ReturnFromOptionsMenu(pItem, g_pCachedMenu_RaceResultsSummary);
}

int RaceResultsSummaryUI_OnOpenOptions(MenuActionWidget *) {
    ShowOptionsMenu();
    g_pOptionsSubMenu->pfnAction = OptionsMenuUI_OnBackToRaceResultsSummary;
    return 1;
}

int SaveSeasonUI_OnBackToRaceResultsSummary(MenuActionWidget *pItem) {
    return ReturnFromSaveSeasonMenu(pItem, g_pCachedMenu_RaceResultsSummary);
}

int RaceResultsSummaryUI_OnOpenSaveSeason(MenuActionWidget *) {
    ShowSaveSeasonScreen();
    g_pSaveSeasonBackHandlerMenu->pfnAction = SaveSeasonUI_OnBackToRaceResultsSummary;
    return 1;
}

int OptionsMenuUI_OnBackToRaceResultsTotals(MenuActionWidget *pItem) {
    return ReturnFromOptionsMenu(pItem, g_pCachedMenu_RaceResultsTotals);
}

int RaceResultsTotalsUI_OnOpenOptions(MenuActionWidget *) {
    ShowOptionsMenu();
    g_pOptionsSubMenu->pfnAction = OptionsMenuUI_OnBackToRaceResultsTotals;
    return 1;
}

int SaveSeasonUI_OnBackToRaceResultsTotals(MenuActionWidget *pItem) {
    return ReturnFromSaveSeasonMenu(pItem, g_pCachedMenu_RaceResultsTotals);
}

int RaceResultsTotalsUI_OnOpenSaveSeason(MenuActionWidget *) {
    ShowSaveSeasonScreen();
    g_pSaveSeasonBackHandlerMenu->pfnAction = SaveSeasonUI_OnBackToRaceResultsTotals;
    return 1;
}

int RaceResultsSummaryUI_OnBack(MenuActionWidget *pItem) {
    pItem->pScreen->DestroyPanels();
    delete g_pActiveMenuScreen;
    g_pCachedMenu_RaceResultsSummary = nullptr;
    g_pActiveMenuScreen = g_pCachedMenu_RaceResults;
    return 0;
}

int ShowRaceResultsScreen(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_RaceResults, BuildRaceResultsInfoUI);
}

int ShowLoadSeasonMenu(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_LoadSeason, BuildLoadSeasonUI);
}

int ShowSaveSeasonScreen(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_SaveSeason, BuildSaveSeasonUI);
}

int ShowRaceResultsTotalsScreen(MenuWidget *) {
    return ShowCachedMenuScreen(g_pCachedMenu_RaceResultsTotals, BuildRaceResultsTotalsUI);
}

int ShowSeasonCongratsScreen(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_SeasonCongrats, BuildSeasonCongratsUI);
}

int ShowNotEnoughPointsScreen(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_NotEnoughPoints, BuildNotEnoughPointsUI);
}

static inline int ShowCachedMenuWithExitConfirmation(MenuScreen *&pScreen,
                                                     void (*pfnBuild)(MenuScreen *),
                                                     MenuScreen *&pConfirmation,
                                                     void (*pfnBuildConfirmation)(MenuScreen *)) {
    const bool wasCached = pScreen != nullptr;
    if (!ShowCachedMenuScreen(pScreen, pfnBuild)) {
        return 0;
    }
    if (!wasCached) {
        pConfirmation = new (std::nothrow) MenuScreen();
        if (!pConfirmation) {
            return 0;
        }
        pConfirmation->pfnBuildMenu = pfnBuildConfirmation;
        pConfirmation->pfnBuildMenu(pConfirmation);
        pConfirmation->bBuilt = true;
    }
    return 1;
}

int ShowRaceResultsSummaryScreen(MenuWidget *) {
    return ShowCachedMenuWithExitConfirmation(g_pCachedMenu_RaceResultsSummary,
                                              BuildRaceResultsSummaryUI,
                                              g_pRaceResultsSummaryMenuScreen,
                                              BuildResultsSummaryExitSeasonConfirmUI);
}

static inline int CompleteConnectionTextEdit(MenuWidget *) {
    /* The text callback shares the controller initialiser's constant return. */
    return osContInit(nullptr, nullptr, nullptr);
}

static inline MenuWidget *AddMenuWidget(
    MenuPanel *panel, MenuScreen *screen, float x, float y, unsigned flags, int mode, short image) {
    MenuWidget *widget = new (std::nothrow) MenuWidget();
    panel->apWidgets[panel->wWidgetCount] = widget;
    if (!widget) {
        ShowErrorMessage(4);
    }
    widget->Initialize(screen, x, y, flags, 2, 5, mode, image);
    return widget;
}

static inline void SetMenuWidgetLabel(MenuWidget *widget, int stringId, const RECT *rect) {
    widget->nSpriteId = 3;
    widget->SetText(GetStringResource(stringId), 1, 1, rect);
}

static inline void AddMenuActionButtons(MenuPanel *panel,
                                        MenuScreen *screen,
                                        int (*save)(MenuWidget *),
                                        int (*options)(MenuWidget *)) {
    float x = static_cast<float>(g_rcHoverBottomLeft.left);
    float y = static_cast<float>(g_rcHoverBottomLeft.top);
    int right;
    for (int i = 0; i < 3; ++i) {
        const short images[3] = {120, 82, 84};
        int (*actions[3])(MenuWidget *) = {save, MenuScreen_OnReturnToMainMenu, options};
        MenuWidget *widget = AddMenuWidget(panel, screen, x, y, 0x402001, 1, images[i]);
        widget->pfnIdleAction = ValidateEntityState;
        widget->pfnAction = actions[i];
        widget->rcBounds.top = static_cast<int>(y);
        if (i == 0) {
            widget->rcBounds.left = static_cast<int>(x);
            right = widget->rcBounds.left + 127;
        } else {
            widget->rcBounds.left = static_cast<int>(x);
        }
        widget->rcBounds.right = right;
        widget->rcBounds.bottom = widget->rcBounds.top + 33;
        widget->nImageMode = 0;
        widget->anSpriteIds[0] = images[i] + 1;
        ++panel->wWidgetCount;
        y += 33;
    }
}

void BuildRaceResultsTotalsUI(MenuScreen *pScreen) {
    pScreen->wCurrentPanel = 0;
    pScreen->anPanelActive[pScreen->wPanelCount] = 1;
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    pScreen->apPanels[pScreen->wPanelCount] = panel;
    if (!panel) {
        ShowErrorMessage(4);
    }
    ++pScreen->wPanelCount;
    panel->pScreen = pScreen;
    panel->pfnBeforeWidgets = RaceResultsPanel_PreUpdateSelectLastRace;
    panel->pfnAfterWidgets = RaceResultsPanel_PostUpdateRestoreSeasonProgress;
    panel->nFlags = 0;
    panel->fX = 195;
    panel->fY = 130;
    auto add = [&](float x, float y, unsigned flags, int mode, short image) {
        return AddMenuWidget(panel, pScreen, x, y, flags, mode, image);
    };
    add(0, 0, 9, 0, 0);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(panel->fX, 10, 0x100009, 1, -1), 0x4f, &g_rcMenuTitleBar);
    ++panel->wWidgetCount;
    MenuWidget *widget = add(panel->fX, panel->fY + 95, 0x102001, 1, -1);
    widget->pfnIdleAction = ValidateEntityState;
    widget->pfnAction = RaceResultsTotalsUI_OnContinue;
    SetMenuWidgetLabel(widget, 0x50, &g_rcMenuCenterPanel);
    ++panel->wWidgetCount;
    ++panel->wNavigationCount;
    widget = add(panel->fX, panel->fY + 114, 0x102001, 1, -1);
    widget->pfnIdleAction = ValidateEntityState;
    widget->pfnAction = RaceResultsTotalsUI_OnBack;
    SetMenuWidgetLabel(widget, 0xc, &g_rcMenuCenterPanel);
    ++panel->wWidgetCount;
    ++panel->wNavigationCount;
    AddMenuActionButtons(
        panel, pScreen, RaceResultsTotalsUI_OnOpenSaveSeason, RaceResultsTotalsUI_OnOpenOptions);
    SetMenuWidgetLabel(add(106, 85, 0x100001, 1, -1), 0x38, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    widget = add(440, 66, 0x5001, 1, -1);
    widget->pfnUpdate = SeasonUI_UpdateSeasonNameLabel;
    widget->nSpriteId = 0x34;
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(106, 123, 0x100001, 1, -1), 0x36, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    widget = add(440, 104, 0x5001, 1, -1);
    widget->pfnUpdate = SeasonUI_UpdateYearOrdinalLabel;
    widget->nSpriteId = 0x34;
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(0, 70, 0x100009, 1, -1), 0x52, &g_rcMenuRightLabel);
    ++panel->wWidgetCount;
    widget = add(330, 97, 0x5001, 1, -1);
    widget->pfnUpdate = RaceResultsTotalsUI_UpdateRoundsCompletedLabel;
    widget->nSpriteId = 5;
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightLabel);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(0, 150, 0x100009, 1, -1), 0x53, &g_rcMenuRightLabel);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(450, 125, 0x100009, 1, -1), 0x40, &g_rcMenuRightListBox);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(450, 185, 0x100009, 1, -1), 0x46, &g_rcMenuRightListBox);
    ++panel->wWidgetCount;
    widget = add(450, 141, 0x5001, 1, -1);
    widget->pfnUpdate = RaceResultsTotalsUI_UpdatePointsNeededLabel;
    widget->nSpriteId = 5;
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightListBox);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(panel->fX, 203, 0x100009, 1, -1), 0x40, &g_rcMenuRightColumnWide);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(panel->fX, 265, 0x100009, 1, -1), 0x41, &g_rcMenuRightColumnWide);
    ++panel->wWidgetCount;
    widget = add(450, 217, 0x5001, 1, -1);
    widget->pfnUpdate = RaceResultsTotalsUI_UpdateTotalPointsLabel;
    widget->nSpriteId = 5;
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightColumnWide);
    ++panel->wWidgetCount;
}

void BuildRaceResultsInfoUI(MenuScreen *pScreen) {
    pScreen->wCurrentPanel = 0;
    g_dwSeasonSavedRound = g_dwSeasonRound;
    pScreen->anPanelActive[pScreen->wPanelCount] = 1;
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    pScreen->apPanels[pScreen->wPanelCount] = panel;
    if (!panel) {
        ShowErrorMessage(4);
    }
    ++pScreen->wPanelCount;
    panel->pScreen = pScreen;
    panel->nFlags = 0;
    panel->fX = 195;
    panel->fY = 130;
    panel->pfnBeforeWidgets = RaceResultsPanel_PreUpdateSelectLastRace;
    panel->pfnAfterWidgets = RaceResultsPanel_PostUpdateRestoreSeasonProgress;
    auto add = [&](float x, float y, unsigned flags, int mode, short image) {
        return AddMenuWidget(panel, pScreen, x, y, flags, mode, image);
    };
    add(0, 0, 9, 0, 0);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(panel->fX, 10, 0x100009, 1, -1), 0x47, &g_rcMenuTitleBar);
    ++panel->wWidgetCount;
    int (*actions[5])(MenuWidget *) = {RaceResultsInfoUI_OnSelectLap1,
                                       RaceResultsInfoUI_OnSelectLap2,
                                       RaceResultsInfoUI_OnSelectLap3,
                                       RaceResultsInfoUI_OnSelectTotalTime,
                                       ShowRaceResultsSummaryScreen};
    const float steps[5] = {0, 19, 38, 57, 95};
    for (int i = 0; i < 5; ++i) {
        MenuWidget *widget = add(panel->fX, panel->fY + steps[i], 0x102001, 1, -1);
        widget->pfnIdleAction = ValidateEntityState;
        widget->pfnAction = actions[i];
        SetMenuWidgetLabel(widget, i == 4 ? 0x1e : 0x48 + i, &g_rcMenuCenterPanel);
        ++panel->wWidgetCount;
        ++panel->wNavigationCount;
    }
    AddMenuActionButtons(
        panel, pScreen, RaceResultsInfoUI_OnOpenSaveSeason, RaceResultsInfoUI_OnOpenOptions);
    SetMenuWidgetLabel(add(106, 123, 0x100001, 1, -1), 0x4c, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    MenuWidget *widget = add(440, 79, 0x5001, 1, -1);
    widget->pfnUpdate = SeasonUI_UpdateRoundPlaceLabel;
    widget->nSpriteId = 5;
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    widget = add(73, 212, 1, 1, 22);
    widget->pfnUpdate = RaceSetupUI_UpdateWeatherImage;
    ++widget->nLinkedWidgetCount;
    widget->anLinkedWidgetIndices[0] = panel->wWidgetCount + 1;
    ++panel->wWidgetCount;
    widget = add(panel->fX, 275, 0x101001, 1, -1);
    widget->pfnUpdate = RaceSetupUI_UpdateWeatherLabel;
    widget->nSpriteId = 3;
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuLeftColumn);
    ++panel->wWidgetCount;
    widget = add(325, 72, 1, 1, 17);
    widget->pfnUpdate = RaceSetupUI_UpdateTrackImage;
    ++widget->nLinkedWidgetCount;
    widget->anLinkedWidgetIndices[0] = panel->wWidgetCount + 1;
    ++panel->wWidgetCount;
    widget = add(panel->fX, 152, 0x101001, 1, -1);
    widget->pfnUpdate = RaceSetupUI_UpdateTrackNameLabel;
    widget->nSpriteId = 3;
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuRightColumnMid);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(450, 185, 0x100009, 1, -1), 0x4d, &g_rcMenuRightListBox);
    ++panel->wWidgetCount;
    widget = add(450, 166, 0x5001, 1, -1);
    widget->pfnUpdate = RaceResultsInfoUI_UpdateSelectedTimeLabel;
    widget->nSpriteId = 0x34;
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuRightListBox);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(panel->fX, 258, 0x100009, 1, -1), 0x40, &g_rcMenuRightColumnWide);
    ++panel->wWidgetCount;
    widget = add(450, 211, 0x5001, 1, -1);
    widget->pfnUpdate = SeasonUI_UpdateRoundPointsLabel;
    widget->nSpriteId = 5;
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightColumnWide);
    ++panel->wWidgetCount;
}

void BuildRaceResultsSummaryUI(MenuScreen *pScreen) {
    pScreen->wCurrentPanel = 0;
    g_dwSeasonSavedRound = g_dwSeasonRound;
    pScreen->anPanelActive[pScreen->wPanelCount] = 1;
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    pScreen->apPanels[pScreen->wPanelCount] = panel;
    if (!panel) {
        ShowErrorMessage(4);
    }
    ++pScreen->wPanelCount;
    panel->pScreen = pScreen;
    panel->pfnBeforeWidgets = RaceResultsPanel_PreUpdateSelectLastRace;
    panel->pfnAfterWidgets = RaceResultsPanel_PostUpdateRestoreSeasonProgress;
    panel->nFlags = 0;
    panel->fX = 195;
    panel->fY = 130;
    auto add = [&](float x, float y, unsigned flags, int mode, short image) {
        return AddMenuWidget(panel, pScreen, x, y, flags, mode, image);
    };
    add(0, 0, 9, 0, 0);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(panel->fX, 10, 0x100009, 1, -1), 0x4e, &g_rcMenuTitleBar);
    ++panel->wWidgetCount;
    MenuWidget *widget = add(panel->fX, panel->fY, 0x3001, 1, -1);
    MenuItemList *list = &widget->itemList;
    MenuItemListVTable *listVTable = list->pVTable;
    widget->pfnUpdate = SeasonRoundList_UpdateRoundSelection;
    list->nSelectionMode = 1;
    listVTable->Initialize(list, 0x200000, &g_rcMenuListBox, 5, 0, -1);
    widget->pfnBeforeUpdate = SeasonRoundList_SetBrowsingFlag;
    widget->pfnAfterUpdate = SeasonRoundList_ClearBrowsingFlag;
    if (g_SeasonTable[g_bSeasonClass].raceCount > 0) {
        auto addItem = listVTable->AddItem;
        int round = 0;
        do {
            char label[32];
            char number[32];
            strcpy(label, GetStringResource(0x37));
            strcat(label, g_szSpace);
            ++round;
            strcat(label, _strupr(_itoa(round, number, 10)));
            addItem(list, label, 1, 1, &g_rcMenuCenterPanel, true);
        } while (round < g_SeasonTable[g_bSeasonClass].raceCount);
    }
    ++panel->wWidgetCount;
    ++panel->wNavigationCount;
    widget = add(panel->fX, panel->fY + 95, 0x102001, 1, -1);
    widget->pfnIdleAction = ValidateEntityState;
    widget->pfnAction = ShowRaceResultsTotalsScreen;
    SetMenuWidgetLabel(widget, 0x1e, &g_rcMenuCenterPanel);
    ++panel->wWidgetCount;
    ++panel->wNavigationCount;
    widget = add(panel->fX, panel->fY + 114, 0x102001, 1, -1);
    widget->pfnIdleAction = ValidateEntityState;
    widget->pfnAction = RaceResultsSummaryUI_OnBack;
    SetMenuWidgetLabel(widget, 0xc, &g_rcMenuCenterPanel);
    ++panel->wWidgetCount;
    ++panel->wNavigationCount;
    AddMenuActionButtons(
        panel, pScreen, RaceResultsSummaryUI_OnOpenSaveSeason, RaceResultsSummaryUI_OnOpenOptions);
    SetMenuWidgetLabel(add(106, 85, 0x100001, 1, -1), 0x38, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    widget = add(440, 66, 0x5001, 1, -1);
    widget->pfnUpdate = SeasonUI_UpdateSeasonNameLabel;
    widget->nSpriteId = 0x34;
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(106, 123, 0x100001, 1, -1), 0x36, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    widget = add(440, 104, 0x5001, 1, -1);
    widget->pfnUpdate = SeasonUI_UpdateYearOrdinalLabel;
    widget->nSpriteId = 0x34;
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(106, 185, 0x100001, 1, -1), 0x4c, &g_rcMenuRightListBox);
    ++panel->wWidgetCount;
    widget = add(440, 141, 0x5001, 1, -1);
    widget->pfnUpdate = SeasonUI_UpdateRoundPlaceLabel;
    widget->nSpriteId = 5;
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightListBox);
    ++panel->wWidgetCount;
    widget = add(73, 212, 1, 1, 22);
    widget->pfnUpdate = RaceSetupUI_UpdateWeatherImage;
    ++widget->nLinkedWidgetCount;
    widget->anLinkedWidgetIndices[0] = panel->wWidgetCount + 1;
    ++panel->wWidgetCount;
    widget = add(panel->fX, 275, 0x101001, 1, -1);
    widget->pfnUpdate = RaceSetupUI_UpdateWeatherLabel;
    widget->nSpriteId = 3;
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuLeftColumn);
    ++panel->wWidgetCount;
    widget = add(325, 72, 1, 1, 17);
    widget->pfnUpdate = RaceSetupUI_UpdateTrackImage;
    ++widget->nLinkedWidgetCount;
    widget->anLinkedWidgetIndices[0] = panel->wWidgetCount + 1;
    ++panel->wWidgetCount;
    widget = add(panel->fX, 152, 0x101001, 1, -1);
    widget->pfnUpdate = RaceSetupUI_UpdateTrackNameLabel;
    widget->nSpriteId = 3;
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuRightColumnMid);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(panel->fX, 258, 0x100009, 1, -1), 0x40, &g_rcMenuRightColumnWide);
    ++panel->wWidgetCount;
    widget = add(450, 211, 0x5001, 1, -1);
    widget->pfnUpdate = SeasonUI_UpdateRoundPointsLabel;
    widget->nSpriteId = 5;
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightColumnWide);
    ++panel->wWidgetCount;
}

void BuildSeasonProgressInfoUI(MenuScreen *pScreen) {
    pScreen->wCurrentPanel = 0;
    pScreen->anPanelActive[pScreen->wPanelCount] = 1;
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    pScreen->apPanels[pScreen->wPanelCount] = panel;
    if (!panel) {
        ShowErrorMessage(4);
    }
    ++pScreen->wPanelCount;
    panel->pScreen = pScreen;
    panel->nFlags = 0;
    panel->fX = 195;
    panel->fY = 130;
    auto add = [&](float x, float y, unsigned flags, int mode, short image) {
        return AddMenuWidget(panel, pScreen, x, y, flags, mode, image);
    };
    add(0, 0, 9, 0, 0);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(panel->fX, 10, 0x100009, 1, -1), 0x44, &g_rcMenuTitleBar);
    ++panel->wWidgetCount;
    MenuWidget *widget = add(panel->fX, panel->fY + 19, 0x102001, 1, -1);
    widget->pfnIdleAction = ValidateEntityState;
    widget->pfnAction = SeasonProgressInfoUI_OnResetRound;
    SetMenuWidgetLabel(widget, 0x45, &g_rcMenuCenterPanel);
    ++panel->wWidgetCount;
    ++panel->wNavigationCount;
    widget = add(panel->fX, panel->fY + 95, 0x102001, 1, -1);
    widget->pfnIdleAction = ValidateEntityState;
    widget->pfnAction = SeasonProgressInfoUI_OnCarSetup;
    widget->nSpriteId = 2;
    widget->SetText(GetStringResource(0x1e), 1, 0, &g_rcMenuCenterPanel);
    ++panel->wWidgetCount;
    ++panel->wNavigationCount;
    widget = add(panel->fX, panel->fY + 114, 0x102001, 1, -1);
    widget->pfnIdleAction = ValidateEntityState;
    widget->pfnAction = SeasonProgressInfoUI_OnBack;
    SetMenuWidgetLabel(widget, 0xc, &g_rcMenuCenterPanel);
    ++panel->wWidgetCount;
    ++panel->wNavigationCount;
    AddMenuActionButtons(
        panel, pScreen, SeasonProgressInfoUI_OnSaveSeason, SeasonProgressInfoUI_OnOptions);
    widget = add(73, 212, 1, 1, 22);
    widget->pfnUpdate = RaceSetupUI_UpdateWeatherImage;
    ++widget->nLinkedWidgetCount;
    widget->anLinkedWidgetIndices[0] = panel->wWidgetCount + 1;
    ++panel->wWidgetCount;
    widget = add(panel->fX, 275, 0x101001, 1, -1);
    widget->pfnUpdate = RaceSetupUI_UpdateWeatherLabel;
    widget->nSpriteId = 3;
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuLeftColumn);
    ++panel->wWidgetCount;
    widget = add(325, 72, 1, 1, 17);
    widget->pfnUpdate = RaceSetupUI_UpdateTrackImage;
    ++widget->nLinkedWidgetCount;
    widget->anLinkedWidgetIndices[0] = panel->wWidgetCount + 1;
    ++panel->wWidgetCount;
    widget = add(panel->fX, 152, 0x101001, 1, -1);
    widget->pfnUpdate = RaceSetupUI_UpdateTrackNameLabel;
    widget->nSpriteId = 3;
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuRightColumnMid);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(450, 125, 0x100009, 1, -1), 0x40, &g_rcMenuRightListBox);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(450, 185, 0x100009, 1, -1), 0x46, &g_rcMenuRightListBox);
    ++panel->wWidgetCount;
    widget = add(450, 141, 0x5001, 1, -1);
    widget->pfnUpdate = SeasonProgressInfoUI_UpdatePointsNeededLabel;
    widget->nSpriteId = 5;
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightListBox);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(panel->fX, 203, 0x100009, 1, -1), 0x40, &g_rcMenuRightColumnWide);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(panel->fX, 265, 0x100009, 1, -1), 0x41, &g_rcMenuRightColumnWide);
    ++panel->wWidgetCount;
    widget = add(450, 217, 0x5001, 1, -1);
    widget->pfnUpdate = SeasonUI_UpdateTotalPointsLabel;
    widget->nSpriteId = 5;
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightColumnWide);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(106, 85, 0x100001, 1, -1), 0x38, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    widget = add(440, 66, 0x5001, 1, -1);
    widget->pfnUpdate = SeasonUI_UpdateSeasonNameLabel;
    widget->nSpriteId = 0x34;
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(106, 123, 0x100001, 1, -1), 0x36, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    widget = add(440, 104, 0x5001, 1, -1);
    widget->pfnUpdate = SeasonUI_UpdateYearOrdinalLabel;
    widget->nSpriteId = 0x34;
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    widget = add(panel->fX, panel->fY - 19, 0x5001, 1, -1);
    widget->pfnUpdate = SeasonProgressInfoUI_UpdateRoundLabel;
    widget->nSpriteId = 0x34;
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuCenterPanel);
    ++panel->wWidgetCount;
}

void BuildMultiplayerConnectionUI(MenuScreen *pScreen) {
    pScreen->wCurrentPanel = 0;
    g_dwGameMode = 6;
    pScreen->anPanelActive[pScreen->wPanelCount] = 1;
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    pScreen->apPanels[pScreen->wPanelCount] = panel;
    if (!panel) {
        ShowErrorMessage(4);
    }
    ++pScreen->wPanelCount;
    panel->pScreen = pScreen;
    panel->nFlags = 0;
    panel->fX = 195;
    panel->fY = 130;
    auto add = [&](float x, float y, unsigned flags, int mode, short image) {
        return AddMenuWidget(panel, pScreen, x, y, flags, mode, image);
    };
    add(0, 0, 9, 0, 0);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(panel->fX, 10, 0x100009, 1, -1), 0x54, &g_rcMenuTitleBar);
    ++panel->wWidgetCount;
    int (*actions[3])(MenuWidget *) = {MultiplayerConnectionUI_OnSelectIpx,
                                       MultiplayerConnectionUI_OnSelectTcpIp,
                                       MultiplayerConnectionUI_OnSelectModem};
    int (*updates[3])(MenuWidget *) = {MultiplayerConnectionUI_UpdateIpxItem,
                                       MultiplayerConnectionUI_UpdateTcpIpItem,
                                       MultiplayerConnectionUI_UpdateModemItem};
    for (int i = 0; i < 3; ++i) {
        MenuWidget *widget = add(panel->fX, panel->fY + 19 * i, 0x102001, 1, -1);
        widget->pfnIdleAction = updates[i];
        widget->pfnAction = actions[i];
        SetMenuWidgetLabel(widget, 0x55 + i, &g_rcMenuCenterPanel);
        ++panel->wWidgetCount;
        ++panel->wNavigationCount;
    }
    MenuWidget *widget = add(panel->fX, panel->fY + 114, 0x102001, 1, -1);
    widget->pfnIdleAction = ValidateEntityState;
    widget->pfnAction = MultiplayerConnectionUI_OnBack;
    SetMenuWidgetLabel(widget, 0xc, &g_rcMenuCenterPanel);
    ++panel->wWidgetCount;
    ++panel->wNavigationCount;
    add(95, 317, 9, 0, 103);
    ++panel->wWidgetCount;
    widget = add(130, 336, 0x100009, 1, -1);
    widget->nSpriteId = 0x34;
    widget->SetText(GetStringResource(0x59), 1, 4, &g_rcMenuCenterPanel);
    ++panel->wWidgetCount;
    SetMenuWidgetLabel(add(130, 374, 0x100009, 1, -1), 0x5a, &g_rcMenuCenterPanel);
    ++panel->wWidgetCount;
    add(155, 393, 9, 0, 57);
    ++panel->wWidgetCount;
    for (int i = 0; i < 2; ++i) {
        int top = 393 + 38 * i;
        widget = add(155, static_cast<float>(top), 0x200001, 1, -1);
        widget->pfnAction = ConnectionUI_OnToggleTextEdit;
        widget->pfnUpdate = i == 0 ? OnTcpIpPortEdited : OnTcpIpAddressEdited;
        widget->pfnTextEditComplete = CompleteConnectionTextEdit;
        widget->nSpriteId = 3;
        widget->SetText(g_szEmptyUiString, 1, 1, &g_rcMenuCenterPanel);
        MenuFont *font = &widget->fonts[0];
        font->pVTable->MeasureText(font);
        widget->rcBounds.left = font->rcBounds.left = 155;
        widget->rcBounds.right = font->rcBounds.right = 347;
        widget->rcBounds.top = font->rcBounds.top = top;
        widget->rcBounds.bottom = font->rcBounds.bottom = top + 16;
        font->nInputMaxWidth =
            static_cast<short>(static_cast<unsigned short>(font->rcBounds.right) -
                               static_cast<unsigned short>(font->rcBounds.left) - 16);
        ++panel->wWidgetCount;
        ++panel->wNavigationCount;
        if (i == 0) {
            SetMenuWidgetLabel(add(130, 412, 0x100009, 1, -1), 0x5b, &g_rcMenuCenterPanel);
            ++panel->wWidgetCount;
            add(155, 431, 9, 0, 57);
            ++panel->wWidgetCount;
        }
    }
    add(80, 46, 9, 0, 7);
    ++panel->wWidgetCount;
    widget = add(339, 61, 1, 1, 69);
    widget->pfnUpdate = ConnectionUI_UpdateServiceProviderImage;
    ++widget->nLinkedWidgetCount;
    widget->anLinkedWidgetIndices[0] = panel->wWidgetCount + 1;
    ++panel->wWidgetCount;
    widget = add(panel->fX, 160, 0x101001, 1, -1);
    widget->pfnUpdate = ConnectionUI_UpdateServiceProviderLabel;
    widget->nSpriteId = 3;
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuRightColumnMid);
    ++panel->wWidgetCount;
}

void BuildPreRaceInfoUI(MenuScreen *pScreen) {
    pScreen->wCurrentPanel = 0;
    pScreen->anPanelActive[pScreen->wPanelCount] = 1;
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    pScreen->apPanels[pScreen->wPanelCount] = panel;
    if (!panel) {
        ShowErrorMessage(4);
    }
    ++pScreen->wPanelCount;
    panel->pScreen = pScreen;
    panel->nFlags = 0;
    panel->fX = 195;
    panel->fY = 130;
    auto add = [&](float x, float y, unsigned flags, int mode, short image) {
        return AddMenuWidget(panel, pScreen, x, y, flags, mode, image);
    };
    auto text = [](MenuWidget *widget, int stringId, const RECT *rect) {
        SetMenuWidgetLabel(widget, stringId, rect);
    };
    add(0, 0, 9, 0, 0);
    ++panel->wWidgetCount;
    MenuWidget *widget = add(panel->fX, 10, 0x100009, 1, -1);
    text(widget, 0x3d, &g_rcMenuTitleBar);
    ++panel->wWidgetCount;
    widget = add(panel->fX, panel->fY, 0x3001, 1, -1);
    MenuItemList *list = &widget->itemList;
    widget->pfnUpdate = SeasonRoundList_UpdateRoundSelection;
    list->nSelectionMode = 1;
    list->pVTable->Initialize(list, 0x240000, &g_rcMenuListBox, 5, 0, -1);
    widget->pfnBeforeUpdate = SeasonRoundList_SetBrowsingFlag;
    widget->pfnAfterUpdate = SeasonRoundList_ClearBrowsingFlag;
    if (g_SeasonTable[g_bSeasonClass].raceCount > 0) {
        auto addItem = list->pVTable->AddItem;
        for (int round = 0; round < g_SeasonTable[g_bSeasonClass].raceCount; ++round) {
            char number[32];
            char label[32];
            _itoa(round + 1, number, 10);
            sprintf(label, GetStringResource(0xbf), _strupr(number));
            addItem(list, label, 1, 1, &g_rcMenuCenterPanel, true);
        }
    }
    ++panel->wWidgetCount;
    widget = add(panel->fX, panel->fY + 95, 0x102001, 1, -1);
    widget->pfnIdleAction = ValidateEntityState;
    widget->pfnAction = ShowSeasonProgressInfoScreen;
    widget->nSpriteId = 2;
    widget->SetText(GetStringResource(0x1e), 1, 0, &g_rcMenuCenterPanel);
    ++panel->wWidgetCount;
    ++panel->wNavigationCount;
    widget = add(panel->fX, panel->fY + 114, 0x102001, 1, -1);
    widget->pfnIdleAction = ValidateEntityState;
    widget->pfnAction = PreRaceInfoUI_OnBack;
    widget->pfnBeforeUpdate = PreRaceInfoUI_UpdateBackButtonText;
    widget->nSpriteId = 3;
    widget->SetText(GetStringResource(0xc), 1, 1, &g_rcMenuCenterPanel);
    ++panel->wWidgetCount;
    ++panel->wNavigationCount;
    AddMenuActionButtons(panel, pScreen, PreRaceInfoUI_OnSaveSeason, PreRaceInfoUI_OnOptions);
    widget = add(73, 212, 1, 1, 22);
    widget->pfnUpdate = RaceSetupUI_UpdateWeatherImage;
    ++widget->nLinkedWidgetCount;
    widget->anLinkedWidgetIndices[0] = panel->wWidgetCount + 1;
    ++panel->wWidgetCount;
    widget = add(panel->fX, 275, 0x101001, 1, -1);
    widget->pfnUpdate = RaceSetupUI_UpdateWeatherLabel;
    widget->nSpriteId = 3;
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuLeftColumn);
    ++panel->wWidgetCount;
    widget = add(325, 72, 1, 1, 17);
    widget->pfnUpdate = RaceSetupUI_UpdateTrackImage;
    ++widget->nLinkedWidgetCount;
    widget->anLinkedWidgetIndices[0] = panel->wWidgetCount + 1;
    ++panel->wWidgetCount;
    widget = add(panel->fX, 152, 0x101001, 1, -1);
    widget->pfnUpdate = RaceSetupUI_UpdateTrackNameLabel;
    widget->nSpriteId = 3;
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuRightColumnMid);
    ++panel->wWidgetCount;
    text(add(450, 147, 0x100009, 1, -1), 0x3e, &g_rcMenuRightListBox);
    ++panel->wWidgetCount;
    widget = add(450, 128, 0x5001, 1, -1);
    widget->pfnUpdate = PreRaceInfoUI_UpdateBestTimeLabel;
    widget->nSpriteId = 0x34;
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuRightListBox);
    ++panel->wWidgetCount;
    text(add(450, 185, 0x100009, 1, -1), 0x3f, &g_rcMenuRightListBox);
    ++panel->wWidgetCount;
    widget = add(450, 166, 0x5001, 1, -1);
    widget->pfnUpdate = PreRaceInfoUI_UpdateBestLapLabel;
    widget->nSpriteId = 0x34;
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuRightListBox);
    ++panel->wWidgetCount;
    text(add(panel->fX, 203, 0x100009, 1, -1), 0x40, &g_rcMenuRightColumnWide);
    ++panel->wWidgetCount;
    text(add(panel->fX, 265, 0x100009, 1, -1), 0x41, &g_rcMenuRightColumnWide);
    ++panel->wWidgetCount;
    widget = add(450, 217, 0x5001, 1, -1);
    widget->pfnUpdate = SeasonUI_UpdateTotalPointsLabel;
    widget->nSpriteId = 5;
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightColumnWide);
    ++panel->wWidgetCount;
    text(add(106, 85, 0x100001, 1, -1), 0x38, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    widget = add(440, 66, 0x5001, 1, -1);
    widget->pfnUpdate = SeasonUI_UpdateSeasonNameLabel;
    widget->nSpriteId = 0x34;
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    text(add(106, 123, 0x100001, 1, -1), 0x36, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
    widget = add(440, 104, 0x5001, 1, -1);
    widget->pfnUpdate = SeasonUI_UpdateYearOrdinalLabel;
    widget->nSpriteId = 0x34;
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuSubPanel);
    ++panel->wWidgetCount;
}

int ShowPreRaceInfoScreen(void) {
    return ShowCachedMenuWithExitConfirmation(g_pCachedMenu_PreRaceInfo,
                                              BuildPreRaceInfoUI,
                                              g_pPreRaceInfoMenuScreen,
                                              BuildPreRaceExitSeasonConfirmUI);
}

int RaceResultsTotalsUI_OnContinue(MenuActionWidget *pItem) {
    ApplyLoadedSeasonSettings(true);
    if (g_pCachedMenu_RaceResults) {
        g_pCachedMenu_RaceResults->DestroyPanels();
        g_pCachedMenu_RaceResults = nullptr;
    }
    if (g_pCachedMenu_RaceResultsSummary) {
        g_pCachedMenu_RaceResultsSummary->DestroyPanels();
        g_pCachedMenu_RaceResultsSummary = nullptr;
    }
    if (g_dwSeasonFailed) {
        ShowSeasonCongratsScreen();
        g_dwSeasonFailed = 0;
    } else if (g_dwSeasonRoundQualified) {
        ShowNotEnoughPointsScreen();
        g_dwSeasonRoundQualified = 0;
    } else {
        ShowPreRaceInfoScreen();
    }
    pItem->pScreen->DestroyPanels();
    delete g_pCachedMenu_RaceResultsTotals;
    g_pCachedMenu_RaceResultsTotals = nullptr;
    return 0;
}

static inline bool DestroyCachedMenu(MenuScreen *&pScreen) {
    if (pScreen == nullptr) {
        return false;
    }
    pScreen->DestroyPanels();
    delete pScreen;
    pScreen = nullptr;
    return true;
}

void MenuScreen::ShutdownFrontEnd(int bPartial) {
    DWORD dwDelay = 0;
    DWORD dwEnd;

    if (g_dwNetworkMenuState == 2) {
        dwDelay = SHUTDOWN_DELAY_STATE_2_MS;
    } else if (g_dwNetworkMenuState == 3) {
        dwDelay = SHUTDOWN_DELAY_STATE_3_MS;
    }
    dwEnd = GetHighResolutionTime() + dwDelay;
    while (GetHighResolutionTime() < dwEnd) {
        Sleep(0);
    }
    if (!bPartial) {
        g_dwNullStubEnable = false;
        g_pActiveMenuScreen = nullptr;
        ReleaseMenuSurfaces();
        for (MenuImage &image : g_aMenuImages) {
            delete[] image.pszPath;
            image.pszPath = nullptr;
        }
    }
    // The connection screen is freed first, and its pointer is checked again further down.
    if (DestroyCachedMenu(g_pCachedMenu_MultiplayerConnection)) {
        g_dwDirectPlaySessionActive = false;
    }
    if (DestroyCachedMenu(g_pCachedMenu_MainMenu)) {
        g_pRaceSetupMenu = nullptr;
    }
    DestroyCachedMenu(g_pCachedMenu_MainMenuAlternate);
    if (DestroyCachedMenu(g_pCachedMenu_CarSetup)) {
        g_pCarSetupMenu = nullptr;
    }
    DestroyCachedMenu(g_pCachedMenu_Options);
    DestroyCachedMenu(g_pCachedMenu_TimeTrial);
    if (DestroyCachedMenu(g_pCachedMenu_Unused2920)) {
        g_pPlayerNameContinueItem = nullptr;
    }
    DestroyCachedMenu(g_pCachedMenu_LoadSeason);
    if (DestroyCachedMenu(g_pCachedMenu_SaveSeason)) {
        g_pSaveSeasonMenu = nullptr;
        g_pSaveSeasonEditMenu = nullptr;
        g_pSaveSeasonBackHandlerMenu = nullptr;
    }
    if (DestroyCachedMenu(g_pCachedMenu_PreRaceInfo)) {
        g_pExitSeasonConfirmMenu = nullptr;
    }
    DestroyCachedMenu(g_pCachedMenu_SeasonProgressInfo);
    DestroyCachedMenu(g_pCachedMenu_RaceResults);
    DestroyCachedMenu(g_pCachedMenu_RaceResultsSummary);
    DestroyCachedMenu(g_pCachedMenu_RaceResultsTotals);
    DestroyCachedMenu(g_pCachedMenu_MultiplayerConnection);
    DestroyCachedMenu(g_pCachedMenu_Unused2944);
    if (DestroyCachedMenu(g_pCachedMenu_JoinCreateGame)) {
        g_pMultiplayerGameSetupMenu = nullptr;
        g_pNetworkSessionJoinMenu = nullptr;
        g_pJoinCreateGameMenu = nullptr;
        g_dwSelectedSessionIndex = 0;
    }
    if (DestroyCachedMenu(g_pCachedMenu_CreateGame)) {
        g_pMultiplayerGameSetupMenu = nullptr;
    }
    DestroyCachedMenu(g_pCachedMenu_RaceSetupNetwork);
    if (DestroyCachedMenu(g_pCachedMenu_NetworkCarSetup)) {
        g_pNetworkPlayerListMenu = nullptr;
        g_pNetworkCarSetupMenu = nullptr;
    }
    if (DestroyCachedMenu(g_pCachedMenu_MultiplayerName)) {
        g_pPlayerNameContinueItem = nullptr;
    }
    if (DestroyCachedMenu(g_pCachedMenu_ModemDialUp)) {
        g_pModemContinueItem = nullptr;
    }
    DestroyCachedMenu(g_pCachedMenu_TimeAttack);
    DestroyCachedMenu(g_pCachedMenu_Unused2960);
    DestroyCachedMenu(g_pCachedMenu_LoadGhost);
    if (DestroyCachedMenu(g_pCachedMenu_SaveGhost)) {
        g_pSaveTimeAttackMenu = nullptr;
        g_pSaveTimeAttackEditMenu = nullptr;
    }
    DestroyCachedMenu(g_pCachedMenu_SeasonCongrats);
    DestroyCachedMenu(g_pCachedMenu_NotEnoughPoints);
    DestroyCachedMenu(g_pPreRaceInfoMenuScreen);
    DestroyCachedMenu(g_pCachedMenu_Credits);
    DestroyCachedMenu(g_pCachedMenu_InputConfig);
    DestroyCachedMenu(g_pCachedMenu_GameplaySettings);
    DestroyCachedMenu(g_pCachedMenu_Championship);
    if (DestroyCachedMenu(g_pCachedMenu_VideoSettings)) {
        g_pD3DDeviceSelectMenu = nullptr;
    }
    if (DestroyCachedMenu(g_pCachedMenu_AudioSettings)) {
        g_pDisplayModeSelectMenu = nullptr;
    }
    DestroyCachedMenu(g_pCachedMenu_KeyConfig);
    if (!bPartial) {
        DestroyCachedMenu(g_pRootMenuScreen);
        if (g_pFrontEndScratchBuffer != nullptr) {
            // The original runs a destructor on the buffer that does nothing.
            ::operator delete(g_pFrontEndScratchBuffer);
            g_pFrontEndScratchBuffer = nullptr;
        }
        SyncSeasonProgress();
    }
}

void MenuScreen::DestroyPanels() {
    for (int nPanel = 0; nPanel < wPanelCount; nPanel++) {
        MenuPanel *pPanel = apPanels[nPanel];

        for (MenuWidget *&pWidget : pPanel->apWidgets) {
            delete pWidget;
            pWidget = nullptr;
        }
        delete pPanel;
    }
    g_wMenuHoverIndex = 0;
}

// The settings of the car a game mode starts with.
#define DEFAULT_TRANSMISSION_TYPE 1
#define DEFAULT_TIRE_TYPE 2
#define DEFAULT_SUSPENSION_TYPE 1
#define DEFAULT_HANDLING_TYPE 1

void ResetGameModeConfig(void) {
    g_dwSelectedTrackId = 0;
    g_dwChosenWeather = 0;
    g_dwChosenCar = 0;
    g_dwTransmissionType = DEFAULT_TRANSMISSION_TYPE;
    g_dwTireType = DEFAULT_TIRE_TYPE;
    g_dwSuspensionType = DEFAULT_SUSPENSION_TYPE;
    g_GlobalRallyConfig.dwInputDeviceType = 0;
    g_GlobalRallyConfig.pActiveInputBindings = g_GlobalRallyConfig.aInputBindingsKeyboard;
    g_dwHandlingType = DEFAULT_HANDLING_TYPE;
}

// The string of the caption of the message box of an error.
#define ERROR_CAPTION_STRING 0xaa

ErrorMessage g_ErrorMessageTable[ERROR_MESSAGE_COUNT] = {
    {true, 0xa2},
    {true, 0xa3},
    {false, 0xa4},
    {false, 0xa5},
    {true, 0xa6},
    {false, 0xa7},
    {true, 0xa8},
    {true, 0xa9},
};

LRESULT WINAPI
FrontEndWindowProc(void *pUnused, HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 1;
    switch (nMessage) {
    case WM_ACTIVATE:
        g_fHostWindowActive = wParam == 0;
        break;
    case WM_CHAR:
        if (!g_fCharInputSuppressed && !(g_abKeyboardState[DIK_UP] & 0x80) &&
            !(g_abKeyboardState[DIK_DOWN] & 0x80) && !(g_abKeyboardState[DIK_LEFT] & 0x80) &&
            !(g_abKeyboardState[DIK_RIGHT] & 0x80)) {
            g_dwLastWmCharParam = static_cast<int>(wParam);
        }
        return 1;
    case WM_COMMAND:
        if (LOWORD(wParam) == 0x9c41) {
            g_dwLastWmCharParam = VK_ESCAPE;
        }
        return 1;
    case WM_SYSCOMMAND:
        result = DefWindowProcA(hWnd, WM_SYSCOMMAND, wParam, lParam);
        if (!IsWindow(hWnd)) {
            return result;
        }
        break;
    case WM_ENTERMENULOOP:
    case WM_ENTERSIZEMOVE:
        g_fHostWindowActive = 1;
        break;
    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE:
        if (GetActiveWindow() == hWnd) {
            BOOL iconic = IsIconic(hWnd);
            g_fHostWindowActive = 0;
            if (iconic) {
                g_fHostWindowActive = 1;
            }
        } else {
            g_fHostWindowActive = 1;
        }
        PostMessageA(hWnd, WM_USER, 0, 0);
        return 1;
    case WM_USER:
        break;
    default:
        return 1;
    }
    if (g_pMouseDevice) {
        g_pMouseDevice->SyncMouseAcquireState(hWnd);
    }
    return result;
}

void ShowErrorMessage(int nErrorCode) {
    // A code of ERROR_MESSAGE_COUNT passes the test and reads past the end of the table.
    if (nErrorCode > ERROR_MESSAGE_COUNT) {
        return;
    }
    const char *pszMessage = GetStringResource(g_ErrorMessageTable[nErrorCode].nStringId);
    MessageBoxA(g_hMainWindow, pszMessage + 1, GetStringResource(ERROR_CAPTION_STRING), MB_OK);
    if (g_ErrorMessageTable[nErrorCode].bFatal) {
        exit(1);
    }
}

int g_dwCheatAllTracksUnlocked;
int g_dwCheatBonusTracksUnlocked;
int g_dwCheatBonusCarsUnlocked;
int g_dwCheatOutroSequenceEnabled;
short g_wVehicleDeformMinThreshold = 0x100;

// The samples of the sounds of the cheats, and the levels of both sides they play at.
#define CHEAT_SOUND_SAMPLE 2
#define CHEAT_DEFORMATION_SOUND_SAMPLE 3
#define CHEAT_SOUND_LEVELS 0x200020
// The states of the network menus after a cheat.
#define CHEAT_NETWORK_MENU_STATE 2
#define CHEAT_DEFORMATION_NETWORK_MENU_STATE 3
// The force no impact reaches.
#define VEHICLE_DEFORM_THRESHOLD_NEVER 0x7fff

static inline void EnableCheat(int *pbFlag) {
    *pbFlag = true;
    PlaySoundChannelOneOnce(CHEAT_SOUND_SAMPLE, CHEAT_SOUND_LEVELS);
    g_dwNetworkMenuState = CHEAT_NETWORK_MENU_STATE;
}

void Cheat_Hazel_UnlockAllTracks(void) {
    EnableCheat(&g_dwCheatAllTracksUnlocked);
}

void Cheat_Brielle_UnlockBonusTracks(void) {
    EnableCheat(&g_dwCheatBonusTracksUnlocked);
}

void Cheat_DisableVehicleDeformation(void) {
    g_wVehicleDeformMinThreshold = VEHICLE_DEFORM_THRESHOLD_NEVER;
    PlaySoundChannelOneOnce(CHEAT_DEFORMATION_SOUND_SAMPLE, CHEAT_SOUND_LEVELS);
    g_dwNetworkMenuState = CHEAT_DEFORMATION_NETWORK_MENU_STATE;
}

void Cheat_Sophia_EnableOutroSequence(void) {
    EnableCheat(&g_dwCheatOutroSequenceEnabled);
}

void Cheat_Lynette_UnlockBonusCars(void) {
    EnableCheat(&g_dwCheatBonusCarsUnlocked);
}

int g_dwCheatAllCarsUnlocked;

void Cheat_Benjamin_UnlockAllCars(void) {
    EnableCheat(&g_dwCheatAllCarsUnlocked);
}

CheatCode g_aCheatCodes[CHEAT_CODE_COUNT] = {
    {Cheat_DisableVehicleDeformation, "madeleine"},
    {Cheat_Benjamin_UnlockAllCars, "benjamin"},
    {Cheat_Brielle_UnlockBonusTracks, "brielle"},
    {Cheat_Lynette_UnlockBonusCars, "lynette"},
    {Cheat_Sophia_EnableOutroSequence, "sophia"},
    {Cheat_Hazel_UnlockAllTracks, "hazel"},
    {nullptr, nullptr},
};
int g_anCheatInputBuffer[CHEAT_INPUT_BUFFER_SIZE];
int g_dwCheatInputCount;
int g_dwLastWmCharParam;

void CheckCheatCodeMatches(void) {
    for (CheatCode *pCheat = g_aCheatCodes; pCheat->pszCode != nullptr; pCheat++) {
        int nLength = static_cast<int>(strlen(pCheat->pszCode));
        int i;

        for (i = 0; i < nLength; i++) {
            int nEntry = (g_dwCheatInputCount + i - nLength) & (CHEAT_INPUT_BUFFER_SIZE - 1);
            if (pCheat->pszCode[i] != static_cast<char>(g_anCheatInputBuffer[nEntry])) {
                break;
            }
        }
        if (i == nLength) {
            pCheat->pfnApply();
        }
    }
}

void ProcessCheatCodeInput(void) {
    char chInput;

    if (g_dwLastWmCharParam == 0) {
        return;
    }
    chInput = static_cast<char>(g_dwLastWmCharParam);
    if (chInput >= 'A' && chInput <= 'Z') {
        chInput += 'a' - 'A';
    }
    g_anCheatInputBuffer[g_dwCheatInputCount++] = chInput;
    if (g_dwCheatInputCount >= CHEAT_INPUT_BUFFER_SIZE) {
        g_dwCheatInputCount = 0;
    }
    CheckCheatCodeMatches();
    g_dwLastWmCharParam = 0;
}

int EnterTimeAttackMode(void) {
    if (g_aVehicles[0].lap >= g_dwTotalLaps) {
        ShowTimeAttackMenu(0);
        ShowSaveGhostMenu(0);
    }
    g_dwGameMode = GAME_MODE_TIME_ATTACK;
    return true;
}

void ApplyCarGroupPreset(void) {
    if (g_dwAppliedCarGroupPreset == g_dwSelectedCarClass) {
        return;
    }
    g_dwAppliedCarGroupPreset = g_dwSelectedCarClass;
    switch (g_dwSelectedCarClass) {
    case 0:
        g_dwSelectableCarMask = 0x102;
        g_dwCarMenuIndex = 1;
        break;
    case 1:
        g_dwSelectableCarMask = 0x81;
        g_dwCarMenuIndex = 0;
        break;
    case 2:
        g_dwSelectableCarMask = 0x4050;
        g_dwCarMenuIndex = 6;
        break;
    case 3:
        g_dwSelectableCarMask = 0x202c;
        g_dwCarMenuIndex = 3;
        break;
    case 4:
        g_dwSelectableCarMask = 0x1e00;
        g_dwCarMenuIndex = 11;
        break;
    default:
        g_dwCarMenuIndex = 1;
        g_dwSelectableCarMask = 0x102;
        break;
    }
}

unsigned IsTrackSelectable(int nTrack) {
    if (nTrack == 12) {
        return 0;
    }
    if (g_dwCheatAllTracksUnlocked) {
        return 1;
    }
    const unsigned mask = 1u << (static_cast<unsigned>(nTrack) & 31);
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        return mask & (g_dwSeasonLoaded ? g_SeasonProgress.unlockedTracks : g_dwUnlockedTrackMask);
    }
    if (g_dwCheatBonusTracksUnlocked && (nTrack == 13 || nTrack == 14)) {
        return 1;
    }
    return mask & static_cast<unsigned>(g_dwSelectableTrackMaskAlt);
}

void ResolveRaceSelections(void) {
    RestoreMenuSelections();
    g_dwHandlingType = g_dwHandlingTypeMenuIndex;
    if (g_dwGameMode == GAME_MODE_NETWORK) {
        ApplyCarGroupPreset();
    }
    const int originalCar = g_dwCarMenuIndex;
    if (!IsCarSelectable(g_dwCarMenuIndex)) {
        do {
            if (++g_dwCarMenuIndex > 31) {
                g_dwCarMenuIndex = 0;
            }
            if (g_dwCarMenuIndex == originalCar) {
                break;
            }
        } while (!IsCarSelectable(g_dwCarMenuIndex));
    }
    g_dwChosenCar = g_adwCarMenuIndexToCarId[g_dwCarMenuIndex];
    g_dwTireType = g_adwTireMenuIndexToTireType[g_dwTireMenuIndex];
    g_dwSuspensionType = g_adwSuspensionMenuIndexToType[g_dwSuspensionMenuIndex];
    g_dwTransmissionType = g_adwTransmissionMenuIndexToType[g_dwSelectedTransmission];
    if (g_dwGameMode != GAME_MODE_CHAMPIONSHIP) {
        const int originalTrack = g_dwTrackMenuIndex;
        if (!IsTrackSelectable(g_dwTrackMenuIndex)) {
            do {
                if (++g_dwTrackMenuIndex > (g_dwCheatBonusTracksUnlocked ? 14 : 11)) {
                    g_dwTrackMenuIndex = 0;
                } else if (g_dwTrackMenuIndex == originalTrack) {
                    break;
                }
            } while (!IsTrackSelectable(g_dwTrackMenuIndex));
        }
        g_dwSelectedTrackId = g_adwTrackMenuIndexToTrackId[g_dwTrackMenuIndex];
        g_dwTotalLaps = g_dwTotalLapsSetting;
        g_dwChosenWeather = g_adwWeatherMenuIndexToWeather[g_dwSelectedWeather];
    } else {
        const u8 *race = g_SeasonTable[g_SeasonProgress.season].races[g_SeasonProgress.round];
        g_dwSelectedTrackId = race[0];
        g_dwChosenWeather = race[1];
    }
    SyncSeasonProgress();
}

void ResetMenuSelectionsToDefaults(void) {
    memset(g_awSeasonRoundPoints, 0, sizeof(g_awSeasonRoundPoints));
    g_dwTrackMenuIndex = 2;
    g_dwSelectedWeather = 0;
    g_dwSelectedTransmission = 0;
    g_dwTireMenuIndex = 1;
    g_dwSuspensionMenuIndex = 1;
    g_dwCarMenuIndex = 1;
    g_dwTotalLapsSetting = 3;
    g_dwAccumUnlockedCarMask = 0;
    g_dwAccumUnlockedTrackMask = 0;
    g_dwSeasonYear = 0;
    g_dwSeasonRound = 0;
    g_dwSeasonSavedRound = 0;
    g_dwSeasonFailed = 0;
    g_dwSeasonRoundQualified = 0;
    g_bSeasonClass = 0;
    g_dwUnusedSeasonMenuValue = 0;
    g_dwLastRaceResultField18 = 0;
    memset(g_abSeasonRoundPlace, 0, sizeof(g_abSeasonRoundPlace));
    g_dwSeasonTotalPoints = 0;
    g_fGhostBestRaceTime = 0;
    g_dwRaceResultsSelectedLap = 0;
    g_dwSeasonLoaded = 0;
    sprintf(g_szSeasonYearLabel, "%d", 1);
    sprintf(g_szSeasonRoundLabel, "%d", g_dwSeasonRound + 1);
    ResetSeasonSaveBuffers();
    g_dwSeasonLoaded = 0;
    memset(&g_SeasonProgress, 0, sizeof(g_SeasonProgress));
    memset(&g_SeasonProgressPrev, 0, sizeof(g_SeasonProgressPrev));
    memset(&g_SeasonResumeState, 0, sizeof(g_SeasonResumeState));
    g_SeasonProgress.unlocks = 0x102;
    g_SeasonResumeState.state = -1;
    ResolveRaceSelections();
}

void BackupMenuSelections(void) {
    int *pBackup = g_GlobalRallyConfig.adwMenuSelectionBackup;

    pBackup[0] = g_dwTrackMenuIndex;
    pBackup[1] = g_dwSelectedWeather;
    pBackup[2] = g_dwSelectedTransmission;
    pBackup[3] = g_dwTireMenuIndex;
    pBackup[4] = g_dwSuspensionMenuIndex;
    pBackup[5] = g_dwCarMenuIndex;
    pBackup[6] = g_dwSelectedInputDevice;
    pBackup[7] = g_dwTotalLapsSetting;
    pBackup[8] = g_dwAccumUnlockedCarMask;
    pBackup[9] = g_dwAccumUnlockedTrackMask;
    pBackup[10] = g_dwHandlingTypeMenuIndex;
    pBackup[11] = g_dwSelectedCarClass;
}

#if TGR_RENDERER == TGR_RENDERER_D3D
void SelectConfiguredDisplayMode(void) {
    g_pSelectedDisplayDriver =
        FindDirectDrawDriverOrDefault(&g_GlobalRallyConfig.DisplayDriverGuid);
    g_pSelectedDisplayMode = FindDisplayModeWithFallback(g_pSelectedDisplayDriver,
                                                         g_GlobalRallyConfig.dwDisplayModeWidth,
                                                         g_GlobalRallyConfig.dwDisplayModeHeight,
                                                         g_GlobalRallyConfig.dwDisplayModeBitDepth,
                                                         0,
                                                         g_pSelectedRenderDevice);
}
#endif

void RestoreMenuSelections(void) {
    const int *pBackup = g_GlobalRallyConfig.adwMenuSelectionBackup;

    // The device comes from the choice before the choice is restored below.
    g_GlobalRallyConfig.dwInputDeviceType =
        g_adwInputDeviceMenuIndexToDevice[g_dwSelectedInputDevice];
    g_GlobalRallyConfig.SelectActiveInputBindings();
    g_dwOptionForceFeedback = !g_GlobalRallyConfig.bForceFeedbackEnabled;
    g_dwOptionCarShadow = !g_GlobalRallyConfig.bCarShadowSelection;
    g_dwOptionSpecularLighting = !g_GlobalRallyConfig.bSpecularLightingSelection;
    g_dwOptionSkidMarks = !g_GlobalRallyConfig.bSkidMarksSelection;
    strcpy(g_szModemPhoneNumber, g_GlobalRallyConfig.szSavedModemPhoneNumber);
#if TGR_RENDERER == TGR_RENDERER_D3D
    SelectConfiguredDisplayMode();
#else
    NullStub();
#endif
    g_dwTrackMenuIndex = pBackup[0];
    g_dwSelectedWeather = pBackup[1];
    g_dwSelectedTransmission = pBackup[2];
    g_dwTireMenuIndex = pBackup[3];
    g_dwSuspensionMenuIndex = pBackup[4];
    g_dwCarMenuIndex = pBackup[5];
    g_dwSelectedInputDevice = pBackup[6];
    // A choice of 1 is restored as 2, as in the original.
    if (g_dwSelectedInputDevice == 1) {
        g_dwSelectedInputDevice = 2;
    }
    g_dwTotalLapsSetting = pBackup[7];
    g_dwHandlingTypeMenuIndex = pBackup[10];
    g_wUnlockedCarModeMask |= static_cast<unsigned short>(pBackup[8]);
    g_dwAccumUnlockedCarMask = pBackup[8];
    g_dwAccumUnlockedTrackMask = pBackup[9];
    g_dwSelectedCarClass = pBackup[11];
    g_dwSelectableTrackMaskAlt |= pBackup[9];
}

// Place the rectangles of a table on a grid of cells, row by row.
static inline void SetGridRects(RECT *pRects, int nCount, int nColumns, int nWidth, int nHeight) {
    for (int i = 0; i < nCount; i++) {
        pRects[i].left = i % nColumns * nWidth;
        pRects[i].top = i / nColumns * nHeight;
        pRects[i].right = pRects[i].left + nWidth;
        pRects[i].bottom = pRects[i].top + nHeight;
    }
}

void InitMenuSpriteRects(void) {
    SetGridRects(g_aMenuGlyphRects, MENU_GLYPH_RECT_COUNT, 8, 16, 16);
    SetGridRects(g_aMenuAltGlyphRects, MENU_ALT_GLYPH_RECT_COUNT, 5, 39, 44);
    SetGridRects(g_aMenuLargeIconRects, MENU_LARGE_ICON_RECT_COUNT, 5, 128, 128);
    SetGridRects(g_aMenuLargeIconRects3Col, MENU_LARGE_ICON_3_COLUMN_RECT_COUNT, 3, 128, 128);
}

bool InitializeFrontEndResources(void) {
    memset(g_aMenuImages, 0, sizeof(g_aMenuImages));
    g_wMenuImageLoadedCount = 0;
    g_wUnusedFrontEndWord0 = 0;
    g_wUnusedFrontEndWord1 = 0;
    ClearKeyboardStateBuffers();
    // A path that is not allocated is copied to all the same, as in the original.
    for (int i = 0; i < MENU_IMAGE_COUNT; i++) {
        g_aMenuImages[i].pszPath = new (std::nothrow) char[MENU_IMAGE_PATH_SIZE];
        strcpy(g_aMenuImages[i].pszPath, s_apszMenuImageFiles[i]);
    }

    memset(g_aFrontEndResourceSlots, 0xff, sizeof(g_aFrontEndResourceSlots));
    memset(g_adwFrontEndResourceFlags, 0xff, sizeof(g_adwFrontEndResourceFlags));
    for (FrontEndResourceSlot &slot : g_aFrontEndResourceSlots) {
        slot.anValues[2] = 0;
    }

    g_pRootMenuScreen = new (std::nothrow) MenuScreen();
    g_pActiveMenuScreen = g_pRootMenuScreen;
    if (g_pRootMenuScreen == nullptr) {
        return false;
    }
    g_pRootMenuScreen->pfnBuildMenu = BuildFrontEndRootMenuUI;
    if (g_pFrontEndScratchBuffer == nullptr) {
        // The original runs a constructor on the buffer that does nothing.
        g_pFrontEndScratchBuffer = ::operator new(FRONT_END_SCRATCH_SIZE, std::nothrow);
        if (g_pFrontEndScratchBuffer == nullptr) {
            ShowErrorMessage(1);
        }
    }
    strcpy(g_szSeasonSaveFileName, "c:\\RallySeason.dat");
    strcpy(g_szTimeAttackSaveFileName, "c:\\RallyGhost.dat");
    InitMenuSpriteRects();
    return true;
}

char LookupKeyCharacter(DWORD dwKey) {
    // The search runs to the size of the table in bytes, past its last entry, until a key matches.
    for (size_t i = 0; i < sizeof(g_KeyNameLookupTable); i++) {
        if (g_KeyNameLookupTable[i].dwKey == dwKey) {
            return g_KeyNameLookupTable[i].chText;
        }
    }
    return 0;
}
