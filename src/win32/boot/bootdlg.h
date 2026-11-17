#ifndef COASTLINE_LAUNCHER_BOOTDLG_H
#define COASTLINE_LAUNCHER_BOOTDLG_H

/**
 * @file
 * The dialog of the Boss Rally disc launcher.
 */

#include <afxwin.h>

/** The launcher dialog for installation and display setup. */
class CBootDlg : public CDialog {
public:
    /** Load the launcher icon and associate the dialog with @p parent.
     * @param parent The owner window, or nullptr for the main window of the application.
     * @ghidraAddress Boot: 0x00401860 */
    explicit CBootDlg(CWnd *parent = nullptr);
    /** Release the launcher dialog.
     * @ghidraAddress Boot: 0x00401510 */
    ~CBootDlg() override;

private:
    // Bind the four buttons to their controls, 1004, 1003, 1000, and 1001.
    // @ghidraAddress Boot: 0x00401950
    void DoDataExchange(CDataExchange *exchange) override;
    // Set the icon of the window and label the first button `Run Boss Rally` when the game is
    // installed. Otherwise the button reads `Install Boss Rally` and the other setup buttons are
    // hidden. Returns TRUE to let the dialog set the focus.
    // @ghidraAddress Boot: 0x004019c0
    BOOL OnInitDialog() override;
    // Draw the icon in the middle of the client area while the dialog is minimised, or paint the
    // dialog normally otherwise.
    // @ghidraAddress Boot: 0x00401a70
    afx_msg void OnPaint();
    // Return the launcher icon as the cursor shown while the minimised dialog is dragged.
    // @ghidraAddress Boot: 0x00401b70
    afx_msg HCURSOR OnQueryDragIcon();
    // End the dialog with 1001, to run or install Boss Rally.
    // @ghidraAddress Boot: 0x00401b80
    afx_msg void OnBnClickedInstallBossRally();
    // End the dialog with 1002, to exit the launcher.
    // @ghidraAddress Boot: 0x00401b90
    afx_msg void OnBnClickedExit();
    // End the dialog with 1000, to run DirectX setup from the disc.
    // @ghidraAddress Boot: 0x00401ba0
    afx_msg void OnBnClickedInstallDirectX();
    // End the dialog with 1003, to run `setvideo.exe` in the installation directory.
    // @ghidraAddress Boot: 0x00401bb0
    afx_msg void OnBnClickedRunDisplayWizard();
    // Read the installation directory from the registry into g_pszInstallDir, make it the current
    // directory, and check that `brally.exe` opens there. The directory remains allocated when a
    // later check fails.
    // @ghidraAddress Boot: 0x00401bc0
    bool IsBossRallyInstalled();
    // End the dialog with 1004, to run DirectX Media setup from the disc.
    // @ghidraAddress Boot: 0x00401d80
    afx_msg void OnBnClickedInstallDirectXMedia();

    CButton installDirectXMedia; // The DirectX Media setup button, control 1004.
    CButton displayWizard;       // The display wizard button, control 1003.
    CButton installDirectX;      // The DirectX setup button, control 1000.
    CButton installBossRally;    // The button that runs or installs the game, control 1001.
    HICON icon;                  // The launcher icon, resource 128.

    DECLARE_MESSAGE_MAP()
};

#endif /* COASTLINE_LAUNCHER_BOOTDLG_H */
