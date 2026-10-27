#ifndef COASTLINE_LAUNCHER_BOOTDLG_H
#define COASTLINE_LAUNCHER_BOOTDLG_H

#include <afxwin.h>

class CBootDlg : public CDialog {
public:
    /** @ghidraAddress Boot: 0x00401860 */
    explicit CBootDlg(CWnd *parent = nullptr);
    /** @ghidraAddress Boot: 0x00401510 */
    ~CBootDlg() override;

private:
    /** @ghidraAddress Boot: 0x00401950 */
    void DoDataExchange(CDataExchange *exchange) override;
    /** @ghidraAddress Boot: 0x004019c0 */
    BOOL OnInitDialog() override;
    /** @ghidraAddress Boot: 0x00401a70 */
    afx_msg void OnPaint();
    /** @ghidraAddress Boot: 0x00401b70 */
    afx_msg HCURSOR OnQueryDragIcon();
    /** @ghidraAddress Boot: 0x00401b80 */
    afx_msg void OnBnClickedInstallBossRally();
    /** @ghidraAddress Boot: 0x00401b90 */
    afx_msg void OnBnClickedExit();
    /** @ghidraAddress Boot: 0x00401ba0 */
    afx_msg void OnBnClickedInstallDirectX();
    /** @ghidraAddress Boot: 0x00401bb0 */
    afx_msg void OnBnClickedRunDisplayWizard();
    /** @ghidraAddress Boot: 0x00401bc0 */
    bool IsBossRallyInstalled();
    /** @ghidraAddress Boot: 0x00401d80 */
    afx_msg void OnBnClickedInstallDirectXMedia();

    CButton installDirectXMedia;
    CButton displayWizard;
    CButton installDirectX;
    CButton installBossRally;
    HICON icon;

    DECLARE_MESSAGE_MAP()
};

#endif
