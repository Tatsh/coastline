#ifndef COASTLINE_LAUNCHER_BOOTAPP_H
#define COASTLINE_LAUNCHER_BOOTAPP_H

/**
 * @file
 * The application object of the Boss Rally disc launcher.
 */

#include <afxwin.h>

/** The Boss Rally disc launcher and its process lifecycle. */
class CBootApp : public CWinApp {
public:
    /** Initialise the launcher application.
     * @ghidraAddress Boot: 0x00401010 */
    CBootApp();
    /** Release the launcher application.
     * @ghidraAddress Boot: 0x00401050 */
    ~CBootApp() override;

private:
    // Show the launcher dialog repeatedly and run the setup program, the display wizard, or the
    // game the user selects. A running game is brought to the foreground instead. Returns FALSE
    // to end the application after the dialog closes.
    // @ghidraAddress Boot: 0x00401120
    BOOL InitInstance() override;
    // Run a program and wait for it to exit, returning its exit code. A non-zero code is treated
    // as a failure, and a NULL directory runs the program in the current directory. The original
    // formats a log message here and discards it.
    // @ghidraAddress Boot: 0x00401590
    DWORD RunProcessAndWait(const char *directory, const char *application, char *commandLine);
    // Show `Error while` followed by the operation (such as `running Display Wizard`) in a message
    // box and exit the process.
    // @ghidraAddress Boot: 0x004016e0
    [[noreturn]] void FatalErrorExit(const char *operation);
    // Check whether a drive is a CD-ROM drive with a volume labelled `Boss Rally`. The drive
    // number is 1 for A, and the drive becomes the current drive.
    // @ghidraAddress Boot: 0x00401770
    bool IsBossRallyCdInDrive(int drive);

    DECLARE_MESSAGE_MAP()
};

#endif /* COASTLINE_LAUNCHER_BOOTAPP_H */
