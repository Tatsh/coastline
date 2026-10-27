#include "bootdlg.h"

#include <cstdlib>
#include <cstring>

#include <direct.h>

#include "installer.h"

BEGIN_MESSAGE_MAP(CBootDlg, CDialog)
ON_WM_PAINT()
ON_WM_QUERYDRAGICON()
ON_BN_CLICKED(1001, OnBnClickedInstallBossRally)
ON_BN_CLICKED(1002, OnBnClickedExit)
ON_BN_CLICKED(1000, OnBnClickedInstallDirectX)
ON_BN_CLICKED(1003, OnBnClickedRunDisplayWizard)
ON_BN_CLICKED(1004, OnBnClickedInstallDirectXMedia)
END_MESSAGE_MAP()

CBootDlg::CBootDlg(CWnd *parent) : CDialog(102, parent) {
    AfxGetModuleState();
    icon = ::LoadIconA(AfxGetResourceHandle(), MAKEINTRESOURCEA(128));
}

CBootDlg::~CBootDlg() = default;

void CBootDlg::DoDataExchange(CDataExchange *exchange) {
    DDX_Control(exchange, 1004, installDirectXMedia);
    DDX_Control(exchange, 1003, displayWizard);
    DDX_Control(exchange, 1000, installDirectX);
    DDX_Control(exchange, 1001, installBossRally);
}

BOOL CBootDlg::OnInitDialog() {
    CDialog::OnInitDialog();
    ::SendMessageA(GetSafeHwnd(), WM_SETICON, ICON_BIG, (LPARAM)icon);
    ::SendMessageA(GetSafeHwnd(), WM_SETICON, ICON_SMALL, (LPARAM)icon);
    if (IsBossRallyInstalled()) {
        installBossRally.SetWindowText("Run Boss Rally");
    } else {
        installBossRally.SetWindowText("Install Boss Rally");
        installDirectX.ModifyStyle(WS_VISIBLE, 0, 0);
        displayWizard.ModifyStyle(WS_VISIBLE, 0, 0);
        installDirectXMedia.ModifyStyle(WS_VISIBLE, 0, 0);
    }
    return TRUE;
}

void CBootDlg::OnPaint() {
    if (::IsIconic(GetSafeHwnd())) {
        CPaintDC dc(this);
        ::SendMessageA(GetSafeHwnd(), WM_ICONERASEBKGND, (WPARAM)dc.GetSafeHdc(), 0);
        int width = GetSystemMetrics(SM_CXICON);
        int height = GetSystemMetrics(SM_CYICON);
        RECT rect;
        ::GetClientRect(GetSafeHwnd(), &rect);
        int x = (rect.right - rect.left - width + 1) / 2;
        int y = (rect.bottom - rect.top - height + 1) / 2;
        ::DrawIcon(dc.GetSafeHdc(), x, y, icon);
    } else {
        CDialog::OnPaint();
    }
}

HCURSOR CBootDlg::OnQueryDragIcon() {
    return icon;
}

void CBootDlg::OnBnClickedInstallBossRally() {
    EndDialog(1001);
}

void CBootDlg::OnBnClickedExit() {
    EndDialog(1002);
}

void CBootDlg::OnBnClickedInstallDirectX() {
    EndDialog(1000);
}

void CBootDlg::OnBnClickedRunDisplayWizard() {
    EndDialog(1003);
}

bool CBootDlg::IsBossRallyInstalled() {
    HKEY key;
    if (RegOpenKeyExA(
            HKEY_LOCAL_MACHINE, "Software\\SouthPeak Interactive\\Boss Rally", 0, KEY_READ, &key) !=
        ERROR_SUCCESS) {
        return false;
    }
    CFile file;
    DWORD bytes = 260;
    DWORD type;
    if (RegQueryValueExA(key, "Directory", nullptr, &type, nullptr, &bytes) != ERROR_SUCCESS ||
        type != REG_SZ) {
        RegCloseKey(key);
        return false;
    }
    // Directory storage remains published when later checks fail.
    g_pszInstallDir = static_cast<char *>(std::malloc(bytes));
    char *application = static_cast<char *>(std::malloc(bytes + std::strlen("\\brally.exe")));
    if (!g_pszInstallDir) {
        RegCloseKey(key);
        return false;
    }
    if (RegQueryValueExA(
            key, "Directory", nullptr, &type, reinterpret_cast<BYTE *>(g_pszInstallDir), &bytes) !=
        ERROR_SUCCESS) {
        RegCloseKey(key);
        return false;
    }
    RegCloseKey(key);
    if (_chdir(g_pszInstallDir) == -1) {
        return false;
    }
    std::strcpy(application, g_pszInstallDir);
    std::strcat(application, "\\brally.exe");
    // The original does not free the temporary executable path.
    if (!file.Open(application, CFile::modeRead, nullptr)) {
        return false;
    }
    file.Close();
    return true;
}

void CBootDlg::OnBnClickedInstallDirectXMedia() {
    EndDialog(1004);
}
