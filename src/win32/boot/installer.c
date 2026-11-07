#include "installer.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <direct.h>

char *g_pszInstallDir;

BOOL EnsureGameNotRunning(void) {
    HWND game = FindWindowA("BossRally", "Boss Rally");
    if (!game) {
        return TRUE;
    }
    DWORD foregroundThread = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    if (GetWindowThreadProcessId(game, NULL) != foregroundThread || IsIconic(game)) {
        game = GetLastActivePopup(game);
        if (IsIconic(game)) {
            ShowWindow(game, SW_RESTORE);
        }
        BringWindowToTop(game);
        SetForegroundWindow(game);
    }
    return FALSE;
}

bool ProbeInstallerCd(int drive) {
    char root[260];
    char label[260];
    sprintf(root, "%C:\\", drive + 0x40);
    if (_chdrive(drive) || GetDriveTypeA(NULL) != DRIVE_CDROM || _chdir("\\")) {
        return false;
    }
    if (!GetVolumeInformationA(root, label, 260, NULL, NULL, NULL, NULL, 0)) {
        return false;
    }
    return strcmp(label, "Boss Rally") == 0;
}

DWORD RunInstallerProcess(const char *directory, const char *application, char *commandLine) {
    STARTUPINFOA startup = {0};
    PROCESS_INFORMATION process = {0};
#ifdef ENABLE_PATCHES
    // A process that does not start reports a failure to the callers, which test for nonzero.
    DWORD exitCode = 1;
#else
    // The original does not initialise the exit code, so a failed CreateProcessA() returns the
    // value the stack held.
    DWORD exitCode;
#endif
    startup.cb = sizeof(startup);
    startup.wShowWindow = SW_SHOWNORMAL;
    if (CreateProcessA(
            application, commandLine, NULL, NULL, FALSE, 0, NULL, directory, &startup, &process)) {
        DWORD status;
        do {
            status = WaitForSingleObject(process.hProcess, 30);
            MSG message;
            while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            }
        } while (status == WAIT_TIMEOUT);
    }
    GetExitCodeProcess(process.hProcess, &exitCode);
    // The original does not close either process handle.
    return exitCode;
}
