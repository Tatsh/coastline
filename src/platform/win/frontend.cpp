#include "platform/win/frontend.h"

#include <concepts>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include <direct.h>
#include <io.h>
#include <windows.h>

#include "vehicleaccess.h"
#include "vehicledeform.h"

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
void *g_pFrontEndScratchBuffer;
MenuScreen *g_pRootMenuScreen;
char g_szSeasonSaveFileName[MAX_PATH];
char g_szTimeAttackSaveFileName[MAX_PATH];
RECT g_aMenuGlyphRects[MENU_GLYPH_RECT_COUNT];
RECT g_aMenuAltGlyphRects[MENU_ALT_GLYPH_RECT_COUNT];
int g_dwSelectedCdTrack;
int g_dwSessionListRefreshing;
int g_dwFrontEndAtRootMenu;
int g_dwTrackMenuIndex = 2;
int g_dwSelectedWeather;
int g_dwSelectedTransmission;
int g_dwTireMenuIndex = 1;
int g_dwSuspensionMenuIndex = 1;
int g_dwCarMenuIndex = 1;
int g_dwTotalLapsSetting = 3;
int g_dwAccumUnlockedCarMask;
int g_dwAccumUnlockedTrackMask;
int g_dwHandlingTypeMenuIndex = 4;
int g_dwSelectedCarClass;
const int g_adwTireLabelStrIds[3] = {126, 127, 128};
const int g_adwCarImageIds[32] = {31,  33,  39,  42,  43,  41,  37,  35,  36,  34,  11,
                                  32,  44,  40,  30,  38,  124, 126, 132, 135, 136, 134,
                                  130, 128, 129, 127, 122, 125, 137, 133, 123, 131};
int g_dwNetworkMenuState;
int g_dwSelectedSessionIndex;
int g_dwSelectedDisplayModeIndex;
NetworkMenuWidget *g_pJoinCreateGameMenu;
MenuScreen *g_pCachedMenu_RaceSetupNetwork;
MenuScreen *g_pCachedMenu_GameplaySettings;
MenuScreen *g_pCachedMenu_Championship;
MenuWidget *g_pPlayerNameContinueItem;
MenuWidget *g_pMultiplayerGameSetupMenu;
int g_dwMenuTextEditActive;
RECT g_rcMenuCenterPanel = {148, 110, 358, 260};
RECT g_rcMenuTitleBar = {100, 10, 410, 29};
RECT g_rcMenuListBox = {188, 130, 300, 225};
RECT g_rcMenuLeftColumn = {67, 0, 167, 0};
RECT g_rcMenuRightColumnWide = {478, 0, 578, 0};
RECT g_rcMenuNetworkPlayers = {484, 10, 621, 96};
RECT g_rcMenuRightColumn = {440, 0, 540, 0};
RECT g_rcMenuRightLabel = {330, 70, 447, 86};
const char g_szSpace[] = " ";
char g_szEmptyString[1];
MenuWidget *g_pSaveTimeAttackMenu;
MenuItemList *g_pSaveTimeAttackEditMenu;
NetworkMenuWidget *g_pNetworkSessionJoinMenu;
NetworkMenuWidget *g_pNetworkCarSetupMenu;
MenuWidget *g_pNetworkPlayerListMenu;
MenuWidget *g_pModemContinueItem;
MenuWidget *g_pDisplayModeSelectMenu;
MenuWidget *g_pD3DDeviceSelectMenu;
MenuWidget *g_pOptionsSubMenu;
int g_dwFileListSelectedSlot = -1;

