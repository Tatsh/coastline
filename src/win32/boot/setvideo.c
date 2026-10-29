#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include "ini.h"
#include "registry.h"
#include "win32/fileio.h"

typedef struct VideoSelection {
    int initialIndex;
    int selectedIndex;
    int method;
} VideoSelection;

// @ghidraAddress SetVideo: 0x0040b29c
static int g_bAlphaCompare = 1;
// @ghidraAddress SetVideo: 0x0040b2a0
static int g_bDrawCarShadow = 1;
// @ghidraAddress SetVideo: 0x00416bbc
static int g_bClearDepthBuffer;
// @ghidraAddress SetVideo: 0x00416bc0
static int g_bInverseSourceAlpha;
// @ghidraAddress SetVideo: 0x00416bc4
static int g_bReconfigureDisplay;
// @ghidraAddress SetVideo: 0x004169a0
static VideoSelection g_VideoSelection;
// @ghidraAddress SetVideo: 0x00416bb0
static LauncherIniFile *g_pVideoDatabase;
// @ghidraAddress SetVideo: 0x0040e998
static int g_dwVideoSectionCount;
// @ghidraAddress SetVideo: 0x0040e198
static char g_szIniFilePath[1024];

// @ghidraAddress SetVideo: 0x00401c10
static INT_PTR CALLBACK WelcomeDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_INITDIALOG) {
        SetWindowLongPtrA(window, DWLP_USER, lParam);
        return 1;
    }
    if (message == WM_COMMAND) {
        if (LOWORD(wParam) == IDOK) {
            EndDialog(window, 1);
        } else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(window, 0);
        }
    }
    return 0;
}

// @ghidraAddress SetVideo: 0x00401c70
static INT_PTR CALLBACK SymptomsDialogProc(HWND window,
                                           UINT message,
                                           WPARAM wParam,
                                           LPARAM lParam) {
    if (message == WM_INITDIALOG) {
        SetWindowLongPtrA(window, DWLP_USER, lParam);
        CheckDlgButton(window, 1006, !g_bAlphaCompare);
        CheckDlgButton(window, 1007, !g_bDrawCarShadow);
        CheckDlgButton(window, 1008, g_bInverseSourceAlpha != 0);
        CheckDlgButton(window, 1009, g_bClearDepthBuffer != 0);
        return 1;
    }
    if (message == WM_COMMAND) {
        switch (LOWORD(wParam)) {
        case IDOK:
            g_bAlphaCompare = IsDlgButtonChecked(window, 1006) == 0;
            g_bDrawCarShadow = IsDlgButtonChecked(window, 1007) == 0;
            g_bInverseSourceAlpha = IsDlgButtonChecked(window, 1008) != 0;
            g_bClearDepthBuffer = IsDlgButtonChecked(window, 1009) != 0;
            EndDialog(window, 1);
            break;
        case IDCANCEL:
            EndDialog(window, 0);
            break;
        case 1002:
            EndDialog(window, -1);
            break;
        }
    }
    return 0;
}

// @ghidraAddress SetVideo: 0x00401dc0
static INT_PTR CALLBACK ChooseMethodDialogProc(HWND window,
                                               UINT message,
                                               WPARAM wParam,
                                               LPARAM lParam) {
    if (message == WM_INITDIALOG) {
        SetWindowLongPtrA(window, DWLP_USER, lParam);
        int selected;
        if (lParam == 2) {
            selected = 1004;
        } else if (lParam == 3) {
            selected = 1005;
        } else {
            selected = g_bReconfigureDisplay ? 1003 : 1004;
        }
        CheckRadioButton(window, 1003, 1005, selected);
        return 1;
    }
    if (message == WM_COMMAND) {
        switch (LOWORD(wParam)) {
        case IDOK:
            if (IsDlgButtonChecked(window, 1005)) {
                EndDialog(window, 3);
            } else if (IsDlgButtonChecked(window, 1004)) {
                EndDialog(window, 2);
            } else {
                EndDialog(window, 1);
            }
            break;
        case IDCANCEL:
            EndDialog(window, 0);
            break;
        case 1002:
            EndDialog(window, -1);
            break;
        }
    }
    return 0;
}

// @ghidraAddress SetVideo: 0x00401ec0
static int GetSelectedListItemData(HWND window) {
    LRESULT selection = SendDlgItemMessageA(window, 1001, CB_GETCURSEL, 0, 0);
    if (selection == CB_ERR) {
        return -1;
    }
    return SendDlgItemMessageA(window, 1001, CB_GETITEMDATA, selection, 0);
}

