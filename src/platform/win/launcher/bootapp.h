#ifndef COASTLINE_LAUNCHER_BOOTAPP_H
#define COASTLINE_LAUNCHER_BOOTAPP_H

#include <afxwin.h>

class CBootApp : public CWinApp {
public:
    /** @ghidraAddress Boot: 0x00401010 */
    CBootApp();
    /** @ghidraAddress Boot: 0x00401050 */
    ~CBootApp() override;

private:
    /** @ghidraAddress Boot: 0x00401120 */
    BOOL InitInstance() override;
    /** @ghidraAddress Boot: 0x00401590 */
    DWORD RunProcessAndWait(const char *directory, const char *application, char *commandLine);
    /** @ghidraAddress Boot: 0x004016e0 */
    [[noreturn]] void FatalErrorExit(const char *operation);
    /** @ghidraAddress Boot: 0x00401770 */
    bool IsBossRallyCdInDrive(int drive);

    DECLARE_MESSAGE_MAP()
};

#endif
