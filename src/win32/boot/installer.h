#ifndef COASTLINE_LAUNCHER_INSTALLER_H
#define COASTLINE_LAUNCHER_INSTALLER_H

/**
 * @file
 * The process and disc checks of the Boss Rally disc launcher.
 */

#include <stdbool.h>

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Bring an existing Boss Rally window to the foreground if the game is already running.
 * @return False when a game window was found; true otherwise.
 * @ghidraAddress Boot: 0x004010a0 */
BOOL EnsureGameNotRunning(void);

/**
 * Make a drive current and check whether it is a CD-ROM drive with a volume labelled
 * `Boss Rally`. The current drive and directory remain changed afterwards.
 *
 * @param drive The drive number, 1 for A.
 * @return Whether the drive has the Boss Rally installation disc.
 */
bool ProbeInstallerCd(int drive);
/**
 * Start a program and pump the window messages of the thread every 30 milliseconds until it exits.
 * The process handles are not closed.
 *
 * @param directory The working directory of the program, or NULL for the current directory.
 * @param application The path of the executable.
 * @param commandLine The command line passed to the program.
 * @return The exit code of the program. When the program does not start, patched builds return 1
 *     and the original returns an uninitialised value.
 */
DWORD RunInstallerProcess(const char *directory, const char *application, char *commandLine);

/** The installation directory read from the Boss Rally registry entry.
 * @ghidraAddress Boot: 0x00421d98 */
extern char *g_pszInstallDir;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_LAUNCHER_INSTALLER_H */
