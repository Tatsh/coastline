#include "platform/win/frontend.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include <windows.h>

#include "game.h"
#include "laps.h"
#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/boot/strings.h"
#include "platform/win/directplay.h"
#include "platform/win/input.h"
#include "platform/win/season.h"
#include "platform/win/settings.h"
#include "platform/win/sound.h"
#include "platform/win/timer.h"
#include "profile.h"
#include "trackselect.h"
#if TGR_RENDERER == TGR_RENDERER_D3D
#include "platform/win/d3d/ddutil.h"
#include "platform/win/d3d/renderer.h"
#endif

MenuScreen *g_pActiveMenuScreen;
void *g_pFrontEndScratchBuffer;
MenuImage g_aMenuImages[MENU_IMAGE_COUNT];
unsigned short g_wMenuImageLoadedCount;
unsigned short g_wUnusedFrontEndWord0;
unsigned short g_wUnusedFrontEndWord1;
FrontEndResourceSlot g_aFrontEndResourceSlots[FRONT_END_RESOURCE_SLOT_COUNT];
int g_adwFrontEndResourceFlags[FRONT_END_RESOURCE_FLAG_COUNT];
MenuScreen *g_pRootMenuScreen;
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
MenuWidget *g_pJoinCreateGameMenu;
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
MenuWidget *g_pSaveTimeAttackMenu;
MenuWidget *g_pSaveSeasonEditMenu;
MenuWidget *g_pSaveTimeAttackEditMenu;
MenuWidget *g_pNetworkSessionJoinMenu;
MenuWidget *g_pNetworkCarSetupMenu;
MenuWidget *g_pNetworkPlayerListMenu;
MenuWidget *g_pModemContinueItem;
MenuWidget *g_pDisplayModeSelectMenu;
MenuWidget *g_pD3DDeviceSelectMenu;
MenuWidget *g_pSaveSeasonBackHandlerMenu;

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

// Free a cached screen and report whether there was one.
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
    if (g_aVehicles_LapCount >= g_dwTotalLaps) {
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