static int FillVideoSectionList(HWND window, const char *prefix) {
    VideoSelection *selection = (VideoSelection *)GetWindowLongPtrA(window, DWLP_USER);
    if (!selection) {
        return 0;
    }
    LauncherIniSectionIterator *iterator = FindFirstIniSection(g_pVideoDatabase);
    for (int index = 0; index < g_dwVideoSectionCount; ++index) {
        char *section = GetIniSectionName(iterator);
        if (!strncmp(section, prefix, 3)) {
            char text[80];
            int length = (int)strlen(section + 1) - 1;
            if (length >= 79) {
                length = 79;
            }
            // The original copies the closing bracket into the displayed text.
            strncpy(text, section + 3, length);
            text[length] = '\0';
            LRESULT item = SendDlgItemMessageA(window, 1001, CB_ADDSTRING, 0, (LPARAM)text);
            SendDlgItemMessageA(window, 1001, CB_SETITEMDATA, item, index);
            if (selection->selectedIndex == index) {
                SendDlgItemMessageA(window, 1001, CB_SETCURSEL, item, 0);
            }
        }
        iterator = FindNextIniSection(iterator);
    }
    FreeIniSectionIterator(iterator);
    return 1;
}

// @ghidraAddress SetVideo: 0x00401f00
static int FillVideoCardList(HWND window) {
    return FillVideoSectionList(window, "[v:");
}

// @ghidraAddress SetVideo: 0x00402030
static int FillChipsetList(HWND window) {
    return FillVideoSectionList(window, "[c:");
}

static INT_PTR ProcessVideoSelectionDialog(
    HWND window, UINT message, WPARAM wParam, LPARAM lParam, int (*fillList)(HWND)) {
    if (message == WM_INITDIALOG) {
        SetWindowLongPtrA(window, DWLP_USER, lParam);
        return !GetDlgItem(window, 1001) || fillList(window) ? 1 : 0;
    }
    if (message == WM_COMMAND) {
        switch (LOWORD(wParam)) {
        case IDOK: {
            VideoSelection *selection = (VideoSelection *)GetWindowLongPtrA(window, DWLP_USER);
            if (!selection) {
                // EndDialog does not stop the original callback from using its payload.
                EndDialog(window, 0);
            }
            int index = GetSelectedListItemData(window);
            if (index >= 0) {
                selection->selectedIndex = index;
                EndDialog(window, 1);
            } else {
                selection->selectedIndex = selection->initialIndex;
                EndDialog(window, 0);
            }
            break;
        }
        case IDCANCEL:
            EndDialog(window, 0);
            break;
        case 1002:
            EndDialog(window, -1);
            break;
        }
    }
    return 0;
}

// @ghidraAddress SetVideo: 0x00402160
static INT_PTR CALLBACK VideoCardDialogProc(HWND window,
                                            UINT message,
                                            WPARAM wParam,
                                            LPARAM lParam) {
    return ProcessVideoSelectionDialog(window, message, wParam, lParam, FillVideoCardList);
}

// @ghidraAddress SetVideo: 0x00402260
static INT_PTR CALLBACK ChipsetDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    return ProcessVideoSelectionDialog(window, message, wParam, lParam, FillChipsetList);
}

// @ghidraAddress SetVideo: 0x00402360
static LauncherIniSectionIterator *ResolveVdbUseChain(LauncherIniFile *file, const char *section) {
    LauncherIniSectionIterator *iterator = FindIniSection(file, section);
    const char *target;
    while ((target = GetIniSectionValue(iterator, "Use"))) {
        FreeIniSectionIterator(iterator);
        iterator = FindIniSection(file, target);
    }
    return iterator;
}

static void WriteSelectedVideoSection(void) {
    char *section = GetIniSectionHeaderByIndex(g_VideoSelection.selectedIndex, g_pVideoDatabase);
    GameFile *output = CHK_FWriteOpen(g_szIniFilePath, "wt");
    CHK_FPutS("[Video]", output);
    CHK_FPutS("\n", output);
    CHK_FPutS("Card", output);
    CHK_FPutS("=", output);
    CHK_FPutS(section, output);
    CHK_FPutS("\n", output);
    LauncherIniSectionIterator *iterator = ResolveVdbUseChain(g_pVideoDatabase, section);
    LauncherIniFile *file = SelectIniSection(iterator);
    if (file) {
        char *line;
        while ((line = GetNextIniSectionLine(file))) {
            CHK_FPutS(line, output);
            CHK_FPutS("\n", output);
        }
    }
    FreeIniSectionIterator(iterator);
    CHK_FClose(output);
}

static void WriteSymptomSettings(char *buffer) {
    GameFile *output = CHK_FWriteOpen(g_szIniFilePath, "wt");
    CHK_FPutS("[Video]", output);
    CHK_FPutS("\n", output);
    CHK_FPutS("Card", output);
    CHK_FPutS("=", output);
    CHK_FPutS("[Set via symptoms (use Direct3D)]", output);
    CHK_FPutS("\n", output);
    CHK_FPutS("Driver=D3D\n", output);
    sprintf(buffer, "D3DAlphaCompare=%d\n", g_bAlphaCompare);
    CHK_FPutS(buffer, output);
    CHK_FPutS("D3DAlwaysSquareTextures=0\n", output);
    sprintf(buffer, "D3DClearZBuffer=%d\n", g_bClearDepthBuffer);
    CHK_FPutS(buffer, output);
    sprintf(buffer, "D3DDrawCarShadow=%d\n", g_bDrawCarShadow);
    CHK_FPutS(buffer, output);
    CHK_FPutS("D3DWaitCanFlip=0\n", output);
    CHK_FPutS("D3DWaitFlipDone=0\n", output);
    sprintf(buffer, "D3DInvSrcAlpha=%d\n", g_bInverseSourceAlpha);
    CHK_FPutS(buffer, output);
    CHK_FClose(output);
}

