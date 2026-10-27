#include "bootapp.h"

#include <cstdio>
#include <cstdlib>

#include <direct.h>

#include "bootdlg.h"
#include "installer.h"

namespace {
// @ghidraAddress Boot: 0x00421cd8
CBootApp theApp;
// @ghidraAddress Boot: 0x00421d9c
char g_szEmptyCmdLine[] = "";
// @ghidraAddress Boot: 0x0041e120
char g_szDirectXMediaArgs[] = " -NQ -id:bossgame";
} // namespace

BEGIN_MESSAGE_MAP(CBootApp, CWinApp)
END_MESSAGE_MAP()

CBootApp::CBootApp() : CWinApp(nullptr) {
}

CBootApp::~CBootApp() = default;

DWORD CBootApp::RunProcessAndWait(const char *directory,
                                  const char *application,
                                  char *commandLine) {
    CString message;
    message.Format(
        "about to run \"%s\" in directory \"%s\" with \"%s\"", application, directory, commandLine);
    return RunInstallerProcess(directory, application, commandLine);
}

[[noreturn]] void CBootApp::FatalErrorExit(const char *operation) {
    CString message;
    message.GetBuffer(255);
    message = "Error while ";
    message += operation;
    AfxMessageBox(message, MB_ICONHAND, 0);
    std::exit(0);
}

bool CBootApp::IsBossRallyCdInDrive(int drive) {
    return ProbeInstallerCd(drive);
}

BOOL CBootApp::InitInstance() {
    CBootDlg dialog;
    m_pMainWnd = nullptr;
    g_pszInstallDir = nullptr;
    if (!EnsureGameNotRunning()) {
        return FALSE;
    }
    for (;;) {
        m_pMainWnd = nullptr;
        INT_PTR result = dialog.DoModal();
        switch (result) {
        case 1000:
        case 1004: {
            int drive;
            for (drive = 3; drive <= 26; ++drive) {
                if (!_chdrive(drive) && GetDriveTypeA(nullptr) == DRIVE_CDROM && !_chdir("\\") &&
                    IsBossRallyCdInDrive(drive)) {
                    break;
                }
            }
            if (drive > 26) {
                FatalErrorExit(result == 1000 ? "finding DirectX Setup" :
                                                "finding DirectXMedia Setup");
            }
            char application[260];
            if (result == 1000) {
                std::sprintf(application, "%c:\\directx\\DXSETUP.EXE", drive + 0x40);
                if (RunProcessAndWait(nullptr, application, g_szEmptyCmdLine)) {
                    FatalErrorExit("running DirectX Setup");
                }
            } else {
                std::sprintf(application, "%c:\\directx\\DXMEDIA.exe", drive + 0x40);
                if (RunProcessAndWait(nullptr, application, g_szDirectXMediaArgs)) {
                    FatalErrorExit("running DirectXMedia");
                }
            }
            break;
        }
        case 1003:
            if (RunProcessAndWait(g_pszInstallDir, "setvideo.exe", g_szEmptyCmdLine)) {
                FatalErrorExit("running Display Wizard");
            }
            break;
        case 1001:
            if (g_pszInstallDir) {
                if (_chdir(g_pszInstallDir)) {
                    FatalErrorExit("running Boss Rally");
                }
                if (WinExec("bossrally.exe", SW_SHOWMAXIMIZED) <= 31) {
                    FatalErrorExit("running Boss Rally");
                }
            } else if (WinExec(".\\setup.exe", SW_SHOWMAXIMIZED) <= 31) {
                FatalErrorExit("running Setup.exe");
            }
            return FALSE;
        default:
            return FALSE;
        }
    }
}
