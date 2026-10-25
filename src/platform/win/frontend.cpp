#include "platform/win/frontend.h"

#include <cstdio>
#include <cstring>
#include <new>

#include <windows.h>

#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/boot/strings.h"
#include "platform/win/directplay.h"
#include "platform/win/input.h"
#include "platform/win/settings.h"
#include "platform/win/sound.h"
#if TGR_RENDERER == TGR_RENDERER_D3D
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
    nUnknownC = 0;
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
    pMaterial->nUnknown58 = 1;
    pMaterial->nUnknown58 = 0;
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

#if TGR_RENDERER == TGR_RENDERER_D3D
void MenuScreen::FlipAndWait() {
    DirectDrawRenderer *pRenderer = g_pRenderHost->pRenderer;

    pRenderer->pPrimarySurface->Flip(pRenderer->pBackBuffer, DDFLIP_WAIT);
    while (pRenderer->pPrimarySurface->GetFlipStatus(DDGFS_ISFLIPDONE) != DD_OK) {
        Sleep(0);
    }
}
#endif

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