/** @ghidraAddress SetVideo: 0x00402480 */
int WINAPI WinMain(HINSTANCE instance,
                   HINSTANCE previousInstance,
                   LPSTR commandLine,
                   int showCommand) {
    char line[256];
    char buffer[1024];
    HWND desktop = GetDesktopWindow();
    ReadInstallDirectoryFromRegistry();
    strcpy(g_szIniFilePath, g_szInstallDir);
    strcat(g_szIniFilePath, "BossRally.ini");
    if (commandLine && strlen(commandLine)) {
        g_bReconfigureDisplay = strstr(commandLine, "+d") != nullptr;
    }
    if (!CHK_FileExists("BossRally.vdb")) {
        sprintf(buffer, "Error: file %s is missing.", "BossRally.vdb");
        MessageBoxA(desktop, buffer, "Boss Rally Display Wizard", MB_OK);
        return 0;
    }
    g_pVideoDatabase = ReadIniFile("BossRally.vdb");
    g_dwVideoSectionCount = CountIniSections(g_pVideoDatabase);
    g_VideoSelection.initialIndex = -1;
    g_VideoSelection.selectedIndex = -1;
    g_VideoSelection.method = 0;
    if (CHK_FileExists(g_szIniFilePath)) {
        GameFile *file = CHK_FReadOpen(g_szIniFilePath);
        while (FileReadLine(line, 256, file)) {
            if (!strncmp(line, "D3DDrawCarShadow=", 17)) {
                g_bDrawCarShadow = atoi(line + 17);
            } else if (!strncmp(line, "D3DAlphaCompare=", 16)) {
                g_bAlphaCompare = atoi(line + 16);
            } else if (!strncmp(line, "D3DClearZBuffer=", 16)) {
                g_bClearDepthBuffer = atoi(line + 16);
            } else if (!strncmp(line, "D3DInvSrcAlpha=", 15)) {
                g_bInverseSourceAlpha = atoi(line + 15);
            }
        }
        CHK_FClose(file);
    }
    if (CHK_FileExists(g_szIniFilePath)) {
        LauncherIniFile *settings = ReadIniFile(g_szIniFilePath);
        LauncherIniSectionIterator *video = FindIniSection(settings, "[Video]");
        const char *card = GetIniSectionValue(video, "Card");
        if (card) {
            LauncherIniSectionIterator *iterator = FindFirstIniSection(g_pVideoDatabase);
            for (int index = 0; index < g_dwVideoSectionCount; ++index) {
                char *section = GetIniSectionName(iterator);
                if (!strcmp(section, card)) {
                    g_VideoSelection.initialIndex = index;
                    g_VideoSelection.selectedIndex = index;
                    if (!strncmp(section, "[c:", 3)) {
                        g_VideoSelection.method = 3;
                    } else if (!strncmp(section, "[v:", 3)) {
                        g_VideoSelection.method = 2;
                    }
                    break;
                }
                iterator = FindNextIniSection(iterator);
            }
            // The original does not release the database iterator from this lookup.
        }
        FreeIniSectionIterator(video);
        FreeIniFile(settings);
    }
welcome:
    if (!DialogBoxParamA(instance,
                         MAKEINTRESOURCEA(g_bReconfigureDisplay ? 103 : 108),
                         desktop,
                         WelcomeDialogProc,
                         0)) {
        return 0;
    }
    int method = g_VideoSelection.method;
    for (;;) {
        method = DialogBoxParamA(instance,
                                 MAKEINTRESOURCEA(g_bReconfigureDisplay ? 104 : 107),
                                 desktop,
                                 ChooseMethodDialogProc,
                                 method);
        switch (method) {
        case -1:
            goto welcome;
        case 0:
            FreeIniFile(g_pVideoDatabase);
            return 0;
        case 1: {
            INT_PTR result =
                DialogBoxParamA(instance, MAKEINTRESOURCEA(106), desktop, SymptomsDialogProc, 0);
            if (result == -1) {
                continue;
            }
            if (result) {
                WriteSymptomSettings(buffer);
            }
            FreeIniFile(g_pVideoDatabase);
            return 0;
        }
        case 2:
        case 3: {
            VideoSelection previous = g_VideoSelection;
            if (g_VideoSelection.method != method) {
                g_VideoSelection.method = 0;
                g_VideoSelection.initialIndex = -1;
                g_VideoSelection.selectedIndex = -1;
            }
            INT_PTR result = DialogBoxParamA(instance,
                                             MAKEINTRESOURCEA(method == 2 ? 102 : 105),
                                             desktop,
                                             method == 2 ? VideoCardDialogProc : ChipsetDialogProc,
                                             (LPARAM)&g_VideoSelection);
            if (result == -1) {
                g_VideoSelection = previous;
                continue;
            }
            if (result) {
                WriteSelectedVideoSection();
            }
            FreeIniFile(g_pVideoDatabase);
            return 0;
        }
        default:
            return 0;
        }
    }
}