namespace {
// @ghidraAddress BRD3D: 0x100ab4e8
RECT g_rcMenuSessionListBox = {188, 130, 300, 197};
// @ghidraAddress BRD3D: 0x100ab4c8
RECT g_rcMenuWideColumn = {230, 0, 547, 0};
// @ghidraAddress BRD3D: 0x10aa2a5c
unsigned g_dwMenuListRepeatAccumMs;
// @ghidraAddress BRD3D: 0x10aa2a60
unsigned g_dwMenuListRepeatLastTick;
// @ghidraAddress BRD3D: 0x10aa2a64
float g_fMenuListDragDeltaX;
// @ghidraAddress BRD3D: 0x10aa2a68
float g_fMenuListDragDeltaY;
// @ghidraAddress BRD3D: 0x10aa2598
unsigned g_dwUnlockedCarMask = 0x102;
// @ghidraAddress BRD3D: 0x100ab3d8
// @ghidraAddress BRGlide: 0x100aab78
int g_bAudioOptionsSfxSelected;
// @ghidraAddress BRD3D: 0x10a9e360
// @ghidraAddress BRGlide: 0x10ac53e8
MenuImage g_aMenuImages[MENU_IMAGE_COUNT];
// @ghidraAddress BRD3D: 0x10aa28d4
// @ghidraAddress BRGlide: 0x10ac5c2c
unsigned short g_wMenuImageLoadedCount;
// @ghidraAddress BRD3D: 0x10aa29f8
// @ghidraAddress BRGlide: 0x10ac5d50
[[maybe_unused]] unsigned short g_wUnusedFrontEndWord0;
// @ghidraAddress BRD3D: 0x10aa29fc
// @ghidraAddress BRGlide: 0x10ac5d54
[[maybe_unused]] unsigned short g_wUnusedFrontEndWord1;
// @ghidraAddress BRD3D: 0x10a9d778
// @ghidraAddress BRGlide: 0x10ac4800
FrontEndResourceSlot g_aFrontEndResourceSlots[FRONT_END_RESOURCE_SLOT_COUNT];
// @ghidraAddress BRD3D: 0x10a9e1d0
// @ghidraAddress BRGlide: 0x10ac5258
int g_adwFrontEndResourceFlags[FRONT_END_RESOURCE_FLAG_COUNT];
// @ghidraAddress BRD3D: 0x10aa2848
int g_dwSaveSlotListIsSaveMode;
// @ghidraAddress BRD3D: 0x10a9da50
// @ghidraAddress BRGlide: 0x10ac4ad8
// Both grids form a single animation sequence.
RECT g_aMenuLargeIconRects[MENU_LARGE_ICON_RECT_COUNT + MENU_LARGE_ICON_3_COLUMN_RECT_COUNT];
// @ghidraAddress BRD3D: 0x10aa2a4c
// @ghidraAddress BRGlide: 0x10ac5da4
int g_dwFrontEndFrameCounter;
// @ghidraAddress BRD3D: 0x10aa2a0c
// @ghidraAddress BRGlide: 0x10ac5d64
int g_dwSelectedInputDevice;
// @ghidraAddress BRD3D: 0x100ac3c8
const int g_adwCarClassLabelStrIds[5] = {157, 158, 159, 160, 161};
// @ghidraAddress BRD3D: 0x10aa2a44
// @ghidraAddress BRGlide: 0x10ac5d9c
int g_dwAppliedCarGroupPreset;
// @ghidraAddress BRD3D: 0x100ab3e8
// @ghidraAddress BRGlide: 0x100aab88
unsigned g_dwSelectableCarMask = 0x102;
// @ghidraAddress BRD3D: 0x100ac520
// @ghidraAddress BRGlide: 0x100abcc0
const int g_adwInputDeviceMenuIndexToDevice[INPUT_DEVICE_MENU_CHOICES] = {0, 1, 2, 3};
// @ghidraAddress BRD3D: 0x10aa2a1c
// @ghidraAddress BRGlide: 0x10ac5d74
int g_dwOptionForceFeedback;
// @ghidraAddress BRD3D: 0x10aa2a20
// @ghidraAddress BRGlide: 0x10ac5d78
int g_dwOptionCarShadow;
// @ghidraAddress BRD3D: 0x10aa2a24
// @ghidraAddress BRGlide: 0x10ac5d7c
int g_dwOptionSpecularLighting;
// @ghidraAddress BRD3D: 0x10aa2a28
// @ghidraAddress BRGlide: 0x10ac5d80
int g_dwOptionSkidMarks;
// @ghidraAddress BRD3D: 0x100ab3e4
// @ghidraAddress BRGlide: 0x100aab84
unsigned short g_wUnlockedCarModeMask = 0x102;
// @ghidraAddress BRD3D: 0x100ab3ec
// @ghidraAddress BRGlide: 0x100aab8c
int g_dwSelectableTrackMaskAlt = 0x24;
// @ghidraAddress BRD3D: 0x10aa290c
// @ghidraAddress BRGlide: 0x10ac5c64
MenuScreen *g_pCachedMenu_MainMenu;
// @ghidraAddress BRD3D: 0x10aa2910
// @ghidraAddress BRGlide: 0x10ac5c68
MenuScreen *g_pCachedMenu_MainMenuAlternate;
// @ghidraAddress BRD3D: 0x10aa2914
// @ghidraAddress BRGlide: 0x10ac5c6c
MenuScreen *g_pCachedMenu_CarSetup;
// @ghidraAddress BRD3D: 0x10aa2918
// @ghidraAddress BRGlide: 0x10ac5c70
MenuScreen *g_pCachedMenu_Options;
// @ghidraAddress BRD3D: 0x10aa291c
// @ghidraAddress BRGlide: 0x10ac5c74
MenuScreen *g_pCachedMenu_TimeTrial;
// @ghidraAddress BRD3D: 0x10aa2920
// @ghidraAddress BRGlide: 0x10ac5c78
MenuScreen *g_pCachedMenu_Unused2920;
// @ghidraAddress BRD3D: 0x10aa2924
// @ghidraAddress BRGlide: 0x10ac5c7c
MenuScreen *g_pCachedMenu_LoadSeason;
// @ghidraAddress BRD3D: 0x10aa2928
// @ghidraAddress BRGlide: 0x10ac5c80
MenuScreen *g_pCachedMenu_SaveSeason;
// @ghidraAddress BRD3D: 0x10aa292c
// @ghidraAddress BRGlide: 0x10ac5c84
MenuScreen *g_pCachedMenu_PreRaceInfo;
// @ghidraAddress BRD3D: 0x10aa2930
// @ghidraAddress BRGlide: 0x10ac5c88
MenuScreen *g_pCachedMenu_SeasonProgressInfo;
// @ghidraAddress BRD3D: 0x10aa2934
// @ghidraAddress BRGlide: 0x10ac5c8c
MenuScreen *g_pCachedMenu_RaceResults;
// @ghidraAddress BRD3D: 0x10aa2938
// @ghidraAddress BRGlide: 0x10ac5c90
MenuScreen *g_pCachedMenu_RaceResultsSummary;
// @ghidraAddress BRD3D: 0x10aa293c
// @ghidraAddress BRGlide: 0x10ac5c94
MenuScreen *g_pCachedMenu_RaceResultsTotals;
// @ghidraAddress BRD3D: 0x10aa2978
MenuScreen *g_pRaceResultsSummaryMenuScreen;
// @ghidraAddress BRD3D: 0x10aa2940
// @ghidraAddress BRGlide: 0x10ac5c98
MenuScreen *g_pCachedMenu_MultiplayerConnection;
// @ghidraAddress BRD3D: 0x10aa2944
// @ghidraAddress BRGlide: 0x10ac5c9c
MenuScreen *g_pCachedMenu_Unused2944;
// @ghidraAddress BRD3D: 0x10aa2948
// @ghidraAddress BRGlide: 0x10ac5ca0
MenuScreen *g_pCachedMenu_JoinCreateGame;
// @ghidraAddress BRD3D: 0x10aa294c
// @ghidraAddress BRGlide: 0x10ac5ca4
MenuScreen *g_pCachedMenu_CreateGame;
// @ghidraAddress BRD3D: 0x10aa2954
// @ghidraAddress BRGlide: 0x10ac5cac
MenuScreen *g_pCachedMenu_NetworkCarSetup;
// @ghidraAddress BRD3D: 0x10aa2958
// @ghidraAddress BRGlide: 0x10ac5cb0
MenuScreen *g_pCachedMenu_MultiplayerName;
// @ghidraAddress BRD3D: 0x10aa295c
// @ghidraAddress BRGlide: 0x10ac5cb4
MenuScreen *g_pCachedMenu_TimeAttack;
// @ghidraAddress BRD3D: 0x10aa2960
// @ghidraAddress BRGlide: 0x10ac5cb8
MenuScreen *g_pCachedMenu_Unused2960;
// @ghidraAddress BRD3D: 0x10aa2964
// @ghidraAddress BRGlide: 0x10ac5cbc
MenuScreen *g_pCachedMenu_LoadGhost;
// @ghidraAddress BRD3D: 0x10aa2968
// @ghidraAddress BRGlide: 0x10ac5cc0
MenuScreen *g_pCachedMenu_SaveGhost;
// @ghidraAddress BRD3D: 0x10aa296c
// @ghidraAddress BRGlide: 0x10ac5cc4
MenuScreen *g_pCachedMenu_SeasonCongrats;
// @ghidraAddress BRD3D: 0x10aa2970
// @ghidraAddress BRGlide: 0x10ac5cc8
MenuScreen *g_pCachedMenu_NotEnoughPoints;
// @ghidraAddress BRD3D: 0x10aa2974
// @ghidraAddress BRGlide: 0x10ac5ccc
MenuScreen *g_pPreRaceInfoMenuScreen;
// @ghidraAddress BRD3D: 0x10aa297c
// @ghidraAddress BRGlide: 0x10ac5cd4
MenuScreen *g_pCachedMenu_Credits;
// @ghidraAddress BRD3D: 0x10aa2980
// @ghidraAddress BRGlide: 0x10ac5cd8
MenuScreen *g_pCachedMenu_InputConfig;
// @ghidraAddress BRD3D: 0x10aa298c
// @ghidraAddress BRGlide: 0x10ac5ce4
MenuScreen *g_pCachedMenu_ModemDialUp;
// @ghidraAddress BRD3D: 0x10aa2990
// @ghidraAddress BRGlide: 0x10ac5ce8
MenuScreen *g_pCachedMenu_VideoSettings;
// @ghidraAddress BRD3D: 0x10aa2994
// @ghidraAddress BRGlide: 0x10ac5cec
MenuScreen *g_pCachedMenu_AudioSettings;
// @ghidraAddress BRD3D: 0x10aa2998
// @ghidraAddress BRGlide: 0x10ac5cf0
MenuScreen *g_pCachedMenu_KeyConfig;
// @ghidraAddress BRD3D: 0x10aa29ac
// @ghidraAddress BRGlide: 0x10ac5d04
MenuWidget *g_pRaceSetupMenu;
// @ghidraAddress BRD3D: 0x10aa29b0
// @ghidraAddress BRGlide: 0x10ac5d08
MenuWidget *g_pExitSeasonConfirmMenu;
// @ghidraAddress BRD3D: 0x10aa29b4
// @ghidraAddress BRGlide: 0x10ac5d0c
MenuWidget *g_pCarSetupMenu;
// @ghidraAddress BRD3D: 0x10aa29c0
// @ghidraAddress BRGlide: 0x10ac5d18
MenuWidget *g_pSaveSeasonMenu;
// @ghidraAddress BRD3D: 0x10aa2858
int g_dwModalDialogBlocking;
// @ghidraAddress BRD3D: 0x10aa2870
unsigned short g_wMenuHoverIndexPrev;
// @ghidraAddress BRD3D: 0x10aa284c
// @ghidraAddress BRGlide: 0x10ac5ba4
int g_dwMenuHoverActive;
// @ghidraAddress BRD3D: 0x100ab4a8
RECT g_rcMenuRightColumnMid = {330, 0, 447, 0};
// @ghidraAddress BRD3D: 0x100ab4f8
RECT g_rcMenuRightListBox = {440, 128, 540, 204};
// @ghidraAddress BRD3D: 0x100ab498
RECT g_rcMenuLeftColumnNarrow = {88, 0, 185, 0};
// @ghidraAddress BRD3D: 0x100ab438
RECT g_rcMenuFullScreen = {0, 0, 639, 479};
// @ghidraAddress BRD3D: 0x100ab458
RECT g_rcMenuSubPanel = {87, 61, 186, 132};
// @ghidraAddress BRD3D: 0x100ad274
const char g_szMenuDigitOne[] = "1";
// @ghidraAddress BRD3D: 0x100ab418
RECT g_rcHoverTopLeft = {0, 0, 200, 200};
// @ghidraAddress BRD3D: 0x100ab428
RECT g_rcHoverBottomLeft = {0, 380, 200, 480};
// @ghidraAddress BRD3D: 0x10aa29cc
// @ghidraAddress BRGlide: 0x10ac5d24
MenuItemList *g_pSaveSeasonEditMenu;
// @ghidraAddress BRD3D: 0x10aa29f4
// @ghidraAddress BRGlide: 0x10ac5d4c
MenuWidget *g_pSaveSeasonBackHandlerMenu;
// @ghidraAddress BRD3D: 0x10aa28e4
// @ghidraAddress BRGlide: 0x10ac5c3c
int g_dwSaveSeasonSlotNamed;

// The size of the scratch buffer of the front end.
#define FRONT_END_SCRATCH_SIZE 0x400

// The files of the menu images, in the order of g_aMenuImages.
const char *const s_apszMenuImageFiles[MENU_IMAGE_COUNT] = {
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
} // namespace

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

MenuItemList::MenuItemList()
    : pfnSelectionChanged(nullptr), pfnScrollUp(nullptr), pfnScrollDown(nullptr),
      pfnScrollDrag(nullptr), pfnItemChanged(nullptr), dwFlags(0), fX(0), fY(0), itemData{},
      wItemCount(0), nFirstVisibleRow(0), wVisibleRowCount(0), nScrollUpImage(-1),
      nScrollDownImage(-1), nBackgroundImage(-1), nScrollThumbImage(-1), bScrollUpHovered(0),
      bScrollDownHovered(0), abUnknown1a9a4{}, fScrollThumbX(0), fScrollThumbY(0),
      bScrollDragging(0), bHorizontalScrollbar(0), bVerticalScrollbar(0), fMinScrollThumbX(0),
      fMaxScrollThumbX(0), fMinScrollThumbY(0), fMaxScrollThumbY(0), fScrollThumbTravel(0) {
}

MenuItemList::~MenuItemList() {
}

MenuWidget::MenuWidget()
    : pfnUpdate(nullptr), pfnAction(nullptr), pfnIdleAction(nullptr), pfnTextEditComplete(nullptr),
      pfnBeforeUpdate(nullptr), pfnAfterUpdate(nullptr), dwFlags(1), abUnknown20{},
      dwSecondaryFlags(0), dwTertiaryFlags(0), fSavedX(0), fSavedY(0), fSavedZ(0), fX(0), fY(0),
      fZ(0.99f), nWidth(0), nHeight(0), abUnknown4c{}, abUnknown60{}, nAnimationFrame(0),
      abUnknown283c{}, abUnknown2904{}, nImageMode(0), bCustomFrameDurations(0),
      nAnimationLastTime(0), nAnimationElapsed(0), anFrameDurations{}, abUnknown2aa4{},
      nLinkedWidgetCount(0), anLinkedWidgetIndices{}, pScreen(nullptr), abUnknown2aec{1},
      abUnknown2b54{1}, abUnknown2b58{}, bSlideX(0), bSlideY(0), nSlideDirectionX(0),
      nSlideDirectionY(0), fSlideTargetX(0), fSlideTargetY(0), bSliding(0), fSlideStart(0),
      fSlideEnd(0), fSlideStep(0), nSlideLastTime(0), nSlideElapsed(0), abUnknown3830{},
      nSpriteId(0) {
    abUnknown2c[0] = static_cast<char>(0xff);
    memset(abUnknown12a, 0xff, sizeof(abUnknown12a));
    memset(anSpriteIds, 0xff, sizeof(anSpriteIds));
    memset(abUnknown2af0, 0xff, sizeof(abUnknown2af0));
}

MenuWidget::~MenuWidget() {
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
        strcpy(description, g_pActiveMenuScreen->GetFileList(0)->GetName(slot));
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
        g_pActiveMenuScreen->GetFileList(0)->SetName(nSlot, _strupr(description));
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
    g_pRootMenuScreen->GetFileList(season ? 0 : 1)->SetName(slot, pszDescription);
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
    font.SetText(pszText);

    font.SetFlags(font.GetFlags() | (dwTextFlags));
    font.SetStyle(bStyle);
    font.SetInputMaxWidth(0);
    font.SetHeight(0);
    font.SetWidth(0);
    font.SetBoundsLeft(pRect->left);
    font.SetBoundsRight(pRect->right);
    font.SetX(fX);
    font.SetY(fY);
    font.SetZ(0.0f);
    font.SetDrawCaret(0);
    if (bStyle == 3) {
        font.MeasureTextAltFont();
    } else {
        font.MeasureText();
    }
    if (dwTextFlags & 1) {
        font.ComputeCenteredX();
    }
    rcBounds.top = ConvertMenuCoordinate(fY);
    rcBounds.left = pRect->left;
    rcBounds.right = pRect->right;
    rcBounds.bottom = static_cast<int>(static_cast<unsigned>(rcBounds.top) + font.GetHeight());
    nWidth = font.GetWidth();
    nHeight = font.GetHeight();
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
            fonts[0].DrawText();
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

MenuWidget *MenuWidget::LinkedMenuWidget(int index) {
    return pScreen->GetCurrentPanel()->GetWidget(anLinkedWidgetIndices[index]);
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
        if ((flags & 0x400000) &&
            (g_pMouseDevice->IsButtonHeld(0) || g_pMouseDevice->IsButtonHeld(1))) {
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
                MenuWidget *linked = LinkedMenuWidget(i);
                linked->dwFlags |= 0x20000;
                linked->nAnimationFrame = previousFrame;
                linked->nAnimationElapsed = 0;
                linked->nAnimationLastTime = 0;
                rcBounds.right =
                    static_cast<int>(static_cast<unsigned>(rcBounds.right) + linked->nWidth);
                linked->Update();
                LinkedMenuWidget(i)->dwFlags &= ~0x20000u;
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
                fonts[0].SetStyle(1);
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

int MenuItemList::Initialize(
    unsigned flags, const RECT *bounds, int visibleRows, int firstVisibleRow, int backgroundImage) {
    auto add = [](int left, int right) {
        return static_cast<int>(static_cast<unsigned>(left) + static_cast<unsigned>(right));
    };
    auto subtract = [](int left, int right) {
        return static_cast<int>(static_cast<unsigned>(left) - static_cast<unsigned>(right));
    };
    fX = static_cast<float>(bounds->left);
    fY = static_cast<float>(bounds->top);
    rcBounds = *bounds;
    dwFlags = flags;
    wVisibleRowCount = static_cast<unsigned short>(visibleRows);
    nFirstVisibleRow = static_cast<short>(firstVisibleRow);
    nBackgroundImage = static_cast<short>(backgroundImage);
    nScrollUpImage = 0x30;
    nScrollDownImage = 0x2e;
    nScrollThumbImage = 0x3a;
    int width = subtract(g_aMenuSprites[48].rc.right, g_aMenuSprites[48].rc.left);
    int height = subtract(g_aMenuSprites[48].rc.bottom, g_aMenuSprites[48].rc.top);
    if (width < 0) {
        width = 0;
    }
    if (height < 0) {
        height = 0;
    }
    if (bHorizontalScrollbar) {
        rcHorizontalScrollLeft.left = bounds->left;
        rcHorizontalScrollLeft.top = add(bounds->bottom, 3);
        rcHorizontalScrollLeft.right = add(bounds->left, width);
        rcHorizontalScrollLeft.bottom = add(rcHorizontalScrollLeft.top, height);
        rcHorizontalScrollRight.left = subtract(bounds->right, width);
        rcHorizontalScrollRight.top = add(bounds->bottom, 3);
        rcHorizontalScrollRight.right = add(rcHorizontalScrollRight.left, width);
        rcHorizontalScrollRight.bottom = add(rcHorizontalScrollRight.top, height);
        long double position = static_cast<long double>(rcHorizontalScrollLeft.right) + 1;
        fScrollThumbX = static_cast<float>(position);
        fScrollThumbY = static_cast<float>(rcHorizontalScrollLeft.top);
        int maximum = subtract(rcHorizontalScrollRight.left, width);
        fScrollThumbTravel = static_cast<float>(static_cast<long double>(maximum) - position);
        fMinScrollThumbX = static_cast<float>(add(rcHorizontalScrollLeft.right, 1));
        fMaxScrollThumbX = static_cast<float>(maximum);
    } else if (bVerticalScrollbar) {
        rcScrollUp.left = add(bounds->right, 3);
        rcScrollUp.top = bounds->top;
        rcScrollUp.right = add(rcScrollUp.left, g_aMenuSprites[48].rc.right);
        rcScrollUp.bottom = add(rcScrollUp.top, g_aMenuSprites[48].rc.bottom);
        rcScrollDown.left = add(bounds->right, 3);
        rcScrollDown.top = subtract(bounds->bottom, g_aMenuSprites[46].rc.bottom);
        rcScrollDown.right = add(rcScrollDown.left, g_aMenuSprites[46].rc.right);
        rcScrollDown.bottom = bounds->bottom;
        fScrollThumbX = static_cast<float>(rcScrollUp.left);
        long double position = static_cast<long double>(rcScrollUp.bottom) + 1;
        fScrollThumbY = static_cast<float>(position);
        int maximum = subtract(rcScrollDown.top, height);
        fScrollThumbTravel = static_cast<float>(static_cast<long double>(maximum) - position);
        fMinScrollThumbY = static_cast<float>(add(rcScrollUp.bottom, 1));
        fMaxScrollThumbY = static_cast<float>(maximum);
    }
    rcScrollThumb.left = ConvertMenuCoordinate(fScrollThumbX);
    rcScrollThumb.top = ConvertMenuCoordinate(fScrollThumbY);
    rcScrollThumb.right = add(rcScrollThumb.left, width);
    rcScrollThumb.bottom = add(rcScrollThumb.top, height);
    return 1;
}

int MenuItemList::AddItem(
    const char *text, unsigned flags, unsigned char style, const RECT *bounds, bool full) {
    if (text == nullptr) {
        return 0;
    }
    if (wItemCount >= 100) {
        RemoveItem(0);
        wItemCount = 99;
    }
    if (full) {
        fonts[wItemCount].SetText(text);
    } else {
        fonts[wItemCount].CopyTextPrefix(text, 10);
        fonts[wItemCount].AppendText(g_szEmptyString);
    }
    MenuFont &font = fonts[wItemCount];
    font.SetFlags(font.GetFlags() | (flags));
    font.SetStyle(style);
    font.SetInputMaxWidth(0);
    font.SetHeight(0);
    font.SetWidth(0);
    font.SetBoundsLeft(bounds->left);
    font.SetBoundsRight(bounds->right);
    font.SetBoundsTop(
        static_cast<int>(static_cast<unsigned>(ConvertMenuCoordinate(fY)) + 19u * wItemCount));
    font.SetBoundsBottom(static_cast<int>(static_cast<unsigned>(font.GetBoundsTop()) + 18u));
    font.SetX(static_cast<float>(bounds->left));
    font.SetY(static_cast<float>(font.GetBoundsTop()));
    font.SetZ(0);
    font.SetDrawCaret(0);
    if (style == 3) {
        font.MeasureTextAltFont();
    } else {
        font.MeasureText();
    }
    MenuFont &measured = fonts[wItemCount];
    measured.SetInputMaxWidth(static_cast<short>(static_cast<unsigned>(measured.GetBoundsRight()) -
                                                 measured.GetBoundsLeft() - 16u));
    ++wItemCount;
    if (dwFlags & 0x800000) {
        short row = static_cast<short>(wVisibleRowCount + nFirstVisibleRow);
        if (row >= 100) {
            row = static_cast<short>(wItemCount - 1);
        }
        if (!_stricmp(fonts[row].GetText(), g_szEmptyUiString)) {
            return 0;
        }
        ++nFirstVisibleRow;
        if (nFirstVisibleRow >= wItemCount) {
            nFirstVisibleRow = static_cast<short>(wItemCount - 1);
        }
        if (pfnScrollDown) {
            pfnScrollDown();
        }
        unsigned rows = static_cast<unsigned short>(wItemCount - 1);
        if (!rows) {
            rows = 1;
        }
        long double y = static_cast<long double>(fScrollThumbTravel) / rows + fScrollThumbY;
        fScrollThumbY = static_cast<float>(y);
        if (!(y >= fMinScrollThumbY)) {
            fScrollThumbY = fMinScrollThumbY;
        } else if (y > fMaxScrollThumbY) {
            fScrollThumbY = fMaxScrollThumbY;
        }
        rcScrollThumb.top = ConvertMenuCoordinate(fScrollThumbY);
        rcScrollThumb.bottom = static_cast<int>(static_cast<unsigned>(rcScrollThumb.top) + 16u);
    }
    return 1;
}

int MenuItemList::Draw(int selection) {
    if (nBackgroundImage >= 0) {
        BlitSurfaceById(nBackgroundImage, fBackgroundX, fBackgroundY);
    }
    if (!(dwFlags & 0x1000000) && selection >= 0 && !(fonts[selection].GetFlags() & 0x10)) {
        fonts[selection].SetStyle(4);
    }
    int rowOffset = 0;
    for (int row = 0; row < wVisibleRowCount; ++row, rowOffset += 19) {
        int index = row + nFirstVisibleRow;
        if (index < 0 || index >= wItemCount) {
            continue;
        }
        MenuFont *font = &fonts[index];

        font->MeasureText();
        font->SetBoundsLeft(rcBounds.left);
        font->SetBoundsRight(rcBounds.right);
        int y = static_cast<int>(static_cast<unsigned>(rcBounds.top) + rowOffset);
        font->SetBoundsTop(y);
        font->SetBoundsBottom(static_cast<int>(static_cast<unsigned>(y) + 18u));
        font->DrawMenuItemText(static_cast<float>(rcBounds.left), static_cast<float>(y));
        if (dwFlags & 0x2000000) {
            int x = static_cast<int>(static_cast<unsigned>(rcBounds.left) - 19u);
            bool ready = GetSessionSlotFlag(font->GetItemData()) != 0;
            BlitSurfaceById(ready ? 0x8b : 0x8a, static_cast<float>(x), static_cast<float>(y));
        }
    }
    if (!(dwFlags & 0x200000)) {
        if (nScrollUpImage > 0) {
            BlitSurfaceById(bScrollUpHovered ? 0x2f : nScrollUpImage,
                            static_cast<float>(rcScrollUp.left),
                            static_cast<float>(rcScrollUp.top));
        }
        if (nScrollDownImage > 0) {
            BlitSurfaceById(bScrollDownHovered ? 0x2d : nScrollDownImage,
                            static_cast<float>(rcScrollDown.left),
                            static_cast<float>(rcScrollDown.top));
        }
        if (nScrollThumbImage > 0) {
            BlitSurfaceById(nScrollThumbImage, fScrollThumbX, fScrollThumbY);
        }
    }
    return 1;
}

int MenuItemList::RemoveItem(int index) {
    auto clearItem = [this](int item, bool clearDataValue) {
        MenuFont &font = fonts[item];
        font.SetText(g_szEmptyUiString);
        font.SetStyle(0);
        font.SetInputMaxWidth(0);
        font.SetHeight(0);
        font.SetWidth(0);
        font.SetBoundsLeft(0);
        font.SetBoundsTop(0);
        font.SetBoundsRight(0);
        font.SetBoundsBottom(0);
        font.SetX(0);
        font.SetY(0);
        font.SetZ(0);
        font.SetDrawCaret(0);
        if (clearDataValue) {
            font.SetItemData(0);
        }
        itemData[item] = {};
    };
    if (index >= 0) {
        clearItem(index, true);
    }
    int shifted = 0;
    int next = index + 1;
    if (next != wItemCount) {
        for (shifted = next; shifted <= wItemCount - 1; ++shifted) {
            // The original copies the complete font object, including its virtual pointer.
            memcpy(static_cast<void *>(&fonts[shifted - 1]),
                   static_cast<const void *>(&fonts[shifted]),
                   sizeof(MenuFont));
            MenuItemData &data = itemData[shifted];
            if (data.pData && static_cast<int>(data.dwSize) > 0) {
                SetItemData(data.pData, data.dwSize, shifted - 1);
            }
        }
        shifted = 0;
    }
    int last = wItemCount - 1;
    if (last > shifted) {
        clearItem(last, false);
    }
    --wItemCount;
    return 1;
}

int MenuItemList::SetItemData(const void *data, unsigned size, int index) {
    if (index == -1) {
        index = wItemCount - 1;
        if (index < 0) {
            index = 0;
        }
    }
    MenuItemData &item = itemData[index];
    if (item.pData == nullptr) {
        item.pData = static_cast<unsigned char *>(::operator new(size, std::nothrow));
    }
    // Existing allocations are reused without resizing.
    memcpy(item.pData, data, size);
    item.dwSize = size;
    return 1;
}

int MenuItemList::BlitSurfaceById(unsigned short image, float x, float y) {
    const MenuSprite &sprite = g_aMenuSprites[image];
    int destinationY = ConvertMenuCoordinate(y);
    int destinationX = ConvertMenuCoordinate(x);
    BlitSurfaceRegion(
        destinationX, destinationY, static_cast<WORD>(sprite.nImage), &sprite.rc, sprite.dwFlags);
    return 1;
}

int MenuItemList::UpdateAndRenderMenuWidget(int selection) {
    int result = -1;

    Draw(selection);
    if (!g_dwMenuTextEditActive && !g_dwModalDialogBlocking) {
        ProcessInput(&result);
    }
    ++g_wMenuHoverIndexPrev;
    return result;
}

int MenuItemList::UpdateItemAndNotify(int nIndex) {
    MenuFont *font = &fonts[nIndex];

    font->MeasureText();
    if (!font->GetDrawCaret()) {
        return 0;
    }
    signed char result = font->HandleTextInput();
    if (result <= 0) {
        g_dwMenuTextEditActive = 0;
        font->SetDrawCaret(0);
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
    return bounds.left <= pMouse->GetCursorX() && bounds.right >= pMouse->GetCursorX() &&
           bounds.top <= pMouse->GetCursorY() && bounds.bottom >= pMouse->GetCursorY();
}

int MenuItemList::ProcessInput(int *selection) {
    auto truncate = [](long double value) {
        if (!(value >= -0x1p63L && value < 0x1p63L)) {
            return 0;
        }
        return static_cast<int>(static_cast<long long>(value));
    };
    auto clamp = [](long double value, float minimum, float maximum) {
        float result = static_cast<float>(value);
        if (!(value >= minimum)) {
            result = minimum;
        } else if (value > maximum) {
            result = maximum;
        }
        return result;
    };
    auto updateThumbY = [this] {
        rcScrollThumb.top = ConvertMenuCoordinate(fScrollThumbY);
        rcScrollThumb.bottom = static_cast<int>(static_cast<unsigned>(rcScrollThumb.top) + 16u);
    };
    if (dwFlags & 0x18) {
        return 0;
    }
    bool repeated = false;
    if (dwFlags & 0x80000) {
        if (!g_pMouseDevice->IsButtonHeld(0) && !g_pMouseDevice->IsButtonHeld(1)) {
            bScrollDragging = 0;
            dwFlags &= ~0x80002u;
        } else {
            dwFlags |= 0x22;
            unsigned now = GetHighResolutionTime();
            g_dwMenuListRepeatAccumMs += now - g_dwMenuListRepeatLastTick;
            g_dwMenuListRepeatLastTick = now;
            if (static_cast<int>(g_dwMenuListRepeatAccumMs) < 60) {
                return 1;
            }
            g_dwMenuListRepeatAccumMs = 0;
            repeated = true;
        }
    }
    if ((UpdateHoverInRect(&rcScrollThumb) || bScrollDragging) && !(dwFlags & 0x200000)) {
        if (!(dwFlags & 2)) {
            return 1;
        }
        long double ratio =
            static_cast<long double>(wItemCount <= 1 ? 1 : wItemCount - 1) / fScrollThumbTravel;
        if (bHorizontalScrollbar) {
            int delta = static_cast<int>(static_cast<unsigned>(g_pMousePosition->GetCursorX()) -
                                         static_cast<unsigned>(rcScrollThumb.left));
            if (!bScrollDragging) {
                g_fMenuListDragDeltaX = static_cast<float>(delta);
            }
            long double position = static_cast<long double>(delta) +
                                   g_pMousePosition->GetCursorX() - g_fMenuListDragDeltaX;
            fScrollThumbX = clamp(position, fMinScrollThumbX, fMaxScrollThumbX);
            rcScrollThumb.left = ConvertMenuCoordinate(fScrollThumbX);
            rcScrollThumb.right = static_cast<int>(static_cast<unsigned>(rcScrollThumb.left) + 16u);
            if (wItemCount > 1) {
                nFirstVisibleRow = static_cast<short>(
                    truncate((static_cast<long double>(fScrollThumbX) - fMinScrollThumbX) * ratio));
            }
        } else if (bVerticalScrollbar) {
            float cursorY = static_cast<float>(g_pMousePosition->GetCursorY());
            long double movement =
                static_cast<long double>(cursorY) - g_pMousePosition->GetPreviousCursorY();
            if (!bScrollDragging) {
                g_fMenuListDragDeltaY =
                    static_cast<float>(static_cast<long double>(cursorY) - rcScrollThumb.top);
            }
            long double position =
                movement + g_pMousePosition->GetCursorY() - g_fMenuListDragDeltaY;
            fScrollThumbY = clamp(position, fMinScrollThumbY, fMaxScrollThumbY);
            updateThumbY();
            if (wItemCount > 1) {
                nFirstVisibleRow = static_cast<short>(
                    truncate((static_cast<long double>(fScrollThumbY) - fMinScrollThumbY) * ratio));
            }
        }
        if (pfnScrollDrag) {
            pfnScrollDrag();
        }
        bScrollDragging = 1;
        return 1;
    }
    if (UpdateHoverInRect(&rcScrollUp) && !(dwFlags & 0x200000)) {
        if (!(dwFlags & 2)) {
            bScrollUpHovered = 0;
            return 1;
        }
        nFirstVisibleRow = static_cast<short>(nFirstVisibleRow - 1);
        bScrollUpHovered = 1;
        if (nFirstVisibleRow < 0) {
            nFirstVisibleRow = 0;
        }
        if (pfnScrollUp) {
            pfnScrollUp();
        }
        unsigned rows = static_cast<unsigned short>(wItemCount - 1);
        if (!rows) {
            rows = 1;
        }
        fScrollThumbY = clamp(static_cast<long double>(fScrollThumbY) -
                                  static_cast<long double>(fScrollThumbTravel) / rows,
                              fMinScrollThumbY,
                              fMaxScrollThumbY);
        updateThumbY();
        return 1;
    }
    if (UpdateHoverInRect(&rcScrollDown) && !(dwFlags & 0x200000)) {
        if (!(dwFlags & 2)) {
            bScrollDownHovered = 0;
            return 1;
        }
        nFirstVisibleRow = static_cast<short>(nFirstVisibleRow + 1);
        bScrollDownHovered = 1;
        if (nFirstVisibleRow >= wItemCount) {
            nFirstVisibleRow = static_cast<short>(wItemCount - 1);
        }
        if (pfnScrollDown) {
            pfnScrollDown();
        }
        unsigned rows = static_cast<unsigned short>(wItemCount - 1);
        if (!rows) {
            rows = 1;
        }
        fScrollThumbY = clamp(static_cast<long double>(fScrollThumbTravel) / rows + fScrollThumbY,
                              fMinScrollThumbY,
                              fMaxScrollThumbY);
        updateThumbY();
        return 1;
    }
    if (repeated) {
        return 0;
    }
    bool hovered = false;
    int last = nFirstVisibleRow + wVisibleRowCount;
    if (last > 100) {
        last = 100;
    }
    for (int index = nFirstVisibleRow; index < last; ++index) {
        MenuFont &font = fonts[index];
        if (font.GetFlags() & 0x10) {
            font.SetStyle(0);
            continue;
        }
        if (!UpdateHoverInRect(&font.GetBounds()) || !font.GetText()[0]) {
            font.SetStyle(1);
            continue;
        }
        hovered = true;
        if (dwFlags & 0x1000000) {
            continue;
        }
        if (dwFlags & 0x100) {
            int style = static_cast<signed char>(font.GetStyle());
            font.SetStyle(style == 0 || style == 2 ? 1 : style == 1 ? 2 : 0);
            dwFlags &= ~0x100u;
        }
        if (IsInputOrEventActive()) {
            PollInputAndUpdateEdges();
            if (!(font.GetFlags() & 0x10)) {
                *selection = index;
                PlaySoundChannelOneOnce(1, 0x200020);
                g_dwNetworkMenuState = 1;
                if (pfnSelectionChanged) {
                    pfnSelectionChanged(this, selection);
                }
            }
        }
    }
    if (!hovered) {
        dwFlags &= ~0x22u;
        return 0;
    }
    return 1;
}

int MenuItemList::UpdateHoverInRect(const RECT *bounds) {
    if (!MenuBoundsIncludeCursor(*bounds, g_pMousePosition)) {
        return 0;
    }
    if (dwFlags & 0x40000) {
        if (g_pMouseDevice->IsButtonHeld(0) || g_pMouseDevice->IsButtonHeld(1)) {
            dwFlags |= 0x80002;
        } else {
            dwFlags &= ~2u;
        }
    } else if (IsInputOrEventActive()) {
        dwFlags |= 2;
    } else {
        dwFlags &= ~2u;
    }
    dwFlags |= 0x20;
    return 1;
}

bool MenuWidget::UpdateHoverState() {
    if (dwFlags & 8) {
        return false;
    }
    if (dwFlags & 0x10) {
        if (g_wMenuHoverIndex == g_wMenuHoverIndexPrev) {
            g_wMenuHoverIndex =
                static_cast<unsigned short>(g_wMenuHoverIndex + g_dwMouseScrollDirection);
            pScreen->GetCurrentPanel()->RestoreSelection();
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
        if (g_pMouseDevice->IsButtonHeld(0) || g_pMouseDevice->IsButtonHeld(1)) {
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
                itemList.SetFlags(itemList.GetFlags() | (0x100));
                nAnimationFrame =
                    static_cast<short>(static_cast<unsigned short>(nAnimationFrame) + 1);
                if (anFrameDurations[nAnimationFrame] <= 0) {
                    nAnimationFrame = 0;
                }
            }
        } else if (nAnimationElapsed > 60) {
            nAnimationElapsed = 0;
            dwFlags |= 0x100;
            itemList.SetFlags(itemList.GetFlags() | (0x100));
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

bool MenuWidget::UpdateMenuSlideAxis(
    int bEnabled, signed char direction, const float &saved, const float &target, float &current) {
    if (!bEnabled || direction == 0) {
        return true;
    }
    if (direction != 1 && direction != -1) {
        return false;
    }
    long double offset = ComputeSlideOffset(nSlideElapsed);
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
        bool doneX = UpdateMenuSlideAxis(bSlideX, nSlideDirectionX, fSavedX, fSlideTargetX, fX);
        bool doneY = UpdateMenuSlideAxis(bSlideY, nSlideDirectionY, fSavedY, fSlideTargetY, fY);
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
    if (g_pSaveSeasonMenu && g_pSaveSeasonMenu->GetScreen()->GetPanelActive(1) == 1) {
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
        apFiles[0]->SetFormattedName(i, GetStringResource(SAVE_FILE_DEFAULT_NAME_STRING), i);
        apFiles[1]->SetFormattedName(i, GetStringResource(SAVE_FILE_DEFAULT_NAME_STRING), i);
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
    pCursor = g_pRootMenuScreen->GetPanel(0)->GetWidget(MENU_CURSOR_WIDGET_INDEX);
    pCursor->SetX(static_cast<float>(g_pMouseDevice->GetCursorX()));
    pCursor->SetY(static_cast<float>(g_pMouseDevice->GetCursorY()));
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
    pScreen->SetCurrentPanelIndex(0);
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
    DirectDrawRenderer *pRenderer = g_pRenderHost->GetRenderer();
    IDirect3DViewport2 *pViewport = pRenderer->GetViewport();
    D3DMaterial *pMaterial;
    D3DRECT rect;

    if (pRenderer->GetDirect3D2() == nullptr || pRenderer->GetD3DDevice() == nullptr ||
        pViewport == nullptr) {
        return E_FAIL;
    }
    rect.x1 = 0;
    rect.y1 = 0;
    rect.x2 = g_dwConfigFramebufferWidth;
    rect.y2 = g_dwConfigFramebufferHeight;
    pMaterial = pRenderer->GetMaterial();
    pMaterial->SetDiffuseColor(MENU_BACKGROUND_RED, 0.0f, 0.0f);
    pMaterial->SetRampSize(1);
    pMaterial->ApplyDescription();
    pRenderer->GetViewport()->SetBackground(pMaterial->GetMaterialHandle());
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
        hr = g_pRenderHost->GetRenderer()->GetBackBuffer()->BltFast(
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

    g_pMenuDirectDraw = g_pRenderHost->GetRenderer()->GetDirectDraw2();
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
    DirectDrawRenderer *pRenderer = g_pRenderHost->GetRenderer();

    pRenderer->GetPrimarySurface()->Flip(pRenderer->GetBackBuffer(), DDFLIP_WAIT);
    while (pRenderer->GetPrimarySurface()->GetFlipStatus(DDGFS_ISFLIPDONE) != DD_OK) {
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
        pScreen->SetBuildCallback(pfnBuild);
        pScreen->GetBuildCallback()(pScreen);
        g_pActiveMenuScreen->SetBuilt(true);
        g_pActiveMenuScreen->SetRunning(true);
    } else {
        g_pActiveMenuScreen = pScreen;
    }
    return 1;
}

int ShowInputOptionsMenu(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_Credits, BuildInputOptionsUI);
}

int ShowMultiplayerConnectionScreen(MenuWidget *) {
    return ShowCachedMenuScreen(g_pCachedMenu_MultiplayerConnection, BuildMultiplayerConnectionUI);
}

static inline void BuildCreateGameScreen(MenuScreen *screen) {
    BuildCreateGameMenuUI(screen);
}

int ShowCreateGameScreen(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_CreateGame, BuildCreateGameScreen);
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
    if (strlen(pWidget->GetFont(0).GetText())) {
        g_pCreateGameContinueItem->SetFlags(g_pCreateGameContinueItem->GetFlags() & ~0x10u);
    }
    return 1;
}

int OnSessionNameEdited(MenuWidget *pWidget) {
    ProcessMenuTextEditInput(pWidget, 0);
    const char *text = pWidget->GetFont(0).GetText();
    if (std::strlen(text) != 0) {
        g_pCreateGameContinueItem->SetFlags(g_pCreateGameContinueItem->GetFlags() & ~0x10u);
    }
    if (_stricmp(g_szSessionName, text) != 0) {
        std::strcpy(g_szSessionName, text);
    }
    return 1;
}

int PlayerNameEntryUI_EnableContinueIfNameEntered(MenuWidget *pWidget) {
    if (strlen(pWidget->GetFont(0).GetText())) {
        g_pPlayerNameContinueItem->SetFlags(g_pPlayerNameContinueItem->GetFlags() & ~0x10u);
    }
    return 1;
}

int ModemConnectionUI_EnableContinueIfNumberEntered(MenuWidget *pWidget) {
    if (strlen(pWidget->GetFont(0).GetText())) {
        g_pModemContinueItem->SetFlags(g_pModemContinueItem->GetFlags() & ~0x10u);
    }
    return 1;
}

int CarSetupUI_UpdateSteeringIndicatorPos(MenuWidget *pWidget) {
    int position = static_cast<int>(static_cast<unsigned>(g_dwHandlingTypeMenuIndex) * 11u + 61u);
    pWidget->SetX(static_cast<float>(position));
    return 1;
}

int AudioOptionsUI_UpdateVolumeChannelImage(MenuWidget *pWidget) {
    short image = g_bAudioOptionsSfxSelected ? 0x68 : 0x69;
    pWidget->SetAnimationSpriteId(0, image);
    pWidget->SetSpriteId(image);
    return 1;
}

int AudioOptionsUI_UpdateVolumeLevelIndicatorPos(MenuWidget *pWidget) {
    unsigned volume = g_bAudioOptionsSfxSelected ? g_GlobalRallyConfig.dwSfxVolume :
                                                   g_GlobalRallyConfig.dwMusicVolume;
    int position = static_cast<int>(volume * 8u + 74u);
    pWidget->SetX(static_cast<float>(position));
    return 1;
}

static inline int DrawAudioVolumeSlider(MenuWidget *widget, const int &volume) {
    unsigned x = static_cast<unsigned>(ConvertMenuCoordinate(widget->GetX()));
    int y = static_cast<int>(static_cast<unsigned>(ConvertMenuCoordinate(widget->GetY())) + 19u);
    widget->BlitSurfaceById(0x74, static_cast<int>(x), y);
    for (unsigned index = 0; index < static_cast<unsigned>(volume); index++, x += 12u) {
        widget->BlitSurfaceById(0x75, static_cast<int>(x), y);
    }
    return 1;
}

int DrawGameplaySliderA(MenuWidget *pWidget) {
    return DrawAudioVolumeSlider(pWidget, g_GlobalRallyConfig.dwSfxVolume);
}

int DrawGameplaySliderB(MenuWidget *pWidget) {
    return DrawAudioVolumeSlider(pWidget, g_GlobalRallyConfig.dwMusicVolume);
}

static inline void UpdateMenuListSelection(MenuWidget *pWidget, int &selection) {
    MenuItemList *list = &pWidget->GetItemList();
    int result = list->UpdateAndRenderMenuWidget(selection);
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

int NetworkCarSetupUI_UpdateSelectedPlayer(MenuWidget *pWidget) {
    UpdateMenuListSelection(pWidget, g_dwSelectedSessionIndex);
    g_dwNetCarSetupSelectedPlayerId =
        pWidget->GetItemList().GetFont(g_dwSelectedSessionIndex).GetItemData();
    return 1;
}

int KeyConfigUI_UpdateActionSelection(MenuWidget *pWidget) {
    UpdateMenuListSelection(pWidget, g_dwKeyConfigSelectedAction);
    return 1;
}

int SaveSeasonUI_UpdateSlotSelection(MenuWidget *pWidget) {
    MenuItemList *list = &pWidget->GetItemList();

    int selection = list->UpdateAndRenderMenuWidget(g_dwFileListSelectedSlot);
    if (selection >= 0) {
        g_dwFileListSelectedSlot = selection;
    } else {
        selection = g_dwFileListSelectedSlot;
    }
    if (g_dwMenuTextEditActive && selection >= 0) {
        list->UpdateItemAndNotify(selection);
    }
    return 1;
}

int SaveTimeAttackUI_UpdateSlotSelection(MenuWidget *pWidget) {
    return SaveSeasonUI_UpdateSlotSelection(pWidget);
}

void RefreshSessionListSlots(void) {
    if (g_pNetworkPlayerListMenu != nullptr) {
        unsigned count = g_pNetworkPlayerListMenu->GetItemList().GetItemCount();
        for (unsigned i = 0; i < count; i++) {
            g_pNetworkPlayerListMenu->GetItemList().RemoveItem(i);
        }
    }
}

int GetSelectedSessionGuid(GUID *guid) {
    if (g_pJoinCreateGameMenu != nullptr) {
        const GUID *data = static_cast<const GUID *>(
            g_pJoinCreateGameMenu->GetItemList().GetItemData(g_dwSelectedSessionIndex));
        if (data != nullptr) {
            *guid = *data;
        }
    }
    return 0;
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
    MenuItemList *list = &pWidget->GetItemList();
    int track = g_dwSelectedCdTrack;
    if (track < 0 || track >= 12) {
        track = -1;
    }
    int result = list->UpdateAndRenderMenuWidget(track);
    if (result >= 0) {
        g_dwSelectedCdTrack = result;
    }
    return 1;
}

int NetworkCarSetupUI_PollChatListSelection(MenuWidget *pWidget) {
    MenuItemList *list = &pWidget->GetItemList();
    list->UpdateAndRenderMenuWidget(g_dwSelectedSessionIndex);
    return 1;
}

static inline int SynchronizeEditedMenuText(MenuWidget *pWidget, char *destination) {
    ProcessMenuTextEditInput(pWidget, 0);
    const char *text = pWidget->GetFont(0).GetText();
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
    strcpy(g_szNetworkChatText, pWidget->GetFont(0).GetText());
    SendNetworkChatMessage(g_hMainWindow, info, false);
    strcpy(g_szNetworkChatText, g_szEmptyUiString);
    pWidget->GetFont(0).SetText(g_szEmptyUiString);
    return 1;
}

int OnTcpIpAddressEdited(MenuWidget *pWidget) {
    return SynchronizeEditedMenuText(pWidget, g_GlobalRallyConfig.szTcpIpAddress);
}

int OnTcpIpPortEdited(MenuWidget *pWidget) {
    return SynchronizeEditedMenuText(pWidget, g_GlobalRallyConfig.szTcpIpPort);
}

int OnPlayerNameEdited(MenuWidget *pWidget) {
    ProcessMenuTextEditInput(pWidget, 0);
    const char *text = pWidget->GetFont(0).GetText();
    if (strlen(text)) {
        g_pPlayerNameContinueItem->SetFlags(g_pPlayerNameContinueItem->GetFlags() & ~0x10u);
    }
    if (_stricmp(g_GlobalRallyConfig.szPlayerName, text)) {
        strcpy(g_GlobalRallyConfig.szPlayerName, text);
    }
    return 1;
}

int OnModemPhoneNumberEdited(MenuWidget *pWidget) {
    ProcessMenuTextEditInput(pWidget, 0);
    const char *text = pWidget->GetFont(0).GetText();
    if (strlen(text)) {
        g_pModemContinueItem->SetFlags(g_pModemContinueItem->GetFlags() & ~0x10u);
    }
    if (_stricmp(g_szModemPhoneNumber, text)) {
        strcpy(g_szModemPhoneNumber, text);
        strcpy(g_GlobalRallyConfig.szSavedModemPhoneNumber, g_szModemPhoneNumber);
    }
    return 1;
}

int ShowVideoOptionsMenu(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_InputConfig, BuildVideoOptionsUI);
}

static inline void BuildAudioOptionsScreen(MenuScreen *screen) {
    BuildAudioOptionsUI(screen);
}

static inline void BuildGameOptionsScreen(MenuScreen *screen) {
    BuildGameOptionsUI(screen);
}

int ShowAudioOptionsMenu(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_GameplaySettings, BuildAudioOptionsScreen);
}

int ShowGameOptionsMenu(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_Championship, BuildGameOptionsScreen);
}

int ShowKeyConfigScreen(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_KeyConfig, BuildKeyConfigUI);
}

static inline void BuildModemConnectionScreen(MenuScreen *screen) {
    BuildModemConnectionUI(screen);
}

int ShowModemDialUpScreen(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_ModemDialUp, BuildModemConnectionScreen);
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
int CompareDisplayModes(const void *left, const void *right) {
    DisplayMode *leftMode = *static_cast<DisplayMode *const *>(left);
    DisplayMode *rightMode = *static_cast<DisplayMode *const *>(right);
    if (!leftMode && !rightMode) {
        return 0;
    }
    if (leftMode && !rightMode) {
        return -1;
    }
    int leftWidth, leftHeight, leftDepth, leftRefresh;
    int rightWidth, rightHeight, rightDepth, rightRefresh;
    // A null left mode still enters the dimension query, as in the original comparator.
    leftMode->GetSurfaceDescDimensions(&leftWidth, &leftHeight, &leftDepth, &leftRefresh);
    rightMode->GetSurfaceDescDimensions(&rightWidth, &rightHeight, &rightDepth, &rightRefresh);
    const unsigned leftValues[] = {static_cast<unsigned>(leftDepth),
                                   static_cast<unsigned>(leftWidth),
                                   static_cast<unsigned>(leftHeight)};
    const unsigned rightValues[] = {static_cast<unsigned>(rightDepth),
                                    static_cast<unsigned>(rightWidth),
                                    static_cast<unsigned>(rightHeight)};
    for (int index = 0; index < 3; index++) {
        if (leftValues[index] < rightValues[index]) {
            return -1;
        }
        if (leftValues[index] > rightValues[index]) {
            return 1;
        }
    }
    return 0;
}

int PopulateDisplayModeList(void) {
    if (!g_pSelectedDisplayDriver) {
        return 0;
    }
    if (!g_pSelectedRenderDevice) {
        g_pSelectedRenderDevice =
            ResolveRenderDeviceOrFind(g_pSelectedDisplayDriver, nullptr, nullptr);
        if (!g_pSelectedRenderDevice) {
            return 0;
        }
    }
    DisplayMode *mode = g_pSelectedDisplayDriver->GetDisplayModesHead();
    if (!mode) {
        return 0;
    }
    unsigned count = g_pSelectedDisplayDriver->GetDisplayModeCount();
    if (!count) {
        return 0;
    }
    if (!g_pSelectedDisplayMode) {
        g_pSelectedDisplayMode = FindDisplayModeWithFallback(
            g_pSelectedDisplayDriver, 640, 480, 16, 0, g_pSelectedRenderDevice);
    }
    if (g_apSortedDisplayModes) {
        ::operator delete(g_apSortedDisplayModes);
        g_apSortedDisplayModes = nullptr;
    }
    g_apSortedDisplayModes = static_cast<DisplayMode **>(::operator new(count * 4u, std::nothrow));
    if (!g_apSortedDisplayModes) {
        return 0;
    }
    unsigned index = 0;
    do {
        DisplayMode *next = mode->GetNext();
        g_apSortedDisplayModes[index++] = mode;
        mode = next;
    } while (mode);
    std::qsort(g_apSortedDisplayModes, count, sizeof(DisplayMode *), CompareDisplayModes);
    int visibleIndex = 0;
    bool selected = false;
    for (unsigned sortedIndex = 0; sortedIndex < count; sortedIndex++) {
        mode = g_apSortedDisplayModes[sortedIndex];
        if (!mode || !mode->IsDisplayModeSupportedByDevice(g_pSelectedRenderDevice)) {
            continue;
        }
        int width, height, depth, refresh;
        g_apSortedDisplayModes[sortedIndex]->GetSurfaceDescDimensions(
            &width, &height, &depth, &refresh);
        char text[80];
        if (refresh) {
            wsprintfA(text, "%4d x %4d x %2d (%4d Hz)", width, height, depth, refresh);
        } else {
            wsprintfA(text, "%4d x %4d x %2d", width, height, depth);
        }
        if (g_GlobalRallyConfig.dwDisplayModeWidth == width &&
            g_GlobalRallyConfig.dwDisplayModeHeight == height &&
            g_GlobalRallyConfig.dwDisplayModeBitDepth == depth &&
            (!refresh || g_GlobalRallyConfig.dwDisplayModeExtra == refresh)) {
            g_dwActiveDisplayModeIndex = visibleIndex;
            g_dwSelectedDisplayModeIndex = visibleIndex;
            selected = true;
        }
        if (!selected && width == 640 && height == 480 && depth == 16) {
            g_dwActiveDisplayModeIndex = visibleIndex;
            g_dwSelectedDisplayModeIndex = visibleIndex;
        }
        if (g_pDisplayModeSelectMenu) {
            g_pDisplayModeSelectMenu->GetItemList().AddItem(
                text, 0, 1, &g_rcMenuDeviceListBox, true);
            g_pDisplayModeSelectMenu->GetItemList().SetItemData(
                &sortedIndex, sizeof(sortedIndex), visibleIndex);
        }
        visibleIndex++;
    }
    return 1;
}

int PopulateDisplayDeviceList(void) {
    GUID *guid =
        g_pSelectedDisplayDriver ? g_pSelectedDisplayDriver->GetDirectDrawDriverGuid() : nullptr;
    g_pSelectedDisplayDriver = FindDirectDrawDriverOrDefault(guid);
    if (!g_pSelectedDisplayDriver) {
        return 0;
    }
    int index = 0;
    for (DirectDrawDriver *driver = g_pDirectDrawDriverListHead; driver; index++) {
        DirectDrawDriver *next = driver->GetNext();
        char text[80];
        if (driver->GetFlags() & DIRECTDRAW_DRIVER_PRIMARY) {
            wsprintfA(text, "%s (Primary)", driver->GetName());
            g_dwActiveDisplayDeviceIndex = index;
            g_dwSelectedDisplayDeviceIndex = index;
        } else {
            wsprintfA(text, "%s", driver->GetName());
        }
        if (g_pD3DDeviceSelectMenu) {
            g_pD3DDeviceSelectMenu->GetItemList().AddItem(text, 0, 1, &g_rcMenuDeviceListBox, true);
            g_pD3DDeviceSelectMenu->GetItemList().SetItemData(
                driver->GetGuid(), sizeof(GUID), index);
        }
        driver = next;
    }
    g_pSelectedDisplayDriver = FindDisplayDeviceByGuid(&g_GlobalRallyConfig.DisplayDriverGuid);
    return 1;
}

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
    g_fGhostBestLapTime = VEHICLE_GET(&(g_aVehicles[0]), BestLapTime);
    g_fGhostBestRaceTime = VEHICLE_GET(&(g_aVehicles[0]), TotalTime);
    return ShowCachedMenuScreen(g_pCachedMenu_SaveGhost, BuildSaveTimeAttackMenuUI);
}

static inline void PostSelectionNotice(void) {
    SendNetworkChatMessage(g_hMainWindow, g_pDirectPlayInfo, true);
    strcpy(g_szNetworkChatText, "");
}

template <std::predicate<int> Available>
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

int MultiplayerGameSetupUI_OnCycleCarClass(MenuWidget *) {
    if (g_adwMouseButtonPressed[1]) {
        g_dwSelectedCarClass = static_cast<int>(static_cast<unsigned>(g_dwSelectedCarClass) + 1);
        if (g_dwSelectedCarClass >= 5) {
            g_dwSelectedCarClass = 0;
        }
        ApplyCarGroupPreset();
    } else if (g_adwMouseButtonPressed[0]) {
        g_dwSelectedCarClass = static_cast<int>(static_cast<unsigned>(g_dwSelectedCarClass) - 1);
        if (g_dwSelectedCarClass < 0) {
            g_dwSelectedCarClass = 4;
        }
        ApplyCarGroupPreset();
    }
    if (g_DirectPlayInfo.pDirectPlay) {
        const char *label = GetStringResource(g_adwCarClassLabelStrIds[g_dwSelectedCarClass]);
        sprintf(g_szNetworkChatText, GetStringResource(0xbd), label);
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

static inline void BuildTimeAttackMenuScreen(MenuScreen *screen) {
    BuildTimeAttackMenuUI(screen);
}

int ShowTimeAttackMenu(int) {
    SetActiveMenuItemText(" ");
    return ShowCachedMenuScreen(g_pCachedMenu_TimeAttack, BuildTimeAttackMenuScreen);
}

int ShowLoadGhostMenu(void) {
    g_fGhostBestRaceTime = 0.0f;
    g_fGhostBestLapTime = 0.0f;
    return ShowCachedMenuScreen(g_pCachedMenu_LoadGhost, BuildLoadTimeAttackMenuUI);
}

int JoinSessionAndShowRaceSetup(void) {
    g_dwLobbyIsHost = 0;
    g_dwSessionEnumPending = 0;
    bool ready = g_dwMenuScreenActivated != 0;
    if (!ready) {
        if (g_dwServiceProviderIndex == 2 || g_dwServiceProviderIndex == 3) {
            g_dwSessionEnumPending = 1;
            if (g_dwServiceProviderIndex == 2 && strlen(g_szModemPhoneNumber) < 7) {
                return 1;
            }
            if (g_pNetworkSessionJoinMenu &&
                g_pJoinCreateGameMenu->GetItemList().GetItemCount() > 0) {
                ready = JoinNetworkSession() != 0;
            } else {
                InitServiceProviderAndStartSession();
            }
        } else {
            ready = JoinNetworkSession() != 0;
        }
    }
    if (ready) {
        if (!ShowCachedMenuScreen(g_pCachedMenu_RaceSetupNetwork, BuildMultiplayerGameSetupUI)) {
            return 0;
        }
        g_pMultiplayerGameSetupMenu->SetActionCallback(MultiplayerGameSetupUI_OnBackAsClient);
    }
    return 1;
}

int ShowHostRaceSetupScreen(void) {
    g_dwLobbyIsHost = 1;
    g_dwNetworkPlayMode = 2;
    g_dwSessionEnumPending = 0;
    if (!ShowCachedMenuScreen(g_pCachedMenu_RaceSetupNetwork, BuildMultiplayerGameSetupUI)) {
        return 0;
    }
    g_pMultiplayerGameSetupMenu->SetActionCallback(MultiplayerGameSetupUI_OnBackAsHost);
    return 1;
}

int ShowNetworkCarSetupScreen(MenuWidget *) {
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

static inline void FreeMenuSessionDesc(DPSESSIONDESC2 *desc) {
    GlobalUnlock(GlobalHandle(desc));
    GlobalFree(GlobalHandle(desc));
}

int NetworkCarSetupUI_OnRefreshSessionDesc(MenuWidget *) {
    DPSESSIONDESC2 *desc = nullptr;
    GetDirectPlaySessionDesc(g_DirectPlayInfo.pDirectPlay, &desc);
    if (desc == nullptr) {
        return 1;
    }
    if (desc->dwCurrentPlayers <= 1) {
        strcpy(g_szNetworkChatText, GetStringResource(0xbb));
        SendNetworkChatMessage(g_hMainWindow, g_pDirectPlayInfo, true);
        strcpy(g_szNetworkChatText, g_szEmptyUiString);
        FreeMenuSessionDesc(desc);
        return 1;
    }
    if (g_dwLobbyIsHost) {
        bool ready = true;
        for (const SessionSlot &slot : g_aSessionSlots) {
            if (!slot.nFlag && slot.dpidPlayer != 0xffffffff) {
                ready = false;
                break;
            }
        }
        if (ready) {
            SendNetworkMessage3(g_pDirectPlayInfo);
            g_dwNetworkSendBlocked = 1;
            desc->dwFlags |= 0x20;
            g_DirectPlayInfo.pDirectPlay->SetSessionDesc(desc, 0);
        } else {
            strcpy(g_szNetworkChatText, GetStringResource(0xbc));
            SendNetworkChatMessage(g_hMainWindow, g_pDirectPlayInfo, true);
            strcpy(g_szNetworkChatText, g_szEmptyUiString);
        }
    } else {
        int ready = ToggleSelectedSessionSlotFlag();
        SendNetworkReadyMessage(g_pDirectPlayInfo, ready);
    }
    FreeMenuSessionDesc(desc);
    return 1;
}

void EnterNetworkLobbyScreen(void) {
    g_dwServiceProviderIndex = 1;
    ResetSessionSlotTable();
    if (g_dwLobbyIsHost) {
        DPSESSIONDESC2 *desc = nullptr;
        if (g_DirectPlayInfo.pDirectPlay) {
            GetDirectPlaySessionDesc(g_DirectPlayInfo.pDirectPlay, &desc);
        }
        if (desc) {
            desc->dwFlags &= ~0x20u;
            g_DirectPlayInfo.pDirectPlay->SetSessionDesc(desc, 0);
        }
    }
    ShowMultiplayerNameScreen();
    ShowMultiplayerConnectionScreen();
    ShowJoinCreateGameScreen();
    if (g_dwLobbyIsHost) {
        ShowCreateGameScreen();
        ShowHostRaceSetupScreen();
    } else {
        LoadNetworkSettings();
        JoinSessionAndShowRaceSetup();
    }
    if (g_dwLobbyIsHost) {
        if (!g_dwNetworkCarSetupShown) {
            HostNetworkSession();
            g_dwNetworkCarSetupShown = 1;
            return;
        }
        UpdateNetworkSessionSettings();
    }
}

int ReturnToNetworkLobbyScreen(void) {
    g_dwDirectPlaySessionActive = 1;
    ResetSessionSlotTable();
    if (g_dwLobbyIsHost) {
        DPSESSIONDESC2 *desc = nullptr;
        if (g_DirectPlayInfo.pDirectPlay) {
            GetDirectPlaySessionDesc(g_DirectPlayInfo.pDirectPlay, &desc);
        }
        if (desc) {
            desc->dwFlags &= ~0x20u;
            g_DirectPlayInfo.pDirectPlay->SetSessionDesc(desc, 0);
        }
    }
    ShowMultiplayerNameScreen();
    ShowMultiplayerConnectionScreen();
    ShowJoinCreateGameScreen();
    if (g_dwLobbyIsHost) {
        ShowCreateGameScreen();
        ShowHostRaceSetupScreen();
    } else {
        JoinSessionAndShowRaceSetup();
    }
    if (!ShowCachedMenuScreen(g_pCachedMenu_NetworkCarSetup, BuildNetworkCarSetupUI)) {
        return 0;
    }
    g_dwGameMode = GAME_MODE_NETWORK;
    if (g_dwLobbyIsHost) {
        if (!g_dwNetworkCarSetupShown) {
            HostNetworkSession();
            g_dwNetworkCarSetupShown = 1;
        } else {
            UpdateNetworkSessionSettings();
        }
    }
    if (g_pDirectPlayInfo && g_pDirectPlayInfo->dpidPlayer) {
        SendNetworkMessage7(g_pDirectPlayInfo, g_pDirectPlayInfo->dpidPlayer);
    }
    return 1;
}

int NetworkCarSetupUI_OnEscape(MenuWidget *pWidget) {
    if (g_dwNetworkDataStreamActive) {
        if (!g_dwLaunchedFromLobby) {
            ReturnToMultiplayerLobby(pWidget);
            if (g_pCachedMenu_RaceSetupNetwork != nullptr) {
                g_pCachedMenu_RaceSetupNetwork->DestroyPanels();
                g_pCachedMenu_RaceSetupNetwork = nullptr;
            }
            g_pActiveMenuScreen = g_pCachedMenu_JoinCreateGame;
            DisconnectNetworkSession();
            g_dwSessionEnumPending = 1;
            if (g_dwServiceProviderIndex == 0 || g_dwServiceProviderIndex == 1) {
                InitializeNetworkServiceProvider();
            }
            if ((g_dwServiceProviderIndex == 2 || g_dwServiceProviderIndex == 3) &&
                g_pNetworkSessionJoinMenu != nullptr) {
                g_pNetworkSessionJoinMenu->SetFlags(g_pNetworkSessionJoinMenu->GetFlags() & ~0x10u);
            }
            g_dwNetworkDataStreamActive = 0;
            return 0;
        }
        pWidget->GetScreen()->SetRunning(0);
        pWidget->GetScreen()->ShutdownFrontEnd(0);
        ShutdownApplication(0);
    }
    if (g_dwNetworkEscapePending) {
        ReturnToMultiplayerLobby(pWidget);
        if (g_pCachedMenu_RaceSetupNetwork != nullptr) {
            g_pCachedMenu_RaceSetupNetwork->DestroyPanels();
            g_pCachedMenu_RaceSetupNetwork = nullptr;
        }
        ShowCreateGameScreen();
        ShowHostRaceSetupScreen();
        ShowNetworkCarSetupScreen();
        g_dwNetworkEscapePending = 0;
        return 0;
    }
    if (g_dwLobbyIsHost) {
        DPSESSIONDESC2 *desc = nullptr;
        if (g_DirectPlayInfo.pDirectPlay != nullptr) {
            GetDirectPlaySessionDesc(g_DirectPlayInfo.pDirectPlay, &desc);
        }
        if (desc != nullptr) {
            DPID localPlayer = g_pDirectPlayInfo->dpidPlayer;
            for (SessionSlot &slot : g_aSessionSlots) {
                if (slot.dpidPlayer == localPlayer) {
                    slot.nFlag = desc->dwCurrentPlayers > 1;
                    break;
                }
            }
            FreeMenuSessionDesc(desc);
        }
    }
    if (g_dwNetworkSendBlocked) {
        BackupMenuSelections();
        g_GlobalRallyConfig.SaveConfigToFile(g_szConfigFilePath);
        pWidget->GetScreen()->SetRunning(0);
        pWidget->GetScreen()->ShutdownFrontEnd(0);
        g_dwInNetworkCarSetup = 0;
        PlaySoundChannelOneOnce(2, 0x200020);
        g_dwNetworkMenuState = 2;
        return 0;
    }
    return 1;
}

static inline void BuildPlayerNameEntryScreen(MenuScreen *screen) {
    BuildPlayerNameEntryUI(screen);
}

int ShowMultiplayerNameScreen(void) {
    SetActiveMenuItemText(" ");
    ResolveRaceSelections();
    return ShowCachedMenuScreen(g_pCachedMenu_MultiplayerName, BuildPlayerNameEntryScreen);
}

static inline void BuildJoinCreateGameScreen(MenuScreen *screen) {
    BuildJoinCreateGameUI(screen);
}

int ShowJoinCreateGameScreen(void) {
    if (!ShowCachedMenuScreen(g_pCachedMenu_JoinCreateGame, BuildJoinCreateGameScreen)) {
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
    g_aVehicles[0].SetProfile(nullptr);
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
        g_pRaceSetupMenu->SetActionCallback(RaceSetupUI_OnBackToTimeAttack);
    }
    return 1;
}

static inline void BuildQuitGameConfirmScreen(MenuScreen *screen) {
    BuildQuitGameConfirmUI(screen);
}

int ShowQuitGameConfirmMenu(void) {
    SetActiveMenuItemText(" ");
    PlaySoundChannelOneOnce(3, 0x200020);
    g_dwNetworkMenuState = 3;
    return ShowCachedMenuScreen(g_pCachedMenu_MainMenuAlternate, BuildQuitGameConfirmScreen);
}

static inline void BuildOptionsMenuScreen(MenuScreen *screen) {
    BuildOptionsMenuUI(screen);
}

int ShowOptionsMenu(void) {
    SetActiveMenuItemText(" ");
    return ShowCachedMenuScreen(g_pCachedMenu_Options, BuildOptionsMenuScreen);
}

static inline int ReturnFromOptionsMenu(MenuActionWidget *pItem, MenuScreen *&pTarget) {
    pItem->GetScreen()->DestroyPanels();
    delete g_pActiveMenuScreen;
    g_pCachedMenu_Options = nullptr;
    g_pActiveMenuScreen = pTarget;
    return 0;
}

static inline int ReturnFromSaveSeasonMenu(MenuActionWidget *pItem, MenuScreen *&pTarget) {
    pItem->GetScreen()->DestroyPanels();
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
    pItem->GetScreen()->DestroyPanels();
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
    pItem->GetScreen()->DestroyPanels();
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
    pItem->GetScreen()->DestroyPanels();
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
    pItem->GetScreen()->DestroyPanels();
    delete g_pActiveMenuScreen;
    pDestination = g_pCachedMenu_SeasonProgressInfo;
    g_pCachedMenu_CarSetup = nullptr;
    g_pCarSetupMenu = nullptr;
    g_pActiveMenuScreen = pDestination;
    return 0;
}

int CarSetupUI_OnBackToTimeAttack(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    pItem->GetScreen()->DestroyPanels();
    delete g_pActiveMenuScreen;
    pDestination = g_pCachedMenu_TimeAttack;
    g_pCachedMenu_CarSetup = nullptr;
    g_pCarSetupMenu = nullptr;
    g_pActiveMenuScreen = pDestination;
    return 0;
}

int PlayerNameEntryUI_OnBack(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    pItem->GetScreen()->DestroyPanels();
    delete g_pActiveMenuScreen;
    pDestination = g_pRootMenuScreen;
    g_pCachedMenu_MultiplayerName = nullptr;
    g_pPlayerNameContinueItem = nullptr;
    g_pActiveMenuScreen = pDestination;
    return 0;
}

int RaceSetupUI_OnBack(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    pItem->GetScreen()->DestroyPanels();
    delete g_pActiveMenuScreen;
    pDestination = g_pRootMenuScreen;
    g_pCachedMenu_MainMenu = nullptr;
    g_pRaceSetupMenu = nullptr;
    g_pActiveMenuScreen = pDestination;
    return 0;
}

int RaceSetupUI_OnBackToTimeAttack(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    pItem->GetScreen()->DestroyPanels();
    delete g_pActiveMenuScreen;
    pDestination = g_pCachedMenu_TimeAttack;
    g_pCachedMenu_MainMenu = nullptr;
    g_pRaceSetupMenu = nullptr;
    g_pActiveMenuScreen = pDestination;
    return 0;
}

int ModemConnectionUI_OnBack(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    pItem->GetScreen()->DestroyPanels();
    delete g_pActiveMenuScreen;
    pDestination = g_pCachedMenu_MultiplayerConnection;
    g_pCachedMenu_ModemDialUp = nullptr;
    g_pModemContinueItem = nullptr;
    g_pActiveMenuScreen = pDestination;
    return 0;
}

int LoadSeasonUI_OnBack(MenuActionWidget *pItem) {
    pItem->GetScreen()->DestroyPanels();
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
    pItem->GetScreen()->DestroyPanels();
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
    pWidget->SetSpriteId(g_abTransmissionImageIds[g_dwSelectedTransmission]);
    return 1;
}

int CarSetupUI_UpdateCarImage(MenuWidget *pWidget) {
    if (g_pActiveMenuScreen == g_pCachedMenu_LoadGhost && !g_dwTimeAttackLoadSelected) {
        return -2;
    }
    pWidget->SetSpriteId(g_adwCarImageIds[g_dwCarMenuIndex]);
    return 1;
}

int CarSetupUI_UpdateTiresImage(MenuWidget *pWidget) {
    pWidget->SetSpriteId(g_abTireImageIds[g_dwTireMenuIndex]);
    return 1;
}

int CarSetupUI_UpdateShocksImage(MenuWidget *pWidget) {
    pWidget->SetSpriteId(g_abSuspensionImageIds[g_dwSuspensionMenuIndex]);
    return 1;
}

int InputOptionsUI_UpdateInputDeviceImage(MenuWidget *pWidget) {
    pWidget->SetSpriteId(g_abInputDeviceImageIds[g_dwSelectedInputDevice]);
    return 1;
}

int ConnectionUI_UpdateServiceProviderImage(MenuWidget *pWidget) {
    pWidget->SetSpriteId(g_abServiceProviderImageIds[g_dwServiceProviderIndex]);
    return 1;
}

int GameOptionsUI_UpdateCarShadowImage(MenuWidget *pWidget) {
    pWidget->SetSpriteId(g_abCarShadowImageIds[g_dwOptionCarShadow]);
    return 1;
}

int GameOptionsUI_UpdateSpecularLightingImage(MenuWidget *pWidget) {
    pWidget->SetSpriteId(g_abSpecularLightingImageIds[g_dwOptionSpecularLighting]);
    return 1;
}

int GameOptionsUI_UpdateSkidMarksImage(MenuWidget *pWidget) {
    pWidget->SetSpriteId(static_cast<short>(g_awSkidMarksImageIds[g_dwOptionSkidMarks]));
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
        pWidget->SetFlags(pWidget->GetFlags() & ~0x1010u);
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
    pWidget->GetScreen()->ShutdownFrontEnd(1);
    g_pActiveMenuScreen = g_pCachedMenu_MainMenu;
    return 0;
}

int PreRaceInfoUI_OnOptions(MenuWidget *) {
    ShowOptionsMenu();
    g_pOptionsSubMenu->SetActionCallback(OptionsMenuUI_OnBackToPreRaceInfo);
    return 1;
}

int PreRaceInfoUI_OnSaveSeason(MenuWidget *) {
    ShowSaveSeasonScreen();
    g_pSaveSeasonBackHandlerMenu->SetActionCallback(SaveSeasonUI_OnBackToPreRaceInfo);
    return 1;
}

int SeasonProgressInfoUI_OnOptions(MenuWidget *) {
    ShowOptionsMenu();
    g_pOptionsSubMenu->SetActionCallback(OptionsMenuUI_OnBackToSeasonProgressInfo);
    return 1;
}

int SeasonProgressInfoUI_OnSaveSeason(MenuWidget *) {
    ShowSaveSeasonScreen();
    g_pSaveSeasonBackHandlerMenu->SetActionCallback(SaveSeasonUI_OnBackToSeasonProgressInfo);
    return 1;
}

void SetActiveMenuItemText(const char *pszText) {
    MenuWidget *pWidget = g_pActiveMenuScreen->GetPanel(0)->GetWidget(g_dwRootMenuStringIndex);
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
    pWidget->GetScreen()->SetPanelActive(1, 0);
    MenuFont &font = list->GetFont(g_dwFileListSelectedSlot);
    font.SetDrawCaret(!font.GetDrawCaret());
    MenuFont &selected = list->GetFont(g_dwFileListSelectedSlot);
    g_dwMenuTextEditActive = selected.GetDrawCaret();
    if (g_dwMenuTextEditActive) {
        strcpy(g_szMenuItemTextBackup, selected.GetText());
        selected.SetText(g_szEmptyUiString);
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
    pWidget->GetScreen()->SetPanelActive(1, 0);
    named = 0;
    if (g_dwMenuTextEditActive) {
        list->GetFont(g_dwFileListSelectedSlot).SetText(g_szMenuItemTextBackup);
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
        pWidget->GetScreen()->SetRunning(0);
        pWidget->GetScreen()->ShutdownFrontEnd(0);
        ShutdownApplication(0);
    }
    return 1;
}

int ChampionshipSeasonUI_OnStartNewSeason(MenuWidget *) {
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    ResetMenuSelectionsToDefaults();
    g_aVehicles[0].SetProfile(nullptr);
    ShowPreRaceInfoScreen();
    g_pExitSeasonConfirmMenu->SetActionCallback(PreRaceInfoUI_OnBackToChampionshipSeason);
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    ResolveRaceSelections();
    return 1;
}

int ReturnToMultiplayerLobby(MenuWidget *pWidget) {
    pWidget->GetScreen()->DestroyPanels();
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
    pWidget->GetScreen()->DestroyPanels();
    delete g_pActiveMenuScreen;
    MenuScreen *screen = g_pCachedMenu_InputConfig;
    g_pCachedMenu_AudioSettings = nullptr;
    g_pDisplayModeSelectMenu = nullptr;
    g_pActiveMenuScreen = screen;
    return 0;
}

int ReturnToVideoOptionsFromDeviceSelect(MenuWidget *pWidget) {
    pWidget->GetScreen()->DestroyPanels();
    delete g_pActiveMenuScreen;
    MenuScreen *screen = g_pCachedMenu_InputConfig;
    g_pCachedMenu_VideoSettings = nullptr;
    g_pD3DDeviceSelectMenu = nullptr;
    g_pActiveMenuScreen = screen;
    return 0;
}

int ModemConnectionUI_OnAccept(MenuWidget *pWidget) {
    g_dwServiceProviderIndex = 2;
    pWidget->GetScreen()->DestroyPanels();
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
        pList->GetFont(g_dwFileListSelectedSlot).SetText(g_szMenuItemTextBackup);
        return 1;
    }
    char fileName[260];
    char number[4];
    strcpy(fileName, "RallySeason");
    _itoa(g_dwFileListSelectedSlot, number, 10);
    strcat(fileName, number);
    strcat(fileName, ".brf");
    strcpy(g_szSeasonSaveFileName, fileName);
    g_pRootMenuScreen->GetFileList(0)->SetName(g_dwFileListSelectedSlot,
                                               pList->GetFont(g_dwFileListSelectedSlot).GetText());
    strcpy(g_szSeasonSaveSlotName, pList->GetFont(g_dwFileListSelectedSlot).GetText());
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
        g_pSaveSeasonMenu->GetScreen()->SetPanelActive(1, 1);
    }
    return 1;
}

int PreRaceExitSeasonConfirmUI_OnYes(MenuWidget *pWidget) {
    pWidget->GetScreen()->DestroyPanels();
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
    pWidget->GetScreen()->DestroyPanels();
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
    pWidget->GetScreen()->SetRunning(0);
    pWidget->GetScreen()->ShutdownFrontEnd(0);
    return 0;
}

int ShowCarSetupScreen(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_CarSetup, BuildCarSetupUI);
}

int SeasonProgressInfoUI_OnCarSetup(MenuWidget *) {
    g_bRaceSetupShowCarSetup = 0;
    ShowCarSetupScreen();
    g_bRaceSetupShowCarSetup = 1;
    g_pCarSetupMenu->SetActionCallback(CarSetupUI_OnBackToSeasonProgressInfo);
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    return 1;
}

int LoadTimeAttackMenuUI_OnCarSetup(MenuWidget *) {
    g_bRaceSetupShowCarSetup = 0;
    ShowCarSetupScreen();
    g_bRaceSetupShowCarSetup = 1;
    g_pCarSetupMenu->SetActionCallback(CarSetupUI_OnBackToTimeAttack);
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
    g_pExitSeasonConfirmMenu->SetActionCallback(PreRaceInfoUI_OnBackToLoadSeason);
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    return 1;
}

int OnListDialogItemSelected(MenuWidget *pWidget) {
    ShowPreRaceInfoScreen();
    pWidget->GetScreen()->DestroyPanels();
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
    pWidget->GetScreen()->DestroyPanels();
    delete g_pCachedMenu_SeasonCongrats;
    g_pCachedMenu_SeasonCongrats = nullptr;
    return 0;
}

int SeasonCongratsUI_UpdatePrimaryBannerImage(MenuWidget *pWidget) {
    int flags = g_SeasonProgress.championshipFlags;
    if (flags > 0) {
        pWidget->SetSpriteId(flags <= 5 ? 0x74 - flags : -1);
    }
    if (g_SeasonProgress.championshipFlags == 0) {
        unsigned season = g_SeasonProgress.season;
        if (season >= 1 && season <= 3) {
            pWidget->SetSpriteId(0x45 + season * 2);
        } else {
            pWidget->SetSpriteId(season >= 4 && season <= 6 ? 0x4d : -1);
        }
    }
    return 1;
}

int SeasonCongratsUI_UpdateSecondaryBannerImage(MenuWidget *pWidget) {
    int flags = g_SeasonProgress.championshipFlags;
    if (flags > 0) {
        switch (flags) {
        case 2:
            pWidget->SetSpriteId(0x6d);
            break;
        case 3:
            pWidget->SetSpriteId(0x6e);
            break;
        case 4:
            pWidget->SetSpriteId(0x6c);
            break;
        default:
            pWidget->SetSpriteId(-1);
            break;
        }
    }
    if (g_SeasonProgress.championshipFlags == 0) {
        unsigned season = g_SeasonProgress.season;
        pWidget->SetSpriteId(season >= 1 && season <= 3 ? 0x46 + season * 2 : -1);
    }
    return 1;
}

int MultiplayerSetupUI_UpdateCarGroupImage(MenuWidget *pWidget) {
    unsigned group = static_cast<unsigned>(g_dwSelectedCarClass);
    pWidget->SetSpriteId(group >= 1 && group <= 4 ? 0x55 + group * 2 : 0x56);
    return 1;
}

int MultiplayerSetupUI_UpdateCarGroupSubImage(MenuWidget *pWidget) {
    unsigned group = static_cast<unsigned>(g_dwSelectedCarClass);
    pWidget->SetSpriteId(group >= 1 && group <= 4 ? 0x56 + group * 2 : -1);
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
    unsigned flags = pWidget->GetFlags();
    if ((flags & (0x10 | 0x1000000)) || (pWidget->GetItemList().GetFlags() & 0x1000000)) {
        return 0;
    }
    if (g_dwMenuHoverActive &&
        (g_pMouseDevice->IsButtonHeld(0) || g_pMouseDevice->IsButtonHeld(1) ||
         g_pMouseDevice->IsButtonHeld(2) || g_pMouseDevice->IsButtonHeld(3))) {
        pWidget->GetFont(0).SetStyle(4);
        return 1;
    }
    if (flags & 0x100) {
        pWidget->SetSpriteId(
            static_cast<short>(static_cast<unsigned short>(pWidget->GetSpriteId()) + 1));
        switch (pWidget->GetSpriteId()) {
        case 2:
            pWidget->GetFont(0).SetStyle(0);
            break;
        case 3:
            pWidget->GetFont(0).SetStyle(1);
            break;
        case 4:
            pWidget->GetFont(0).SetStyle(2);
            break;
        case 52:
            pWidget->GetFont(0).SetStyle(4);
            break;
        default:
            pWidget->SetSpriteId(2);
            break;
        }
        pWidget->SetFlags(flags & ~0x100u);
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
        pWidget->GetFont(0).SetDrawCaret(!pWidget->GetFont(0).GetDrawCaret());
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
    g_pSaveSeasonBackHandlerMenu->GetActionCallback()(pWidget);
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    return 0;
}

int AudioOptionsUI_OnMusicTrackChanged(MenuItemList *, int *pTrack) {
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
    MenuFont *font = &pWidget->GetFont(0);
    if (updateText) {
        font->SetText(text);
    }
    font->MeasureText();
    ProcessMenuTextEditInput(pWidget, 0);
    return 1;
}

void PollInputAndUpdateEdges(void) {
    UpdateKeyboardEdgeState();
    UpdateMouseButtonEdges();
}

int ProcessMenuTextEditInput(MenuWidget *pWidget, short nFont) {
    MenuFont *font = &pWidget->GetFont(nFont);

    font->MeasureText();
    if (!font->GetDrawCaret()) {
        font->DrawText();
        return 0;
    }
    signed char result = font->HandleTextInput();
    if (result <= 0 || (pWidget->GetFlags() & 2)) {
        if (!g_dwInNetworkCarSetup) {
            g_dwMenuTextEditActive = 0;
            font->SetDrawCaret(0);
            pWidget->SetFlags(pWidget->GetFlags() & ~2u);
        }
        PollInputAndUpdateEdges();
        if (pWidget->GetTextEditCompleteCallback()) {
            pWidget->GetTextEditCompleteCallback()(pWidget);
        }
    }
    font->DrawText();
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
    pWidget->GetScreen()->ShutdownFrontEnd(0);
    ShutdownDirectPlaySession(g_pDirectPlayInfo);
    ShutdownApplication(0);
}

int RaceSetupUI_UpdateTrackImage(MenuWidget *pWidget) {
    int track = g_dwTrackMenuIndex;
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        int round = g_bSeasonRoundListBrowsing ? g_dwSeasonSavedRound : g_dwSeasonRound;
        track = g_SeasonTable[g_bSeasonClass].races[round][0];
    }
    pWidget->SetSpriteId(g_awTrackImageIds[track]);
    return 1;
}

int TimeAttackUI_UpdateTrackImage(MenuWidget *pWidget) {
    if (g_pActiveMenuScreen == g_pCachedMenu_LoadGhost && !g_dwTimeAttackLoadSelected) {
        return -2;
    }
    pWidget->SetSpriteId(g_awTimeAttackTrackImageIds[g_dwTrackMenuIndex]);
    return 1;
}

int GameOptionsUI_UpdateForceFeedbackImage(MenuWidget *pWidget) {
    int option = g_bForceFeedbackAvailable ? g_dwOptionForceFeedback : 1;
    pWidget->SetSpriteId(g_abForceFeedbackImageIds[option]);
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
    pWidget->SetSpriteId(g_abWeatherImageIds[weather]);
    return 1;
}

int SeasonUI_UpdateSeasonNameLabel(MenuWidget *pWidget) {
    int season = g_dwSeasonLoaded ? g_bSeasonClass : 0;
    if (strlen(GetStringResource(g_SeasonTable[season].nameStringId)) == 0) {
        return 0;
    }
    MenuFont *font = &pWidget->GetFont(0);
    font->SetText(_strupr(GetStringResource(g_SeasonTable[season].nameStringId)));

    font->MeasureText();
    font->DrawText();
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
    MenuFont *font = &pWidget->GetFont(0);
    font->SetText(text);

    font->MeasureText();
    font->DrawText();
    return 1;
}

static inline int UpdateLoadedMenuItemState(MenuWidget *pWidget, int loaded) {
    if (loaded) {
        pWidget->SetFlags(pWidget->GetFlags() & ~0x1010u);
    } else {
        pWidget->SetFlags(pWidget->GetFlags() | 0x1010u);
        pWidget->SetSpriteId(2);
        pWidget->GetFont(0).SetStyle(0);
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
    MenuFont *font = &pWidget->GetFont(0);
    font->SetText(text);

    font->MeasureTextAltFont();
    font->DrawTextAltFont();
}

static inline int DrawSeasonRoundValue(MenuWidget *pWidget, int value) {
    MenuFont *font = &pWidget->GetFont(0);
    font->SetNumber(value);

    font->MeasureTextAltFont();
    font->DrawTextAltFont();
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

static inline s32 TruncateMenuTime(long double value) {
    return value >= -0x1p63L && value < 0x1p63L ? static_cast<s32>(static_cast<s64>(value)) : 0;
}

static inline int DrawTimeAttackTimeLabel(MenuWidget *widget, float seconds) {
    char text[32] = {};
    if (seconds > 0.0f) {
        // Staged products preserve x87 rounding and the single-precision seconds spill.
        volatile long double scaledTime = static_cast<long double>(seconds) * 100.0f;
        s32 totalHundredths = TruncateMenuTime(scaledTime);
        volatile long double wholeTime = static_cast<long double>(totalHundredths) * 0.01f;
        s32 wholeSeconds = TruncateMenuTime(wholeTime);
        float storedSeconds = wholeSeconds;
        volatile long double scaledSeconds = static_cast<long double>(wholeSeconds) * 100.0f;
        s32 hundredths =
            TruncateMenuTime(static_cast<long double>(totalHundredths) - scaledSeconds);
        volatile long double minuteTime = static_cast<long double>(storedSeconds) * 0.016666668f;
        s32 minutes = TruncateMenuTime(minuteTime);
        volatile long double minuteSeconds = static_cast<long double>(minutes) * 60.0f;
        s32 remainingSeconds = TruncateMenuTime(storedSeconds - minuteSeconds);
        std::sprintf(text, "%d:%02d.%02d", minutes, remainingSeconds, hundredths);
    } else {
        std::strcpy(text, "--:--");
    }
    if (std::strlen(text) == 0) {
        return 0;
    }
    MenuFont &font = widget->GetFont(0);
    font.SetText(_strupr(text));
    font.MeasureText();
    font.DrawText();
    return 1;
}

int TimeAttackUI_UpdateRaceTimeLabel(MenuWidget *pWidget) {
    return DrawTimeAttackTimeLabel(pWidget, g_fGhostBestRaceTime);
}

int TimeAttackUI_UpdateBestLapLabel(MenuWidget *pWidget) {
    return DrawTimeAttackTimeLabel(pWidget, g_fGhostBestLapTime);
}

int RaceResultsInfoUI_UpdateSelectedTimeLabel(MenuWidget *pWidget) {
    float seconds = g_dwRaceResultsSelectedLap == 3 ?
                        g_fGhostBestRaceTime :
                        g_afLastRaceLapTimes[g_dwRaceResultsSelectedLap];
    return DrawTimeAttackTimeLabel(pWidget, seconds);
}

int PreRaceInfoUI_UpdateBestTimeLabel(MenuWidget *pWidget) {
    float seconds = 0.0f;
    if (g_dwSeasonLoaded) {
        u8 track = g_SeasonTable[g_bSeasonClass].races[g_dwSeasonSavedRound][0];
        seconds = g_SeasonProgress.raceRecords[track];
    }
    return DrawTimeAttackTimeLabel(pWidget, seconds);
}

int PreRaceInfoUI_UpdateBestLapLabel(MenuWidget *pWidget) {
    float seconds = 0.0f;
    if (g_dwSeasonLoaded) {
        u8 track = g_SeasonTable[g_bSeasonClass].races[g_dwSeasonSavedRound][0];
        seconds = g_SeasonProgress.lapRecords[track];
    }
    return DrawTimeAttackTimeLabel(pWidget, seconds);
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
    MenuFont *font = &pWidget->GetFont(0);
    font->SetText(GetStringResource(0x37));
    font->AppendText("  ");
    font->AppendText(g_szSeasonRoundLabel);

    font->MeasureText();
    font->DrawText();
    return 1;
}

int KeyConfigUI_UpdateDuplicateBindingWarning(MenuWidget *pWidget) {
    MenuFont *font = &pWidget->GetFont(0);
    if (g_dwKeyBindingDuplicatesFound) {
        font->SetText(GetStringResource(0xaf));
        font->SetStyle(g_adwKeyBindingDuplicateFlags[g_dwKeyConfigSelectedAction] ? 4 : 1);
    } else {
        font->SetText(" ");
    }

    font->MeasureText();
    ProcessMenuTextEditInput(pWidget, 0);
    return 1;
}

int PreRaceInfoUI_UpdateBackButtonText(MenuWidget *pWidget) {
    int stringId = g_dwGameMode == 0 && g_SeasonResumeState.state == 0 ? 0x51 : 0xc;
    pWidget->GetFont(0).SetText(GetStringResource(stringId));
    MenuFont *font = &pWidget->GetFont(0);
    font->MeasureText();
    return 1;
}

static inline void DrawPrimaryMenuLabel(MenuWidget *pWidget, const char *text) {
    MenuFont *font = &pWidget->GetFont(0);

    font->SetText(text);
    font->MeasureText();
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

int CarSetupUI_UpdateTiresLabel(MenuWidget *pWidget) {
    DrawPrimaryMenuLabel(pWidget, GetStringResource(g_adwTireLabelStrIds[g_dwTireMenuIndex]));
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
        float y = pWidget->GetY();
        pWidget->GetFont(0).SetY(130.0f);
        DrawPrimaryMenuLabel(pWidget, GetStringResource(0xb0));
        // The widget supplies the font's restored coordinate.
        pWidget->GetFont(0).SetY(y);
    }
    DrawPrimaryMenuLabel(pWidget, text);
    return 1;
}

int RaceSetupUI_UpdateWeatherLabel(MenuWidget *pWidget) {
    bool loadingGhost =
        g_pActiveMenuScreen == g_pCachedMenu_LoadGhost && !g_dwTimeAttackLoadSelected;
    MenuFont *font = &pWidget->GetFont(0);
    int stringId;
    if (loadingGhost) {
        font->SetY(font->GetY() - (8.0f));
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

    font->SetText(text);
    font->MeasureText();
    ProcessMenuTextEditInput(pWidget, 0);
    if (loadingGhost) {
        font->SetY(font->GetY() + (8.0f));
    }
    return 1;
}

int RaceSetupUI_UpdateLapCountLabel(MenuWidget *pWidget) {
    if (g_pActiveMenuScreen != g_pCachedMenu_LoadGhost || g_dwTimeAttackLoadSelected) {
        MenuFont *font = &pWidget->GetFont(0);
        font->SetNumber(g_dwTotalLaps);

        font->MeasureTextAltFont();
        font->DrawTextAltFont();
    }
    return 1;
}

int CarSetupUI_OnAcceptAndSave(MenuActionWidget *pWidget) {
    pWidget->GetScreen()->SetRunning(0);
    pWidget->GetScreen()->ShutdownFrontEnd(0);
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
        pItem->GetScreen()->ShutdownFrontEnd(0);
        ShutdownApplication(0);
    }
    ReturnFromCachedMenu(pItem, g_pCachedMenu_RaceSetupNetwork, g_pCachedMenu_JoinCreateGame);
    if (g_pNetworkSessionJoinMenu) {
        g_pNetworkSessionJoinMenu->SetFlags(g_pNetworkSessionJoinMenu->GetFlags() & ~0x10u);
    }
    DisconnectNetworkSession();
    g_dwSessionEnumPending = 1;
    if ((g_dwServiceProviderIndex == 0 || g_dwServiceProviderIndex == 1) &&
        !g_dwLaunchedFromLobby) {
        InitializeNetworkServiceProvider();
    }
    if ((g_dwServiceProviderIndex == 2 || g_dwServiceProviderIndex == 3) &&
        g_pNetworkSessionJoinMenu) {
        g_pNetworkSessionJoinMenu->SetFlags(g_pNetworkSessionJoinMenu->GetFlags() & ~0x10u);
    }
    return 0;
}

int MultiplayerGameSetupUI_OnBackAsHost(MenuActionWidget *pItem) {
    if (g_dwLaunchedFromLobby) {
        pItem->GetScreen()->ShutdownFrontEnd(0);
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
        g_pNetworkSessionJoinMenu->SetFlags(g_pNetworkSessionJoinMenu->GetFlags() & ~0x10u);
        g_pNetworkSessionJoinMenu->GetFont(0).SetStyle(0);
    }
    return 0;
}

int CreateGameMenuUI_OnBack(MenuActionWidget *pItem) {
    MenuScreen *pDestination;
    NetworkMenuWidget *pJoinMenu;
    pItem->GetScreen()->DestroyPanels();
    delete g_pActiveMenuScreen;
    pJoinMenu = g_pNetworkSessionJoinMenu;
    pDestination = g_pCachedMenu_JoinCreateGame;
    g_pCachedMenu_CreateGame = nullptr;
    g_pMultiplayerGameSetupMenu = nullptr;
    g_pActiveMenuScreen = pDestination;
    if (pJoinMenu) {
        pJoinMenu->SetFlags(pJoinMenu->GetFlags() & ~0x10u);
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
    pItem->GetScreen()->DestroyPanels();
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
    g_pExitSeasonConfirmMenu->SetActionCallback(PreRaceInfoUI_OnBackToLoadSeason);
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    return 1;
}

int CarSetupUI_OnOpenOptions(MenuActionWidget *) {
    ShowOptionsMenu();
    g_pOptionsSubMenu->SetActionCallback(OptionsMenuUI_OnBackToCarSetup);
    return 1;
}

int CarSetupUI_OnOpenSaveSeason(MenuActionWidget *) {
    ShowSaveSeasonScreen();
    g_pSaveSeasonBackHandlerMenu->SetActionCallback(SaveSeasonUI_OnBackToCarSetup);
    return 1;
}

int OptionsMenuUI_OnBackToRaceResultsInfo(MenuActionWidget *pItem) {
    return ReturnFromOptionsMenu(pItem, g_pCachedMenu_RaceResults);
}

int RaceResultsInfoUI_OnOpenOptions(MenuActionWidget *) {
    ShowOptionsMenu();
    g_pOptionsSubMenu->SetActionCallback(OptionsMenuUI_OnBackToRaceResultsInfo);
    return 1;
}

int SaveSeasonUI_OnBackToRaceResultsInfo(MenuActionWidget *pItem) {
    return ReturnFromSaveSeasonMenu(pItem, g_pCachedMenu_RaceResults);
}

int RaceResultsInfoUI_OnOpenSaveSeason(MenuActionWidget *) {
    ShowSaveSeasonScreen();
    g_pSaveSeasonBackHandlerMenu->SetActionCallback(SaveSeasonUI_OnBackToRaceResultsInfo);
    return 1;
}

int OptionsMenuUI_OnBackToRaceResultsSummary(MenuActionWidget *pItem) {
    return ReturnFromOptionsMenu(pItem, g_pCachedMenu_RaceResultsSummary);
}

int RaceResultsSummaryUI_OnOpenOptions(MenuActionWidget *) {
    ShowOptionsMenu();
    g_pOptionsSubMenu->SetActionCallback(OptionsMenuUI_OnBackToRaceResultsSummary);
    return 1;
}

int SaveSeasonUI_OnBackToRaceResultsSummary(MenuActionWidget *pItem) {
    return ReturnFromSaveSeasonMenu(pItem, g_pCachedMenu_RaceResultsSummary);
}

int RaceResultsSummaryUI_OnOpenSaveSeason(MenuActionWidget *) {
    ShowSaveSeasonScreen();
    g_pSaveSeasonBackHandlerMenu->SetActionCallback(SaveSeasonUI_OnBackToRaceResultsSummary);
    return 1;
}

int OptionsMenuUI_OnBackToRaceResultsTotals(MenuActionWidget *pItem) {
    return ReturnFromOptionsMenu(pItem, g_pCachedMenu_RaceResultsTotals);
}

int RaceResultsTotalsUI_OnOpenOptions(MenuActionWidget *) {
    ShowOptionsMenu();
    g_pOptionsSubMenu->SetActionCallback(OptionsMenuUI_OnBackToRaceResultsTotals);
    return 1;
}

int SaveSeasonUI_OnBackToRaceResultsTotals(MenuActionWidget *pItem) {
    return ReturnFromSaveSeasonMenu(pItem, g_pCachedMenu_RaceResultsTotals);
}

int RaceResultsTotalsUI_OnOpenSaveSeason(MenuActionWidget *) {
    ShowSaveSeasonScreen();
    g_pSaveSeasonBackHandlerMenu->SetActionCallback(SaveSeasonUI_OnBackToRaceResultsTotals);
    return 1;
}

int RaceResultsSummaryUI_OnBack(MenuActionWidget *pItem) {
    pItem->GetScreen()->DestroyPanels();
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

static inline void BuildSeasonCongratsScreen(MenuScreen *screen) {
    BuildSeasonCongratsUI(screen);
}

static inline void BuildNotEnoughPointsScreen(MenuScreen *screen) {
    BuildNotEnoughPointsUI(screen);
}

int ShowSeasonCongratsScreen(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_SeasonCongrats, BuildSeasonCongratsScreen);
}

int ShowNotEnoughPointsScreen(void) {
    return ShowCachedMenuScreen(g_pCachedMenu_NotEnoughPoints, BuildNotEnoughPointsScreen);
}

static inline void BuildPreRaceExitSeasonConfirmScreen(MenuScreen *screen) {
    BuildPreRaceExitSeasonConfirmUI(screen);
}

static inline void BuildResultsSummaryExitSeasonConfirmScreen(MenuScreen *screen) {
    BuildResultsSummaryExitSeasonConfirmUI(screen);
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
        pConfirmation->SetBuildCallback(pfnBuildConfirmation);
        pConfirmation->GetBuildCallback()(pConfirmation);
        pConfirmation->SetBuilt(true);
    }
    return 1;
}

int ShowRaceResultsSummaryScreen(MenuWidget *) {
    return ShowCachedMenuWithExitConfirmation(g_pCachedMenu_RaceResultsSummary,
                                              BuildRaceResultsSummaryUI,
                                              g_pRaceResultsSummaryMenuScreen,
                                              BuildResultsSummaryExitSeasonConfirmScreen);
}

static inline int CompleteConnectionTextEdit(MenuWidget *) {
    /* The text callback shares the controller initialiser's constant return. */
    return osContInit(nullptr, nullptr, nullptr);
}

static inline MenuWidget *AddMenuWidget(
    MenuPanel *panel, MenuScreen *screen, float x, float y, unsigned flags, int mode, short image) {
    MenuWidget *widget = new (std::nothrow) MenuWidget();
    panel->SetWidget(panel->GetWidgetCount(), widget);
    if (!widget) {
        ShowErrorMessage(4);
    }
    widget->Initialize(screen, x, y, flags, 2, 5, mode, image);
    return widget;
}

static inline void SetMenuWidgetLabel(MenuWidget *widget, int stringId, const RECT *rect) {
    widget->SetSpriteId(3);
    widget->SetText(GetStringResource(stringId), 1, 1, rect);
}

static inline int BuildConfirmationUI(MenuScreen *screen,
                                      int prompt,
                                      const RECT *promptBounds,
                                      int yesLabel,
                                      int noLabel,
                                      float spacing,
                                      int (*yesAction)(MenuWidget *),
                                      int (*noAction)(MenuWidget *),
                                      bool exitSeason) {
    screen->SetCurrentPanelIndex(0);
    screen->SetPanelActive(screen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    screen->SetPanel(screen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    screen->SetPanelCount(screen->GetPanelCount() + 1);
    panel->SetScreen(screen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    AddMenuWidget(panel, screen, 0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget = AddMenuWidget(panel, screen, panel->GetX(), 10, 0x100009, 1, -1);
    SetMenuWidgetLabel(widget, prompt, promptBounds);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = AddMenuWidget(panel, screen, panel->GetX(), panel->GetY(), 0x102001, 1, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(yesAction);
    SetMenuWidgetLabel(widget, yesLabel, &g_rcMenuCenterPanel);
    if (exitSeason) {
        g_pExitSeasonConfirmMenu = widget;
    }
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    float y = static_cast<float>(static_cast<long double>(panel->GetY()) + spacing);
    widget = AddMenuWidget(panel, screen, panel->GetX(), y, 0x102001, 1, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(noAction);
    SetMenuWidgetLabel(widget, noLabel, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    return 1;
}

static inline MenuPanel *CreateSeasonResultPanel(MenuScreen *screen) {
    screen->SetCurrentPanelIndex(0);
    screen->SetPanelActive(screen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    screen->SetPanel(screen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    screen->SetPanelCount(screen->GetPanelCount() + 1);
    panel->SetScreen(screen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    return panel;
}

int BuildSeasonCongratsUI(MenuScreen *screen) {
    MenuPanel *panel = CreateSeasonResultPanel(screen);
    AddMenuWidget(panel, screen, 0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    AddMenuWidget(panel, screen, 0, 29, 9, 0, 78);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    AddMenuWidget(panel, screen, 13, 7, 9, 0, 79);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget = AddMenuWidget(panel, screen, 16, 153, 9, 1, 71);
    widget->SetUpdateCallback(SeasonCongratsUI_UpdatePrimaryBannerImage);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = AddMenuWidget(panel, screen, 392, 181, 9, 1, 72);
    widget->SetUpdateCallback(SeasonCongratsUI_UpdateSecondaryBannerImage);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = AddMenuWidget(panel, screen, panel->GetX(), 460, 0x102001, 0, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(OnPressAnyKeyContinue);
    widget->SetUpdateCallback(SeasonCongratsUI_PollAnyKey);
    widget->SetSpriteId(2);
    widget->SetText(GetStringResource(66), 1, 0, &g_rcMenuFullScreen);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    return 1;
}

int BuildNotEnoughPointsUI(MenuScreen *screen) {
    MenuPanel *panel = CreateSeasonResultPanel(screen);
    AddMenuWidget(panel, screen, 0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget = AddMenuWidget(panel, screen, panel->GetX(), 10, 0x100009, 1, -1);
    SetMenuWidgetLabel(widget, 67, &g_rcMenuFullScreen);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    float y = static_cast<float>(static_cast<long double>(panel->GetY()) - 12);
    float x = static_cast<float>(static_cast<long double>(panel->GetX()) - 8);
    widget = AddMenuWidget(panel, screen, x, y, 0x22001, 0, 80);
    widget->SetImageRects(g_aMenuLargeIconRects);
    widget->SetImageMode(1);
    widget->SetCustomFrameDurations(true);
    for (int i = 0; i < 24; ++i) {
        widget->SetFrameDuration(i, 60);
        widget->SetAnimationSpriteId(i, i < 15 ? 80 : 81);
    }
    widget->SetActionCallback(OnListDialogItemSelected);
    widget->SetSpriteId(80);
    widget->SetBoundsTop(ConvertMenuCoordinate(panel->GetY()));
    widget->SetBoundsLeft(ConvertMenuCoordinate(panel->GetX()));
    widget->SetBoundsRight(
        static_cast<int>(static_cast<unsigned>(ConvertMenuCoordinate(panel->GetX())) + 128u));
    widget->SetBoundsBottom(
        static_cast<int>(static_cast<unsigned>(ConvertMenuCoordinate(panel->GetY())) + 128u));
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    return 1;
}

static inline int TimeAttackMenuUI_OnStart(MenuWidget *) {
    return ReturnToMainMenuFromSeason();
}

static inline int TimeAttackMenuUI_OnLoadGhost(MenuWidget *) {
    return ShowLoadGhostMenu();
}

int BuildTimeAttackMenuUI(MenuScreen *screen) {
    screen->SetCurrentPanelIndex(0);
    screen->SetPanelActive(screen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    screen->SetPanel(screen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    screen->SetPanelCount(screen->GetPanelCount() + 1);
    panel->SetScreen(screen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    AddMenuWidget(panel, screen, 0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget = AddMenuWidget(panel, screen, panel->GetX(), 10, 0x100009, 1, -1);
    SetMenuWidgetLabel(widget, 106, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    const float offsets[3] = {0, 19, 114};
    const int labels[3] = {107, 108, 12};
    int (*actions[3])(MenuWidget *) = {
        TimeAttackMenuUI_OnStart, TimeAttackMenuUI_OnLoadGhost, TimeAttackMenuUI_OnBack};
    for (int i = 0; i < 3; ++i) {
        float y = static_cast<float>(static_cast<long double>(panel->GetY()) + offsets[i]);
        widget = AddMenuWidget(panel, screen, panel->GetX(), y, 0x102001, 1, -1);
        widget->SetIdleActionCallback(ValidateEntityState);
        widget->SetActionCallback(actions[i]);
        SetMenuWidgetLabel(widget, labels[i], &g_rcMenuCenterPanel);
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    }
    AddMenuWidget(panel, screen, 80, 46, 9, 0, 8);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    return 1;
}

int BuildQuitGameConfirmUI(MenuScreen *screen) {
    return BuildConfirmationUI(screen,
                               14,
                               &g_rcMenuTitleBar,
                               15,
                               12,
                               114,
                               QuitGameConfirmUI_OnConfirmQuit,
                               QuitGameConfirmUI_OnBack,
                               false);
}

int BuildPreRaceExitSeasonConfirmUI(MenuScreen *screen) {
    return BuildConfirmationUI(screen,
                               16,
                               &g_rcMenuFullScreen,
                               17,
                               18,
                               19,
                               PreRaceExitSeasonConfirmUI_OnYes,
                               PreRaceExitSeasonConfirmUI_OnNo,
                               true);
}

int BuildResultsSummaryExitSeasonConfirmUI(MenuScreen *screen) {
    return BuildConfirmationUI(screen,
                               16,
                               &g_rcMenuFullScreen,
                               17,
                               18,
                               19,
                               ResultsSummaryExitSeasonConfirmUI_OnYes,
                               ResultsSummaryExitSeasonConfirmUI_OnNo,
                               true);
}

static inline int JoinCreateGameUI_OnDial(MenuWidget *) {
    return ShowModemDialUpScreen();
}

static inline int JoinCreateGameUI_OnJoin(MenuWidget *) {
    return JoinSessionAndShowRaceSetup();
}

static inline int JoinCreateGameUI_OnCreate(MenuWidget *) {
    return ShowCreateGameScreen();
}

int BuildJoinCreateGameUI(MenuScreen *screen) {
    screen->SetCurrentPanelIndex(0);
    ResetSessionSlotTable();
    g_dwSessionEnumPending = 1;
    screen->SetPanelActive(screen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    screen->SetPanel(screen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    screen->SetPanelCount(screen->GetPanelCount() + 1);
    panel->SetScreen(screen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    AddMenuWidget(panel, screen, 0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(
        AddMenuWidget(panel, screen, panel->GetX(), 10, 0x100009, 1, -1), 0x5f, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget = AddMenuWidget(panel, screen, panel->GetX(), panel->GetY(), 0x3001, 1, -1);
    widget->SetUpdateCallback(JoinCreateGameUI_UpdateSessionSelection);
    widget->GetItemList().SetVerticalScrollbar(1);
    widget->GetItemList().Initialize(0x40001, &g_rcMenuSessionListBox, 3, 0, -1);
    g_pJoinCreateGameMenu = widget;
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    if (g_dwServiceProviderIndex == 2) {
        widget = AddMenuWidget(panel,
                               screen,
                               panel->GetX(),
                               static_cast<float>(static_cast<long double>(panel->GetY()) + 57),
                               0x102001,
                               1,
                               -1);
        widget->SetIdleActionCallback(ValidateEntityState);
        widget->SetActionCallback(JoinCreateGameUI_OnDial);
        SetMenuWidgetLabel(widget, 0x5e, &g_rcMenuCenterPanel);
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    }
    const bool canJoin = g_dwServiceProviderIndex == 2 || g_dwServiceProviderIndex == 3;
    widget = AddMenuWidget(panel,
                           screen,
                           panel->GetX(),
                           static_cast<float>(static_cast<long double>(panel->GetY()) + 76),
                           canJoin ? 0x102001 : 0x102011,
                           1,
                           -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(JoinCreateGameUI_OnJoin);
    widget->SetSpriteId(2);
    widget->SetText(GetStringResource(0x60), 1, canJoin ? 1 : 0, &g_rcMenuCenterPanel);
    g_pNetworkSessionJoinMenu = widget;
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    int (*actions[2])(MenuWidget *) = {JoinCreateGameUI_OnCreate, JoinCreateGameUI_OnBack};
    for (int i = 0; i < 2; ++i) {
        widget = AddMenuWidget(
            panel,
            screen,
            panel->GetX(),
            static_cast<float>(static_cast<long double>(panel->GetY()) + (i == 0 ? 95 : 114)),
            0x102001,
            1,
            -1);
        widget->SetIdleActionCallback(ValidateEntityState);
        widget->SetActionCallback(actions[i]);
        SetMenuWidgetLabel(widget, i == 0 ? 0x61 : 0xc, &g_rcMenuCenterPanel);
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    }
    AddMenuWidget(panel, screen, 80, 46, 9, 0, 7);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    return 1;
}

static inline int OptionsMenuUI_OnInput(MenuWidget *) {
    return ShowInputOptionsMenu();
}

static inline int OptionsMenuUI_OnVideo(MenuWidget *) {
    return ShowVideoOptionsMenu();
}

static inline int OptionsMenuUI_OnAudio(MenuWidget *) {
    return ShowAudioOptionsMenu();
}

static inline int OptionsMenuUI_OnGame(MenuWidget *) {
    return ShowGameOptionsMenu();
}

int BuildOptionsMenuUI(MenuScreen *screen) {
    screen->SetCurrentPanelIndex(0);
    screen->SetPanelActive(screen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    screen->SetPanel(screen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    screen->SetPanelCount(screen->GetPanelCount() + 1);
    panel->SetScreen(screen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    AddMenuWidget(panel, screen, 0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(
        AddMenuWidget(panel, screen, panel->GetX(), 10, 0x100009, 1, -1), 6, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    int (*actions[5])(MenuWidget *) = {OptionsMenuUI_OnInput,
                                       OptionsMenuUI_OnVideo,
                                       OptionsMenuUI_OnAudio,
                                       OptionsMenuUI_OnGame,
                                       OptionsMenuUI_OnBack};
    for (int i = 0; i < 5; ++i) {
        int spacing = i < 4 ? 19 * i : 114;
        MenuWidget *widget =
            AddMenuWidget(panel,
                          screen,
                          panel->GetX(),
                          static_cast<float>(static_cast<long double>(panel->GetY()) + spacing),
                          0x102001,
                          1,
                          -1);
        widget->SetIdleActionCallback(ValidateEntityState);
        widget->SetActionCallback(actions[i]);
        SetMenuWidgetLabel(widget, i < 4 ? 0x1f + i : 0xc, &g_rcMenuCenterPanel);
        if (i == 4) {
            g_pOptionsSubMenu = widget;
        }
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    }
    AddMenuWidget(panel, screen, 80, 46, 9, 0, 9);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget = AddMenuWidget(panel, screen, 324, 46, 1, 1, 22);
    widget->SetUpdateCallback(InputOptionsUI_UpdateInputDeviceImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = AddMenuWidget(panel, screen, panel->GetX(), 155, 0x101001, 1, -1);
    widget->SetUpdateCallback(InputOptionsUI_UpdateInputDeviceLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuWideColumn);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    return 1;
}

int BuildModemConnectionUI(MenuScreen *screen) {
    screen->SetCurrentPanelIndex(0);
    screen->SetPanelActive(screen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    screen->SetPanel(screen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    screen->SetPanelCount(screen->GetPanelCount() + 1);
    panel->SetScreen(screen);
    panel->SetFlags(0);
    panel->SetX(190);
    panel->SetY(130);
    AddMenuWidget(panel, screen, 0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(
        AddMenuWidget(panel, screen, panel->GetX(), 10, 0x100009, 1, -1), 0x5d, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget =
        AddMenuWidget(panel, screen, panel->GetX(), panel->GetY(), 0x100009, 1, -1);
    widget->SetSpriteId(0x34);
    widget->SetText(GetStringResource(0x5e), 1, 4, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    AddMenuWidget(panel, screen, 156, 172, 9, 0, 57);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = AddMenuWidget(panel, screen, panel->GetX(), 174, 0x200001, 1, -1);
    widget->SetActionCallback(ConnectionUI_OnToggleTextEdit);
    widget->SetUpdateCallback(OnModemPhoneNumberEdited);
    widget->SetTextEditCompleteCallback(ModemConnectionUI_EnableContinueIfNumberEntered);
    widget->SetSpriteId(3);
    widget->SetText(g_szEmptyUiString, 1, 1, &g_rcMenuCenterPanel);
    MenuFont *font = &widget->GetFont(0);
    font->SetText(g_szModemPhoneNumber);
    font->MeasureText();
    widget->SetBoundsLeft(font->SetBoundsLeft(155));
    widget->SetBoundsRight(font->SetBoundsRight(347));
    widget->SetBoundsTop(font->SetBoundsTop(172));
    widget->SetBoundsBottom(font->SetBoundsBottom(188));
    font->SetInputMaxWidth(static_cast<short>(static_cast<unsigned short>(font->GetBoundsRight()) -
                                              static_cast<unsigned short>(font->GetBoundsLeft()) -
                                              16));
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    widget = AddMenuWidget(panel,
                           screen,
                           panel->GetX(),
                           static_cast<float>(static_cast<long double>(panel->GetY()) + 95),
                           0x102011,
                           1,
                           -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(ModemConnectionUI_OnAccept);
    widget->SetSpriteId(2);
    widget->SetText(GetStringResource(0x1e), 1, 0, &g_rcMenuCenterPanel);
    g_pModemContinueItem = widget;
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    widget = AddMenuWidget(panel,
                           screen,
                           panel->GetX(),
                           static_cast<float>(static_cast<long double>(panel->GetY()) + 114),
                           0x102001,
                           1,
                           -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(ModemConnectionUI_OnBack);
    SetMenuWidgetLabel(widget, 0xc, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    AddMenuWidget(panel, screen, 80, 46, 9, 0, 7);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    return 1;
}

#if TGR_RENDERER == TGR_RENDERER_D3D
static inline int VideoOptionsUI_ShowDeviceSelect(MenuWidget *) {
    return ShowD3DDeviceSelectMenu();
}

static inline void BuildDisplaySelectionUI(MenuScreen *screen,
                                           int title,
                                           int (*update)(MenuWidget *),
                                           MenuWidget *&selection,
                                           int (*apply)(MenuWidget *),
                                           int applyLabel,
                                           int (*back)(MenuWidget *)) {
    screen->SetCurrentPanelIndex(0);
    screen->SetPanelActive(screen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    screen->SetPanel(screen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    screen->SetPanelCount(screen->GetPanelCount() + 1);
    panel->SetScreen(screen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    AddMenuWidget(panel, screen, 0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(
        AddMenuWidget(panel, screen, panel->GetX(), 10, 0x100009, 1, -1), title, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    AddMenuWidget(panel, screen, 80, 46, 9, 0, 9);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    AddMenuWidget(panel, screen, 96, 61, 9, 0, 0x8e);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget = AddMenuWidget(panel, screen, panel->GetX(), panel->GetY(), 0x3001, 1, -1);
    widget->SetUpdateCallback(update);
    MenuItemList &list = widget->GetItemList();
    list.SetVerticalScrollbar(1);
    list.Initialize(0x40001, &g_rcMenuDeviceListBox, 7, 0, -1);
    selection = widget;
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    const float offsets[] = {95, 114};
    for (int index = 0; index < 2; index++) {
        widget = AddMenuWidget(
            panel,
            screen,
            panel->GetX(),
            static_cast<float>(static_cast<long double>(panel->GetY()) + offsets[index]),
            0x102001,
            1,
            -1);
        widget->SetIdleActionCallback(ValidateEntityState);
        widget->SetActionCallback(index == 0 ? apply : back);
        SetMenuWidgetLabel(widget, index == 0 ? applyLabel : 0xc, &g_rcMenuCenterPanel);
        g_pOptionsSubMenu = widget;
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    }
}

void BuildD3DDeviceSelectUI(MenuScreen *screen) {
    BuildDisplaySelectionUI(screen,
                            0x2b,
                            D3DDeviceSelectUI_UpdateDeviceSelection,
                            g_pD3DDeviceSelectMenu,
                            D3DDeviceSelectUI_OnSetDriver,
                            0x2c,
                            ReturnToVideoOptionsFromDeviceSelect);
}

void BuildDisplayModeSelectUI(MenuScreen *screen) {
    BuildDisplaySelectionUI(screen,
                            0x2a,
                            DisplayModeSelectUI_UpdateModeSelection,
                            g_pDisplayModeSelectMenu,
                            DisplayModeSelectUI_OnSetMode,
                            0x2d,
                            ReturnToVideoOptionsFromResolution);
}

static inline int VideoOptionsUI_ShowDisplayModeSelect(MenuWidget *) {
    return ShowDisplayModeSelectMenu();
}

void BuildVideoOptionsUI(MenuScreen *screen) {
    screen->SetCurrentPanelIndex(0);
    screen->SetPanelActive(screen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    screen->SetPanel(screen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    screen->SetPanelCount(screen->GetPanelCount() + 1);
    panel->SetScreen(screen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    AddMenuWidget(panel, screen, 0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(
        AddMenuWidget(panel, screen, panel->GetX(), 10, 0x100009, 1, -1), 0x20, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    AddMenuWidget(panel, screen, 80, 46, 9, 0, 9);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    const int strings[] = {0x29, 0x2a, 0xc};
    const float offsets[] = {0, 19, 114};
    int (*const actions[])(MenuWidget *) = {VideoOptionsUI_ShowDeviceSelect,
                                            VideoOptionsUI_ShowDisplayModeSelect,
                                            VideoOptionsUI_OnBack};
    for (int index = 0; index < 3; index++) {
        MenuWidget *widget = AddMenuWidget(
            panel,
            screen,
            panel->GetX(),
            index == 0 ?
                panel->GetY() :
                static_cast<float>(static_cast<long double>(panel->GetY()) + offsets[index]),
            0x102001,
            1,
            -1);
        widget->SetIdleActionCallback(ValidateEntityState);
        widget->SetActionCallback(actions[index]);
        SetMenuWidgetLabel(widget, strings[index], &g_rcMenuCenterPanel);
        if (index == 2) {
            g_pOptionsSubMenu = widget;
        }
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    }
}
#endif

template <auto Action>
    requires std::invocable<decltype(Action)> && std::same_as<decltype(Action()), int>
static int InvokeRootMenuAction(MenuWidget *) {
    return Action();
}

static inline int RootMenuUI_ShowTimeAttack(MenuWidget *) {
    return ShowTimeAttackMenu(0);
}

void BuildFrontEndRootMenuUI(MenuScreen *screen) {
    screen->SetCurrentPanelIndex(0);
    screen->SetPanelActive(screen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    screen->SetPanel(screen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    screen->SetPanelCount(screen->GetPanelCount() + 1);
    panel->SetScreen(screen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(125);
    AddMenuWidget(panel, screen, 0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget = new (std::nothrow) MenuWidget();
    panel->SetWidget(199, widget);
    if (!widget) {
        ShowErrorMessage(4);
    }
    widget->Initialize(screen, 0, 0, 9, 2, 5, 0, 1);
    SetMenuWidgetLabel(
        AddMenuWidget(panel, screen, panel->GetX(), 10, 0x100009, 1, -1), 1, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    const float offsets[] = {0, 19, 38, 57, 76, 95, 114};
    const short images[] = {6, 7, 8, 10, 9};
    int (*const actions[])(MenuWidget *) = {InvokeRootMenuAction<ShowChampionshipSeasonMenu>,
                                            InvokeRootMenuAction<ShowMultiplayerNameScreen>,
                                            RootMenuUI_ShowTimeAttack,
                                            InvokeRootMenuAction<ShowRaceSetupMenu>,
                                            InvokeRootMenuAction<ShowOptionsMenu>,
                                            MainMenuUI_OnPlayCredits,
                                            InvokeRootMenuAction<ShowQuitGameConfirmMenu>};
    int (*const idleActions[])(MenuWidget *) = {ValidateEntityState,
                                                ValidateEntityStateAlwaysTrue,
                                                ValidateEntityStateAlwaysTrue,
                                                ValidateEntityState,
                                                ValidateEntityStateAlwaysTrue,
                                                ProcessCheatInputAndValidateEntity,
                                                ValidateEntityStateAlwaysTrue};
    for (int index = 0; index < 7; index++) {
        widget = AddMenuWidget(
            panel,
            screen,
            panel->GetX(),
            index == 0 ?
                panel->GetY() :
                static_cast<float>(static_cast<long double>(panel->GetY()) + offsets[index]),
            0x102001,
            1,
            -1);
        widget->SetIdleActionCallback(idleActions[index]);
        widget->SetActionCallback(actions[index]);
        SetMenuWidgetLabel(widget, index + 2, &g_rcMenuCenterPanel);
        if (index < 6) {
            widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
            widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
        }
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        panel->SetNavigationCount(panel->GetNavigationCount() + 1);
        if (index < 5) {
            AddMenuWidget(panel, screen, 80, 46, 0x809, 0, images[index]);
            panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        }
    }
    widget = AddMenuWidget(panel, screen, panel->GetX(), 29, 0x100009, 1, -1);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuRootTitleBar);
    g_dwRootMenuStringIndex = panel->GetWidgetCount();
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
}

static inline int ChampionshipSeasonUI_ShowLoadSeason(MenuWidget *) {
    return ShowLoadSeasonMenu();
}

void BuildChampionshipSeasonUI(MenuScreen *screen) {
    screen->SetCurrentPanelIndex(0);
    screen->SetPanelActive(screen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    screen->SetPanel(screen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    screen->SetPanelCount(screen->GetPanelCount() + 1);
    panel->SetScreen(screen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    AddMenuWidget(panel, screen, 0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(
        AddMenuWidget(panel, screen, panel->GetX(), 10, 0x100009, 1, -1), 9, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    const int strings[] = {10, 11, 12};
    const float offsets[] = {0, 19, 114};
    int (*const actions[])(MenuWidget *) = {ChampionshipSeasonUI_ShowLoadSeason,
                                            ChampionshipSeasonUI_OnStartNewSeason,
                                            ChampionshipSeasonUI_OnBack};
    for (int index = 0; index < 3; index++) {
        MenuWidget *widget = AddMenuWidget(
            panel,
            screen,
            panel->GetX(),
            index == 0 ?
                panel->GetY() :
                static_cast<float>(static_cast<long double>(panel->GetY()) + offsets[index]),
            0x102001,
            1,
            -1);
        widget->SetIdleActionCallback(ValidateEntityState);
        widget->SetActionCallback(actions[index]);
        SetMenuWidgetLabel(widget, strings[index], &g_rcMenuCenterPanel);
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    }
    AddMenuWidget(panel, screen, 80, 46, 9, 0, 6);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
}

static inline int InputOptionsUI_ShowKeyConfig(MenuWidget *) {
    return ShowKeyConfigScreen();
}

void BuildInputOptionsUI(MenuScreen *screen) {
    screen->SetCurrentPanelIndex(0);
    screen->SetPanelActive(screen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    screen->SetPanel(screen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    screen->SetPanelCount(screen->GetPanelCount() + 1);
    panel->SetScreen(screen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    AddMenuWidget(panel, screen, 0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(
        AddMenuWidget(panel, screen, panel->GetX(), 10, 0x100009, 1, -1), 0x1f, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    const int strings[] = {0x23, 0x24, 0x25, 0xc};
    const float offsets[] = {0, 19, 38, 114};
    int (*const actions[])(MenuWidget *) = {InputOptionsUI_OnCycleInputDevice,
                                            InputOptionsUI_ShowKeyConfig,
                                            InputOptionsUI_OnRestoreDefaults,
                                            InputOptionsUI_OnBack};
    for (int index = 0; index < 4; index++) {
        MenuWidget *widget = AddMenuWidget(
            panel,
            screen,
            panel->GetX(),
            index == 0 ?
                panel->GetY() :
                static_cast<float>(static_cast<long double>(panel->GetY()) + offsets[index]),
            0x102001,
            1,
            -1);
        widget->SetIdleActionCallback(ValidateEntityState);
        widget->SetActionCallback(actions[index]);
        SetMenuWidgetLabel(widget, strings[index], &g_rcMenuCenterPanel);
        if (index == 3) {
            g_pOptionsSubMenu = widget;
        }
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    }
    AddMenuWidget(panel, screen, 80, 46, 9, 0, 9);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget = AddMenuWidget(panel, screen, 324, 46, 1, 1, 0x16);
    widget->SetUpdateCallback(InputOptionsUI_UpdateInputDeviceImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = AddMenuWidget(panel, screen, panel->GetX(), 155, 0x101001, 1, -1);
    widget->SetUpdateCallback(InputOptionsUI_UpdateInputDeviceLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuWideColumn);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
}

int BuildGameOptionsUI(MenuScreen *screen) {
    screen->SetCurrentPanelIndex(0);
    screen->SetPanelActive(screen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    screen->SetPanel(screen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    screen->SetPanelCount(screen->GetPanelCount() + 1);
    panel->SetScreen(screen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    AddMenuWidget(panel, screen, 0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(
        AddMenuWidget(panel, screen, panel->GetX(), 10, 0x100009, 1, -1), 0x22, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    ProbeJoystickDevice();
    MenuWidget *widget = AddMenuWidget(panel,
                                       screen,
                                       panel->GetX(),
                                       panel->GetY(),
                                       g_bForceFeedbackAvailable ? 0x102001 : 0x102011,
                                       1,
                                       -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(GameOptionsUI_OnToggleForceFeedback);
    bool available = g_bForceFeedbackAvailable != 0;
    widget->SetSpriteId(available ? 3 : 2);
    widget->SetText(GetStringResource(0x30), 1, available, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    const int strings[] = {0x31, 0x32, 0x33, 0xc};
    const float offsets[] = {19, 38, 57, 114};
    int (*const actions[])(MenuWidget *) = {GameOptionsUI_OnToggleSkidMarks,
                                            GameOptionsUI_OnToggleSpecularLighting,
                                            GameOptionsUI_OnToggleCarShadow,
                                            GameOptionsUI_OnBack};
    for (int index = 0; index < 4; index++) {
        widget = AddMenuWidget(
            panel,
            screen,
            panel->GetX(),
            static_cast<float>(static_cast<long double>(panel->GetY()) + offsets[index]),
            0x102001,
            1,
            -1);
        widget->SetIdleActionCallback(ValidateEntityState);
        widget->SetActionCallback(actions[index]);
        SetMenuWidgetLabel(widget, strings[index], &g_rcMenuCenterPanel);
        if (index == 3) {
            g_pOptionsSubMenu = widget;
        }
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    }
    AddMenuWidget(panel, screen, 80, 46, 9, 0, 9);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    const float imageX[] = {336, 67, 440, 484};
    const float imageY[] = {48, 199, 114, 214};
    const short images[] = {0x61, 0x8c, 0x65, 99};
    const float labelY[] = {166, 279, 196, 273};
    const RECT *bounds[] = {
        &g_rcMenuWideColumn, &g_rcMenuLeftColumn, &g_rcMenuRightColumn, &g_rcMenuRightColumnWide};
    int (*const imageUpdates[])(MenuWidget *) = {GameOptionsUI_UpdateForceFeedbackImage,
                                                 GameOptionsUI_UpdateSkidMarksImage,
                                                 GameOptionsUI_UpdateSpecularLightingImage,
                                                 GameOptionsUI_UpdateCarShadowImage};
    int (*const labelUpdates[])(MenuWidget *) = {GameOptionsUI_UpdateForceFeedbackLabel,
                                                 GameOptionsUI_UpdateSkidMarksLabel,
                                                 GameOptionsUI_UpdateSpecularLightingLabel,
                                                 GameOptionsUI_UpdateCarShadowLabel};
    for (int index = 0; index < 4; index++) {
        widget = AddMenuWidget(panel, screen, imageX[index], imageY[index], 1, 1, images[index]);
        widget->SetUpdateCallback(imageUpdates[index]);
        widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
        widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        widget = AddMenuWidget(panel, screen, panel->GetX(), labelY[index], 0x101001, 1, -1);
        widget->SetUpdateCallback(labelUpdates[index]);
        widget->SetSpriteId(3);
        widget->SetText(g_szSpace, 1, 1, bounds[index]);
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    }
    return 1;
}

int BuildAudioOptionsUI(MenuScreen *screen) {
    screen->SetCurrentPanelIndex(0);
    screen->SetPanelActive(screen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    screen->SetPanel(screen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    screen->SetPanelCount(screen->GetPanelCount() + 1);
    panel->SetScreen(screen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    AddMenuWidget(panel, screen, 0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(
        AddMenuWidget(panel, screen, panel->GetX(), 10, 0x100009, 1, -1), 0x21, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    AddMenuWidget(panel, screen, 80, 46, 9, 0, 9);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget = AddMenuWidget(panel, screen, panel->GetX(), panel->GetY(), 0x3001, 1, -1);
    widget->SetUpdateCallback(AudioOptionsUI_UpdateCdTrackSelection);
    MenuItemList &list = widget->GetItemList();
    list.SetVerticalScrollbar(1);
    int selection = g_dwSelectedCdTrack < 0 ? 0 : g_dwSelectedCdTrack;
    if (selection > 11) {
        selection = 11;
    }
    list.Initialize(0x40001, &g_rcMenuAudioListBox, 4, selection, -1);
    list.SetSelectionChangedCallback(AudioOptionsUI_OnMusicTrackChanged);
    for (const char *track : g_apszBgmTrackNames) {
        list.AddItem(track, 0, 1, &g_rcMenuListBox, true);
    }
    if (g_dwSelectedCdTrack < 0) {
        list.SetScrollThumbY(list.GetMinScrollThumbY());
    } else if (g_dwSelectedCdTrack < 12) {
        volatile long double span =
            static_cast<long double>(list.GetMaxScrollThumbY()) - list.GetMinScrollThumbY();
        volatile long double position = span * g_dwSelectedCdTrack;
        volatile long double scaledPosition = position * -0.09090909f;
        list.SetScrollThumbY(static_cast<float>(list.GetMinScrollThumbY() - scaledPosition));
    } else {
        list.SetScrollThumbY(list.GetMaxScrollThumbY());
    }
    int top = ConvertMenuCoordinate(list.GetScrollThumbY());
    list.SetScrollThumbTop(top);
    list.SetScrollThumbBottom(static_cast<int>(static_cast<unsigned>(top) + 16u));
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    const int strings[] = {0x2e, 0x2f, 0xc};
    const float offsets[] = {76, 95, 114};
    int (*const actions[])(MenuWidget *) = {AudioOptionsUI_OnCycleSfxVolume,
                                            AudioOptionsUI_OnCycleMusicVolume,
                                            AudioOptionsUI_OnBackAndSave};
    for (int index = 0; index < 3; index++) {
        widget = AddMenuWidget(
            panel,
            screen,
            panel->GetX(),
            static_cast<float>(static_cast<long double>(panel->GetY()) + offsets[index]),
            0x102001,
            1,
            -1);
        widget->SetIdleActionCallback(ValidateEntityState);
        widget->SetActionCallback(actions[index]);
        SetMenuWidgetLabel(widget, strings[index], &g_rcMenuCenterPanel);
        if (index == 2) {
            g_pOptionsSubMenu = widget;
        }
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    }
    widget = AddMenuWidget(panel, screen, 76, 211, 1, 1, 0x68);
    widget->SetUpdateCallback(AudioOptionsUI_UpdateVolumeChannelImage);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    AddMenuWidget(panel, screen, 75, 267, 9, 1, 0x6a);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = AddMenuWidget(panel, screen, 79, 256, 1, 1, 0x6b);
    widget->SetUpdateCallback(AudioOptionsUI_UpdateVolumeLevelIndicatorPos);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = AddMenuWidget(panel, screen, 339, 90, 0x102001, 1, -1);
    widget->SetUpdateCallback(DrawGameplaySliderA);
    SetMenuWidgetLabel(widget, 0x2e, &g_rcMenuRightLabel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = AddMenuWidget(panel, screen, 339, 128, 0x102001, 1, -1);
    widget->SetUpdateCallback(DrawGameplaySliderB);
    SetMenuWidgetLabel(widget, 0x2f, &g_rcMenuRightLabel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    return 1;
}

static inline int CreateGameUI_OnHost(MenuWidget *) {
    return ShowHostRaceSetupScreen();
}

int BuildCreateGameMenuUI(MenuScreen *screen) {
    screen->SetCurrentPanelIndex(0);
    ResetSessionSlotTable();
    screen->SetPanelActive(screen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    screen->SetPanel(screen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    screen->SetPanelCount(screen->GetPanelCount() + 1);
    panel->SetScreen(screen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    AddMenuWidget(panel, screen, 0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(
        AddMenuWidget(panel, screen, panel->GetX(), 10, 0x100009, 1, -1), 0x62, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget =
        AddMenuWidget(panel, screen, panel->GetX(), panel->GetY(), 0x100009, 1, -1);
    widget->SetSpriteId(0x34);
    widget->SetText(GetStringResource(0x63), 1, 4, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    AddMenuWidget(panel, screen, 156, 172, 9, 0, 57);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = AddMenuWidget(panel, screen, panel->GetX(), 174, 0x200001, 1, -1);
    widget->SetActionCallback(CreateGameMenuUI_OnToggleTextEdit);
    widget->SetUpdateCallback(OnSessionNameEdited);
    widget->SetTextEditCompleteCallback(CreateGameUI_EnableContinueIfNameEntered);
    widget->SetSpriteId(3);
    widget->SetText(g_szEmptyUiString, 1, 1, &g_rcMenuCenterPanel);
    MenuFont *font = &widget->GetFont(0);
    font->SetText(std::strlen(g_szSessionName) <= 1 ? GetStringResource(0xc1) : g_szSessionName);
    font->MeasureText();
    widget->SetBoundsLeft(font->SetBoundsLeft(197));
    widget->SetBoundsRight(font->SetBoundsRight(309));
    widget->SetBoundsTop(font->SetBoundsTop(172));
    widget->SetBoundsBottom(font->SetBoundsBottom(188));
    font->SetInputMaxWidth(static_cast<short>(static_cast<unsigned short>(font->GetBoundsRight()) -
                                              static_cast<unsigned short>(font->GetBoundsLeft()) -
                                              16));
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    widget = AddMenuWidget(panel,
                           screen,
                           panel->GetX(),
                           static_cast<float>(static_cast<long double>(panel->GetY()) + 95),
                           0x102001,
                           1,
                           -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(CreateGameUI_OnHost);
    SetMenuWidgetLabel(widget, 0x1e, &g_rcMenuCenterPanel);
    g_pCreateGameContinueItem = widget;
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    widget = AddMenuWidget(panel,
                           screen,
                           panel->GetX(),
                           static_cast<float>(static_cast<long double>(panel->GetY()) + 114),
                           0x102001,
                           1,
                           -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(CreateGameMenuUI_OnBack);
    SetMenuWidgetLabel(widget, 0xc, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    AddMenuWidget(panel, screen, 80, 46, 9, 0, 7);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    return 1;
}

int BuildPlayerNameEntryUI(MenuScreen *screen) {
    screen->SetCurrentPanelIndex(0);
    screen->SetPanelActive(screen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    screen->SetPanel(screen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    screen->SetPanelCount(screen->GetPanelCount() + 1);
    panel->SetScreen(screen);
    panel->SetFlags(0);
    panel->SetX(190);
    panel->SetY(130);
    AddMenuWidget(panel, screen, 0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(
        AddMenuWidget(panel, screen, panel->GetX(), 10, 0x100009, 1, -1), 0x5c, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget =
        AddMenuWidget(panel, screen, panel->GetX(), panel->GetY(), 0x100009, 1, -1);
    widget->SetSpriteId(0x34);
    widget->SetText(GetStringResource(0x3c), 1, 4, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    AddMenuWidget(panel, screen, 156, 172, 9, 0, 57);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = AddMenuWidget(panel, screen, panel->GetX(), 174, 0x200001, 1, -1);
    widget->SetUpdateCallback(OnPlayerNameEdited);
    widget->SetActionCallback(PlayerNameEntryUI_OnToggleTextEdit);
    widget->SetTextEditCompleteCallback(PlayerNameEntryUI_EnableContinueIfNameEntered);
    widget->SetSpriteId(3);
    widget->SetText(g_szEmptyUiString, 1, 1, &g_rcMenuCenterPanel);
    if (strlen(g_GlobalRallyConfig.szPlayerName) <= 1) {
        strcpy(g_GlobalRallyConfig.szPlayerName, GetStringResource(0xc0));
    }
    MenuFont *font = &widget->GetFont(0);
    font->SetText(g_GlobalRallyConfig.szPlayerName);
    font->MeasureText();
    widget->SetBoundsLeft(font->SetBoundsLeft(186));
    widget->SetBoundsRight(font->SetBoundsRight(316));
    widget->SetBoundsTop(font->SetBoundsTop(172));
    widget->SetBoundsBottom(font->SetBoundsBottom(188));
    font->SetInputMaxWidth(static_cast<short>(static_cast<unsigned short>(font->GetBoundsRight()) -
                                              static_cast<unsigned short>(font->GetBoundsLeft()) -
                                              16));
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    widget = AddMenuWidget(panel,
                           screen,
                           panel->GetX(),
                           static_cast<float>(static_cast<long double>(panel->GetY()) + 95),
                           0x102001,
                           1,
                           -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(ShowMultiplayerConnectionScreen);
    SetMenuWidgetLabel(widget, 0x1e, &g_rcMenuCenterPanel);
    g_pPlayerNameContinueItem = widget;
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    widget = AddMenuWidget(panel,
                           screen,
                           panel->GetX(),
                           static_cast<float>(static_cast<long double>(panel->GetY()) + 114),
                           0x102001,
                           1,
                           -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(PlayerNameEntryUI_OnBack);
    SetMenuWidgetLabel(widget, 0xc, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    AddMenuWidget(panel, screen, 80, 46, 9, 0, 7);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    return 1;
}

void BuildNetworkCarSetupUI(MenuScreen *pScreen) {
    pScreen->SetCurrentPanelIndex(0);
    g_dwGameMode = 6;
    BackupMenuSelections();
    ResolveRaceSelections();
    pScreen->SetPanelActive(pScreen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    pScreen->SetPanel(pScreen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    pScreen->SetPanelCount(pScreen->GetPanelCount() + 1);
    panel->SetScreen(pScreen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(111);
    auto add = [&](float x, float y, unsigned flags, int mode, short image) {
        return AddMenuWidget(panel, pScreen, x, y, flags, mode, image);
    };
    add(0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    add(0, 320, 9, 0, 95);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    int (*actions[5])(MenuWidget *) = {CarSetupUI_OnCycleCar,
                                       CarSetupUI_OnCycleTireType,
                                       CarSetupUI_OnCycleSuspensionType,
                                       CarSetupUI_OnCycleHandlingType,
                                       CarSetupUI_OnCycleTransmissionType};
    for (int i = 0; i < 5; ++i) {
        MenuWidget *widget = add(panel->GetX(), panel->GetY() + 19 * i, 0x102001, 1, -1);
        widget->SetIdleActionCallback(ValidateEntityState);
        widget->SetActionCallback(actions[i]);
        SetMenuWidgetLabel(widget, 0x14 + i, &g_rcMenuCenterPanel);
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    }
    if (g_dwNetworkPlayMode == 2) {
        MenuWidget *widget = add(panel->GetX(), panel->GetY() + 95, 0x102001, 1, -1);
        widget->SetIdleActionCallback(ValidateEntityState);
        widget->SetActionCallback(NetworkCarSetupUI_OnBootPlayer);
        SetMenuWidgetLabel(widget, 0x68, &g_rcMenuCenterPanel);
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    }
    MenuWidget *widget = add(panel->GetX(), panel->GetY() + 114, 0x102001, 1, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(NetworkCarSetupUI_OnRefreshSessionDesc);
    widget->SetAfterUpdateCallback(NetworkCarSetupUI_OnEscape);
    SetMenuWidgetLabel(widget, g_dwLobbyIsHost ? 0x19 : 0x69, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    widget = add(panel->GetX(), panel->GetY() + 133, 0x102001, 1, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(ReturnToMultiplayerLobby);
    SetMenuWidgetLabel(widget, 0xc, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    widget = add(32, 330, 0x3001, 1, -1);
    widget->SetUpdateCallback(NetworkCarSetupUI_PollChatListSelection);
    widget->GetItemList().SetVerticalScrollbar(1);
    MenuItemList *list = &widget->GetItemList();
    list->Initialize(0x1840001, &g_rcNetCarSetupPlayerList, 5, 0, -1);
    g_pNetworkCarSetupMenu = widget;
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(32, 330, 0x200001, 1, -1);
    widget->SetActionCallback(CompleteConnectionTextEdit);
    widget->SetUpdateCallback(OnChatMessageEdited);
    widget->SetTextEditCompleteCallback(NetworkCarSetupUI_SendChatMessage);
    widget->SetSpriteId(3);
    widget->SetText(g_szEmptyUiString, 0, 1, &g_rcMenuCenterPanel);
    MenuFont *font = &widget->GetFont(0);
    widget->SetBoundsLeft(font->SetBoundsLeft(32));
    widget->SetBoundsRight(font->SetBoundsRight(415));
    widget->SetBoundsTop(font->SetBoundsTop(330));
    widget->SetBoundsBottom(font->SetBoundsBottom(346));
    font->SetInputMaxWidth(367);
    g_dwMenuTextEditActive = 0;
    g_dwInNetworkCarSetup = 1;
    font->SetDrawCaret(1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(484, 2, 0x1001, 1, -1);
    widget->SetUpdateCallback(NetworkCarSetupUI_UpdateSelectedPlayer);
    widget->GetItemList().SetVerticalScrollbar(1);
    list = &widget->GetItemList();
    list->Initialize(
        g_dwNetworkPlayMode == 2 ? 0x2040001 : 0x3040001, &g_rcMenuNetworkPlayers, 4, 0, -1);
    g_pNetworkPlayerListMenu = widget;
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    add(61, 244, 9, 1, 54);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(73, 214, 1, 1, 53);
    widget->SetUpdateCallback(CarSetupUI_UpdateSteeringIndicatorPos);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(339, 70, 1, 1, 11);
    widget->SetUpdateCallback(CarSetupUI_UpdateCarImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), 137, 0x101001, 1, -1);
    widget->SetUpdateCallback(CarSetupUI_UpdateCarNameLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuRightLabel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(108, 66, 1, 1, 25);
    widget->SetUpdateCallback(CarSetupUI_UpdateTiresImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), 123, 0x101001, 1, -1);
    widget->SetUpdateCallback(CarSetupUI_UpdateTiresLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuLeftColumnNarrow);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(476, 129, 1, 1, 14);
    widget->SetUpdateCallback(CarSetupUI_UpdateShocksImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), 181, 0x101001, 1, -1);
    widget->SetUpdateCallback(CarSetupUI_UpdateShocksLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuRightColumn);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(505, 200, 1, 1, 12);
    widget->SetUpdateCallback(CarSetupUI_UpdateTransmissionImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), 262, 0x101001, 1, -1);
    widget->SetUpdateCallback(CarSetupUI_UpdateTransmissionLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuRightColumnWide);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(320, 10, 0x100001, 1, -1);
    widget->SetSpriteId(0x34);
    widget->SetText(g_szSpace, 1, 4, &g_rcMenuFullScreen);
    font = &widget->GetFont(0);

    font->SetText(g_szSessionName);
    font->MeasureText();
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
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
        widget->SetIdleActionCallback(ValidateEntityState);
        widget->SetActionCallback(actions[i]);
        widget->SetBoundsTop(static_cast<int>(y));
        if (i == 0) {
            widget->SetBoundsLeft(static_cast<int>(x));
            right = widget->GetBoundsLeft() + 127;
        } else {
            widget->SetBoundsLeft(static_cast<int>(x));
        }
        widget->SetBoundsRight(right);
        widget->SetBoundsBottom(widget->GetBoundsTop() + 33);
        widget->SetImageMode(0);
        widget->SetAnimationSpriteId(0, images[i] + 1);
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        y += 33;
    }
}

void BuildRaceResultsTotalsUI(MenuScreen *pScreen) {
    pScreen->SetCurrentPanelIndex(0);
    pScreen->SetPanelActive(pScreen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    pScreen->SetPanel(pScreen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    pScreen->SetPanelCount(pScreen->GetPanelCount() + 1);
    panel->SetScreen(pScreen);
    panel->SetBeforeWidgetsCallback(RaceResultsPanel_PreUpdateSelectLastRace);
    panel->SetAfterWidgetsCallback(RaceResultsPanel_PostUpdateRestoreSeasonProgress);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    auto add = [&](float x, float y, unsigned flags, int mode, short image) {
        return AddMenuWidget(panel, pScreen, x, y, flags, mode, image);
    };
    add(0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(panel->GetX(), 10, 0x100009, 1, -1), 0x4f, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget = add(panel->GetX(), panel->GetY() + 95, 0x102001, 1, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(RaceResultsTotalsUI_OnContinue);
    SetMenuWidgetLabel(widget, 0x50, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    widget = add(panel->GetX(), panel->GetY() + 114, 0x102001, 1, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(RaceResultsTotalsUI_OnBack);
    SetMenuWidgetLabel(widget, 0xc, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    AddMenuActionButtons(
        panel, pScreen, RaceResultsTotalsUI_OnOpenSaveSeason, RaceResultsTotalsUI_OnOpenOptions);
    SetMenuWidgetLabel(add(106, 85, 0x100001, 1, -1), 0x38, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(440, 66, 0x5001, 1, -1);
    widget->SetUpdateCallback(SeasonUI_UpdateSeasonNameLabel);
    widget->SetSpriteId(0x34);
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(106, 123, 0x100001, 1, -1), 0x36, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(440, 104, 0x5001, 1, -1);
    widget->SetUpdateCallback(SeasonUI_UpdateYearOrdinalLabel);
    widget->SetSpriteId(0x34);
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(0, 70, 0x100009, 1, -1), 0x52, &g_rcMenuRightLabel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(330, 97, 0x5001, 1, -1);
    widget->SetUpdateCallback(RaceResultsTotalsUI_UpdateRoundsCompletedLabel);
    widget->SetSpriteId(5);
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightLabel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(0, 150, 0x100009, 1, -1), 0x53, &g_rcMenuRightLabel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(450, 125, 0x100009, 1, -1), 0x40, &g_rcMenuRightListBox);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(450, 185, 0x100009, 1, -1), 0x46, &g_rcMenuRightListBox);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(450, 141, 0x5001, 1, -1);
    widget->SetUpdateCallback(RaceResultsTotalsUI_UpdatePointsNeededLabel);
    widget->SetSpriteId(5);
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightListBox);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(panel->GetX(), 203, 0x100009, 1, -1), 0x40, &g_rcMenuRightColumnWide);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(panel->GetX(), 265, 0x100009, 1, -1), 0x41, &g_rcMenuRightColumnWide);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(450, 217, 0x5001, 1, -1);
    widget->SetUpdateCallback(RaceResultsTotalsUI_UpdateTotalPointsLabel);
    widget->SetSpriteId(5);
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightColumnWide);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
}

void BuildRaceResultsInfoUI(MenuScreen *pScreen) {
    pScreen->SetCurrentPanelIndex(0);
    g_dwSeasonSavedRound = g_dwSeasonRound;
    pScreen->SetPanelActive(pScreen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    pScreen->SetPanel(pScreen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    pScreen->SetPanelCount(pScreen->GetPanelCount() + 1);
    panel->SetScreen(pScreen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    panel->SetBeforeWidgetsCallback(RaceResultsPanel_PreUpdateSelectLastRace);
    panel->SetAfterWidgetsCallback(RaceResultsPanel_PostUpdateRestoreSeasonProgress);
    auto add = [&](float x, float y, unsigned flags, int mode, short image) {
        return AddMenuWidget(panel, pScreen, x, y, flags, mode, image);
    };
    add(0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(panel->GetX(), 10, 0x100009, 1, -1), 0x47, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    int (*actions[5])(MenuWidget *) = {RaceResultsInfoUI_OnSelectLap1,
                                       RaceResultsInfoUI_OnSelectLap2,
                                       RaceResultsInfoUI_OnSelectLap3,
                                       RaceResultsInfoUI_OnSelectTotalTime,
                                       ShowRaceResultsSummaryScreen};
    const float steps[5] = {0, 19, 38, 57, 95};
    for (int i = 0; i < 5; ++i) {
        MenuWidget *widget = add(panel->GetX(), panel->GetY() + steps[i], 0x102001, 1, -1);
        widget->SetIdleActionCallback(ValidateEntityState);
        widget->SetActionCallback(actions[i]);
        SetMenuWidgetLabel(widget, i == 4 ? 0x1e : 0x48 + i, &g_rcMenuCenterPanel);
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    }
    AddMenuActionButtons(
        panel, pScreen, RaceResultsInfoUI_OnOpenSaveSeason, RaceResultsInfoUI_OnOpenOptions);
    SetMenuWidgetLabel(add(106, 123, 0x100001, 1, -1), 0x4c, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget = add(440, 79, 0x5001, 1, -1);
    widget->SetUpdateCallback(SeasonUI_UpdateRoundPlaceLabel);
    widget->SetSpriteId(5);
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(73, 212, 1, 1, 22);
    widget->SetUpdateCallback(RaceSetupUI_UpdateWeatherImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), 275, 0x101001, 1, -1);
    widget->SetUpdateCallback(RaceSetupUI_UpdateWeatherLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuLeftColumn);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(325, 72, 1, 1, 17);
    widget->SetUpdateCallback(RaceSetupUI_UpdateTrackImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), 152, 0x101001, 1, -1);
    widget->SetUpdateCallback(RaceSetupUI_UpdateTrackNameLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuRightColumnMid);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(450, 185, 0x100009, 1, -1), 0x4d, &g_rcMenuRightListBox);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(450, 166, 0x5001, 1, -1);
    widget->SetUpdateCallback(RaceResultsInfoUI_UpdateSelectedTimeLabel);
    widget->SetSpriteId(0x34);
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuRightListBox);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(panel->GetX(), 258, 0x100009, 1, -1), 0x40, &g_rcMenuRightColumnWide);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(450, 211, 0x5001, 1, -1);
    widget->SetUpdateCallback(SeasonUI_UpdateRoundPointsLabel);
    widget->SetSpriteId(5);
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightColumnWide);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
}

void BuildRaceResultsSummaryUI(MenuScreen *pScreen) {
    pScreen->SetCurrentPanelIndex(0);
    g_dwSeasonSavedRound = g_dwSeasonRound;
    pScreen->SetPanelActive(pScreen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    pScreen->SetPanel(pScreen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    pScreen->SetPanelCount(pScreen->GetPanelCount() + 1);
    panel->SetScreen(pScreen);
    panel->SetBeforeWidgetsCallback(RaceResultsPanel_PreUpdateSelectLastRace);
    panel->SetAfterWidgetsCallback(RaceResultsPanel_PostUpdateRestoreSeasonProgress);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    auto add = [&](float x, float y, unsigned flags, int mode, short image) {
        return AddMenuWidget(panel, pScreen, x, y, flags, mode, image);
    };
    add(0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(panel->GetX(), 10, 0x100009, 1, -1), 0x4e, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget = add(panel->GetX(), panel->GetY(), 0x3001, 1, -1);
    MenuItemList *list = &widget->GetItemList();

    widget->SetUpdateCallback(SeasonRoundList_UpdateRoundSelection);
    list->SetVerticalScrollbar(1);
    list->Initialize(0x200000, &g_rcMenuListBox, 5, 0, -1);
    widget->SetBeforeUpdateCallback(SeasonRoundList_SetBrowsingFlag);
    widget->SetAfterUpdateCallback(SeasonRoundList_ClearBrowsingFlag);
    if (g_SeasonTable[g_bSeasonClass].raceCount > 0) {
        int round = 0;
        do {
            char label[32];
            char number[32];
            strcpy(label, GetStringResource(0x37));
            strcat(label, g_szSpace);
            ++round;
            strcat(label, _strupr(_itoa(round, number, 10)));
            list->AddItem(label, 1, 1, &g_rcMenuCenterPanel, true);
        } while (round < g_SeasonTable[g_bSeasonClass].raceCount);
    }
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    widget = add(panel->GetX(), panel->GetY() + 95, 0x102001, 1, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(ShowRaceResultsTotalsScreen);
    SetMenuWidgetLabel(widget, 0x1e, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    widget = add(panel->GetX(), panel->GetY() + 114, 0x102001, 1, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(RaceResultsSummaryUI_OnBack);
    SetMenuWidgetLabel(widget, 0xc, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    AddMenuActionButtons(
        panel, pScreen, RaceResultsSummaryUI_OnOpenSaveSeason, RaceResultsSummaryUI_OnOpenOptions);
    SetMenuWidgetLabel(add(106, 85, 0x100001, 1, -1), 0x38, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(440, 66, 0x5001, 1, -1);
    widget->SetUpdateCallback(SeasonUI_UpdateSeasonNameLabel);
    widget->SetSpriteId(0x34);
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(106, 123, 0x100001, 1, -1), 0x36, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(440, 104, 0x5001, 1, -1);
    widget->SetUpdateCallback(SeasonUI_UpdateYearOrdinalLabel);
    widget->SetSpriteId(0x34);
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(106, 185, 0x100001, 1, -1), 0x4c, &g_rcMenuRightListBox);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(440, 141, 0x5001, 1, -1);
    widget->SetUpdateCallback(SeasonUI_UpdateRoundPlaceLabel);
    widget->SetSpriteId(5);
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightListBox);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(73, 212, 1, 1, 22);
    widget->SetUpdateCallback(RaceSetupUI_UpdateWeatherImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), 275, 0x101001, 1, -1);
    widget->SetUpdateCallback(RaceSetupUI_UpdateWeatherLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuLeftColumn);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(325, 72, 1, 1, 17);
    widget->SetUpdateCallback(RaceSetupUI_UpdateTrackImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), 152, 0x101001, 1, -1);
    widget->SetUpdateCallback(RaceSetupUI_UpdateTrackNameLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuRightColumnMid);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(panel->GetX(), 258, 0x100009, 1, -1), 0x40, &g_rcMenuRightColumnWide);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(450, 211, 0x5001, 1, -1);
    widget->SetUpdateCallback(SeasonUI_UpdateRoundPointsLabel);
    widget->SetSpriteId(5);
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightColumnWide);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
}

void BuildSeasonProgressInfoUI(MenuScreen *pScreen) {
    pScreen->SetCurrentPanelIndex(0);
    pScreen->SetPanelActive(pScreen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    pScreen->SetPanel(pScreen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    pScreen->SetPanelCount(pScreen->GetPanelCount() + 1);
    panel->SetScreen(pScreen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    auto add = [&](float x, float y, unsigned flags, int mode, short image) {
        return AddMenuWidget(panel, pScreen, x, y, flags, mode, image);
    };
    add(0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(panel->GetX(), 10, 0x100009, 1, -1), 0x44, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget = add(panel->GetX(), panel->GetY() + 19, 0x102001, 1, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(SeasonProgressInfoUI_OnResetRound);
    SetMenuWidgetLabel(widget, 0x45, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    widget = add(panel->GetX(), panel->GetY() + 95, 0x102001, 1, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(SeasonProgressInfoUI_OnCarSetup);
    widget->SetSpriteId(2);
    widget->SetText(GetStringResource(0x1e), 1, 0, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    widget = add(panel->GetX(), panel->GetY() + 114, 0x102001, 1, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(SeasonProgressInfoUI_OnBack);
    SetMenuWidgetLabel(widget, 0xc, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    AddMenuActionButtons(
        panel, pScreen, SeasonProgressInfoUI_OnSaveSeason, SeasonProgressInfoUI_OnOptions);
    widget = add(73, 212, 1, 1, 22);
    widget->SetUpdateCallback(RaceSetupUI_UpdateWeatherImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), 275, 0x101001, 1, -1);
    widget->SetUpdateCallback(RaceSetupUI_UpdateWeatherLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuLeftColumn);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(325, 72, 1, 1, 17);
    widget->SetUpdateCallback(RaceSetupUI_UpdateTrackImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), 152, 0x101001, 1, -1);
    widget->SetUpdateCallback(RaceSetupUI_UpdateTrackNameLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuRightColumnMid);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(450, 125, 0x100009, 1, -1), 0x40, &g_rcMenuRightListBox);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(450, 185, 0x100009, 1, -1), 0x46, &g_rcMenuRightListBox);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(450, 141, 0x5001, 1, -1);
    widget->SetUpdateCallback(SeasonProgressInfoUI_UpdatePointsNeededLabel);
    widget->SetSpriteId(5);
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightListBox);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(panel->GetX(), 203, 0x100009, 1, -1), 0x40, &g_rcMenuRightColumnWide);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(panel->GetX(), 265, 0x100009, 1, -1), 0x41, &g_rcMenuRightColumnWide);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(450, 217, 0x5001, 1, -1);
    widget->SetUpdateCallback(SeasonUI_UpdateTotalPointsLabel);
    widget->SetSpriteId(5);
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightColumnWide);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(106, 85, 0x100001, 1, -1), 0x38, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(440, 66, 0x5001, 1, -1);
    widget->SetUpdateCallback(SeasonUI_UpdateSeasonNameLabel);
    widget->SetSpriteId(0x34);
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(106, 123, 0x100001, 1, -1), 0x36, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(440, 104, 0x5001, 1, -1);
    widget->SetUpdateCallback(SeasonUI_UpdateYearOrdinalLabel);
    widget->SetSpriteId(0x34);
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), panel->GetY() - 19, 0x5001, 1, -1);
    widget->SetUpdateCallback(SeasonProgressInfoUI_UpdateRoundLabel);
    widget->SetSpriteId(0x34);
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
}

void BuildMultiplayerConnectionUI(MenuScreen *pScreen) {
    pScreen->SetCurrentPanelIndex(0);
    g_dwGameMode = 6;
    pScreen->SetPanelActive(pScreen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    pScreen->SetPanel(pScreen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    pScreen->SetPanelCount(pScreen->GetPanelCount() + 1);
    panel->SetScreen(pScreen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    auto add = [&](float x, float y, unsigned flags, int mode, short image) {
        return AddMenuWidget(panel, pScreen, x, y, flags, mode, image);
    };
    add(0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(panel->GetX(), 10, 0x100009, 1, -1), 0x54, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    int (*actions[3])(MenuWidget *) = {MultiplayerConnectionUI_OnSelectIpx,
                                       MultiplayerConnectionUI_OnSelectTcpIp,
                                       MultiplayerConnectionUI_OnSelectModem};
    int (*updates[3])(MenuWidget *) = {MultiplayerConnectionUI_UpdateIpxItem,
                                       MultiplayerConnectionUI_UpdateTcpIpItem,
                                       MultiplayerConnectionUI_UpdateModemItem};
    for (int i = 0; i < 3; ++i) {
        MenuWidget *widget = add(panel->GetX(), panel->GetY() + 19 * i, 0x102001, 1, -1);
        widget->SetIdleActionCallback(updates[i]);
        widget->SetActionCallback(actions[i]);
        SetMenuWidgetLabel(widget, 0x55 + i, &g_rcMenuCenterPanel);
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    }
    MenuWidget *widget = add(panel->GetX(), panel->GetY() + 114, 0x102001, 1, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(MultiplayerConnectionUI_OnBack);
    SetMenuWidgetLabel(widget, 0xc, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    add(95, 317, 9, 0, 103);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(130, 336, 0x100009, 1, -1);
    widget->SetSpriteId(0x34);
    widget->SetText(GetStringResource(0x59), 1, 4, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(130, 374, 0x100009, 1, -1), 0x5a, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    add(155, 393, 9, 0, 57);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    for (int i = 0; i < 2; ++i) {
        int top = 393 + 38 * i;
        widget = add(155, static_cast<float>(top), 0x200001, 1, -1);
        widget->SetActionCallback(ConnectionUI_OnToggleTextEdit);
        widget->SetUpdateCallback(i == 0 ? OnTcpIpPortEdited : OnTcpIpAddressEdited);
        widget->SetTextEditCompleteCallback(CompleteConnectionTextEdit);
        widget->SetSpriteId(3);
        widget->SetText(g_szEmptyUiString, 1, 1, &g_rcMenuCenterPanel);
        MenuFont *font = &widget->GetFont(0);
        font->MeasureText();
        widget->SetBoundsLeft(font->SetBoundsLeft(155));
        widget->SetBoundsRight(font->SetBoundsRight(347));
        widget->SetBoundsTop(font->SetBoundsTop(top));
        widget->SetBoundsBottom(font->SetBoundsBottom(top + 16));
        font->SetInputMaxWidth(
            static_cast<short>(static_cast<unsigned short>(font->GetBoundsRight()) -
                               static_cast<unsigned short>(font->GetBoundsLeft()) - 16));
        panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        panel->SetNavigationCount(panel->GetNavigationCount() + 1);
        if (i == 0) {
            SetMenuWidgetLabel(add(130, 412, 0x100009, 1, -1), 0x5b, &g_rcMenuCenterPanel);
            panel->SetWidgetCount(panel->GetWidgetCount() + 1);
            add(155, 431, 9, 0, 57);
            panel->SetWidgetCount(panel->GetWidgetCount() + 1);
        }
    }
    add(80, 46, 9, 0, 7);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(339, 61, 1, 1, 69);
    widget->SetUpdateCallback(ConnectionUI_UpdateServiceProviderImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), 160, 0x101001, 1, -1);
    widget->SetUpdateCallback(ConnectionUI_UpdateServiceProviderLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuRightColumnMid);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
}

void BuildMultiplayerGameSetupUI(MenuScreen *pScreen) {
    pScreen->SetCurrentPanelIndex(0);
    pScreen->SetPanelActive(pScreen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    pScreen->SetPanel(pScreen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    pScreen->SetPanelCount(pScreen->GetPanelCount() + 1);
    panel->SetScreen(pScreen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    auto add = [&](float x, float y, unsigned flags, int mode, short image) {
        return AddMenuWidget(panel, pScreen, x, y, flags, mode, image);
    };
    add(0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(panel->GetX(), 10, 0x100009, 1, -1), 0x64, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    if (g_dwNetworkPlayMode == 2) {
        int (*actions[4])(MenuWidget *) = {RaceSetupUI_OnCycleTrack,
                                           RaceSetupUI_OnCycleWeather,
                                           RaceSetupUI_OnCycleLapCount,
                                           MultiplayerGameSetupUI_OnCycleCarClass};
        for (int i = 0; i < 4; ++i) {
            MenuWidget *widget = add(panel->GetX(), panel->GetY() + 19 * i, 0x102001, 1, -1);
            widget->SetIdleActionCallback(i == 0 ? ValidateEntityStateAlwaysTrue :
                                                   ValidateEntityState);
            widget->SetActionCallback(actions[i]);
            SetMenuWidgetLabel(widget, i == 3 ? 0x65 : 0x1b + i, &g_rcMenuCenterPanel);
            panel->SetWidgetCount(panel->GetWidgetCount() + 1);
            panel->SetNavigationCount(panel->GetNavigationCount() + 1);
        }
    }
    MenuWidget *widget = add(panel->GetX(), panel->GetY() + 95, 0x102001, 1, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(ShowNetworkCarSetupScreen);
    widget->SetAfterUpdateCallback(MultiplayerGameSetupUI_OnEscape);
    SetMenuWidgetLabel(widget, g_dwLobbyIsHost ? 0x66 : 0x1e, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    widget = add(panel->GetX(), panel->GetY() + 114, 0x102001, 1, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    SetMenuWidgetLabel(widget, g_dwLaunchedFromLobby ? 0x67 : 0xc, &g_rcMenuCenterPanel);
    g_pMultiplayerGameSetupMenu = widget;
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    widget = add(73, 212, 1, 1, 22);
    widget->SetUpdateCallback(RaceSetupUI_UpdateWeatherImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), 275, 0x101001, 1, -1);
    widget->SetUpdateCallback(RaceSetupUI_UpdateWeatherLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuLeftColumn);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(325, 72, 1, 1, 17);
    widget->SetUpdateCallback(RaceSetupUI_UpdateTrackImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), 152, 0x101001, 1, -1);
    widget->SetUpdateCallback(RaceSetupUI_UpdateTrackNameLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuRightColumnMid);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(106, 68, 0x5001, 1, -1);
    widget->SetUpdateCallback(RaceSetupUI_UpdateLapCountLabel);
    widget->SetSpriteId(5);
    widget->SetText(g_szMenuDigitOne, 1, 3, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    SetMenuWidgetLabel(add(106, 115, 0x100001, 1, -1), 0x1d, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(437, 141, 1, 1, 86);
    widget->SetUpdateCallback(MultiplayerSetupUI_UpdateCarGroupImage);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(476, 224, 1, 1, -1);
    widget->SetUpdateCallback(MultiplayerSetupUI_UpdateCarGroupSubImage);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
}

void BuildPreRaceInfoUI(MenuScreen *pScreen) {
    pScreen->SetCurrentPanelIndex(0);
    pScreen->SetPanelActive(pScreen->GetPanelCount(), 1);
    MenuPanel *panel = new (std::nothrow) MenuPanel();
    pScreen->SetPanel(pScreen->GetPanelCount(), panel);
    if (!panel) {
        ShowErrorMessage(4);
    }
    pScreen->SetPanelCount(pScreen->GetPanelCount() + 1);
    panel->SetScreen(pScreen);
    panel->SetFlags(0);
    panel->SetX(195);
    panel->SetY(130);
    auto add = [&](float x, float y, unsigned flags, int mode, short image) {
        return AddMenuWidget(panel, pScreen, x, y, flags, mode, image);
    };
    auto text = [](MenuWidget *widget, int stringId, const RECT *rect) {
        SetMenuWidgetLabel(widget, stringId, rect);
    };
    add(0, 0, 9, 0, 0);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    MenuWidget *widget = add(panel->GetX(), 10, 0x100009, 1, -1);
    text(widget, 0x3d, &g_rcMenuTitleBar);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), panel->GetY(), 0x3001, 1, -1);
    MenuItemList *list = &widget->GetItemList();
    widget->SetUpdateCallback(SeasonRoundList_UpdateRoundSelection);
    list->SetVerticalScrollbar(1);
    list->Initialize(0x240000, &g_rcMenuListBox, 5, 0, -1);
    widget->SetBeforeUpdateCallback(SeasonRoundList_SetBrowsingFlag);
    widget->SetAfterUpdateCallback(SeasonRoundList_ClearBrowsingFlag);
    if (g_SeasonTable[g_bSeasonClass].raceCount > 0) {

        for (int round = 0; round < g_SeasonTable[g_bSeasonClass].raceCount; ++round) {
            char number[32];
            char label[32];
            _itoa(round + 1, number, 10);
            sprintf(label, GetStringResource(0xbf), _strupr(number));
            list->AddItem(label, 1, 1, &g_rcMenuCenterPanel, true);
        }
    }
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), panel->GetY() + 95, 0x102001, 1, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(ShowSeasonProgressInfoScreen);
    widget->SetSpriteId(2);
    widget->SetText(GetStringResource(0x1e), 1, 0, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    widget = add(panel->GetX(), panel->GetY() + 114, 0x102001, 1, -1);
    widget->SetIdleActionCallback(ValidateEntityState);
    widget->SetActionCallback(PreRaceInfoUI_OnBack);
    widget->SetBeforeUpdateCallback(PreRaceInfoUI_UpdateBackButtonText);
    widget->SetSpriteId(3);
    widget->SetText(GetStringResource(0xc), 1, 1, &g_rcMenuCenterPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    panel->SetNavigationCount(panel->GetNavigationCount() + 1);
    AddMenuActionButtons(panel, pScreen, PreRaceInfoUI_OnSaveSeason, PreRaceInfoUI_OnOptions);
    widget = add(73, 212, 1, 1, 22);
    widget->SetUpdateCallback(RaceSetupUI_UpdateWeatherImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), 275, 0x101001, 1, -1);
    widget->SetUpdateCallback(RaceSetupUI_UpdateWeatherLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuLeftColumn);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(325, 72, 1, 1, 17);
    widget->SetUpdateCallback(RaceSetupUI_UpdateTrackImage);
    widget->SetLinkedWidgetCount(widget->GetLinkedWidgetCount() + 1);
    widget->SetLinkedWidgetIndex(0, panel->GetWidgetCount() + 1);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(panel->GetX(), 152, 0x101001, 1, -1);
    widget->SetUpdateCallback(RaceSetupUI_UpdateTrackNameLabel);
    widget->SetSpriteId(3);
    widget->SetText(g_szSpace, 1, 1, &g_rcMenuRightColumnMid);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    text(add(450, 147, 0x100009, 1, -1), 0x3e, &g_rcMenuRightListBox);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(450, 128, 0x5001, 1, -1);
    widget->SetUpdateCallback(PreRaceInfoUI_UpdateBestTimeLabel);
    widget->SetSpriteId(0x34);
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuRightListBox);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    text(add(450, 185, 0x100009, 1, -1), 0x3f, &g_rcMenuRightListBox);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(450, 166, 0x5001, 1, -1);
    widget->SetUpdateCallback(PreRaceInfoUI_UpdateBestLapLabel);
    widget->SetSpriteId(0x34);
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuRightListBox);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    text(add(panel->GetX(), 203, 0x100009, 1, -1), 0x40, &g_rcMenuRightColumnWide);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    text(add(panel->GetX(), 265, 0x100009, 1, -1), 0x41, &g_rcMenuRightColumnWide);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(450, 217, 0x5001, 1, -1);
    widget->SetUpdateCallback(SeasonUI_UpdateTotalPointsLabel);
    widget->SetSpriteId(5);
    widget->SetText(g_szEmptyUiString, 1, 3, &g_rcMenuRightColumnWide);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    text(add(106, 85, 0x100001, 1, -1), 0x38, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(440, 66, 0x5001, 1, -1);
    widget->SetUpdateCallback(SeasonUI_UpdateSeasonNameLabel);
    widget->SetSpriteId(0x34);
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    text(add(106, 123, 0x100001, 1, -1), 0x36, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
    widget = add(440, 104, 0x5001, 1, -1);
    widget->SetUpdateCallback(SeasonUI_UpdateYearOrdinalLabel);
    widget->SetSpriteId(0x34);
    widget->SetText(g_szEmptyUiString, 1, 4, &g_rcMenuSubPanel);
    panel->SetWidgetCount(panel->GetWidgetCount() + 1);
}

int ShowPreRaceInfoScreen(void) {
    return ShowCachedMenuWithExitConfirmation(g_pCachedMenu_PreRaceInfo,
                                              BuildPreRaceInfoUI,
                                              g_pPreRaceInfoMenuScreen,
                                              BuildPreRaceExitSeasonConfirmScreen);
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
    pItem->GetScreen()->DestroyPanels();
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

        for (int index = 0; index < MENU_PANEL_WIDGET_COUNT; ++index) {
            delete pPanel->GetWidget(index);
            pPanel->SetWidget(index, nullptr);
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
s16 g_wVehicleDeformMaxDent = 0x100;

// The samples of the sounds of the cheats, and the levels of both sides they play at.
#define CHEAT_SOUND_SAMPLE 2
#define CHEAT_DEFORMATION_SOUND_SAMPLE 3
#define CHEAT_SOUND_LEVELS 0x200020
// The states of the network menus after a cheat.
#define CHEAT_NETWORK_MENU_STATE 2
#define CHEAT_DEFORMATION_NETWORK_MENU_STATE 3
// The maximum accumulated dent under the deformation cheat.
#define VEHICLE_DEFORM_CHEAT_MAXIMUM 0x7fff

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
    g_wVehicleDeformMaxDent = VEHICLE_DEFORM_CHEAT_MAXIMUM;
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
    if (VEHICLE_GET(&(g_aVehicles[0]), Lap) >= g_dwTotalLaps) {
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

int IsCarSelectable(int nCar) {
    if (g_dwCheatBonusCarsUnlocked && nCar > 15) {
        nCar -= 16;
    }
    if (g_dwSeasonLoaded && nCar > 15 && (g_SeasonProgress.unlocks & 0x8000)) {
        nCar -= 16;
    }
    unsigned available;
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        if (nCar == 15) {
            nCar = 11;
        }
        available = g_dwSeasonLoaded ? g_SeasonProgress.unlocks : g_dwUnlockedCarMask;
    } else if (g_dwGameMode == GAME_MODE_NETWORK) {
        if (nCar == 15) {
            nCar = 7;
        }
        available = g_dwSelectableCarMask;
    } else {
        if (g_dwGameMode == GAME_MODE_TIME_ATTACK && nCar == g_LoadedSeasonSettings.car) {
            return 1;
        }
        if (nCar == 15) {
            nCar = 11;
        }
        available = static_cast<unsigned>(static_cast<short>(g_wUnlockedCarModeMask));
    }
    if (g_dwCheatAllCarsUnlocked && nCar <= 15) {
        return 1;
    }
    return static_cast<int>((1u << (static_cast<unsigned>(nCar) & 31)) & available);
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
    SetGridRects(g_aMenuLargeIconRects + MENU_LARGE_ICON_RECT_COUNT,
                 MENU_LARGE_ICON_3_COLUMN_RECT_COUNT,
                 3,
                 128,
                 128);
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
    g_pRootMenuScreen->SetBuildCallback(BuildFrontEndRootMenuUI);
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
